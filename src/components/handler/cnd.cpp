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

// Button state tracking and debounce timing variables
bool lastSwitchState = HIGH;      
unsigned long lastDebounceTime = 0; 
const unsigned long debounceDelay = 50; 

void handleCameraAndDisplay() {
  // Read immediate physical logic level from your push button
  bool rawSwitchReading = digitalRead(switchPin);

  // --- 1. STARTUP INITIALIZATION (Always default boot to your weather clock dashboard) ---
  if (currentMode == MODE_STARTUP) {
    Serial.println("[System] Wire-free Reset Handler Init...");
    delay(200); 
    
    pinMode(switchPin, INPUT_PULLUP); // Guard against floating line interference

    currentMode = MODE_ANIMATION; 
    lcd_init();
    delay(150);
    
    lastSwitchState = digitalRead(switchPin); 
    Serial.println("[System] Initial Boot Complete: Weather Dashboard Online.");
    return; 
  }

  // --- 2. EDGE DETECTION & DEBOUNCE FILTER (CURRENT VS LAST) ---
  bool switchPressedEvent = false;
  static bool debouncedSwitchState = HIGH;

  if (rawSwitchReading != lastSwitchState) {
    lastDebounceTime = millis();
    lastSwitchState = rawSwitchReading;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Catch a falling-edge logic event (HIGH transition to LOW ground click)
    if (rawSwitchReading == LOW && debouncedSwitchState == HIGH) {
      switchPressedEvent = true; 
    }
    debouncedSwitchState = rawSwitchReading;
  }

  // --- 3. CONDITIONAL BEHAVIOR STATE ENGINE ---
  if (switchPressedEvent) {
    
    // BRANCH A: Currently in LCD Mode -> Move smoothly to Camera Server
    if (currentMode == MODE_ANIMATION) {
      Serial.println("[Mode Change] Switching over to Camera Streaming...");
      currentMode = MODE_CAMERA;
      
      lcd_deinit(); 
      delay(100); 
      
      camera_init(); 
      delay(50);
      startCameraServer();
      Serial.println("[System] Camera Active. Stream server open at local IP.");
    } 
    
    //BRANCH B: Currently in Camera Mode -> Force instant wire-free software reboot
    else if (currentMode == MODE_CAMERA) {
      Serial.println("\n[🚨 SOFTWARE REBOOT] Click registered during active network stream!");
      Serial.println("[🚨 SOFTWARE REBOOT] Executing clean system restart via registers...");
      delay(50); // Small margin to flush the outbox serial stream cache cleanly
      
      //  directly reset the camera back to initial state(LCD mode)
      ESP.restart(); 
      
      // delay after restart for enough buffer for bootloader to load first

      while(1) { delay(1000); } 
    }
  }

  // --- 4. RUNNING CONTINUOUS TRACK ACTIONS ---
  if (currentMode == MODE_CAMERA) {
    // Keep this main thread idle to clear execution slots for background M-JPEG transfers
    delay(100); 
  } 
  else {
    // Normal Operation Branch: Feed time modifiers and update the PSRAM Canvas frames
    weather_page_lcd(); 
    delay(12); // Pacing delay ~30 FPS frame rate
  }
}
