#include "q15_oscillator.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace etch::target {
namespace {
constexpr std::int32_t Q15One=32767;
std::int16_t clampQ15(std::int64_t v) noexcept {
 return static_cast<std::int16_t>(std::clamp<std::int64_t>(v,-32768,32767));
}
std::int16_t mulQ15(std::int32_t a,std::int32_t b) noexcept {
 const auto p=static_cast<std::int64_t>(a)*static_cast<std::int64_t>(b);
 const auto rounded=p>=0?p+16384:p-16384;
 return clampQ15(rounded/32768);
}
std::uint16_t curveMorph(std::uint16_t m,Curve curve) noexcept {
 const auto x=static_cast<std::int32_t>(std::min<std::uint16_t>(m,32767));
 if(curve==Curve::Linear)return static_cast<std::uint16_t>(x);
 if(curve==Curve::S){
  const auto x2=static_cast<std::int32_t>(mulQ15(x,x));
  const auto twoX=static_cast<std::int32_t>(std::min(65534,2*x));
  const auto factor=98301-twoX; // 3.0 - 2x, Q15
  return static_cast<std::uint16_t>(std::clamp<std::int32_t>(static_cast<std::int32_t>((static_cast<std::int64_t>(x2)*factor+16384)/32768),0,32767));
 }
 if(x<16384){
  const auto x2=static_cast<std::int32_t>(mulQ15(x,x));
  return static_cast<std::uint16_t>(std::min(32767,2*x2));
 }
 const auto inv=32767-x;
 const auto inv2=static_cast<std::int32_t>(mulQ15(inv,inv));
 return static_cast<std::uint16_t>(std::clamp(32767-2*inv2,0,32767));
}
std::int16_t invertQ15(std::int16_t value,bool invert) noexcept {
 if(!invert)return value;
 return value==std::numeric_limits<std::int16_t>::min()?std::numeric_limits<std::int16_t>::max():static_cast<std::int16_t>(-value);
}
}

std::uint32_t phaseIncrement(double frequency,double sampleRate) noexcept {
 if(!std::isfinite(frequency)||!std::isfinite(sampleRate)||frequency<=0||sampleRate<=0)return 0;
 const long double ratio=static_cast<long double>(frequency)/static_cast<long double>(sampleRate);
 if(ratio>=1)return std::numeric_limits<std::uint32_t>::max();
 const auto scaled=ratio*static_cast<long double>(PhaseScale);
 if(scaled<=0)return 0;
 if(scaled>=static_cast<long double>(std::numeric_limits<std::uint32_t>::max()))return std::numeric_limits<std::uint32_t>::max();
 return static_cast<std::uint32_t>(std::llround(scaled));
}

std::int16_t readTableQ15(const Table& table,std::uint32_t phase) noexcept {
 const auto index=static_cast<std::size_t>(phase>>PhaseFracBits)&(TableSize-1u);
 const auto next=(index+1u)&(TableSize-1u);
 const auto fraction=phase&PhaseFracMask;
 const auto a=static_cast<std::int32_t>(table[index]);
 const auto delta=static_cast<std::int32_t>(table[next])-a;
 const auto interpolated=static_cast<std::int64_t>(a)+(static_cast<std::int64_t>(delta)*fraction+(1ll<<(PhaseFracBits-1u)))/(1ll<<PhaseFracBits);
 return clampQ15(interpolated);
}

std::int16_t q15Lerp(std::int16_t a,std::int16_t b,std::uint16_t amount) noexcept {
 const auto m=static_cast<std::int32_t>(std::min<std::uint16_t>(amount,32767));
 const auto delta=static_cast<std::int32_t>(b)-static_cast<std::int32_t>(a);
 return clampQ15(static_cast<std::int32_t>(a)+(static_cast<std::int64_t>(delta)*m+16384)/32768);
}

std::int16_t readWaveQ15(const Wave& wave,std::uint32_t phase,std::uint32_t increment) noexcept {
 if(increment==0)return readTableQ15(wave[0],phase);
 std::size_t level=0;
 while(level<5u&&static_cast<std::uint64_t>(increment)*static_cast<std::uint64_t>(Harmonics[level])>NyquistGuardQ32)++level;
 if(level==5u){
  const auto sine=readTableQ15(wave[5],phase);
  if(increment>=NyquistGuardQ32)return 0;
  const auto distance=NyquistGuardQ32-increment;
  const auto fade=static_cast<std::uint16_t>(std::min<std::uint64_t>(32767ull,(static_cast<std::uint64_t>(distance)*32767ull)/NyquistFadeQ32));
  return mulQ15(sine,fade);
 }
 const auto denominator=static_cast<std::uint64_t>(increment)*static_cast<std::uint64_t>(Harmonics[level]);
 std::uint16_t mix=32767;
 if(denominator>0){
  if(denominator>=NyquistGuardQ32)mix=0;
  else {
   const auto numerator=static_cast<std::uint64_t>(NyquistGuardQ32)-denominator;
   mix=static_cast<std::uint16_t>(std::min<std::uint64_t>(32767ull,(numerator*32767ull)/denominator));
  }
 }
 const auto limited=readTableQ15(wave[level+1u],phase);
 const auto brighter=readTableQ15(wave[level],phase);
 return q15Lerp(limited,brighter,mix);
}

std::int16_t carrierSampleQ15(const Bank& bank,
 std::uint32_t carrierPhase,std::uint32_t modPhase,
 std::uint32_t carrierIncrement,std::uint32_t modIncrement,
 const Q15OscConfig& config) noexcept {
 const auto c=readWaveQ15(bank[2],modPhase,modIncrement);
 auto morph=static_cast<std::int32_t>(std::min<std::uint16_t>(config.morph,32767));
 auto phase=carrierPhase;
 const auto modulated=static_cast<std::int32_t>(mulQ15(c,config.depth));
 if(config.target==Target::Morph)morph=std::clamp(morph+modulated,0,32767);
 else {
  const auto offset=(static_cast<std::int64_t>(modulated)*static_cast<std::int64_t>(PhaseScale))/131072ll;
  phase=static_cast<std::uint32_t>(static_cast<std::uint64_t>(phase)+static_cast<std::uint64_t>(offset));
 }
 const auto shaped=curveMorph(static_cast<std::uint16_t>(morph),config.curve);
 const auto a=invertQ15(readWaveQ15(bank[0],phase+config.phaseA,carrierIncrement),config.invertA);
 const auto b=invertQ15(readWaveQ15(bank[1],phase+config.phaseB,carrierIncrement),config.invertB);
 return q15Lerp(a,b,shaped);
}

std::int16_t Q15OscillatorCore::next(const Bank& bank) noexcept {
 const auto out=carrierSampleQ15(bank,carrierPhase_,modPhase_,carrierIncrement_,modIncrement_,config_);
 carrierPhase_+=carrierIncrement_;
 modPhase_+=modIncrement_;
 return out;
}
}
