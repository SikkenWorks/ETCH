#include "etch.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
static void le(std::ostream& f,uint32_t v,int count){while(count--){f.put(static_cast<char>(v&255));v>>=8;}}
int main(int argc,char** argv){
 etch::Points a{},b{},c{};
 for(std::size_t i=0;i<128;i++){a[i]=static_cast<int16_t>(std::lround(29199*std::sin(2*3.14159265358979323846*i/128)));b[i]=static_cast<int16_t>(std::lround(29199*(1-4*std::abs(i/128.-.5))));c[i]=a[i];}
 const etch::Bank bank{etch::compileWave(a),etch::compileWave(b),etch::compileWave(c)};
 etch::Synth synth;etch::Params p;p.depth=.38f;p.ratio=2;p.release=.4f;if(argc>2&&std::string(argv[2])=="filter"){p.filter=etch::Filter::LP;p.cutoff=1500;p.resonance=.6f;p.drive=.3f;p.phaseA=91;p.invertB=1;p.target=etch::Target::Morph;p.mode=etch::Mode::Free;p.freeHz=8.5f;p.curve=etch::Curve::Hard;}synth.setParams(p);synth.setBank(bank);
 std::vector<float> out(44100*4);synth.noteOn(48,.85f);synth.noteOn(55,.7f);synth.noteOn(60,.65f);
 synth.process(out.data(),44100*3);for(int n:{48,55,60})synth.noteOff(n);synth.process(out.data()+44100*3,44100);
 const char* path=argc>1?argv[1]:"ETCH-first-sound.wav";std::ofstream f(path,std::ios::binary);if(!f){std::cerr<<"Cannot write output\n";return 1;}
 f.write("RIFF",4);le(f,36+static_cast<uint32_t>(out.size()*2),4);f.write("WAVEfmt ",8);le(f,16,4);le(f,1,2);le(f,1,2);le(f,44100,4);le(f,88200,4);le(f,2,2);le(f,16,2);f.write("data",4);le(f,static_cast<uint32_t>(out.size()*2),4);
 for(float v:out)le(f,static_cast<uint16_t>(static_cast<int16_t>(std::lround(v*32767))),2);
 if(!f){std::cerr<<"Write failed\n";return 1;}std::cout<<"Rendered "<<path<<"\n";
}
