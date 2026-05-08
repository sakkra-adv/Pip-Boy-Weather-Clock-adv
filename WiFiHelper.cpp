#include "WiFiHelper.h"
#include "WeatherHelper.h"
#include <lvgl.h>
#include "LEDHelper.h"
#include <SD.h>
#include <SPI.h>

extern lv_obj_t *ui_Label_Time;   
extern lv_obj_t *ui_Label_date;   
extern LEDHelper led; 

// 1. Zmienne globalne (Twoje domyślne dane - Plan B)
String WIFI_SSID = "";
String WIFI_PASSWORD = "";
String TIME_ZONE = "";
String API_KEY = "";
String LOCATION = "";

// 2. Funkcja czytająca z karty SD
void loadConfigFromSD() {
    vTaskDelay(pdMS_TO_TICKS(500)); 
    Serial.println("Inicjalizacja SD...");

    // Spróbujmy pełnej inicjalizacji pinów Cardputera (MISO=39, MOSI=14, SCK=40, CS=12)
    SPI.begin(40, 39, 14, 12); 
    
    if (!SD.begin(12, SPI, 40000000)) { 
        Serial.println("SD Fail! Fizyczny błąd napędu.");
        return;
    }

    File file = SD.open("/config.txt");
    if (!file) {
        Serial.println("Brak pliku config.txt! Pozostaje przy domyślnych.");
        return;
    }

    Serial.println("Wczytuje config.txt...");
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        // Magia wycinania danych po znaku '='
        if (line.startsWith("WIFI_SSID=")) WIFI_SSID = line.substring(10);
        else if (line.startsWith("WIFI_PASSWORD=")) WIFI_PASSWORD = line.substring(14);
        else if (line.startsWith("TIME_ZONE=")) TIME_ZONE = line.substring(10);
        else if (line.startsWith("API_KEY=")) API_KEY = line.substring(8);
        else if (line.startsWith("LOCATION=")) LOCATION = line.substring(9);
    }
    file.close();
    Serial.println("Dane z SD załadowane!");
}

// 3. Główny proces WiFi
void wifiTask(void *pvParameters) {
    // Najpierw spróbuj nadpisać zmienne danymi z SD
    loadConfigFromSD();
    
    Serial.print("Laczenie z: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        led.setLedColor(RED); // Miga na czerwono podczas łączenia
    }
    
    led.setLedColor(BLUE); // Sukces! Niebieski!
    Serial.println("WiFi Polaczone!");

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", TIME_ZONE.c_str(), 1);
    tzset();

    // Po połączeniu odpal zadanie pogody
    xTaskCreatePinnedToCore(weatherTask, "weatherTask", 4096, NULL, 1, NULL, 1);
    
    vTaskDelete(NULL); 
}

void setupWiFi() {
    // Możesz zostawić puste
}

void fetchNTPTime() {
    if (WiFi.status() == WL_CONNECTED) {
        struct tm timeInfo;
        if (getLocalTime(&timeInfo)) {
            char timeString[10];
            char dateString[20];
            
            strftime(timeString, sizeof(timeString), "%H:%M", &timeInfo);
            strftime(dateString, sizeof(dateString), "%d.%m.%Y", &timeInfo); // Format: DD.MM.YYYY
            
            lv_label_set_text(ui_Label_Time, timeString);
            lv_label_set_text(ui_Label_date, dateString); // To doda brakującą datę!
        }
    }
}