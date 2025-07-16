#include <LiquidCrystal.h>
#include <IRremote.h>

// LCD CONFIGURATION
// Initialize LCD using analog pins A0-A5 as digital control pins
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

// MOTOR CONTROL PINS
// Motor driver pin assignments (note right-side motors use inverse logic)
const int motor1Pin1 = 2;  // Front left wheel - IN1
const int motor1Pin2 = 3;  // IN2
const int motor2Pin1 = 4;  // Front right wheel - IN3 (inverse direction)
const int motor2Pin2 = 5;  // IN4
const int motor3Pin1 = 6;  // Rear left wheel - IN1
const int motor3Pin2 = 7;  // IN2
const int motor4Pin1 = 8;  // Rear right wheel - IN3 (inverse direction)
const int motor4Pin2 = 9;  // IN4

// ULTRASONIC SENSORS
const int sigPinFront = 10; // Front sensor (trigger/echo)
const int sigPinRear = 11;  // Rear sensor (trigger/echo)

// IR RECEIVER SETUP
const int irReceiverPin = 12;
IRrecv irrecv(irReceiverPin);  // IR receiver object

// NAVIGATION PARAMETERS
const long distanceThreshold = 20;    // Minimum obstacle detection distance (cm)
const unsigned long avoidTurnTime = 600; // Time for 90° turn (ms)
const unsigned long maxBackTime = 2000;  // Maximum backing duration (ms)
const int MAZE_MEMORY = 5;           // Size of action history buffer
const unsigned long ESCAPE_TIME = 3000; // Max time without progress before escape (ms)

// STATE ENUMERATIONS
enum RobotState { MOVING, BACKING, TURNING_RIGHT, TURNING_LEFT, STOPPED };
enum OperationMode { AUTO, MANUAL };
enum NavAction { FORWARD, BACK, TURN_RIGHT, TURN_LEFT };

// GLOBAL VARIABLES
RobotState currentState = MOVING;     // Current navigation state
OperationMode currentMode = AUTO;     // Default operation mode
NavAction actionHistory[MAZE_MEMORY]; // Circular buffer for path memory
int actionIndex = 0;                  // Pointer for action history
unsigned long lastProgressTime = 0;   // Timestamp of last forward movement
bool alternateTurn = false;            // Alternator for turn direction checking
bool frontObstacle = false;           // Front obstacle detection flag
bool rearObstacle = false;            // Rear obstacle detection flag
// Manual control flags
bool manualForward = false;
bool manualBackward = false;
bool manualLeft = false;
bool manualRight = false;
unsigned long backStartTime = 0;      // Timestamp when backing started
unsigned long lastSensorCheck = 0;    // Last sensor polling time

// SENSOR READINGS
long frontDistance = 0;  // Measured front distance (cm)
long rearDistance = 0;   // Measured rear distance (cm)

// IR REMOTE CODES (customized for specific remote)
#define IR_BUTTON_1 0xEF10BF00  // Toggle auto/manual mode
#define IR_BUTTON_2 0xEE11BF00  // Manual forward
#define IR_BUTTON_4 0xEB14BF00  // Manual left turn
#define IR_BUTTON_5 0xEA15BF00  // Stop/emergency brake
#define IR_BUTTON_6 0xE916BF00  // Manual right turn
#define IR_BUTTON_8 0xE619BF00  // Manual reverse

// ============== SENSOR FUNCTIONS ============== //

/**
 * Measures distance from single-pin ultrasonic sensor
 * @param trigPin The sensor pin (used for both trigger and echo)
 * @returns Distance in centimeters (400cm max)
 */
long measureDistance(int trigPin) {
  // Reset sensor state
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);
  
  // Generate 10μs trigger pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure echo pulse duration with 20ms timeout
  pinMode(trigPin, INPUT);
  long duration = pulseIn(trigPin, HIGH, 20000);
  
  // Handle timeout (no echo)
  if (duration == 0) return 400; 
  
  // Calculate distance: (duration * speed_of_sound) / 2
  // 0.0343 cm/μs * duration / 2 * calibration factor 1.255
  float distance_cm = duration * 0.0343 * 1.255 / 2.0;
  return (long)(distance_cm + 0.5); // Round to nearest integer
}

