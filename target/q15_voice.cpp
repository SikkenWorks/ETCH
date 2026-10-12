#include "q15_voice.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace etch::target {
namespace {
std::int16_t clampQ15(std::int64_t v) noexcept {
 return static_cast<std::int16_t>(std::clamp<std::int64_t>(v,-32768,32767));
}
std::uint16_t stepForSeconds(double seconds,double sampleRate,std::uint16_t span) noexcept {
 if(!std::isfinite(seconds)||!std::isfinite(sampleRate)||sampleRate<=0)return 32767;
 if(seconds<=0)return 32767;
 const auto samples=std::max(1.0,seconds*sampleRate);
 const auto step=static_cast<long long>(std::ceil(static_cast<double>(span)/samples));
 return static_cast<std::uint16_t>(std::clamp<long long>(step,1,32767));
}
std::int32_t mulQ15Wide(std::int32_t a,std::int32_t b) noexcept {
 const auto p=static_cast<std::int64_t>(a)*static_cast<std::int64_t>(b);
 return static_cast<std::int32_t>((p+(p>=0?16384:-16384))/32768);
}
}

std::int16_t q15MulSample(std::int32_t a,std::int32_t b) noexcept {
 return clampQ15(mulQ15Wide(a,b));
}

std::int16_t q15FromUnit(double value) noexcept {
 if(!std::isfinite(value))return 0;
 const auto v=std::clamp(value,-1.0,1.0);
 return static_cast<std::int16_t>(std::lround(v*(v<0?32768.0:32767.0)));
}

std::uint16_t q15UnsignedFromUnit(double value) noexcept {
 if(!std::isfinite(value))return 0;
 return static_cast<std::uint16_t>(std::lround(std::clamp(value,0.0,1.0)*32767.0));
}

Q15AdsrConfig adsrConfigFromSeconds(double attack,double decay,double sustain,double release,double sampleRate) noexcept {
 Q15AdsrConfig c{};
 c.sustain=q15UnsignedFromUnit(sustain);
 c.attackStep=stepForSeconds(attack,sampleRate,32767);
 c.decayStep=stepForSeconds(decay,sampleRate,static_cast<std::uint16_t>(32767u-c.sustain));
 c.releaseStep=stepForSeconds(release,sampleRate,32767);
 return c;
}

std::uint16_t Q15Adsr::next() noexcept {
 switch(stage_){
  case Stage::Idle: value_=0; break;
  case Stage::Attack: {
   const auto next=static_cast<std::uint32_t>(value_)+std::max<std::uint16_t>(1,config_.attackStep);
   if(next>=32767u){value_=32767;stage_=Stage::Decay;}else value_=static_cast<std::uint16_t>(next);
   break;
  }
  case Stage::Decay:
   if(value_<=config_.sustain){value_=config_.sustain;stage_=Stage::Sustain;}
   else {
    const auto step=std::max<std::uint16_t>(1,config_.decayStep);
    value_=value_>step?static_cast<std::uint16_t>(value_-step):0;
    if(value_<=config_.sustain){value_=config_.sustain;stage_=Stage::Sustain;}
   }
   break;
  case Stage::Sustain: value_=config_.sustain; break;
  case Stage::Release: {
   const auto step=std::max<std::uint16_t>(1,config_.releaseStep);
   if(value_<=step){value_=0;stage_=Stage::Idle;} else value_=static_cast<std::uint16_t>(value_-step);
   break;
  }
 }
 return value_;
}

Q15SvfCoeffs svfCoeffs(double cutoffHz,double resonance,double sampleRate) noexcept {
 Q15SvfCoeffs c{};
 if(!std::isfinite(cutoffHz)||!std::isfinite(resonance)||!std::isfinite(sampleRate)||sampleRate<=0)return c;
 const auto cutoff=std::clamp(cutoffHz,5.0,std::min(sampleRate*.45,20000.0));
 const auto res=std::clamp(resonance,0.0,1.0);
 const auto g=std::tan(3.14159265358979323846*cutoff/sampleRate);
 const auto k=2.0-1.92*res;
 const auto a1=1.0/(1.0+g*(g+k));
 const auto a2=g*a1;
 const auto a3=g*a2;
 c.a1=q15FromUnit(std::clamp(a1,0.0,1.0));
 c.a2=q15FromUnit(std::clamp(a2,0.0,1.0));
 c.a3=q15FromUnit(std::clamp(a3,0.0,1.0));
 c.k=q15FromUnit(std::clamp(k/2.0,0.0,1.0)); // store k/2; doubled in sample path
 return c;
}

