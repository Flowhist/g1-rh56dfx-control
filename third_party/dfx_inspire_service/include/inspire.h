#ifndef INSPIRE_H
#define INSPIRE_H

#include <eigen3/Eigen/Dense>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "SerialPort.h"

namespace inspire
{

class InspireHand
{
public:
  using RawValues = std::array<int16_t, 6>;
  using StatusValues = std::array<uint8_t, 6>;

  InspireHand(SerialPort::SharedPtr serial = nullptr, id_t id = 0)
  : serial_(serial), id(id)
  {
    if(!serial)
      serial_ = std::make_shared<SerialPort>("/dev/ttyUSB0", B115200);

  }

  void ChangeID(uint8_t before, uint8_t now)
  {
    std::vector<uint8_t> cmd = {0xEB, 0x90, before, 0x04, 0x12, 0xE8, 0x03, now, 0x00};
    cmd.back() = CheckSum(cmd.data(), cmd.size());
    serial_->send(cmd.data(), cmd.size());

    usleep(5000);
    serial_->recv(recvBuff, 9);
  }


  /**
   * @brief Set the finger position
   * -1: no change
   * [0, 1] 0: close
   * ID: pinky, ring, middle, index, thumb_bend, thumb_rotate
   */
  int16_t SetPosition(const Eigen::Matrix<double, 6, 1> & q)
  {
    Eigen::Matrix<int16_t, 6, 1> q_int16 = (q * 1000).cast<int16_t>().cwiseMax(0).cwiseMin(1000);

    uint8_t cmd[20];
    cmd[0] = 0xEB;    // head
    cmd[1] = 0x90;
    cmd[2] = id;      // ID
    cmd[3] = 0x0F;    // data length
    cmd[4] = 0x12;    // write
    cmd[5] = 0xCE;    // address
    cmd[6] = 0x05;

    cmd[7] = q_int16(0) & 0xFF;
    cmd[8] = (q_int16(0) >> 8) & 0xFF;
    cmd[9] = q_int16(1) & 0xFF;
    cmd[10] = (q_int16(1) >> 8) & 0xFF;
    cmd[11] = q_int16(2) & 0xFF;
    cmd[12] = (q_int16(2) >> 8) & 0xFF;
    cmd[13] = q_int16(3) & 0xFF;
    cmd[14] = (q_int16(3) >> 8) & 0xFF;
    cmd[15] = q_int16(4) & 0xFF;
    cmd[16] = (q_int16(4) >> 8) & 0xFF;
    cmd[17] = q_int16(5) & 0xFF;
    cmd[18] = (q_int16(5) >> 8) & 0xFF;

    cmd[19] = CheckSum(cmd, 20);

    serial_->send(cmd, 20);
    usleep(5000);
    serial_->recv(recvBuff, 9);
    return 0;
  }

  /**
   * @brief Get the finger position
   *
   * ID: pinky, ring, middle, index, thumb_bend, thumb_rotate
   */
  int16_t GetPosition(Eigen::Matrix<double, 6, 1> & q)
  {
    std::vector<uint8_t> cmd = {0xEB, 0x90, id, 0x04, 0x11, 0x0A, 0x06, 0x0C, 0x00};
    cmd.back() = CheckSum(cmd.data(), cmd.size());
    serial_->send(cmd.data(), cmd.size());

    usleep(5000);
    size_t len = serial_->recv(recvBuff, 20);

    if(len != 20) return 1;
    if(recvBuff[19] != CheckSum(recvBuff, 20)) return 2;

    q(0) = (recvBuff[7] | (recvBuff[8] << 8)) / 1000.;
    q(1) = (recvBuff[9] | (recvBuff[10] << 8)) / 1000.;
    q(2) = (recvBuff[11] | (recvBuff[12] << 8)) / 1000.;
    q(3) = (recvBuff[13] | (recvBuff[14] << 8)) / 1000.;
    q(4) = (recvBuff[15] | (recvBuff[16] << 8)) / 1000.;
    q(5) = (recvBuff[17] | (recvBuff[18] << 8)) / 1000.;

    return 0;
  }

  /**
   * @brief Set the finger velocity
   */
  void SetVelocity(int16_t v0, int16_t v1, int16_t v2, int16_t v3, int16_t v4, int16_t v5)
  {
    uint8_t cmd[20];
    cmd[0] = 0xEB;    // head
    cmd[1] = 0x90;
    cmd[2] = id;      // ID
    cmd[3] = 0x0F;    // data length
    cmd[4] = 0x12;    // write
    cmd[5] = 0xF2;    // address
    cmd[6] = 0x05;

    cmd[7] = v0 & 0xFF;
    cmd[8] = (v0 >> 8) & 0xFF;
    cmd[9] = v1 & 0xFF;
    cmd[10] = (v1 >> 8) & 0xFF;
    cmd[11] = v2 & 0xFF;
    cmd[12] = (v2 >> 8) & 0xFF;
    cmd[13] = v3 & 0xFF;
    cmd[14] = (v3 >> 8) & 0xFF;
    cmd[15] = v4 & 0xFF;
    cmd[16] = (v4 >> 8) & 0xFF;
    cmd[17] = v5 & 0xFF;
    cmd[18] = (v5 >> 8) & 0xFF;

    cmd[19] = CheckSum(cmd, 20);

    serial_->send(cmd, 20);
    usleep(5000);
    serial_->recv(recvBuff, 9);
  }

