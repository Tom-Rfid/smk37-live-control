/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Knob/fader assignments for the SMK37 (MAP page). Control task only. */
#include "smk37_map.h"
#include "smk37_controls.h"
#include "instrument.h"
#include <string.h>

static struct smk37_map_entry map[SMK37_MAP_CONTROLS];
/* Last position of each fader and the value last sent by each control
 * (MIDI CC knobs keep their own 0..127 position). */
static uint8_t fader_last[SMK37_MAP_FADERS], fader_seen;
static uint8_t cc_value[SMK37_MAP_CONTROLS];
static uint8_t initialised;

static const char *const target_names[SMK37_MAP_TARGETS] = {
    "DEFAULT", "LEVEL", "REVERB", "DELAY", "CUTOFF", "MIDI CC"
};
const char *smk37_map_target_name(unsigned target)
{ return target < SMK37_MAP_TARGETS ? target_names[target] : "?"; }

static void entry_default(struct smk37_map_entry *e)
{ e->target = SMK37_MAP_DEFAULT; e->track = SMK37_MAP_SELECTED; e->cc = 20; e->channel = 0; }

void smk37_map_reset(void)
{
    initialised = 1;
    for (unsigned i = 0; i < SMK37_MAP_CONTROLS; ++i) { entry_default(&map[i]); map[i].cc = (uint8_t)(20 + i); }
    memset(cc_value, 0, sizeof cc_value);
    fader_seen = 0;
}

/* Zero-initialised storage is not a valid default (CC 0, track 1), so the
 * first use installs the defaults; stored preferences then replace them. */
static void ready(void) { if (!initialised) smk37_map_reset(); }

void smk37_map_get(unsigned control, struct smk37_map_entry *entry)
{
    ready();
    if (control < SMK37_MAP_CONTROLS) *entry = map[control];
    else entry_default(entry);
}

static unsigned clamp_add(unsigned value, int delta, unsigned max)
{
    int next = (int)value + delta;
    return next < 0 ? 0U : next > (int)max ? max : (unsigned)next;
}

int smk37_map_edit(unsigned control, unsigned parameter, int delta)
{
    ready();
    if (control >= SMK37_MAP_CONTROLS || !delta) return 0;
    struct smk37_map_entry *e = &map[control], old = *e;
    if (parameter == 0) e->target = (uint8_t)clamp_add(e->target, delta, SMK37_MAP_TARGETS - 1);
    else if (parameter == 1) {
        if (e->target == SMK37_MAP_CC) e->cc = (uint8_t)clamp_add(e->cc, delta, 127);
        else e->track = (uint8_t)clamp_add(e->track, delta, SMK37_MAP_SELECTED);
    } else if (parameter == 2 && e->target == SMK37_MAP_CC)
        e->channel = (uint8_t)clamp_add(e->channel, delta, 15);
    if (!memcmp(&old, e, sizeof old)) return 0;
    /* Fader positions are tracked for every target, so a new assignment
     * acts on the next physical move and never jumps on its own. */
    groove_settings_panel_changed();
    return 1;
}

void smk37_map_clear(unsigned control)
{
    ready();
    if (control >= SMK37_MAP_CONTROLS || map[control].target == SMK37_MAP_DEFAULT) return;
    map[control].target = SMK37_MAP_DEFAULT;
    groove_settings_panel_changed();
}

static unsigned target_track(const struct smk37_map_entry *e)
{ return e->track < GROOVE_TRACKS ? e->track : smk37_screen_track() % GROOVE_TRACKS; }

static void send_cc(unsigned control, const struct smk37_map_entry *e, unsigned value)
{
    cc_value[control] = (uint8_t)value;
    groove_midi_cc_out(e->channel, e->cc, value);
}

