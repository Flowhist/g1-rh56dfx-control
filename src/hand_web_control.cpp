#include "rh56/readonly.hpp"
#include "rh56/writer.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace {

constexpr const char* kRightDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if01-port0";
constexpr const char* kLeftDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if02-port0";
constexpr std::size_t kMaximumRequestBytes = 64 * 1024;
constexpr int16_t kMaximumGripCurrentMa = 300;
constexpr uint8_t kMaximumGripTemperatureC = 60;

volatile std::sig_atomic_t stop_requested = 0;
int server_fd = -1;

void StopHandler(int)
{
    stop_requested = 1;
    if (server_fd >= 0)
        close(server_fd);
}

struct AppConfig
{
    std::string bind_address{"0.0.0.0"};
    uint16_t port{8080};
    std::string hand{"both"};
    bool execute{false};
    std::filesystem::path assets;
    std::filesystem::path poses_file;
    std::string session_token;
};

void PrintAccessUrls(const AppConfig& config)
{
    if (config.bind_address != "0.0.0.0") {
        std::cout << "Hand control UI: http://" << config.bind_address << ':'
                  << config.port << '\n';
        return;
    }

    std::cout << "Hand control UI (LAN):\n";
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0) {
        std::cout << "  http://<this-device-ip>:" << config.port << '\n';
        return;
    }
    bool printed = false;
    for (ifaddrs* item = interfaces; item; item = item->ifa_next) {
        if (!item->ifa_addr || item->ifa_addr->sa_family != AF_INET ||
            !(item->ifa_flags & IFF_UP) || (item->ifa_flags & IFF_LOOPBACK))
            continue;
        const std::string interface_name = item->ifa_name;
        if (interface_name.rfind("docker", 0) == 0 ||
            interface_name.rfind("veth", 0) == 0 ||
            interface_name.rfind("br-", 0) == 0 ||
            interface_name.rfind("virbr", 0) == 0)
            continue;
        char address[INET_ADDRSTRLEN]{};
        const auto* ipv4 = reinterpret_cast<sockaddr_in*>(item->ifa_addr);
        if (inet_ntop(AF_INET, &ipv4->sin_addr, address, sizeof(address))) {
            std::cout << "  http://" << address << ':' << config.port
                      << "  (" << item->ifa_name << ")\n";
            printed = true;
        }
    }
    freeifaddrs(interfaces);
    if (!printed)
        std::cout << "  http://<this-device-ip>:" << config.port << '\n';
}

std::string RandomToken()
{
    std::random_device random;
    std::ostringstream token;
    token << std::hex << std::setfill('0');
    for (int i = 0; i < 4; ++i)
        token << std::setw(8) << random();
    return token.str();
}

std::filesystem::path ProjectRoot(const char* executable)
{
    return std::filesystem::weakly_canonical(executable)
        .parent_path().parent_path().parent_path();
}

class PidFile
{
public:
    explicit PidFile(const std::filesystem::path& root)
        : path_(root / "run" / "hand_controller.pid")
    {
        std::filesystem::create_directories(path_.parent_path());
        if (std::filesystem::exists(path_)) {
            std::ifstream input(path_);
            int existing_pid = 0;
            if ((input >> existing_pid) && existing_pid > 0 &&
                std::filesystem::exists("/proc/" + std::to_string(existing_pid)))
                throw std::runtime_error(
                    "Another hand controller is active (PID " +
                    std::to_string(existing_pid) + ")");
        }
        std::ofstream(path_, std::ios::trunc) << getpid() << '\n';
    }

    ~PidFile()
    {
        std::ifstream input(path_);
        int owner = 0;
        if ((input >> owner) && owner == getpid()) {
            std::error_code error;
            std::filesystem::remove(path_, error);
        }
    }

private:
    std::filesystem::path path_;
};

struct HttpRequest
{
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

bool SendAll(int fd, const std::string& data)
{
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t count = send(fd, data.data() + sent, data.size() - sent,
                                   MSG_NOSIGNAL);
        if (count <= 0) {
            if (count < 0 && errno == EINTR)
                continue;
            return false;
        }
        sent += static_cast<std::size_t>(count);
    }
    return true;
}

void Respond(int fd, int status, const std::string& reason,
             const std::string& content_type, const std::string& body)
{
    std::ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "Connection: close\r\n\r\n"
             << body;
    SendAll(fd, response.str());
}

