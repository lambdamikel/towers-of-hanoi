# Wiring Diagram — Hanoi Robot

## Power Topology

```
                             ┌──────────────────┐
   AC wall ─► [12V/3A PSU] ─►│ barrel jack adap │
                             │ (5.5×2.1 → screw)│
                             └────┬─────────┬───┘
                              +12V│      GND│
                                  │         │
        ┌─────────────────────────┼─────────┼───────────┐
        │                         │         │           │
        ▼                         ▼         │           ▼
  ┌───────────┐         ┌───────────────┐   │   ┌────────────────┐
  │  IRF520   │         │ Buck 12V→6V   │   │   │ Arduino Uno    │
  │  module   │         │ (set to 6.0V) │   │   │  VIN  ◄── +12V │
  │           │         │               │   │   │  GND  ◄── GND  │
  │  V+ ◄ +12V│         │ IN+  ◄── +12V │   │   │                │
  │  GND ◄ GND│         │ IN−  ◄── GND  │   │   │ (internal 5V   │
  │           │         │ OUT+ ──► +6V  │   │   │  regulator     │
  │  SIG ◄ pin5         │ OUT− ──► GND  │   │   │  feeds logic)  │
  │  Vcc ◄ +5V (Arduino)│               │   │   │                │
  │           │         └───────┬───────┘   │   └────────────────┘
  │  OUT+ ───►├──┐              │           │   (OUT+ / OUT− feed
  │  OUT− ───►├──┤              │+6V        │GND  the parallel array
  │           │  │              ▼           │    of 4× P15/5 coils)
  └───────────┘  │      ┌──────────────┐    │
                 │      │ Pan servo    │    │
                 │      │  RED  ◄ +6V  │    │
                 │      │  BLK  ◄ GND  ◄────┤
                 │      │  WHT  ◄ pin9 │    │
                 │      └──────────────┘    │
                 │      ┌──────────────┐    │
                 │      │ Tilt servo   │    │
                 │      │  RED  ◄ +6V  │    │
                 │      │  BLK  ◄ GND  ◄────┤
                 │      │  WHT  ◄ pin10│    │
                 │      └──────────────┘    │
                 │                          │
                 ▼                          │
        ┌─────────────────────────┐         │
        │ Head plate (4× P15/5)   │         │
        │ ┌────┐ ┌────┐           │         │
        │ │ M1 │ │ M2 │ all 4     │         │
        │ └────┘ └────┘ coils in  │         │
        │ ┌────┐ ┌────┐ parallel  │         │
        │ │ M3 │ │ M4 │ → IRF520  │         │
        │ └────┘ └────┘ OUT+/OUT− │         │
        │  (central peg-clearance │         │
        │   hole, ~10 mm)         │         │
        └─────────────────────────┘         │
                                            │
   ALL GROUNDS COMMON: PSU GND, buck GND,   │
   MOSFET GND, Arduino GND must all tie ────┘
   together (single star point recommended)
```

## Signal Wiring — Arduino Uno

```
   Arduino Uno
  ┌─────────────────────┐
  │              +5V ───┼─► IRF520 Vcc
  │              GND ───┼─► common ground rail
  │                     │
  │   pin 2 (INT0) ◄────┼── Microtronic D3   (STROBE)
  │   pin 3        ◄────┼── Microtronic D0   (bit 0)
  │   pin 4        ◄────┼── Microtronic D1   (bit 1)
  │   pin 5  (PWM) ─────┼─► IRF520 SIG       (magnet PWM, 980 Hz)
  │   pin 7        ─────┼─► Microtronic IN0  (BUSY back to host)
  │   pin 8        ◄────┼── Microtronic D2   (bit 2)
  │   pin 9  (PWM) ─────┼─► Pan servo signal
  │   pin 10 (PWM) ─────┼─► Tilt servo signal
  │                     │
  │  USB / barrel jack: │
  │   VIN ─── +12V from PSU
  │   GND ─── common ground
  └─────────────────────┘
```

## Microtronic 2090 ↔ Arduino

```
   Microtronic 2090            Arduino
  ┌──────────────────┐        ┌────────────┐
  │ Output D0  ──────┼────────┤ pin 3      │
  │ Output D1  ──────┼────────┤ pin 4      │
  │ Output D2  ──────┼────────┤ pin 8      │
  │ Output D3  ──────┼────────┤ pin 2 INT0 │  (STROBE)
  │                  │        │            │
  │ Input  IN0 ◄─────┼────────┤ pin 7      │  (BUSY)
  │                  │        │            │
  │ GND    ──────────┼────────┤ GND        │  *MUST common ground
  └──────────────────┘        └────────────┘
```

## Build Notes / Gotchas

1. **Common ground is critical.** All four GND domains (PSU, buck output,
   MOSFET module, Arduino) must tie together. Also share GND with the
   Microtronic. Without this, the MOSFET won't switch reliably and the
   servos may twitch.

2. **Verify buck output is 6.0V** with a multimeter BEFORE plugging in
   the servos. Some modules ship at 5V or 9V; trim pot adjusts.

3. **IRF520 module flyback diode.** Most modules have one across the
   output; verify with a multimeter (diode-test from OUT+ to V+, should
   read ~0.6V one direction). If absent, solder a 1N4007 across the
   parallel coil array (cathode to +12V side). One diode across the
   parallel combination protects all four coils — they appear as a
   single equivalent inductance to the MOSFET.

4. **Magnet wire routing (4-coil head).** All four P15/5 coils on the
   head plate are wired in parallel: tie all four "+" leads together
   to one bus, all four "−" leads to the other. Run that single twisted
   pair from the head plate, along the underside of the U-channel arm,
   back to the IRF520 OUT+/OUT− terminals. Use stranded 22 AWG; leave
   a service loop at the pan pivot so the arm can sweep without tugging.
   Total coil current ~1 A at 12V.

5. **Servo power separation.** Do NOT feed the servos from Arduino's 5V
   pin — the on-board regulator can't supply the ~2 A peak. The buck
   converter is what supplies the servos; Arduino just shares ground.

6. **Microtronic GND tie.** Both systems are TTL, but they each have
   their own PSU. Tie their grounds together or signal levels are
   undefined relative to each other.

7. **Star-point grounding** (recommended): pick one point on the
   baseplate as the common GND, run all GND wires individually to that
   point rather than daisy-chaining. Reduces noise and ground-loop
   issues, especially when the magnet switches.
```
