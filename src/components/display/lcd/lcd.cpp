#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "lcd.h"

#define TFT_SCLK 19
#define TFT_MOSI 20
#define TFT_RST 21
#define TFT_DC 14
#define TFT_CS 2
#define TFT_BL 1

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

// FIX 1: Create a hidden 16-bit Canvas buffer matching your 240x240 display area
GFXcanvas16 *canvas;


#define SHARINGAN_RED 0xD800 
#define RINNEGAN_PURPLE 0xBDFD 
#define RINNEGAN_LINE 0x2010 

float share_angle = 0.0;
float rinne_angle = 0.0;


//Call only for simple testing for lcd functionality in isolation. Do NOT call this from your main setup() if you plan to use the switch-based mode switching, as it will interfere with the camera initialization and server task management in cnd.cpp. The cnd.cpp handler is designed to manage the LCD and camera states cleanly without conflicts, so using lcd_init() directly in setup() is not recommended for the intended dual-mode operation of your project.
void lcd_init() {
    digitalWrite(TFT_CS, LOW);
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.init(240, 240);
    tft.setRotation(2);
    tft.invertDisplay(true);
    
    // Allocate the heap memory for the off-screen frame buffer
    if (canvas == NULL) {
        canvas = new GFXcanvas16(240, 240);
    }
    
    canvas->fillScreen(ST77XX_BLACK);
    tft.setSPISpeed(16000000); 
    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.println("Goouuu S3 Booted!");
    delay(1000);
    tft.fillScreen(ST77XX_BLACK);
}

void lcd_deinit() {
    // 1. FORCE SHUTDOWN LCD CONTROLLER HARDWARE
    tft.writeCommand(ST77XX_SLPIN); // Put display into SLEEP (stops all LCD logic)
    delay(10);
    tft.writeCommand(ST77XX_DISPOFF); // Turn off display output
    delay(10);

    // 2. KILL SPI BUS COMPLETELY (stops all clock/data signals)
    SPI.end();
    SPI.endTransaction();

    // 3. RESET ALL LCD PINS TO HIGH-Z INPUT (SAFE, NO OUTPUT)
    // THIS IS THE MOST IMPORTANT PART FOR PIN ISOLATION
    pinMode(TFT_SCLK, INPUT);
    pinMode(TFT_MOSI, INPUT);
    pinMode(TFT_RST,  INPUT);
    pinMode(TFT_DC,   INPUT);
    pinMode(TFT_CS,   INPUT);
    pinMode(TFT_BL,   INPUT);

    // 4. FREE LCD CANVAS MEMORY (eliminate dangling pointers)
    if (canvas != nullptr) {
        delete canvas;
        canvas = nullptr;
    }

    // 5. SAFE CS STATE
    digitalWrite(TFT_CS, HIGH);
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, LOW);
    delay(10);
    digitalWrite(TFT_RST, HIGH);
    delay(10);
    pinMode(TFT_RST, INPUT);

    Serial.println("✅ LCD FULLY DEINITIALIZED - SPI & PINS ISOLATED");
}

// --- RENDERING REDIRECTED TO THE HIDDEN CANVAS OBJECT ('canvas->') ---

void drawSharingan(int cx, int cy, int size, float angle) {
    int r = size / 2;
    canvas->fillCircle(cx, cy, r, ST77XX_BLACK);
    canvas->fillCircle(cx, cy, r - 3, SHARINGAN_RED);
    canvas->drawCircle(cx, cy, r - 15, ST77XX_BLACK);
    canvas->drawCircle(cx, cy, r - 16, ST77XX_BLACK);
    canvas->fillCircle(cx, cy, r / 4, ST77XX_BLACK);

    // Draw the three comma-shaped tomoe evenly spaced around the center
    for (int i = 0; i < 3; i++) {
        float a = angle + (i * 2.0944);
        int tx = cx + (r - 16) * cos(a);
        int ty = cy + (r - 16) * sin(a);
        canvas->fillCircle(tx, ty, 6, ST77XX_BLACK);

        float tail_angle = a - 0.35;
        int kx = cx + (r - 15) * cos(tail_angle);
        int ky = cy + (r - 15) * sin(tail_angle);
        canvas->fillCircle(kx, ky, 3, ST77XX_BLACK);

        float tail_angle2 = a - 0.18;
        int kx2 = cx + (r - 15) * cos(tail_angle2);
        int ky2 = cy + (r - 15) * sin(tail_angle2);
        canvas->fillCircle(kx2, ky2, 4, ST77XX_BLACK);
    }
}

void drawRinnegan(int cx, int cy, int size, float angle) {
    int r = size / 2;
    canvas->fillCircle(cx, cy, r, ST77XX_BLACK);
    canvas->fillCircle(cx, cy, r - 3, RINNEGAN_PURPLE);

    for (int i = 1; i <= 4; i++) {
        int ring_r = r - (i * 9);
        if (ring_r > 4) {
            canvas->drawCircle(cx, cy, ring_r, RINNEGAN_LINE);
            canvas->drawCircle(cx, cy, ring_r - 1, RINNEGAN_LINE);
        }
    }
    canvas->fillCircle(cx, cy, 5, RINNEGAN_LINE);

    for (int i = 0; i < 3; i++) {
        float a = angle + (i * 2.0944);
        int ring_select = r - 18;
        int tx = cx + ring_select * cos(a);
        int ty = cy + ring_select * sin(a);
        canvas->fillCircle(tx, ty, 3, RINNEGAN_LINE);
    }
}

void animateEyes() {
    // Guard clause to make sure canvas initialization exists
    if (canvas == NULL) return;

    digitalWrite(TFT_CS, LOW); 

    // 1. Wipe the HIDDEN canvas background cleanly out of sight
    canvas->fillScreen(ST77XX_BLACK);

    // 2. Compute and compile shapes onto the memory matrix silently
    drawSharingan(62, 120, 90, share_angle);
    drawRinnegan(178, 120, 90, rinne_angle);

    // 3. PUSH ALL AT ONCE: Blit the pre-assembled layout onto physical hardware
    tft.drawRGBBitmap(0, 0, canvas->getBuffer(), 240, 240);

    // 4. Update coordinates for the next loop run cycle
    share_angle += 0.1;
    rinne_angle -= 0.1;

    digitalWrite(TFT_BL, HIGH); 
}
