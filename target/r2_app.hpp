#pragma once
#include "../engine/boot_state.hpp"
#include "../engine/watchdog.hpp"
#include "../hal/target_contract.hpp"
#include <cstdint>

namespace etch::target {
struct R2Platform {
 virtual ~R2Platform()=default;
 virtual std::uint32_t millis() noexcept=0;
 virtual bool octMinusHeldAtBoot() noexcept=0;
 virtual void muteAudio(bool muted) noexcept=0;
 virtual void showBootSplash(const char* line1,const char* line2) noexcept=0;
 virtual void showSafeMode(BootReason reason) noexcept=0;
 virtual void serviceUsbUpdater() noexcept=0;
 virtual bool pollControl(hal::ControlEvent& event) noexcept=0;
};

class R2Application {
public:
 static constexpr const char* SplashLine1="SikkenWorks";
 static constexpr const char* SplashLine2="Born out of passion";
 explicit R2Application(R2Platform& platform) noexcept;
 BootState begin(BootRecord persisted={},bool storageCrcOk=true) noexcept;
 void tick() noexcept;
 const BootState& state() const noexcept {return boot_.state();}
 const BootRecord& record() const noexcept {return boot_.record();}
 std::size_t droppedControlEvents() const noexcept {return input_.dropped();}
 bool stable() const noexcept {return boot_.state().bootGood;}
private:
 void enterSafe(BootReason reason) noexcept;
 R2Platform& platform_;
 BootSafetyMachine boot_{3000,2};
 WatchdogSupervisor watchdog_{750};
 hal::ControlQueue input_{};
 std::uint32_t startedMs_=0;
 bool began_=false;
};
}
