#!/usr/bin/env python3
"""
cp1emu -- an instruction-level Kosmos CP1 emulator for debugging the Hanoi
robot protocol OFFLINE (no hardware, no Mega, no CP1).

It loads a CP1 listing (the same legacy `<addr> <mnem> OP.operand` format that
cp1asm.py already parses, so HANOIC-CP1-ROBOT.txt loads unchanged), runs it, and
logs everything the robot protocol cares about:

  * every ANZ  -> what the CP1 would show on its own 7-seg display
  * every P2A  -> the state of the four output wires D0 D1 D2 STROBE
  * every strobe edge -> the decoded move (1->2 ... 3->2), in order
  * every P1E  -> what the BUSY line read back as
  * HLT, or a diagnosed infinite loop (which busy-poll it is stuck in)

Because the whole point is to reason about the BUSY handshake, the BUSY input is
*modelled* and selectable:

  --busy auto   (default) pretend a Mega is attached: BUSY idles low, goes high
                on each strobe edge (the ack), then drops low again after a short
                simulated "animation" so the next move can proceed.
  --busy low    BUSY is stuck low  (models: Mega never acks / never raises busy)
  --busy high   BUSY is stuck high (models: input floats/reads high, wrong polarity)

Two I/O-semantic assumptions from CLAUDE.md can also be toggled, so you can test
what the *real* CP1 might be doing:

  --p1e pin|nonzero   how a Port-1 read collapses to 0/1   (default: pin)
  --p2a lsb|nonzero   how P2A writes the accu to a pin     (default: lsb)

Opcode semantics are taken from article/hanoi.tex and the two known-good CP1
programs:  SPU = unconditional jump (used as call), SIU = indirect jump through a
cell (used as return), LIA/AIS = indirect load/store, VGL/VGR/VKL set a compare
flag that SPB branches on. A memory cell's *value* is its operand byte; ABS/AIS
overwrite it.

Usage:
    python3 cp1emu.py ../HANOIC-CP1-ROBOT.txt                 # default: busy auto
    python3 cp1emu.py ../HANOIC-CP1-ROBOT.txt --busy high     # reproduce the freeze
    python3 cp1emu.py ../HANOIC-CP1-ROBOT.txt --trace         # full instruction trace
"""
import sys, os, argparse

# reuse the assembler that already lives in the repo, to parse the listing
HERE = os.path.dirname(os.path.abspath(__file__))
# locate cp1asm.py (the CP1 assembler) whether we live in the working tree's
# debug/ (cassette at ../towers-of-hanoi/cassette) or in the repo's robot/debug/
# (cassette at ../../cassette).
for _cand in ("../towers-of-hanoi/cassette", "../../cassette", "../cassette", "."):
    _p = os.path.join(HERE, _cand)
    if os.path.exists(os.path.join(_p, "cp1asm.py")):
        sys.path.insert(0, _p)
        break
import cp1asm  # noqa: E402

MNEM = {1:'HLT',2:'ANZ',3:'VZG',4:'AKO',5:'LDA',6:'ABS',7:'ADD',8:'SUB',
        9:'SPU',10:'VGL',11:'SPB',12:'VGR',13:'VKL',14:'NEG',15:'UND',
        16:'P1E',17:'P1A',18:'P2A',19:'LIA',20:'AIS',21:'SIU',22:'P3E',
        23:'P4A',24:'P5A'}

MOVE_NAME = {1:'1->2', 2:'1->3', 3:'2->1', 4:'2->3', 5:'3->1', 6:'3->2'}


