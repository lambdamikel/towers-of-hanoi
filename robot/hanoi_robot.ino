// =============================================================
//  Hanoi Robot Controller — Arduino Uno/Nano
//  Host: Busch Microtronic 2090 (4 outputs + 1 input from us)
//  Hardware: Yahboom 2-DOF pan/tilt, P25/20 electromagnet via IRF520
// =============================================================

#include <Servo.h>

// ---------- Pin assignments ----------
const uint8_t PIN_STROBE = 2;   // D3 from Microtronic (INT0)
const uint8_t PIN_D0     = 3;   // bit 0 of move code
const uint8_t PIN_D1     = 4;   // bit 1
const uint8_t PIN_D2     = 8;   // bit 2
const uint8_t PIN_BUSY   = 7;   // to Microtronic, HIGH during move
const uint8_t PIN_MAGNET = 5;   // PWM to IRF520 SIG (980 Hz on Uno)
const uint8_t PIN_PAN    = 9;   // pan servo
const uint8_t PIN_TILT   = 10;  // tilt servo

// ---------- Calibration constants (TUNE THESE) ----------
// Peg angles for pan servo. 270° servo, but we use a small arc.
// Audience-facing layout per design discussion.
const int PAN_PEG[3]  = { 75, 90, 105 };
const int PAN_PARK    = 60;      // safe pose between moves

// Tilt servo angles. Higher number = arm more horizontal (up).
// Approximate; will need empirical lookup table per disk height.
const int TILT_UP     = 110;     // safely above all stacks for transit
const int TILT_BASE   = 70;      // magnet touches empty baseplate
const int TILT_PER_DISK = 3;     // degrees per disk of stack height

// Magnet PWM levels (0..255). MUST calibrate experimentally so
// MAG_PICKUP lifts exactly one disc, not two.
const uint8_t MAG_OFF     = 0;
const uint8_t MAG_PICKUP  = 80;
const uint8_t MAG_TRANSIT = 120;

// Motion smoothing
const int  SERVO_STEP_MS  = 15;  // ms per 1° increment
const int  SETTLE_GRIP_MS = 200; // dwell after magnet on, before lift
const int  SETTLE_DROP_MS = 300; // dwell after magnet off, before lift
const int  MAX_DISKS      = 5;   // physical capacity per peg

// ---------- Runtime state ----------
Servo panServo, tiltServo;

int  stackTop[3];                  // -1 if empty, else top index
int  stack[3][MAX_DISKS];          // disk sizes; -1 if empty slot

volatile bool    commandReady = false;
volatile uint8_t latchedCode  = 0;
volatile bool    lastStrobe   = LOW;

// ---------- ISR ----------
void onStrobeEdge() {
  bool s = digitalRead(PIN_STROBE);
  if (s == lastStrobe) return;     // glitch / no real edge
  lastStrobe = s;
  // Data bits are guaranteed stable: Microtronic wrote all 4 atomically
  latchedCode = (digitalRead(PIN_D0)     )
              | (digitalRead(PIN_D1) << 1)
              | (digitalRead(PIN_D2) << 2);
  commandReady = true;
}

// ---------- Helpers ----------
bool decodeMove(uint8_t code, int &from, int &to) {
  switch (code) {
    case 0b001: from = 0; to = 1; return true;
    case 0b010: from = 0; to = 2; return true;
    case 0b011: from = 1; to = 0; return true;
    case 0b100: from = 1; to = 2; return true;
    case 0b101: from = 2; to = 0; return true;
    case 0b110: from = 2; to = 1; return true;
    default:    return false;
  }
}

int tiltForStackLevel(int disksBelow) {
  // disksBelow = number of disks the magnet must clear from above
  // (0 = touching baseplate; 1 = on top of disk 1; etc.)
  return TILT_BASE + disksBelow * TILT_PER_DISK;
}

