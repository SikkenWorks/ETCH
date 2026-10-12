#include "harmony.hpp"
#include <algorithm>
#include <cstddef>

namespace etch {
namespace {
constexpr std::array<std::array<uint8_t,7>,9> Scales{{
    {{0,2,4,5,7,9,11}},
    {{0,2,3,5,7,9,10}},
    {{0,1,3,5,7,8,10}},
    {{0,2,4,6,7,9,11}},
    {{0,2,4,5,7,9,10}},
    {{0,2,3,5,7,8,10}},
    {{0,1,3,5,6,8,10}},
    {{0,2,3,5,7,8,11}},
    {{0,2,3,5,7,9,11}}
}};

const std::array<uint8_t,7>& intervals(const HarmonyData& h) noexcept {
    return Scales[static_cast<std::size_t>(h.mode)];
}

bool contains(const std::array<uint8_t,7>& pcs,uint8_t pc) noexcept {
    return std::find(pcs.begin(),pcs.end(),pc)!=pcs.end();
}
}

bool validHarmony(const HarmonyData& h) noexcept {
    return h.root<12 && h.mode<=ScaleMode::MelodicMinor &&
           h.keyLock<=KeyLockMode::Lock && h.chord<=ChordMode::Seventh;
}

std::array<uint8_t,7> scalePitchClasses(const HarmonyData& h) noexcept {
    std::array<uint8_t,7> out{};
    if(!validHarmony(h)) return out;
    const auto& in=intervals(h);
    for(std::size_t i=0;i<7;i++) out[i]=static_cast<uint8_t>((h.root+in[i])%12);
    return out;
}

uint8_t quantizeToScale(uint8_t note,const HarmonyData& h) noexcept {
    if(!validHarmony(h)) return note;
    const auto pcs=scalePitchClasses(h);
    const auto pc=static_cast<uint8_t>(note%12);
    if(contains(pcs,pc)) return note;
    for(int d=1;d<=6;d++){
        const int down=static_cast<int>(note)-d;
        const int up=static_cast<int>(note)+d;
        if(down>=0 && contains(pcs,static_cast<uint8_t>(down%12))) return static_cast<uint8_t>(down);
        if(up<=127 && contains(pcs,static_cast<uint8_t>(up%12))) return static_cast<uint8_t>(up);
    }
    return note;
}

HarmonyChord buildDiatonicChord(uint8_t note,const HarmonyData& h,ChordMode mode) noexcept {
    HarmonyChord out{};
    if(!validHarmony(h)) return out;
    const auto root=quantizeToScale(note,h);
    if(mode==ChordMode::Off){
        out.count=1;
        out.notes[0]=root;
        return out;
    }
    const auto& scale=intervals(h);
    const auto interval=static_cast<uint8_t>((root%12+12-h.root)%12);
    std::size_t degree=0;
    while(degree<7 && scale[degree]!=interval) degree++;
    if(degree>=7){
        out.count=1;
        out.notes[0]=root;
        return out;
    }
    const std::size_t count=mode==ChordMode::Seventh?4:3;
    std::array<int,4> raw{};
    for(std::size_t i=0;i<count;i++){
        const auto absolute=degree+i*2;
        const auto index=absolute%7;
        const auto oct=absolute/7;
        raw[i]=static_cast<int>(root)+static_cast<int>(scale[index])+12*static_cast<int>(oct)-static_cast<int>(scale[degree]);
    }
    int shift=0;
    while(*std::max_element(raw.begin(),raw.begin()+static_cast<std::ptrdiff_t>(count))+shift>127) shift-=12;
    while(*std::min_element(raw.begin(),raw.begin()+static_cast<std::ptrdiff_t>(count))+shift<0) shift+=12;
    out.count=static_cast<uint8_t>(count);
    for(std::size_t i=0;i<count;i++) out.notes[i]=static_cast<uint8_t>(raw[i]+shift);
    return out;
}
}