std::optional<HttpRequest> ReadRequest(int fd)
{
    std::string raw;
    std::array<char, 4096> buffer{};
    std::size_t header_end = std::string::npos;
    std::size_t content_length = 0;

    while (raw.size() < kMaximumRequestBytes) {
        const ssize_t count = recv(fd, buffer.data(), buffer.size(), 0);
        if (count <= 0)
            return std::nullopt;
        raw.append(buffer.data(), static_cast<std::size_t>(count));
        header_end = raw.find("\r\n\r\n");
        if (header_end == std::string::npos)
            continue;

        const std::string headers = raw.substr(0, header_end);
        const std::string key = "Content-Length:";
        const auto position = headers.find(key);
        if (position != std::string::npos) {
            const auto start = headers.find_first_not_of(" \t", position + key.size());
            try {
                content_length = std::stoul(headers.substr(start));
            } catch (...) {
                return std::nullopt;
            }
        }
        if (content_length > kMaximumRequestBytes ||
            raw.size() < header_end + 4 + content_length)
            continue;
        break;
    }
    if (header_end == std::string::npos ||
        raw.size() < header_end + 4 + content_length)
        return std::nullopt;

    HttpRequest request;
    std::istringstream stream(raw.substr(0, header_end));
    std::string line;
    if (!std::getline(stream, line))
        return std::nullopt;
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    std::istringstream request_line(line);
    std::string version;
    if (!(request_line >> request.method >> request.path >> version))
        return std::nullopt;
    request.path = request.path.substr(0, request.path.find('?'));

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        const auto colon = line.find(':');
        if (colon != std::string::npos)
            request.headers[line.substr(0, colon)] = line.substr(colon + 1);
    }
    request.body = raw.substr(header_end + 4, content_length);
    return request;
}

std::string UrlDecode(const std::string& value)
{
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            decoded.push_back(' ');
        } else if (value[i] == '%' && i + 2 < value.size()) {
            char* end = nullptr;
            const std::string hex = value.substr(i + 1, 2);
            const long byte = std::strtol(hex.c_str(), &end, 16);
            if (end == hex.c_str() + 2) {
                decoded.push_back(static_cast<char>(byte));
                i += 2;
            } else {
                decoded.push_back(value[i]);
            }
        } else {
            decoded.push_back(value[i]);
        }
    }
    return decoded;
}

std::map<std::string, std::string> ParseForm(const std::string& body)
{
    std::map<std::string, std::string> fields;
    std::size_t start = 0;
    while (start <= body.size()) {
        const auto end = body.find('&', start);
        const std::string pair = body.substr(start, end - start);
        const auto equals = pair.find('=');
        if (equals != std::string::npos)
            fields[UrlDecode(pair.substr(0, equals))] =
                UrlDecode(pair.substr(equals + 1));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return fields;
}

std::string JsonEscape(const std::string& value)
{
    std::ostringstream out;
    for (const unsigned char byte : value) {
        switch (byte) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (byte < 0x20)
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(byte);
            else
                out << byte;
        }
    }
    return out.str();
}

std::string PositionJson(const rh56::Position& position)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << '[';
    for (std::size_t i = 0; i < position.size(); ++i) {
        if (i)
            out << ',';
        out << position[i];
    }
    return out.str() + ']';
}

bool HandEnabled(const AppConfig& config, const std::string& hand)
{
    return hand == config.hand || config.hand == "both";
}

const char* HandDevice(const std::string& hand)
{
    return hand == "right" ? kRightDevice : kLeftDevice;
}

std::string ReadAsset(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot read asset " + path.string());
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

void ServeStatus(int fd, const AppConfig& config)
{
    std::ostringstream json;
    json << "{\"execute\":" << (config.execute ? "true" : "false")
         << ",\"selection\":\"" << config.hand << "\",\"token\":\""
         << config.session_token << "\",\"positions\":{";
    bool first = true;
    std::vector<std::string> errors;

    if (config.execute) {
        for (const std::string hand : {"right", "left"}) {
            if (!HandEnabled(config, hand))
                continue;
            try {
                rh56::Position position{};
                Rh56Readonly reader(HandDevice(hand));
                if (!reader.ReadPosition(position))
                    throw std::runtime_error("position read timed out");
                if (!first)
                    json << ',';
                first = false;
                json << '"' << hand << "\":" << PositionJson(position);
            } catch (const std::exception& error) {
                errors.push_back(hand + ": " + error.what());
            }
        }
    }
    json << "},\"errors\":[";
    for (std::size_t i = 0; i < errors.size(); ++i) {
        if (i)
            json << ',';
        json << '"' << JsonEscape(errors[i]) << '"';
    }
    json << "]}";
    Respond(fd, 200, "OK", "application/json; charset=utf-8", json.str());
}

template <typename Values>
std::string RegisterValuesJson(const Values& values)
{
    std::ostringstream out;
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i)
            out << ',';
        out << +values[i];
    }
    return out.str() + ']';
}

