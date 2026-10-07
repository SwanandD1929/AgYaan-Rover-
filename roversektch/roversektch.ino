#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <esp_now.h>
#include "index.h"

// =====================================================
// WIFI CONFIGURATION
// =====================================================
const char* WIFI_SSID = "Galaxy M30s26CE";
const char* WIFI_PASSWORD = "dwtl7575";

WebServer server(80);

// =====================================================
// ESP-NOW JOYSTICK DATA PACKET
// =====================================================
typedef struct struct_control {
  char command[16];
  bool buttonPressed;
} struct_control;

struct_control rxData;

// =====================================================
// MPU6050 CONFIGURATION
// =====================================================
const int MPU_ADDR = 0x68;
bool mpuConnected = false;
float gyroZ_rad = 0;

// =====================================================
// TF LiDAR
// =====================================================
int lidarDistance = 0;

// =====================================================
// MOTOR PINS (TB6612FNG x2)
// =====================================================
#define FL_PWM 32
#define FL_IN1 33
#define FL_IN2 25
#define FR_PWM 26
#define FR_IN1 27
#define FR_IN2 14

#define BL_PWM 13
#define BL_IN1 18
#define BL_IN2 19
#define BR_PWM 23
#define BR_IN1 5
#define BR_IN2 4

int motorSpeed = 255; 
String currentMotionState = "STOPPED";
String roverMode = "MANUAL";

// =====================================================
// AUTONOMOUS VECTOR NAVIGATION & SCAN ENGINE
// =====================================================
enum NavState {
  NAV_IDLE,
  NAV_TRACKING_TARGET,
  NAV_SCAN_RIGHT,
  NAV_SCAN_LEFT,
  NAV_BYPASS_MOVE,
  NAV_REORIENT
};

NavState navState = NAV_IDLE;
unsigned long navTimer = 0;

float targetX = 0;
float targetY = 0;
bool targetActive = false;

int scanDistRight = 0;
int scanDistLeft = 0;
int bypassChoice = 0; // 1 = Right, -1 = Left

// =====================================================
// ODOMETRY & METRICS
// =====================================================
const float WHEEL_DIAMETER_CM = 6.5; 
const float PULSES_PER_REV = 330.0;  
const float CM_PER_PULSE = (3.14159 * WHEEL_DIAMETER_CM) / PULSES_PER_REV;

volatile long leftCount = 0;
volatile long rightCount = 0;
long prevLeftCount = 0;
long prevRightCount = 0;

float posX = 0.0;
float posY = 0.0;
float headingTheta = 0.0; // Radians

float totalDistanceCm = 0.0;
float totalDisplacement = 0.0;

unsigned long prevTime = 0;

const int leftPhaseA = 34;
const int leftPhaseB = 35;
const int rightPhaseA = 36;
const int rightPhaseB = 39;

void IRAM_ATTR leftEncoderISR() {
  if (digitalRead(leftPhaseB) == HIGH) leftCount++;
  else leftCount--;
}

void IRAM_ATTR rightEncoderISR() {
  if (digitalRead(rightPhaseB) == HIGH) rightCount++;
  else rightCount--;
}

// =====================================================
// MOTOR CONTROL PRIMITIVES
// =====================================================
void motorControl(int pwmPin, int in1, int in2, int direction) {
  if (direction == 1) {
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW); analogWrite(pwmPin, motorSpeed);
  } else if (direction == -1) {
    digitalWrite(in1, LOW); digitalWrite(in2, HIGH); analogWrite(pwmPin, motorSpeed);
  } else {
    digitalWrite(in1, LOW); digitalWrite(in2, LOW); analogWrite(pwmPin, 0);
  }
}

void stopMotors() {
  motorControl(FL_PWM, FL_IN1, FL_IN2, 0); motorControl(FR_PWM, FR_IN1, FR_IN2, 0);
  motorControl(BL_PWM, BL_IN1, BL_IN2, 0); motorControl(BR_PWM, BR_IN1, BR_IN2, 0);
  currentMotionState = "STOPPED";
}

