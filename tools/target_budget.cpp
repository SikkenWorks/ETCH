#include "target_memory.hpp"
#include "../target/q15_oscillator.hpp"
#include "../target/q15_voice.hpp"
#include <iomanip>
#include <iostream>
static double kib(std::size_t b){return static_cast<double>(b)/1024.;}
int main(){
 using namespace etch::target;
 std::cout<<"ETCH V0.23 target SRAM planning report\n";
 std::cout<<std::fixed<<std::setprecision(1);
 std::cout<<"Reported SRAM:        "<<kib(ReportedSramBytes)<<" KiB\n";
 std::cout<<"ETCH planning ceiling:"<<kib(EtchStaticPlanningCeiling)<<" KiB\n";
 std::cout<<"RGB565 framebuffer:   "<<kib(Rgb565FramebufferBytes)<<" KiB\n";
 std::cout<<"A/B/C bank transition:"<<kib(BankTransitionBytes)<<" KiB\n";
 std::cout<<"Wave editor + undo:   "<<kib(WaveEditorBytes)<<" KiB\n";
 std::cout<<"Q15 FX arena:         "<<kib(FxQ15Bytes)<<" KiB\n";
 std::cout<<"Compact sequencer:    "<<kib(CompactSequencerBytes)<<" KiB\n";
 std::cout<<"Bounded HAL queues:   "<<kib(RuntimeQueueBytes)<<" KiB\n";
 std::cout<<"8 Q15 voices + mix:   "<<kib(TargetVoiceBytes)<<" KiB\n";
 std::cout<<"Layout padding:       "<<kib(LayoutPaddingBytes)<<" KiB\n";
 std::cout<<"Tracked concrete:     "<<kib(CoreTrackedBytes)<<" KiB\n";
 std::cout<<"Planning headroom:    "<<kib(CoreHeadroomBytes)<<" KiB\n";
 std::cout<<"SDK/stack reserve:    "<<kib(SdkAndStackReserve)<<" KiB\n";
 std::cout<<"Q15 oscillator state: "<<sizeof(Q15OscillatorCore)<<" bytes\n";
 std::cout<<"Q15 voice state:      "<<sizeof(Q15VoiceCore)<<" bytes\n";
 return 0;
}