void ServeRegisters(int fd, const AppConfig& config, const std::string& hand)
{
    if (!config.execute) {
        Respond(fd, 403, "Forbidden", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Register monitoring requires --execute\"}");
        return;
    }
    if ((hand != "left" && hand != "right") || !HandEnabled(config, hand)) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Hand is not enabled\"}");
        return;
    }

    try {
        rh56::RawValues position{};
        rh56::RawValues force{};
        rh56::RawValues current{};
        rh56::RawValues force_limit{};
        rh56::RawValues current_limit{};
        rh56::ByteValues error{};
        rh56::ByteValues status{};
        rh56::ByteValues temperature{};
        Rh56Readonly reader(HandDevice(hand));
        const bool ok =
            reader.ReadWords(0x060A, position) &&
            reader.ReadWords(0x062E, force) &&
            reader.ReadWords(0x063A, current) &&
            reader.ReadWords(0x05DA, force_limit) &&
            reader.ReadWords(0x03FC, current_limit) &&
            reader.ReadBytes(0x0646, error) &&
            reader.ReadBytes(0x064C, status) &&
            reader.ReadBytes(0x0652, temperature);
        if (!ok)
            throw std::runtime_error("register read timed out or checksum failed");

        std::ostringstream json;
        json << "{\"ok\":true,\"hand\":\"" << hand
             << "\",\"position\":" << RegisterValuesJson(position)
             << ",\"force\":" << RegisterValuesJson(force)
             << ",\"current\":" << RegisterValuesJson(current)
             << ",\"force_limit\":" << RegisterValuesJson(force_limit)
             << ",\"current_limit\":" << RegisterValuesJson(current_limit)
             << ",\"error\":" << RegisterValuesJson(error)
             << ",\"status\":" << RegisterValuesJson(status)
             << ",\"temperature\":" << RegisterValuesJson(temperature)
             << '}';
        Respond(fd, 200, "OK", "application/json; charset=utf-8", json.str());
    } catch (const std::exception& error) {
        const std::string body = "{\"ok\":false,\"error\":\"" +
                                 JsonEscape(error.what()) + "\"}";
        Respond(fd, 503, "Service Unavailable",
                "application/json; charset=utf-8", body);
    }
}

std::optional<rh56::Position> ParseValues(const std::string& text);

std::optional<int> ParseInteger(const std::string& text, int minimum,
                                int maximum)
{
    try {
        std::size_t used = 0;
        const int value = std::stoi(text, &used);
        if (used != text.size() || value < minimum || value > maximum)
            return std::nullopt;
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

void ServeGrip(int fd, const AppConfig& config, const HttpRequest& request)
{
    if (!config.execute) {
        Respond(fd, 403, "Forbidden", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Server is in preview mode\"}");
        return;
    }
    const auto fields = ParseForm(request.body);
    const auto hand = fields.find("hand");
    const auto token = fields.find("token");
    const auto confirm = fields.find("confirm");
    const auto force = fields.find("force");
    const auto current = fields.find("current");
    const auto values = fields.find("values");
    if (hand == fields.end() || token == fields.end() ||
        confirm == fields.end() || force == fields.end() ||
        current == fields.end() || values == fields.end() ||
        token->second != config.session_token || confirm->second != "grip" ||
        (hand->second != "left" && hand->second != "right") ||
        !HandEnabled(config, hand->second)) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Invalid grip request\"}");
        return;
    }
    const auto force_grams = ParseInteger(force->second, 50, 1000);
    const auto current_ma =
        ParseInteger(current->second, 50, kMaximumGripCurrentMa);
    const auto targets = ParseValues(values->second);
    if (!force_grams || !current_ma || !targets) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Grip force, current, or targets are outside the safe range\"}");
        return;
    }

    try {
        rh56::RawValues measured_current{};
        rh56::ByteValues error{};
        rh56::ByteValues temperature{};
        {
            Rh56Readonly reader(HandDevice(hand->second));
            if (!reader.ReadWords(0x063A, measured_current) ||
                !reader.ReadBytes(0x0646, error) ||
                !reader.ReadBytes(0x0652, temperature))
                throw std::runtime_error("pre-grip safety telemetry read failed");
        }
        for (std::size_t joint = 0; joint < 6; ++joint) {
            if (error[joint] != 0)
                throw std::runtime_error("joint " + std::to_string(joint) +
                                         " has error bits " +
                                         std::to_string(error[joint]));
            if (temperature[joint] >= kMaximumGripTemperatureC)
                throw std::runtime_error("joint " + std::to_string(joint) +
                                         " is too hot");
            if (std::abs(static_cast<int>(measured_current[joint])) >=
                kMaximumGripCurrentMa)
                throw std::runtime_error("joint " + std::to_string(joint) +
                                         " current is already at the safety limit");
        }

        rh56::RawValues force_limits{};
        rh56::RawValues current_limits{};
        force_limits.fill(static_cast<int16_t>(*force_grams));
        current_limits.fill(static_cast<int16_t>(*current_ma));
        rh56::JointMask all{};
        all.fill(true);
        Rh56Writer writer(HandDevice(hand->second));
        if (!writer.WriteWords(0x03FC, current_limits, 1500) ||
            !writer.WriteWords(0x05DA, force_limits, 1000) ||
            !writer.WritePosition(*targets, all))
            throw std::runtime_error("grip configuration or target was not acknowledged");
        Respond(fd, 200, "OK", "application/json; charset=utf-8",
                "{\"ok\":true,\"force\":" + std::to_string(*force_grams) +
                    ",\"current\":" + std::to_string(*current_ma) + '}');
    } catch (const std::exception& error_message) {
        Respond(fd, 503, "Service Unavailable",
                "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"" +
                    JsonEscape(error_message.what()) + "\"}");
    }
}

