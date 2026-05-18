#include "weather.h"
#include "../../common/img.h" // Required for BMP asset tracking pathways
#include <Adafruit_ST7789.h> 
#include <lvgl.h>

extern Adafruit_ST7789 tft; 

// Make sure your global canvas object can be accessed inside this scope context
extern GFXcanvas16* canvas; 

char hkUpdateTime[32] = "N/A"; 
char hkTemperature[16] = "--"; 
char hkHumidity[16]    = "--"; 
char hkCondition[32]   = "Unknown"; 

unsigned long lastWeatherCheck = 0;
const unsigned long weatherInterval = 600000; // 10 minutes interval

String lastDisplayedTime = "";
String lastDisplayedWeather = "";

// Declaration of your external canvas rendering engine function block
void drawWeatherDisplay(const char* city, int temp, const char* condition, int humidity);

void weather_page_lcd() {
    // 1. Fetch live API updates from the Hong Kong Observatory every 10 minutes
    if (millis() - lastWeatherCheck >= weatherInterval || lastWeatherCheck == 0) {
        lastWeatherCheck = millis();
        
        HKWeather weather = getHKWeather(); 
        if (weather.valid) { 
            weather.temperature.toCharArray(hkTemperature, sizeof(hkTemperature));
            weather.humidity.toCharArray(hkHumidity, sizeof(hkHumidity));
            weather.condition.toCharArray(hkCondition, sizeof(hkCondition));
            weather.updateTime.toCharArray(hkUpdateTime, sizeof(hkUpdateTime));
        }
    }

    // 2. Fetch real-time time calculations from your non-blocking local modifier loop
    String timeStr = getHKTime(); 
    String weatherStr = "HK: " + String(hkTemperature) + " | " + String(hkHumidity) + " | " + String(hkCondition); 

    // 3. Render everything onto your canvas memory array if changes are detected
    if (timeStr != lastDisplayedTime || weatherStr != lastDisplayedWeather) {
        lastDisplayedTime = timeStr;
        lastDisplayedWeather = weatherStr;

        // Strip unit suffix values (like "°C" or "%") to convert raw numerical strings safely
        int intTemp = atoi(hkTemperature);
        int intHumidity = atoi(hkHumidity);

        // Render data elements instantly via your off-screen drawing engine layout
        // This takes care of clearing backgrounds, mapping PNG icons, and running text buffers
        drawWeatherDisplay("Yuen Long", intTemp, hkCondition, intHumidity);
    }
}