void moveForward() {
  motorControl(FL_PWM, FL_IN1, FL_IN2, -1); motorControl(BL_PWM, BL_IN1, BL_IN2, -1);
  motorControl(FR_PWM, FR_IN1, FR_IN2, 1); motorControl(BR_PWM, BR_IN1, BR_IN2, 1);
  currentMotionState = "FORWARD";
}

void moveBackward() {
  motorControl(FL_PWM, FL_IN1, FL_IN2, 1); motorControl(BL_PWM, BL_IN1, BL_IN2, 1);
  motorControl(FR_PWM, FR_IN1, FR_IN2, -1); motorControl(BR_PWM, BR_IN1, BR_IN2, -1);
  currentMotionState = "BACKWARD";
}

void turnLeft() {
  motorControl(FL_PWM, FL_IN1, FL_IN2, 1); motorControl(BL_PWM, BL_IN1, BL_IN2, 1);
  motorControl(FR_PWM, FR_IN1, FR_IN2, 1); motorControl(BR_PWM, BR_IN1, BR_IN2, 1);
  currentMotionState = "LEFT";
}

void turnRight() {
  motorControl(FL_PWM, FL_IN1, FL_IN2, -1); motorControl(BL_PWM, BL_IN1, BL_IN2, -1);
  motorControl(FR_PWM, FR_IN1, FR_IN2, -1); motorControl(BR_PWM, BR_IN1, BR_IN2, -1);
  currentMotionState = "RIGHT";
}

// =====================================================
// ESP-NOW PACKET RECEIVER
// =====================================================
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&rxData, incomingData, sizeof(rxData));

  if (rxData.buttonPressed) {
    roverMode = (roverMode == "AUTONOMOUS") ? "MANUAL" : "AUTONOMOUS";
    stopMotors();
    targetActive = false;
    navState = NAV_IDLE;
    return;
  }

  if (roverMode == "AUTONOMOUS") return;

  if (strcmp(rxData.command, "FORWARD") == 0) moveForward();
  else if (strcmp(rxData.command, "BACKWARD") == 0) moveBackward();
  else if (strcmp(rxData.command, "LEFT") == 0) turnLeft();
  else if (strcmp(rxData.command, "RIGHT") == 0) turnRight();
  else if (strcmp(rxData.command, "STOP") == 0) stopMotors();
}

// =====================================================
// WEB SERVER ENDPOINTS
// =====================================================
void handleRoot() { server.send(200, "text/html", dashboard); }

void handleMode() {
  if (server.hasArg("state")) {
    roverMode = server.arg("state");
    roverMode.toUpperCase();
    stopMotors();
    targetActive = false;
    navState = (roverMode == "AUTONOMOUS") ? NAV_TRACKING_TARGET : NAV_IDLE;
  }
  server.send(200, "text/plain", "OK");
}

void handleMove() {
  if (roverMode == "AUTONOMOUS") return;
  if (!server.hasArg("direction")) return;
  String dir = server.arg("direction");
  if (dir == "forward") moveForward();
  else if (dir == "backward") moveBackward();
  else if (dir == "left") turnLeft();
  else if (dir == "right") turnRight();
  server.send(200, "text/plain", "OK");
}

void handleExplore() {
  if (server.hasArg("cm")) {
    float dist = server.arg("cm").toFloat();
    // Compute waypoint in current heading direction
    targetX = posX + dist * cos(headingTheta);
    targetY = posY + dist * sin(headingTheta);
    targetActive = true;
    roverMode = "AUTONOMOUS";
    navState = NAV_TRACKING_TARGET;
  }
  server.send(200, "text/plain", "EXPLORING");
}

void handleStop() { 
  stopMotors(); 
  targetActive = false;
  navState = NAV_IDLE;
  server.send(200, "text/plain", "STOPPED"); 
}

