#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>

// Struct structure must be declared here so both .cpp components see it
struct HKWeather {
    bool valid = false;
    String temperature = "";
    String humidity = "";
    String condition = "";
    String updateTime = "";
};

HKWeather getHKWeather();
String getHKTime(); 

#endif
