#include "functions.h"
#include <WiFi.h> 
#include <WiFiClientSecure.h> 
#include <HTTPClient.h> 
#include <Arduino.h>
#include <ArduinoJson.h> 
#include <time.h>

// ==========================================
// CRITICAL: GLOBAL VARIABLES FOR CUSTOM REAL-TIME CLOCK
// ==========================================
struct tm myClock = {0, 0, 0, 1, 0, 126, 0, 0, 0}; // struct tm format: sec, min, hour, mday, mon, year (since 1900), wday, yday, isdst
bool clockSynced = false;                         // Flags when HKO API initializes time
unsigned long lastClockUpdate = 0;                // High-precision millisecond modifier anchor

const char* weather_typeC = "rhrread"; // "rhrread" for current forecast
String base_url = "https://data.weather.gov.hk/weatherAPI/opendata/weather.php?dataType=";
String HKO_WEATHER_URL = base_url + String(weather_typeC) + "&lang=en";

HKWeather getHKWeather() {
    HKWeather weather; 
    weather.valid = false;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Cannot fetch weather data.");
        return weather;
    }

    WiFiClientSecure client; 
    client.setInsecure(); // Required on ESP32 devices to bypass root CA verification layout
    
    HTTPClient http; 

    if (!http.begin(client, HKO_WEATHER_URL)) {
        Serial.println("Failed to initialize HTTP connection");
        return weather;
    }
    
    Serial.println("Fetching weather data from HKO...");

    int code = http.GET();
    if (code == HTTP_CODE_OK) {

        int timeout = 3000; // 3 seconds timeout loop to let secure connection buffer catch up
        while (http.getStream().available() == 0 && timeout > 0) {
            delay(10);
            timeout -= 10;
        }

        String payload = http.getString();
        Serial.println("Weather data received:");
        Serial.println(payload); 
        
        DynamicJsonDocument doc(16384); // Allocate a larger buffer for the expected JSON payload size
        DeserializationError err = deserializeJson(doc, payload);
        
        if (!err) {
            weather.valid = true; 

            // ==========================================
            // API CLOCK OVERRIDE CAPTURE
            // ==========================================
            if (doc.containsKey("updateTime")) {
                String uTime = doc["updateTime"].as<String>();
                weather.updateTime = uTime;

                // Parses standard HKO ISO format: "YYYY-MM-DDTHH:MM:SS+08:00"
                int y = 0, m = 0, d = 0, hr = 0, min = 0, sec = 0;
                if (sscanf(uTime.c_str(), "%d-%d-%dT%d:%d:%d", &y, &m, &d, &hr, &min, &sec) == 6) {
                    
                    myClock.tm_year = y - 1900; // tm format structural rules (years since 1900)
                    myClock.tm_mon = m - 1;     // tm format structural rules (months 0-11)
                    myClock.tm_mday = d;
                    myClock.tm_hour = hr;
                    myClock.tm_min = min;
                    myClock.tm_sec = sec;
                    
                    clockSynced = true;         
                    lastClockUpdate = millis(); // Snap modifier tracking to current MCU execution state
                    Serial.println(">> Local Clock Successfully Synced with HKO Data Engine! <<");
                }
            }

            // Target station filter name "Yuen Long Park"
            JsonArray tempData = doc["temperature"]["data"];
            for (JsonObject item : tempData) {
                if (item["place"].as<String>() == "Yuen Long Park") {
                    weather.temperature = String(item["value"].as<int>()) + "°C";
                    break;
                }
            }

            // Target station filter name "Hong Kong Observatory"
            JsonArray humiData = doc["humidity"]["data"];
            for (JsonObject item : humiData) {
                if (item["place"].as<String>() == "Hong Kong Observatory") {
                    weather.humidity = String(item["value"].as<int>()) + "%";
                    break;
                }
            }

            // Read array index cleanly via HKO spec sheet map
            if (doc.containsKey("icon") && doc["icon"].is<JsonArray>()) {
                JsonArray iconArray = doc["icon"].as<JsonArray>();
                if (iconArray.size() > 0) {
                    int iconCode = iconArray[0].as<int>(); 
                    
                    switch (iconCode) {
                        case 50: weather.condition = "Sunny"; break;
                        case 51: weather.condition = "Sunny Periods"; break;
                        case 52: weather.condition = "Sunny Intervals"; break;
                        case 53: weather.condition = "Sunny Periods with showers"; break;
                        case 54: weather.condition = "Sunny Intervals with showers"; break;
                        
                        case 60: weather.condition = "Cloudy"; break;
                        case 61: weather.condition = "Overcast"; break;
                        case 62: weather.condition = "Light Rain"; break;
                        case 63: weather.condition = "Rain"; break;
                        case 64: weather.condition = "Heavy Rain"; break;
                        case 65: weather.condition = "Thunderstorm"; break;

                        case 70: weather.condition = "Fine(night)"; break;
                        case 71: weather.condition = "Fine(night)"; break;
                        case 72: weather.condition = "Fine(night)"; break;
                        case 73: weather.condition = "Fine(night)"; break;
                        case 74: weather.condition = "Fine(night)"; break;
                        case 75: weather.condition = "Fine(night)"; break;

                        case 76: weather.condition = "Main_Cloudy"; break;
                        case 77: weather.condition = "Main_Fine"; break;
                        case 80: weather.condition = "Windy"; break;
                        case 81: weather.condition = "Dry"; break;
                        case 82: weather.condition = "Humid"; break;
                        case 83: weather.condition = "Fog"; break;
                        case 84: weather.condition = "Mist"; break;
                        case 85: weather.condition = "Haze"; break;

                        case 90: weather.condition = "Hot"; break;
                        case 91: weather.condition = "Warm"; break;
                        case 92: weather.condition = "Cool"; break;
                        case 93: weather.condition = "Cold"; break;
                        
                        default: weather.condition = "Cloudy"; break;
                    }
                }
            }
        } else {
            Serial.print("JSON Deserialize fail: ");
            Serial.println(err.c_str());
        }
    } else {
        Serial.printf("HTTP GET failed, error code: %d\n", code);
    }
    
    http.end(); 
    Serial.printf("Weather: %s, Temp: %s, Humidity: %s\n", weather.condition.c_str(), weather.temperature.c_str(), weather.humidity.c_str());
    return weather;
}