void ServeClearFault(int fd, const AppConfig& config,
                     const HttpRequest& request)
{
    if (!config.execute) {
        Respond(fd, 403, "Forbidden", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Server is in preview mode\"}");
        return;
    }
    const auto fields = ParseForm(request.body);
    const auto hand = fields.find("hand");
    const auto joint = fields.find("joint");
    const auto token = fields.find("token");
    const auto confirm = fields.find("confirm");
    if (hand == fields.end() || joint == fields.end() ||
        token == fields.end() || confirm == fields.end() ||
        token->second != config.session_token || confirm->second != "clear-fault" ||
        (hand->second != "left" && hand->second != "right") ||
        !HandEnabled(config, hand->second)) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Invalid clear-fault request\"}");
        return;
    }
    const auto joint_index = ParseInteger(joint->second, 0, 5);
    if (!joint_index) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Invalid joint index\"}");
        return;
    }

    try {
        rh56::Position position{};
        rh56::ByteValues error{};
        rh56::ByteValues status{};
        rh56::ByteValues temperature{};
        {
            Rh56Readonly reader(HandDevice(hand->second));
            if (!reader.ReadPosition(position) ||
                !reader.ReadBytes(0x0646, error) ||
                !reader.ReadBytes(0x064C, status) ||
                !reader.ReadBytes(0x0652, temperature))
                throw std::runtime_error("pre-clear telemetry read failed");
        }
        if (error[*joint_index] == 0 && status[*joint_index] != 7) {
            Respond(fd, 200, "OK", "application/json; charset=utf-8",
                    "{\"ok\":true,\"cleared\":false,\"message\":\"No fault is present\"}");
            return;
        }
        for (std::size_t i = 0; i < error.size(); ++i)
            if (error[i] != 0 && temperature[i] >= kMaximumGripTemperatureC)
                throw std::runtime_error(
                    "faulted joint " + std::to_string(i) +
                    " is too hot; wait for the overtemperature fault to auto-clear");

        rh56::JointMask recover{};
        for (std::size_t i = 0; i < error.size(); ++i)
            recover[i] = error[i] != 0 || status[i] == 7;
        {
            Rh56Writer writer(HandDevice(hand->second));
            if (!writer.WritePosition(position, recover))
                throw std::runtime_error("failed to hold faulted joints at current feedback");
            if (error[*joint_index] != 0 && !writer.ClearErrors())
                throw std::runtime_error("clear-error register write was not acknowledged");
            if (!writer.WritePosition(position, recover))
                throw std::runtime_error("failed to re-enable current-position hold");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        {
            Rh56Readonly reader(HandDevice(hand->second));
            if (!reader.ReadBytes(0x0646, error) ||
                !reader.ReadBytes(0x064C, status) ||
                !reader.ReadBytes(0x0652, temperature))
                throw std::runtime_error("post-clear telemetry read failed");
        }
        const bool cleared =
            error[*joint_index] == 0 && status[*joint_index] != 7;
        const std::string body =
            "{\"ok\":" + std::string(cleared ? "true" : "false") +
            ",\"cleared\":" + std::string(cleared ? "true" : "false") +
            ",\"error\":" + RegisterValuesJson(error) +
            ",\"status\":" + RegisterValuesJson(status) +
            ",\"temperature\":" + RegisterValuesJson(temperature) +
            (cleared ? "}" : ",\"message\":\"Fault remains after clear\"}");
        Respond(fd, cleared ? 200 : 409, cleared ? "OK" : "Conflict",
                "application/json; charset=utf-8", body);
    } catch (const std::exception& error_message) {
        Respond(fd, 503, "Service Unavailable",
                "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"" +
                    JsonEscape(error_message.what()) + "\"}");
    }
}