std::int16_t Q15Svf::process(std::int16_t input,const Q15SvfCoeffs& c,Filter mode) noexcept {
 if(mode==Filter::Off)return input;
 const auto v3=static_cast<std::int32_t>(input)-ic2_;
 const auto v1=mulQ15Wide(c.a1,ic1_)+mulQ15Wide(c.a2,v3);
 const auto v2=ic2_+mulQ15Wide(c.a2,ic1_)+mulQ15Wide(c.a3,v3);
 ic1_=std::clamp<std::int32_t>(2*v1-ic1_,-131072,131071);
 ic2_=std::clamp<std::int32_t>(2*v2-ic2_,-131072,131071);
 if(mode==Filter::LP)return clampQ15(v2);
 if(mode==Filter::BP)return clampQ15(v1);
 const auto kTimesBand=mulQ15Wide(static_cast<std::int32_t>(c.k)*2,v1);
 return clampQ15(static_cast<std::int32_t>(input)-kTimesBand-v2);
}

std::int16_t q15Drive(std::int16_t sample,std::uint16_t amount) noexcept {
 const auto a=std::min<std::uint16_t>(amount,32767);
 if(a==0)return sample;
 // 1x..5x pre-gain, then a fixed-point rational soft clip. No float/divide-by-zero state.
 const auto gain=32768+static_cast<std::int32_t>((static_cast<std::int64_t>(a)*4));
 auto x=static_cast<std::int32_t>((static_cast<std::int64_t>(sample)*gain)/32768);
 const auto denom=32768+std::abs(x);
 auto clipped=static_cast<std::int32_t>((static_cast<std::int64_t>(x)*65535)/std::max(1,denom));
 // Gentle makeup so drive changes colour more than loudness.
 const auto makeup=32768+static_cast<std::int32_t>((static_cast<std::int64_t>(a)*8192)/32767);
 clipped=static_cast<std::int32_t>((static_cast<std::int64_t>(clipped)*makeup)/32768);
 return clampQ15(clipped);
}

std::int16_t q15SafetyLimit(std::int32_t sample) noexcept {
 // Final fail-safe limiter. Musical saturation belongs to q15Drive(); this stage
 // only guarantees that an 8-voice accumulation can never wrap the DAC word.
 return clampQ15(sample);
}

void Q15VoiceCore::reset() noexcept {
 oscillator_.reset();amp_.reset();filterEnv_.reset();filter_.reset();
}

void Q15VoiceCore::setOscillator(const Q15OscConfig& config,std::uint32_t carrierIncrement,std::uint32_t modIncrement) noexcept {
 oscillator_.setConfig(config);oscillator_.setIncrements(carrierIncrement,modIncrement);
}

void Q15VoiceCore::setConfig(const Q15VoiceConfig& config) noexcept {
 config_=config;amp_.setConfig(config.amp);filterEnv_.setConfig(config.filterEnv);
}

std::int16_t Q15VoiceCore::next(const Bank& bank,const Q15SvfCoeffs& filterCoeffs) noexcept {
 if(!amp_.active())return 0;
 auto sample=oscillator_.next(bank);
 const auto env=amp_.next();
 filterEnv_.next(); // envelope state is available to the future cutoff LUT selector.
 sample=q15MulSample(sample,env);
 sample=q15MulSample(sample,config_.velocity);
 sample=filter_.process(sample,filterCoeffs,config_.filter);
 sample=q15Drive(sample,config_.drive);
 sample=q15MulSample(sample,config_.gain);
 return sample;
}

std::int16_t Q15MixBus::output(std::uint16_t masterGain) const noexcept {
 if(count_==0)return 0;
 // Fixed 8-voice headroom: scale the sum by 1/4, then master gain and safety limiter.
 const auto headroom=accum_/4;
 const auto gained=mulQ15Wide(headroom,std::min<std::uint16_t>(masterGain,32767));
 return q15SafetyLimit(gained);
}

} // namespace etch::target
