/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Knob/fader assignments (MAP page) and LIVE SETUP. Included by
 * smk37_controls.c only, after its own code. */
/* Factory default LIVE layout (track 0 = SEL). Bump MAP_MAGIC in project.c
 * whenever this changes, so a saved map does not hide the new default. */
static struct smk37_map_entry map[SMK37_MAP_CONTROLS] = {
    {.target=SMK37_MAP_KEYS}, {.target=SMK37_MAP_SOUND}, {.target=SMK37_MAP_DELAY}, {.target=SMK37_MAP_DLY_TIME},
    {.target=SMK37_MAP_LFO_WAVE}, {.target=SMK37_MAP_LFO_RATE}, {.target=SMK37_MAP_LFO_DEPTH}, {.target=SMK37_MAP_FX_RETURN},
    {.target=SMK37_MAP_REVERB}, {.target=SMK37_MAP_REV_TIME}, {.target=SMK37_MAP_DEFAULT}, {.target=SMK37_MAP_LEVEL},
};
uint8_t smk37_setup[4];
static uint8_t map_fader_last[SMK37_MAP_FADERS], map_fader_seen, map_cc[SMK37_MAP_CONTROLS];
uint8_t smk37_map_last = 255;
uint16_t smk37_map_moves;
unsigned smk37_map_cc_value(unsigned control) { return map_cc[control % SMK37_MAP_CONTROLS]; }

const char *const smk37_map_target_names[SMK37_MAP_TARGETS] = {
    "DEFAULT", "LEVEL", "REVERB", "DELAY", "CUTOFF", "LFO RATE", "LFO DEPTH", "LFO WAVE",
    "FM FDBK", "REV TIME", "DLY TIME", "DLY FDBK", "FX RETURN", "TEMPO", "MIDI CC", "SOUND", "KEYS TRK"
};
const struct smk37_map_entry *smk37_map_get(unsigned control)
{ return &map[control < SMK37_MAP_CONTROLS ? control : 0]; }

static unsigned map_add(unsigned value, int delta, unsigned max)
{ int next = (int)value + delta; return next < 0 ? 0U : next > (int)max ? max : (unsigned)next; }

void smk37_map_edit(unsigned control, unsigned parameter, int delta)
{
    if (control >= SMK37_MAP_CONTROLS || !delta) return;
    struct smk37_map_entry *e = &map[control];
    unsigned cc = e->target == SMK37_MAP_CC;
    if (parameter == 3) e->all = delta > 0;            /* K5: right = ALL, left = LIVE */
    else if (parameter == 2) { if (!cc) return; e->channel = map_add(e->channel, delta, 15) & 15U; }
    else { uint8_t *field = parameter == 0 ? &e->target : cc ? &e->cc : &e->track;
        *field = (uint8_t)map_add(*field, delta, parameter == 0 ? SMK37_MAP_TARGETS - 1 : cc ? 127 : 8); }
    groove_settings_panel_changed();
}
void smk37_map_clear(unsigned control)
{ if (control < SMK37_MAP_CONTROLS) { map[control].target = 0; groove_settings_panel_changed(); } }
void smk37_setup_edit(unsigned row, int delta)
{
    static const uint8_t max[SMK37_SETUP_ROWS] = {3, 1, 2};
    if (row >= SMK37_SETUP_ROWS || !delta) return;
    smk37_setup[row] = (uint8_t)map_add(smk37_setup[row], delta, max[row]);
    groove_settings_panel_changed();
}
/* Velocity curve for keys and pads (LIVE SETUP). */
static unsigned map_velocity(unsigned v)
{
    unsigned curve = smk37_setup[SMK37_SETUP_VELOCITY];
    if (!v || !curve) return v;
    v = curve == 1 ? v + (127U - v) / 2U : curve == 2 ? v * v / 127U : 100U;
    return v ? v : 1U;
}

