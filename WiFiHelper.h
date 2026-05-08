#ifndef WIFIHELPER_H
#define WIFIHELPER_H

#include <WiFi.h>
#include <Arduino.h>

// Deklaracje zmiennych, aby były widoczne w WeatherHelper i main
extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern String TIME_ZONE;
extern String API_KEY;
extern String LOCATION;

// Deklaracje funkcji WiFi i czasu
void wifiTask(void *pvParameters);
void fetchNTPTime();
bool isWiFiConnected();

#endif