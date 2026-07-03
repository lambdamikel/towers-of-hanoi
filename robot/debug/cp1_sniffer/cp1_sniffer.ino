/*
  CP1 <-> Arduino PROTOCOL SNIFFER  (Arduino Mega 2560)
  --------------------------------------------------------------------------
  A bench instrument for the Hanoi robot 4-bit protocol. It does NOT drive the
  LED matrix or the robot -- it just watches the wires and lets you drive BUSY
  by hand, so you can see exactly what the CP1 is (or isn't) emitting and prove
  out the handshake.

  Wire it EXACTLY like the matrix/robot receiver (same pins) so you can swap it
  in without rewiring:

     CP1 Port 2 pin 1  D0      -> Mega D30   (input)
     CP1 Port 2 pin 2  D1      -> Mega D31   (input)
     CP1 Port 2 pin 3  D2      -> Mega D32   (input)
     CP1 Port 2 pin 4  STROBE  -> Mega D33   (input)
     CP1 Port 1 pin 1  BUSY    <- Mega D34   (output, we drive it)
     CP1 GND                   -- Mega GND   (MUST be common)

  Open Serial Monitor at 115200 baud, newline or no line-ending both fine.

  What it shows
  -------------
   * a 1 Hz heartbeat with the live level of all five lines, so you can SEE the
     idle state even when nothing is edging (a frozen CP1 shows a STROBE that
     never changes; a disconnected input shows up under the pull-up test);
   * a decoded line on every STROBE edge:  code + move (1->2 .. 3->2);
   * BUSY is under YOUR control so you can single-step or auto-run the CP1.

  Serial commands (type the letter, press Enter):
     h : drive BUSY HIGH and hold        (ack / "I am moving")
     l : drive BUSY LOW  and hold        (idle / "ready for next")   [default]
     a : AUTO-ACK -- on each STROBE edge, pulse BUSY high for HOLD ms then low.
         This emulates the matrix/robot so the CP1 runs a whole solve; watch
         the move stream scroll past.
     m : MANUAL -- BUSY stays where you last put it (h/l). [default mode]
     p : PULL-UP FLOATING TEST -- briefly enable input pull-ups on D0..STROBE
         and report. A line the CP1 actively drives keeps its real level; a
         DISCONNECTED / floating line reads HIGH. Restores plain INPUT after.
     s : print a one-shot status snapshot
     ? : this help
*/

const uint8_t PIN_D0     = 30;
const uint8_t PIN_D1     = 31;
const uint8_t PIN_D2     = 32;
const uint8_t PIN_STROBE = 33;
const uint8_t PIN_BUSY   = 34;

const unsigned long HOLD_MS = 300;   // auto-ack BUSY-high hold time

bool          autoAck   = false;     // 'a' vs 'm'
bool          busyLevel = false;     // current commanded BUSY (manual mode)
int           lastStrobe;
unsigned long edgeCount  = 0;
unsigned long busyDropAt = 0;        // millis() when auto-ack should drop BUSY
unsigned long lastBeat   = 0;

const char* moveName(uint8_t code) {
  switch (code) {
    case 1: return "1->2"; case 2: return "1->3"; case 3: return "2->1";
    case 4: return "2->3"; case 5: return "3->1"; case 6: return "3->2";
    case 7: return "RESET";
    default: return "----";
  }
}

uint8_t readCode() {
  return (digitalRead(PIN_D0))
       | (digitalRead(PIN_D1) << 1)
       | (digitalRead(PIN_D2) << 2);
}

void setBusy(bool level) {
  busyLevel = level;
  digitalWrite(PIN_BUSY, level ? HIGH : LOW);
}

void snapshot() {
  uint8_t code = readCode();
  Serial.print("  D0=");     Serial.print(digitalRead(PIN_D0));
  Serial.print(" D1=");      Serial.print(digitalRead(PIN_D1));
  Serial.print(" D2=");      Serial.print(digitalRead(PIN_D2));
  Serial.print(" | STROBE="); Serial.print(digitalRead(PIN_STROBE));
  Serial.print(" | BUSY(out)="); Serial.print(busyLevel);
  Serial.print(" || code="); Serial.print(code);
  Serial.print(" ");         Serial.print(moveName(code));
  Serial.print("  mode=");   Serial.print(autoAck ? "AUTO" : "MANUAL");
  Serial.print("  edges=");  Serial.println(edgeCount);
}

