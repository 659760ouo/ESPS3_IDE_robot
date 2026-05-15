#ifndef LCD_H
#define LCD_H

// for external components to call the LCD functions without needing to know the internal details of the LCD implementation
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "lcd.h"

extern Adafruit_ST7789 tft; // Declare the tft object as extern to be defined in lcd.cpp
extern GFXcanvas16 *canvas; // Declare the canvas pointer as extern to be defined in lcd.cpp

void lcd_init(); // This tells the main app this function exists
void drawSharingan(int x, int y, int size, float angle); // This tells the main app this function exists
void drawRinnegan(int x, int y, int size, float angle); // This tells the main app this function exists
void animateEyes(); // This tells the main app this function exists
void lcd_deinit(); // This tells the main app this function exists

#endif