/* Relative motion (knobs, and faders driving relative targets). */
static void apply_delta(unsigned control, const struct smk37_map_entry *e, int delta)
{
    unsigned t = target_track(e);
    switch (e->target) {
    case SMK37_MAP_LEVEL:  groove_control_edit(t, 0, delta, 0); break;
    case SMK37_MAP_REVERB: groove_sends_edit(t, 0, delta); break;
    case SMK37_MAP_DELAY:  groove_sends_edit(t, 1, delta); break;
    case SMK37_MAP_CUTOFF: groove_control_edit(t, 0, 0, delta); break;
    case SMK37_MAP_CC: {
        /* Two CC steps per detent keep the full range within one turn. */
        unsigned next = clamp_add(cc_value[control], 2 * delta, 127);
        if (next != cc_value[control]) send_cc(control, e, next);
        break;
    }
    default: break;
    }
    smk37_map_feedback_set(control, e->target, t, e->target == SMK37_MAP_CC ? (int)cc_value[control] : -1);
}

int smk37_map_encoder(unsigned id, int delta)
{
    ready();
    if (id >= SMK37_MAP_KNOBS) return 0;
    const struct smk37_map_entry *e = &map[id];
    if (e->target == SMK37_MAP_DEFAULT) return 0;
    if (smk37_screen_map_mode() != 1 || smk37_shift()) return 0;
    if (delta) apply_delta(id, e, delta);
    return 1;
}

/* Absolute faders. LEVEL keeps the panel's own pickup logic, so it is
 * handled by smk37_controls.c; this function covers the other targets. */
int smk37_map_fader(unsigned id, unsigned value)
{
    ready();
    if (id >= SMK37_MAP_FADERS) return 0;
    unsigned control = SMK37_MAP_KNOBS + id, bit = 1U << id;
    const struct smk37_map_entry *e = &map[control];
    unsigned previous = fader_last[id], seen = (fader_seen & bit) != 0;
    fader_last[id] = (uint8_t)value; fader_seen |= (uint8_t)bit;
    if (e->target == SMK37_MAP_DEFAULT || e->target == SMK37_MAP_LEVEL) return 0;
    /* The first sample only records the position: no jump at power-on or
     * after an assignment change. */
    if (!seen || value == previous) return 1;
    unsigned t = target_track(e);
    if (e->target == SMK37_MAP_REVERB || e->target == SMK37_MAP_DELAY) {
        uint8_t sends[2];
        unsigned which = e->target == SMK37_MAP_DELAY;
        groove_sends_get(t, sends);
        unsigned want = (value * 15U + 63U) / 127U;
        if (want != sends[which]) groove_sends_edit(t, which, (int)want - (int)sends[which]);
    } else if (e->target == SMK37_MAP_CUTOFF) {
        groove_control_edit(t, 0, 0, (int)value - (int)previous);
    } else if (e->target == SMK37_MAP_CC) {
        send_cc(control, e, value);
    }
    smk37_map_feedback_set(control, e->target, t, e->target == SMK37_MAP_CC ? (int)value : -1);
    return 1;
}

void smk37_map_preferences_read(uint8_t data[SMK37_MAP_BYTES])
{
    ready();
    for (unsigned i = 0; i < SMK37_MAP_CONTROLS; ++i) {
        data[4 * i] = map[i].target; data[4 * i + 1] = map[i].track;
        data[4 * i + 2] = map[i].cc; data[4 * i + 3] = map[i].channel;
    }
}

int smk37_map_preferences_apply(const uint8_t data[SMK37_MAP_BYTES])
{
    for (unsigned i = 0; i < SMK37_MAP_CONTROLS; ++i)
        if (data[4 * i] >= SMK37_MAP_TARGETS || data[4 * i + 1] > SMK37_MAP_SELECTED ||
            data[4 * i + 2] > 127 || data[4 * i + 3] > 15) return -1;
    ready();
    for (unsigned i = 0; i < SMK37_MAP_CONTROLS; ++i) {
        map[i].target = data[4 * i]; map[i].track = data[4 * i + 1];
        map[i].cc = data[4 * i + 2]; map[i].channel = data[4 * i + 3];
    }
    fader_seen = 0;
    return 0;
}
