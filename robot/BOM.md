# Bill of Materials — Hanoi Robot

## Already ordered

- **Yahboom 2-DOF Metal Servo Pan-Tilt Kit** (Amazon B0BRXVFCKX, variant *without* control board).
  Covers: pan/tilt mechanical platform + both metal-gear servos. Servos drive directly
  from Arduino PWM pins 9 (pan) and 10 (tilt); no separate driver board needed.

## Additional parts required

| # | Part | Spec / Notes |
|---|---|---|
| 1 | Arduino Uno R3 | Or Nano clone. Targets `hanoi_robot.ino` as-is. |
| 2 | 12V / 3A DC power supply | Barrel-jack output, center positive. Single PSU for the whole system. |
| 3 | 5.5×2.1 mm barrel jack → screw-terminal adapter | Splits PSU into +12V / GND rails to feed MOSFET, buck, and Arduino VIN. |
| 4 | IRF520 MOSFET driver module | Switches the electromagnet from pin 5 (PWM, 980 Hz). Verify on-board flyback diode is present. |
| 5 | Adjustable buck converter (LM2596 or similar) | 12V → **6.0V** out for the two servos. Some modules ship at 5V or 9V — trim before connecting servos. |
| 6 | **4× P15/5 12V electromagnets** | ~15 mm OD × 5 mm tall, ~1 kg holding force each, ~0.2–0.3 A each → ~1 A total at 12V. Mounted in a ring on the head plate around a central peg-clearance hole. Wired in parallel, switched together from the IRF520. |
| 7 | 1N4007 diode (×2 spare) | Spare flyback across the parallel magnet array, in case IRF520 module's onboard diode is absent. One diode across the parallel combination is sufficient. |
| 8 | Stranded 22 AWG hookup wire | For magnet leads along the arm. Leave a service loop at the pivot. |
| 9 | Dupont jumper wires (M-M, M-F) | Signal wiring between Arduino, MOSFET, buck, and Microtronic/CP1 host. |
| 10 | Small breadboard or proto-shield | Star ground point + signal fan-out for non-shield connections (Microtronic/CP1 interface). |
| 11 | Multimeter (if not already owned) | Required to set buck to 6.0V before plugging in the servos. |
| 12 | **Arduino Sensor Shield V5.0** | Breaks all Arduino pins out to 3-pin (Signal/V/GND) headers. Has a separate power input for the `V` rail — set the on-board jumper to **external** and feed 6V from the buck. Cleans up wiring and isolates servo motor current from Arduino's 5V regulator (brownout prevention). |
| 13 | 1000 µF electrolytic capacitor (≥10V) | Across the 6V rail at the shield's external V input. Smooths servo start-up and magnet PWM spikes. |
| 14 | 100 µF electrolytic capacitor (≥10V) | Across Arduino 5V/GND as a second line of brownout defense. |

## Mechanical / structural

| # | Part | Spec / Notes |
|---|---|---|
| 15 | Aluminum U-channel | Arm spanning tilt-servo bracket → electromagnet. Suggest ½" × ½" × 1/16" wall (12.7 mm), ~20 cm length. Cut to fit once pan-servo position and peg spacing are finalized. |
| 16 | Plywood baseboard | ~30 × 15 cm × 12 mm. Mounts pan/tilt assembly, Arduino+shield, buck, MOSFET, and PSU breakout. |
| 17 | Steel fender washers, graduated set | 3 sizes for a 3-disk demo: **M10 / M12 / M16 fender** (OD ~30/37/50 mm, ID ~10.5/13/18 mm). Larger hierarchy chosen so all three IDs comfortably clear the 6 mm peg and all three ODs are large enough that the magnet ring on the head plate overlaps the washer's steel annulus. Zinc-plated steel is fine — magnet grabs through the plating. Stock multiples per size for spares / higher N. |
| 18 | Wood screws / M3 standoffs | Mount pan/tilt base, Arduino, buck module, IRF520 module to the plywood. |
| 19 | M3 hardware for head-plate mount | Bolt head plate to U-channel tip (one central bolt, or two for anti-rotation). |
| 20 | **Head plate**: aluminum disc OR 3D-printed PLA | ~50 mm OD × 3 mm thick. Holds the 4 magnets in a ring around a central ~10 mm peg-clearance hole. Drill or print 4 magnet pockets (15 mm OD) symmetric about center, magnet faces flush with bottom surface. |
| 21 | **3× wooden dowels** for pegs | 6 mm dowel × ~30 mm length. Glued into blind holes in the baseboard at the three peg positions. |
| 22 | Wood glue | For pegging the dowels. |

