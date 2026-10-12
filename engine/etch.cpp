#include "etch.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace etch {
namespace {constexpr double Pi=3.14159265358979323846;
float table(const Table& t,double p) noexcept {p=(p-std::floor(p))*TableSize;const auto whole=static_cast<std::size_t>(p);const auto i=whole%TableSize;const auto f=static_cast<float>(p-whole);return (t[i]+(t[(i+1)%TableSize]-t[i])*f)/32768.f;}
bool bound(float v,float low,float high){return std::isfinite(v)&&v>=low&&v<=high;}
}
bool validWaveFx(const WaveFx& fx) noexcept {
 return bound(fx.drive,0,1)&&bound(fx.bitcrush,0,1)&&bound(fx.chorus,0,1)&&bound(fx.delay,0,1)&&bound(fx.reverb,0,1)&&bound(fx.fold,0,1)&&bound(fx.smooth,0,1)&&bound(fx.mix,0,1);
}
namespace {
constexpr double WaveFxTargetRms=.5,WaveFxPeak=.891250938;
std::size_t wrapIndex(long i,std::size_t n) noexcept {const long m=static_cast<long>(n),r=i%m;return static_cast<std::size_t>(r<0?r+m:r);}
using Cycle=std::array<double,PointCount>;
Cycle mixCycle(const Cycle& a,const Cycle& b,double amount){Cycle out{};const auto m=std::clamp(amount,0.,1.);for(std::size_t i=0;i<PointCount;i++)out[i]=a[i]*(1-m)+b[i]*m;return out;}
Cycle baseCycle(const Points& points){double mean=0;for(auto x:points)mean+=x;mean/=PointCount;Cycle out{};double peak=0;for(std::size_t i=0;i<PointCount;i++){out[i]=(points[i]-mean)/32768.;peak=std::max(peak,std::abs(out[i]));}if(peak>0){const auto s=WaveFxPeak/peak;for(auto& x:out)x*=s;}return out;}
Cycle normalizeMorph(Cycle out){double mean=0;for(auto x:out)mean+=x;mean/=PointCount;double rms=0,peak=0;for(auto& x:out){x-=mean;rms+=x*x;peak=std::max(peak,std::abs(x));}rms=std::sqrt(rms/PointCount);if(rms<1e-12)return out;double scale=WaveFxTargetRms/rms;if(peak*scale>WaveFxPeak)scale=WaveFxPeak/peak;for(auto& x:out)x*=scale;return out;}
Cycle processWaveFx(const Points& points,const WaveFx& fx){
 if(!validWaveFx(fx))throw std::invalid_argument("Invalid wave FX");
 const auto dry=baseCycle(points);auto wet=dry;
 if(fx.drive>0){Cycle p{};const double gain=1+11*fx.drive,norm=std::tanh(gain);for(std::size_t i=0;i<PointCount;i++)p[i]=std::tanh(gain*wet[i])/(norm?norm:1);wet=mixCycle(wet,p,fx.drive);}
 if(fx.fold>0){Cycle p{};const double gain=1+5*fx.fold;for(std::size_t i=0;i<PointCount;i++)p[i]=(2/Pi)*std::asin(std::sin((Pi/2)*gain*wet[i]));wet=mixCycle(wet,p,fx.fold);}
 if(fx.bitcrush>0){Cycle p{};const int bits=std::max(3,static_cast<int>(std::lround(16-13*fx.bitcrush))),levels=(1<<std::min(bits,15))-1,hold=1+static_cast<int>(std::floor(7*fx.bitcrush*fx.bitcrush));double held=0;for(std::size_t i=0;i<PointCount;i++){if(i%hold==0){const auto n=std::clamp((wet[i]+1)*.5,0.,1.);held=(std::lround(n*levels)/static_cast<double>(levels))*2-1;}p[i]=held;}wet=mixCycle(wet,p,fx.bitcrush);}
 if(fx.chorus>0){Cycle p{};const int off=1+static_cast<int>(std::lround(7*fx.chorus));for(std::size_t i=0;i<PointCount;i++)p[i]=.5*wet[i]+.25*wet[wrapIndex(static_cast<long>(i)-off,PointCount)]+.25*wet[wrapIndex(static_cast<long>(i)+off,PointCount)];wet=mixCycle(wet,p,.78*fx.chorus);}
 if(fx.delay>0){Cycle p{};const int d=std::max(1,static_cast<int>(std::lround(PointCount*(.08+.30*fx.delay))));for(std::size_t i=0;i<PointCount;i++)p[i]=.62*wet[wrapIndex(static_cast<long>(i)-d,PointCount)]+.25*wet[wrapIndex(static_cast<long>(i)-2*d,PointCount)]+.13*wet[wrapIndex(static_cast<long>(i)-3*d,PointCount)];wet=mixCycle(wet,p,.72*fx.delay);}
 if(fx.reverb>0){constexpr std::array<int,5> offsets{3,11,23,37,53};constexpr std::array<double,5> weights{1,.78,.59,.43,.31};constexpr double weightSum=3.11;Cycle p{};for(std::size_t i=0;i<PointCount;i++){double v=0;for(std::size_t k=0;k<offsets.size();k++)v+=wet[wrapIndex(static_cast<long>(i)-offsets[k],PointCount)]*weights[k];p[i]=v/weightSum;}wet=mixCycle(wet,p,.68*fx.reverb);}
 if(fx.smooth>0){auto p=wet;const int iterations=1+static_cast<int>(std::floor(4*fx.smooth));for(int k=0;k<iterations;k++){Cycle next{};for(std::size_t i=0;i<PointCount;i++)next[i]=.25*p[wrapIndex(static_cast<long>(i)-1,PointCount)]+.5*p[i]+.25*p[wrapIndex(static_cast<long>(i)+1,PointCount)];p=next;}wet=mixCycle(wet,p,fx.smooth);}
 return normalizeMorph(mixCycle(dry,wet,fx.mix));
}
}
Points renderWaveFxPoints(const Points& points,const WaveFx& fx){const auto src=processWaveFx(points,fx);Points out{};for(std::size_t i=0;i<PointCount;i++)out[i]=static_cast<int16_t>(std::lround(std::clamp(src[i],-.999969482421875,.999969482421875)*32768.));return out;}
Wave compileWave(const Points& points,const WaveFx& fx){
 const auto source=processWaveFx(points,fx);
 std::array<double,65> re{},im{};
 for(int k=1;k<=64;k++)for(std::size_t j=0;j<PointCount;j++){const double a=2*Pi*k*j/PointCount;re[k]+=source[j]*std::cos(a)/PointCount;im[k]-=source[j]*std::sin(a)/PointCount;}
 std::array<std::array<double,TableSize>,Levels> floating{};double max=0;
 for(std::size_t level=0;level<Levels;level++)for(std::size_t i=0;i<TableSize;i++){
  for(int k=1;k<=Harmonics[level];k++){const double a=2*Pi*k*i/TableSize;floating[level][i]+=(k==64?1:2)*(re[k]*std::cos(a)-im[k]*std::sin(a));}
  max=std::max(max,std::abs(floating[level][i]));
 }
 const double limiter=max>.95?.95/max:1;Wave result{};
 for(std::size_t level=0;level<Levels;level++)for(std::size_t i=0;i<TableSize;i++){const auto q=std::lround(std::clamp(floating[level][i]*limiter,-.999969482421875,.999969482421875)*32768.);result[level][i]=static_cast<int16_t>(q);}
 return result;
}
float readWave(const Wave& w,double p,double f,double sr) noexcept {
 if(f>=sr*.45)return 0;
 const auto budget=sr*.45/std::max(f,.001);std::size_t i=0;
 while(i<5&&Harmonics[i]>budget)i++;
 if(i==5)return table(w[5],p)*static_cast<float>(std::clamp((sr*.45-f)/(sr*.05),0.,1.));
 const auto mix=static_cast<float>(std::clamp(budget/Harmonics[i]-1.,0.,1.));
 return table(w[i+1],p)*(1-mix)+table(w[i],p)*mix;
}
Synth::Synth(double sr):sr_(sr),rate_(sr*Oversample),smooth_(1-std::exp(-1/(.005*rate_))),morphSmooth_(1-std::exp(-1/(.015*rate_))),fxSmooth_(1-std::exp(-1/(.02*sr_))) {
 if(!std::isfinite(sr)||sr<8000||sr>192000)throw std::invalid_argument("Invalid sample rate");
 filterMax_=std::min(18000.,.4*sr_);
 rebuildFilterTables();
 double sum=0;
 for(int i=0;i<FirTaps;i++){const double t=i-63,fc=.115,sinc=t==0?2*fc:std::sin(2*Pi*fc*t)/(Pi*t),window=.42-.5*std::cos(2*Pi*i/126)+.08*std::cos(4*Pi*i/126);kernel_[i]=sinc*window;sum+=kernel_[i];}
 for(auto& x:kernel_)x/=sum;
 auto length=[this](double seconds){return std::max<std::size_t>(8,static_cast<std::size_t>(std::ceil(sr_*seconds))+4);};
 chorusBuffer_.assign(length(.04),0.);delayBuffer_.assign(length(.26),0.);
 constexpr std::array<double,4> combSeconds{.0297,.0371,.0411,.0437};
 constexpr std::array<double,2> allpassSeconds{.005,.0017};
 for(std::size_t i=0;i<comb_.size();i++)comb_[i].buffer.assign(length(combSeconds[i]),0.);
 for(std::size_t i=0;i<allpass_.size();i++)allpass_[i].buffer.assign(length(allpassSeconds[i]),0.);
}
bool Synth::setParams(const Params& p) noexcept {
 if(!bound(p.morph,0,1)||!bound(p.depth,0,1)||!bound(p.ratio,.125f,16)||!bound(p.freeHz,.05f,2000)||!bound(p.bpm,30,300)||!bound(p.bars,1.f/64,8)||!bound(p.attack,.001f,4)||!bound(p.decay,.001f,4)||!bound(p.sustain,0,1)||!bound(p.release,.005f,5)||!bound(p.fAttack,.001f,4)||!bound(p.fDecay,.001f,4)||!bound(p.fSustain,0,1)||!bound(p.fRelease,.005f,5)||!bound(p.filterEnv,-1,1)||!bound(p.gain,0,1)||!bound(p.phaseA,0,359)||!bound(p.phaseB,0,359)||!bound(p.invertA,0,1)||!bound(p.invertB,0,1)||!bound(p.cutoff,20,18000)||!bound(p.resonance,0,1)||!bound(p.drive,0,1))return false;
 if(!bound(p.chorus,0,1)||!bound(p.delay,0,1)||!bound(p.reverb,0,1))return false;
 if(!bound(p.glide,0,2)||!bound(p.bendRange,1,12)||!bound(p.velSense,0,1))return false;
 if((p.invertA!=0&&p.invertA!=1)||(p.invertB!=0&&p.invertB!=1))return false;
 if(p.mode!=Mode::Keyed&&p.mode!=Mode::Free&&p.mode!=Mode::Tempo)return false;
 if(p.target!=Target::Phase&&p.target!=Target::Morph)return false;
 if(p.curve!=Curve::Linear&&p.curve!=Curve::S&&p.curve!=Curve::Hard)return false;
 if(p.filter!=Filter::Off&&p.filter!=Filter::LP&&p.filter!=Filter::BP&&p.filter!=Filter::HP)return false;
 if(p.voice!=VoiceMode::Poly&&p.voice!=VoiceMode::Mono)return false;
 const auto old=p_;const auto oldVoice=old.voice;p_=p;
 if(p_.filterEnv!=old.filterEnv)rebuildFilterEnvLut();
 if(p_.glide!=old.glide)glideCoeff_=p.glide<=0?1:1-std::exp(-1/(p.glide*rate_));
 if(p_.bendRange!=old.bendRange)bendRatioTarget_=std::pow(2.,bendNorm_*p.bendRange/12.);
 if(oldVoice!=p_.voice){monoCount_=0;monoVelocity_.fill(0);panic();}
 return true;
}

