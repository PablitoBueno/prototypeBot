# UltrasonicBot: Obstacle-Avoidance Mobile Robot with LCD Feedback

## Abstract
This project introduces **UltrasonicBot**, a four‑wheeled Arduino UNO–based robot that uses a 3‑pin ultrasonic sensor for obstacle detection and a 16×2 LCD for real‑time status feedback. When an object comes within 20 cm, the robot stops, displays “Stopped” on the LCD, and waits until the path is clear to resume motion.

## 1. Introduction
Autonomous obstacle avoidance is a core capability for mobile robots. UltrasonicBot demonstrates fundamental concepts in:

- **Signal acquisition** with a single‑pin ultrasonic sensor (trigger & echo).  
- **DC motor control** via L293D drivers and four independent wheels.  
- **Human–machine interface** through an LCD displaying “Moving” or “Stopped.”  

This platform serves as a hands‑on tutorial for beginners in robotics and embedded systems.

## 2. Materials and Methods

| Component                  | Quantity | Arduino Pin(s)   |
|----------------------------|----------|------------------|
| Arduino UNO                | 1        | —                |
| Ultrasonic sensor (3‑pin)  | 1        | D10              |
| L293D motor driver IC      | 2        | D2–D9            |
| DC gear motors (130 RPM)   | 4        | via L293D        |
| LCD 16×2                   | 1        | A0–A5            |
| 9 V battery (snap‑on)      | 2        | VIN              |
| Jumper wires & protoboard  | —        | —                |

1. **Distance measurement**  
   - Emit a 10 µs pulse on the signal pin.  
   - Measure echo duration with `pulseIn(sigPin, HIGH, 30000)`.  
   - Convert to centimeters:  
     \[
       d = \frac{v_{\text{sound}} \times t}{2}, \quad v_{\text{sound}} \approx 0.034\ \mathrm{cm/µs}
     \]

2. **Drive control**  
   - Motors 1 & 3: forward when `Pin1=HIGH, Pin2=LOW`.  
   - Motors 2 & 4: reverse direction to achieve straight motion.

3. **LCD feedback**  
   - Display “Moving” if distance > 20 cm.  
   - Display “Stopped” otherwise.  
   - Clear and center text for readability.

## 3. Expected Performance
- **Autonomous navigation** on flat surfaces, avoiding static obstacles.  
- **Real‑time status updates** via LCD for rapid diagnostics.  
- **Reaction time** within 100 ms (measurement & decision loop every 100 ms).

## 4. Applications and Benefits
- **Robotics education**: teaches ultrasonic sensing, motor drivers, and LCD interfacing.  
- **Prototype platforms**: foundation for delivery bots, inspection robots, or cleaning robots.  
- **Electronics labs**: rapid prototyping with Tinkercad and Arduino.

## 5. Learning Objectives
- Understand the principles of ultrasonic distance sensing.  
- Integrate multiple actuators (DC motors) and control buses.  
- Build a simple user interface with an LCD.  
- Practice C++ programming for microcontrollers.

## 6. Try It Online
Experience and test the complete circuit and code in your browser using Tinkercad:  
🔗 [UltrasonicBot on Tinkercad](https://www.tinkercad.com/things/2lPirGXAuKa-ultrasonicrobot?sharecode=cIJoI-HL6gTz--v_P5dtehzebWO45mbtDTdelOniyGU)

## 7. Conclusion
UltrasonicBot is a compact, effective platform for exploring the fundamentals of mobile robots with obstacle detection. Its simple build and concise code make it ideal for introductory robotics projects and educational workshops.

---
