#include "etch.hpp"
#include "transfer.hpp"
#include "boot_state.hpp"
#include "fixed_queue.hpp"
#include "watchdog.hpp"
#include "target_memory.hpp"
#include "r2_app.hpp"
#include "q15_oscillator.hpp"
#include "q15_voice.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>
#include <cstring>
static void check(bool b,const char* s){if(!b){std::cerr<<s<<'\n';std::exit(1);}}
struct FakeR2Platform final:etch::target::R2Platform{
 std::uint32_t now=0;bool oct=false,muted=false,safeShown=false;etch::BootReason safeReason=etch::BootReason::Normal;int usbServices=0;std::string line1,line2;std::vector<etch::hal::ControlEvent> events;std::size_t eventIndex=0;
 std::uint32_t millis() noexcept override{return now;}
 bool octMinusHeldAtBoot() noexcept override{return oct;}
 void muteAudio(bool value) noexcept override{muted=value;}
 void showBootSplash(const char* a,const char* b) noexcept override{line1=a;line2=b;}
 void showSafeMode(etch::BootReason reason) noexcept override{safeShown=true;safeReason=reason;}
 void serviceUsbUpdater() noexcept override{usbServices++;}
 bool pollControl(etch::hal::ControlEvent& event) noexcept override{if(eventIndex>=events.size())return false;event=events[eventIndex++];return true;}
};
int main(){
 {
  check(etch::target::CoreTrackedBytes<etch::target::EtchStaticPlanningCeiling,"Target tracked SRAM fits planning ceiling");
  check(etch::target::CoreHeadroomBytes>=64u*1024u,"Target planning keeps >=64KiB headroom");
  etch::hal::KeyLedSnapshot leds;leds.set(60);leds.set(64);check(leds.test(60)&&leds.test(64)&&!leds.test(67),"HAL LED bounded snapshot");leds.set(60,false);check(!leds.test(60)&&leds.test(64),"HAL LED clear one note");
  etch::WatchdogSupervisor wd(750);wd.arm(100);check(!wd.poll(849),"Watchdog before timeout");check(wd.poll(850)&&wd.snapshot().crashCount==1,"Watchdog trip increments crash counter");check(wd.poll(900)&&wd.snapshot().crashCount==1,"Watchdog trip counts once while latched");wd.arm(1000);wd.kick(1400);check(!wd.poll(2100)&&wd.poll(2150)&&wd.snapshot().crashCount==2,"Watchdog re-arm and kick");wd.clearCrashCount();check(wd.snapshot().crashCount==0,"Watchdog crash counter clear");
  FakeR2Platform hw;etch::target::R2Application r2(hw);auto rs=r2.begin();check(rs.mode==etch::BootMode::Normal&&hw.muted,"R2 boots muted with no synth");check(hw.line1=="SikkenWorks"&&hw.line2=="Born out of passion","R2 exact boot splash");for(int i=1;i<=6;i++){hw.now=static_cast<std::uint32_t>(i*500);r2.tick();}check(r2.stable()&&r2.record().bootGood&&hw.usbServices==6,"R2 stable window and updater service");
  FakeR2Platform oct;oct.oct=true;etch::target::R2Application r2safe(oct);rs=r2safe.begin();check(rs.mode==etch::BootMode::SafeMode&&oct.safeShown&&oct.safeReason==etch::BootReason::OctMinus,"R2 OCT- Safe Mode");
  FakeR2Platform stalled;etch::target::R2Application r2wd(stalled);r2wd.begin();stalled.now=800;r2wd.tick();check(r2wd.state().mode==etch::BootMode::SafeMode&&stalled.safeReason==etch::BootReason::Watchdog,"R2 watchdog routes Safe Mode");
  FakeR2Platform flood;for(std::size_t i=0;i<etch::hal::ControlEventCapacity+1;i++)flood.events.push_back({});etch::target::R2Application r2flood(flood);r2flood.begin();flood.now=10;r2flood.tick();check(r2flood.state().mode==etch::BootMode::SafeMode&&flood.safeReason==etch::BootReason::InputOverflow&&r2flood.droppedControlEvents()==1,"R2 bounded input overflow fails closed");
 }
 {
  etch::FixedQueue<int,3> q;check(q.capacity()==3&&q.empty(),"FixedQueue init");check(q.push(1)&&q.push(2)&&q.push(3),"FixedQueue fills");check(!q.push(4)&&q.dropped()==1,"FixedQueue bounded overflow");int sum=0;const auto drained=q.drainIf([](int v){return v%2==1;},[&](int v){sum+=v;});check(drained==2&&sum==4&&q.size()==1&&q[0]==2,"FixedQueue in-place drain");q.clear();check(q.empty(),"FixedQueue clear");
  etch::BootSafetyMachine boot(3000,2);etch::BootRecord record;etch::BootInputs inputs;auto state=boot.begin(record,inputs);check(state.mode==etch::BootMode::Normal&&state.bootInProgress&&state.loadLastPatch&&!state.outputMuted,"Boot normal start");check(!boot.markStable(2999)&&boot.markStable(3000)&&boot.record().bootGood&&!boot.record().incomplete,"Boot stable commit");
  record={};inputs.octMinusHeld=true;state=boot.begin(record,inputs);check(state.mode==etch::BootMode::SafeMode&&state.reason==etch::BootReason::OctMinus&&state.updaterAvailable&&state.outputMuted,"OCT- Safe Mode");
  record={};record.incomplete=true;inputs={};state=boot.begin(record,inputs);check(state.reason==etch::BootReason::PreviousIncomplete&&!state.loadLastPatch,"Incomplete boot routes Safe Mode");
  record={};record.earlyFailures=2;state=boot.begin(record,inputs);check(state.reason==etch::BootReason::RepeatedFailures,"Repeated failures latch Safe Mode");
  record={};inputs.storageCrcOk=false;state=boot.begin(record,inputs);check(state.reason==etch::BootReason::StorageCrc&&state.storageReadOnly,"Storage CRC Safe Mode is read-only");
 }
 etch::Points p{};p.fill(1234);auto dc=etch::compileWave(p);for(auto& t:dc)for(auto v:t)check(v==0,"DC removal");
 for(std::size_t i=0;i<128;i++)p[i]=static_cast<int16_t>(std::lround(29199*std::sin(6.283185307179586*i/128)));
 auto wave=etch::compileWave(p);for(auto& t:wave){double mean=0;for(auto v:t){check(std::abs(static_cast<int>(v))<=31130,"Q15 wave bounds");mean+=v;}check(std::abs(mean/512)<=1,"Q15 wave mean");}
 etch::WaveFx waveFx;waveFx.drive=.85f;waveFx.bitcrush=.45f;waveFx.chorus=.35f;waveFx.delay=.25f;waveFx.reverb=.4f;waveFx.fold=.3f;waveFx.smooth=.15f;auto effectedPoints=etch::renderWaveFxPoints(p,waveFx);check(effectedPoints!=p,"Per-wave FX alters cycle");check(etch::validWaveFx(waveFx),"Wave FX bounds");auto effectedWave=etch::compileWave(p,waveFx);check(effectedWave.size()==etch::Levels,"Wave FX compile levels");etch::WaveFx invalidWaveFx=waveFx;invalidWaveFx.drive=1.1f;check(!etch::validWaveFx(invalidWaveFx),"Wave FX rejects out-of-range value");
 check(sizeof(etch::Bank)==18432,"Q15 active bank footprint");
 check(std::isfinite(etch::readWave(wave,-1e-20,440,44100)),"Near-zero negative phase wrap");
 etch::Bank bank{wave,wave,wave};
 {
  using namespace etch::target;
  check(sizeof(TargetRuntimeMemory)==CoreTrackedBytes,"Concrete target memory layout is tracked");
  check(sizeof(TargetFxArena)==FxQ15Bytes,"Concrete target Q15 FX arena matches plan");
  check(sizeof(Q15OscillatorCore)<=48,"Q15 oscillator state stays tiny");
  const auto inc=phaseIncrement(440.,48000.);
  const auto phase=static_cast<std::uint32_t>(0.137*static_cast<double>(PhaseScale));
  const auto fixed=readWaveQ15(wave,phase,inc);
  const auto floating=etch::readWave(wave,static_cast<double>(phase)/static_cast<double>(PhaseScale),440.,48000.);
  check(std::abs(static_cast<int>(fixed)-static_cast<int>(std::lround(floating*32768.)))<=4,"Q15 mip reader matches host reference");
  check(readWaveQ15(wave,phase,phaseIncrement(23000.,48000.))==0,"Q15 reader mutes above anti-alias guard");
  Q15OscConfig qcfg;qcfg.morph=0;qcfg.depth=0;
  const auto a=carrierSampleQ15(bank,phase,phase,inc,inc,qcfg);
  qcfg.morph=32767;const auto b=carrierSampleQ15(bank,phase,phase,inc,inc,qcfg);
  check(std::abs(static_cast<int>(a)-static_cast<int>(b))<=1,"Q15 morph endpoints agree for identical A/B");
  etch::Points tri{};for(std::size_t i=0;i<tri.size();i++)tri[i]=static_cast<std::int16_t>(std::lround(26000*(1-4*std::abs(static_cast<double>(i)/128.-.5))));
  const auto triWave=etch::compileWave(tri);etch::Bank mixedBank{wave,triWave,wave};
  qcfg.morph=0;const auto onlyA=carrierSampleQ15(mixedBank,phase,phase,inc,inc,qcfg);qcfg.morph=32767;const auto onlyB=carrierSampleQ15(mixedBank,phase,phase,inc,inc,qcfg);
  check(onlyA!=onlyB,"Q15 morph reaches distinct A/B shapes");
  qcfg.morph=16384;qcfg.depth=28000;qcfg.target=etch::Target::Phase;const auto phaseMod=carrierSampleQ15(mixedBank,phase,phase+0x20000000u,inc,inc*2u,qcfg);qcfg.depth=0;const auto noPhaseMod=carrierSampleQ15(mixedBank,phase,phase+0x20000000u,inc,inc*2u,qcfg);check(phaseMod!=noPhaseMod,"Q15 Wave C phase modulation changes carrier");
  qcfg.depth=28000;qcfg.target=etch::Target::Morph;const auto morphMod=carrierSampleQ15(mixedBank,phase,phase+0x20000000u,inc,inc*2u,qcfg);qcfg.depth=0;const auto noMorphMod=carrierSampleQ15(mixedBank,phase,phase+0x20000000u,inc,inc*2u,qcfg);check(morphMod!=noMorphMod,"Q15 Wave C morph modulation changes carrier");
  Q15OscillatorCore core;core.setIncrements(inc,inc*2u);qcfg.depth=12000;qcfg.target=etch::Target::Phase;qcfg.curve=etch::Curve::S;core.setConfig(qcfg);std::int64_t energy=0;for(int i=0;i<2048;i++)energy+=std::abs(static_cast<int>(core.next(mixedBank)));check(energy>1000,"Q15 oscillator produces bounded signal");

  const auto envCfg=adsrConfigFromSeconds(.005,.02,.6,.01,48000.);Q15Adsr env;env.setConfig(envCfg);env.noteOn();std::uint16_t envPeak=0;for(int i=0;i<5000;i++)envPeak=std::max(envPeak,env.next());check(envPeak>30000&&env.value()>18000,"Q15 ADSR reaches attack/sustain");env.noteOff();for(int i=0;i<5000;i++)env.next();check(!env.active()&&env.value()==0,"Q15 ADSR release reaches idle");
  const auto fc=svfCoeffs(1200,.55,48000.);Q15Svf svf;std::int64_t filterEnergy=0;for(int i=0;i<4096;i++){const auto x=static_cast<std::int16_t>((i&1)?22000:-22000);const auto y=svf.process(x,fc,etch::Filter::LP);filterEnergy+=std::abs(static_cast<int>(y));}check(filterEnergy>1000,"Q15 SVF produces signal");
  check(q15Drive(12000,0)==12000,"Q15 drive zero is transparent");check(std::abs(static_cast<int>(q15Drive(22000,32767)))<=32767,"Q15 drive stays bounded");check(q15SafetyLimit(90000)==32767&&q15SafetyLimit(-90000)==-32768,"Q15 safety limiter clamps mix");
  Q15VoiceCore voice;Q15VoiceConfig vc;vc.amp=adsrConfigFromSeconds(.001,.01,.75,.005,48000.);vc.filterEnv=adsrConfigFromSeconds(.001,.02,.4,.01,48000.);vc.velocity=28000;vc.gain=30000;vc.drive=9000;vc.filter=etch::Filter::LP;voice.setConfig(vc);voice.setOscillator(qcfg,inc,inc*2u);voice.noteOn();Q15MixBus mix;std::int64_t voiceEnergy=0;for(int i=0;i<2048;i++){const auto y=voice.next(mixedBank,fc);voiceEnergy+=std::abs(static_cast<int>(y));mix.add(y);}check(voiceEnergy>1000&&voice.filterEnvValue()>0,"Q15 voice oscillator->ADSR->filter->drive chain");check(std::abs(static_cast<int>(mix.output()))<=32767,"Q15 mix bus bounded output");voice.noteOff();for(int i=0;i<5000;i++)voice.next(mixedBank,fc);check(!voice.active(),"Q15 voice release reaches inactive");
  check(sizeof(Q15VoiceCore)<=104&&sizeof(Q15Adsr)<=16&&sizeof(Q15Svf)<=8,"Q15 full voice state stays bounded");
 }
 etch::Synth s;check(s.setBank(bank),"Bank");check(s.setExternalTempo(128),"External tempo");check(!s.setExternalTempo(10),"External tempo bounds");s.clearExternalTempo();etch::Params params;params.gain=std::numeric_limits<float>::quiet_NaN();check(!s.setParams(params),"NaN parameter rejection");etch::Params invalidFilterEnv;invalidFilterEnv.filterEnv=1.1f;check(!s.setParams(invalidFilterEnv),"Filter envelope bounds");etch::Params invalidFx;invalidFx.reverb=1.1f;check(!s.setParams(invalidFx),"FX bounds");
 std::vector<float> out(44100);for(int i=0;i<12;i++)s.noteOn(48+i);check(s.activeVoices()==8,"Voice cap");s.process(out.data(),out.size());float peak=0;for(auto v:out){check(std::isfinite(v)&&std::abs(v)<=1,"Finite bounded audio");peak=std::max(peak,std::abs(v));}check(peak>.001f,"Non-silent audio");
 s.panic();s.process(out.data(),256);check(s.activeVoices()==0,"Panic silence");s.process(out.data(),256);for(int i=0;i<256;i++)check(out[i]==0,"Silence after panic");
 s.noteOn(60);s.process(out.data(),10000);s.noteOff(60);s.process(out.data(),out.size());check(s.activeVoices()==0,"Note release");
 etch::Synth layered;layered.setBank(bank);etch::Params layeredP;layeredP.release=.005f;check(layered.setParams(layeredP),"Layer params");layered.noteOn(60,1.f,101);layered.noteOn(60,.7f,102);check(layered.activeVoices()==2,"Same-pitch layered voices");layered.noteOff(60,102);layered.process(out.data(),2048);check(layered.activeVoices()==1,"Targeted note-off preserves sustained layer");layered.noteOff(60,101);layered.process(out.data(),2048);check(layered.activeVoices()==0,"Targeted final release");
 check(s.setBank(bank),"Bank transition");check(!s.setBank(bank),"Bank swap gate");s.process(out.data(),256);check(s.transitionComplete(),"Transition finishes");
 check(etch::readWave(wave,-.25,440,44100)==etch::readWave(wave,.75,440,44100),"Negative phase wrapping");
 for(double rate:{8000.,44100.,48000.,96000.}){etch::Synth stress(rate);stress.setBank(bank);etch::Params p2;p2.depth=1;p2.ratio=16;p2.resonance=1;p2.drive=1;for(int i=0;i<8;i++)stress.noteOn(24+i*12);
  for(auto filter:{etch::Filter::Off,etch::Filter::LP,etch::Filter::BP,etch::Filter::HP})for(float cutoff:{20.f,18000.f}){p2.filter=filter;p2.cutoff=cutoff;check(stress.setParams(p2),"Filter params");stress.process(out.data(),2048);for(int i=0;i<2048;i++)check(std::isfinite(out[i])&&std::abs(out[i])<=1,"Filter stress");}
  stress.panic();stress.process(out.data(),256);stress.process(out.data(),256);for(int i=0;i<256;i++)check(out[i]==0,"Stress panic");
 }
 etch::Synth envFlat,envMove;envFlat.setBank(bank);envMove.setBank(bank);etch::Params ep;ep.filter=etch::Filter::LP;ep.cutoff=300;ep.fAttack=.002f;ep.fDecay=.08f;ep.fSustain=.2f;ep.fRelease=.2f;ep.filterEnv=0;check(envFlat.setParams(ep),"Flat filter envelope params");ep.filterEnv=1;check(envMove.setParams(ep),"Moving filter envelope params");envFlat.noteOn(60);envMove.noteOn(60);std::vector<float> ef(4096),em(4096);envFlat.process(ef.data(),ef.size());envMove.process(em.data(),em.size());bool envDifferent=false;for(std::size_t i=0;i<em.size();i++){check(std::isfinite(em[i])&&std::abs(em[i])<=1,"Filter envelope bounded");if(std::abs(em[i]-ef[i])>1e-5f)envDifferent=true;}check(envDifferent,"Filter envelope changes sound");envMove.noteOff(60);envMove.process(out.data(),out.size());check(envMove.activeVoices()==0,"Filter envelope release follows note release");
 etch::Synth mono;mono.setBank(bank);etch::Params mp;mp.voice=etch::VoiceMode::Mono;mp.glide=.08f;mp.velSense=1;mp.bendRange=2;check(mono.setParams(mp),"Mono params");mono.noteOn(48,.25f);mono.process(out.data(),2048);mono.noteOn(60,.9f);check(mono.activeVoices()==1,"Mono voice cap");mono.setPitchBend(1);mono.process(out.data(),4096);for(int i=0;i<4096;i++)check(std::isfinite(out[i])&&std::abs(out[i])<=1,"Mono bend bounded");mono.noteOff(60);check(mono.activeVoices()==1,"Mono last-note fallback");mono.noteOff(48);mono.process(out.data(),out.size());check(mono.activeVoices()==0,"Mono release");
 etch::Synth fxDry,fxWet;fxDry.setBank(bank);fxWet.setBank(bank);etch::Params dryP;dryP.release=.05f;etch::Params wetP=dryP;wetP.chorus=.55f;wetP.delay=.45f;wetP.reverb=.60f;check(fxDry.setParams(dryP)&&fxWet.setParams(wetP),"FX params");fxDry.noteOn(60,.8f);fxWet.noteOn(60,.8f);std::vector<float> dryFx(22050),wetFx(22050);fxDry.process(dryFx.data(),dryFx.size());fxWet.process(wetFx.data(),wetFx.size());bool fxDifferent=false;for(std::size_t i=0;i<wetFx.size();i++){check(std::isfinite(wetFx[i])&&std::abs(wetFx[i])<=1,"FX bounded");if(std::abs(wetFx[i]-dryFx[i])>1e-5f)fxDifferent=true;}check(fxDifferent,"Global FX change sound");fxWet.noteOff(60);std::vector<float> wetTail(22050);fxWet.process(wetTail.data(),wetTail.size());check(std::any_of(wetTail.begin(),wetTail.end(),[](float x){return std::abs(x)>1e-6f;}),"Global FX tail");fxWet.panic();std::vector<float> panicFx(512);fxWet.process(panicFx.data(),panicFx.size());check(std::all_of(panicFx.begin(),panicFx.end(),[](float x){return x==0;}),"Panic clears FX tails");
 etch::HarmonyData harmony;harmony.root=0;harmony.mode=etch::ScaleMode::Ionian;harmony.keyLock=etch::KeyLockMode::Lock;harmony.chord=etch::ChordMode::Triad;check(etch::validHarmony(harmony),"Harmony config");const auto scale=etch::scalePitchClasses(harmony);check(scale==std::array<uint8_t,7>{0,2,4,5,7,9,11},"C Ionian pitch classes");check(etch::quantizeToScale(61,harmony)==60,"KEYLOCK tie quantizes downward");const auto dMinor=etch::buildDiatonicChord(62,harmony,etch::ChordMode::Triad);check(dMinor.count==3&&dMinor.notes[0]==62&&dMinor.notes[1]==65&&dMinor.notes[2]==69,"Diatonic D minor triad");const auto bDim=etch::buildDiatonicChord(71,harmony,etch::ChordMode::Triad);check(bDim.count==3&&bDim.notes[0]==71&&bDim.notes[1]==74&&bDim.notes[2]==77,"Diatonic B diminished triad");
 etch::WaveTransfer wt;wt.name="TRANSFER";wt.points=p;wt.fx=waveFx;auto ws=etch::encodeWaveSysex(7,wt);check(ws.front()==0xf0&&ws.back()==0xf7,"Wave SysEx framing");auto wd=etch::decodeEtchSysex(ws);check(wd.kind==etch::TransferKind::Wave&&wd.slot==7&&wd.wave.name==wt.name&&wd.wave.points==wt.points,"Wave SysEx roundtrip");check(std::abs(wd.wave.fx.drive-waveFx.drive)<1e-6f&&std::abs(wd.wave.fx.reverb-waveFx.reverb)<1e-6f&&std::abs(wd.wave.fx.mix-waveFx.mix)<1e-6f,"Wave SysEx v2 FX roundtrip");
 etch::PatchTransfer pt;pt.name="PATCH X";pt.slots={0,1,2};pt.params.depth=.42f;pt.params.mode=etch::Mode::Free;pt.params.filter=etch::Filter::LP;pt.params.voice=etch::VoiceMode::Mono;pt.params.fAttack=.12f;pt.params.fDecay=.34f;pt.params.fSustain=.56f;pt.params.fRelease=.78f;pt.params.filterEnv=-.65f;pt.params.chorus=.21f;pt.params.delay=.32f;pt.params.reverb=.43f;pt.performance.arpMode=etch::ArpMode::UpDown;pt.performance.arpOctaves=2;pt.performance.harmony.root=9;pt.performance.harmony.mode=etch::ScaleMode::Aeolian;pt.performance.harmony.keyLock=etch::KeyLockMode::Lock;pt.performance.harmony.chord=etch::ChordMode::Seventh;pt.performance.seqLength=256;pt.performance.steps[3].noteCount=3;pt.performance.steps[3].notes[0]={60,.51f,256};pt.performance.steps[3].notes[1]={64,.62f,2};pt.performance.steps[3].notes[2]={67,.73f,1};pt.performance.steps[3].locks.hasMorph=true;pt.performance.steps[3].locks.morph=.61f;pt.performance.steps[3].locks.hasReverb=true;pt.performance.steps[3].locks.reverb=.22f;auto ps=etch::encodePatchSysex(3,pt);auto pd=etch::decodeEtchSysex(ps);check(pd.kind==etch::TransferKind::Patch&&pd.slot==3&&pd.patch.name==pt.name&&pd.patch.slots==pt.slots,"Patch SysEx identity");check(std::abs(pd.patch.params.depth-.42f)<1e-6f&&pd.patch.params.mode==etch::Mode::Free&&pd.patch.params.filter==etch::Filter::LP&&pd.patch.params.voice==etch::VoiceMode::Mono&&std::abs(pd.patch.params.fAttack-.12f)<1e-6f&&std::abs(pd.patch.params.filterEnv+.65f)<1e-6f&&std::abs(pd.patch.params.chorus-.21f)<1e-6f&&std::abs(pd.patch.params.delay-.32f)<1e-6f&&std::abs(pd.patch.params.reverb-.43f)<1e-6f,"Patch SysEx params");check(pd.patch.performance.arpMode==etch::ArpMode::UpDown&&pd.patch.performance.arpOctaves==2&&pd.patch.performance.seqLength==256&&pd.patch.performance.steps[3].noteCount==3&&pd.patch.performance.steps[3].notes[0].note==60&&pd.patch.performance.steps[3].notes[1].note==64&&pd.patch.performance.steps[3].notes[2].note==67&&std::abs(pd.patch.performance.steps[3].notes[2].velocity-.73f)<.01f&&pd.patch.performance.steps[3].notes[0].length==256&&pd.patch.performance.steps[3].notes[1].length==2&&pd.patch.performance.steps[3].notes[2].length==1&&pd.patch.performance.steps[3].locks.hasMorph&&std::abs(pd.patch.performance.steps[3].locks.morph-.61f)<1e-5f&&pd.patch.performance.steps[3].locks.hasReverb&&pd.patch.performance.harmony.root==9&&pd.patch.performance.harmony.mode==etch::ScaleMode::Aeolian&&pd.patch.performance.harmony.keyLock==etch::KeyLockMode::Lock&&pd.patch.performance.harmony.chord==etch::ChordMode::Seventh,"Patch SysEx performance v8 256-step sequence + uint16 note length + harmony"); etch::PatchTransfer pt128=pt;pt128.name="PATCH 128";pt128.performance.seqLength=128;auto p128=etch::decodeEtchSysex(etch::encodePatchSysex(4,pt128));check(p128.patch.performance.seqLength==128,"Patch SysEx accepts 128-step active length");
 bool rejected=false;try{auto broken=ws;broken[broken.size()/2]^=1;etch::decodeEtchSysex(broken);}catch(...){rejected=true;}check(rejected,"SysEx CRC rejection");
 std::vector<uint8_t> raw{0,127,128,255,1,200,64,129,5};check(etch::unpack7(etch::pack7(raw))==raw,"7-bit packing roundtrip");
 std::cout<<"C++ DSP + ETCH transfer tests passed\n";
}
