#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "lcd.h"
#include <PNGdec.h> // This is for the PNG decoding functionality used in the weather display, ensure it's included at the top of this file for proper compilation and asset handling.
#include "../../common/img.h" // Required for BMP asset tracking pathways



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


PNG weather_png_decoder;  // This is the global PNG decoder instance that will be used for all weather icon rendering, ensuring that we can manage memory and callbacks cleanly across different function scopes without needing to reinitialize or create multiple decoder instances.


int target_draw_x = 0;
int target_draw_y = 0;



//Call only for simple testing for lcd functionality in isolation. Do NOT call this from your main setup() if you plan to use the switch-based mode switching, as it will interfere with the camera initialization and server task management in cnd.cpp. The cnd.cpp handler is designed to manage the LCD and camera states cleanly without conflicts, so using lcd_init() directly in setup() is not recommended for the intended dual-mode operation of your project.
void lcd_init() {
    digitalWrite(TFT_CS, LOW);
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.init(240, 240);
    tft.setRotation(2);
    tft.invertDisplay(true);
    
    // Allocate the heap memory for the off-screen frame buffer
    // if (canvas == NULL) {
    //     canvas = new GFXcanvas16(240, 240);
    // }


    //use PSRAM since png too large 112kb for memory of sram, and canvas also needs memory, so use psram to store canvas to save memory of sram
    if (psramFound()) {
        Serial.println("[Memory Allocation] PSRAM detected! Moving Canvas to PSRAM...");
        
        //use ps_malloc to allocate memory from psram, and assign it to the canvas buffer, so that the canvas can be stored in psram instead of sram, which can save memory of sram for other use
        uint16_t* psramCanvasBuffer = (uint16_t*) ps_malloc(240 * 240 * sizeof(uint16_t));
        
        if (psramCanvasBuffer != NULL) {
            // 💡 將外部 PSRAM 緩衝區傳入 Adafruit Canvas 建構子，完全不吃內部晶片 SRAM
            canvas = new GFXcanvas16(240, 240, psramCanvasBuffer);
            Serial.println("[Memory Allocation] Success! Canvas is now safely hosted on PSRAM.");
        } else {
            Serial.println("[Memory Allocation] ❌ PSRAM ext_malloc failed! Falling back to SRAM...");
            canvas = new GFXcanvas16(240, 240); 
        }
    } else {
        Serial.println("[Memory Allocation] ❌ No PSRAM found! Please check Arduino IDE Tool settings.");
        canvas = new GFXcanvas16(240, 240); // 如果沒開 PSRAM，維持原樣（會擠爆剩 16KB）
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











int pngCanvasDecoderCallback(PNGDRAW *pDraw) {
    uint16_t linePixelBuffer[240]; 
    uint8_t alphaMaskBuffer[240]; 
    
    weather_png_decoder.getLineAsRGB565(pDraw, linePixelBuffer, PNG_RGB565_BIG_ENDIAN, 0);
    
    if (pDraw->iHasAlpha) {
        weather_png_decoder.getAlphaMask(pDraw, alphaMaskBuffer, 128);
    }

    // 💡 FIXED: Your source images are exactly 48x48 pixels. 
    // Replaced the non-existent pDraw->iHeight with the explicit literal size 48.
    int sourceHeight = 48; 

    // Fixed-point scaling ratio logic calculations
    uint32_t scaleX = (pDraw->iWidth << 8) / 52;
    uint32_t scaleY = (sourceHeight << 8) / 52;

    // Calculate bounding row coordinates for mapping this 48px row onto a 52px target layout
    int targetY_Start = (pDraw->y * 52) / sourceHeight;
    int targetY_End = ((pDraw->y + 1) * 52) / sourceHeight;

    // Extrude out row matrices sequentially to maintain image coherence
    for (int outY = targetY_Start; outY < targetY_End; outY++) {
        int currentY = target_draw_y + outY;

        for (int outX = 0; outX < 52; outX++) {
            // Project the target grid step index back to find the source asset pixel coordinates
            int srcX = (outX * scaleX) >> 8;
            if (srcX >= pDraw->iWidth) srcX = pDraw->iWidth - 1;

            // Transparency mask layer safety filter evaluation
            if (pDraw->iHasAlpha && alphaMaskBuffer[srcX] == 0) {
                continue; 
            }

            int currentX = target_draw_x + outX;

            // Absolute screen size safety array boundary checks
            if (currentX >= 0 && currentX < 240 && currentY >= 0 && currentY < 240) {
                canvas->drawPixel(currentX, currentY, linePixelBuffer[srcX]);
            }
        }
    }
    return 1;
}


// ====================================================================
void drawWeatherDisplay(const char* city, int temp, const char* condition, int humidity, const char* hkUpdateTime) {
    if (canvas == NULL) return;

    digitalWrite(TFT_CS, LOW); 
    canvas->fillScreen(ST77XX_BLACK);

    // Decorative static circular layout lines
    canvas->drawCircle(120, 120, 118, 0x2104); 
    canvas->drawCircle(120, 120, 115, 0x10A2); 

    // --- City Name (Centered at Y=25) ---
    canvas->setTextSize(2);
    canvas->setTextColor(ST77XX_WHITE);
    int city_len = strlen(city);
    canvas->setCursor(120 - ((city_len * 12) / 2), 25);
    canvas->println(city);

    // --- Weather Condition Text (Centered at Y=50) ---
    canvas->setTextSize(2);
    canvas->setTextColor(0x9CF3); 
    int cond_len = strlen(condition);
    canvas->setCursor(120 - ((cond_len * 12) / 2), 50);
    canvas->println(condition);

    // --- Update Time (Centered at Y=75) ---
    canvas->setTextSize(1);
    canvas->setTextColor(0x3DFE);
    int time_len = strlen(hkUpdateTime);
    canvas->setCursor(120 - ((time_len * 6) / 2), 75);
    canvas->println(hkUpdateTime);

    // --- MATCH ALL COMPILATION CASES ---
    const uint8_t* selectedPngArray = cloudy_png; // Default fallback icon and size if no conditions match
    int selectedPngSize = sizeof(cloudy_png);
    String condStr = String(condition);
    condStr.trim();

    if (condStr == "Sunny") { selectedPngArray = sunny_png; selectedPngSize = sizeof(sunny_png); } 
    else if (condStr == "Sunny Periods") { selectedPngArray = sunny_periods_png; selectedPngSize = sizeof(sunny_periods_png); } 
    else if (condStr == "Sunny Intervals") { selectedPngArray = sunny_intervals_png; selectedPngSize = sizeof(sunny_intervals_png); } 
    else if (condStr == "Sunny Periods with showers") { selectedPngArray = sunny_periods_with_showers_png; selectedPngSize = sizeof(sunny_periods_with_showers_png); } 
    else if (condStr == "Sunny Intervals with showers") { selectedPngArray = sunny_intervals_with_showers_png; selectedPngSize = sizeof(sunny_intervals_with_showers_png); } 
    else if (condStr == "Cloudy") { selectedPngArray = cloudy_png; selectedPngSize = sizeof(cloudy_png); } 
    else if (condStr == "Overcast") { selectedPngArray = overcast_png; selectedPngSize = sizeof(overcast_png); } 
    else if (condStr == "Light Rain") { selectedPngArray = light_rain_png; selectedPngSize = sizeof(light_rain_png); } 
    else if (condStr == "Rain") { selectedPngArray = rain_png; selectedPngSize = sizeof(rain_png); } 
    else if (condStr == "Heavy Rain") { selectedPngArray = heavy_rain_png; selectedPngSize = sizeof(heavy_rain_png); } 
    else if (condStr == "Thunderstorm") { selectedPngArray = thunderstorm_png; selectedPngSize = sizeof(thunderstorm_png); } 
    else if (condStr == "Fine(night)") { selectedPngArray = fine_night_png; selectedPngSize = sizeof(fine_night_png); } 
    else if (condStr == "Main_Cloudy") { selectedPngArray = main_cloudy_png; selectedPngSize = sizeof(main_cloudy_png); } 
    else if (condStr == "Main_Fine") { selectedPngArray = main_fine_png; selectedPngSize = sizeof(main_fine_png); } 
    else if (condStr == "Windy") { selectedPngArray = windy_png; selectedPngSize = sizeof(windy_png); } 
    else if (condStr == "Dry") { selectedPngArray = dry_png; selectedPngSize = sizeof(dry_png); } 
    else if (condStr == "Humid") { selectedPngArray = humid_png; selectedPngSize = sizeof(humid_png); } 
    else if (condStr == "Fog") { selectedPngArray = fog_png; selectedPngSize = sizeof(fog_png); } 
    else if (condStr == "Mist") { selectedPngArray = mist_png; selectedPngSize = sizeof(mist_png); } 
    else if (condStr == "Haze") { selectedPngArray = haze_png; selectedPngSize = sizeof(haze_png); } 
    else if (condStr == "Hot") { selectedPngArray = hot_png; selectedPngSize = sizeof(hot_png); } 
    else if (condStr == "Warm") { selectedPngArray = warm_png; selectedPngSize = sizeof(warm_png); } 
    else if (condStr == "Cool") { selectedPngArray = cool_png; selectedPngSize = sizeof(cool_png); } 
    else if (condStr == "Cold") { selectedPngArray = cold_png; selectedPngSize = sizeof(cold_png); }

    // Centering adjusted for Python script's 48x48 output: 120 - (48 / 2) = 96
    target_draw_x = 96; 
    target_draw_y = 90; 

    int result = weather_png_decoder.openRAM((uint8_t *)selectedPngArray, selectedPngSize, pngCanvasDecoderCallback);
    if (result == PNG_SUCCESS) {
        weather_png_decoder.decode(NULL, 0);
        weather_png_decoder.close();
    } else {
        Serial.printf("PNG Decompress Error: %d\n", result);
    }

    // --- Temperature Readout (Centered at Y=145 for size 4 font) ---
    canvas->setTextSize(4);
    canvas->setTextColor(0xFCE0); 
    
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%d C", temp); 
    int temp_len = strlen(temp_str);
    int temp_x = 120 - ((temp_len * 24) / 2); 
    canvas->setCursor(temp_x, 145); 
    canvas->print(temp_str);
    
    int degree_x = temp_x + ((temp_len - 1) * 24) - 8;
    canvas->drawCircle(degree_x, 145, 3, 0xFCE0);

    // --- Humidity Readout (Centered at Y=190) ---
    canvas->setTextSize(2);
    canvas->setTextColor(0x3DFE); 
    
    char hum_str[16];
    snprintf(hum_str, sizeof(hum_str), "Hum: %d%%", humidity);
    int hum_len = strlen(hum_str);
    canvas->setCursor(120 - ((hum_len * 12) / 2), 190);
    canvas->print(hum_str);

    // Push the final compound image down to hardware instantly via PSRAM
    tft.drawRGBBitmap(0, 0, canvas->getBuffer(), 240, 240);
    digitalWrite(TFT_BL, HIGH); 
}


//Home screen menu selection global variable (0 to 3 index for the 4 widgets)
// Global tracker variable to manage which widget is currently highlighted/selected (0 to 3)
int currentMenuSelection = 0; 


// ====================================================================
void onCameraWidgetClick() {
    // [Template] 
    Serial.println("[Menu Action] Camera Widget Activated.");
}

void onWeatherWidgetClick() {
    // [Template] 
    Serial.println("[Menu Action] Weather Widget Activated.");
}

void onIoTWidgetClick() {
    // [Template] 
    Serial.println("[Menu Action] IoT Widget Activated.");
}

void onSettingsWidgetClick() {
    // [Template] 
    Serial.println("[Menu Action] Settings Widget Activated.");
}

// ====================================================================
// 🎨 HELPER FUNCTION: DRAW A SINGLE WIDGET CELL WITH HIGHLIGHT FRAME
// ====================================================================
void drawMenuWidget(int id, const char* label, uint16_t iconColor, int centerX, int centerY) {
    // Determine boundary coordinates based on widget ID mapping
    int startX = (id % 2) * 120;
    int startY = (id / 2) * 120;

    // 💡 HIGHLIGHT FILTER: If this specific widget is selected, draw a vibrant thick border
    if (id == currentMenuSelection) {
        canvas->drawRect(startX + 2, startY + 2, 116, 116, 0xFCE0); // Gold/Orange selection box
        canvas->drawRect(startX + 3, startY + 3, 114, 114, 0xFCE0); // Double-thickness border
    } else {
        canvas->drawRect(startX + 4, startY + 4, 112, 112, 0x2104); // Faint decorative dark gray grid frame
    }

    // 💡 PLACEHOLDER GRAPHIC: Draw a decorative colored geometric block as a proxy icon
    // (This ensures your layout looks professional before you convert your binary PNG arrays)
    canvas->fillRect(centerX - 16, centerY - 25, 32, 32, iconColor);
    canvas->drawRect(centerX - 16, centerY - 25, 32, 32, ST77XX_WHITE);

    // --- Widget Label Text (Auto-Centered horizontally below the icon space) ---
    canvas->setTextSize(1);
    canvas->setTextColor(ST77XX_WHITE);
    int label_len = strlen(label);
    int text_x = centerX - ((label_len * 6) / 2); // Font size 1 characters are exactly 6px wide
    canvas->setCursor(text_x, centerY + 18);
    canvas->print(label);
}

// ====================================================================
// 🚀 MAIN MAIN-HOME RENDERING INTERFACE
// ====================================================================
void drawHomeMenuDisplay() {
    if (canvas == NULL) return;

    digitalWrite(TFT_CS, LOW); 
    canvas->fillScreen(ST77XX_BLACK); // Clear layout canvas space using premium PSRAM blocks

    // Draw grid separation lines to anchor the 2x2 dashboard visual style
    canvas->drawFastHLine(0, 120, 240, 0x10A2); // Horizontal cross splitter line
    canvas->drawFastVLine(120, 0, 240, 0x10A2); // Vertical cross splitter line

    // Render Widget 0: Camera (Top-Left, Center at 60, 60)
    drawMenuWidget(0, "CAMERA", 0xF800, 60, 60); // Red thematic indicator color

    // Render Widget 1: Weather (Top-Right, Center at 180, 60)
    drawMenuWidget(1, "WEATHER", 0x3DFE, 180, 60); // Cyan thematic indicator color

    // Render Widget 2: IOT Matrix (Bottom-Left, Center at 60, 180)
    drawMenuWidget(2, "IOT NODE", 0x07E0, 60, 180); // Green thematic indicator color

    // Render Widget 3: Device Settings (Bottom-Right, Center at 180, 180)
    drawMenuWidget(3, "SETTINGS", 0x9CF3, 180, 180); // Purple/Gray thematic indicator color

    // Blast the completed 2x2 matrix dashboard composition layer straight onto display hardware
    tft.drawRGBBitmap(0, 0, canvas->getBuffer(), 240, 240);
    digitalWrite(TFT_BL, HIGH); 
}

// ====================================================================
// 🎮 MENU NAVIGATION CONTROL CONTROLLER
// ====================================================================
void navigateHomeMenu(int direction) {
    // Direction: 1 = Next Item, -1 = Previous Item
    currentMenuSelection += direction;
    
    // Boundary roll-over defense loops (0 to 3 index space constraints)
    if (currentMenuSelection > 3) currentMenuSelection = 0;
    if (currentMenuSelection < 0) currentMenuSelection = 3;
    
    // Instantly update layout state graphics inside your PSRAM array
    drawHomeMenuDisplay();
}

void executeSelectedMenuWidget() {
    // Route execution matrix cleanly based on verified menu state locks
    switch (currentMenuSelection) {
        case 0: onCameraWidgetClick(); break;
        case 1: onWeatherWidgetClick(); break;
        case 2: onIoTWidgetClick();    break;
        case 3: onSettingsWidgetClick(); break;
    }
}

