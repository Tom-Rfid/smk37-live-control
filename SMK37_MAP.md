# Control map (BT long press)

Hold **BT** for 0.6 s to open **CONTROL MAP**. A short press still opens the
BLUETOOTH page (on release).

Each of the twelve controls, K1–K8 and F1–F4, can keep its normal behaviour
(`DEFAULT`) or drive one target:

| Target | Knob (relative) | Fader (absolute) |
|---|---|---|
| LEVEL | track level | track level, with the usual pickup |
| REVERB / DELAY | track send, one step per detent | track send 0–100 % |
| CUTOFF | track cutoff | follows fader motion |
| MIDI CC | 2 per detent, 0–127 | 0–127 |

Tracks are T1–T8 or `SELECTED TRACK` (follows the focused track). MIDI CC is
sent on the enabled MIDI output ports only; it does not reach the internal
engine.

On the MAP page: **K1** (or PRESETS) chooses the row, **K2** the target,
**K3** the track or CC number, **K4** the MIDI channel. Moving a fader selects
its row. **SAVE** sets the row back to DEFAULT. **HOME** exits.

Knob assignments are active on HOME and MIX only; editor pages keep their own
knobs, and holding **SHIFT** bypasses them. Fader assignments apply on every
page. Assignments are device preferences, saved with the panel settings
(independent of projects and undo) about 1.5 s after the last change.

Storage: a 56-byte tail after the panel settings in the tuning/preferences
sector, with its own magic and CRC. Earlier firmware ignores it; a unit that
never stored it starts with every control on DEFAULT. The tuning journal magic
changes to 0x4d544d46 (full sector); 0x4c544d46 journals from V01 are still
read with their original length.

Host test: `tools/host-tests/run.sh` (clang on Linux, macOS or WSL).
