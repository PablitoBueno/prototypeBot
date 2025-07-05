#include <LiquidCrystal.h>
#include <IRremote.h>

// LCD Configuration
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

// Motor pins (conexões atualizadas para controle digital)
const int motor1Pin1 = 2;  // Front left wheel - IN1
const int motor1Pin2 = 3;  // IN2
const int motor2Pin1 = 4;  // Front right wheel - IN3
const int motor2Pin2 = 5;  // IN4
const int motor3Pin1 = 6;  // Rear left wheel - IN1
const int motor3Pin2 = 7;  // IN2
const int motor4Pin1 = 8;  // Rear right wheel - IN3
const int motor4Pin2 = 9;  // IN4

// Ultrasonic sensors
const int sigPinFront = 10; // Front sensor
const int sigPinRear = 11;  // Rear sensor

// IR receiver
const int irReceiverPin = 12;
IRrecv irrecv(irReceiverPin);

// Parameters
const long distanceThreshold = 20;    // Minimum detection distance (cm)
const unsigned long avoidTurnTime = 600; // Time for 90° turn (ms)
const unsigned long maxBackTime = 2000;  // Maximum backing time (ms)
const int MAZE_MEMORY = 5;           // Memory for recent decisions
const unsigned long ESCAPE_TIME = 3000; // Maximum escape time (ms)

// States and modes
enum RobotState { MOVING, BACKING, TURNING_RIGHT, TURNING_LEFT, STOPPED };
enum OperationMode { AUTO, MANUAL };
enum NavAction { FORWARD, BACK, TURN_RIGHT, TURN_LEFT };

// Global variables
RobotState currentState = MOVING;
OperationMode currentMode = AUTO;
NavAction actionHistory[MAZE_MEMORY];
int actionIndex = 0;
unsigned long lastProgressTime = 0;
bool alternateTurn = false;
bool frontObstacle = false;
bool rearObstacle = false;
bool manualForward = false;
bool manualBackward = false;
bool manualLeft = false;
bool manualRight = false;
unsigned long backStartTime = 0;
unsigned long lastSensorCheck = 0;

// Sensor readings
long frontDistance = 0;
long rearDistance = 0;

// Updated IR codes
#define IR_BUTTON_1 0xEF10BF00  // switch auto/manual
#define IR_BUTTON_2 0xEE11BF00  // Forward
#define IR_BUTTON_4 0xEB14BF00  // Left
#define IR_BUTTON_5 0xEA15BF00  // OK/Stop
#define IR_BUTTON_6 0xE916BF00  // Right
#define IR_BUTTON_8 0xE619BF00  // Reverse

// ============== SENSOR FUNCTIONS ============== //

long measureDistance(int trigPin) {
  // Reset sensor
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);
  
  // Send pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);  // Standard pulse width
  digitalWrite(trigPin, LOW);

  // Measure response
  pinMode(trigPin, INPUT);
  long duration = pulseIn(trigPin, HIGH, 20000);  // 20ms timeout (max 3.4m)
  
  // Handle timeout
  if (duration == 0) {
    return 400; // Return max distance
  }
  float distance_cm = duration * 0.0343 * 1.255 / 2.0;
  return (long)(distance_cm + 0.5); // Round to nearest integer
}

// Get stable distance reading with filtering
const int SAMPLE_COUNT = 3;
long getStableDistance(int pin) {
  long sum = 0;
  
  // Take multiple readings
  for(int i = 0; i < SAMPLE_COUNT; i++) {
    sum += measureDistance(pin);
    delay(10);  // Short delay between readings
  }
  
  // Return average value
  return sum / SAMPLE_COUNT;
}

// ============== MOTOR CONTROL ============== //

void moveForward() {
  // Front left wheel: forward
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  
  // Front right wheel: forward (sentido inverso)
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  
  // Rear left wheel: forward
  digitalWrite(motor3Pin1, HIGH);
  digitalWrite(motor3Pin2, LOW);
  
  // Rear right wheel: forward (sentido inverso)
  digitalWrite(motor4Pin1, LOW);
  digitalWrite(motor4Pin2, HIGH);
}

void moveBackward() {
  // Front left wheel: backward
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  
  // Front right wheel: backward (sentido inverso)
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  
  // Rear left wheel: backward
  digitalWrite(motor3Pin1, LOW);
  digitalWrite(motor3Pin2, HIGH);
  
  // Rear right wheel: backward (sentido inverso)
  digitalWrite(motor4Pin1, HIGH);
  digitalWrite(motor4Pin2, LOW);
}

