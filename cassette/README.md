# CP1 cassette loader — generate a loadable WAV (no hand-keying)

Tools to get a CP1 program onto a real Kosmos CP1 **without typing it in cell by
cell**: assemble a listing into a memory image, turn it into a **cassette WAV**,
and play that into the **CP2 Kassetten-Modul** (the CP1's `CAL` cassette-load).
Plus a decoder to read real CP1 tapes back.

> These tools also live as a standalone, more general project with an assembler
> tutorial and examples:
> **[kosmos-cp1-devel-toolchain](https://github.com/lambdamikel/kosmos-cp1-devel-toolchain)**.

## Ready-made tapes

Don't want to generate them? Two prebuilt WAVs are on the
**[`cassette-tapes` release](https://github.com/lambdamikel/towers-of-hanoi/releases/tag/cassette-tapes)**:
- **`hanoi-original.wav`** — solves Hanoi and shows each move on the CP1's own display;
- **`hanoi-matrix.wav`** — emits moves over Port 2 to drive the [`matrix/`](../matrix/) LED panel (or the [`robot/`](../robot/)).

Play one into the CP2 with the CP1 in `CAL` mode. (Needs the CP3 memory expansion.)

The cassette format is **not documented numerically anywhere** — the CP2 manual
only names the two FSK tones symbolically and no emulator implements CP2. It was
**reverse-engineered here from a digitized real CP2 save** (`hanoi-cp1.wav`) and
validated by a byte-exact round-trip.

## The format (reverse-engineered)

- **FSK, two continuous-phase carriers:** `f1 ≈ 984 Hz` (low) and `f2 ≈ 2250 Hz` (high).
- **Each bit = an f2 burst then an f1 burst**, ~100 ms total; the *longer* burst
  encodes the value (the 1/3 : 2/3 duty cycle the manual mentions, which also
  makes it tolerant of cassette speed wobble):
  - bit **1** → 35 ms f2 + 65 ms f1
  - bit **0** → 65 ms f2 + 35 ms f1
- **Bit boundary = the f1→f2 (falling) edge** → the decoder self-clocks, so wow/flutter is fine.
- **LSB first.** ~10 baud, so a full 256-cell program is ~7 minutes of audio.
- **Lead-in:** ~16 s of steady f1. **Trailing:** the line drops to 0 V (= f2), closing the last bit.
- **Memory image:** each CP1 cell is a 16-bit word `(opcode<<8)|operand`; the
  image stores **opcode then operand per cell, interleaved** → 512 bytes = 256 cells.
- **On tape, per 256-byte block the byte order is `0, 255, 254, … , 1`.**

(Bit timing and byte order match [asig/kosmos_tape_emulator](https://github.com/asig/kosmos_tape_emulator);
the image layout matches the [asig/kosmos-cp1](https://github.com/asig/kosmos-cp1) emulator/assembler.)

## Tools

```sh
# 1. assemble a listing -> memory image  (real assembler: mnemonics, labels,
#    DATA/ORG/EQU; legacy numeric listings like HANOIC-CP1-ROBOT.txt also work)
python3 cp1asm.py  ../robot/HANOIC-CP1-ROBOT.txt  hanoi.bin

# 2. memory image -> cassette WAV
python3 cp1wav.py  hanoi.bin  hanoi.wav            # --f1 984 --f2 2250 --lead 16

# 3. decode / verify a WAV (a real tape, or a generated one)
python3 cp1decode.py  hanoi.wav  hanoi.bin         # -> 512/512 byte match
```

**To load on the real machine:** put the CP1 in cassette-load (`CAL`), then play
`hanoi.wav` into the CP2's recorder-playback input; adjust the recorder volume as
the manual describes.

## Validation

- **Round-trip** `cp1wav.py → cp1decode.py` reproduces the image **512/512 bytes**.
- Decoding a **genuine CP2 recording** recovers the program correctly (init cells
  `04.200, 06.146, 04.201, …` exactly).

## Calibration

`f1`, `f2` and the timing were measured from one real CP1/CP2 save. If your unit
or recorder differs, decode one of your own `CAS` saves with `cp1decode.py`,
re-measure, and pass `--f1/--f2` to `cp1wav.py`.

*Reverse-engineering and tools by Claude (Opus 4.8), directed by LambdaMikel.*
