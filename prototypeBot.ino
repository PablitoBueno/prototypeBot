#include <LiquidCrystal.h>
#include <IRremote.h>

// LCD Configuration
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

// Motor pins
const int motor1Pin1 = 2;  // Front left wheel
const int motor1Pin2 = 3;
const int motor2Pin1 = 4;  // Front right wheel
const int motor2Pin2 = 5;
const int motor3Pin1 = 6;  // Rear left wheel
const int motor3Pin2 = 7;
const int motor4Pin1 = 8;  // Rear right wheel
const int motor4Pin2 = 9;

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
int currentSpeed = 150; // PWM speed (0-255)
int displayView = 0;    // Display mode: 0=Default, 1=Sensors, 2=Diagnostic

// Updated IR codes
#define IR_BUTTON_1 0xEF10BF00
#define IR_BUTTON_2 0xEE11BF00  // Forward
#define IR_BUTTON_3 0xED12BF00
#define IR_BUTTON_4 0xEB14BF00  // Left
#define IR_BUTTON_5 0xEA15BF00  // OK/Stop
#define IR_BUTTON_6 0xE916BF00  // Right
#define IR_BUTTON_7 0xE718BF00
#define IR_BUTTON_8 0xE619BF00  // Reverse
#define IR_BUTTON_9 0xE51ABF00
#define IR_BUTTON_0 0xF30CBF00

void setup() {
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("UltrasonicBot");
  updateLCD();

  // Configure motor pins
  for (int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
  }

  // Configure ultrasonic sensors
  pinMode(sigPinFront, OUTPUT);
  pinMode(sigPinRear, OUTPUT);
  
  // Start IR receiver
  irrecv.enableIRIn();
  irrecv.blink13(true);  // Blink LED when receiving signal
  
  // Initialize navigation memory
  for (int i = 0; i < MAZE_MEMORY; i++) {
    actionHistory[i] = FORWARD;
  }
  
  Serial.println("System initialized in AUTO mode");
}

