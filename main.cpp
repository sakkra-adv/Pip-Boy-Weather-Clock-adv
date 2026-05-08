/**
 * ☢️ PIP-BOY WEATHER STATION (M5Cardputer ADV Version)
 * * FEATURES:
 * - LVGL UI with custom animations (Vault-Boy)
 * - Specialized Audio Engine for ES8311 Codec (Fixes ADV silence issues)
 * - I2C Keyboard mapping with IRQ handling
 * - Real-time Weather and NTP Time via WiFi
 */

#include "M5Cardputer.h"
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

// Hardware Definitions
#define ADDR 0x34       // Keyboard I2C Address
#define SDA_PIN 8
#define SCL_PIN 9
#define IRQ_PIN 44      // Keyboard Interrupt Pin
#define SAMPLE_RATE 22050
#define BUF_SIZE 256    // Audio buffer size for I2S

extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern String TIME_ZONE;

bool isShifted = false;
int currentBrightness = 128;
unsigned long lastNTPTimeCheck = 0;
const unsigned long ntpTimeCheckInterval = 10000;

int16_t audioBuf[BUF_SIZE];
LEDHelper led;

/**
 * GENERATE CLICK: Custom I2S Audio Synthesizer
 * @param freq: Frequency in Hz. 
 * (e.g., 1400 = deep click, 2200 = high beep, 0 = silence)
 * This function builds a Sine wave with an Exponential Decay Envelope
 * to simulate a mechanical "click" sound.
 */
void generateClick(int freq = 1800) {
    if (freq == 0) return; // Skip processing if frequency is 0 (silence)

    for (int i = 0; i < BUF_SIZE; i++) {
        // Envelope: 1.0 down to 0.0 (Linear fade out)
        float env = 1.0f - ((float)i / BUF_SIZE);
        // Sine wave generation
        float wave = sinf(2.0f * PI * freq * i / SAMPLE_RATE);
        // Apply envelope and scale to 16-bit volume
        audioBuf[i] = (int16_t)(wave * env * 12000);
    }
    // playRaw: Sends the buffer to the ES8311 Codec via I2S
    M5Cardputer.Speaker.playRaw(audioBuf, BUF_SIZE, SAMPLE_RATE, false, 1, 0);
}

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[240 * 10];
static lv_color_t buf2[240 * 10];

// I2C Helper to write to Keyboard/PMU registers
void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

/**
 * LVGL DISPLAY SETUP
 * Connects the LovyanGFX driver to the LVGL library.
 */
void lvgl_setup() {
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 240 * 10);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = [](lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
        uint32_t w = area->x2 - area->x1 + 1;
        uint32_t h = area->y2 - area->y1 + 1;
        M5Cardputer.Display.startWrite();
        M5Cardputer.Display.setAddrWindow(area->x1, area->y1, w, h);
        M5Cardputer.Display.pushPixels((uint16_t *)&color_p->full, w * h, true);
        M5Cardputer.Display.endWrite();
        lv_disp_flush_ready(disp_drv);
    };
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 135;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

