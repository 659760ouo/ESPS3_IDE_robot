#include "functions.h"
#include <WiFi.h> 
#include <WiFiClientSecure.h> // FIXED: Added for secure HTTPS connections
#include <HTTPClient.h> 
#include <Arduino.h>
#include <ArduinoJson.h> 


const char* weather_typeA = "flw"; // "flw" for current weather, "rhrread" for 3-hourly forecast, etc. (based on HKO API documentation)
const char* weather_typeB = "fnd"; // "fnd" for 9-day forecast, etc. (based on HKO API documentation)
const char* weather_typeC = "rhrread"; // "rhrread" for current forecast, etc. (based on HKO API documentation)
const char* weather_typeD = "warnsum"; // "warnsummary" for weather warnings, etc. (based on HKO API documentation)
const char* weather_typeE = "warningInfo"; // "warningInfo" for detailed weather warnings, etc. (based on HKO API documentation)
const char* weather_typeF = "swt"; // "swt" for sunrise/sunset times, etc. (based on HKO API documentation)


//Added the required query parameters (?dataType=rhrread&lang=en) from Page 6 of the handbook
String base_url = "https://data.weather.gov.hk/weatherAPI/opendata/weather.php?dataType=";

//language = en, english, tc for traditional chinese, sc for simplified chinese
String HKO_WEATHER_URL = base_url + String(weather_typeC) + "&lang=en"; // Using the "rhrread" endpoint for current weather forecast as an example, you can switch to other endpoints by changing weather_typeC to A, B, D, E, or F based on your needs and the HKO API documentation. The "rhrread" endpoint provides real-time weather data which is suitable for our LCD display updates.




HKWeather getHKWeather() {
    HKWeather weather; 



    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Cannot fetch weather data.");
        return weather;
    }

    // FIXED: Changed to WiFiClientSecure to allow communication over HTTPS
    WiFiClientSecure client; 
    client.setInsecure(); // Required on ESP32-S3 to bypass complex certificate verification
    
    HTTPClient http; 

    if (!http.begin(client, HKO_WEATHER_URL)) {
        Serial.println("Failed to initialize HTTP connection");
        return weather;
    }
    
    Serial.println("Fetching weather data from HKO...");

    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Weather data received:");

        //Print the full set data for debugging - the HKO response contains multiple nested objects and arrays, so this helps us understand the structure for parsing
        Serial.println(payload); 
        
        // Increased size to 2048 bytes because the full HKO response contains arrays for multiple stations
        DynamicJsonDocument doc(6144); 
        DeserializationError err = deserializeJson(doc, payload);
        
        if (!err) {
            weather.valid = true; 

            if (doc.containsKey("updateTime")) {
                weather.updateTime = doc["updateTime"].as<String>();
            }

            // FIXED: Set target station filter name to "Yuen Long"
            JsonArray tempData = doc["temperature"]["data"];
            for (JsonObject item : tempData) {
                if (item["place"].as<String>() == "Yuen Long") {
                    weather.temperature = String(item["value"].as<int>()) + "°C";
                    break;
                }

                
                
            }
            //test
           

            // FIXED: Set target station filter name to "Yuen Long"
            JsonArray humiData = doc["humidity"]["data"];
            for (JsonObject item : humiData) {
                if (item["place"].as<String>() == "Hong Kong Observatory") {
                    weather.humidity = String(item["value"].as<int>()) + "%";
                    break;
                }
                
                
                
            }

            //test
            
            


            // FIXED: Replaced "indicator" with the documented "icon" array lookup from Page 9
            if (doc.containsKey("icon") && doc["icon"].is<JsonArray>()) {
                int iconCode = doc["icon"].as<int>(); 
                
                switch (iconCode) {
                    case 50: weather.condition = "Sunny"; break;
                    case 51: weather.condition = "Sunny Intervals"; break;
                    case 53: weather.condition = "Cloudy"; break;
                    case 54: weather.condition = "Light Rain"; break;
                    case 55: weather.condition = "Rain"; break;
                    case 56: weather.condition = "Thunderstorm"; break;
                    default: weather.condition = "Fair"; break;
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

String getHKTime() {
    // Keep your local clock logic here...
    return "12:00"; 
}
