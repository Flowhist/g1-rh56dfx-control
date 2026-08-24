#pragma once

#include "rh56_protocol.hpp"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>

class Rh56Readonly
{
public:
    explicit Rh56Readonly(const std::string& device)
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

    ~Rh56Readonly() { close(fd_); }

    Rh56Readonly(const Rh56Readonly&) = delete;
    Rh56Readonly& operator=(const Rh56Readonly&) = delete;

    bool ReadPosition(std::array<float, 6>& position, uint8_t id = 1)
    {
        const auto request = rh56::MakeReadPosition(id);

        tcflush(fd_, TCIFLUSH);
        if (write(fd_, request.data(), request.size()) !=
            static_cast<ssize_t>(request.size()))
            return false;
        tcdrain(fd_);

        rh56::ReadResponse response{};
        return ReadExact(response) && rh56::ParsePosition(response, id, position);
    }

private:
    template <std::size_t N>
    bool ReadExact(std::array<uint8_t, N>& response)
    {
        std::size_t received = 0;
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(50);

        while (received < response.size()) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now());
            if (remaining.count() <= 0)
                return false;

            pollfd pfd{fd_, POLLIN, 0};
            const int ready =
                poll(&pfd, 1, static_cast<int>(remaining.count()));
            if (ready <= 0)
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