std::optional<float> ParseQ(const std::string& text)
{
    try {
        std::size_t used = 0;
        const float value = std::stof(text, &used);
        if (used != text.size() || !std::isfinite(value) || value < 0.0f ||
            value > 1.0f)
            return std::nullopt;
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<rh56::Position> ParseValues(const std::string& text)
{
    rh56::Position values{};
    std::istringstream input(text);
    std::string item;
    std::size_t count = 0;
    while (std::getline(input, item, ',')) {
        if (count >= values.size())
            return std::nullopt;
        const auto value = ParseQ(item);
        if (!value)
            return std::nullopt;
        values[count++] = *value;
    }
    if (count != values.size())
        return std::nullopt;
    return values;
}

struct SavedPose
{
    std::string id;
    int64_t created_ms{0};
    std::string name;
    rh56::Position right{};
    rh56::Position left{};
};

std::string HexEncode(const std::string& value)
{
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string encoded;
    encoded.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        encoded.push_back(digits[byte >> 4]);
        encoded.push_back(digits[byte & 0x0F]);
    }
    return encoded;
}

std::optional<std::string> HexDecode(const std::string& value)
{
    if (value.size() % 2 != 0)
        return std::nullopt;
    std::string decoded;
    decoded.reserve(value.size() / 2);
    for (std::size_t i = 0; i < value.size(); i += 2) {
        char* end = nullptr;
        const std::string pair = value.substr(i, 2);
        const long byte = std::strtol(pair.c_str(), &end, 16);
        if (end != pair.c_str() + 2)
            return std::nullopt;
        decoded.push_back(static_cast<char>(byte));
    }
    return decoded;
}

std::vector<std::string> SplitTabs(const std::string& line)
{
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (start <= line.size()) {
        const auto end = line.find('\t', start);
        fields.push_back(line.substr(start, end - start));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return fields;
}

std::vector<SavedPose> LoadPoses(const std::filesystem::path& path)
{
    std::vector<SavedPose> poses;
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        const auto fields = SplitTabs(line);
        if (fields.size() != 15)
            continue;
        try {
            SavedPose pose;
            pose.id = fields[0];
            pose.created_ms = std::stoll(fields[1]);
            const auto name = HexDecode(fields[2]);
            if (!name || pose.id.empty())
                continue;
            pose.name = *name;
            bool valid = true;
            for (std::size_t i = 0; i < 6; ++i) {
                const auto right = ParseQ(fields[3 + i]);
                const auto left = ParseQ(fields[9 + i]);
                if (!right || !left) {
                    valid = false;
                    break;
                }
                pose.right[i] = *right;
                pose.left[i] = *left;
            }
            if (valid)
                poses.push_back(std::move(pose));
        } catch (...) {
            continue;
        }
    }
    return poses;
}

void StorePoses(const std::filesystem::path& path,
                const std::vector<SavedPose>& poses)
{
    if (!path.parent_path().empty())
        std::filesystem::create_directories(path.parent_path());
    const auto temporary = path.string() + ".tmp-" + std::to_string(getpid());
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output)
            throw std::runtime_error("Cannot write pose file " + path.string());
        output << std::fixed << std::setprecision(3);
        for (const auto& pose : poses) {
            output << pose.id << '\t' << pose.created_ms << '\t'
                   << HexEncode(pose.name);
            for (const float value : pose.right)
                output << '\t' << value;
            for (const float value : pose.left)
                output << '\t' << value;
            output << '\n';
        }
        output.flush();
        if (!output)
            throw std::runtime_error("Failed to flush pose file " + path.string());
    }
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(temporary);
        throw std::runtime_error("Cannot replace pose file: " + error.message());
    }
}

std::string SavedPoseJson(const SavedPose& pose)
{
    std::ostringstream json;
    json << "{\"id\":\"" << JsonEscape(pose.id)
         << "\",\"created\":" << pose.created_ms
         << ",\"name\":\"" << JsonEscape(pose.name)
         << "\",\"right\":" << PositionJson(pose.right)
         << ",\"left\":" << PositionJson(pose.left) << '}';
    return json.str();
}

