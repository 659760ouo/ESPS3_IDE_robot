#include "cnd.h"
#include "../camera/camera.h"
#include "../display/lcd/lcd.h"
#include "../camera/app_httpd.h"
#include <Arduino.h>
#include "esp_camera.h"

// Define states including a startup flag to synchronize hardware on boot
enum AppMode { MODE_ANIMATION, MODE_CAMERA, MODE_STARTUP };
AppMode currentMode = MODE_STARTUP; 

const int switchPin = 40; // GPIO pin for the physical switch

void handleCameraAndDisplay() {
  bool isSwitchPressed = (digitalRead(switchPin) == LOW);

  // --- 1. STARTUP SYNCHRONIZATION (Runs exactly ONCE on boot) ---
  if (currentMode == MODE_STARTUP) {
    if (isSwitchPressed) {
      currentMode = MODE_CAMERA;
      lcd_deinit();
      camera_init();
      startCameraServer();
    } else {
      currentMode = MODE_ANIMATION;
      // Force clean state: shut off camera noise and start the LCD cleanly
      camera_deinit();
      lcd_init(); 
    }
    return; // Skip the rest of this loop iteration to let states stabilize
  }

  // --- 2. TRANSITION LOGIC (Runs ONCE per physical state change) ---
  if (isSwitchPressed && currentMode == MODE_ANIMATION) {
    // Switch Pressed: Move from Animation to Camera
    currentMode = MODE_CAMERA;
    lcd_deinit(); 
    camera_init(); 
    startCameraServer(); 
    delay(50); // Simple debounce
  } 
  else if (!isSwitchPressed && currentMode == MODE_CAMERA) {
    // Switch Released: Move from Camera to Animation
    currentMode = MODE_ANIMATION;
    stopCameraServer(); 
    delay(50); // Simple debounce
    
    camera_deinit(); 
    Serial.println("Camera server stopped, returned to animation mode");
    lcd_init(); 
  }

  // --- 3. RUNNING LOOP LOGIC ---
  if (currentMode == MODE_CAMERA) {
    delay(100); // Prevent serial flooding, background core handles streaming
  } 
  else {
    // Run the eye animation loop cleanly at ~30 FPS on a safe bus configuration
    animateEyes(); 
    delay(12); // Roughly 30 FPS, adjust as needed for performance
  }
}
