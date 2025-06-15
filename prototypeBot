#include <LiquidCrystal.h>

// LCD configuration (pins A0–A5)
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

// Motor pins (L293D driver)
const int motor1Pin1 = 2;
const int motor1Pin2 = 3;
const int motor2Pin1 = 4;
const int motor2Pin2 = 5;
const int motor3Pin1 = 6;
const int motor3Pin2 = 7;
const int motor4Pin1 = 8;
const int motor4Pin2 = 9;

// Ultrasonic sensor (3-pin) signal pin
const int sigPin = 10;

// Minimum distance (cm) to stop the robot
const long distanceThreshold = 20;

// Variable to track the last state
String lastState = "";

void setup() {
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(2, 0);  // Centered title "UltrasonicBot"
  lcd.print("UltrasonicBot");

  // Configure motor pins as outputs
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(motor3Pin1, OUTPUT);
  pinMode(motor3Pin2, OUTPUT);
  pinMode(motor4Pin1, OUTPUT);
  pinMode(motor4Pin2, OUTPUT);

  // Configure ultrasonic signal pin
  pinMode(sigPin, OUTPUT);
}

// Function to measure distance using 3-pin ultrasonic sensor
long measureDistance() {
  // Trigger pulse
  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(sigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sigPin, LOW);

  // Listen for echo
  pinMode(sigPin, INPUT);
  long duration = pulseIn(sigPin, HIGH, 30000); // Timeout 30ms

  // Convert time to distance in cm
  long distance = duration * 0.034 / 2;
  return distance;
}

// Function to move the robot forward
void moveForward() {
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  digitalWrite(motor3Pin1, HIGH);
  digitalWrite(motor3Pin2, LOW);
  digitalWrite(motor4Pin1, LOW);
  digitalWrite(motor4Pin2, HIGH);
}

// Function to stop all motors
void stopMotors() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor3Pin1, LOW);
  digitalWrite(motor3Pin2, LOW);
  digitalWrite(motor4Pin1, LOW);
  digitalWrite(motor4Pin2, LOW);
}

// Function to update the LCD state line
void updateLCD(String state) {
  if (state != lastState) {
    lcd.setCursor(4, 1); // Default centered position
    lcd.print("        "); // Clear the line

    if (state == "Moving") {
      lcd.setCursor(5, 1); // Center "Moving"
      lcd.print("Moving");
    } else if (state == "Stopped") {
      lcd.setCursor(4, 1); // Center "Stopped"
      lcd.print("Stopped");
    }

    lastState = state;
  }
}

void loop() {
  long distance = measureDistance();

  if (distance > distanceThreshold) {
    moveForward();
    updateLCD("Moving");
  } else {
    stopMotors();
    updateLCD("Stopped");
  }

  delay(100);
}
