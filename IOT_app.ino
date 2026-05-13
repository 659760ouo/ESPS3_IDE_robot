#include "src/components/camera/camera.h"
#include "src/components/display/lcd/lcd.h"
#include "src/components/handler/cnd.h"

const int switchPin = 40; // GPIO pin for the physical switch


void setup() {
  Serial.begin(115200);
  lcd_init(); // Initialize the LCD display     
  
  camera_init(); // Initialize the camera (WiFi connection will be established here)
  pinMode(switchPin, INPUT_PULLUP); // Set the switch pin as input with pull-up resistor
  
  
}

void loop() {
 
  handleCameraAndDisplay(); // This will handle switching between the camera feed and the animated eyes on the LCD based on the state of a switch
  
  
}
