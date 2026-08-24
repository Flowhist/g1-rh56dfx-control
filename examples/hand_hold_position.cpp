// Software stop: read each selected hand and command its current position.

#include "rh56_readonly.hpp"
#include "rh56_writer.hpp"

#include <iostream>
#include <string>

namespace {

constexpr const char* kRightDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if01-port0";
constexpr const char* kLeftDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if02-port0";

bool Hold(const char* name, const std::string& device)
{
    rh56::Position current{};
    {
        Rh56Readonly reader(device);
        if (!reader.ReadPosition(current)) {
            std::cerr << name << ": failed to read current position\n";
            return false;
        }
    }

    rh56::JointMask all{};
    all.fill(true);
    Rh56Writer writer(device);
    if (!writer.WritePosition(current, all)) {
        std::cerr << name << ": hold command was not acknowledged\n";
        return false;
    }
    std::cout << name << ": holding current position\n";
    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string selection = argc > 1 ? argv[1] : "both";
    if (selection != "left" && selection != "right" && selection != "both") {
        std::cerr << "Usage: " << argv[0] << " [left|right|both]\n";
        return 2;
    }

    try {
        bool ok = true;
        if (selection == "left" || selection == "both")
            ok = Hold("left", kLeftDevice) && ok;
        if (selection == "right" || selection == "both")
            ok = Hold("right", kRightDevice) && ok;
        return ok ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