void Synth::rebuildFilterTables() noexcept {
 for(std::size_t i=0;i<filterFreqLut_.size();i++){const double cutoff=20.+(filterMax_-20.)*static_cast<double>(i)/(filterFreqLut_.size()-1);filterFreqLut_[i]=std::tan(Pi*cutoff/rate_);}
 rebuildFilterEnvLut();
}
void Synth::rebuildFilterEnvLut() noexcept {
 for(std::size_t i=0;i<filterEnvLut_.size();i++)filterEnvLut_[i]=std::pow(2.,5.*p_.filterEnv*(static_cast<double>(i)/256.));
}
double Synth::filterG(double env) const noexcept {
 const double x=std::clamp(env,0.,1.)*256.;const auto ei=std::min<std::size_t>(255,static_cast<std::size_t>(x));const double ef=x-ei;
 const double mul=filterEnvLut_[ei]+(filterEnvLut_[ei+1]-filterEnvLut_[ei])*ef;const double cutoff=std::clamp(static_cast<double>(p_.cutoff)*mul,20.,filterMax_);
 const double pos=(cutoff-20.)/(filterMax_-20.)*(filterFreqLut_.size()-1);const auto i=std::min<std::size_t>(filterFreqLut_.size()-2,static_cast<std::size_t>(pos));const double f=pos-i;
 return filterFreqLut_[i]+(filterFreqLut_[i+1]-filterFreqLut_[i])*f;
}

