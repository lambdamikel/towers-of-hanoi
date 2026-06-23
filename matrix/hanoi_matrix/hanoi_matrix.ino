/*
  Towers of Hanoi  --  LED matrix display
  ------------------------------------------------------------------
  Host:        Kosmos CP1  (runs the recursion, sends each move)
  Controller:  Arduino Mega 2560 + Adafruit RGB Matrix Shield + 64x32 HUB75
  Library:     RGBmatrixPanel   (the AVR path -- Protomatter has no AVR support)

  The CP1 does the real work; this sketch only renders the moves it sends.

  ---- CP1 <-> Mega link  (5 V direct -- NO level shifters; CP1 ports are 5 V) ----
     CP1 Port 2 pin 1   D0      ->  Mega D30
     CP1 Port 2 pin 2   D1      ->  Mega D31
     CP1 Port 2 pin 3   D2      ->  Mega D32
     CP1 Port 2 pin 4   STROBE  ->  Mega D33     (toggles on each new move)
     CP1 Port 1 pin 1   BUSY    <-  Mega D34     (HIGH while this sketch is busy)
     CP1 GND                    --  Mega GND     (MUST be common)

  Move code (bits D2 D1 D0) -- matches the CP1 robot program:
     1 = 0->1   2 = 0->2   3 = 1->0   4 = 1->2   5 = 2->0   6 = 2->1
     7 = RESET (rebuild the starting tower)        0 = ignored

  ---- Matrix wiring  (Mega + RGB Matrix Shield) ----
  RGBmatrixPanel hardwires the 6 RGB data pins to PORTA = Mega D24..D29, and the
  clock must live on PORTB (pin 11). The shield is laid out for the Uno (RGB on
  D2..D7, CLK on D8), so on the Mega reroute:
        shield R1,G1,B1,R2,G2,B2 (D2..D7)  ->  Mega D24..D29   (PORTA)
        shield CLK (D8)                     ->  Mega D11        (PORTB)
  LAT (D10), OE (D9), A..D (A0..A3) already match -- no change.
  Panel power: a SEPARATE 5 V / >=2 A supply to the shield screw terminals,
  common ground. Do NOT power the panel from the Mega.
*/

#include <RGBmatrixPanel.h>

// ---- Matrix control pins (data pins are fixed inside the library) ----
#define CLK 11      // Mega: clock must be on PORTB
#define LAT 10
#define OE   9
#define A   A0
#define B   A1
#define C   A2
#define D   A3
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false, 64);  // 64x32, single buffer

// ---- CP1 link pins (free PORTC pins; 5 V direct) ----
const uint8_t PIN_D0     = 30;
const uint8_t PIN_D1     = 31;
const uint8_t PIN_D2     = 32;
const uint8_t PIN_STROBE = 33;
const uint8_t PIN_BUSY   = 34;

// ---- Puzzle size -- MUST match the CP1 program's disk count (CP1 addr 008) ----
const uint8_t NUM_DISKS = 5;        // 1..8 render well on 64x32

// ---- Geometry (64 wide x 32 tall) ----
const int16_t PEG_X[3] = { 11, 32, 53 };
const int16_t BASE_Y   = 29;        // bottom disk's bottom row
const int16_t TOP_Y    = 1;         // transit height (above all stacks)
const int16_t MIN_W = 5, MAX_W = 19;
int16_t DISK_H;                     // computed in setup() from NUM_DISKS

// ---- Tower model ----
int8_t  stackArr[3][16];            // disk sizes, [p][0] = bottom
uint8_t stackN[3];                  // disks on each peg

int lastStrobe;

uint16_t diskColor(uint8_t size) {  // rainbow by size
  long hue = (long)(size - 1) * 1400L / (NUM_DISKS > 1 ? NUM_DISKS - 1 : 1);
  return matrix.ColorHSV(hue, 255, 255, true);
}

int16_t diskWidth(uint8_t size) {
  if (NUM_DISKS <= 1) return MAX_W;
  return MIN_W + (int32_t)(size - 1) * (MAX_W - MIN_W) / (NUM_DISKS - 1);
}

void drawDiskAt(int16_t cx, int16_t bottomY, uint8_t size) {
  int16_t w = diskWidth(size);
  matrix.fillRect(cx - w / 2, bottomY - DISK_H + 1, w, DISK_H, diskColor(size));
}

