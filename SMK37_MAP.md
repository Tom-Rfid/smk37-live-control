# LIVE control page (BT)

Assignments play **only on the BT page**, so the firmware's own knob and fader
roles stay untouched everywhere else.

- **BT click**: opens **LIVE CONTROL** (grey, orange text). Assigned knobs and
  faders act here; the last one moved is shown large with its value and a bar.
  Unassigned K1-K7 do nothing on LIVE; K8 stays MASTER. Keys keep playing.
- **HOME** (`<` or PATCH) or **BT held 2 s** (green progress bar): editing
  (CONTROL MAP 1/2, LIVE SETUP 2/2; OCT-/OCT+ change page).
- In editing, **HOME** or **BT** returns to LIVE. On LIVE, **BT** exits.

This build removes the BLUETOOTH settings page, the DX7 PATCH NAME page and the
startup animation, and size-optimises the LCD transfer functions: the V01 app
image had only 29 bytes of free Flash.

Each of the twelve controls, K1–K8 and F1–F4, can keep its normal behaviour
(`DEFAULT`) or drive one target:

| Target | Scope | Fader |
|---|---|---|
| LEVEL | track | absolute, with pickup (or JUMP) |
| REVERB / DELAY | track send | absolute 0–100 % |
| CUTOFF | track filter | follows motion |
| LFO RATE / LFO DEPTH / LFO WAVE | track LFO (DX7: SPEED, PMD, WAVE TRI/SAW-/SAW+/SQR/SINE/S&H; others: rate, depth, TRI/SQR/UP/DOWN) | follows motion |
| FM FDBK | DX7 operator feedback | follows motion |
| REV TIME / DLY TIME / DLY FDBK / FX RETURN | shared effects | follows motion |
| TEMPO | BPM | follows motion |
| MIDI CC | external CC, channel | absolute 0–127 |

There is no chorus effect in the SMK37 Open engine.

Page 2/2 **LIVE SETUP** (BT again, or OCT-/OCT+): VELOCITY NORMAL/SOFT/HARD/FIXED 100
(keys and pads), FADERS PICKUP/JUMP, KNOB SPEED X1/X2/X4 (assigned knobs). K1 row,
K2 value, SAVE default. ACTIONS lists CONTROL MAP and LIVE SETUP.

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
