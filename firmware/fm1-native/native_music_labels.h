/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH: one pitch-class / scale vocabulary across GUIDE, HARM and FIFTHS.
 * 0=C, 1=C#, ... 11=B. Never display raw pitch class as a root key.
 */
#pragma once
#include <stdint.h>
static inline const char *etch_note_name(uint8_t root){
 static const char *const notes[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
 return notes[root%12u];
}
static inline const char *etch_mode_name(uint8_t mode){
 static const char *const names[9]={"MAJOR","DORIAN","PHRYGIAN","LYDIAN","MIXOLYDIAN","MINOR","LOCRIAN","HARM MIN","MELOD MIN"};
 return names[mode<9u?mode:0u];
}
static inline const char *etch_lock_name(uint8_t lock){
 static const char *const names[3]={"OFF","GUIDE","LOCK"};
 return names[lock<3u?lock:0u];
}

/* Four physical-parameter cells are only 60 pixels wide at 8px glyphs. */
static inline const char *etch_mode_short(uint8_t mode){
 static const char *const names[9]={"MAJOR","DORIAN","PHRYG","LYDIAN","MIXOLYD","MINOR","LOCRIAN","H.MIN","M.MIN"};
 return names[mode<9u?mode:0u];
}

/* Fixed 12-pitch-class GUIDE keyboard, always C, C#, D ... B.
 * The selected root changes HIGHLIGHTS, never the physical note layout.
 * 12 tiles x 19 px = 228 px, left margin 6 px, width 16 px per tile.
 */
#define ETCH_GUIDE_KEY_COUNT 12u
#define ETCH_GUIDE_KEY_WIDTH 16u
static inline uint8_t etch_guide_pitch_at_column(uint8_t col){return (uint8_t)(col%ETCH_GUIDE_KEY_COUNT);}
static inline uint32_t etch_guide_column_x(uint8_t col){return 6u+19u*(uint32_t)col;}
static inline uint8_t etch_guide_is_accidental(uint8_t pitch_class){
 static const uint16_t accidentals=(1u<<1)|(1u<<3)|(1u<<6)|(1u<<8)|(1u<<10);
 return (uint8_t)((accidentals>>(pitch_class%12u))&1u);
}
