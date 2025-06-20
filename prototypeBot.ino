#include <LiquidCrystal.h>
#include <IRremote.h>

// Configuração do LCD
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

// Pinos dos motores
const int motor1Pin1 = 2;  // Roda esquerda dianteira
const int motor1Pin2 = 3;
const int motor2Pin1 = 4;  // Roda direita dianteira
const int motor2Pin2 = 5;
const int motor3Pin1 = 6;  // Roda esquerda traseira
const int motor3Pin2 = 7;
const int motor4Pin1 = 8;  // Roda direita traseira
const int motor4Pin2 = 9;

// Sensores ultrassônicos
const int sigPinFront = 10; // Sensor frontal
const int sigPinRear = 11;  // Sensor traseiro

// Receptor IR
const int irReceiverPin = 12;
IRrecv irrecv(irReceiverPin);

// Faróis LED
const int frontLightsPin = 1;  // LEDs frontais (D1)
const int rearLightsPin = 0;   // LEDs traseiros (D0)

// Parâmetros
const long distanceThreshold = 20;    // Distância mínima para detecção (cm)
const unsigned long avoidTurnTime = 600; // Tempo para virar 90° (ms)
const unsigned long maxBackTime = 2000;  // Tempo máximo de recuo (ms)
const int MAZE_MEMORY = 5;           // Memória de últimas decisões
const unsigned long ESCAPE_TIME = 3000; // Tempo máximo para escapar (ms)

// Estados e modos
enum RobotState { MOVING, BACKING, TURNING_RIGHT, TURNING_LEFT, STOPPED };
enum OperationMode { AUTO, MANUAL };
enum LightMode { ALL_OFF, FRONT_ONLY, REAR_ONLY, ALL_ON };
enum NavAction { FORWARD, BACK, TURN_RIGHT, TURN_LEFT };

// Variáveis globais
RobotState currentState = MOVING;
OperationMode currentMode = AUTO;
LightMode currentLightMode = ALL_OFF;
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
int currentSpeed = 150; // Velocidade PWM (0-255)
int displayView = 0;    // Modo de visualização: 0=Padrão, 1=Sensores, 2=Diagnóstico

// Códigos IR atualizados conforme especificado
#define IR_BUTTON_1 0xEF10BF00
#define IR_BUTTON_2 0xEE11BF00  // Frente
#define IR_BUTTON_3 0xED12BF00
#define IR_BUTTON_4 0xEB14BF00  // Esquerda
#define IR_BUTTON_5 0xEA15BF00  // OK/Stop
#define IR_BUTTON_6 0xE916BF00  // Direita
#define IR_BUTTON_7 0xE718BF00
#define IR_BUTTON_8 0xE619BF00  // Ré
#define IR_BUTTON_9 0xE51ABF00
#define IR_BUTTON_0 0xF30CBF00
#define IR_BUTTON_STAR 0xF708BF00  // Estrela (*)
#define IR_BUTTON_HASH 0xE31CBF00  // Hashtag (#)

void setup() {
  Serial.begin(9600);
  
  // Inicializar LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("UltrasonicBot");
  updateLCD();

  // Configurar pinos dos motores
  for (int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
  }

  // Configurar sensores ultrassônicos
  pinMode(sigPinFront, OUTPUT);
  pinMode(sigPinRear, OUTPUT);
  
  // Configurar faróis LED
  pinMode(frontLightsPin, OUTPUT);
  pinMode(rearLightsPin, OUTPUT);
  updateLights();
  
  // Iniciar receptor IR
  irrecv.enableIRIn();
  irrecv.blink13(true);  // LED pisca ao receber sinal
  
  // Inicializar memória de navegação
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
  
  // Lado esquerdo: frente
  analogWrite(motor1Pin1, speed);
  digitalWrite(motor1Pin2, LOW);
  analogWrite(motor3Pin1, speed);
  digitalWrite(motor3Pin2, LOW);
  
  // Lado direito: ré
  digitalWrite(motor2Pin1, LOW);
  analogWrite(motor2Pin2, speed);
  digitalWrite(motor4Pin1, LOW);
  analogWrite(motor4Pin2, speed);
}