def load(path):
    img, maxaddr = cp1asm.assemble(open(path).read())
    op  = [img[2*i]   for i in range(len(img)//2)]     # opcode per cell (for fetch)
    val = [img[2*i+1] for i in range(len(img)//2)]     # operand byte = cell's value
    # pad to 256 cells so runtime stack writes (200..255) have somewhere to land
    while len(op) < 256:
        op.append(0); val.append(0)
    return op, val, maxaddr


class Busy:
    """Models the BUSY line the CP1 polls on Port 1."""
    def __init__(self, mode, anim_reads=4):
        self.mode = mode
        self.level = 0          # current busy level 0/1
        self.anim = 0           # remaining "animation" polls before busy drops
        self.anim_reads = anim_reads

    def strobe_edge(self):
        if self.mode == 'auto':
            self.level = 1                 # Mega acks / starts moving
            self.anim = self.anim_reads

    def read(self):
        if self.mode == 'low':
            return 0
        if self.mode == 'high':
            return 1
        # auto: hand back current level, wind the animation down as it's polled
        lvl = self.level
        if self.level == 1:
            self.anim -= 1
            if self.anim <= 0:
                self.level = 0             # move finished
        return lvl


def run(op, val, busy, p1e_mode, p2a_mode, max_steps, trace):
    pc = 0
    accu = 0
    flag = False
    wire = [0, 0, 0, 0]          # D0 D1 D2 STROBE  (pins 1..4)
    last_strobe = 0
    moves = []                   # (code, name) in emission order
    anz_log = []
    p1e_count = 0
    steps = 0
    halted = False

    def bit_of(a):
        return (a & 1) if p2a_mode == 'lsb' else (1 if a != 0 else 0)

    while steps < max_steps:
        steps += 1
        opc = op[pc]
        operand = val_operand(op, pc)   # instruction's own operand byte
        m = MNEM.get(opc, f'?{opc}')
        if trace:
            print(f"  {pc:03d}: {m} .{operand:03d}   accu={accu} flag={int(flag)} "
                  f"wire={wire} busy={busy.level}")
        nxt = pc + 1

        if opc == 1:                                    # HLT
            halted = True
            break
        elif opc == 2:                                  # ANZ  (display accu)
            anz_log.append((pc, accu))
            print(f"[{steps:6d}] ANZ  @{pc:03d}  display shows: {accu}")
        elif opc == 3:                                  # VZG  (delay) -- no-op
            pass
        elif opc == 4:                                  # AKO  accu = operand
            accu = operand
        elif opc == 5:                                  # LDA  accu = mem[operand]
            accu = val[operand]
        elif opc == 6:                                  # ABS  mem[operand] = accu
            val[operand] = accu
        elif opc == 7:                                  # ADD
            accu = (accu + val[operand]) & 0xFF
        elif opc == 8:                                  # SUB
            accu = (accu - val[operand]) & 0xFF
        elif opc == 9:                                  # SPU  jump
            nxt = operand
        elif opc == 10:                                 # VGL  ==
            flag = (accu == val[operand])
        elif opc == 11:                                 # SPB  branch if flag
            if flag:
                nxt = operand
        elif opc == 12:                                 # VGR  >
            flag = (accu > val[operand])
        elif opc == 13:                                 # VKL  <
            flag = (accu < val[operand])
        elif opc == 14:                                 # NEG
            accu = (-accu) & 0xFF
        elif opc == 15:                                 # UND  and
            accu = accu & val[operand]
        elif opc == 16:                                 # P1E  read Port 1 -> accu
            b = busy.read()
            p1e_count += 1
            accu = b if p1e_mode == 'pin' else (1 if b != 0 else 0)
        elif opc == 17:                                 # P1A  (unused)
            pass
        elif opc == 18:                                 # P2A  write pin=operand
            pin = operand
            if 1 <= pin <= 4:
                wire[pin-1] = bit_of(accu)
                if pin == 4:                            # strobe pin
                    if wire[3] != last_strobe:          # edge!
                        last_strobe = wire[3]
                        code = wire[0] | (wire[1] << 1) | (wire[2] << 2)
                        name = MOVE_NAME.get(code, f'INVALID({code})')
                        moves.append((code, name))
                        print(f"[{steps:6d}] STROBE edge#{len(moves)}  "
                              f"D2D1D0={wire[2]}{wire[1]}{wire[0]}  "
                              f"code={code} = {name}")
                        busy.strobe_edge()
        elif opc == 19:                                 # LIA  accu = mem[mem[operand]]
            accu = val[val[operand]]
        elif opc == 20:                                 # AIS  mem[mem[operand]] = accu
            val[val[operand]] = accu
        elif opc == 21:                                 # SIU  indirect jump
            nxt = val[operand]
        else:                                           # ports we don't model
            pass
        pc = nxt

    return {
        'halted': halted, 'steps': steps, 'pc': pc,
        'moves': moves, 'anz': anz_log, 'p1e_count': p1e_count,
        'wire': wire,
    }


# the instruction's operand byte lives in the same `val` array position as the
# cell; but once a *data* cell is overwritten at runtime its value changes while
# its instruction operand should not. Code cells (0..~138) are never written, so
# reading the (immutable) image operand is what we want for fetch. We captured
# both arrays in load(); to keep run() simple we thread the operand via a small
# helper that reads from the pristine opcode-paired operand table.
_OPERANDS = None
def val_operand(op, pc):
    return _OPERANDS[pc]


def diagnose_loop(pc):
    if pc in (61, 62, 63):
        return ("STUCK in the FIRST busy-poll (cells 061-063): waiting for BUSY=LOW.\n"
                "        The CP1 never gets to emit a move or flash the display.\n"
                "        => BUSY is reading HIGH when the CP1 expects LOW.")
    if pc in (93, 94, 95, 96):
        return ("STUCK in the SECOND busy-poll (cells 093-096): waiting for BUSY=HIGH.\n"
                "        One move's data+strobe were already presented.\n"
                "        => the ack (BUSY going HIGH) never arrived.")
    return f"Stuck looping around pc={pc} (not one of the known busy-poll loops)."


def main():
    global _OPERANDS
    ap = argparse.ArgumentParser(description="Kosmos CP1 emulator / protocol tracer")
    ap.add_argument("listing")
    ap.add_argument("--busy", choices=['auto', 'low', 'high'], default='auto')
    ap.add_argument("--p1e", choices=['pin', 'nonzero'], default='pin')
    ap.add_argument("--p2a", choices=['lsb', 'nonzero'], default='lsb')
    ap.add_argument("--anim", type=int, default=4, help="busy=auto: polls per move")
    ap.add_argument("--max-steps", type=int, default=2_000_000)
    ap.add_argument("--trace", action="store_true", help="print every instruction")
    a = ap.parse_args()

    op, val, maxaddr = load(a.listing)
    _OPERANDS = list(val)            # pristine operand table for instruction fetch
    busy = Busy(a.busy, a.anim)

    print(f"# {a.listing}: highest cell {maxaddr};  busy={a.busy}  "
          f"p1e={a.p1e}  p2a={a.p2a}")
    print("#" + "-" * 68)
    r = run(op, val, busy, a.p1e, a.p2a, a.max_steps, a.trace)
    print("#" + "-" * 68)

    codes = [c for c, _ in r['moves']]
    seq = ", ".join(n for _, n in r['moves'])
    print(f"moves emitted : {len(r['moves'])}   [{seq}]")
    print(f"move codes    : {codes}")
    print(f"ANZ displays  : {len(r['anz'])}   values={[v for _,v in r['anz']]}")
    print(f"P1E reads     : {r['p1e_count']}")
    print(f"steps run     : {r['steps']}")
    if r['halted']:
        print(f"RESULT        : HLT reached cleanly at pc={r['pc']}.")
    else:
        print(f"RESULT        : did NOT halt after {r['steps']} steps (last pc={r['pc']}).")
        print("                " + diagnose_loop(r['pc']))


if __name__ == "__main__":
    main()