bool Synth::setBank(const Bank& b) noexcept {if(fade_)return false;old_=bank_;bank_=&b;fade_=old_?256*Oversample:0;return true;}
double Synth::noteHz(int note) noexcept {return 440*std::pow(2.,(note-69)/12.);}
void Synth::monoRemove(int note) noexcept {
 for(std::size_t i=0;i<monoCount_;i++)if(monoStack_[i]==note){for(std::size_t j=i+1;j<monoCount_;j++)monoStack_[j-1]=monoStack_[j];monoCount_--;break;}
 monoVelocity_[note]=0;
}
void Synth::noteOn(int note,float vel,uint32_t instance) noexcept {
 if(note<0||note>127||!std::isfinite(vel))return;
 if(p_.chorus>0||p_.delay>0||p_.reverb>0)fxTail_=std::max(fxTail_,static_cast<std::size_t>(sr_*4));
 if(vel<=0){noteOff(note,instance);return;}
 const auto effective=std::clamp(1-p_.velSense+p_.velSense*std::clamp(vel,0.f,1.f),0.f,1.f);
 if(p_.voice==VoiceMode::Mono){
  const bool hadHeld=monoCount_>0;monoRemove(note);if(monoCount_<monoStack_.size())monoStack_[monoCount_++]=note;monoVelocity_[note]=effective;
  auto& v=voices_[0];
  for(std::size_t i=1;i<voices_.size();i++)voices_[i]=Voice{};
  if(!v.stage||!hadHeld){const auto tail=v.last;const auto env=v.env;v=Voice{};v.note=note;v.id=instance;v.stage=1;v.fstage=1;v.baseF=v.targetF=noteHz(note);v.f=v.baseF;v.vel=effective;v.tail=tail;v.steal=64*Oversample;v.env=env;v.cf=modFrequency(v);}
  else {v.note=note;v.id=instance;v.targetF=noteHz(note);v.vel=effective;}
  return;
 }
 auto it=std::find_if(voices_.begin(),voices_.end(),[](const Voice& v){return !v.stage;});
 if(it==voices_.end())it=std::min_element(voices_.begin(),voices_.end(),[](const Voice& a,const Voice& b){return a.env<b.env;});
 const double tail=it->last;*it=Voice{};it->note=note;it->id=instance;it->stage=1;it->fstage=1;it->baseF=it->targetF=noteHz(note);it->f=it->baseF;it->vel=effective;it->tail=tail;it->steal=64*Oversample;it->cf=modFrequency(*it);
}
void Synth::noteOff(int note,uint32_t instance) noexcept {
 if(p_.voice==VoiceMode::Mono){monoRemove(note);auto& v=voices_[0];if(monoCount_){const int next=monoStack_[monoCount_-1];v.note=next;v.targetF=noteHz(next);v.vel=monoVelocity_[next];}else if(v.stage&&v.stage!=4){v.stage=4;v.releaseStep=v.env/(p_.release*rate_);v.fstage=4;v.fReleaseStep=v.fenv/(p_.fRelease*rate_);}return;}
 for(auto& v:voices_)if(v.note==note&&(!instance||v.id==instance)&&v.stage&&v.stage!=4){v.stage=4;v.releaseStep=v.env/(p_.release*rate_);v.fstage=4;v.fReleaseStep=v.fenv/(p_.fRelease*rate_);}
}
void Synth::setPitchBend(float normalized) noexcept {if(!std::isfinite(normalized))return;bendNorm_=std::clamp(normalized,-1.f,1.f);bendRatioTarget_=std::pow(2.,bendNorm_*p_.bendRange/12.);}
bool Synth::setExternalTempo(float bpm) noexcept {if(!bound(bpm,30,300))return false;externalBpm_=bpm;return true;}
void Synth::clearExternalTempo() noexcept {externalBpm_=0;}
void Synth::panic() noexcept {monoCount_=0;monoVelocity_.fill(0);for(auto& v:voices_)v=Voice{};history_.fill(0);head_=0;clearEffects();}
double Synth::modFrequency(const Voice& v) const noexcept {const auto bpm=externalBpm_>0?externalBpm_:p_.bpm;return p_.mode==Mode::Keyed?v.f*p_.ratio:p_.mode==Mode::Free?p_.freeHz:bpm/(240*p_.bars);}
double Synth::carrier(const Bank& b,const Voice& v) const noexcept {
 const auto c=readWave(b[2],v.q,v.cf,rate_);const double phase=v.phase+depth_*.25*c*(1-targetMix_);
 double m=std::clamp(morph_+depth_*c*targetMix_,0.,1.);
 const double s=m*m*(3-2*m),hard=m<.5?2*m*m:1-2*(1-m)*(1-m);
 m=m*(1-curveS_-curveHard_)+s*curveS_+hard*curveHard_;
 return readWave(b[0],phase+phaseA_,v.f,rate_)*invA_*(1-m)+readWave(b[1],phase+phaseB_,v.f,rate_)*invB_*m;
}
double Synth::readDelay(const std::vector<double>& buffer,std::size_t head,double delaySamples) const noexcept {
 if(buffer.empty())return 0;
 double pos=static_cast<double>(head)-delaySamples;const double size=static_cast<double>(buffer.size());
 while(pos<0)pos+=size;
 while(pos>=size)pos-=size;
 const auto i=static_cast<std::size_t>(pos),j=(i+1)%buffer.size();const double f=pos-i;
 return buffer[i]+(buffer[j]-buffer[i])*f;
}
void Synth::clearEffects() noexcept {
 std::fill(chorusBuffer_.begin(),chorusBuffer_.end(),0.);std::fill(delayBuffer_.begin(),delayBuffer_.end(),0.);
 chorusHead_=delayHead_=0;chorusPhase_=0;fxTail_=0;
 for(auto& line:comb_){std::fill(line.buffer.begin(),line.buffer.end(),0.);line.head=0;}
 for(auto& line:allpass_){std::fill(line.buffer.begin(),line.buffer.end(),0.);line.head=0;}
}
double Synth::processEffects(double input) noexcept {
 chorus_+=(p_.chorus-chorus_)*fxSmooth_;delay_+=(p_.delay-delay_)*fxSmooth_;reverb_+=(p_.reverb-reverb_)*fxSmooth_;
 if(std::abs(input)>1e-8&&(chorus_>1e-5||delay_>1e-5||reverb_>1e-5))fxTail_=std::max(fxTail_,static_cast<std::size_t>(sr_*4));
 double x=input;
 chorusBuffer_[chorusHead_]=x;
 const double lfo=.5+.5*std::sin(2*Pi*chorusPhase_);chorusPhase_+=.35/sr_;if(chorusPhase_>=1)chorusPhase_-=1;
 const double chorusSamples=sr_*(.012+.010*lfo),chorused=readDelay(chorusBuffer_,chorusHead_,chorusSamples);
 chorusHead_=(chorusHead_+1)%chorusBuffer_.size();x+=chorus_*.35*(chorused-x);
 const double delayed=readDelay(delayBuffer_,delayHead_,sr_*.22);delayBuffer_[delayHead_]=x+delayed*(.18+.42*delay_);delayHead_=(delayHead_+1)%delayBuffer_.size();x+=delayed*(.50*delay_);
 double wet=0;const double feedback=.72+.18*reverb_,rvIn=x*.24;
 for(auto& line:comb_){const double y=line.buffer[line.head];line.buffer[line.head]=rvIn+y*feedback;line.head=(line.head+1)%line.buffer.size();wet+=y;}
 wet*=.25;
 for(auto& line:allpass_){const double b=line.buffer[line.head],y=-wet+b;line.buffer[line.head]=wet+b*.5;line.head=(line.head+1)%line.buffer.size();wet=y;}
 x+=wet*(.42*reverb_);
 if(fxTail_>0)--fxTail_;
 const double a=std::abs(x);if(a>.92){const double over=(a-.92)/.08;x=std::copysign(.92+.08*(over/(1+over)),x);}return std::clamp(x,-1.,1.);
}
void Synth::process(float* out,std::size_t frames) noexcept {
 if(!fade_&&activeVoices()==0&&fxTail_==0&&std::all_of(history_.begin(),history_.end(),[](double x){return x==0;})){std::fill_n(out,frames,0.f);return;}
 const double z=smooth_;
 for(std::size_t i=0;i<frames;i++){
  for(int os=0;os<Oversample;os++){
   sustain_+=(p_.sustain-sustain_)*z;morph_+=(p_.morph-morph_)*morphSmooth_;depth_+=(p_.depth-depth_)*z;gain_+=(p_.gain-gain_)*z;
   bendRatio_+=(bendRatioTarget_-bendRatio_)*z;
   targetMix_+=((p_.target==Target::Morph?1:0)-targetMix_)*z;curveS_+=((p_.curve==Curve::S?1:0)-curveS_)*z;curveHard_+=((p_.curve==Curve::Hard?1:0)-curveHard_)*z;
   double da=p_.phaseA/360.-phaseA_;da-=std::floor(da+.5);phaseA_+=da*z;
   double db=p_.phaseB/360.-phaseB_;db-=std::floor(db+.5);phaseB_+=db*z;
   invA_+=(1-2*p_.invertA-invA_)*z;invB_+=(1-2*p_.invertB-invB_)*z;
   damp_+=(2-1.8*p_.resonance-damp_)*z;drive_+=(p_.drive-drive_)*z;
   lp_+=((p_.filter==Filter::LP?1:0)-lp_)*z;bp_+=((p_.filter==Filter::BP?1:0)-bp_)*z;hp_+=((p_.filter==Filter::HP?1:0)-hp_)*z;
   double sum=0;
   for(auto& v:voices_){
    if(!v.stage)continue;
    if(glideCoeff_>=1)v.baseF=v.targetF;else v.baseF+=(v.targetF-v.baseF)*glideCoeff_;v.f=v.baseF*bendRatio_;
    if(v.stage==1){v.env=std::min(1.,v.env+1/(p_.attack*rate_));if(v.env>=1)v.stage=2;}
    else if(v.stage==2){v.env=std::max(sustain_,v.env-(1-sustain_)/(p_.decay*rate_));if(v.env<=sustain_)v.stage=3;}
    else if(v.stage==3)v.env+=(sustain_-v.env)*z;
    else {v.env=std::max(0.,v.env-v.releaseStep);if(!v.env){v.stage=0;v.fstage=0;v.fenv=0;v.last=0;v.ic1=0;v.ic2=0;continue;}}
    if(v.fstage==1){v.fenv=std::min(1.,v.fenv+1/(p_.fAttack*rate_));if(v.fenv>=1)v.fstage=2;}
    else if(v.fstage==2){v.fenv=std::max(static_cast<double>(p_.fSustain),v.fenv-(1-p_.fSustain)/(p_.fDecay*rate_));if(v.fenv<=p_.fSustain)v.fstage=3;}
    else if(v.fstage==3)v.fenv+=(p_.fSustain-v.fenv)*z;
    else if(v.fstage==4)v.fenv=std::max(0.,v.fenv-v.fReleaseStep);
    v.cf+=(modFrequency(v)-v.cf)*z;
    double y=bank_?carrier(*bank_,v):0;
    if(fade_&&old_){const double w=fade_/static_cast<double>(256*Oversample);y=y*(1-w)+carrier(*old_,v)*w;}
    const double fg=filterG(v.fenv),a1=1/(1+fg*(fg+damp_)),a2=fg*a1,a3=fg*a2;
    const double v3=y-v.ic2,v1=a1*v.ic1+a2*v3,v2=v.ic2+a2*v.ic1+a3*v3;
    v.ic1=2*v1-v.ic1;v.ic2=2*v2-v.ic2;
    const double hp=y-damp_*v1-v2;y=y*(1-lp_-bp_-hp_)+v2*lp_+v1*damp_*bp_+hp*hp_;
    const double driven=y*(1+7*drive_),sat=driven/(1+std::abs(driven));y=y*(1-drive_)+sat*drive_;
    y*=v.env*v.vel;if(v.steal){const double w=v.steal--/static_cast<double>(64*Oversample);y=y*(1-w)+v.tail*w;}v.last=y;sum+=y;
    v.phase=std::fmod(v.phase+v.f/rate_,1.);v.q=std::fmod(v.q+v.cf/rate_,1.);
   }
   history_[head_]=sum*gain_/8;head_=(head_+1)&127;if(fade_&&!--fade_)old_=nullptr;
  }
  double y=0;for(int k=0;k<FirTaps;k++)y+=kernel_[k]*history_[(head_-1-k)&127];out[i]=static_cast<float>(processEffects(y));
 }
}
std::size_t Synth::activeVoices() const noexcept {return static_cast<std::size_t>(std::count_if(voices_.begin(),voices_.end(),[](const Voice& v){return v.stage!=0;}));}
}
