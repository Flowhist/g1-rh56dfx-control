#include "inspire.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

namespace {

constexpr const char* kRightSerial =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if01-port0";
constexpr const char* kLeftSerial =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if02-port0";

template <typename Values>
void PrintValues(const char* label, const Values& values)
{
    std::cout << label << "=[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0)
            std::cout << ", ";
        std::cout << +values[i];
    }
    std::cout << "]\n";
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2 || (std::string(argv[1]) != "right" &&
                      std::string(argv[1]) != "left")) {
        std::cerr << "Usage: " << argv[0] << " right|left\n";
        return 2;
    }

    const std::string side = argv[1];
    const char* serial_path = side == "right" ? kRightSerial : kLeftSerial;
    std::cout << "READ-ONLY diagnostic: side=" << side
              << " serial=" << serial_path << " id=1\n";

    // Diagnostics favor complete frames over the service's low-latency default.
    auto serial = std::make_shared<SerialPort>(serial_path, B115200, 50);
    inspire::InspireHand hand(serial, 1);
    int failures = 0;

    Eigen::Matrix<double, 6, 1> position{};
    if (hand.GetPosition(position) == 0) {
        std::cout << "position_q=[";
        for (int i = 0; i < position.size(); ++i) {
            if (i != 0)
                std::cout << ", ";
            std::cout << std::fixed << std::setprecision(3) << position(i);
        }
        std::cout << "]\n";
    } else {
        std::cerr << "position_q=READ_FAILED\n";
        ++failures;
    }
    usleep(20000);

    inspire::InspireHand::RawValues current{};
    if (hand.GetCurrent(current) == 0)
        PrintValues("current_mA", current);
    else {
        std::cerr << "current_mA=READ_FAILED\n";
        ++failures;
    }
    usleep(20000);

    inspire::InspireHand::RawValues force{};
    if (hand.GetForceRaw(force) == 0)
        PrintValues("force_raw", force);
    else {
        std::cerr << "force_raw=READ_FAILED\n";
        ++failures;
    }
    usleep(20000);

    inspire::InspireHand::StatusValues error{};
    if (hand.GetError(error) == 0)
        PrintValues("error_bits", error);
    else {
        std::cerr << "error_bits=READ_FAILED\n";
        ++failures;
    }
    usleep(20000);

    inspire::InspireHand::StatusValues temperature{};
    if (hand.GetTemperature(temperature) == 0)
        PrintValues("temperature_C", temperature);
    else {
        std::cerr << "temperature_raw=READ_FAILED\n";
        ++failures;
    }
    usleep(20000);

    inspire::InspireHand::StatusValues status{};
    if (hand.GetStatus(status) == 0) {
        PrintValues("status_raw", status);
        bool valid = true;
        for (const uint8_t value : status)
            valid = valid && (value <= 3 || value == 5 || value == 6 || value == 7);
        if (!valid)
            std::cout << "status_note=contains undocumented state code\n";
    } else {
        std::cerr << "status_raw=READ_FAILED\n";
        ++failures;
    }

    return failures == 0 ? 0 : 1;
}
