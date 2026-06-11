/*******************************************************************************
 * LVGL 7-Inch Waveshare Victron BLE Dashboard - Clean Native Hardware Version
 * Preconfigured for Waveshare ESP32-S3-Touch-LCD-7 (800x480)
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <VictronBLE.h> 

#define TFT_BL 2 // Native Backlight Pin for this 7" board layout

#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else 

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
    9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
    15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */
);

Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
    bus,
    800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);
#endif 

#include "touch.h"

/* LVGL Engine Globals */
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
    bool dataReady;
};
volatile VictronSharedState sharedMetrics = {0.0f, 0.0f, 0.0f, 0.0f, false};

lv_obj_t *lbl_battery;
lv_obj_t *lbl_solar;

void onVictronBleData(const VictronDevice* device) {
    if (device->dataValid) {
        if (device->deviceType == DEVICE_TYPE_BATTERY_MONITOR) { 
            sharedMetrics.voltage   = device->battery.voltage;
            sharedMetrics.current   = device->battery.current;
            sharedMetrics.soc       = device->battery.soc;
            sharedMetrics.dataReady = true;
            Serial.printf("[BLE] Shunt updated: %.2fV, %.2fA\n", sharedMetrics.voltage, sharedMetrics.current);
        } 
        else if (device->deviceType == DEVICE_TYPE_SOLAR_CHARGER) { 
            sharedMetrics.power     = device->solar.panelPower;
            sharedMetrics.dataReady = true;
            Serial.printf("[BLE] MPPT updated: %.0fW\n", sharedMetrics.power);
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

void setup() {
    Serial.begin(115200);
    delay(1000); 
    Serial.println("\n[SYSTEM] Initializing Waveshare 7-Inch Display Setup...");

    // 1. Fire up Backlight natively via PWM on GPIO 2 (Stops I2C timeouts)
    Serial.println("[HARDWARE] Powering up Backlight Rail natively (GPIO 2)...");
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    ledcSetup(0, 300, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, 255); // Force maximum brightness right away

    // 2. Launch the RGB Parallel Panel Driver matrix
    Serial.println("[HARDWARE] Starting RGB Parallel Matrix configuration...");
    gfx->begin(); 
    Serial.println("[HARDWARE] GFX Controller initialized.");

    // 3. Optional visual confirmation test
    gfx->fillScreen(RED); delay(150);
    gfx->fillScreen(GREEN); delay(150);
    gfx->fillScreen(BLUE); delay(150);
    gfx->fillScreen(BLACK);

    // 4. Fire up the layout framework engine
    Serial.println("[SYSTEM] Starting LVGL graphics workspace...");
    lv_init();

    // 5. Connect touch layers
    touch_init();

    screenWidth = gfx->width();
    screenHeight = gfx->height();
    Serial.printf("[DISPLAY] Resolution Reported: %dx%d\n", screenWidth, screenHeight);

    disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("[CRITICAL ERROR] Internal framework graphics memory allocation failed!");
        while(1) delay(100);
    }

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
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // Generate diagnostic UI text elements
    lbl_battery = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_14, 0); 
    lv_obj_align(lbl_battery, LV_ALIGN_CENTER, 0, -50);
    lv_label_set_text(lbl_battery, "Waiting for SmartShunt BLE...");

    lbl_solar = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(lbl_solar, &lv_font_montserrat_14, 0); 
    lv_obj_align(lbl_solar, LV_ALIGN_CENTER, 0, 50);
    lv_label_set_text(lbl_solar, "Waiting for MPPT BLE...");

    // 6. Connect your background scanning infrastructure
    Serial.println("[BLE] Registering target physical signatures...");
    victron.addDevice("SmartShunt", "aa:bb:cc:dd:ee:ff", "00112233445566778899aabbccddeeff");
    victron.addDevice("SmartMPPT",  "11:22:33:44:55:66", "ffeeddccbbaa99887766554433221100");
    
    victron.setCallback(onVictronBleData);
    victron.begin(); 
    
    Serial.println("[SYSTEM] Framework Engine up and processing loops cleanly.");
}

void loop() {
    lv_timer_handler(); 
    
    static uint32_t lastWidgetRefresh = 0;
    if (millis() - lastWidgetRefresh > 1000) {
        if (sharedMetrics.dataReady) {
            lv_label_set_text_fmt(lbl_battery, "Battery: %.2fV | %.2fA | %.1f%%", 
                                  sharedMetrics.voltage, sharedMetrics.current, sharedMetrics.soc);
            lv_label_set_text_fmt(lbl_solar, "Solar Production: %.0f Watts", 
                                  sharedMetrics.power);
            sharedMetrics.dataReady = false; 
        }
        lastWidgetRefresh = millis();
    }
    delay(5);
}