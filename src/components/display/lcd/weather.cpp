#include "weather.h"
#include <Adafruit_ST7789.h> 

extern Adafruit_ST7789 tft; 

char hkUpdateTime[32] = "N/A"; 
char hkTemperature[16] = "--"; 
char hkHumidity[16]    = "--"; 
char hkCondition[32]   = "Unknown"; 

unsigned long lastWeatherCheck = 0;
const unsigned long weatherInterval = 600000; 

// CRITICAL: These MUST be global variables outside the function
// If they are inside the function, they reset every frame and cause flashing!
String lastDisplayedTime = "";
String lastDisplayedWeather = "";

void weather_page_lcd() {
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

    String timeStr = getHKTime(); 
    String weatherStr = "HK: " + String(hkTemperature) + " | " + String(hkHumidity); 

    // This check stops the flash by skipping tft.fillScreen unless data changes
    if (timeStr != lastDisplayedTime || weatherStr != lastDisplayedWeather) {
        lastDisplayedTime = timeStr;
        lastDisplayedWeather = weatherStr;

        tft.fillScreen(ST77XX_BLACK); 
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(2);
        
        tft.setCursor(0, 0); 
        tft.print("time: "); 
        tft.print(timeStr); 
        
        tft.setCursor(0, 20); 
        tft.print("weather: "); 
        tft.print(weatherStr); 
    }
}
