// Read-only RH56DFX position probe. It never writes control registers.

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <poll.h>
#include <string>
#include <termios.h>
#include <unistd.h>

namespace {

constexpr std::size_t kResponseSize = 20;

uint8_t Checksum(const uint8_t* data, std::size_t size)
{
    uint8_t sum = 0;
    for (std::size_t i = 2; i + 1 < size; ++i)
        sum = static_cast<uint8_t>(sum + data[i]);
    return sum;
}

bool ReadExact(int fd, std::array<uint8_t, kResponseSize>& response)
{
    std::size_t received = 0;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(150);

    while (received < response.size()) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0)
            return false;

        pollfd pfd{fd, POLLIN, 0};
        const int ready = poll(&pfd, 1, static_cast<int>(remaining.count()));
        if (ready <= 0)
            return false;

        const ssize_t count =
            read(fd, response.data() + received, response.size() - received);
        if (count > 0)
            received += static_cast<std::size_t>(count);
        else if (count < 0 && errno != EINTR)
            return false;
    }
    return true;
}

bool Probe(int fd, uint8_t id)
{
    std::array<uint8_t, 9> request{
        0xEB, 0x90, id, 0x04, 0x11, 0x0A, 0x06, 0x0C, 0x00};
    request.back() = Checksum(request.data(), request.size());

    tcflush(fd, TCIFLUSH);
    if (write(fd, request.data(), request.size()) !=
        static_cast<ssize_t>(request.size())) {
        std::cerr << "ID " << static_cast<int>(id) << ": write failed\n";
        return false;
    }
    tcdrain(fd);

    std::array<uint8_t, kResponseSize> response{};
    if (!ReadExact(fd, response)) {
        std::cout << "ID " << static_cast<int>(id) << ": no response\n";
        return false;
    }
    if (response.back() != Checksum(response.data(), response.size())) {
        std::cout << "ID " << static_cast<int>(id) << ": checksum error\n";
        return false;
    }

    std::cout << "ID " << static_cast<int>(id) << ':'
              << std::fixed << std::setprecision(3);
    for (std::size_t i = 0; i < 6; ++i) {
        const uint16_t raw = static_cast<uint16_t>(response[7 + 2 * i]) |
                             (static_cast<uint16_t>(response[8 + 2 * i]) << 8);
        std::cout << ' ' << raw / 1000.0;
    }
    std::cout << '\n';
    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " /dev/ttyUSBX\n";
        return 2;
    }

    const std::string device = argv[1];
    const int fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (fd < 0) {
        std::cerr << "Cannot open " << device << ": " << std::strerror(errno)
                  << '\n';
        return 1;
    }

    termios options{};
    if (tcgetattr(fd, &options) != 0) {
        std::cerr << "tcgetattr failed: " << std::strerror(errno) << '\n';
        close(fd);
        return 1;
    }
    cfmakeraw(&options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag |= CLOCAL | CREAD;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        std::cerr << "tcsetattr failed: " << std::strerror(errno) << '\n';
        close(fd);
        return 1;
    }

    std::cout << device << '\n';
    const bool found = Probe(fd, 1) | Probe(fd, 2);
    close(fd);
    return found ? 0 : 3;
}
