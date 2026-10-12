#include "q15_runtime.hpp"
namespace etch::target {
void Q15Runtime::configure(const Q15VoiceConfig& v,const Q15OscConfig& o,const Q15SvfCoeffs& f) noexcept {
 voiceConfig_=v;oscConfig_=o;filter_=f;
 // New notes receive this snapshot. Existing notes retain velocity/envelopes.
}
void Q15Runtime::setNoteIncrement(std::uint8_t n,std::uint32_t i) noexcept {if(n<128)increments_[n]=i;}
bool Q15Runtime::enqueue(Event e) noexcept {
 if(overflow_)return false;
 if(e.note>=128 || (!events_.empty() && e.offset<lastOffset_) || !events_.push(e)){overflow_=true;return false;}
 lastOffset_=e.offset;return true;
}
void Q15Runtime::panic() noexcept {for(auto& s:slots_){s.voice.reset();s.held=false;}sustain_=false;}
std::size_t Q15Runtime::activeVoices() const noexcept {std::size_t n=0;for(const auto& s:slots_)if(s.voice.active())++n;return n;}
void Q15Runtime::apply(Event e) noexcept {
 if(e.kind==Event::Kind::Panic){panic();return;}
 if(e.kind==Event::Kind::Sustain){sustain_=e.velocity!=0;if(!sustain_)for(auto& s:slots_)if(!s.held)s.voice.noteOff();return;}
 if(e.kind==Event::Kind::Off || e.velocity==0){for(auto& s:slots_)if(s.note==e.note&&s.held){s.held=false;if(!sustain_)s.voice.noteOff();}return;}
 if(increments_[e.note]==0)return;
 std::size_t index=VoiceCount;
 for(std::size_t i=0;i<VoiceCount;++i)if(!slots_[i].voice.active()){index=i;break;}
 if(index==VoiceCount){index=steal_;steal_=(steal_+1)%VoiceCount;}
 auto& s=slots_[index];s.voice.reset();s.note=e.note;s.held=true;
 auto config=voiceConfig_;config.velocity=static_cast<std::uint16_t>((static_cast<std::uint32_t>(e.velocity>127?127:e.velocity)*32767)/127);
 s.voice.setConfig(config);s.voice.setOscillator(oscConfig_,increments_[e.note],increments_[e.note]);s.voice.noteOn();
}
bool Q15Runtime::render(std::int16_t* out,std::size_t frames) noexcept {
 if(!out || frames==0 || frames>65536){events_.clear();lastOffset_=0;overflow_=false;panic();return false;}
 bool ok=!overflow_;
 if(overflow_){panic();events_.clear();}
 std::size_t cursor=0;
 Event pending{};bool have=cursor<events_.size();if(have)pending=events_[cursor++];
 for(std::size_t i=0;i<frames;++i){
  while(have&&pending.offset==i){apply(pending);have=cursor<events_.size();if(have)pending=events_[cursor++];}
  Q15MixBus mix;for(auto& s:slots_)if(s.voice.active())mix.add(s.voice.next(bank_,filter_));out[i]=mix.output();
 }
 if(have){ok=false;panic();for(std::size_t i=0;i<frames;++i)out[i]=0;}
 events_.clear();lastOffset_=0;overflow_=false;return ok;
}
}
