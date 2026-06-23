#!/usr/bin/env python3
"""
CP1 listing -> .bin memory image.

Produces the exact byte format used by the asig/kosmos-cp1 emulator and the
asig/kosmos_tape_emulator (so the result loads in the emulator for validation,
and onto a real CP1 via the tape emulator):

  - each CP1 cell is one 16-bit word = (opcode << 8) | operand
  - the .bin stores, per cell i, two bytes: [2i] = opcode, [2i+1] = operand
  - size = 256 bytes if the highest cell used <= 127, else 512 bytes (256 cells)

Input lines look like:   '<addr> [mnemonic] <OP>.<operand>   # comment'
We read the numeric  OP.operand  directly (mnemonic is ignored), so any
flag-annotated jumps like 'spb 11.011 (A2)' just work.
"""
import re, sys

def assemble(text):
    mem = [(0, 0)] * 256          # (opcode, operand) per cell, default 00.000
    maxaddr = -1
    for raw in text.splitlines():
        line = raw.split('#', 1)[0].strip()
        if not line:
            continue
        m_addr = re.match(r'(\d+)\b', line)
        m_oo   = re.search(r'(\d+)\.(\d+)', line)
        if not (m_addr and m_oo):
            continue
        addr = int(m_addr.group(1))
        op, arg = int(m_oo.group(1)), int(m_oo.group(2))
        if not 0 <= addr < 256:
            raise ValueError(f"address {addr} out of range (0..255)")
        if not (0 <= op < 256 and 0 <= arg < 256):
            raise ValueError(f"cell {addr}: {op}.{arg} out of byte range")
        mem[addr] = (op, arg)
        maxaddr = max(maxaddr, addr)
    ncells = 256 if maxaddr > 127 else 128
    out = bytearray()
    for i in range(ncells):
        op, arg = mem[i]
        out += bytes((op, arg))
    return bytes(out), maxaddr

if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: cp1asm.py listing.txt out.bin")
    img, maxaddr = assemble(open(sys.argv[1]).read())
    open(sys.argv[2], "wb").write(img)
    print(f"highest cell = {maxaddr}; wrote {len(img)} bytes ({len(img)//2} cells) -> {sys.argv[2]}")