void handleSensor() {
  float headingDeg = headingTheta * (180.0 / 3.14159);
  while(headingDeg >= 360.0) headingDeg -= 360.0;
  while(headingDeg < 0.0) headingDeg += 360.0;

  String stateStr = "IDLE";
  if (navState == NAV_TRACKING_TARGET) stateStr = "NAVIGATING";
  else if (navState == NAV_SCAN_RIGHT || navState == NAV_SCAN_LEFT) stateStr = "SCANNING SECTORS";
  else if (navState == NAV_BYPASS_MOVE) stateStr = "EVADING OBSTACLE";
  else if (navState == NAV_REORIENT) stateStr = "RE-AIMING TARGET";

  String json = "{";
  json += "\"tot\":" + String(totalDistanceCm, 1) + ",";
  json += "\"disp\":" + String(totalDisplacement, 1) + ",";
  json += "\"heading\":" + String(headingDeg, 1) + ",";
  json += "\"x\":" + String(posX, 1) + ",";
  json += "\"y\":" + String(posY, 1) + ",";
  json += "\"tx\":" + String(targetX, 1) + ",";
  json += "\"ty\":" + String(targetY, 1) + ",";
  json += "\"targetActive\":" + String(targetActive ? "true" : "false") + ",";
  json += "\"navState\":\"" + stateStr + "\",";
  json += "\"lidar\":" + String(lidarDistance) + ",";
  json += "\"mode\":\"" + roverMode + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

// =====================================================
// CONTINUOUS FAST SENSORS & AUTONOMOUS ENGINE
// =====================================================
void processLiDAR() {
  while (Serial2.available() >= 9) {
    if (Serial2.read() == 0x59) {
      if (Serial2.read() == 0x59) {
        int distLow = Serial2.read();
        int distHigh = Serial2.read();
        lidarDistance = distLow + (distHigh * 256);
        for(int i=0; i<5; i++) Serial2.read(); 
      }
    }
  }
}

void runAutonomousEngine() {
  if (roverMode != "AUTONOMOUS") return;

  unsigned long currentMillis = millis();

  // If executing targeted exploration, verify if distance to target is reached
  if (targetActive) {
    float dx = targetX - posX;
    float dy = targetY - posY;
    float distanceRemaining = sqrt((dx * dx) + (dy * dy));

    if (distanceRemaining < 12.0) { // Target reached within 12 cm
      stopMotors();
      targetActive = false;
      navState = NAV_IDLE;
      roverMode = "MANUAL";
      return;
    }
  }

  switch (navState) {
    case NAV_TRACKING_TARGET:
      // Check if LiDAR detects hazard blocking forward path
      if (lidarDistance > 0 && lidarDistance < 28) {
        stopMotors();
        navTimer = currentMillis;
        navState = NAV_SCAN_RIGHT;
      } else {
        moveForward();
      }
      break;

    // Scan Sector 1: Turn Right 350ms and sample LiDAR clearance
    case NAV_SCAN_RIGHT:
      turnRight();
      if (currentMillis - navTimer >= 350) {
        stopMotors();
        scanDistRight = (lidarDistance == 0) ? 999 : lidarDistance;
        navTimer = currentMillis;
        navState = NAV_SCAN_LEFT;
      }
      break;

    // Scan Sector 2: Turn Left 700ms (to sample left sector)
    case NAV_SCAN_LEFT:
      turnLeft();
      if (currentMillis - navTimer >= 700) {
        stopMotors();
        scanDistLeft = (lidarDistance == 0) ? 999 : lidarDistance;
        
        // Decide optimal bypass vector: Which side has more clearance?
        if (scanDistRight >= scanDistLeft && scanDistRight > 30) {
          bypassChoice = 1; // Turn right to bypass
        } else if (scanDistLeft > 30) {
          bypassChoice = -1; // Turn left to bypass
        } else {
          bypassChoice = 2; // Dead end: complete 180 turnaround
        }

        navTimer = currentMillis;
        navState = NAV_BYPASS_MOVE;
      }
      break;

    // Execute bypass motion along the clearer corridor
    case NAV_BYPASS_MOVE:
      if (bypassChoice == 1) {
        turnRight(); // Turn back towards right opening
      } else if (bypassChoice == -1) {
        // Already pointing left
        moveForward();
      } else {
        moveBackward();
      }

      if (currentMillis - navTimer >= 500) {
        stopMotors();
        navTimer = currentMillis;
        navState = NAV_REORIENT;
      }
      break;

    // Reorient forward and resume tracking target
    case NAV_REORIENT:
      moveForward();
      if (currentMillis - navTimer >= 600) { // Advance past obstacle flank
        stopMotors();
        navState = NAV_TRACKING_TARGET;
      }
      break;

    default:
      break;
  }
}

void updateOdometry() {
  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0;
  if (dt <= 0) return;
  prevTime = currentTime;

  if (mpuConnected) {
    Wire.beginTransmission(MPU_ADDR); Wire.write(0x47); Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 2, true);
    if (Wire.available() >= 2) {
      int16_t rawGz = Wire.read()<<8 | Wire.read();
      gyroZ_rad = (rawGz / 131.0) * (3.14159 / 180.0);
      if (abs(gyroZ_rad) < 0.03) gyroZ_rad = 0; 
      headingTheta += gyroZ_rad * dt;
    }
  }

  noInterrupts();
  long currentLeft = leftCount;
  long currentRight = rightCount;
  interrupts();

  long deltaLeft = currentLeft - prevLeftCount;
  long deltaRight = currentRight - prevRightCount;
  prevLeftCount = currentLeft;
  prevRightCount = currentRight;

  float distLeft = deltaLeft * CM_PER_PULSE;
  float distRight = deltaRight * CM_PER_PULSE;
  float deltaDistance = (distLeft + distRight) / 2.0;

  if (currentMotionState == "FORWARD" || currentMotionState == "BACKWARD") {
    totalDistanceCm += abs(deltaDistance);
  }

  posX += deltaDistance * cos(headingTheta);
  posY += deltaDistance * sin(headingTheta);
  totalDisplacement = sqrt((posX * posX) + (posY * posY));
}

