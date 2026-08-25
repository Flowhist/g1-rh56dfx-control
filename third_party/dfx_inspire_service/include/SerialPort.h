#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <termios.h>
#include <sys/select.h>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <memory>
#include <chrono>
#include <queue>

class SerialPort
{
public:
  using SharedPtr = std::shared_ptr<SerialPort>;

  SerialPort(std::string port, speed_t baudrate, int timeout_ms = 2)
  {
    set_timeout(timeout_ms);
    Init(port, baudrate);
  }

  ~SerialPort()
  {
    close(fd_);
  }

  ssize_t send(const uint8_t* data, size_t len)
  {
    ssize_t ret = ::write(fd_, data, len);
    return ret;
  }

  ssize_t recv(uint8_t* data, size_t len)
  {
    size_t received = 0;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeout_ms_);
    while(received < len)
    {
      const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
        deadline - std::chrono::steady_clock::now());
      if(remaining.count() <= 0) break;

      fd_set read_set;
      FD_ZERO(&read_set);
      FD_SET(fd_, &read_set);
      timeval timeout;
      timeout.tv_sec = remaining.count() / 1000000;
      timeout.tv_usec = remaining.count() % 1000000;
      const int ready = select(fd_ + 1, &read_set, NULL, NULL, &timeout);
      if(ready == 0) break;
      if(ready < 0)
      {
        if(errno == EINTR) continue;
        break;
      }

      const ssize_t count = ::read(fd_, data + received, len - received);
      if(count > 0)
        received += static_cast<size_t>(count);
      else if(count < 0 && errno != EINTR && errno != EAGAIN)
        break;
    }
    return static_cast<ssize_t>(received);
  }

  void set_timeout(int timeout_ms)
  {
    timeout_ms_ = timeout_ms;
  }

private:
  void Init(std::string port, speed_t baudrate)
  {
    int ret;
    // Open serial port
    fd_ = open(port.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ < 0)
    {
      printf("Open serial port %s failed\n", port.c_str());
      exit(-1);
    }

    // Set attributes
    struct termios option;
    memset(&option, 0, sizeof(option));
    ret = tcgetattr(fd_, &option);

    option.c_oflag = 0;
    option.c_lflag = 0;
    option.c_iflag = 0;

    cfsetispeed(&option, baudrate);
    cfsetospeed(&option, baudrate);

    option.c_cflag &= ~CSIZE;
    option.c_cflag |= CS8; // 8
    option.c_cflag &= ~PARENB; // no parity
    option.c_iflag &= ~INPCK; // no parity
    option.c_cflag &= ~CSTOPB; // 1 stop bit

    option.c_cc[VTIME] = 0;
    option.c_cc[VMIN] = 0;
    option.c_lflag |= CBAUDEX;

    ret = tcflush(fd_, TCIFLUSH);
    ret = tcsetattr(fd_, TCSANOW, &option);
  }

  int fd_;
  int timeout_ms_{2};

  std::queue<uint8_t> recv_queue;
  std::array<uint8_t, 1024> recv_buf;
};

#endif // SERIAL_PORT_H