void help() {
  Serial.println(F("--- CP1 protocol sniffer ---"));
  Serial.println(F("  h=BUSY high  l=BUSY low  a=auto-ack  m=manual"));
  Serial.println(F("  p=pull-up floating test  s=status  ?=help"));
}

void pullupTest() {
  Serial.println(F("[pull-up test] enabling INPUT_PULLUP on D0..STROBE for 1s..."));
  pinMode(PIN_D0, INPUT_PULLUP); pinMode(PIN_D1, INPUT_PULLUP);
  pinMode(PIN_D2, INPUT_PULLUP); pinMode(PIN_STROBE, INPUT_PULLUP);
  delay(50);
  for (int i = 0; i < 5; i++) { snapshot(); delay(200); }
  Serial.println(F("  A line stuck HIGH here (esp. jittery) is likely FLOATING /"));
  Serial.println(F("  not driven by the CP1. A line the CP1 drives keeps its level."));
  pinMode(PIN_D0, INPUT); pinMode(PIN_D1, INPUT);
  pinMode(PIN_D2, INPUT); pinMode(PIN_STROBE, INPUT);
}

void setup() {
  pinMode(PIN_D0, INPUT);
  pinMode(PIN_D1, INPUT);
  pinMode(PIN_D2, INPUT);
  pinMode(PIN_STROBE, INPUT);
  pinMode(PIN_BUSY, OUTPUT);
  setBusy(false);                 // idle low, exactly like the real receiver

  Serial.begin(115200);
  while (!Serial) { }
  Serial.println(F("\nCP1 protocol sniffer ready. BUSY idling LOW (manual mode)."));
  help();
  lastStrobe = digitalRead(PIN_STROBE);
  snapshot();
}

void loop() {
  // --- serial command handling ---
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'h': autoAck = false; setBusy(true);  Serial.println(F("[BUSY -> HIGH]")); break;
      case 'l': autoAck = false; setBusy(false); Serial.println(F("[BUSY -> LOW]"));  break;
      case 'a': autoAck = true;  Serial.println(F("[AUTO-ACK on: pulse BUSY per strobe edge]")); break;
      case 'm': autoAck = false; Serial.println(F("[MANUAL mode]")); break;
      case 'p': pullupTest(); break;
      case 's': snapshot(); break;
      case '?': help(); break;
      default: break;               // ignore whitespace / newlines
    }
  }

  // --- strobe edge detect ---
  int s = digitalRead(PIN_STROBE);
  if (s != lastStrobe) {
    lastStrobe = s;
    edgeCount++;
    uint8_t code = readCode();      // data is settled before the strobe edge
    Serial.print(F("STROBE edge #")); Serial.print(edgeCount);
    Serial.print(F("  strobe=")); Serial.print(s);
    Serial.print(F("  D2D1D0=")); Serial.print(digitalRead(PIN_D2));
    Serial.print(digitalRead(PIN_D1)); Serial.print(digitalRead(PIN_D0));
    Serial.print(F("  code=")); Serial.print(code);
    Serial.print(F("  ")); Serial.print(moveName(code));
    if (autoAck) {
      setBusy(true);
      busyDropAt = millis() + HOLD_MS;
      Serial.println(F("   [auto-ack: BUSY HIGH]"));
    } else {
      Serial.println();
    }
  }

  // --- auto-ack BUSY drop ---
  if (autoAck && busyLevel && (long)(millis() - busyDropAt) >= 0) {
    setBusy(false);
  }

  // --- 1 Hz heartbeat ---
  if (millis() - lastBeat >= 1000) {
    lastBeat = millis();
    Serial.print(F("[hb] ")); snapshot();
  }
}