// =====================================================
// SETUP & LOOP
// =====================================================
void setup() {
  Serial.begin(115200); 
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  delay(400);

  pinMode(FL_PWM, OUTPUT); pinMode(FL_IN1, OUTPUT); pinMode(FL_IN2, OUTPUT);
  pinMode(FR_PWM, OUTPUT); pinMode(FR_IN1, OUTPUT); pinMode(FR_IN2, OUTPUT);
  pinMode(BL_PWM, OUTPUT); pinMode(BL_IN1, OUTPUT); pinMode(BL_IN2, OUTPUT);
  pinMode(BR_PWM, OUTPUT); pinMode(BR_IN1, OUTPUT); pinMode(BR_IN2, OUTPUT);
  stopMotors();

  pinMode(leftPhaseA, INPUT); pinMode(leftPhaseB, INPUT);
  pinMode(rightPhaseA, INPUT); pinMode(rightPhaseB, INPUT);
  attachInterrupt(digitalPinToInterrupt(leftPhaseA), leftEncoderISR, RISING);
  attachInterrupt(digitalPinToInterrupt(rightPhaseA), rightEncoderISR, RISING);

  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x6B); Wire.write(0);
  if (Wire.endTransmission(true) == 0) {
    mpuConnected = true;
    Serial.println("MPU6050 OK");
  }

  WiFi.mode(WIFI_AP_STA); 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  Serial.print("Rover IP: ");
  Serial.println(WiFi.localIP());

  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(OnDataRecv);
  }

  server.on("/", handleRoot);
  server.on("/mode", handleMode);
  server.on("/move", handleMove);
  server.on("/explore", handleExplore);
  server.on("/stop", handleStop);
  server.on("/sensor", handleSensor);
  server.begin();
  
  prevTime = millis();
}

void loop() {
  server.handleClient();
  processLiDAR();
  runAutonomousEngine();
  updateOdometry();
}