#include <Arduino.h> // Required for PlatformIO
#include <SPI.h>
#include <Wire.h>
#include <GxEPD2_BW.h> 
#include <GxEPD2_4C.h>
#include <GxEPD2_EPD.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "LSM6DS3.h"
#include <gfxDraw.h>

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
#define LSM6DS3_INT1_CTRL 0x0D

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

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

void imuISR();
void updateRotation();


void drawHappyDogFace(Adafruit_GFX &g) {
  g.fillScreen(WHITE);


}

static const char catFacePath[] =
  //outline
  "M686 30"
  "C660 94 655 213 700 296"
  "C676 343 668 397 684 447"
  "C724 553 857 642 1024 648"
  "C1191 642 1324 553 1364 447"
  "C1380 397 1372 343 1348 296"
  "C1393 213 1388 94 1362 30"
  "C1300 31 1226 74 1162 139"
  "C1084 108 964 108 886 139"
  "C822 74 748 31 686 30 Z";

static const char catEarsPath[] =
  //inner ears
  "M 718 86 C 762 97 793 121 822 165 C 801 181 757 216 738 243 C 714 193 704 135 718 86 Z M 1331 84 C 1284 99 1240 127 1211 164 C 1247 181 1291 216 1313 246 C 1332 196 1337 129 1331 84 Z";

static const char catSadEyesPath[] =  
  //eyes
  "M812 381 C853 381 891 361 916 338"
  "C906 374 881 400 850 401"
  "C835 401 822 394 812 381 Z"
  "M1236 381 C1195 381 1157 361 1132 338"
  "C1142 374 1167 400 1198 401"
  "C1213 401 1226 394 1236 381 Z";

static const char catNosePath[] =
  //Nose
  "M990 414 C992 397 1056 397 1058 414"
  "C1055 439 1040 453 1024 461"
  "C1008 453 993 439 990 414 Z";  

static const char catSadMouthPath[] =
  //Mouth
  "M1024 459 L1024 484"
  "M1024 484 C1003 484 984 496 970 511"
  "M1024 484 C1045 484 1064 496 1078 511";

static const char catWhiskersPath[] =
  //whiskers
  "M750 380 "
  "L596 356 "
  "M748 432 "
  "L596 438 "
  "M750 480 "
  "L618 516"
  "M1298 380 "
  "L1452 356 "
  "M1300 432 "
  "L1452 438 "
  "M1298 480 "
  "L1430 516";

static const char catHappyEyesPath[] =  
  //eyes
  //closed eyes
  "M325 345 C327 310 351 286 382 286 "
  "C414 286 438 310 440 345"
  "M560 345 C562 310 586 286 618 286 "
  "C649 286 673 310 675 345";

static const char catNosePath2[] =
  //Nose
  "M478 365 C480 351 520 351 522 365 "
  "C520 388 508 404 500 408 "
  "C492 404 480 388 478 365 Z"; //fill="#111111" stroke="#111111"
  
static const char catHappyMouthPath[] =
  //Mouth
  "M500 408 C500 447 476 466 450 466 C426 466 410 449 410 427"
  "M500 408 C500 447 524 466 550 466 C574 466 590 449 590 427";

void drawHappyCatFace(Adafruit_GFX &g) {
  g.fillScreen(GxEPD_WHITE);
  gfxDraw::pathByText(
      catFacePath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      [&](int16_t x, int16_t y) { g.drawPixel(x, y, GxEPD_YELLOW); });

  gfxDraw::pathByText(
      catEarsPath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      [&](int16_t x, int16_t y) { g.drawPixel(x, y, GxEPD_BLACK); });

  gfxDraw::pathByText(
      catHappyEyesPath, -10, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      nullptr);

  gfxDraw::pathByText(
      catNosePath2, -10, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      [&](int16_t x, int16_t y) { g.drawPixel(x, y, GxEPD_BLACK); });
  
  gfxDraw::pathByText(
      catHappyMouthPath, -10, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      nullptr);

  gfxDraw::pathByText(
      catWhiskersPath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      nullptr);
}

