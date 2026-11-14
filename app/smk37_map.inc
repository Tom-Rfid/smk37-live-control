/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Knob/fader assignments (MAP page). Included by smk37_controls.c only. */
static struct smk37_map_entry map[SMK37_MAP_CONTROLS];
static uint8_t map_fader_last[SMK37_MAP_FADERS], map_fader_seen, map_cc[SMK37_MAP_CONTROLS];

const char *const smk37_map_target_names[SMK37_MAP_TARGETS] = {
    "DEFAULT", "LEVEL", "REVERB", "DELAY", "CUTOFF", "MIDI CC"
};
const struct smk37_map_entry *smk37_map_get(unsigned control)
{ return &map[control < SMK37_MAP_CONTROLS ? control : 0]; }

static unsigned map_add(unsigned value, int delta, unsigned max)
{ int next = (int)value + delta; return next < 0 ? 0U : next > (int)max ? max : (unsigned)next; }

void smk37_map_edit(unsigned control, unsigned parameter, int delta)
{
    if (control >= SMK37_MAP_CONTROLS || !delta) return;
    struct smk37_map_entry *e = &map[control];
    uint8_t *field = parameter == 0 ? &e->target : parameter == 2 ? &e->channel
        : e->target == SMK37_MAP_CC ? &e->cc : &e->track;
    unsigned max = parameter == 0 ? SMK37_MAP_TARGETS - 1 : parameter == 2 ? 15
        : e->target == SMK37_MAP_CC ? 127 : 8;
    if (parameter == 2 && e->target != SMK37_MAP_CC) return;
    *field = (uint8_t)map_add(*field, delta, max);
    groove_settings_panel_changed();
}
void smk37_map_clear(unsigned control)
{ if (control < SMK37_MAP_CONTROLS) { map[control].target = 0; groove_settings_panel_changed(); } }

/* Apply one assignment: delta for relative motion, or absolute >= 0 for a fader. */
static void map_apply(unsigned control, int delta, int absolute)
{
    const struct smk37_map_entry *e = &map[control];
    unsigned t = e->track ? e->track - 1U : smk37_screen_track() % GROOVE_TRACKS;
    if (e->target == SMK37_MAP_LEVEL) groove_control_edit(t, 0, delta, 0);
    else if (e->target == SMK37_MAP_CUTOFF) groove_control_edit(t, 0, 0, delta);
    else if (e->target == SMK37_MAP_CC) {
        unsigned value = absolute >= 0 ? (unsigned)absolute : map_add(map_cc[control], 2 * delta, 127);
        map_cc[control] = (uint8_t)value;
        groove_midi_cc_out(e->channel, e->cc, value);
    } else {
        unsigned which = e->target == SMK37_MAP_DELAY;
        if (absolute >= 0) {
            uint8_t sends[2]; groove_sends_get(t, sends);
            delta = (int)(((unsigned)absolute * 15U + 63U) / 127U) - (int)sends[which];
        }
        groove_sends_edit(t, which, delta);
    }
}
int smk37_map_encoder(unsigned id, int delta)
{
    if (id >= SMK37_MAP_KNOBS || !map[id].target || smk37_screen_map_mode() != 1 || smk37_shift()) return 0;
    map_apply(id, delta, -1);
    return 1;
}
/* LEVEL faders keep the panel's pickup logic in fader(); the others are
 * absolute here. Positions are tracked for every target, so a new
 * assignment acts on the next physical move and never jumps on its own. */
int smk37_map_fader(unsigned id, unsigned value)
{
    unsigned control = SMK37_MAP_KNOBS + id, bit = 1U << id, target = map[control].target;
    int previous = map_fader_last[id];
    unsigned seen = map_fader_seen & bit;
    map_fader_last[id] = (uint8_t)value; map_fader_seen |= (uint8_t)bit;
    if (target == SMK37_MAP_DEFAULT || target == SMK37_MAP_LEVEL) return 0;
    if (seen && (int)value != previous && smk37_screen_map_mode() != 2)
        map_apply(control, (int)value - previous, target == SMK37_MAP_CUTOFF ? -1 : (int)value);
    return 1;
}
void smk37_map_preferences_read(uint8_t data[SMK37_MAP_BYTES]) { memcpy(data, map, sizeof map); }
int smk37_map_preferences_apply(const uint8_t data[SMK37_MAP_BYTES])
{
    for (unsigned i = 0; i < SMK37_MAP_BYTES; i += 4)
        if (data[i] >= SMK37_MAP_TARGETS || data[i + 1] > 8 || data[i + 2] > 127 || data[i + 3] > 15) return -1;
    memcpy(map, data, sizeof map);
    return 0;
}