void drawScene(int8_t floatSize, int16_t floatCx, int16_t floatBottomY) {
  matrix.fillScreen(0);
  uint16_t baseCol = matrix.Color333(0, 0, 7);              // bright blue base line
  uint16_t pegCol  = matrix.Color333(0, 1, 4);             // dimmer blue pegs
  matrix.drawFastHLine(0, BASE_Y + 1, 64, baseCol);         // base
  for (uint8_t p = 0; p < 3; p++)                           // pegs (perpendicular)
    matrix.drawFastVLine(PEG_X[p], TOP_Y, BASE_Y - TOP_Y + 1, pegCol);
  for (uint8_t p = 0; p < 3; p++)                            // settled disks
    for (uint8_t i = 0; i < stackN[p]; i++)
      drawDiskAt(PEG_X[p], BASE_Y - i * DISK_H, stackArr[p][i]);
  if (floatSize > 0) drawDiskAt(floatCx, floatBottomY, floatSize);  // moving disk
}

void buildTower() {
  stackN[0] = NUM_DISKS; stackN[1] = 0; stackN[2] = 0;
  for (uint8_t i = 0; i < NUM_DISKS; i++)
    stackArr[0][i] = NUM_DISKS - i;          // bottom = largest
  drawScene(0, 0, 0);
}

void animateMove(uint8_t from, uint8_t to) {
  if (stackN[from] == 0) return;
  uint8_t size = stackArr[from][stackN[from] - 1];
  stackN[from]--;
  int16_t srcBottom = BASE_Y - stackN[from] * DISK_H;
  int16_t dstBottom = BASE_Y - stackN[to]   * DISK_H;
  int16_t topBottom = TOP_Y + DISK_H;
  const int MS = 14;

  for (int16_t y = srcBottom; y > topBottom; y--)            // 1. lift
    { drawScene(size, PEG_X[from], y); delay(MS); }
  int16_t dir = (PEG_X[to] > PEG_X[from]) ? 1 : -1;
  for (int16_t x = PEG_X[from]; x != PEG_X[to]; x += dir)    // 2. traverse
    { drawScene(size, x, topBottom); delay(MS); }
  for (int16_t y = topBottom; y < dstBottom; y++)            // 3. drop
    { drawScene(size, PEG_X[to], y); delay(MS); }

  stackN[to]++;
  stackArr[to][stackN[to] - 1] = size;
  drawScene(0, 0, 0);
}

bool decodeMove(uint8_t code, uint8_t &from, uint8_t &to) {
  switch (code) {
    case 1: from = 0; to = 1; return true;
    case 2: from = 0; to = 2; return true;
    case 3: from = 1; to = 0; return true;
    case 4: from = 1; to = 2; return true;
    case 5: from = 2; to = 0; return true;
    case 6: from = 2; to = 1; return true;
    default: return false;
  }
}

void setup() {
  pinMode(PIN_D0, INPUT);
  pinMode(PIN_D1, INPUT);
  pinMode(PIN_D2, INPUT);
  pinMode(PIN_STROBE, INPUT);
  pinMode(PIN_BUSY, OUTPUT);
  digitalWrite(PIN_BUSY, LOW);

  DISK_H = (BASE_Y - TOP_Y - 1) / NUM_DISKS;
  if (DISK_H > 4) DISK_H = 4;
  if (DISK_H < 2) DISK_H = 2;

  matrix.begin();
  buildTower();
  lastStrobe = digitalRead(PIN_STROBE);
}

void loop() {
  int s = digitalRead(PIN_STROBE);
  if (s != lastStrobe) {                       // new command on each edge
    lastStrobe = s;
    uint8_t code = (digitalRead(PIN_D0)     )
                 | (digitalRead(PIN_D1) << 1)
                 | (digitalRead(PIN_D2) << 2);

    digitalWrite(PIN_BUSY, HIGH);              // ack -- CP1 waits for this
    if (code == 7) {
      buildTower();                            // explicit reset (CP1 may send code 7)
    } else {
      uint8_t from, to;
      if (decodeMove(code, from, to)) {
        if (stackN[2] == NUM_DISKS) {          // a move after a finished solve =
          buildTower();                        //   a new run -> auto-reset (no CP1
          delay(800);                          //   change needed); pause so it shows
        }
        animateMove(from, to);
      }
    }
    digitalWrite(PIN_BUSY, LOW);               // done -- CP1 may send next
  }
}
