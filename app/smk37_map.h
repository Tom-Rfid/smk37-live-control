/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * User assignment of the SMK37 knobs and faders (MAP page, BT long press).
 *
 * Twelve physical controls: K1..K8 (physical encoders 0..7) and F1..F4.
 * Each one can keep its normal firmware behaviour (DEFAULT) or drive one
 * target: a track's level, reverb send, delay send or filter cutoff, or an
 * external MIDI CC on a chosen channel. Assignments are device preferences,
 * independent of projects and undo, and are stored with the panel settings.
 *
 * Knob assignments apply only on the performance pages (OVERVIEW / MIX) and
 * are bypassed while SHIFT is held, so every editor keeps its own knobs.
 * Fader assignments apply on every page except the MAP page itself, where a
 * moved fader selects its own row instead ("fader learn"). */
#ifndef SMK37_MAP_H
#define SMK37_MAP_H
#include <stdint.h>

enum { SMK37_MAP_KNOBS = 8, SMK37_MAP_FADERS = 4,
       SMK37_MAP_CONTROLS = SMK37_MAP_KNOBS + SMK37_MAP_FADERS };
enum smk37_map_target {
    SMK37_MAP_DEFAULT, SMK37_MAP_LEVEL, SMK37_MAP_REVERB, SMK37_MAP_DELAY,
    SMK37_MAP_CUTOFF, SMK37_MAP_CC, SMK37_MAP_TARGETS
};
/* track: 0..7, or SMK37_MAP_SELECTED to follow the selected track. */
enum { SMK37_MAP_SELECTED = 8 };
struct smk37_map_entry { uint8_t target, track, cc, channel; };
/* Stored bytes: four per control, in control order. */
enum { SMK37_MAP_BYTES = SMK37_MAP_CONTROLS * 4 };

void smk37_map_reset(void);
void smk37_map_get(unsigned control, struct smk37_map_entry *entry);
/* parameter 0 target, 1 track (or CC number for MIDI CC), 2 MIDI channel.
 * Returns nonzero when the stored assignment changed. */
int smk37_map_edit(unsigned control, unsigned parameter, int delta);
void smk37_map_clear(unsigned control);
const char *smk37_map_target_name(unsigned target);

/* Panel hooks. Return nonzero when the event was consumed by an assignment. */
int smk37_map_encoder(unsigned id, int delta);
int smk37_map_fader(unsigned id, unsigned value);

/* Persistence (called by project.c through weak references). */
void smk37_map_preferences_read(uint8_t data[SMK37_MAP_BYTES]);
int smk37_map_preferences_apply(const uint8_t data[SMK37_MAP_BYTES]);
#endif
