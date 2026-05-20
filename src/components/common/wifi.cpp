

#include <WiFi.h>
#include <Arduino.h>
#include "wifi.h"

//change to the wifi your esp connect to, and make sure it's 2.4GHz since ESP32 doesn't support 5GHz wifi
const char *ssid = "Tenda_wifi_2.4GHz";
const char *password = "91297386";


void connect_wifi() {
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    Serial.print("WiFi connecting");
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // startCameraServer(); // We will start the camera server in the handler when the user presses the button to switch to camera mode, to save resources when not in use

    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
}