  /** Set all six velocity limits. Values are in the firmware range [0, 1000]. */
  bool SetVelocity(const RawValues& velocity)
  {
    return WriteWords(0x05F2, velocity, 1000);
  }

  /**
   * @brief Get the force control threshold
   *
   * [0, 1000] Unit: g
   */
  void SetForce(uint16_t f0, uint16_t f1, uint16_t f2, uint16_t f3, uint16_t f4, uint16_t f5)
  {
    uint8_t cmd[20];
    cmd[0] = 0xEB;    // head
    cmd[1] = 0x90;
    cmd[2] = id;      // ID
    cmd[3] = 0x0F;    // data length
    cmd[4] = 0x12;    // write
    cmd[5] = 0xDA;    // address
    cmd[6] = 0x05;

    cmd[7] = f0 & 0xFF;
    cmd[8] = (f0 >> 8) & 0xFF;
    cmd[9] = f1 & 0xFF;
    cmd[10] = (f1 >> 8) & 0xFF;
    cmd[11] = f2 & 0xFF;
    cmd[12] = (f2 >> 8) & 0xFF;
    cmd[13] = f3 & 0xFF;
    cmd[14] = (f3 >> 8) & 0xFF;
    cmd[15] = f4 & 0xFF;
    cmd[16] = (f4 >> 8) & 0xFF;
    cmd[17] = f5 & 0xFF;
    cmd[18] = (f5 >> 8) & 0xFF;

    cmd[19] = CheckSum(cmd, 20);

    serial_->send(cmd, 20);
    usleep(5000);
    serial_->recv(recvBuff, 9);
  }

  /** Set firmware contact-force thresholds in grams, range [0, 1000]. */
  bool SetForceLimit(const RawValues& force)
  {
    return WriteWords(0x05DA, force, 1000);
  }

  /**
   * @brief Get the force of each finger
   *
   * The force is in the unit of g, [0 - 1000], convert to N
   */
  int16_t GetForce(Eigen::Matrix<double, 6, 1> & f)
  {
    std::vector<uint8_t> cmd = {0xEB, 0x90, id, 0x04, 0x11, 0x2E, 0x06, 0x0C, 0x00};
    cmd.back() = CheckSum(cmd.data(), cmd.size());
    serial_->send(cmd.data(), cmd.size());

    usleep(5000);
    size_t len = serial_->recv(recvBuff, 20);

    if(len != 20) return 1;
    if(recvBuff[19] != CheckSum(recvBuff, 20)) return 2;

    f(0) = int16_t(recvBuff[7] | (recvBuff[8] << 8)) / 1000. * 9.8;
    f(1) = int16_t(recvBuff[9] | (recvBuff[10] << 8)) / 1000. * 9.8;
    f(2) = int16_t(recvBuff[11] | (recvBuff[12] << 8)) / 1000. * 9.8;
    f(3) = int16_t(recvBuff[13] | (recvBuff[14] << 8)) / 1000. * 9.8;
    f(4) = int16_t(recvBuff[15] | (recvBuff[16] << 8)) / 1000. * 9.8;
    f(5) = int16_t(recvBuff[17] | (recvBuff[18] << 8)) / 1000. * 9.8;

    return 0;
  }

  /** Read raw actuator currents in mA. */
  int16_t GetCurrent(RawValues& current)
  {
    return ReadWords(0x063A, current);
  }

  /** Read unscaled force-sensor values in the firmware's native units. */
  int16_t GetForceRaw(RawValues& force)
  {
    return ReadWords(0x062E, force);
  }

  /** Read one firmware error-bit byte for each actuator. */
  int16_t GetError(StatusValues& error)
  {
    return ReadBytes(0x0646, error);
  }

  /** Read one state code (0, 1, 2, 3, 5, 6 or 7) for each actuator. */
  int16_t GetStatus(StatusValues& status)
  {
    return ReadBytes(0x064C, status);
  }

  /** Read one temperature byte for each actuator. */
  int16_t GetTemperature(StatusValues& temperature)
  {
    return ReadBytes(0x0652, temperature);
  }

  /** Set firmware current-protection thresholds in mA, range [0, 1500]. */
  bool SetCurrentLimit(const RawValues& current_limit)
  {
    return WriteWords(0x03FC, current_limit, 1500);
  }

  /**
   * @brief Clear error
   *
   * When the Inspire Hand has a fault such as a stall, overcurrent, or abnormality,
   * the fault can be cleared by the clear fault command.
   */
  void ClearError()
  {
    uint8_t cmd[9];
    cmd[0] = 0xEB;    // head
    cmd[1] = 0x90;
    cmd[2] = id;      // ID
    cmd[3] = 0x04;    // data length
    cmd[4] = 0x12;    // write
    cmd[5] = 0xEC;    // address
    cmd[6] = 0x03;
    cmd[7] = 0x01;
    cmd[8] = CheckSum(cmd, 9);

    serial_->send(cmd, 9);
    usleep(5000);
    serial_->recv(recvBuff, 9);
  }

