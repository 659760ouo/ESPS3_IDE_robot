#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <PNGdec.h> // Make sure to install PNGdec in your Library Manager

#define TFT_SCLK 19
#define TFT_MOSI 20
#define TFT_RST 21
#define TFT_DC 14
#define TFT_CS 2
#define TFT_BL 1

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
PNG png; 

// --- ISOLATED TEST ICON (3x3 pixels: Red, Green, Blue dots) ---
const uint8_t test_png_icon[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03,
  0x08, 0x06, 0x00, 0x00, 0x00, 0x56, 0x28, 0xb5, 0xbf, 0x00, 0x00, 0x00,
  0x1a, 0x49, 0x44, 0x41, 0x54, 0x08, 0x99, 0x63, 0xfc, 0xcf, 0xc0, 0xc0,
  0xcc, 0xc0, 0xc4, 0xc0, 0xc8, 0xc0, 0x00, 0x23, 0x30, 0x30, 0x63, 0x00,
  0x91, 0x20, 0x01, 0x27, 0x18, 0x94, 0x65, 0x90, 0x00, 0x00, 0x00, 0x00,
  0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};

int test_x = 104; // Rendering X coordinate
int test_y = 85;  // Rendering Y coordinate

// Direct hardware screen plot callback function
int pngTestCallback(PNGDRAW *pDraw) {
    uint16_t pixelBuffer[240]; // Local row buffer (fits screen width)
    png.getLineAsRGB565(pDraw, pixelBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    
    for (int x = 0; x < pDraw->iWidth; x++) {
        tft.drawPixel(test_x + x, test_y + pDraw->y, pixelBuffer[x]);
    }
    return 1;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initializing hardware pins
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_CS, LOW);
    
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.init(240, 240);
    tft.setRotation(2);
    tft.invertDisplay(true);
    tft.setSPISpeed(16000000); 
    
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.println("Goouuu S3 Booted!");
    digitalWrite(TFT_BL, HIGH);
    delay(1000);
    
    // --- DIRECT PNG HARDWARE DRAW TEST ---
    int result = png.openRAM((uint8_t *)test_png_icon, sizeof(test_png_icon), pngTestCallback);
    if (result == PNG_SUCCESS) {
        png.decode(NULL, 0);
        png.close();
        Serial.println("PNG Test Rendered successfully!");
    } else {
        Serial.printf("PNG Test Failed! Error: %d\n", result);
    }
}

void loop() {
    // Keep it empty for testing
}
