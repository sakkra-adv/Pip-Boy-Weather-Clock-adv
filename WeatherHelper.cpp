#include "WeatherHelper.h"
#include "Config.h"
#include <HTTPClient.h>
#include <lvgl.h>
#include "ui.h"
#include <ArduinoJson.h>

extern String API_KEY;
extern String LOCATION;
extern lv_obj_t *ui_Label_temp;
extern lv_obj_t *ui_Label_hum;


void weatherTask(void *pvParameters) {
    Serial.println("!!! Weather Task URUCHOMIONY !!!"); // To musi się pojawić w Serialu
    while (true) {
        if (WiFi.status() == WL_CONNECTED) {
            fetchWeatherData();
        } else {
            Serial.println("WeatherTask: Czekam na WiFi...");
        }
        vTaskDelay(600000 / portTICK_PERIOD_MS); 
    }
}


void fetchWeatherData() {
    String url = String("https://api.weatherapi.com/v1/current.json?key=") + API_KEY + "&q=" + LOCATION;
    
    Serial.println("Pobieram pogode z URL:");
    Serial.println(url);

    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("SUKCES! Odpowiedz serwera:");
        Serial.println(response); // Tu musi sie pojawic tekst JSON
        updateWeatherUI(response);
    } else {
        Serial.printf("BLAD POGODY (Kod HTTP): %d\n", httpResponseCode);
    }
    http.end();
}


// Helper function to parse JSON and update the LVGL UI elements
void updateWeatherUI(const String &jsonData) {
    // Initialize a JSON document
    DynamicJsonDocument doc(2048);

    // Parse the JSON data
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    // Extract weather data
    float temp_c = doc["current"]["temp_c"];
    int humidity = doc["current"]["humidity"];
    float feelslike_c = doc["current"]["feelslike_c"];
    float heatindex_c = doc["current"]["heatindex_c"];


    lv_bar_set_value(ui_Bar_Temp, temp_c, LV_ANIM_ON); 
    lv_bar_set_value(ui_Bar_Hum, humidity, LV_ANIM_ON);
    lv_bar_set_value(ui_Bar_FL, feelslike_c, LV_ANIM_ON); 
    lv_bar_set_value(ui_Bar_HI, heatindex_c, LV_ANIM_ON); 

    // Format and update the temperature label (T:36°C)
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "T:%d°C", (int)round(temp_c));  // Round and cast to int for display
    lv_label_set_text(ui_Label_temp, tempStr);

    // Format and update the humidity label (H:47%)
    char humStr[16];
    snprintf(humStr, sizeof(humStr), "H:%d%%", humidity);
    lv_label_set_text(ui_Label_hum, humStr);

}
