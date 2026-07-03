#!/usr/bin/env bash
# Render the committed CP1 memory images (*.bin) to cassette WAVs you can play
# into the CP2 module. The WAVs are large (~19 MB) and gitignored; build them
# here on demand.
#
#   ./build-tapes.sh
#
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
CASS="$HERE/../../cassette"
shopt -s nullglob
built=0
for bin in "$HERE"/*.bin; do
  wav="${bin%.bin}.wav"
  python3 "$CASS/cp1wav.py" "$bin" "$wav" --rate 22050
  echo "  -> $wav"
  built=$((built+1))
done
[ "$built" -gt 0 ] || { echo "no *.bin found in $HERE"; exit 1; }
echo "Done ($built tapes). Play a .wav into the CP2 in CAL mode to load."
echo "For another disk count: edit addr 008 in ../HANOIC-CP1-ROBOT.txt, assemble"
echo "with ../../cassette/cp1asm.py to a new .bin, then re-run this script."
