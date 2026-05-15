#include "src/components/camera/camera.h"
#include "src/components/display/lcd/lcd.h"
#include "src/components/handler/cnd.h"

#include "src/components/common/wifi.h" // For WiFi connection function

const int switchPin = 40; // GPIO pin for the physical switch


void setup() {
  Serial.begin(115200);
  connect_wifi(); // Connect to WiFi at startup so it's ready when we switch to camera mode, and to avoid WiFi interference during camera setup. The camera server will be started separately in the handler when the user presses the button to switch to camera mode, to save resources when not in use.
  pinMode(switchPin, INPUT_PULLUP); // Set the switch pin as input with pull-up resistor
  
  
}

void loop() {
  
  handleCameraAndDisplay(); // This will handle switching between the camera feed and the animated eyes on the LCD based on the state of a switch
  
  
}
