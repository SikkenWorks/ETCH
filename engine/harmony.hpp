#pragma once
#include <array>
#include <cstdint>

namespace etch {
enum class ScaleMode : uint8_t { Ionian=0,Dorian,Phrygian,Lydian,Mixolydian,Aeolian,Locrian,HarmonicMinor,MelodicMinor };
enum class KeyLockMode : uint8_t { Off=0,Guide,Lock };
enum class ChordMode : uint8_t { Off=0,Triad,Seventh };
struct HarmonyData { uint8_t root=0; ScaleMode mode=ScaleMode::Ionian; KeyLockMode keyLock=KeyLockMode::Off; ChordMode chord=ChordMode::Off; };
struct HarmonyChord { uint8_t count=0; std::array<uint8_t,4> notes{}; };

bool validHarmony(const HarmonyData& h) noexcept;
std::array<uint8_t,7> scalePitchClasses(const HarmonyData& h) noexcept;
uint8_t quantizeToScale(uint8_t note,const HarmonyData& h) noexcept;
HarmonyChord buildDiatonicChord(uint8_t note,const HarmonyData& h,ChordMode mode) noexcept;
}
