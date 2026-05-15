#include "cnd.h"
#include "../camera/camera.h"
#include "../display/lcd/lcd.h"
#include "../camera/app_httpd.h"
#include <Arduino.h>
#include "esp_camera.h"
#include "../display/lcd/weather.h"

enum AppMode { MODE_ANIMATION, MODE_CAMERA, MODE_STARTUP };
AppMode currentMode = MODE_STARTUP; 


const int switchPin = 40; 

void handleCameraAndDisplay() {
  bool isSwitchPressed = (digitalRead(switchPin) == LOW);

  // --- 1. STARTUP SYNCHRONIZATION (Determines mode cleanly on boot) ---
  if (currentMode == MODE_STARTUP) {
    // Give hardware voltage rails a moment to settle at boot
    delay(200);
    isSwitchPressed = (digitalRead(switchPin) == LOW);

    if (isSwitchPressed) {
      // If switch is grounded at boot -> Go directly to Camera
      currentMode = MODE_CAMERA;


      camera_init(); 
      delay(50);

      startCameraServer();
    
    } else {
      // If switch is 5V at boot -> Go directly to Animation
      currentMode = MODE_ANIMATION;
      // stopCameraServer(); // Ensure camera server is stopped if we boot into animation mode
      
      lcd_init();
      delay(150);
    }
    return; 
  }

  //Switch from animation to camera mode when switch is pressed (moved from 5V to GND)
  if (isSwitchPressed && currentMode == MODE_ANIMATION) {
    // Transition: Switch Pressed (Moved from 5V to GND)
    currentMode = MODE_CAMERA;
    lcd_deinit(); 
    delay(100); 
    
    
    camera_init(); // Ensure camera hardware is initialized before starting server
    
    // Instead of calling full camera_init() which breaks connections,
    // just start the server task directly since setup() handled init
    startCameraServer(); 
    
  } 
  //Switch from camera to animation mode when switch is released (moved from GND to 5V)
  else if (!isSwitchPressed && currentMode == MODE_CAMERA) {
    // Transition: Switch Released (Moved from GND to 5V)
    Serial.println("Switch released, transitioning to animation mode...");
    currentMode = MODE_ANIMATION;
    stopCameraServer(); 
    delay(50); 
    
    
    Serial.println("Camera server stopped, returned to animation mode");
    lcd_init(); 
  }

  // --- 3. RUNNING LOOP LOGIC ---
  if (currentMode == MODE_CAMERA) {
    delay(100); // Prevents serial flooding, background core streams automatically
  } 
  else {
    // Run the eye animation loop cleanly at ~30 FPS on a safe bus configuration
   
    weather_page_lcd(); // Update LCD with weather data (runs at ~30 FPS)
    delay(12); 
  }
}
