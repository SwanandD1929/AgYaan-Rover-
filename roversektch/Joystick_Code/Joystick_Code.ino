#include <esp_now.h>
#include <WiFi.h>

// ==========================================
// 1. ROVER MAC ADDRESS CONFIGURATION
// Formatted correctly as hex bytes
// ==========================================
uint8_t roverAddress[] = {0x28, 0x05, 0xA5, 0x2F, 0x9E, 0x3C}; 

// Hardware Pin Definitions
const int pinVRx = 34;
const int pinVRy = 35;
const int pinSW  = 4;

const int CENTER_VAL = 2048;
const int THRESHOLD  = 1000;

// Data packet structure (Must match Rover struct exactly)
typedef struct struct_control {
  char command[16];
  bool buttonPressed;
} struct_control;

struct_control txData;
esp_now_peer_info_t peerInfo;

void setup() {
  Serial.begin(115200);
  pinMode(pinSW, INPUT_PULLUP);
  analogSetAttenuation(ADC_11db);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  // Configure peer connection
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, roverAddress, 6);
  peerInfo.channel = 0;  // 0 uses the current WiFi channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to pair peer!");
    return;
  }
}

void loop() {
  int valX = analogRead(pinVRx);
  int valY = analogRead(pinVRy);
  int valSW = digitalRead(pinSW);

  if (valSW == LOW) {
    strcpy(txData.command, "OK");
    txData.buttonPressed = true;
  } 
  else if (valX > (CENTER_VAL + THRESHOLD)) {
    strcpy(txData.command, "FORWARD");
    txData.buttonPressed = false;
  } 
  else if (valX < (CENTER_VAL - THRESHOLD)) {
    strcpy(txData.command, "BACKWARD");
    txData.buttonPressed = false;
  } 
  else if (valY > (CENTER_VAL + THRESHOLD)) {
    strcpy(txData.command, "RIGHT");
    txData.buttonPressed = false;
  } 
  else if (valY < (CENTER_VAL - THRESHOLD)) {
    strcpy(txData.command, "LEFT");
    txData.buttonPressed = false;
  } 
  else {
    strcpy(txData.command, "STOP");
    txData.buttonPressed = false;
  }

  // Send packet directly to Rover ESP32
  esp_now_send(roverAddress, (uint8_t *) &txData, sizeof(txData));
  
  delay(40); // Transmit rate (~25 Hz)
}