void slewServo(Servo &s, int target) {
  int cur = s.read();
  int step = (target > cur) ? 1 : -1;
  while (cur != target) {
    cur += step;
    s.write(cur);
    delay(SERVO_STEP_MS);
  }
}

// ---------- Main motion ----------
void executeMove(int from, int to) {
  int srcLevel = stackTop[from];          // disks at index srcLevel
  int dstLevel = stackTop[to] + 1;        // we'll land at this index

  // 1. Rotate to source, arm up
  slewServo(panServo,  PAN_PEG[from]);
  slewServo(tiltServo, TILT_UP);

  // 2. Lower onto source top disc; energize magnet at PICKUP level
  //    (intentionally start at low PWM — never pulse full power first)
  analogWrite(PIN_MAGNET, MAG_PICKUP);
  slewServo(tiltServo, tiltForStackLevel(srcLevel));
  delay(SETTLE_GRIP_MS);

  // 3. Lift, bump to TRANSIT level for the swing
  slewServo(tiltServo, TILT_UP);
  analogWrite(PIN_MAGNET, MAG_TRANSIT);

  // 4. Rotate to destination
  slewServo(panServo, PAN_PEG[to]);

  // 5. Lower to landing height (one disc above current dest top)
  slewServo(tiltServo, tiltForStackLevel(dstLevel));
  analogWrite(PIN_MAGNET, MAG_OFF);
  delay(SETTLE_DROP_MS);

  // 6. Tiny upward jog to break any residual magnetism
  slewServo(tiltServo, tiltForStackLevel(dstLevel) + 2);
  slewServo(tiltServo, TILT_UP);

  // 7. Update bookkeeping
  int disc = stack[from][srcLevel];
  stack[from][srcLevel] = -1;
  stackTop[from] = srcLevel - 1;
  stack[to][dstLevel] = disc;
  stackTop[to] = dstLevel;

  // 8. Park (optional — reduces servo heating during idle)
  slewServo(panServo, PAN_PARK);
}

// ---------- Setup / loop ----------
void setup() {
  pinMode(PIN_STROBE, INPUT);
  pinMode(PIN_D0, INPUT);
  pinMode(PIN_D1, INPUT);
  pinMode(PIN_D2, INPUT);
  pinMode(PIN_BUSY, OUTPUT);
  pinMode(PIN_MAGNET, OUTPUT);

  digitalWrite(PIN_BUSY, LOW);
  analogWrite(PIN_MAGNET, MAG_OFF);

  panServo.attach(PIN_PAN);
  tiltServo.attach(PIN_TILT);
  panServo.write(PAN_PARK);
  tiltServo.write(TILT_UP);

  // Initialize tower: 3 disks on peg 0 (largest=2 at bottom, smallest=0 on top)
  for (int p = 0; p < 3; p++) {
    for (int i = 0; i < MAX_DISKS; i++) stack[p][i] = -1;
    stackTop[p] = -1;
  }
  stack[0][0] = 2; stack[0][1] = 1; stack[0][2] = 0;
  stackTop[0] = 2;

  lastStrobe = digitalRead(PIN_STROBE);
  attachInterrupt(digitalPinToInterrupt(PIN_STROBE), onStrobeEdge, CHANGE);
}

void loop() {
  if (!commandReady) return;
  noInterrupts();
  uint8_t code = latchedCode;
  commandReady = false;
  interrupts();

  int from, to;
  if (!decodeMove(code, from, to)) return;          // ignore invalid
  if (stackTop[from] < 0) return;                   // empty source: ignore
  if (stackTop[to] >= MAX_DISKS - 1) return;        // overflow: ignore
  // Optional: enforce Hanoi rule (small-on-large) — uncomment to be strict
  // if (stackTop[to] >= 0 &&
  //     stack[to][stackTop[to]] < stack[from][stackTop[from]]) return;

  digitalWrite(PIN_BUSY, HIGH);
  executeMove(from, to);
  digitalWrite(PIN_BUSY, LOW);
}
