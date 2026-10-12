#include "fm1_platform_port.hpp"

// Copying this template is not verification. A real bridge must set this to 1
// only after every TODO below has been replaced with mappings verified on the
// exact FM-1 revision. The build scripts reject marker 0 and remaining TODOs.
#define ETCH_FM1_BOARD_BRIDGE_VERIFIED_V1 0

// TEMPLATE ONLY. Copy to fm1_sdk_bridge.cpp after R0 recovery is proven and
// replace every TODO with a mapping verified on the exact FM-1 hardware.
// Never guess GPIOs, LCD commands, updater addresses or persistent flash pages.

namespace {
std::uint32_t board_millis() noexcept { return 0; /* TODO verified timer */ }
bool board_oct_minus() noexcept { return false; /* TODO verified OCT- scan */ }
void board_mute(bool) noexcept { /* TODO verified codec/DAC mute */ }
void board_splash(const char*,const char*) noexcept { /* TODO verified 240x240 LCD driver */ }
void board_safe(etch::BootReason) noexcept { /* TODO verified Safe Mode display */ }
void board_updater() noexcept { /* TODO verified existing updater service/entry */ }
bool board_control(etch::hal::ControlEvent&) noexcept { return false; /* TODO verified scan */ }
}

extern "C" etch::fm1test::BoardHooks etch_fm1_make_verified_board_hooks() noexcept {
 return {board_millis,board_oct_minus,board_mute,board_splash,board_safe,board_updater,board_control};
}
extern "C" etch::BootRecord etch_fm1_load_boot_record_read_only() noexcept { return {}; }
extern "C" void etch_fm1_store_boot_record(const etch::BootRecord&) noexcept {}
extern "C" bool etch_fm1_storage_crc_ok_read_only() noexcept { return true; }
