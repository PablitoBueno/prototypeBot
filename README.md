# UltrasonicBot: Advanced Obstacle-Avoidance Mobile Robot with IR Control and Dual Ultrasonic Sensors

## Abstract

**UltrasonicBot** is an Arduino UNO–based four-wheeled mobile robot featuring:

- **Dual ultrasonic sensors** (front & rear) for obstacle detection
- **Infrared (IR) remote control** for switching between _AUTO_ and _MANUAL_ modes and direct navigation commands
- **16×2 LCD display** for real-time status and sensor readings
- **L293D motor drivers** controlling four independent DC gear motors
- **Adaptive state machine** with automatic escape logic using memory of past actions

## Dependencies

- Arduino IDE (≥ 1.8.13)
- Libraries:
  - `LiquidCrystal`
  - `IRremote`

## Pin Configuration

| Device                          | Arduino Pin | Notes                                      |
| ------------------------------- | ----------- | ------------------------------------------ |
| **Motors (via L293D drivers)**  |             |                                            |
| Front-left (IN1/IN2)            | 2, 3        |                                            |
| Front-right (IN3/IN4)           | 4, 5        | Note: Reverse polarity                     |
| Rear-left (IN1/IN2)             | 6, 7        |                                            |
| Rear-right (IN3/IN4)            | 8, 9        | Note: Reverse polarity                     |
| **Ultrasonic Sensors**          |             |                                            |
| Front Sensor (Trig/Echo)        | 10          | Single pin operation                       |
| Rear Sensor (Trig/Echo)         | 11          | Single pin operation                       |
| **IR Receiver**                 | 12          | Receives IR commands                       |
| **LCD 16×2 Display**            | A0-A5       | RS, E, D4-D7 pins respectively             |

## Operation Modes

### AUTO Mode

- Robot moves forward until **front ultrasonic sensor** detects obstacle ≤ 20 cm
- On detection:
  1. Backs up for ≤ 2 seconds
  2. Checks left/right clearance with 300ms test turns
  3. Selects first clear path; alternates turn direction if both blocked
- Maintains memory of last 5 navigation actions
- If no progress for 3 seconds, executes **escape sequence** (reverses recent turns)
- LCD displays:
  - State: `Auto Mov`, `Auto Bck`, `Auto TrR`, `Auto TrL`, or `Auto Stp`
  - Front/rear distances (cm)
  - Obstacle indicators: `F!` (Front), `R!` (Rear)

### MANUAL Mode

- Fully controlled by IR remote:

| Button | Function             | IR Code      |
| ------ | -------------------- | ------------ |
| 1      | Toggle Auto/Manual   | 0xEF10BF00   |
| 2      | Forward              | 0xEE11BF00   |
| 4      | Turn Left            | 0xEB14BF00   |
| 5      | Emergency Stop       | 0xEA15BF00   |
| 6      | Turn Right           | 0xE916BF00   |
| 8      | Reverse              | 0xE619BF00   |

- LCD displays:
  - `Manual` mode indicator
  - Real-time front/rear distance readings
  - Obstacle warnings

## State Machine & Logic

- **States:**
  - `MOVING`: Normal forward motion
  - `BACKING`: Reverse maneuver
  - `TURNING_RIGHT`: 600ms right turn
  - `TURNING_LEFT`: 600ms left turn
  - `STOPPED`: Full halt (emergency)
- **Escape Logic:** Reverses last actions after 3s inactivity
- **Obstacle Handling:** Stops backing if rear obstacle detected

## Usage

1. Upload `liteBot.ino` to Arduino UNO
2. Connect components per pin mapping
3. Power requirements:
   - 5 V for control logic
   - Separate 6-12V supply for motors (recommended)
4. Control with IR remote:
   - Button 1: Toggle Auto/Manual
   - Buttons 2/4/6/8: Directional control
   - Button 5: Emergency stop
5. Monitor status via LCD

## Tinkercad Simulation

Try it online:  
[https://www.tinkercad.com/things/5Ym8NNUPtSY-ultrasonicrobot](https://www.tinkercad.com/things/5Ym8NNUPtSY-ultrasonicrobot)