void turnRight() {
  // Left side: forward
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor3Pin1, HIGH);
  digitalWrite(motor3Pin2, LOW);
  
  // Right side: backward
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor4Pin1, HIGH);
  digitalWrite(motor4Pin2, LOW);
}

void turnLeft() {
  // Left side: backward
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor3Pin1, LOW);
  digitalWrite(motor3Pin2, HIGH);
  
  // Right side: forward
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  digitalWrite(motor4Pin1, LOW);
  digitalWrite(motor4Pin2, HIGH);
}

void stopMotors() {
  for (int i = 2; i <= 9; i++) {
    digitalWrite(i, LOW);
  }
}

// ============== LCD & UI FUNCTIONS ============== //

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  
  // First line: display mode and state
  lcd.setCursor(0, 0);
  if (currentMode == AUTO) {
    lcd.print("Auto ");
    switch (currentState) {
      case MOVING: lcd.print("Mov"); break;
      case BACKING: lcd.print("Bck"); break;
      case TURNING_RIGHT: lcd.print("TrR"); break;
      case TURNING_LEFT: lcd.print("TrL"); break;
      case STOPPED: lcd.print("Stp"); break;
    }
  } else {
    lcd.print("Manual");
  }
  
  // Second line: sensor information
  lcd.setCursor(0, 1);
  lcd.print("F:");
  lcd.print(frontDistance);
  lcd.print("cm R:");
  lcd.print(rearDistance);
  lcd.print("cm");
  
  // Obstacle indicators
  if (frontObstacle) {
    lcd.setCursor(14, 0);
    lcd.print("F!");
  }
  if (rearObstacle) {
    lcd.setCursor(14, 1);
    lcd.print("R!");
  }
}

// ============== SENSOR MANAGEMENT ============== //

void checkSensors() {
  static unsigned long lastFrontCheck = 0;
  static unsigned long lastRearCheck = 0;
  
  // Stagger sensor checks to prevent interference
  if (millis() - lastFrontCheck > 150) {
    frontDistance = getStableDistance(sigPinFront);
    frontObstacle = (frontDistance <= distanceThreshold);
    lastFrontCheck = millis();
  }
  
  if (millis() - lastRearCheck > 150) {
    rearDistance = getStableDistance(sigPinRear);
    rearObstacle = (rearDistance <= distanceThreshold);
    lastRearCheck = millis();
  }
}

// ============== NAVIGATION FUNCTIONS ============== //

bool checkSide(bool isLeft) {
  if (isLeft) {
    turnLeft();
    delay(300);
  } else {
    turnRight();
    delay(300);
  }
  
  bool obstacle = frontDistance <= distanceThreshold;
  
  if (isLeft) {
    turnRight();
  } else {
    turnLeft();
  }
  delay(300);
  stopMotors();
  
  return obstacle;
}

// ============== OPERATION MODES ============== //

