#pragma once
#include "q15_voice.hpp"
#include "../engine/fixed_queue.hpp"

namespace etch::target {
// Single audio-thread owner. Producer queues must be drained by the board
// bridge into enqueue() on this same thread; this queue is not an IRQ SPSC.
class Q15Runtime {
public:
 struct Event { enum class Kind : std::uint8_t {On,Off,Sustain,Panic}; Kind kind; std::uint8_t note=0,velocity=0; std::uint16_t offset=0; };
 explicit Q15Runtime(const Bank& bank) noexcept : bank_(bank) {}
 void configure(const Q15VoiceConfig&,const Q15OscConfig&,const Q15SvfCoeffs&) noexcept;
 void setNoteIncrement(std::uint8_t note,std::uint32_t increment) noexcept;
 bool enqueue(Event event) noexcept;
 // Mono Q15 output. No heap, locks, flash, table rebuild or float conversion.
 // Events are supplied in nondecreasing offset order for this block only.
 bool render(std::int16_t* output,std::size_t frames) noexcept;
 void panic() noexcept;
 std::size_t activeVoices() const noexcept;
private:
 struct Slot { Q15VoiceCore voice{}; std::uint8_t note=0; bool held=false; };
 void apply(Event) noexcept;
 const Bank& bank_;
 std::array<Slot,VoiceCount> slots_{};
 std::array<std::uint32_t,128> increments_{};
 FixedQueue<Event,128> events_{};
 Q15VoiceConfig voiceConfig_{};
 Q15OscConfig oscConfig_{};
 Q15SvfCoeffs filter_{};
 std::size_t steal_=0;
 std::uint16_t lastOffset_=0;
 bool sustain_=false,overflow_=false;
};
}
