#include <M5Unified.h>
#define LGFX_USE_V1
#include <lvgl.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include "LEDHelper.h"

#include "ui.h"
#include "keyboard_map.h"
#include "WiFiHelper.h"
#include "WeatherHelper.h"

// Cardputer Hardware Definitions
#define ADDR 0x34
#define SDA_PIN 8
#define SCL_PIN 9
#define IRQ_PIN 44

// Global variables declared in WiFiHelper.cpp (accessed via extern)
extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern String TIME_ZONE;

// Local control variables
bool isShifted = false;
int currentBrightness = 128;
unsigned long lastNTPTimeCheck = 0;
const unsigned long ntpTimeCheckInterval = 10000;

LEDHelper led;

// LVGL Buffers (for graphics rendering)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[240 * 10];
static lv_color_t buf2[240 * 10];

// Helper function to write to keyboard registers via I2C
void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

// LVGL Display Driver Setup
void lvgl_setup() {
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 240 * 10);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = [](lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
        uint32_t w = area->x2 - area->x1 + 1;
        uint32_t h = area->y2 - area->y1 + 1;
        M5.Display.startWrite();
        M5.Display.setAddrWindow(area->x1, area->y1, w, h);
        // 'true' at the end handles Swap Bytes for correct color order
        M5.Display.pushPixels((uint16_t *)&color_p->full, w * h, true);
        M5.Display.endWrite();
        lv_disp_flush_ready(disp_drv);
    };
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 135;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

// Background task for battery monitoring
void batteryTask(void *pvParameters) {
    for(;;) {
        float v = M5.Power.getBatteryVoltage() / 1000.0;
        int p = constrain((int)(((v - 3.3) / (4.2 - 3.3)) * 100), 0, 100);
        lv_bar_set_value(ui_Bar_battery, p, LV_ANIM_ON);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// LVGL Tick Task for timing animations/logic
void lv_tick_task(void *arg) {
    for(;;) { 
        lv_tick_inc(10); 
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    // Initialize I2C Bus (Keyboard & Power Management)
    Wire.begin(SDA_PIN, SCL_PIN, 100000U);
    pinMode(IRQ_PIN, INPUT_PULLUP);
    
    // Wake up Cardputer ADV Keyboard (Register Init)
    writeReg(0x92, 0xFF);
    writeReg(0x1D, 0xFF); 
    writeReg(0x1E, 0xFF); 
    writeReg(0x1F, 0x03);
    writeReg(0x01, 0x11); 
    writeReg(0x02, 0x01);

    // Set initial display brightness
    M5.Lcd.setBrightness(currentBrightness);
    
    // Start Graphics Engine
    lvgl_setup();
    ui_init();

    // Start Vault Boy animations
    walking_Animation(ui_Img_stat, 0);
    thumpsup_Animation(ui_Img_data, 0);

    // Create FreeRTOS Tasks
    // wifiTask handles SD card reading and WiFi/NTP connection
    xTaskCreatePinnedToCore(wifiTask, "wifiTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(batteryTask, "batteryTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(lv_tick_task, "lv_tick_task", 4096, NULL, 5, NULL, 1);
}

void loop() {
    // Handle LVGL timer events
    lv_timer_handler();

    // NTP Time Synchronization check
    if (millis() - lastNTPTimeCheck >= ntpTimeCheckInterval) {
        fetchNTPTime();
        lastNTPTimeCheck = millis();
    }

    // Handle I2C Keyboard Input (ADV Model)
    if (digitalRead(IRQ_PIN) == LOW) {
        Wire.beginTransmission(ADDR);
        Wire.write(0x04);
        Wire.endTransmission(false);
        if (Wire.requestFrom(ADDR, 1)) {
            uint8_t val = Wire.read();
            if (val > 0) {
                uint8_t id = val & 0x7F;
                bool pressed = (val & 0x80);

                if (id == 7) { // Shift key handling
                    isShifted = pressed;
                } else if (pressed) {
                    char c = getCharADV(id, isShifted);
                    
                    if (c == '/') { // '/' key - Change Tab
                        lv_tabview_set_act(ui_Tab_main, 1, LV_ANIM_ON);
                        M5.Speaker.tone(4000, 50);
                    }
                    else if (c == ',') { // ',' key - Back to main tab
                        lv_tabview_set_act(ui_Tab_main, 0, LV_ANIM_ON);
                        M5.Speaker.tone(4000, 50);
                    }
                    else if (c == ';') { // ';' key - Increase brightness
                        currentBrightness = constrain(currentBrightness + 25, 0, 255);
                        M5.Lcd.setBrightness(currentBrightness);
                        M5.Speaker.tone(4000, 50);
                    }
                    else if (c == '.') { // '.' key - Decrease brightness
                        currentBrightness = constrain(currentBrightness - 25, 0, 255);
                        M5.Lcd.setBrightness(currentBrightness);
                        M5.Speaker.tone(4000, 50);
                    }
                }
                writeReg(0x02, 0x01); // Acknowledge byte read from keyboard
            }
        }
    }
    delay(5);
}
    }
    delay(5);
}
