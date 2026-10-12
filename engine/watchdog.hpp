#pragma once
#include <cstdint>

namespace etch {
struct WatchdogSnapshot {
 std::uint32_t timeoutMs=500;
 std::uint32_t lastKickMs=0;
 std::uint16_t crashCount=0;
 bool armed=false;
 bool tripped=false;
};

class WatchdogSupervisor {
public:
 explicit WatchdogSupervisor(std::uint32_t timeoutMs=500) noexcept;
 void arm(std::uint32_t nowMs) noexcept;
 void kick(std::uint32_t nowMs) noexcept;
 bool poll(std::uint32_t nowMs) noexcept;
 void disarm() noexcept;
 void clearCrashCount() noexcept;
 const WatchdogSnapshot& snapshot() const noexcept { return state_; }
private:
 WatchdogSnapshot state_{};
};
}