long measureDistance(int trigPin) {
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  pinMode(trigPin, INPUT);
  long duration = pulseIn(trigPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

void moveForward(int speed = 150) {
  speed = constrain(speed, 80, 255);
  
  analogWrite(motor1Pin1, speed);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  analogWrite(motor2Pin2, speed);
  analogWrite(motor3Pin1, speed);
  digitalWrite(motor3Pin2, LOW);
  digitalWrite(motor4Pin1, LOW);
  analogWrite(motor4Pin2, speed);
}

void moveBackward(int speed = 150) {
  speed = constrain(speed, 80, 255);
  
  digitalWrite(motor1Pin1, LOW);
  analogWrite(motor1Pin2, speed);
  analogWrite(motor2Pin1, speed);
  digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor3Pin1, LOW);
  analogWrite(motor3Pin2, speed);
  analogWrite(motor4Pin1, speed);
  digitalWrite(motor4Pin2, LOW);
}

void turnRight(int speed = 150) {
  speed = constrain(speed, 80, 255);
  
  // Left side: forward
  analogWrite(motor1Pin1, speed);
  digitalWrite(motor1Pin2, LOW);
  analogWrite(motor3Pin1, speed);
  digitalWrite(motor3Pin2, LOW);
  
  // Right side: backward
  digitalWrite(motor2Pin1, LOW);
  analogWrite(motor2Pin2, speed);
  digitalWrite(motor4Pin1, LOW);
  analogWrite(motor4Pin2, speed);
}

void turnLeft(int speed = 150) {
  speed = constrain(speed, 80, 255);
  
  // Left side: backward
  digitalWrite(motor1Pin1, LOW);
  analogWrite(motor1Pin2, speed);
  digitalWrite(motor3Pin1, LOW);
  analogWrite(motor3Pin2, speed);
  
  // Right side: forward
  analogWrite(motor2Pin1, speed);
  digitalWrite(motor2Pin2, LOW);
  analogWrite(motor4Pin1, speed);
  digitalWrite(motor4Pin2, LOW);
}

void stopMotors() {
  for (int i = 2; i <= 9; i++) {
    digitalWrite(i, LOW);
  }
}

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  
  // First line: main status
  lcd.setCursor(0, 0);
  lcd.print(currentMode == AUTO ? "A" : "M");
  lcd.print(" ");
  
  switch (currentState) {
    case MOVING: lcd.print("MOV"); break;
    case BACKING: lcd.print("BCK"); break;
    case TURNING_RIGHT: lcd.print("TR-R"); break;
    case TURNING_LEFT: lcd.print("TR-L"); break;
    case STOPPED: lcd.print("STP"); break;
  }
  
  // Second line: mode-specific information
  if (currentMode == AUTO) {
    lcd.setCursor(0, 1);
    lcd.print("M:");
    lcd.print(actionIndex);
    lcd.print(" ");
    
    lcd.setCursor(6, 1);
    lcd.print("F:");
    lcd.print(measureDistance(sigPinFront));
    lcd.print("cm");
  } else {
    switch (displayView) {
      case 0: // Default view
        lcd.setCursor(0, 1);
        if (manualForward) lcd.print("FWD");
        if (manualBackward) lcd.print("REV");
        if (manualLeft) lcd.print(" LEFT");
        if (manualRight) lcd.print(" RIGHT");
        
        lcd.setCursor(10, 1);
        lcd.print("SPD:");
        lcd.print(map(currentSpeed, 0, 255, 0, 100));
        lcd.print("%");
        break;
        
      case 1: // Sensor view
        lcd.setCursor(0, 1);
        lcd.print("F:");
        lcd.print(measureDistance(sigPinFront));
        lcd.print(" R:");
        lcd.print(measureDistance(sigPinRear));
        break;
        
      case 2: // Diagnostic view
        lcd.setCursor(0, 1);
        lcd.print("V:");
        lcd.print(displayView);
        lcd.print("                ");
        break;
    }
  }
  
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

void checkSensors() {
  frontObstacle = (measureDistance(sigPinFront) <= distanceThreshold);
  rearObstacle = (measureDistance(sigPinRear) <= distanceThreshold);
}

bool checkSide(bool isLeft) {
  if (isLeft) {
    turnLeft(150);
    delay(300);
  } else {
    turnRight(150);
    delay(300);
  }
  
  bool obstacle = measureDistance(sigPinFront) <= distanceThreshold;
  
  if (isLeft) {
    turnRight(150);
  } else {
    turnLeft(150);
  }
  delay(300);
  stopMotors();
  
  return obstacle;
}

void autoMode() {
  checkSensors();
  
  // Detect if stuck
  if (millis() - lastProgressTime > ESCAPE_TIME) {
    Serial.println("ESCAPE: No progress detected - executing escape");
    
    // Follow memorized path backwards
    int steps = min(actionIndex, 2);
    for (int i = 0; i < steps; i++) {
      NavAction lastAction = actionHistory[(actionIndex - i - 1) % MAZE_MEMORY];
      
      if (lastAction == TURN_RIGHT) {
        turnLeft(150);
        delay(avoidTurnTime);
      } else if (lastAction == TURN_LEFT) {
        turnRight(150);
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
        Serial.println("AUTO: Front obstacle -> Backing");
      } else {
        moveForward(150);
        lastProgressTime = millis();
      }
      break;
      
    case BACKING:
      // Safety: stop if rear obstacle detected
      if (rearObstacle) {
        Serial.println("AUTO: Rear obstacle detected! Stopping.");
        stopMotors();
        currentState = STOPPED;
        break;
      }
      
      if (frontObstacle && (millis() - backStartTime < maxBackTime)) {
        moveBackward(150);
      } else {
        stopMotors();
        
        // Check sides alternately
        bool tryRight = alternateTurn ? checkSide(false) : checkSide(true);
        bool tryLeft = alternateTurn ? checkSide(true) : checkSide(false);
        alternateTurn = !alternateTurn;
        
        if (!tryRight) {
          currentState = TURNING_RIGHT;
          actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_RIGHT;
          Serial.println("AUTO: Turning right (side clear)");
        } else if (!tryLeft) {
          currentState = TURNING_LEFT;
          actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_LEFT;
          Serial.println("AUTO: Turning left (side clear)");
        } else {
          // Both sides blocked, try opposite to last turn
          if (actionIndex > 0 && actionHistory[(actionIndex-1) % MAZE_MEMORY] == TURN_RIGHT) {
            currentState = TURNING_LEFT;
            actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_LEFT;
            Serial.println("AUTO: Turning left (opposite to last)");
          } else {
            currentState = TURNING_RIGHT;
            actionHistory[actionIndex++ % MAZE_MEMORY] = TURN_RIGHT;
            Serial.println("AUTO: Turning right (default)");
          }
        }
      }
      break;
      
    case TURNING_RIGHT:
      turnRight(150);
      delay(avoidTurnTime);
      stopMotors();
      
      checkSensors();
      if (frontObstacle) {
        Serial.println("AUTO: Path still blocked after turn");
        currentState = BACKING;
        backStartTime = millis();
      } else {
        currentState = MOVING;
        lastProgressTime = millis();
      }
      break;
      
    case TURNING_LEFT:
      turnLeft(150);
      delay(avoidTurnTime);
      stopMotors();
      
      checkSensors();
      if (frontObstacle) {
        Serial.println("AUTO: Path still blocked after turn");
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
          turnLeft(150);
          delay(avoidTurnTime);
        } else {
          turnRight(150);
          delay(avoidTurnTime);
        }
        currentState = MOVING;
      }
      break;
  }
}

void manualMode() {
  // Update sensors
  if (millis() - lastSensorCheck > 200) {
    checkSensors();
    lastSensorCheck = millis();
  }

  // Execute movements
  if (manualForward) {
    moveForward(currentSpeed);
  } else if (manualBackward) {
    moveBackward(currentSpeed);
  } else if (manualLeft) {
    turnLeft(currentSpeed);
  } else if (manualRight) {
    turnRight(currentSpeed);
  } else {
    stopMotors();
  }
  
  updateLCD();
}

void processIR() {
  if (irrecv.decode()) {
    unsigned long irValue = irrecv.decodedIRData.decodedRawData;
    
    Serial.print("IR Received: 0x");
    Serial.println(irValue, HEX);
    
    // Button 1: Toggle AUTO/MANUAL
    if (irValue == IR_BUTTON_1) {
      currentMode = (currentMode == AUTO) ? MANUAL : AUTO;
      Serial.print("MODE: Switched to ");
      Serial.println(currentMode == AUTO ? "AUTO" : "MANUAL");
      
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
          Serial.println("MANUAL: Forward engaged");
          break;
          
        case IR_BUTTON_8: // Reverse
          manualForward = false;
          manualBackward = true;
          manualLeft = false;
          manualRight = false;
          Serial.println("MANUAL: Backward engaged");
          break;
          
        case IR_BUTTON_4: // Left
          manualForward = false;
          manualBackward = false;
          manualLeft = true;
          manualRight = false;
          Serial.println("MANUAL: Left turn engaged");
          break;
          
        case IR_BUTTON_6: // Right
          manualForward = false;
          manualBackward = false;
          manualLeft = false;
          manualRight = true;
          Serial.println("MANUAL: Right turn engaged");
          break;
          
        case IR_BUTTON_5: // Emergency stop
          manualForward = false;
          manualBackward = false;
          manualLeft = false;
          manualRight = false;
          stopMotors();
          Serial.println("MANUAL: Emergency stop!");
          break;
          
        case IR_BUTTON_7: // Cycle display views
          displayView = (displayView + 1) % 3;
          Serial.print("DISPLAY: View changed to ");
          Serial.println(displayView);
          break;
          
        case IR_BUTTON_9: // Increase speed
          currentSpeed = min(255, currentSpeed + 30);
          Serial.print("SPEED: Increased to ");
          Serial.println(currentSpeed);
          break;
          
        case IR_BUTTON_0: // Decrease speed
          currentSpeed = max(80, currentSpeed - 30);
          Serial.print("SPEED: Decreased to ");
          Serial.println(currentSpeed);
          break;
      }
    }
    
    irrecv.resume();
  }
}

void loop() {
  processIR();
  
  if (currentMode == AUTO) {
    autoMode();
    updateLCD();
  } else {
    manualMode();
  }
  
  // Small delay for stability
  delay(50);
}