void drawSadCatFace(Adafruit_GFX &g) { 
  g.fillScreen(GxEPD_WHITE);
  gfxDraw::pathByText(
      catFacePath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      [&](int16_t x, int16_t y) { g.drawPixel(x, y, GxEPD_YELLOW); });

  gfxDraw::pathByText(
      catEarsPath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      [&](int16_t x, int16_t y) { g.drawPixel(x, y, GxEPD_BLACK); });

  gfxDraw::pathByText(
      catSadEyesPath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      [&](int16_t x, int16_t y) { g.drawPixel(x, y, GxEPD_BLACK); });

  gfxDraw::pathByText(
      catNosePath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      [&](int16_t x, int16_t y) { g.drawPixel(x, y, GxEPD_BLACK); });

  gfxDraw::pathByText(
      catSadMouthPath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      nullptr);

  gfxDraw::pathByText(
      catWhiskersPath, -125, 30, 22,
      [&](int16_t x, int16_t y) {
        if (y != POINT_BREAK_Y && y != POINT_INVALID_Y) {
          g.fillCircle(x, y, 3, GxEPD_BLACK);
        }
      },
      nullptr);
}


void setup() {
  Serial.begin(115200);
  Serial.println("Pet feeder starting");
  
  // Initialize Display
  display.init(115200, true, 2, false);
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(GxEPD_BLACK);
  
  // Initialize IMU
  if (myIMU.begin() != 0) {
    Serial.println("IMU Error");
  }

  //gyroscope can be powered off to save power, we only need the accelerometer for orientation detection
  myIMU.settings.accelEnabled = 1; // Enable accelerometer
  myIMU.settings.gyroEnabled = 0; // Disable gyroscope

  //config INT1_CTRL register to route significant motion/tilt interrupt to INT1
  //

  // Configure IMU for motion/tilt detection routing to INT1
  pinMode(PIN_LSM6DS3TR_C_INT1, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_LSM6DS3TR_C_INT1), imuISR, RISING);

  // Write IMU registers to trigger interrupt on significant motion/tilt
  // Enable accelerometer at 26 Hz, ±2g
  myIMU.writeRegister(LSM6DS3_CTRL1_XL, 0x60); //

  myIMU.writeRegister(LSM6DS3_TAP_CFG, 0x80); // Enable significant motion detection
#define LSM6DS3_TAP_THS_6D 0x59
  myIMU.writeRegister(LSM6DS3_TAP_THS_6D, 0x40); //60 degree threshold 0x60 for 50 degree 0x20 for 70 0x00 for 80 degree
#define LSM6DS3_CTRL8_XL 0x17
  myIMU.writeRegister(LSM6DS3_CTRL8_XL, 0x01); //enable LPF2 for 6D detection
  myIMU.writeRegister(LSM6DS3_MD1_CFG, 0x04); // Route significant motion interrupt to INT1

  // Enable embedded function Bank A
  #define LSM6DS3_FUNC_CFG_ACCESS 0x01
  //myIMU.writeRegister(LSM6DS3_FUNC_CFG_ACCESS, 0x01);
  //configure significant motion detection threshold
  #define LSM6DS3_SM_THS 0x13
  //myIMU.writeRegister(LSM6DS3_SM_THS, 0x06); //default value 
  // disable embedded function Bank A
  //myIMU.writeRegister(LSM6DS3_FUNC_CFG_ACCESS, 0x00);

  // Enable embedded functions + significant motion
  //myIMU.writeRegister(LSM6DS3_CTRL10_C, 0x05); 
  
  // Route significant motion interrupt to INT1 
  //myIMU.writeRegister(LSM6DS3_INT1_CTRL, 0x40);
  
  

  // Initial Boot Sequence
  updateRotation();
  display.setRotation(currentRotation);
  display.setFullWindow();
  display.firstPage();
  do {
    drawHappyCatFace(display);
  } while (display.nextPage());
  fedStartTime = millis();
  isFed = true;
  Serial.println("Pet feeder ready");
}




void loop() {
  

  /*if (isFed && (millis() - fedStartTime >= FED_DURATION_MS)) {
    isFed = false;
    //drawScreen("Animals Hungry");
  }*/

  delay(1000); 
  updateRotation();
  if (imuInterruptTriggered) {
    imuInterruptTriggered = false;
    delay(100); 
    //updateRotation();
    //if(isFed){

    //}
    //fedStartTime = millis();
    //isFed = true;
    display.setRotation(currentRotation);
    display.setFullWindow();
    display.firstPage();
    do {
      drawHappyCatFace(display);
    } while (display.nextPage());
  }
  Serial.print("Current Rotation: ");
  Serial.println(currentRotation);
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


// Interrupt Service Routine for the IMU
void imuISR() {
  imuInterruptTriggered = true;
  Serial.print("IMU Interrupt Triggered.");

  //updateRotation();
  
  //Serial.print("Current Rotation: ");
  //Serial.println(currentRotation);

  /*display.setRotation(currentRotation);
  display.setFullWindow();
  display.firstPage();
  do {
    drawHappyCatFace(display);
  } while (display.nextPage());*/

}