/**
 * Gets stable distance reading through averaging
 * @param pin Sensor pin to read
 * @returns Averaged distance measurement
 */
const int SAMPLE_COUNT = 3;
long getStableDistance(int pin) {
  long sum = 0;
  for(int i = 0; i < SAMPLE_COUNT; i++) {
    sum += measureDistance(pin);
    delay(10);  // Delay between samples
  }
  return sum / SAMPLE_COUNT; // Return integer average
}

// ============== MOTOR CONTROL FUNCTIONS ============== //

/** Drives all wheels forward (handles inverse right-side logic) */
void moveForward() {
  // Front left wheel forward
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  // Front right wheel forward (inverse logic)
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  // Rear left wheel forward
  digitalWrite(motor3Pin1, HIGH);
  digitalWrite(motor3Pin2, LOW);
  // Rear right wheel forward (inverse logic)
  digitalWrite(motor4Pin1, LOW);
  digitalWrite(motor4Pin2, HIGH);
}

/** Drives all wheels backward (handles inverse right-side logic) */
void moveBackward() {
  // Front left wheel backward
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  // Front right wheel backward (inverse logic)
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  // Rear left wheel backward
  digitalWrite(motor3Pin1, LOW);
  digitalWrite(motor3Pin2, HIGH);
  // Rear right wheel backward (inverse logic)
  digitalWrite(motor4Pin1, HIGH);
  digitalWrite(motor4Pin2, LOW);
}

/** Pivot turn right (left wheels forward, right wheels backward) */
void turnRight() {
  // Left side forward
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor3Pin1, HIGH);
  digitalWrite(motor3Pin2, LOW);
  // Right side backward
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor4Pin1, HIGH);
  digitalWrite(motor4Pin2, LOW);
}

/** Pivot turn left (right wheels forward, left wheels backward) */
void turnLeft() {
  // Left side backward
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor3Pin1, LOW);
  digitalWrite(motor3Pin2, HIGH);
  // Right side forward
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  digitalWrite(motor4Pin1, LOW);
  digitalWrite(motor4Pin2, HIGH);
}

/** Cuts power to all motor driver pins */
void stopMotors() {
  for (int i = 2; i <= 9; i++) {
    digitalWrite(i, LOW);
  }
}

// ============== LCD INTERFACE FUNCTIONS ============== //

