#pragma once
#include "../engine/fixed_queue.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace etch::hal {
constexpr std::size_t LcdWidth=240,LcdHeight=240;
constexpr std::size_t KeyCount=27;
constexpr std::size_t ControlEventCapacity=128;
constexpr std::size_t MidiEventCapacity=256;
constexpr std::size_t UiCommandCapacity=128;

enum class ControlId : std::uint8_t {
 Master,Select,Presets,Algorithm,Knob1,Knob2,Knob3,Knob4,
 Home,Save,Arp,Seq,PlayStop,Rec,Fx,Sel,Env,Lfo,Edit,Glo,OctMinus,OctPlus,
 Key0
};
enum class EventType : std::uint8_t {EncoderDelta,ButtonDown,ButtonUp,KeyDown,KeyUp};

struct ControlEvent {
 std::uint32_t timestampMs=0;
 std::int16_t value=0;
 ControlId control=ControlId::Select;
 EventType type=EventType::EncoderDelta;
 std::uint8_t velocity=0;
 std::uint8_t reserved=0;
};
static_assert(sizeof(ControlEvent)<=12,"HAL control event must remain compact");

struct MidiPacket {
 std::uint32_t timestampSamples=0;
 std::array<std::uint8_t,3> bytes{};
 std::uint8_t size=0;
};
static_assert(sizeof(MidiPacket)<=8,"MIDI packet must remain compact");

struct UiCommand {
 enum class Kind : std::uint8_t {SetPage,Invalidate,SetKeyLeds,SetBattery,ShowBootReason,EnterSafeMode};
 Kind kind=Kind::Invalidate;
 std::uint8_t a=0,b=0,c=0;
 std::uint32_t value=0;
};
static_assert(sizeof(UiCommand)<=8,"UI command must remain compact");

struct KeyLedSnapshot {
 std::array<std::uint32_t,4> bits{};
 void clear() noexcept {bits.fill(0);}
 void set(std::uint8_t midiNote,bool on=true) noexcept {
  const auto word=static_cast<std::size_t>(midiNote>>5);
  const auto bit=static_cast<std::uint32_t>(1u<<(midiNote&31));
  if(word>=bits.size())return;
  if(on)bits[word]|=bit;
  else bits[word]&=~bit;
 }
 bool test(std::uint8_t midiNote) const noexcept {const auto word=static_cast<std::size_t>(midiNote>>5);return word<bits.size()&&(bits[word]&(1u<<(midiNote&31)));}
};

struct BatterySnapshot {std::uint8_t percent=0;bool charging=false;bool present=false;};

using ControlQueue=etch::FixedQueue<ControlEvent,ControlEventCapacity>;
using MidiQueue=etch::FixedQueue<MidiPacket,MidiEventCapacity>;
using UiQueue=etch::FixedQueue<UiCommand,UiCommandCapacity>;

struct RuntimeQueues {ControlQueue controls{};MidiQueue midi{};UiQueue ui{};};
}