// Background task to monitor and update battery status bar
void batteryTask(void *pvParameters) {
    for(;;) {
        float v = M5Cardputer.Power.getBatteryVoltage() / 1000.0;
        int p = constrain((int)(((v - 3.3) / (4.2 - 3.3)) * 100), 0, 100);
        lv_bar_set_value(ui_Bar_battery, p, LV_ANIM_ON);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// LVGL Tick timer required for animations and UI response
void lv_tick_task(void *arg) {
    for(;;) { 
        lv_tick_inc(10); 
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void setup() {
    auto cfg = M5.config();
    
    // CRITICAL FOR ADV MODEL: 
    // The 'true' parameter initializes the ES8311 Audio Codec hardware.
    M5Cardputer.begin(cfg, true); 

    // Speaker Configuration for the I2S Audio Engine
    auto spk_cfg = M5Cardputer.Speaker.config();
    spk_cfg.sample_rate = SAMPLE_RATE;
    spk_cfg.task_priority = 3;
    spk_cfg.dma_buf_count = 4;
    spk_cfg.dma_buf_len = BUF_SIZE;
    M5Cardputer.Speaker.config(spk_cfg);
    M5Cardputer.Speaker.begin();
    M5Cardputer.Speaker.setVolume(255);
    
    delay(200); 

    // Keyboard I2C and Interrupt Setup
    Wire.begin(SDA_PIN, SCL_PIN, 100000U);
    pinMode(IRQ_PIN, INPUT_PULLUP);
    
    // Initialize Keyboard controller registers
    writeReg(0x92, 0xFF);
    writeReg(0x1D, 0xFF); 
    writeReg(0x1E, 0xFF); 
    writeReg(0x1F, 0x03);
    writeReg(0x01, 0x11); 
    writeReg(0x02, 0x01); // Reset keyboard status

    M5Cardputer.Display.setBrightness(currentBrightness);
    
    lvgl_setup();
    ui_init(); // Initialize generated UI

    // Start UI Animations
    walking_Animation(ui_Img_stat, 0);
    thumpsup_Animation(ui_Img_data, 0);

    // Create RTOS Tasks for WiFi, Battery and UI Ticks
    xTaskCreatePinnedToCore(wifiTask, "wifiTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(batteryTask, "batteryTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(lv_tick_task, "lv_tick_task", 4096, NULL, 5, NULL, 1);

    // Initial boot-up sound (1200Hz warm tone)
    generateClick(1200);
}

void loop() {
    lv_timer_handler(); // Updates LVGL UI

    // Periodically sync time with NTP
    if (millis() - lastNTPTimeCheck >= ntpTimeCheckInterval) {
        fetchNTPTime();
        lastNTPTimeCheck = millis();
    }

    // KEYBOARD INTERRUPT HANDLING
    if (digitalRead(IRQ_PIN) == LOW) {
        Wire.beginTransmission(ADDR);
        Wire.write(0x04); // Request key data
        Wire.endTransmission(false);
        if (Wire.requestFrom(ADDR, 1)) {
            uint8_t val = Wire.read();
            if (val > 0) {
                uint8_t id = val & 0x7F;   // Key ID
                bool pressed = (val & 0x80); // Pressed or Released

                if (id == 7) { 
                    isShifted = pressed;
                } else if (pressed) {
                    char c = getCharADV(id, isShifted);
                    bool isFuncKey = false;
                    
                    // NAVIGATION SOUNDS & LOGIC
                    if (c == '/') { // Next Tab
                        lv_tabview_set_act(ui_Tab_main, 1, LV_ANIM_ON);
                        generateClick(1400); 
                        isFuncKey = true;
                    }
                    else if (c == ',') { // Previous Tab
                        lv_tabview_set_act(ui_Tab_main, 0, LV_ANIM_ON);
                        generateClick(1400);
                        isFuncKey = true;
                    }
                    else if (c == ';') { // Increase Brightness
                        currentBrightness = constrain(currentBrightness + 25, 10, 255);
                        M5Cardputer.Display.setBrightness(currentBrightness);
                        generateClick(2200); // High pitch chirp
                        isFuncKey = true;
                    }
                    else if (c == '.') { // Decrease Brightness
                        currentBrightness = constrain(currentBrightness - 25, 10, 255);
                        M5Cardputer.Display.setBrightness(currentBrightness);
                        generateClick(2000); // Lower pitch chirp
                        isFuncKey = true;
                    }

                    // MUTE ALL OTHER KEYS
                    if (!isFuncKey) {
                        // generateClick(0) results in total silence for standard typing
                        generateClick(0); 
                    }
                }
                // IMPORTANT: Reset keyboard register 0x02 to enable further interrupts
                writeReg(0x02, 0x01);
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
}