void ServePoseList(int fd, const AppConfig& config)
{
    try {
        const auto poses = LoadPoses(config.poses_file);
        std::ostringstream json;
        json << "{\"ok\":true,\"poses\":[";
        for (std::size_t i = 0; i < poses.size(); ++i) {
            if (i)
                json << ',';
            json << SavedPoseJson(poses[i]);
        }
        json << "]}";
        Respond(fd, 200, "OK", "application/json; charset=utf-8", json.str());
    } catch (const std::exception& error) {
        Respond(fd, 500, "Internal Server Error",
                "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"" +
                    JsonEscape(error.what()) + "\"}");
    }
}

bool ValidPoseId(const std::string& id)
{
    return !id.empty() && id.size() <= 80 &&
           std::all_of(id.begin(), id.end(), [](unsigned char byte) {
               return std::isalnum(byte) || byte == '-';
           });
}

void ServePoseSave(int fd, const AppConfig& config,
                   const HttpRequest& request)
{
    const auto fields = ParseForm(request.body);
    const auto token = fields.find("token");
    const auto name = fields.find("name");
    const auto right = fields.find("right");
    const auto left = fields.find("left");
    if (token == fields.end() || token->second != config.session_token ||
        name == fields.end() || name->second.empty() || name->second.size() > 96 ||
        right == fields.end() || left == fields.end()) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Invalid pose fields\"}");
        return;
    }
    const auto right_values = ParseValues(right->second);
    const auto left_values = ParseValues(left->second);
    if (!right_values || !left_values) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Invalid pose values\"}");
        return;
    }

    try {
        auto poses = LoadPoses(config.poses_file);
        const auto requested_id = fields.find("id");
        if (requested_id != fields.end() && ValidPoseId(requested_id->second)) {
            const auto existing = std::find_if(
                poses.begin(), poses.end(), [&](const SavedPose& pose) {
                    return pose.id == requested_id->second;
                });
            if (existing != poses.end()) {
                Respond(fd, 200, "OK", "application/json; charset=utf-8",
                        "{\"ok\":true,\"pose\":" + SavedPoseJson(*existing) + '}');
                return;
            }
        }

        SavedPose pose;
        pose.id = requested_id != fields.end() && ValidPoseId(requested_id->second)
                      ? requested_id->second
                      : std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count()) +
                            "-" + RandomToken().substr(0, 8);
        pose.created_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::system_clock::now().time_since_epoch()).count();
        pose.name = name->second;
        pose.right = *right_values;
        pose.left = *left_values;
        poses.insert(poses.begin(), pose);
        StorePoses(config.poses_file, poses);
        Respond(fd, 200, "OK", "application/json; charset=utf-8",
                "{\"ok\":true,\"pose\":" + SavedPoseJson(pose) + '}');
    } catch (const std::exception& error) {
        Respond(fd, 500, "Internal Server Error",
                "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"" +
                    JsonEscape(error.what()) + "\"}");
    }
}

void ServePoseDelete(int fd, const AppConfig& config,
                     const HttpRequest& request)
{
    const auto fields = ParseForm(request.body);
    const auto token = fields.find("token");
    const auto id = fields.find("id");
    if (token == fields.end() || token->second != config.session_token ||
        id == fields.end() || !ValidPoseId(id->second)) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Invalid pose id\"}");
        return;
    }
    try {
        auto poses = LoadPoses(config.poses_file);
        poses.erase(std::remove_if(poses.begin(), poses.end(),
                                   [&](const SavedPose& pose) {
                                       return pose.id == id->second;
                                   }),
                    poses.end());
        StorePoses(config.poses_file, poses);
        Respond(fd, 200, "OK", "application/json; charset=utf-8",
                "{\"ok\":true}");
    } catch (const std::exception& error) {
        Respond(fd, 500, "Internal Server Error",
                "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"" +
                    JsonEscape(error.what()) + "\"}");
    }
}

