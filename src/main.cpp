/*******************************************************************************
 * LVGL 7-Inch Waveshare Victron BLE Dashboard - Native C++ Pipeline
 * Low CPU Overhead, Safe Integer Text Parsers, and Direct Struct Mapping
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <VictronBLE.h> 
#include <Wire.h>

// Clean, native C++ header mapping
#include "ui.h"
#include "vars.h"

#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define CH422G_I2C_ADDR 0x24

#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else 
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    5  /* DE */,  3 /* VSYNC */, 46 /* HSYNC */, 7 /* PCLK */,
    1  /* R3 */, 2  /* R4 */, 42 /* R5 */, 41 /* R6 */, 40 /* R7 */, 
    39 /* G2 */, 0  /* G3 */, 45 /* G4 */, 48 /* G5 */, 47 /* G6 */, 21 /* G7 */, 
    14 /* B3 */, 38 /* B4 */, 18 /* B5 */, 17 /* B6 */, 10 /* B7 */  
);

// Standard Waveshare 7-Inch 800x480 Calibration Timings
Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
    bus,
    800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

#endif 

// Included AFTER gfx definition to prevent "not declared in this scope" macro compiler drops
#include "touch.h"

/* --- VICTRON BLE RADIO ENGINE --- */
VictronBLE victron;

struct VictronSharedState {
    float voltage;
    float current;
    float soc;
    float power;
    uint32_t shuntPacketsReceived;
    uint32_t mpptPacketsReceived;
    bool dataReady;
};
volatile VictronSharedState sharedMetrics = {0.0f, 0.0f, 0.0f, 0.0f, 0, 0, false};

void onVictronBleData(const VictronDevice* device) {
    if (device->dataValid) {
        if (device->deviceType == DEVICE_TYPE_BATTERY_MONITOR) { 
            sharedMetrics.voltage = device->battery.voltage;
            sharedMetrics.current = device->battery.current;
            sharedMetrics.soc     = device->battery.soc;
            sharedMetrics.shuntPacketsReceived++;
            sharedMetrics.dataReady = true;
        } 
        else if (device->deviceType == DEVICE_TYPE_SOLAR_CHARGER) { 
            sharedMetrics.power   = device->solar.panelPower;
            sharedMetrics.mpptPacketsReceived++;
            sharedMetrics.dataReady = true;
        }
    }
}

/* --- DISPLAY & TOUCH INTERFACE CALLBACKS --- */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch_has_signal() && touch_touched()) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void writeCH422GRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(CH422G_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void setup() {
    Serial.begin(115200);
    delay(1000); 
    Serial.println("[SYSTEM] Starting Native C++ Hardware Framework...");

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    lv_init();
    touch_init();

    writeCH422GRegister(0x0E, 0xFF); 
    delay(100); 

    // Initialize hardware first
    gfx->begin(); 
    gfx->fillScreen(BLACK);

    // FIXED: Calculate screen variables AFTER gfx->begin() has finished booting the hardware
    uint32_t screenWidth = gfx->width();
    uint32_t screenHeight = gfx->height();

    // Fallback protection: Force exact specifications if driver dimensions report 0 or garbage
    if (screenWidth == 0 || screenWidth > 800) screenWidth = 800;
    if (screenHeight == 0 || screenHeight > 480) screenHeight = 480;

    victron.addDevice("SmartShunt", "11:22:33:44:55:66", "ffeeddccbbaa99887766554433221100");
    victron.addDevice("SmartMPPT",  "11:22:33:44:55:66", "ffeeddccbbaa99887766554433221100");
    victron.setCallback(onVictronBleData);
    victron.begin(); 

    // Buffer structure scales perfectly to the enforced boundaries
    lv_color_t *disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) { while(1) delay(100); }

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 4);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read; 
    lv_indev_drv_register(&indev_drv);

    ui_init(); 
    Serial.println("[SYSTEM] Custom PicoPixel layout configuration successfully loaded.");
}

void loop() {
    victron.loop();       
    lv_timer_handler();   

    static uint32_t lastWidgetRefresh = 0;
    if (millis() - lastWidgetRefresh > 5000) { 
        
        VictronSharedState snap;
        noInterrupts();
        snap.voltage = sharedMetrics.voltage;
        snap.current = sharedMetrics.current;
        snap.soc     = sharedMetrics.soc;
        snap.power   = sharedMetrics.power;
        snap.shuntPacketsReceived = sharedMetrics.shuntPacketsReceived;
        snap.mpptPacketsReceived  = sharedMetrics.mpptPacketsReceived;
        interrupts();

        int wholeVolts = (int)snap.voltage;
        int milliVolts = (int)abs((int)((snap.voltage - wholeVolts) * 100));

        // 1. POPULATE SMARTSHUNT METRICS 
        if (snap.shuntPacketsReceived > 0) {
            int wholeAmps  = (int)snap.current;
            int milliAmps  = (int)abs((int)((snap.current - wholeAmps) * 100));
            int wholeSoc   = (int)snap.soc;

            lv_label_set_text_fmt(objects.loadsvoltsdisplay, "%d.%02d", wholeVolts, milliVolts);
            
            if (snap.current < 0.0f && wholeAmps == 0) {
                lv_label_set_text_fmt(objects.loadamps, "-0.%02d", milliAmps);
            } else {
                lv_label_set_text_fmt(objects.loadamps, "%d.%02d", wholeAmps, milliAmps);
            }

            lv_label_set_text_fmt(objects.battery, "%d%%", wholeSoc);
            lv_arc_set_value(objects.arc_1, wholeSoc);
        }

        // 2. POPULATE SMARTMPPT SOLAR METRICS
        if (snap.mpptPacketsReceived > 0) {
            int wholePower = (int)snap.power;
            lv_label_set_text_fmt(objects.solarwattschange, "%d", wholePower);
            
            if (snap.voltage > 1.0f) {
                float calculatedSolarAmps = snap.power / snap.voltage;
                int sAmpsWhole = (int)calculatedSolarAmps;
                int sAmpsMilli = (int)abs((int)((calculatedSolarAmps - sAmpsWhole) * 100));
                
                lv_label_set_text_fmt(objects.solaramps, "%d.%02d", sAmpsWhole, sAmpsMilli);
                lv_label_set_text_fmt(objects.solarvoltagevolts, "%d.%02d", wholeVolts, milliVolts);
            }
        }

        lastWidgetRefresh = millis();
    }
    delay(1); 
}