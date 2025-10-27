#define P1 18   // Start/Stop button
#define P2 19   // Home limit switch
#define P3 21   // End limit switch

#define ENABLE 12
#define STEP 26
#define DIR 25

// Motion parameters
int stepDelay = 800;           // Base step delay (speed control)
int accelStepDelay = 2000;     // Initial delay (for acceleration)
int accelRate = 5;             // Step delay decrement rate per cycle

// State tracking
bool running = false;
bool homed = false;
bool movingClockwise = true;

// ---- Function Declarations ----
void moveOneStep(bool clockwise, int customDelay = -1);
void homeSequence();
void accelerateMotor();

void setup() {
  Serial.begin(115200);
  pinMode(P1, INPUT_PULLUP);
  pinMode(P2, INPUT_PULLUP);
  pinMode(P3, INPUT_PULLUP);

  pinMode(ENABLE, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);

  digitalWrite(ENABLE, LOW);   // Enable driver
  digitalWrite(STEP, LOW);
  digitalWrite(DIR, HIGH);

  Serial.println("Stepper driver initialized.");
  Serial.println("Press P1 to start homing...");
}

void loop() {
  bool p1Pressed = !digitalRead(P1);
  static bool lastP1 = false;

  // Detect P1 edge
  if (p1Pressed && !lastP1) {
    if (!running) {
      if (!homed) {
        homeSequence();
      }
      running = true;
      Serial.println("Motor running...");
    } else {
      running = false;
      Serial.println("Motor stopped.");
    }
    delay(200); // debounce
  }
  lastP1 = p1Pressed;

  if (!running) return;

  // Motion control with end-stop handling
  if (movingClockwise) {
    if (!digitalRead(P3)) { // end limit pressed
      Serial.println("End limit hit. Reversing...");
      movingClockwise = false;
      delay(300);
    } else {
      moveOneStep(true);
    }
  } else {
    if (!digitalRead(P2)) { // home limit pressed
      Serial.println("Home limit hit. Reversing...");
      movingClockwise = true;
      delay(300);
    } else {
      moveOneStep(false);
    }
  }
}

// ---------------------------------------------
// HOMING FUNCTION
// ---------------------------------------------
void homeSequence() {
  Serial.println("Starting homing sequence...");

  digitalWrite(DIR, LOW); // Move toward home (P2)
  while (digitalRead(P2)) {  // Keep moving until home switch is hit
    moveOneStep(false);
  }

  Serial.println("Home switch detected.");
  delay(500);

  // Back off slightly
  digitalWrite(DIR, HIGH);
  for (int i = 0; i < 200; i++) {
    moveOneStep(true);
  }

  // Move slowly again toward home for precision
  digitalWrite(DIR, LOW);
  while (digitalRead(P2)) {
    moveOneStep(false, 1500);
  }

  homed = true;
  Serial.println("Homing complete. Position set to zero.");
  delay(500);
}

// ---------------------------------------------
// Move one step with optional speed parameter
// ---------------------------------------------
void moveOneStep(bool clockwise, int customDelay) {
  int delayVal = (customDelay > 0) ? customDelay : stepDelay;
  digitalWrite(DIR, clockwise ? HIGH : LOW);
  digitalWrite(STEP, HIGH);
  delayMicroseconds(delayVal);
  digitalWrite(STEP, LOW);
  delayMicroseconds(delayVal);
}

// ---------------------------------------------
// Simple acceleration/deceleration (soft start)
// ---------------------------------------------
void accelerateMotor() {
  for (int d = accelStepDelay; d > stepDelay; d -= accelRate) {
    moveOneStep(movingClockwise, d);
  }
}
