#pragma once
#include "q15_oscillator.hpp"
#include "../engine/etch.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace etch::target {

constexpr std::int16_t Q15Max = 32767;
constexpr std::int16_t Q15Min = -32768;

std::int16_t q15MulSample(std::int32_t a, std::int32_t b) noexcept;
std::int16_t q15FromUnit(double value) noexcept;
std::uint16_t q15UnsignedFromUnit(double value) noexcept;

struct Q15AdsrConfig {
 std::uint16_t attackStep = 32767;
 std::uint16_t decayStep = 32767;
 std::uint16_t sustain = 22937; // 0.70
 std::uint16_t releaseStep = 32767;
};

Q15AdsrConfig adsrConfigFromSeconds(double attack, double decay, double sustain,
                                     double release, double sampleRate) noexcept;

class Q15Adsr {
public:
 enum class Stage : std::uint8_t {Idle, Attack, Decay, Sustain, Release};
 void setConfig(const Q15AdsrConfig& config) noexcept { config_ = config; }
 void noteOn() noexcept { stage_ = Stage::Attack; if(config_.attackStep>=32767u){value_=32767; stage_=Stage::Decay;} }
 void noteOff() noexcept { if(stage_!=Stage::Idle) stage_ = Stage::Release; }
 void reset() noexcept { value_=0; stage_=Stage::Idle; }
 std::uint16_t next() noexcept;
 std::uint16_t value() const noexcept { return value_; }
 Stage stage() const noexcept { return stage_; }
 bool active() const noexcept { return stage_ != Stage::Idle; }
private:
 Q15AdsrConfig config_{};
 std::uint16_t value_ = 0;
 Stage stage_ = Stage::Idle;
};
static_assert(sizeof(Q15Adsr)<=16,"Target ADSR state must remain compact");

struct Q15SvfCoeffs {
 std::int16_t a1 = 32767;
 std::int16_t a2 = 0;
 std::int16_t a3 = 0;
 std::int16_t k = 0;
};

Q15SvfCoeffs svfCoeffs(double cutoffHz, double resonance, double sampleRate) noexcept;

class Q15Svf {
public:
 void reset() noexcept { ic1_=0; ic2_=0; }
 std::int16_t process(std::int16_t input, const Q15SvfCoeffs& c, Filter mode) noexcept;
 std::int32_t ic1() const noexcept { return ic1_; }
 std::int32_t ic2() const noexcept { return ic2_; }
private:
 std::int32_t ic1_ = 0;
 std::int32_t ic2_ = 0;
};
static_assert(sizeof(Q15Svf)<=8,"Target SVF state must remain compact");

std::int16_t q15Drive(std::int16_t sample, std::uint16_t amount) noexcept;
std::int16_t q15SafetyLimit(std::int32_t sample) noexcept;

struct Q15VoiceConfig {
 Q15AdsrConfig amp{};
 Q15AdsrConfig filterEnv{};
 std::uint16_t velocity = 32767;
 std::uint16_t gain = 32767;
 std::uint16_t drive = 0;
 Filter filter = Filter::Off;
};

class Q15VoiceCore {
public:
 void reset() noexcept;
 void setOscillator(const Q15OscConfig& config, std::uint32_t carrierIncrement,
                    std::uint32_t modIncrement) noexcept;
 void setConfig(const Q15VoiceConfig& config) noexcept;
 void noteOn() noexcept { amp_.noteOn(); filterEnv_.noteOn(); }
 void noteOff() noexcept { amp_.noteOff(); filterEnv_.noteOff(); }
 std::int16_t next(const Bank& bank, const Q15SvfCoeffs& filterCoeffs) noexcept;
 bool active() const noexcept { return amp_.active(); }
 std::uint16_t ampValue() const noexcept { return amp_.value(); }
 std::uint16_t filterEnvValue() const noexcept { return filterEnv_.value(); }
 Q15OscillatorCore& oscillator() noexcept { return oscillator_; }
 const Q15OscillatorCore& oscillator() const noexcept { return oscillator_; }
private:
 Q15OscillatorCore oscillator_{};
 Q15Adsr amp_{};
 Q15Adsr filterEnv_{};
 Q15Svf filter_{};
 Q15VoiceConfig config_{};
};
static_assert(sizeof(Q15VoiceCore)<=104,"Target voice state must remain bounded");

class Q15MixBus {
public:
 static constexpr std::size_t MaxVoices = VoiceCount;
 void clear() noexcept { accum_=0; count_=0; }
 void add(std::int16_t sample) noexcept { accum_ += sample; if(count_<MaxVoices) ++count_; }
 std::int16_t output(std::uint16_t masterGain=32767) const noexcept;
 std::int32_t accumulator() const noexcept { return accum_; }
 std::uint8_t count() const noexcept { return count_; }
private:
 std::int32_t accum_ = 0;
 std::uint8_t count_ = 0;
};
static_assert(sizeof(Q15MixBus)<=8,"Target mix state must remain tiny");

} // namespace etch::target