void ServePoseRename(int fd, const AppConfig& config,
                     const HttpRequest& request)
{
    const auto fields = ParseForm(request.body);
    const auto token = fields.find("token");
    const auto id = fields.find("id");
    const auto name = fields.find("name");
    if (token == fields.end() || token->second != config.session_token ||
        id == fields.end() || !ValidPoseId(id->second) ||
        name == fields.end() || name->second.empty() || name->second.size() > 96) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Invalid pose name\"}");
        return;
    }
    try {
        auto poses = LoadPoses(config.poses_file);
        const auto pose = std::find_if(poses.begin(), poses.end(),
                                       [&](const SavedPose& item) {
                                           return item.id == id->second;
                                       });
        if (pose == poses.end()) {
            Respond(fd, 404, "Not Found", "application/json; charset=utf-8",
                    "{\"ok\":false,\"error\":\"Pose not found\"}");
            return;
        }
        pose->name = name->second;
        StorePoses(config.poses_file, poses);
        Respond(fd, 200, "OK", "application/json; charset=utf-8",
                "{\"ok\":true,\"pose\":" + SavedPoseJson(*pose) + '}');
    } catch (const std::exception& error) {
        Respond(fd, 500, "Internal Server Error",
                "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"" +
                    JsonEscape(error.what()) + "\"}");
    }
}

void ServeCommand(int fd, const AppConfig& config, const HttpRequest& request)
{
    if (!config.execute) {
        Respond(fd, 403, "Forbidden", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Server is in preview mode\"}");
        return;
    }
    const auto fields = ParseForm(request.body);
    const auto hand_it = fields.find("hand");
    const auto confirm_it = fields.find("confirm");
    const auto token_it = fields.find("token");
    if (hand_it == fields.end() || confirm_it == fields.end() ||
        token_it == fields.end() || confirm_it->second != "move" ||
        token_it->second != config.session_token ||
        (hand_it->second != "left" && hand_it->second != "right") ||
        !HandEnabled(config, hand_it->second)) {
        Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"ok\":false,\"error\":\"Invalid hand or confirmation\"}");
        return;
    }

    rh56::Position position{};
    rh56::JointMask mask{};
    const auto values_it = fields.find("values");
    if (values_it != fields.end()) {
        const auto values = ParseValues(values_it->second);
        if (!values) {
            Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                    "{\"ok\":false,\"error\":\"Expected six q values in [0,1]\"}");
            return;
        }
        position = *values;
        mask.fill(true);
    } else {
        const auto joint_it = fields.find("joint");
        const auto q_it = fields.find("q");
        if (joint_it == fields.end() || q_it == fields.end()) {
            Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                    "{\"ok\":false,\"error\":\"Missing joint or q\"}");
            return;
        }
        try {
            std::size_t used = 0;
            const unsigned long joint = std::stoul(joint_it->second, &used);
            const auto q = ParseQ(q_it->second);
            if (used != joint_it->second.size() || joint >= position.size() || !q)
                throw std::invalid_argument("range");
            position[joint] = *q;
            mask[joint] = true;
        } catch (...) {
            Respond(fd, 400, "Bad Request", "application/json; charset=utf-8",
                    "{\"ok\":false,\"error\":\"Invalid joint or q\"}");
            return;
        }
    }

    try {
        Rh56Writer writer(HandDevice(hand_it->second));
        if (!writer.WritePosition(position, mask))
            throw std::runtime_error("command was not acknowledged");
        Respond(fd, 200, "OK", "application/json; charset=utf-8",
                "{\"ok\":true}");
    } catch (const std::exception& error) {
        const std::string body = "{\"ok\":false,\"error\":\"" +
                                 JsonEscape(error.what()) + "\"}";
        Respond(fd, 503, "Service Unavailable",
                "application/json; charset=utf-8", body);
    }
}

void HandleClient(int fd, const AppConfig& config)
{
    const auto request = ReadRequest(fd);
    if (!request) {
        Respond(fd, 400, "Bad Request", "text/plain; charset=utf-8",
                "Bad request\n");
        return;
    }
    if (request->method == "GET" && request->path == "/api/status") {
        ServeStatus(fd, config);
        return;
    }
    if (request->method == "GET" && request->path == "/api/poses") {
        ServePoseList(fd, config);
        return;
    }
    if (request->method == "POST" && request->path == "/api/poses/save") {
        ServePoseSave(fd, config, *request);
        return;
    }
    if (request->method == "POST" && request->path == "/api/poses/delete") {
        ServePoseDelete(fd, config, *request);
        return;
    }
    if (request->method == "POST" && request->path == "/api/poses/rename") {
        ServePoseRename(fd, config, *request);
        return;
    }
    if (request->method == "GET" &&
        request->path.rfind("/api/registers/", 0) == 0) {
        ServeRegisters(fd, config,
                       request->path.substr(std::string("/api/registers/").size()));
        return;
    }
    if (request->method == "POST" && request->path == "/api/command") {
        ServeCommand(fd, config, *request);
        return;
    }
    if (request->method == "POST" && request->path == "/api/grip") {
        ServeGrip(fd, config, *request);
        return;
    }
    if (request->method == "POST" && request->path == "/api/faults/clear") {
        ServeClearFault(fd, config, *request);
        return;
    }

    std::filesystem::path asset;
    std::string content_type;
    if (request->method == "GET" &&
        (request->path == "/" || request->path == "/index.html")) {
        asset = config.assets / "index.html";
        content_type = "text/html; charset=utf-8";
    } else if (request->method == "GET" && request->path == "/app.css") {
        asset = config.assets / "app.css";
        content_type = "text/css; charset=utf-8";
    } else if (request->method == "GET" && request->path == "/app.js") {
        asset = config.assets / "app.js";
        content_type = "text/javascript; charset=utf-8";
    } else {
        Respond(fd, 404, "Not Found", "text/plain; charset=utf-8",
                "Not found\n");
        return;
    }

    try {
        Respond(fd, 200, "OK", content_type, ReadAsset(asset));
    } catch (const std::exception& error) {
        Respond(fd, 500, "Internal Server Error", "text/plain; charset=utf-8",
                std::string(error.what()) + '\n');
    }
}