void turnLeft(int speed = 150) {
  speed = constrain(speed, 80, 255);
  
  // Lado esquerdo: ré
  digitalWrite(motor1Pin1, LOW);
  analogWrite(motor1Pin2, speed);
  digitalWrite(motor3Pin1, LOW);
  analogWrite(motor3Pin2, speed);
  
  // Lado direito: frente
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

void updateLights() {
  switch (currentLightMode) {
    case ALL_OFF:
      digitalWrite(frontLightsPin, LOW);
      digitalWrite(rearLightsPin, LOW);
      break;
    case FRONT_ONLY:
      digitalWrite(frontLightsPin, HIGH);
      digitalWrite(rearLightsPin, LOW);
      break;
    case REAR_ONLY:
      digitalWrite(frontLightsPin, LOW);
      digitalWrite(rearLightsPin, HIGH);
      break;
    case ALL_ON:
      digitalWrite(frontLightsPin, HIGH);
      digitalWrite(rearLightsPin, HIGH);
      break;
  }
}

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  
  // Primeira linha: estado principal
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
  
  // Segunda linha: informações específicas do modo
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
      case 0: // Visão padrão
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
        
      case 1: // Visão de sensores
        lcd.setCursor(0, 1);
        lcd.print("F:");
        lcd.print(measureDistance(sigPinFront));
        lcd.print(" R:");
        lcd.print(measureDistance(sigPinRear));
        break;
        
      case 2: // Visão de diagnóstico
        lcd.setCursor(0, 1);
        lcd.print("L:");
        switch (currentLightMode) {
          case ALL_OFF: lcd.print("OFF"); break;
          case FRONT_ONLY: lcd.print("FRONT"); break;
          case REAR_ONLY: lcd.print("REAR"); break;
          case ALL_ON: lcd.print("ALL"); break;
        }
        lcd.print(" V:");
        lcd.print(displayView);
        break;
    }
  }
  
  // Indicadores de obstáculo
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
  
  // Detectar se está preso
  if (millis() - lastProgressTime > ESCAPE_TIME) {
    Serial.println("ESCAPE: No progress detected - executing escape");
    
    // Voltar pelo caminho memorizado
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
    
    // Resetar memória
    actionIndex = 0;
    lastProgressTime = millis();
    currentState = MOVING;
    return;
  }

  // Máquina de estados principal
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
      // Segurança: parar se obstáculo traseiro detectado
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
        
        // Verificar lados alternadamente
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
          // Ambos lados bloqueados, tentar direção oposta à última curva
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
      // Tentar encontrar saída
      checkSensors();
      if (!frontObstacle) {
        currentState = MOVING;
      } else if (!rearObstacle) {
        currentState = BACKING;
        backStartTime = millis();
      } else {
        // Girar aleatoriamente para tentar sair
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
  // Atualizar sensores
  if (millis() - lastSensorCheck > 200) {
    checkSensors();
    lastSensorCheck = millis();
  }

  // Executar movimentos
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
  
  // Luz de ré automática
  if (manualBackward) {
    digitalWrite(rearLightsPin, HIGH);
  } else if (currentLightMode != REAR_ONLY && currentLightMode != ALL_ON) {
    digitalWrite(rearLightsPin, LOW);
  }
  
  updateLCD();
}

void processIR() {
  if (irrecv.decode()) {
    unsigned long irValue = irrecv.decodedIRData.decodedRawData;
    
    Serial.print("IR Received: 0x");
    Serial.println(irValue, HEX);
    
    // Botão 1: Alternar entre AUTO/MANUAL
    if (irValue == IR_BUTTON_1) {
      currentMode = (currentMode == AUTO) ? MANUAL : AUTO;
      Serial.print("MODE: Switched to ");
      Serial.println(currentMode == AUTO ? "AUTO" : "MANUAL");
      
      // Resetar controles
      manualForward = false;
      manualBackward = false;
      manualLeft = false;
      manualRight = false;
      stopMotors();
      updateLights();
    }
    
    // Comandos específicos do modo manual
    if (currentMode == MANUAL) {
      switch (irValue) {
        case IR_BUTTON_2: // Frente
          manualForward = true;
          manualBackward = false;
          manualLeft = false;
          manualRight = false;
          Serial.println("MANUAL: Forward engaged");
          break;
          
        case IR_BUTTON_8: // Ré
          manualForward = false;
          manualBackward = true;
          manualLeft = false;
          manualRight = false;
          Serial.println("MANUAL: Backward engaged");
          break;
          
        case IR_BUTTON_4: // Esquerda
          manualForward = false;
          manualBackward = false;
          manualLeft = true;
          manualRight = false;
          Serial.println("MANUAL: Left turn engaged");
          break;
          
        case IR_BUTTON_6: // Direita
          manualForward = false;
          manualBackward = false;
          manualLeft = false;
          manualRight = true;
          Serial.println("MANUAL: Right turn engaged");
          break;
          
        case IR_BUTTON_5: // Parada de emergência
          manualForward = false;
          manualBackward = false;
          manualLeft = false;
          manualRight = false;
          stopMotors();
          Serial.println("MANUAL: Emergency stop!");
          break;
          
        case IR_BUTTON_3: // Ciclar modos de faróis
          currentLightMode = (LightMode)((currentLightMode + 1) % 4);
          updateLights();
          Serial.print("LIGHTS: Mode changed to ");
          switch (currentLightMode) {
            case ALL_OFF: Serial.println("ALL OFF"); break;
            case FRONT_ONLY: Serial.println("FRONT ONLY"); break;
            case REAR_ONLY: Serial.println("REAR ONLY"); break;
            case ALL_ON: Serial.println("ALL ON"); break;
          }
          break;
          
        case IR_BUTTON_7: // Alternar visualizações
          displayView = (displayView + 1) % 3;
          Serial.print("DISPLAY: View changed to ");
          Serial.println(displayView);
          break;
          
        case IR_BUTTON_9: // Aumentar velocidade
          currentSpeed = min(255, currentSpeed + 30);
          Serial.print("SPEED: Increased to ");
          Serial.println(currentSpeed);
          break;
          
        case IR_BUTTON_0: // Diminuir velocidade
          currentSpeed = max(80, currentSpeed - 30);
          Serial.print("SPEED: Decreased to ");
          Serial.println(currentSpeed);
          break;
          
        case IR_BUTTON_STAR: // Combinação: Frente + Esquerda
          manualForward = true;
          manualLeft = true;
          manualRight = false;
          Serial.println("MANUAL: Forward + Left");
          break;
          
        case IR_BUTTON_HASH: // Combinação: Frente + Direita
          manualForward = true;
          manualRight = true;
          manualLeft = false;
          Serial.println("MANUAL: Forward + Right");
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
  
  // Pequeno delay para estabilidade
  delay(50);
}
