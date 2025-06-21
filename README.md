# UltrasonicBot: Advanced Obstacle-Avoidance Mobile Robot with IR Control and Dual Ultrasonic Sensors

## Abstract

**UltrasonicBot** is an Arduino UNO–based four-wheeled mobile robot featuring:

- **Dual ultrasonic sensors** (front & rear) for obstacle detection.
- **Infrared (IR) remote control** for switching between _AUTO_ and _MANUAL_ modes and direct navigation commands.
- **16×2 LCD display** for real-time status, sensor readings, and diagnostics.
- **L293D motor drivers** controlling four independent DC gear motors.
- **Adaptive state machine** with automatic escape logic using memory of past actions.

## Dependencies

- Arduino IDE (≥ 1.8.13)
- Libraries:
  - `LiquidCrystal`
  - `IRremote`

## Pin Configuration

| Device                          | Arduino Pin | Notes                                      |
| ------------------------------- | ----------- | ------------------------------------------ |
| **Motors (via L293D drivers)**  |             |                                            |
| Motor 1 (front-left) - IN1      | 2           | PWM forward/backward                       |
| Motor 1 - IN2                    | 3           |                                            |
| Motor 2 (front-right) - IN1     | 4           |                                            |
| Motor 2 - IN2                    | 5           |                                            |
| Motor 3 (rear-left) - IN1       | 6           |                                            |
| Motor 3 - IN2                    | 7           |                                            |
| Motor 4 (rear-right) - IN1      | 8           |                                            |
| Motor 4 - IN2                    | 9           |                                            |
| **Ultrasonic Sensors**          |             |                                            |
| Front Sensor (Trig/Echo)        | 10          | Single pin (combined trigger/echo)         |
| Rear Sensor (Trig/Echo)         | 11          | Single pin (combined trigger/echo)         |
| **IR Receiver**                 | 12          | Receives IR commands                       |
| **LCD 16×2 Display**            |             |                                            |
| RS                              | A0          |                                            |
| Enable (E)                      | A1          |                                            |
| D4                              | A2          |                                            |
| D5                              | A3          |                                            |
| D6                              | A4          |                                            |
| D7                              | A5          |                                            |

## Operation Modes

### AUTO Mode

- The robot moves forward until the **front ultrasonic sensor** detects an obstacle ≤ 20 cm.
- On detection:
  - It backs up (≤ 2 seconds).
  - Then checks left and right by turning briefly.
  - Chooses the first side that is clear; if both sides are blocked, alternates based on the last turn.
- Keeps a memory of the last 5 navigation actions.
- If no progress for 3 seconds, it executes an **escape sequence**, reversing previous moves.
- LCD displays:
  - State (`MOV`, `BCK`, `TR-R`, `TR-L`, `STP`).
  - Memory index.
  - Front distance in cm.
- Indicators:
  - `F!` (Front obstacle).
  - `R!` (Rear obstacle).

### MANUAL Mode

- Fully controlled by the IR remote:

| Button | Function             | IR Code      |
| ------ | -------------------- | ------------ |
| 1      | switch control mode  | 0xEF10BF00   |
| 2      | Forward              | 0xEE11BF00   |
| 8      | Backward             | 0xE619BF00   |
| 4      | Turn Left            | 0xEB14BF00   |
| 6      | Turn Right           | 0xE916BF00   |
| 5      | Emergency Stop       | 0xEA15BF00   |
| 7      | Change LCD View      | 0xE718BF00   |
| 9      | Increase Speed (+30) | 0xE51ABF00   |
| 0      | Decrease Speed (-30) | 0xF30CBF00   |

- LCD Views:
  - **Standard View:** Current direction and speed percentage.
  - **Sensor View:** Front and rear distance readings.

## State Machine & Logic

- **States:**
  - `MOVING`
  - `BACKING`
  - `TURNING_RIGHT`
  - `TURNING_LEFT`
  - `STOPPED`
- **Escape Timer:** If no progress for 3000 ms, the robot automatically reverses the last actions to escape.
- **Obstacle Avoidance:**
  - Backs off if the rear sensor detects an obstacle while reversing.

## Usage

1. Upload the `prototypeBot.ino` sketch to your Arduino UNO.
2. Connect motors, ultrasonic sensors, IR receiver, and LCD according to the pin mapping above.
3. Provide power:
   - 5 V for logic.
   - Separate battery for motors (recommended).
4. Use the IR remote to:
   - Switch between AUTO and MANUAL modes.
   - Control the robot manually.
5. Monitor robot status and diagnostics on the LCD screen.

## Tinkercad Simulation

Try it online:  
[https://www.tinkercad.com/things/5Ym8NNUPtSY-ultrasonicrobot](https://www.tinkercad.com/things/5Ym8NNUPtSY-ultrasonicrobot)
