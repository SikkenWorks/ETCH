#pragma once
#include <cstdint>

namespace etch {
enum class BootReason : std::uint8_t {Normal,OctMinus,PreviousIncomplete,StorageCrc,AudioInit,UiInit,RepeatedFailures,Watchdog,InputOverflow};
enum class BootMode : std::uint8_t {Normal,SafeMode};
struct BootRecord {bool incomplete=false;std::uint8_t earlyFailures=0;BootReason lastReason=BootReason::Normal;bool bootGood=true;};
struct BootInputs {bool octMinusHeld=false;bool storageCrcOk=true;bool audioInitOk=true;bool uiInitOk=true;};
struct BootState {BootMode mode=BootMode::SafeMode;BootReason reason=BootReason::Normal;bool outputMuted=true;bool storageReadOnly=true;bool loadLastPatch=false;bool updaterAvailable=true;bool bootInProgress=false;bool bootGood=false;};
class BootSafetyMachine {
public:
 explicit BootSafetyMachine(std::uint32_t stableWindowMs=3000,std::uint8_t maxEarlyFailures=2) noexcept;
 BootState begin(const BootRecord& record,const BootInputs& inputs) noexcept;
 bool markStable(std::uint32_t runtimeMs) noexcept;
 BootState failEarly(BootReason reason) noexcept;
 const BootRecord& record() const noexcept {return record_;}
 const BootState& state() const noexcept {return state_;}
private:
 BootState safe(BootReason reason) const noexcept;
 std::uint32_t stableWindowMs_;std::uint8_t maxEarlyFailures_;BootRecord record_{};BootState state_{};
};
}
