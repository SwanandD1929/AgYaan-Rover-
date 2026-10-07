# AgYaan: Autonomous Lunar Exploration Rover

> An autonomous lunar exploration rover featuring real-time 3D isometric terrain hazard scanning, sub-5ms ESP-NOW teleoperation, and a dual-pipeline command HUD.

---

## 🛰️ Project Overview

**AgYaan** is an autonomous planetary exploration rover developed for the **Avishkar Research Convention**. The system addresses computational and power bottlenecks of traditional single-board computer rovers by implementing an ultra-efficient, distributed **ESP32 microcontroller architecture**. 

Taking inspiration from ISRO's *Pragyan* rover (Chandrayaan-3), which utilized stereoscopic vision for Earth-evaluated 2D path planning, AgYaan advances this capability by generating an **onboard real-time 3D surface hazard and obstacle scan model**. 

The rover features:
* **Custom 3D-Engineered Chassis:** Designed and fabricated over **90+ hours of additive manufacturing**, providing mechanical shock dampening and rigid baseline mounts for perception sensors.
* **Autonomous Vector Navigation:** When commanded to explore (e.g., 50 cm or 100 cm), the rover actively scans sectors with its TF-LiDAR, determines the widest obstacle clearance corridor, executes bypass maneuvers, and re-aims toward the designated target waypoint.
* **Extended Sensor Fusion & Odometry:** Dead-reckoning mathematics combining high-frequency quadrature optical encoder pulses with 6-DOF inertial rates (MPU6050) and active Time-of-Flight LiDAR ranging.
* **Sub-5ms ESP-NOW Handheld Control:** An independent physical joystick module operating peer-to-peer over 2.4 GHz raw radio frames, bypassing Wi-Fi congestion and allowing instantaneous toggle between manual and autonomous modes.
* **AgYaan Command HUD:** A custom browser-based dashboard running on the rover's local AP/Station server displaying live dual Haz-Cam streams, compass heading, surface metrics, and a dynamic **3D Isometric Lunar Hazard Grid**.

---

## 📂 Repository File Structure

```text
roversektch/
├── roversektch.ino     # Main rover controller firmware (kinematics, LiDAR, WebServer, ESP-NOW Rx)
├── index.h             # Apollo-X Command HUD interface (HTML5, CSS, JS 3D Isometric Canvas)
└── Joystick_Code/      # Firmware for secondary handheld ESP32 joystick remote (ESP-NOW Tx)
