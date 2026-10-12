#include "q15_runtime.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
using namespace etch;using namespace etch::target;
static void check(bool b){if(!b){std::cerr<<"runtime regression\n";std::exit(1);}}
int main(){
 Bank bank{};for(auto& w:bank)for(auto& t:w)t.fill(12000);
 Q15Runtime r(bank);Q15VoiceConfig c{};c.amp.sustain=32767;c.amp.releaseStep=32767;r.configure(c,{},{});
 for(int n=0;n<128;++n)r.setNoteIncrement(n,1000000);
 std::int16_t out[64]{};
 check(r.enqueue({Q15Runtime::Event::Kind::On,60,127,8}));check(r.render(out,64));
 check(std::all_of(out,out+8,[](auto x){return x==0;}));check(out[8]>0);
 check(r.enqueue({Q15Runtime::Event::Kind::Sustain,0,127,0}));check(r.enqueue({Q15Runtime::Event::Kind::Off,60,0,0}));check(r.render(out,64));check(r.activeVoices()==1);
 check(r.enqueue({Q15Runtime::Event::Kind::Sustain,0,0,0}));check(r.render(out,64));check(r.activeVoices()==0);
 for(int n=60;n<72;++n)check(r.enqueue({Q15Runtime::Event::Kind::On,static_cast<std::uint8_t>(n),127,0}));check(r.render(out,64));check(r.activeVoices()==8);
 check(r.enqueue({Q15Runtime::Event::Kind::Panic,0,0,5}));check(r.render(out,64));check(r.activeVoices()==0);check(out[5]==0);
 for(int i=0;i<128;++i)check(r.enqueue({Q15Runtime::Event::Kind::On,60,127,0}));check(!r.enqueue({Q15Runtime::Event::Kind::Off,60,0,0}));check(!r.render(out,64));check(r.activeVoices()==0);check(std::all_of(out,out+64,[](auto x){return x==0;}));
 check(r.enqueue({Q15Runtime::Event::Kind::On,60,127,64}));check(!r.render(out,64));check(r.activeVoices()==0);
 check(r.enqueue({Q15Runtime::Event::Kind::On,60,127,3}));check(!r.enqueue({Q15Runtime::Event::Kind::Off,60,0,2}));check(!r.render(out,64));
 check(r.enqueue({Q15Runtime::Event::Kind::On,60,0,0}));check(r.render(out,64));check(r.activeVoices()==0);
 std::cout<<"Target runtime: offset timing, sustain, eight voices, panic and fault handling passed\n";
}
