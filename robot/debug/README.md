# CP1 robot — protocol debugging kit

Tools built while bringing up the CP1 → Arduino move protocol on real hardware.
They diagnosed (and fixed) a freeze where the CP1 program loaded fine but stopped
almost immediately, driving no moves.

## What was wrong (and the fix)

The CP1's **`P2A` writes "any non-zero → 1", not the LSB** of the accumulator.
The strobe bit was produced by an *incrementing* counter (cell 155: 1, 2, 3, …),
so after the first move the value was always non-zero → the strobe **jammed
HIGH**, the Arduino never saw another edge, and the CP1 hung at the next
handshake. This was the second assumption flagged in the program's own notes; it
is now **confirmed on hardware**.

Fix (in [`../HANOIC-CP1-ROBOT.txt`](../HANOIC-CP1-ROBOT.txt), cells 089–092):
replace the `+1` counter with **`strobe := 1 - strobe`**, so the value written is
always exactly `0` or `1` — correct under either `P2A` interpretation.

```
089 ako 04.001     # accu = 1
090 sub 08.155     # accu = 1 - strobe
091 abs 06.155     # strobe := 1 - strobe   (clean 0<->1 toggle)
092 p2a 18.004     # write 0/1 to Port 2 pin 4
```

## The tools

### `cp1emu.py` — offline CP1 emulator / protocol tracer
Runs a CP1 listing with no hardware and logs every `ANZ`, `P2A` wire change,
decoded strobe edge (→ move), and `P1E` (busy) read. The BUSY line and the two
I/O-semantic assumptions are all selectable, so every theory is testable offline:

```sh
python3 cp1emu.py ../HANOIC-CP1-ROBOT.txt                    # busy=auto: full solve
python3 cp1emu.py ../HANOIC-CP1-ROBOT.txt --busy high        # reproduces the freeze
python3 cp1emu.py ../HANOIC-CP1-ROBOT.txt --p2a nonzero      # the real hardware case
python3 cp1emu.py ../HANOIC-CP1-ROBOT.txt --trace            # full instruction trace
```

It reuses the assembler in [`../../cassette/cp1asm.py`](../../cassette/cp1asm.py),
so any legacy CP1 listing loads unchanged. **Note:** `cp1emu.py`'s core is a
general-purpose CP1 interpreter — only its `diagnose_loop()` hints (the busy-poll
cell numbers) are Hanoi-specific. It is a natural candidate to upstream into the
standalone **[kosmos-cp1-devel-toolchain](https://github.com/lambdamikel/kosmos-cp1-devel-toolchain)**
alongside `cp1asm`/`cp1wav`/`cp1decode`.

### `cp1_sniffer/cp1_sniffer.ino` — Mega serial protocol sniffer
Drop-in replacement for the matrix/robot receiver (**same pins, D30–D34**). It
prints a decoded line on every strobe edge plus a 1 Hz level heartbeat, and puts
**BUSY under your control** so you can single-step or auto-run the CP1:
`h`/`l` force BUSY, `a` auto-acks each edge, `p` is a floating-line test, `s`/`?`
status/help.

### `CP1-PORTTEST.txt` — output-path test (no handshake)
Loops emitting two move codes with a strobe toggle, never reading BUSY. Isolates
Port 2 data + strobe wiring + decode from the handshake. (It also used the old
`+1` strobe, which is exactly how the `P2A` bug was caught — the data bits toggled
but the strobe stuck HIGH.)

### `CP1-BUSYTEST.txt` — input-path test
Reads Port 1 pin 1 and shows it on the CP1's own display (and mirrors to D0), so
you can toggle the Mega's BUSY and confirm the `P1E` read and its polarity.

## Bring-up order (the handshake is edge-triggered)

Get the receiver acking **before** the CP1 emits, hold STROBE low when the Mega
samples its reference, and start the CP1 **last**:

1. CP1: reset → PC `000`, **don't run** (holds STROBE low).
2. Mega: reset / open the sniffer monitor (samples a LOW reference). Sniffer only:
   type `a`. (The matrix auto-acks in hardware — nothing to type.)
3. CP1: **RUN**.

Auto-ack can't rescue an already-stuck CP1 (it needs a *fresh* edge) — reset the
CP1, not the Mega.

## Tapes

Ready-to-load memory images and a WAV builder are in [`../tapes/`](../tapes/).
