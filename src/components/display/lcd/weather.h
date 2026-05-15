#ifndef WEATHER_H
#define WEATHER_H
#include "../../common/functions.h"
//initialize the weather page on the LCD, fetching data from the HKO API and displaying it. This function is designed to be called repeatedly in the main loop to keep the weather information updated in real-time, while also allowing for smooth animation of the Sharingan/Rinnegan eyes without blocking delays.

void weather_page_lcd();



#endif