### Multi-magnet head + real pegs

The head has **4 small electromagnets arranged in a ring** around a central clearance hole. The hole lets the peg pass through the head plate as the head descends onto a stack. The washer is picked up against the underside of the head plate, with the peg sliding through the washer's central hole at the same time.

On placement, the head descends with the peg already aligned through both the head's central hole and the washer's ID; the magnets release; the washer drops onto the peg and **self-centers as it slides down**. Stack alignment is enforced mechanically by the peg, not by servo accuracy.

**Geometric constraints (verified for M10/M12/M16 fender washer hierarchy):**
- Peg OD 6 mm < smallest washer ID (M10 fender ~10.5 mm) — peg passes through with ~2 mm radial clearance.
- Head plate central hole 10 mm > peg OD — peg passes through head plate.
- Magnet ring centered at radius ~15 mm — magnet faces span radius ~8–23 mm, overlapping all three washer ODs (15/18.5/25 mm outer radii).
- Peg height ~25–30 mm > max stack height (3 washers × ~3 mm = 9 mm) + head plate thickness + safety.

**Why not pegless** *(rejected approach, preserved for reasoning trail)*: An earlier design proposed shallow recesses in the baseboard to self-locate the *bottom* washer and rely on servo accuracy for everything else. Rejected because servo position error (~±1° → ±2–3 mm at arm radius) accumulates across 15–31 moves for N=4–5 disks; intermediate washers in the stack have nothing to constrain them and would drift, destabilising stacks. Pegs replace "place to ±1 mm by calibration" with "place to ±(peg-hole clearance) by mechanics" — much more forgiving.

### Wiring note for 4 magnets

All four coils in parallel from the IRF520 OUT terminals. The IRF520 module's onboard flyback diode protects the parallel combination as a single equivalent inductance — no need for per-coil diodes, but 1N4007 spares are listed if you want belt-and-braces. Total current ~1 A at 12V — well within the IRF520's rating and inside the 3 A PSU budget alongside the servos.

### Tools (not BOM but required)

- Drill + 6 mm bit for peg blind holes; 10 mm bit (or Forstner) for head-plate central hole.
- Hacksaw or rotary cutter for the U-channel.
- M3 tap if mounting hardware requires threaded holes in the U-channel.
- If using aluminum for head plate: small step drill or 15 mm Forstner for magnet pockets. If 3D printing: model the pockets directly.

## Why no servo driver board

Hobby servos contain their own internal driver electronics. They take a low-current
PWM control pulse (50 Hz, 1–2 ms) which the Arduino `Servo` library produces directly.
The buck converter supplies the *motor current* (up to ~2 A peak per servo); Arduino
only shares ground with it. Dedicated driver boards are needed for stepper motors,
bare DC motors, or large servo counts (>~6) — none of which applies here.

## Verification checklist on arrival

- Yahboom servo connectors are standard 3-pin (RED +V, BLK GND, WHT signal), landing on Arduino pins 9 and 10.
- IRF520 module's onboard flyback diode present (diode-test OUT+ → V+, ~0.6V one direction).
- Buck trim pot set to **6.0V unloaded** before connecting servos.
- Sensor Shield V5 power-source jumper set to **external** before applying power — otherwise Arduino's 5V regulator backfeeds the servo rail.
- All grounds tied to a single star point (PSU GND, buck GND, MOSFET GND, Arduino GND, shield V−, Microtronic GND).
- All four P15/5 magnet coils measure roughly equal resistance (DMM ohms mode) — outliers mean a damaged coil or open lead.
- Test-fit each washer over a peg before gluing the dowels: smallest (M10) should drop freely with visible radial clearance.
