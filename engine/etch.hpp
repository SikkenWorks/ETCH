#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace etch {
constexpr std::size_t PointCount=128, TableSize=512, Levels=6, VoiceCount=8;
constexpr int Oversample=4, FirTaps=127;
using Points=std::array<int16_t,PointCount>;
using Table=std::array<int16_t,TableSize>;
using Wave=std::array<Table,Levels>;
using Bank=std::array<Wave,3>;
constexpr std::array<int,Levels> Harmonics{64,32,16,8,2,1};
enum class Mode {Keyed,Free,Tempo};
enum class Target {Phase,Morph};
enum class Curve {Linear,S,Hard};
enum class Filter {Off,LP,BP,HP};
enum class VoiceMode {Poly,Mono};
struct WaveFx {float drive=0,bitcrush=0,chorus=0,delay=0,reverb=0,fold=0,smooth=0,mix=1;};
struct Params {float morph=.35f,depth=0,ratio=1,freeHz=2,bpm=120,bars=1;
 float attack=.01f,decay=.2f,sustain=.7f,release=.3f;
 float fAttack=.01f,fDecay=.2f,fSustain=.7f,fRelease=.3f,filterEnv=0,gain=.55f;
 float phaseA=0,phaseB=0,invertA=0,invertB=0,cutoff=8000,resonance=0,drive=0;
 float chorus=0,delay=0,reverb=0;
 float glide=0,bendRange=2,velSense=1;
 Filter filter=Filter::Off;
 Mode mode=Mode::Keyed; Target target=Target::Phase; Curve curve=Curve::Linear; VoiceMode voice=VoiceMode::Poly;};
// Control-thread only: uses stack scratch space and trigonometric reconstruction.
bool validWaveFx(const WaveFx& fx) noexcept;
Points renderWaveFxPoints(const Points& points,const WaveFx& fx=WaveFx{});
Wave compileWave(const Points& points,const WaveFx& fx=WaveFx{});
float readWave(const Wave& wave,double phase,double frequency,double sampleRate) noexcept;
class Synth {
public:
 explicit Synth(double sampleRate=44100);
 // Single audio-thread ownership. Call controls only between process blocks.
 bool setParams(const Params& params) noexcept;
 // Bank must remain alive until transitionComplete(). No allocation or copying.
 // Returns false if a previous transition is still active.
 bool setBank(const Bank& bank) noexcept;
 bool transitionComplete() const noexcept {return fade_==0;}
 void noteOn(int note,float velocity=1,uint32_t instance=0) noexcept;
 void noteOff(int note,uint32_t instance=0) noexcept;
 void setPitchBend(float normalized) noexcept;
 bool setExternalTempo(float bpm) noexcept;
 void clearExternalTempo() noexcept;
 void panic() noexcept;
 void process(float* output,std::size_t frames) noexcept;
 std::size_t activeVoices() const noexcept;
private:
 struct Voice {int note=-1,stage=0,fstage=0,steal=0;uint32_t id=0;double phase=0,q=0,f=0,baseF=0,targetF=0,cf=0,env=0,fenv=0,vel=0,last=0,tail=0,releaseStep=0,fReleaseStep=0,ic1=0,ic2=0;};
 struct ReverbLine {std::vector<double> buffer;std::size_t head=0;};
 double sr_,rate_,smooth_,morphSmooth_,fxSmooth_;
 Params p_;
 double sustain_=.7,morph_=.35,depth_=0,gain_=.55,targetMix_=0,curveS_=0,curveHard_=0;
 double phaseA_=0,phaseB_=0,invA_=1,invB_=1,damp_=2,drive_=0,lp_=0,bp_=0,hp_=0;
 double chorus_=0,delay_=0,reverb_=0,chorusPhase_=0;
 std::array<double,257> filterEnvLut_{};
 std::array<double,2049> filterFreqLut_{};
 double filterMax_=18000;
 double glideCoeff_=1,bendNorm_=0,bendRatio_=1,bendRatioTarget_=1,externalBpm_=0;
 std::array<Voice,VoiceCount> voices_{};
 std::array<int,128> monoStack_{};std::array<float,128> monoVelocity_{};std::size_t monoCount_=0;
 std::array<double,FirTaps> kernel_{};std::array<double,128> history_{};int head_=0;
 std::vector<double> chorusBuffer_,delayBuffer_;
 std::size_t chorusHead_=0,delayHead_=0;
 std::array<ReverbLine,4> comb_{};
 std::array<ReverbLine,2> allpass_{};
 std::size_t fxTail_=0;
 const Bank* bank_=nullptr;const Bank* old_=nullptr;int fade_=0;
 double modFrequency(const Voice& v) const noexcept;
 double carrier(const Bank& b,const Voice& v) const noexcept;
 void rebuildFilterTables() noexcept;
 void rebuildFilterEnvLut() noexcept;
 double filterG(double env) const noexcept;
 double processEffects(double input) noexcept;
 double readDelay(const std::vector<double>& buffer,std::size_t head,double delaySamples) const noexcept;
 void clearEffects() noexcept;
 static double noteHz(int note) noexcept;
 void monoRemove(int note) noexcept;
};
}
