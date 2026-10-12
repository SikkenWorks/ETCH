#include "r2_app.hpp"

namespace etch::target {
R2Application::R2Application(R2Platform& platform) noexcept:platform_(platform){}
BootState R2Application::begin(BootRecord persisted,bool storageCrcOk) noexcept {
 began_=true;input_.clear();startedMs_=platform_.millis();
 platform_.muteAudio(true);
 platform_.showBootSplash(SplashLine1,SplashLine2);
 BootInputs inputs{};inputs.octMinusHeld=platform_.octMinusHeldAtBoot();inputs.storageCrcOk=storageCrcOk;inputs.audioInitOk=true;inputs.uiInitOk=true;
 const auto state=boot_.begin(persisted,inputs);
 if(state.mode==BootMode::SafeMode){enterSafe(state.reason);return state;}
 watchdog_.arm(startedMs_);
 platform_.muteAudio(true); // R2 deliberately has no synth/audio engine.
 return state;
}
void R2Application::enterSafe(BootReason reason) noexcept {
 platform_.muteAudio(true);platform_.showSafeMode(reason);watchdog_.disarm();
}
void R2Application::tick() noexcept {
 if(!began_)return;
 const auto now=platform_.millis();
 platform_.serviceUsbUpdater();
 hal::ControlEvent event{};
 while(platform_.pollControl(event))if(!input_.push(event)){boot_.failEarly(BootReason::InputOverflow);enterSafe(BootReason::InputOverflow);return;}
 input_.clear(); // R2 only validates bounded scan traffic; no synth/UI routing yet.
 if(boot_.state().mode==BootMode::SafeMode){platform_.muteAudio(true);return;}
 if(watchdog_.poll(now)){boot_.failEarly(BootReason::Watchdog);enterSafe(BootReason::Watchdog);return;}
 watchdog_.kick(now);
 if(!boot_.state().bootGood)boot_.markStable(static_cast<std::uint32_t>(now-startedMs_));
}
}