void autoMode() {
  checkSensors();
  
  // Detect if stuck
  if (millis() - lastProgressTime > ESCAPE_TIME) {
    // Follow memorized path backwards
    int steps = min(actionIndex, 2);
    for (int i = 0; i < steps; i++) {
      NavAction lastAction = actionHistory[(actionIndex - i - 1) % MAZE_MEMORY];
      
      if (lastAction == TURN_RIGHT) {
        turnLeft();
        delay(avoidTurnTime);
      } else if (lastAction == TURN_LEFT) {
        turnRight();
        delay(avoidTurnTime);
      }
    }
    
    // Reset memory
    actionIndex = 0;
    lastProgressTime = millis();
    currentState = MOVING;
    return;
  }

  // Main state machine
  switch (currentState) {
    case MOVING:
      if (frontObstacle) {
        currentState = BACKING;
        backStartTime = millis();
      } else {
        moveForward();
        lastProgressTime = millis();
      }
      break;
      
    case BACKING:
      // Safety: stop if rear obstacle detected
      if (rearObstacle) {
        stopMotors();
        currentState = STOPPED;
        break;
      }
      
      if (frontObstacle && (millis() - backStartTime < maxBackTime)) {
        moveBackward();
      } else {
        stopMotors();
        
        // Check sides alternately
        bool tryRight = alternateTurn ? checkSide(false) : checkSide(true);
        bool tryLeft = alternateTurn ? checkSide(true) : checkSide(false);
        alternateTurn = !alternateTurn;
        
        if (!tryRight) {
          currentState = TURNING_RIGHT;
          actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_RIGHT;
        } else if (!tryLeft) {
          currentState = TURNING_LEFT;
          actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_LEFT;
        } else {
          // Both sides blocked, try opposite to last turn
          if (actionIndex > 0 && actionHistory[(actionIndex-1) % MAZE_MEMORY] == TURN_RIGHT) {
            currentState = TURNING_LEFT;
            actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_LEFT;
          } else {
            currentState = TURNING_RIGHT;
            actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_RIGHT;
          }
        }
      }
      break;
      
    case TURNING_RIGHT:
      turnRight();
      delay(avoidTurnTime);
      stopMotors();
      
      checkSensors();
      if (frontObstacle) {
        currentState = BACKING;
        backStartTime = millis();
      } else {
        currentState = MOVING;
        lastProgressTime = millis();
      }
      break;
      
    case TURNING_LEFT:
      turnLeft();
      delay(avoidTurnTime);
      stopMotors();
      
      checkSensors();
      if (frontObstacle) {
        currentState = BACKING;
        backStartTime = millis();
      } else {
        currentState = MOVING;
        lastProgressTime = millis();
      }
      break;
      
    case STOPPED:
      // Try to find exit
      checkSensors();
      if (!frontObstacle) {
        currentState = MOVING;
      } else if (!rearObstacle) {
        currentState = BACKING;
        backStartTime = millis();
      } else {
        // Random turn to escape
        randomSeed(analogRead(0));
        if (random(2) == 0) {
          turnLeft();
          delay(avoidTurnTime);
        } else {
          turnRight();
          delay(avoidTurnTime);
        }
        currentState = MOVING;
      }
      break;
  }
  
  updateLCD();
}

void manualMode() {
  // Update sensors
  if (millis() - lastSensorCheck > 200) {
    checkSensors();
    lastSensorCheck = millis();
  }

  // Execute movements
  if (manualForward) {
    moveForward();
  } else if (manualBackward) {
    moveBackward();
  } else if (manualLeft) {
    turnLeft();
  } else if (manualRight) {
    turnRight();
  } else {
    stopMotors();
  }
  
  updateLCD();
}

// ============== IR CONTROL ============== //

void processIR() {
  if (irrecv.decode()) {
    unsigned long irValue = irrecv.decodedIRData.decodedRawData;
    
    // Button 1: Toggle AUTO/MANUAL
    if (irValue == IR_BUTTON_1) {
      currentMode = (currentMode == AUTO) ? MANUAL : AUTO;
      // Reset controls
      manualForward = false;
      manualBackward = false;
      manualLeft = false;
      manualRight = false;
      stopMotors();
    }
    
    // Manual mode specific commands
    if (currentMode == MANUAL) {
      switch (irValue) {
        case IR_BUTTON_2: // Forward
          manualForward = true;
          manualBackward = false;
          manualLeft = false;
          manualRight = false;
          break;
          
        case IR_BUTTON_8: // Reverse
          manualForward = false;
          manualBackward = true;
          manualLeft = false;
          manualRight = false;
          break;
          
        case IR_BUTTON_4: // Left
          manualForward = false;
          manualBackward = false;
          manualLeft = true;
          manualRight = false;
          break;
          
        case IR_BUTTON_6: // Right
          manualForward = false;
          manualBackward = false;
          manualLeft = false;
          manualRight = true;
          break;
          
        case IR_BUTTON_5: // Emergency stop
          manualForward = false;
          manualBackward = false;
          manualLeft = false;
          manualRight = false;
          stopMotors();
          break;
      }
    }
    
    irrecv.resume();
  }
}

// ============== MAIN PROGRAM ============== //

void setup() {
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("UltrasonicBot");
  updateLCD();

  // Configure motor pins
  for (int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // Configure ultrasonic sensors
  pinMode(sigPinFront, OUTPUT);
  pinMode(sigPinRear, OUTPUT);
  
  // Start IR receiver
  irrecv.enableIRIn();
  irrecv.blink13(false);  // Disable LED blinking when receiving signal
  
  // Initialize navigation memory
  for (int i = 0; i < MAZE_MEMORY; i++) {
    actionHistory[i] = FORWARD;
  }
}

void loop() {
  processIR();
  
  if (currentMode == AUTO) {
    autoMode();
  } else {
    manualMode();
  }
  
  // Small delay for stability
  delay(25);
}
