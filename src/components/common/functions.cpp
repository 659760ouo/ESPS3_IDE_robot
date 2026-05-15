#include "functions.h"
#include <WiFi.h>            // Native ESP32 Wi-Fi Stack library
#include <HTTPClient.h>      // Native ESP32 HTTP client library
#include <Arduino.h>
#include <ArduinoJson.h>    // ArduinoJson library for JSON parsing

// Endpoint URL string pointing directly to the live HKO JSON data engine
const char* HKO_WEATHER_URL = "http://weather.gov.hk";

HKWeather getHKWeather() {
    HKWeather weather; // Default initializes valid = false

    // Check wireless connection state before issuing HTTP requests
    if (WiFi.status() != WL_CONNECTED) {
        return weather; 
    }

    WiFiClient client; // Explicitly declared client buffer necessary for ESP32 streams
    HTTPClient http;
    
    // Pass both the client stream wrapper and the endpoint link address string
    if (!http.begin(client, HKO_WEATHER_URL)) {
        return weather;
    }

    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // 1536 byte heap tracking frame allocation blocks 
        DynamicJsonDocument doc(1536); 
        DeserializationError err = deserializeJson(doc, payload);
        
        if (!err) {
            weather.valid = true; 
            
            // 1. Match core HKO JSON timestamp tracking keys
            if (doc.containsKey("updateTime")) {
                weather.updateTime = doc["updateTime"].as<String>();
            }

            // 2. Loop through Temperature values array blocks
            JsonArray tempData = doc["temperature"]["data"];
            for (JsonObject item : tempData) {
                if (String(item["place"].as<const char*>()) == "Hong Kong Observatory") {
                    weather.temperature = String(item["value"].as<int>()) + "°C";
                    break;
                }
            }

            // 3. Loop through Relative Humidity array blocks
            JsonArray humiData = doc["humidity"]["data"];
            for (JsonObject item : humiData) {
                if (String(item["place"].as<const char*>()) == "Hong Kong Observatory") {
                    weather.humidity = String(item["value"].as<int>()) + "%";
                    break;
                }
            }

            // 4. Read the single raw integer marker indicating condition type
            int icon = doc["indicator"].as<int>();
            switch (icon) {
                case 50: weather.condition = "Sunny"; break;
                case 51: weather.condition = "Sunny Intervals"; break;
                case 53: weather.condition = "Cloudy"; break;
                case 54: weather.condition = "Light Rain"; break;
                case 55: weather.condition = "Rain"; break;
                case 56: weather.condition = "Thunderstorm"; break;
                default: weather.condition = "Fair"; break;
            }
        }
    }
    
    http.end(); // Safely close and tear down TCP socket streams
    return weather; 
}

// Global hook for extracting your current runtime layout clock
String getHKTime() {
    // Replace this string layout placeholder with your system NTP or internal RTC calculations
    return "12:00"; 
}
