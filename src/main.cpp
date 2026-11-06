#include <Arduino.h> // Required for PlatformIO
#include <SPI.h>
#include <Wire.h>
#include <GxEPD2_BW.h> 
#include <GxEPD2_4C.h>
#include <GxEPD2_EPD.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "LSM6DS3.h"

// XIAO nRF52840 Pin Definitions for E-Paper
#define EPD_CS   3
#define EPD_DC   2
#define EPD_RST  1
#define EPD_BUSY 0

// Missing LSM6DS3 Register Addresses
#define LSM6DS3_CTRL1_XL  0x10
#define LSM6DS3_CTRL10_C  0x19
#define LSM6DS3_TAP_CFG   0x58
#define LSM6DS3_MD1_CFG   0x5E

#include <GxEPD2_4C.h>

// Instantiate E-paper class
// 4-color e-paper
GxEPD2_4C<GxEPD2_154c_GDEM0154F51H, GxEPD2_154c_GDEM0154F51H::HEIGHT> display(GxEPD2_154c_GDEM0154F51H(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)); // GDEM0154F51H 200x200, JD79660 (FPC-8101)

// IMU Instance
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// State tracking
volatile bool imuInterruptTriggered = false;
bool isFed = true;
uint32_t fedStartTime = 0;
const uint32_t FED_DURATION_MS = 60UL * 60UL * 1000UL; // 60 minutes
int currentRotation = 1;

// Interrupt Service Routine for the IMU
void imuISR() {
  imuInterruptTriggered = true;
}

// Function declarations (PlatformIO requires these if defined below setup/loop)
void updateRotation();
void drawScreen(const char* message);

void setup() {
  Serial.begin(115200);
  
  // Initialize Display
  display.init(115200, true, 2, false);
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(GxEPD_BLACK);
  
  // Initialize IMU
  if (myIMU.begin() != 0) {
    Serial.println("IMU Error");
  }

  // Configure IMU for motion/tilt detection routing to INT1
  pinMode(PIN_LSM6DS3TR_C_INT1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_LSM6DS3TR_C_INT1), imuISR, RISING);

  // Write IMU registers to trigger interrupt on significant motion/tilt
  myIMU.writeRegister(LSM6DS3_CTRL1_XL, 0x50); 
  myIMU.writeRegister(LSM6DS3_TAP_CFG, 0x0E);  
  myIMU.writeRegister(LSM6DS3_CTRL10_C, 0x38); 
  myIMU.writeRegister(LSM6DS3_MD1_CFG, 0x02);  

  // Initial Boot Sequence
  updateRotation();
  drawScreen("Animals Fed");
  fedStartTime = millis();
  isFed = true;
}

void loop() {
  if (imuInterruptTriggered) {
    imuInterruptTriggered = false;
    delay(100); 
    updateRotation();
    drawScreen("Animals Fed");
    fedStartTime = millis();
    isFed = true;
  }

  if (isFed && (millis() - fedStartTime >= FED_DURATION_MS)) {
    isFed = false;
    drawScreen("Animals Hungry");
  }

  delay(100); 
}

void updateRotation() {
  float x = myIMU.readFloatAccelX();
  float y = myIMU.readFloatAccelY();
  
  if (abs(x) > abs(y)) {
    if (x > 0.5) currentRotation = 1;       
    else if (x < -0.5) currentRotation = 3; 
  } else {
    if (y > 0.5) currentRotation = 2;       
    else if (y < -0.5) currentRotation = 0; 
  }
}

void drawScreen(const char* message) {
  display.setRotation(currentRotation);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    
    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(message, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby;
    
    display.setCursor(x, y);
    display.print(message);
  } while (display.nextPage());
}