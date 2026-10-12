#pragma once
#include "../engine/etch.hpp"
#include <cstdint>

namespace etch::target {
// Allocation-free target oscillator reference. The realtime sample path uses
// integer/Q-format math only; floating point is restricted to control-thread
// helpers such as phaseIncrement().
constexpr std::uint32_t PhaseIndexBits=9u;
constexpr std::uint32_t PhaseFracBits=32u-PhaseIndexBits;
constexpr std::uint32_t PhaseFracMask=(1u<<PhaseFracBits)-1u;
constexpr std::uint64_t PhaseScale=0x100000000ull;
constexpr std::uint32_t NyquistGuardQ32=1932735283u; // round(0.45 * 2^32)
constexpr std::uint32_t NyquistFadeQ32=214748365u;   // round(0.05 * 2^32)

std::uint32_t phaseIncrement(double frequency,double sampleRate) noexcept;
std::int16_t readTableQ15(const Table& table,std::uint32_t phase) noexcept;
std::int16_t readWaveQ15(const Wave& wave,std::uint32_t phase,std::uint32_t increment) noexcept;
std::int16_t q15Lerp(std::int16_t a,std::int16_t b,std::uint16_t amount) noexcept;

struct Q15OscConfig {
 std::uint16_t morph=0;
 std::uint16_t depth=0;
 std::uint32_t phaseA=0;
 std::uint32_t phaseB=0;
 bool invertA=false;
 bool invertB=false;
 Target target=Target::Phase;
 Curve curve=Curve::Linear;
};

std::int16_t carrierSampleQ15(const Bank& bank,
 std::uint32_t carrierPhase,std::uint32_t modPhase,
 std::uint32_t carrierIncrement,std::uint32_t modIncrement,
 const Q15OscConfig& config) noexcept;

class Q15OscillatorCore {
public:
 void reset(std::uint32_t carrierPhase=0,std::uint32_t modPhase=0) noexcept {carrierPhase_=carrierPhase;modPhase_=modPhase;}
 void setIncrements(std::uint32_t carrier,std::uint32_t modulator) noexcept {carrierIncrement_=carrier;modIncrement_=modulator;}
 void setConfig(const Q15OscConfig& config) noexcept {config_=config;}
 std::int16_t next(const Bank& bank) noexcept;
 std::uint32_t carrierPhase() const noexcept {return carrierPhase_;}
 std::uint32_t modulatorPhase() const noexcept {return modPhase_;}
private:
 std::uint32_t carrierPhase_=0,modPhase_=0,carrierIncrement_=0,modIncrement_=0;
 Q15OscConfig config_{};
};
static_assert(sizeof(Q15OscillatorCore)<=48,"Target oscillator state must stay tiny");
}
