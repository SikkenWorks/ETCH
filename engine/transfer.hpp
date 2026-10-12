#pragma once
#include "etch.hpp"
#include "harmony.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace etch {
constexpr uint8_t EtchSysExManufacturer=0x7d;
constexpr uint8_t EtchTransferProtocol=1;
constexpr std::size_t MaxStepNotes=8;
enum class TransferKind : uint8_t { Wave=1, Patch=2 };
enum class ArpMode : uint8_t { Off=0, Up=1, Down=2, UpDown=3, Order=4, Random=5 };
enum class StepRate : uint8_t { Quarter=0, Eighth=1, Sixteenth=2, ThirtySecond=3 };
struct StepLocks {
 bool hasMorph=false,hasDepth=false,hasCutoff=false,hasChorus=false,hasDelay=false,hasReverb=false;
 float morph=0,depth=0,cutoff=8000,chorus=0,delay=0,reverb=0;
};
struct StepNote { uint8_t note=0; float velocity=.85f; uint16_t length=1; };
struct SequenceStep { uint8_t noteCount=0; std::array<StepNote,MaxStepNotes> notes{}; StepLocks locks{}; };
struct PerformanceData {
 ArpMode arpMode=ArpMode::Off; StepRate arpRate=StepRate::Sixteenth; uint8_t arpOctaves=1; float arpGate=.70f;
 HarmonyData harmony{};
 uint16_t seqLength=16; StepRate seqRate=StepRate::Sixteenth; float seqSwing=0,seqGate=.75f;
 std::array<SequenceStep,256> steps{};
};
struct WaveTransfer { std::string name; Points points{}; WaveFx fx{}; };
struct PatchTransfer { std::string name; std::array<uint8_t,3> slots{}; Params params{}; PerformanceData performance{}; };
struct DecodedTransfer { TransferKind kind=TransferKind::Wave; uint8_t slot=0; WaveTransfer wave{}; PatchTransfer patch{}; };

// Control/editor-thread protocol. This is object transfer only; it never writes firmware.
std::vector<uint8_t> pack7(const std::vector<uint8_t>& raw);
std::vector<uint8_t> unpack7(const std::vector<uint8_t>& packed);
std::vector<uint8_t> encodeWaveSysex(uint8_t slot,const WaveTransfer& wave);
std::vector<uint8_t> encodePatchSysex(uint8_t slot,const PatchTransfer& patch);
DecodedTransfer decodeEtchSysex(const std::vector<uint8_t>& data);
}
