/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * User assignment of the SMK37 knobs and faders (MAP page, BT long press).
 *
 * Twelve controls: K1..K8 (physical encoders 0..7) and F1..F4. Each keeps
 * its normal behaviour (DEFAULT) or drives a track's level, reverb send,
 * delay send or cutoff, or an external MIDI CC. Knob assignments apply on
 * HOME/MIX only and are bypassed while SHIFT is held; fader assignments
 * apply everywhere except the MAP page, where a moved fader selects its row.
 * Assignments are device preferences stored with the panel settings.
 * The code lives in smk37_map.inc, compiled inside smk37_controls.c so it
 * shares that unit's size optimisation (the app image is close to its limit). */
#ifndef SMK37_MAP_H
#define SMK37_MAP_H
#include <stdint.h>

enum { SMK37_MAP_KNOBS = 8, SMK37_MAP_FADERS = 4,
       SMK37_MAP_CONTROLS = SMK37_MAP_KNOBS + SMK37_MAP_FADERS };
enum smk37_map_target {
    SMK37_MAP_DEFAULT, SMK37_MAP_LEVEL, SMK37_MAP_REVERB, SMK37_MAP_DELAY,
    SMK37_MAP_CUTOFF, SMK37_MAP_CC, SMK37_MAP_TARGETS
};
/* track 0 follows the selected track; 1..8 are T1..T8. channel is 0..15.
 * All-zero bytes are the default (DEFAULT, selected track, CC 0, channel 1). */
struct smk37_map_entry { uint8_t target, track, cc, channel; };
enum { SMK37_MAP_BYTES = SMK37_MAP_CONTROLS * 4 };

const struct smk37_map_entry *smk37_map_get(unsigned control);
/* parameter 0 target, 1 track (or CC number for MIDI CC), 2 MIDI channel. */
void smk37_map_edit(unsigned control, unsigned parameter, int delta);
void smk37_map_clear(unsigned control);
extern const char *const smk37_map_target_names[SMK37_MAP_TARGETS];

int smk37_map_encoder(unsigned id, int delta);
int smk37_map_fader(unsigned id, unsigned value);

void smk37_map_preferences_read(uint8_t data[SMK37_MAP_BYTES]);
int smk37_map_preferences_apply(const uint8_t data[SMK37_MAP_BYTES]);
#endif