  /**
   * @brief Force sensor calibration
   *
   * @attention The Inspire Hand must be in an unloaded state during calibration
   */
  void Calibration()
  {
    std::vector<uint8_t> cmd = {0xEB, 0x90, id, 0x04, 0x12, 0x2F, 0x06, 0x01, 0x00};
    cmd.back() = CheckSum(cmd.data(), cmd.size());
    serial_->send(cmd.data(), cmd.size());

    usleep(5000);
    serial_->recv(recvBuff, 9); // First frame
    sleep(10); // The calibration process takes about 6 seconds
    serial_->recv(recvBuff, 9); // Second frame
  }

  uint8_t id = 1;
private:
  int16_t ReadWords(uint16_t address, RawValues& values)
  {
    std::vector<uint8_t> cmd = {
      0xEB, 0x90, id, 0x04, 0x11,
      static_cast<uint8_t>(address & 0xFF),
      static_cast<uint8_t>(address >> 8), 0x0C, 0x00};
    cmd.back() = CheckSum(cmd.data(), cmd.size());
    serial_->send(cmd.data(), cmd.size());

    usleep(5000);
    const size_t len = serial_->recv(recvBuff, 20);
    if(!ValidReadResponse(len, address, 20)) return 1;

    for(std::size_t i = 0; i < values.size(); ++i)
      values[i] = static_cast<int16_t>(
        static_cast<uint16_t>(recvBuff[7 + 2 * i]) |
        (static_cast<uint16_t>(recvBuff[8 + 2 * i]) << 8));
    return 0;
  }

  int16_t ReadBytes(uint16_t address, StatusValues& values)
  {
    std::vector<uint8_t> cmd = {
      0xEB, 0x90, id, 0x04, 0x11,
      static_cast<uint8_t>(address & 0xFF),
      static_cast<uint8_t>(address >> 8), 0x06, 0x00};
    cmd.back() = CheckSum(cmd.data(), cmd.size());
    serial_->send(cmd.data(), cmd.size());

    usleep(5000);
    constexpr size_t response_size = 14;
    const size_t len = serial_->recv(recvBuff, response_size);
    if(!ValidReadResponse(len, address, response_size)) return 1;

    for(std::size_t i = 0; i < values.size(); ++i)
      values[i] = recvBuff[7 + i];
    return 0;
  }

  bool WriteWords(uint16_t address, const RawValues& values, int16_t maximum)
  {
    for(const int16_t value : values)
      if(value < 0 || value > maximum) return false;

    uint8_t cmd[20]{};
    cmd[0] = 0xEB;
    cmd[1] = 0x90;
    cmd[2] = id;
    cmd[3] = 0x0F;
    cmd[4] = 0x12;
    cmd[5] = static_cast<uint8_t>(address & 0xFF);
    cmd[6] = static_cast<uint8_t>(address >> 8);
    for(std::size_t i = 0; i < values.size(); ++i) {
      const uint16_t raw = static_cast<uint16_t>(values[i]);
      cmd[7 + 2 * i] = static_cast<uint8_t>(raw & 0xFF);
      cmd[8 + 2 * i] = static_cast<uint8_t>(raw >> 8);
    }
    cmd[19] = CheckSum(cmd, 20);
    serial_->send(cmd, 20);
    usleep(5000);
    const size_t len = serial_->recv(recvBuff, 9);
    return ValidWriteResponse(len, address);
  }

  bool ValidReadResponse(size_t len, uint16_t address, size_t expected) const
  {
    return len == expected && recvBuff[0] == 0x90 && recvBuff[1] == 0xEB &&
      recvBuff[2] == id && recvBuff[4] == 0x11 &&
      recvBuff[5] == static_cast<uint8_t>(address & 0xFF) &&
      recvBuff[6] == static_cast<uint8_t>(address >> 8) &&
      recvBuff[expected - 1] == CheckSum(recvBuff, expected);
  }

  bool ValidWriteResponse(size_t len, uint16_t address) const
  {
    return len == 9 && recvBuff[0] == 0x90 && recvBuff[1] == 0xEB &&
      recvBuff[2] == id && recvBuff[4] == 0x12 &&
      recvBuff[5] == static_cast<uint8_t>(address & 0xFF) &&
      recvBuff[6] == static_cast<uint8_t>(address >> 8) &&
      recvBuff[8] == CheckSum(recvBuff, 9);
  }

  uint8_t CheckSum(const uint8_t* data, uint8_t len) const
  {
    uint8_t sum = 0;
    for (int i = 2; i < len - 1; i++)
    {
      sum += data[i];
    }
    return sum;
  }

  SerialPort::SharedPtr serial_;
  uint8_t recvBuff[1024];
};

} // namespace inspire
#endif // INSPIRE_H
