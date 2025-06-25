#include <LiquidCrystal.h>
#include <IRremote.h>

// ==== Pin Configuration ====
const int motorPins[4][2] = { {2, 3}, {4, 5}, {6, 7}, {8, 9} };
const int trigFront = 10;
const int trigRear  = 11;
const int irPin     = 12;

LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);
IRrecv irrecv(irPin);

// ==== Parameters ====
const long DIST_THRESH = 20;
const unsigned long TURN_DELAY = 600;
const int DEFAULT_SPEED = 150;

long frontDist = 0;
long rearDist  = 0;
bool manualFwd = false, manualBack = false, manualLeft = false, manualRight = false;

// ==== Helper Functions ====
void stopMotors() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(motorPins[i][0], LOW);
    digitalWrite(motorPins[i][1], LOW);
  }
}

void moveForward() {
  stopMotors();
  for (int i = 0; i < 4; i += 2) {
    analogWrite(motorPins[i][0], DEFAULT_SPEED);
  }
}

void moveBackward() {
  stopMotors();
  for (int i = 0; i < 4; i += 2) {
    analogWrite(motorPins[i][1], DEFAULT_SPEED);
  }
}

void turnRight() {
  stopMotors();
  analogWrite(motorPins[0][0], DEFAULT_SPEED);
  analogWrite(motorPins[2][0], DEFAULT_SPEED);
  analogWrite(motorPins[1][1], DEFAULT_SPEED);
  analogWrite(motorPins[3][1], DEFAULT_SPEED);
}

void turnLeft() {
  stopMotors();
  analogWrite(motorPins[1][0], DEFAULT_SPEED);
  analogWrite(motorPins[3][0], DEFAULT_SPEED);
  analogWrite(motorPins[0][1], DEFAULT_SPEED);
  analogWrite(motorPins[2][1], DEFAULT_SPEED);
}

long measureDistance(int trigPin) {
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  pinMode(trigPin, INPUT);
  long d = pulseIn(trigPin, HIGH, 20000);
  return d ? (d * 0.0343 / 2) : 400;
}

void readSensors() {
  frontDist = measureDistance(trigFront);
  rearDist  = measureDistance(trigRear);
}

void updateLCD(bool autoMode) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(autoMode ? "AUTO" : "MANU");
  lcd.setCursor(0, 1);
  lcd.print("F:"); lcd.print(frontDist);
  lcd.print(" R:"); lcd.print(rearDist);
}

// ==== Operation ====
bool isObstacleFront() { return frontDist < DIST_THRESH; }

void autoMode() {
  readSensors();
  if (isObstacleFront()) {
    // simple avoid: back + turn
    moveBackward();
    delay(TURN_DELAY);
    turnRight();
    delay(TURN_DELAY);
  } else {
    moveForward();
  }
  updateLCD(true);
}

void manualMode() {
  readSensors();
  if (manualFwd) moveForward();
  else if (manualBack) moveBackward();
  else if (manualLeft) turnLeft();
  else if (manualRight) turnRight();
  else stopMotors();
  updateLCD(false);
}

void processIR() {
  if (!irrecv.decode()) return;
  unsigned long code = irrecv.decodedIRData.decodedRawData;
  irrecv.resume();

  switch (code) {
    case 0xEF10BF00: manualFwd = !manualFwd; break; // toggle forward
    case 0xEE11BF00: manualFwd = true; manualBack = manualLeft = manualRight = false; break;
    case 0xE619BF00: manualBack = true; manualFwd = manualLeft = manualRight = false; break;
    case 0xEB14BF00: manualLeft = true; manualFwd = manualBack = manualRight = false; break;
    case 0xE916BF00: manualRight = true; manualFwd = manualBack = manualLeft = false; break;
    case 0xEA15BF00: manualFwd = manualBack = manualLeft = manualRight = false; break; // stop
  }
}

// ==== Setup & Loop ====
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  irrecv.enableIRIn();
  for (int i = 0; i < 4; i++) {
    pinMode(motorPins[i][0], OUTPUT);
    pinMode(motorPins[i][1], OUTPUT);
    stopMotors();
  }
}

void loop() {
  processIR();
  // if any manual command active -> manualMode, else autoMode
  if (manualFwd || manualBack || manualLeft || manualRight) manualMode();
  else autoMode();
  delay(50);
}
