#include "boot_state.hpp"
#include <algorithm>
namespace etch {
BootSafetyMachine::BootSafetyMachine(std::uint32_t stableWindowMs,std::uint8_t maxEarlyFailures) noexcept:stableWindowMs_(std::max<std::uint32_t>(100,stableWindowMs)),maxEarlyFailures_(std::max<std::uint8_t>(1,maxEarlyFailures)){}
BootState BootSafetyMachine::safe(BootReason reason) const noexcept {return {BootMode::SafeMode,reason,true,true,false,true,false,false};}
BootState BootSafetyMachine::begin(const BootRecord& record,const BootInputs& inputs) noexcept {
 record_=record;BootReason reason=BootReason::Normal;
 if(inputs.octMinusHeld)reason=BootReason::OctMinus;
 else if(record_.incomplete)reason=BootReason::PreviousIncomplete;
 else if(record_.earlyFailures>=maxEarlyFailures_)reason=BootReason::RepeatedFailures;
 else if(!inputs.storageCrcOk)reason=BootReason::StorageCrc;
 else if(!inputs.audioInitOk)reason=BootReason::AudioInit;
 else if(!inputs.uiInitOk)reason=BootReason::UiInit;
 if(reason!=BootReason::Normal){record_.lastReason=reason;record_.bootGood=false;state_=safe(reason);return state_;}
 record_.incomplete=true;record_.bootGood=false;record_.lastReason=BootReason::Normal;state_={BootMode::Normal,BootReason::Normal,false,false,true,true,true,false};return state_;
}
bool BootSafetyMachine::markStable(std::uint32_t runtimeMs) noexcept {
 if(state_.mode!=BootMode::Normal||!state_.bootInProgress||runtimeMs<stableWindowMs_)return false;
 record_.incomplete=false;record_.earlyFailures=0;record_.bootGood=true;state_.bootInProgress=false;state_.bootGood=true;return true;
}
BootState BootSafetyMachine::failEarly(BootReason reason) noexcept {
 if(reason==BootReason::Normal)reason=BootReason::AudioInit;
 record_.incomplete=true;
 record_.earlyFailures=static_cast<std::uint8_t>(std::min<unsigned>(255,record_.earlyFailures+1));
 record_.bootGood=false;
 record_.lastReason=reason;
 state_=safe(reason);
 return state_;
}

}
