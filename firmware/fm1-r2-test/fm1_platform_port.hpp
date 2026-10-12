#pragma once
#include "../../target/r2_app.hpp"
#include <cstdint>

// This bridge is intentionally tiny: ETCH application code never reaches GPIO,
// LCD, USB or flash directly. The board implementation is the only file allowed
// to bind verified JieLi/FM-1 hardware functions to the R2 application.
namespace etch::fm1test {

struct BoardHooks {
 std::uint32_t (*millis)() noexcept = nullptr;
 bool (*octMinusHeldAtBoot)() noexcept = nullptr;
 void (*muteAudio)(bool) noexcept = nullptr;
 void (*showSplash)(const char*,const char*) noexcept = nullptr;
 void (*showSafeMode)(etch::BootReason) noexcept = nullptr;
 void (*serviceUsbUpdater)() noexcept = nullptr;
 bool (*pollControl)(etch::hal::ControlEvent&) noexcept = nullptr;
};

class Fm1R2Platform final : public etch::target::R2Platform {
public:
 explicit Fm1R2Platform(BoardHooks hooks) noexcept : hooks_(hooks) {}
 bool complete() const noexcept;
 std::uint32_t millis() noexcept override;
 bool octMinusHeldAtBoot() noexcept override;
 void muteAudio(bool muted) noexcept override;
 void showBootSplash(const char* line1,const char* line2) noexcept override;
 void showSafeMode(etch::BootReason reason) noexcept override;
 void serviceUsbUpdater() noexcept override;
 bool pollControl(etch::hal::ControlEvent& event) noexcept override;
private:
 BoardHooks hooks_{};
};

} // namespace etch::fm1test