/* Apply one assignment: delta for relative motion, or absolute >= 0 for a fader. */
static void map_apply(unsigned control, int delta, int absolute)
{
    const struct smk37_map_entry *e = &map[control];
    smk37_map_last = (uint8_t)control; ++smk37_map_moves;
    unsigned target = e->target, t = e->track ? e->track - 1U : smk37_screen_track() % GROOVE_TRACKS;
    unsigned dx7 = groove_source_type(t) == GROOVE_DX7;
    if (target == SMK37_MAP_LEVEL) groove_control_edit(t, 0, delta, 0);
    else if (target == SMK37_MAP_CUTOFF) groove_control_edit(t, 0, 0, delta);
    else if (target <= SMK37_MAP_DELAY) {
        unsigned which = target == SMK37_MAP_DELAY;
        if (absolute >= 0) {
            uint8_t sends[2]; groove_sends_get(t, sends);
            delta = (int)(((unsigned)absolute * 15U + 63U) / 127U) - (int)sends[which];
        }
        groove_sends_edit(t, which, delta);
    } else if (target <= SMK37_MAP_LFO_WAVE) {
        /* DX7: LFO SPEED, PMD (vibrato), WAVE. Others: rate, depth, wave. */
        unsigned k = target - SMK37_MAP_LFO_RATE;
        if (dx7) groove_fm_edit(t, k == 0 ? 137U : k == 1 ? 139U : 142U, delta);
        else groove_lfo_edit(t, k == 2 ? 3U : k, delta);
    } else if (target == SMK37_MAP_FM_FEEDBACK) { if (dx7) groove_fm_edit(t, 135, delta); }
    else if (target <= SMK37_MAP_FX_RETURN) groove_effects_edit(target - SMK37_MAP_REV_TIME, delta);
    else if (target == SMK37_MAP_TEMPO) groove_tempo_edit(delta);
    else if (target == SMK37_MAP_SOUND) groove_control_edit(t, delta, 0, 0); /* next/previous sound of the track */
    else if (target == SMK37_MAP_KEYS) smk37_screen_keys_track(delta);       /* track played by the keys */
    else {
        unsigned value = absolute >= 0 ? (unsigned)absolute : map_add(map_cc[control], 2 * delta, 127);
        map_cc[control] = (uint8_t)value;
        groove_midi_cc_out(e->channel, e->cc, value);
    }
}
int smk37_map_encoder(unsigned id, int delta)
{
    unsigned mode = smk37_screen_map_mode();
    if (id >= SMK37_MAP_KNOBS || mode == 2 || (!mode && !(map[id].target && map[id].all))) return 0;
    /* LIVE: unassigned K1-K7 do nothing but show "-"; K8 stays MASTER. */
    if (!map[id].target) { if (id < 7) { smk37_map_last = (uint8_t)id; ++smk37_map_moves; } return id < 7; }
    map_apply(id, delta << smk37_setup[SMK37_SETUP_SPEED], -1);
    return 1;
}
/* LEVEL faders keep the panel's pickup logic in fader(); sends and MIDI CC
 * are absolute, the other targets follow fader motion. Positions are tracked
 * for every target, so a new assignment acts on the next physical move. */
int smk37_map_fader(unsigned id, unsigned value)
{
    unsigned control = SMK37_MAP_KNOBS + id, bit = 1U << id, target = map[control].target;
    int previous = map_fader_last[id];
    unsigned seen = map_fader_seen & bit;
    map_fader_last[id] = (uint8_t)value; map_fader_seen |= (uint8_t)bit;
    unsigned mode = smk37_screen_map_mode();
    if (target == SMK37_MAP_LEVEL || mode == 2 || (!mode && !(target && map[control].all))) return 0;
    if (seen && (int)value != previous) {
        /* LIVE: an unassigned fader does nothing but shows "-". */
        if (!target) { smk37_map_last = (uint8_t)control; ++smk37_map_moves; return 1; }
        unsigned absolute = target == SMK37_MAP_REVERB || target == SMK37_MAP_DELAY || target == SMK37_MAP_CC;
        map_apply(control, (int)value - previous, absolute ? (int)value : -1);
    }
    return 1;
}
void smk37_map_preferences_read(uint8_t data[SMK37_MAP_BYTES])
{ memcpy(data, map, sizeof map); memcpy(data + sizeof map, smk37_setup, sizeof smk37_setup); }
int smk37_map_preferences_apply(const uint8_t data[SMK37_MAP_BYTES])
{
    for (unsigned i = 0; i < sizeof map; i += 4)
        if (data[i] >= SMK37_MAP_TARGETS || data[i + 1] > 8 || data[i + 2] > 127 || data[i + 3] > 31) return -1;
    if (data[48] > 3 || data[49] > 1 || data[50] > 2) return -1;
    memcpy(map, data, sizeof map); memcpy(smk37_setup, data + sizeof map, sizeof smk37_setup);
    return 0;
}
