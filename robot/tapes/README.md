# CP1 robot tapes

Ready-to-load **CP1 memory images** for the robot/matrix Hanoi program
([`../HANOIC-CP1-ROBOT.txt`](../HANOIC-CP1-ROBOT.txt), which includes the
`1 - strobe` fix). Each `.bin` is a 512-byte image (256 cells, opcode/operand
interleaved) — the same format the cassette toolchain produces.

| Image | Disks | Moves (2ᴺ−1) | Notes |
|---|---|---|---|
| `hanoi-robot-3disk.bin` | 3 | 7 | quick bring-up |
| `hanoi-robot-5disk.bin` | 5 | 31 | |
| `hanoi-robot-6disk.bin` | 6 | 63 | matches `matrix` default `NUM_DISKS=6` |
| `hanoi-robot-7disk.bin` | 7 | 127 | near the panel's clean-rendering limit |
| `hanoi-robot-6disk-endless.bin` | 6 | ∞ | loops forever (`019` jumps to `000` instead of `HLT`); the matrix auto-rebuilds each cycle |

## Getting a playable tape

The `.wav` files are large (~19 MB each) and are **not** committed. Build them
from the images with the cassette toolchain:

```sh
./build-tapes.sh                 # renders every *.bin here to a matching *.wav
```

Then play a `.wav` into the **CP2** module with the CP1 in `CAL` mode. (Loading a
256-cell image is ~7 minutes of audio.) Prebuilt tapes may also be attached to a
GitHub Release, mirroring the `cassette/` project's convention.

## Making a different disk count

Edit address `008` in [`../HANOIC-CP1-ROBOT.txt`](../HANOIC-CP1-ROBOT.txt)
(`008 ako 04.0NN`, for 3–13 disks), assemble, and render:

```sh
python3 ../../cassette/cp1asm.py ../HANOIC-CP1-ROBOT.txt hanoi-robot-NNdisk.bin
python3 ../../cassette/cp1wav.py hanoi-robot-NNdisk.bin hanoi-robot-NNdisk.wav --rate 22050
```

For an **endless** build of any size, start from
[`../HANOIC-CP1-ROBOT-endless.txt`](../HANOIC-CP1-ROBOT-endless.txt) instead
(its only difference is `019 spu 09.000` in place of the `HLT`).

## Bring-up

See [`../debug/README.md`](../debug/README.md) for the receiver-first, CP1-last
bring-up order and the debugging tools.
