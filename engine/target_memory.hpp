#pragma once
#include "etch.hpp"
#include "../hal/target_contract.hpp"
#include "../target/q15_voice.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace etch::target {
// Planning numbers only. Actual linker-map validation is mandatory on the JieLi target.
constexpr std::size_t ReportedSramBytes=578u*1024u;
constexpr std::size_t EtchStaticPlanningCeiling=384u*1024u;
constexpr std::size_t SdkAndStackReserve=ReportedSramBytes-EtchStaticPlanningCeiling;
constexpr std::size_t Rgb565FramebufferBytes=hal::LcdWidth*hal::LcdHeight*2u;
constexpr std::size_t ActiveBankBytes=sizeof(Bank);
constexpr std::size_t BankTransitionBytes=ActiveBankBytes*2u;
constexpr std::size_t WaveEditorBytes=sizeof(Points)*(1u+16u); // edit buffer + 16 full undo snapshots
constexpr std::size_t MaxTargetSampleRate=48000u;
constexpr std::size_t fxLength(double seconds) noexcept {return static_cast<std::size_t>(MaxTargetSampleRate*seconds+0.999999)+4u;}

constexpr std::size_t ChorusQ15Samples=fxLength(.04);
constexpr std::size_t DelayQ15Samples=fxLength(.26);
constexpr std::size_t Comb0Q15Samples=fxLength(.0297);
constexpr std::size_t Comb1Q15Samples=fxLength(.0371);
constexpr std::size_t Comb2Q15Samples=fxLength(.0411);
constexpr std::size_t Comb3Q15Samples=fxLength(.0437);
constexpr std::size_t Allpass0Q15Samples=fxLength(.005);
constexpr std::size_t Allpass1Q15Samples=fxLength(.0017);
constexpr std::size_t FxQ15Samples=ChorusQ15Samples+DelayQ15Samples+Comb0Q15Samples+Comb1Q15Samples+Comb2Q15Samples+Comb3Q15Samples+Allpass0Q15Samples+Allpass1Q15Samples;
constexpr std::size_t FxQ15Bytes=FxQ15Samples*sizeof(std::int16_t);

struct TargetFxArena {
 std::array<std::int16_t,ChorusQ15Samples> chorus{};
 std::array<std::int16_t,DelayQ15Samples> delay{};
 std::array<std::int16_t,Comb0Q15Samples> comb0{};
 std::array<std::int16_t,Comb1Q15Samples> comb1{};
 std::array<std::int16_t,Comb2Q15Samples> comb2{};
 std::array<std::int16_t,Comb3Q15Samples> comb3{};
 std::array<std::int16_t,Allpass0Q15Samples> allpass0{};
 std::array<std::int16_t,Allpass1Q15Samples> allpass1{};
};
static_assert(sizeof(TargetFxArena)==FxQ15Bytes,"Q15 FX arena must exactly match the target FX plan");

struct CompactStepNote {std::uint8_t note=0,velocity=0;std::uint16_t length=1;};
struct CompactStep {std::array<CompactStepNote,8> notes{};std::array<std::int16_t,6> locks{};std::uint8_t noteCount=0,lockMask=0;};
constexpr std::size_t CompactSequencerBytes=sizeof(CompactStep)*256u;
constexpr std::size_t RuntimeQueueBytes=sizeof(hal::RuntimeQueues);
constexpr std::size_t TargetVoiceBytes=sizeof(Q15VoiceCore)*VoiceCount+sizeof(Q15MixBus);

// This type is deliberately never placed on the stack. It turns the planning
// arithmetic into a concrete C++ layout so unexpected padding/type growth is
// visible to the compiler before target integration.
struct TargetRuntimeMemory {
 std::array<std::uint16_t,hal::LcdWidth*hal::LcdHeight> framebuffer{};
 std::array<Bank,2> bankTransition{};
 std::array<Points,17> waveEditorUndo{};
 TargetFxArena fx{};
 std::array<CompactStep,256> sequencer{};
 hal::RuntimeQueues queues{};
 std::array<Q15VoiceCore,VoiceCount> voices{};
 Q15MixBus mix{};
};
constexpr std::size_t CoreTrackedArithmeticBytes=Rgb565FramebufferBytes+BankTransitionBytes+WaveEditorBytes+FxQ15Bytes+CompactSequencerBytes+RuntimeQueueBytes+TargetVoiceBytes;
constexpr std::size_t CoreTrackedBytes=sizeof(TargetRuntimeMemory);
constexpr std::size_t LayoutPaddingBytes=CoreTrackedBytes-CoreTrackedArithmeticBytes;
constexpr std::size_t CoreHeadroomBytes=EtchStaticPlanningCeiling-CoreTrackedBytes;
static_assert(ActiveBankBytes==18432,"Unexpected active Q15 bank size");
static_assert(CoreTrackedBytes>=CoreTrackedArithmeticBytes,"Concrete target memory layout accounting error");
static_assert(LayoutPaddingBytes<=64,"Target memory layout gained unexpected padding");
static_assert(CoreTrackedBytes<EtchStaticPlanningCeiling,"ETCH target planning budget exceeded");
static_assert(CoreHeadroomBytes>=64u*1024u,"Keep at least 64 KiB headroom inside ETCH planning ceiling");
}