/** Updates LCD display with status and sensor data */
void updateLCD() {
  // Clear display
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  
  // Line 1: Display mode and state
  lcd.setCursor(0, 0);
  if (currentMode == AUTO) {
    lcd.print("Auto ");
    // State abbreviations
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
  
  // Line 2: Display sensor readings
  lcd.setCursor(0, 1);
  lcd.print("F:");
  lcd.print(frontDistance);
  lcd.print("cm R:");
  lcd.print(rearDistance);
  lcd.print("cm");
  
  // Obstacle warning indicators
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

/** Polls ultrasonic sensors with staggered timing */
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

/**
 * Checks for obstacles in specified direction
 * @param isLeft Set true to check left side, false for right
 * @returns True if obstacle detected
 */
bool checkSide(bool isLeft) {
  if (isLeft) {
    turnLeft();
    delay(300);
  } else {
    turnRight();
    delay(300);
  }
  
  // Capture obstacle status
  bool obstacle = frontDistance <= distanceThreshold;
  
  // Return to original orientation
  if (isLeft) turnRight();
  else turnLeft();
  delay(300);
  stopMotors();
  
  return obstacle;
}

// ============== OPERATION MODES ============== //

/** Autonomous navigation state machine */
void autoMode() {
  checkSensors(); // Update sensor readings
  
  // Escape handler - triggers when no progress for ESCAPE_TIME
  if (millis() - lastProgressTime > ESCAPE_TIME) {
    // Reverse last 2 actions from history
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
    // Reset navigation memory
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
      if (rearObstacle) {  // Safety stop
        stopMotors();
        currentState = STOPPED;
        break;
      }
      
      if (frontObstacle && (millis() - backStartTime < maxBackTime)) {
        moveBackward();
      } else {
        stopMotors();
        // Check both sides (alternating start direction)
        bool tryRight = alternateTurn ? checkSide(false) : checkSide(true);
        bool tryLeft = alternateTurn ? checkSide(true) : checkSide(false);
        alternateTurn = !alternateTurn;
        
        // Decide turn direction based on availability
        if (!tryRight) {
          currentState = TURNING_RIGHT;
          actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_RIGHT;
        } else if (!tryLeft) {
          currentState = TURNING_LEFT;
          actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_LEFT;
        } else {
          // Both blocked - use history to decide
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
      delay(avoidTurnTime); // Fixed-duration 90° turn
      stopMotors();
      
      checkSensors();
      // Re-evaluate path after turn
      if (frontObstacle) currentState = BACKING;
      else {
        currentState = MOVING;
        lastProgressTime = millis();
      }
      break;
      
    case TURNING_LEFT:
      turnLeft();
      delay(avoidTurnTime); // Fixed-duration 90° turn
      stopMotors();
      
      checkSensors();
      // Re-evaluate path after turn
      if (frontObstacle) currentState = BACKING;
      else {
        currentState = MOVING;
        lastProgressTime = millis();
      }
      break;
      
    case STOPPED:
      checkSensors();
      if (!frontObstacle) {
        currentState = MOVING;
      } else if (!rearObstacle) {
        currentState = BACKING;
      } else {
        // Random escape maneuver
        randomSeed(analogRead(0));
        if (random(2) == 0) turnLeft();
        else turnRight();
        delay(avoidTurnTime);
        currentState = MOVING;
      }
      break;
  }
  
  updateLCD(); // Refresh display
}

/** Manual IR remote control mode */
void manualMode() {
  // Throttled sensor updates
  if (millis() - lastSensorCheck > 200) {
    checkSensors();
    lastSensorCheck = millis();
  }

  // Execute movement based on button flags
  if (manualForward) moveForward();
  else if (manualBackward) moveBackward();
  else if (manualLeft) turnLeft();
  else if (manualRight) turnRight();
  else stopMotors();
  
  updateLCD();
}

// ============== IR CONTROL SYSTEM ============== //

/** Decodes IR signals and controls mode/operation */
void processIR() {
  if (irrecv.decode()) {
    unsigned long irValue = irrecv.decodedIRData.decodedRawData;
    
    // Button 1: Mode toggle
    if (irValue == IR_BUTTON_1) {
      currentMode = (currentMode == AUTO) ? MANUAL : AUTO;
      // Reset manual controls
      manualForward = manualBackward = manualLeft = manualRight = false;
      stopMotors();
    }
    
    // Manual mode commands
    if (currentMode == MANUAL) {
      switch (irValue) {
        case IR_BUTTON_2: // Forward
          manualForward = true;
          manualBackward = manualLeft = manualRight = false;
          break;
        case IR_BUTTON_8: // Reverse
          manualBackward = true;
          manualForward = manualLeft = manualRight = false;
          break;
        case IR_BUTTON_4: // Left
          manualLeft = true;
          manualForward = manualBackward = manualRight = false;
          break;
        case IR_BUTTON_6: // Right
          manualRight = true;
          manualForward = manualBackward = manualLeft = false;
          break;
        case IR_BUTTON_5: // Stop
          manualForward = manualBackward = manualLeft = manualRight = false;
          stopMotors();
          break;
      }
    }
    
    irrecv.resume(); // Prepare for next signal
  }
}

// ============== MAIN PROGRAM ============== //

void setup() {
  // LCD initialization
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("UltrasonicBot"); // Splash screen
  updateLCD();

  // Configure motor pins as outputs
  for (int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // Initialize ultrasonic pins
  pinMode(sigPinFront, OUTPUT);
  pinMode(sigPinRear, OUTPUT);
  
  // Start IR receiver
  irrecv.enableIRIn();
  irrecv.blink13(false); // Disable built-in LED blinking
  
  // Initialize navigation history
  for (int i = 0; i < MAZE_MEMORY; i++) {
    actionHistory[i] = FORWARD;
  }
}

void loop() {
  processIR(); // Handle incoming IR signals
  
  // Execute appropriate mode handler
  if (currentMode == AUTO) autoMode();
  else manualMode();
  
  delay(25); // Main loop throttle
}
