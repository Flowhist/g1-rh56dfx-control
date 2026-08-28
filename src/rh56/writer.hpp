#pragma once

#include "protocol.hpp"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>

class Rh56Writer
{
public:
    explicit Rh56Writer(const std::string& device)
        : fd_(open(device.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC))
    {
        if (fd_ < 0)
            throw std::runtime_error("Cannot open " + device + ": " +
                                     std::strerror(errno));

        termios options{};
        if (tcgetattr(fd_, &options) != 0) {
            const std::string error = std::strerror(errno);
            close(fd_);
            throw std::runtime_error("tcgetattr failed: " + error);
        }
        cfmakeraw(&options);
        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        options.c_cflag |= CLOCAL | CREAD;
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 0;
        if (tcsetattr(fd_, TCSANOW, &options) != 0) {
            const std::string error = std::strerror(errno);
            close(fd_);
            throw std::runtime_error("tcsetattr failed: " + error);
        }
    }

    ~Rh56Writer() { close(fd_); }

    Rh56Writer(const Rh56Writer&) = delete;
    Rh56Writer& operator=(const Rh56Writer&) = delete;

    bool WritePosition(const rh56::Position& position,
                       const rh56::JointMask& mask, uint8_t id = 1)
    {
        const auto request = rh56::MakeWritePosition(id, position, mask);
        return WriteAndAcknowledge(request, 0x05CE, id);
    }

    bool WriteWords(uint16_t address, const rh56::RawValues& values,
                    int16_t maximum, uint8_t id = 1)
    {
        for (const int16_t value : values)
            if (value < 0 || value > maximum)
                return false;
        const auto request = rh56::MakeWriteWords(id, address, values);
        return WriteAndAcknowledge(request, address, id);
    }

    bool ClearErrors(uint8_t id = 1)
    {
        constexpr uint16_t clear_error_address = 0x03EC;
        const auto request =
            rh56::MakeWriteByte(id, clear_error_address, uint8_t{1});
        return WriteAndAcknowledge(request, clear_error_address, id);
    }

private:
    template <std::size_t N>
    bool WriteAndAcknowledge(const std::array<uint8_t, N>& request,
                             uint16_t address, uint8_t id)
    {
        tcflush(fd_, TCIFLUSH);
        if (write(fd_, request.data(), request.size()) !=
            static_cast<ssize_t>(request.size()))
            return false;
        tcdrain(fd_);

        std::array<uint8_t, 9> response{};
        if (!ReadExact(response))
            return false;
        return response[0] == 0x90 && response[1] == 0xEB &&
               response[2] == id && response[4] == 0x12 &&
               response[5] == static_cast<uint8_t>(address & 0xFF) &&
               response[6] == static_cast<uint8_t>(address >> 8) &&
               response.back() ==
                   rh56::Checksum(response.data(), response.size());
    }

    template <std::size_t N>
    bool ReadExact(std::array<uint8_t, N>& response)
    {
        std::size_t received = 0;
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        while (received < response.size()) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now());
            if (remaining.count() <= 0)
                return false;
            pollfd pfd{fd_, POLLIN, 0};
            if (poll(&pfd, 1, static_cast<int>(remaining.count())) <= 0)
                return false;
            const ssize_t count =
                read(fd_, response.data() + received, response.size() - received);
            if (count > 0)
                received += static_cast<std::size_t>(count);
            else if (count < 0 && errno != EINTR)
                return false;
        }
        return true;
    }

    int fd_;
};
