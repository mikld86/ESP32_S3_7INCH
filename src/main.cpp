/*******************************************************************************
 * LVGL 7-Inch Waveshare Victron BLE Dashboard - Complete Working Core
 * Corrected Input Driver API, Memory Sequence & Shared I2C Control
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <VictronBLE.h> 
#include <Wire.h>

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

Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
    bus,
    800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);
#endif 

#include "touch.h"

static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

/* --- VICTRON INTEGRATION ENGINE --- */
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

lv_obj_t *lbl_battery;
lv_obj_t *lbl_solar;

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
/* --------------------------------- */

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
    Serial.println("\n[SYSTEM] Booting Waveshare 7-Inch Matrix Core...");

    lv_init();
    touch_init();

    writeCH422GRegister(0x0E, 0xFF); 
    delay(100); 

    gfx->begin(); 
    gfx->fillScreen(BLACK);

    // Prioritize BLE configuration initialization
    Serial.println("[BLE] Arming background Victron decryption scanning cores...");
    
    // Remember to verify your real hardware MAC addresses and 32-character encryption keys match here:
    victron.addDevice("SmartShunt", "11:22:33:44:55:66", "ffeeddccbbaa99887766554433221100");
    victron.addDevice("SmartMPPT",  "11:22:33:44:55:66", "ffeeddccbbaa99887766554433221100");
    
    victron.setCallback(onVictronBleData);
    victron.setDebug(true); 
    victron.begin(); 

    screenWidth = gfx->width();
    screenHeight = gfx->height();

    disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) { while(1) delay(100); }

    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 4);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read; // FIXED: Set to correct LVGL 8 API parameter
    lv_indev_drv_register(&indev_drv);

    lbl_battery = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_14, 0); 
    lv_obj_align(lbl_battery, LV_ALIGN_CENTER, 0, -40);

    lbl_solar = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(lbl_solar, &lv_font_montserrat_14, 0); 
    lv_obj_align(lbl_solar, LV_ALIGN_CENTER, 0, 40);

    Serial.println("[SYSTEM] Initialization cycle fully completed.");
}

void loop() {
    victron.loop(); // Processes background over-the-air raw queues
    lv_timer_handler(); 
    
    static uint32_t lastWidgetRefresh = 0;
    if (millis() - lastWidgetRefresh > 5000) {
        uint32_t uptimeSeconds = millis() / 1000;

        // Pull out clean local variables
        float currentVoltage = sharedMetrics.voltage;
        float currentAmps    = sharedMetrics.current;
        float currentSoc     = sharedMetrics.soc;
        float currentPower   = sharedMetrics.power;
        uint32_t rxShunt     = sharedMetrics.shuntPacketsReceived;
        uint32_t rxMppt      = sharedMetrics.mpptPacketsReceived;

        // SmartShunt UI Update Logic
        if (rxShunt > 0) {
            // PURE LVGL: Multiply by 10 or 100 and print as integers to bypass 
            // the missing float support in your lv_conf.h configuration
            int wholeVolts = (int)currentVoltage;
            int milliVolts = (int)((currentVoltage - wholeVolts) * 100);
            
            int wholeAmps  = (int)currentAmps;
            int milliAmps  = (int)abs((int)((currentAmps - wholeAmps) * 100)); // Keep decimals positive
            
            int wholeSoc   = (int)currentSoc;

            lv_label_set_text_fmt(lbl_battery, "SmartShunt: %d.%02dV | %d.%02dA | %d%% (%u rx)", 
                                  wholeVolts, milliVolts, 
                                  wholeAmps, milliAmps, 
                                  wholeSoc, rxShunt);
        } else {
            lv_label_set_text_fmt(lbl_battery, "Scanning Shunt... Uptime: %u s", uptimeSeconds);
        }

        // MPPT UI Update Logic
        if (rxMppt > 0) {
            // Power is already a whole number Watt value
            int wholePower = (int)currentPower;
            lv_label_set_text_fmt(lbl_solar, "MPPT Charger: %d Watts (%u rx)", 
                                  wholePower, rxMppt);
        } else {
            lv_label_set_text_fmt(lbl_solar, "Scanning MPPT... Uptime: %u s", uptimeSeconds);
        }

        lastWidgetRefresh = millis();
    }
    delay(1); 
}