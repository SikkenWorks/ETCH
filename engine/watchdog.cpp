#include "watchdog.hpp"
#include <algorithm>
#include <limits>

namespace etch {
WatchdogSupervisor::WatchdogSupervisor(std::uint32_t timeoutMs) noexcept {
 state_.timeoutMs=std::max<std::uint32_t>(50,timeoutMs);
}

void WatchdogSupervisor::arm(std::uint32_t nowMs) noexcept {
 state_.lastKickMs=nowMs;
 state_.armed=true;
 state_.tripped=false;
}

void WatchdogSupervisor::kick(std::uint32_t nowMs) noexcept {
 if(!state_.armed)return;
 state_.lastKickMs=nowMs;
 state_.tripped=false;
}

bool WatchdogSupervisor::poll(std::uint32_t nowMs) noexcept {
 if(!state_.armed||state_.tripped)return state_.tripped;
 const auto elapsed=static_cast<std::uint32_t>(nowMs-state_.lastKickMs);
 if(elapsed<state_.timeoutMs)return false;
 state_.tripped=true;
 if(state_.crashCount<std::numeric_limits<std::uint16_t>::max())++state_.crashCount;
 return true;
}

void WatchdogSupervisor::disarm() noexcept {
 state_.armed=false;
 state_.tripped=false;
}

void WatchdogSupervisor::clearCrashCount() noexcept {
 state_.crashCount=0;
}
}
