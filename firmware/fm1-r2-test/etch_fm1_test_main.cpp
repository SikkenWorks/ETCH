#include "fm1_platform_port.hpp"
#include <cstdint>

// Implement this function in fm1_sdk_bridge.cpp using ONLY hardware mappings
// verified for the exact FM-1 revision. ETCH deliberately does not guess pins.
extern "C" etch::fm1test::BoardHooks etch_fm1_make_verified_board_hooks() noexcept;
extern "C" etch::BootRecord etch_fm1_load_boot_record_read_only() noexcept;
extern "C" void etch_fm1_store_boot_record(const etch::BootRecord&) noexcept;
extern "C" bool etch_fm1_storage_crc_ok_read_only() noexcept;

extern "C" int etch_fm1_r2_test_main() noexcept {
 auto hooks=etch_fm1_make_verified_board_hooks();
 etch::fm1test::Fm1R2Platform platform(hooks);
 if(!platform.complete())return 0xE201;
 etch::target::R2Application app(platform);
 auto record=etch_fm1_load_boot_record_read_only();
 const auto state=app.begin(record,etch_fm1_storage_crc_ok_read_only());
 etch_fm1_store_boot_record(app.record());
 (void)state;
 for(;;){
  app.tick();
  etch_fm1_store_boot_record(app.record());
 }
}