void PrintUsage(const char* executable)
{
    std::cerr << "Usage: " << executable
              << " [--execute] [--hand left|right|both]"
                 " [--bind ADDRESS] [--port PORT] [--assets PATH]"
                 " [--poses-file PATH]\n";
}

std::optional<AppConfig> ParseArguments(int argc, char** argv)
{
    AppConfig config;
    const auto project_root = ProjectRoot(argv[0]);
    config.assets = project_root / "web" / "hand_control";
    config.poses_file = project_root / "data" / "hand_poses.tsv";
    config.session_token = RandomToken();
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--execute") {
            config.execute = true;
        } else if ((argument == "--hand" || argument == "--bind" ||
                    argument == "--port" || argument == "--assets" ||
                    argument == "--poses-file") &&
                   i + 1 < argc) {
            const std::string value = argv[++i];
            if (argument == "--hand")
                config.hand = value;
            else if (argument == "--bind")
                config.bind_address = value;
            else if (argument == "--assets")
                config.assets = value;
            else if (argument == "--poses-file")
                config.poses_file = value;
            else {
                try {
                    const unsigned long port = std::stoul(value);
                    if (port == 0 || port > 65535)
                        return std::nullopt;
                    config.port = static_cast<uint16_t>(port);
                } catch (...) {
                    return std::nullopt;
                }
            }
        } else {
            return std::nullopt;
        }
    }
    if (config.hand != "left" && config.hand != "right" &&
        config.hand != "both")
        return std::nullopt;
    return config;
}

int RunServer(const AppConfig& config)
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        throw std::runtime_error("socket failed: " + std::string(std::strerror(errno)));
    const int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(config.port);
    if (inet_pton(AF_INET, config.bind_address.c_str(), &address.sin_addr) != 1)
        throw std::runtime_error("Invalid IPv4 bind address: " + config.bind_address);
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        throw std::runtime_error("bind failed: " + std::string(std::strerror(errno)));
    if (listen(server_fd, 8) != 0)
        throw std::runtime_error("listen failed: " + std::string(std::strerror(errno)));

    PrintAccessUrls(config);
    std::cout << "Mode: " << (config.execute ? "HARDWARE (motion enabled)" : "preview")
              << ", hand: " << config.hand << '\n';
    if (config.execute)
        std::cout << "Keep the physical E-stop within immediate reach.\n";

    while (!stop_requested) {
        sockaddr_in peer{};
        socklen_t peer_size = sizeof(peer);
        const int client = accept(server_fd, reinterpret_cast<sockaddr*>(&peer),
                                  &peer_size);
        if (client < 0) {
            if (errno == EINTR || stop_requested)
                continue;
            throw std::runtime_error("accept failed: " +
                                     std::string(std::strerror(errno)));
        }
        const timeval receive_timeout{2, 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout,
                   sizeof(receive_timeout));
        pollfd ready{client, POLLIN, 0};
        if (poll(&ready, 1, 500) > 0 && (ready.revents & POLLIN))
            HandleClient(client, config);
        close(client);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    const auto config = ParseArguments(argc, argv);
    if (!config) {
        PrintUsage(argv[0]);
        return 2;
    }

    std::signal(SIGINT, StopHandler);
    std::signal(SIGTERM, StopHandler);
    try {
        std::unique_ptr<PidFile> pid_file;
        if (config->execute)
            pid_file = std::make_unique<PidFile>(ProjectRoot(argv[0]));
        return RunServer(*config);
    } catch (const std::exception& error) {
        if (server_fd >= 0)
            close(server_fd);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
