#include "fm1_platform_port.hpp"

namespace etch::fm1test {
bool Fm1R2Platform::complete() const noexcept {
 return hooks_.millis&&hooks_.octMinusHeldAtBoot&&hooks_.muteAudio&&hooks_.showSplash&&
        hooks_.showSafeMode&&hooks_.serviceUsbUpdater&&hooks_.pollControl;
}
std::uint32_t Fm1R2Platform::millis() noexcept {return hooks_.millis?hooks_.millis():0;}
bool Fm1R2Platform::octMinusHeldAtBoot() noexcept {return hooks_.octMinusHeldAtBoot?hooks_.octMinusHeldAtBoot():false;}
void Fm1R2Platform::muteAudio(bool muted) noexcept {if(hooks_.muteAudio)hooks_.muteAudio(muted);}
void Fm1R2Platform::showBootSplash(const char* a,const char* b) noexcept {if(hooks_.showSplash)hooks_.showSplash(a,b);}
void Fm1R2Platform::showSafeMode(etch::BootReason r) noexcept {if(hooks_.showSafeMode)hooks_.showSafeMode(r);}
void Fm1R2Platform::serviceUsbUpdater() noexcept {if(hooks_.serviceUsbUpdater)hooks_.serviceUsbUpdater();}
bool Fm1R2Platform::pollControl(etch::hal::ControlEvent& e) noexcept {return hooks_.pollControl?hooks_.pollControl(e):false;}
}