// ==========================================
// HIGH-SPEED REAL-TIME MODIFIER CLOCK ENGINE (0ms DELAY)
// ==========================================
String getHKTime() {
    static char timeBuffer[128] = "Syncing Clock...";

    if (!clockSynced) {
        return String(timeBuffer);
    }

    // High-speed modifier accumulator (increments every 1000ms strictly)
    if (millis() - lastClockUpdate >= 1000) {
        lastClockUpdate += 1000; // Preserves leftover elapsed fractional intervals cleanly
        
        myClock.tm_sec++; 
        
        if (myClock.tm_sec >= 60) {
            myClock.tm_sec = 0;
            myClock.tm_min++; 
            
            if (myClock.tm_min >= 60) {
                myClock.tm_min = 0;
                myClock.tm_hour++; 
                
                if (myClock.tm_hour >= 24) {
                    myClock.tm_hour = 0;
                    myClock.tm_mday++; 
                    
                    // Normalize standard structural rules for cross-month boundary transitions cleanly
                    time_t t = mktime(&myClock);
                    struct tm* normalized = localtime(&t);
                    if (normalized != NULL) {
                        myClock = *normalized;
                    }
                }
            }
        }
    }

    //time buffer is static formatted, while myclock is raw
    // Direct buffer layout output generation
    strftime(timeBuffer, sizeof(timeBuffer), "%Y %m %d- %H:%M", &myClock);
    return String(timeBuffer);
}
