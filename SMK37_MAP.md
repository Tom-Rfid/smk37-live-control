
**Default LIVE layout** (loaded fresh after this update; it replaces any map saved by an earlier build, and resets LIVE SETUP): K1 KEYS TRK, K2 SOUND (SEL), K3 DELAY, K4 DLY TIME, K5 LFO WAVE (SEL), K6 LFO RATE, K7 LFO DEPTH, K8 FX RETURN, F1 REVERB (SEL), F2 REV TIME, F3 default, F4 LEVEL (keyboard volume; its bar turns green within 5% of max and red at max). On LIVE the readout label and the sound name take the track's colour, as in the overview.
# LIVE control page (BT)

Assignments play **only on the BT page**, so the firmware's own knob and fader
roles stay untouched everywhere else.

- **BT click**: opens **LIVE CONTROL** (grey, orange text). Assigned knobs and
  faders act here; the last one moved is shown large with its value and a bar.
  Unassigned K1-K7 and F1-F4 do nothing on LIVE and show "-"; K8 stays MASTER.
  Keys keep playing; the last key or pad played is shown beside the title.
- **BT held 1 s** (green progress bar): editing
  (CONTROL MAP 1/2, LIVE SETUP 2/2; OCT-/OCT+ change page).
- In CONTROL MAP, **K5** sets each assignment to **LIVE** (default: acts on the
  BT page only) or **ALL** (acts on every page except editing, replacing that
  control's normal role; the row shows "ALL"). Prefer ALL on controls you do not
  need for menu navigation (K1-K4 are used by most menus).
- In editing, **BT** returns to LIVE. On LIVE, **BT** exits. OCT-/OCT+ change
  the keys octave on LIVE (in editing they switch page).
  **`<` and PATCH** keep their HOME role on both pages: they leave to the track overview.
- On-screen hints name the physical buttons: `</PATCH` (HOME), `>` (SAVE),
  KNOB BANK (SHIFT), PARA (EDIT), CHORD (ENV), GLOBE (MIX), NOTE REPEAT
  (PAD/STEP), K5 (SELECT), K6 (PAGE), K7 (PRESETS).
- SOURCE page (KNOB BANK+PATCH): turning K2 (track mode) returns to the
  previous page 300 ms after the last change.
- LFO targets (RATE, DEPTH, WAVE) draw a preview on LIVE: current shape,
  1-6 cycles from RATE, height from DEPTH.
- SOUND and LFO WAVE show `n/max` (the sound/shape and how many the engine has). SOUND
  also shows the sound name under the big value. Each LIVE row shows its own value
  at the end (e.g. `K4 KEYS TRK T8`).

This build removes the BLUETOOTH settings page, the DX7 PATCH NAME page and the
startup animation, the BLE stage timer and the ACTIONS shortcuts, shortens some
status messages, and size-optimises the LCD transfer
functions: the V01 app image had only 29 bytes of free Flash (this build: 29).

Each of the twelve controls, K1–K8 and F1–F4, can keep its normal behaviour
(`DEFAULT`) or drive one target:

| Target | Scope | Fader |
|---|---|---|
| LEVEL | track | absolute, with pickup (or JUMP) |
| REVERB / DELAY | track send | absolute 0–100 % |
| CUTOFF | track filter (BASS 303 only; other engines show "303 ONLY") | follows motion |
| LFO RATE / LFO DEPTH / LFO WAVE | track LFO (DX7: SPEED, PMD, WAVE TRI/SAW-/SAW+/SQR/SINE/S&H; others: rate, depth, TRI/SQR/UP/DOWN) | follows motion |
| FM FDBK | DX7 operator feedback | follows motion |
| REV TIME / DLY TIME / DLY FDBK / FX RETURN | shared effects | follows motion |
| TEMPO | BPM | follows motion |
| MIDI CC | external CC, channel | absolute 0–127 |
| SOUND | next / previous sound (program) of a track | follows motion |
| KEYS TRK | track played by the keys (T1–T8) | follows motion |

There is no chorus effect in the SMK37 Open engine.

Page 2/2 **LIVE SETUP** (BT again, or OCT-/OCT+): VELOCITY NORMAL/SOFT/HARD/FIXED 100
(keys and pads), FADERS PICKUP/JUMP, KNOB SPEED X1/X2/X4 (assigned knobs),
BLUETOOTH ON/OFF (stored with the device settings; OFF disables Bluetooth audio
and MIDI). K1 row, K2 value, SAVE default.

Tracks are T1–T8 or `SELECTED TRACK` (follows the focused track). MIDI CC is
sent on the enabled MIDI output ports only; it does not reach the internal
engine.

On the MAP page: **K1** (or PRESETS) chooses the row, **K2** the target,
**K3** the track (SEL = selected track) or CC number, **K4** the MIDI channel.
Moving a fader selects its row. **SAVE** sets the row back to DEFAULT. **HOME** exits.

Assignments are device preferences, saved with the panel settings
(independent of projects and undo) about 1.5 s after the last change.

Storage: a 60-byte tail (magic SMK2) after the panel settings in the tuning/preferences
sector, with its own magic and CRC. Earlier firmware ignores it; a unit that
never stored it starts with every control on DEFAULT. The tuning journal magic
changes to 0x4d544d46 (full sector); 0x4c544d46 journals from V01 are still
read with their original length.

Host test: `tools/host-tests/run.sh` (clang on Linux, macOS or WSL).

Also removed for Flash space: the DX7 PATCH NAME page and the startup animation.

Size: Elite build 453,444-byte app image for a 453,921-byte limit (V01: 453,892).

## VS Code

Open this folder in VS Code (File > Open Folder) and install the recommended C/C++ extension.

- `Ctrl+Shift+B`: build the Elite firmware (`build-elite.bat`, gives `SMK37_ELITE_OPEN_MAP.fwsc`).
- Terminal > Run Task: workbench to flash (`build.bat elite --package`), browser simulator (`build.bat --web`), simulator tests.
- `F5`: build the simulator tests with `-g` using CodeBlocks' MinGW and debug them with gdb. Put breakpoints in `app/smk37_map.inc`, `app/screen.c` or `app/smk37_controls.c`.
- Without VS Code: `tools\host-tests\run.bat` runs the same tests.
