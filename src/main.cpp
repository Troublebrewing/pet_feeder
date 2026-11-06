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

// Interrupt Service Routine for the IMU
void imuISR() {
  imuInterruptTriggered = true;
}

// Function declarations (PlatformIO requires these if defined below setup/loop)
void updateRotation();
void drawScreen(const char* message);

// Drawing helpers
void thickLine(Adafruit_GFX &g, int x0, int y0,
               int x1, int y1, int thickness) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  int steps = max(abs(dx), abs(dy));

  for (int i = 0; i <= steps; i++) {
    int x = x0 + (steps ? dx * i / steps : 0);
    int y = y0 + (steps ? dy * i / steps : 0);
    g.fillCircle(x, y, thickness / 2, BLACK);
  }
}

// Quadratic Bezier curve
void curve(Adafruit_GFX &g,
           int x0, int y0,
           int cx, int cy,
           int x1, int y1,
           int thickness = 6) {
  int px = x0, py = y0;

  for (int i = 1; i <= 40; i++) {
    float t = i / 40.0;
    float u = 1.0 - t;

    int x = u*u*x0 + 2*u*t*cx + t*t*x1;
    int y = u*u*y0 + 2*u*t*cy + t*t*y1;

    thickLine(g, px, py, x, y, thickness);
    px = x;
    py = y;
  }
}

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
  /*"M707 84 C753 94 800 121 846 155"
  "C801 181 757 216 721 253"
  "C697 204 697 133 707 84 Z"     
  "M1341 84 C1295 94 1248 121 1202 155"
  "C1247 181 1291 216 1327 253"
  "C1351 204 1351 133 1341 84 Z";

  "M715 75 "
  "C758 83 803 116 837 159 "
  "C801 181 757 216 730 252 "
  "C705 200 704 135 715 75 Z "
  "M1339 70 "
  "C1285 80 1238 106 1202 155 "
  "C1247 181 1291 216 1327 253 "
  "C1351 204 1351 133 1339 70 Z";*/

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
  display.setRotation(currentRotation);
  display.setFullWindow();
  display.firstPage();
  do {
    drawHappyCatFace(display);
  } while (display.nextPage());
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
    //drawScreen("Animals Hungry");
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