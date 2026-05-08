#ifndef WEATHERHELPER_H
#define WEATHERHELPER_H

#include <Arduino.h>

// Deklaracje funkcji pogodowych
void weatherTask(void *pvParameters);
void fetchWeatherData();
void updateWeatherUI(const String& jsonData);

#endif