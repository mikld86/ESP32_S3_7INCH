/*******************************************************************************
 * LVGL 7-Inch Waveshare Victron BLE Dashboard - Official Documentation Mapped
 * Mapped to Waveshare ESP32-S3-Touch-LCD-7 (800x480) Shared I2C Architecture
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <VictronBLE.h> 
#include <Wire.h>

// Onboard single I2C Bus layout shared by CH422G and GT911
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define CH422G_I2C_ADDR 0x24

#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else 

// Strictly mapped to official Waveshare RGB Parallel Pinout
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    5  /* DE */,  3 /* VSYNC */, 46 /* HSYNC */, 7 /* PCLK */,
    1  /* R3 */, 2  /* R4 */, 42 /* R5 */, 41 /* R6 */, 40 /* R7 */, // Red Data
    39 /* G2 */, 0  /* G3 */, 45 /* G4 */, 48 /* G5 */, 47 /* G6 */, 21 /* G7 */, // Green Data
    14 /* B3 */, 38 /* B4 */, 18 /* B5 */, 17 /* B6 */, 10 /* B7 */  // Blue Data
);

Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
    bus,
    800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);
#endif 

#include "touch.h"

/* LVGL Framework Globals */
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
        } 
        else if (device->deviceType == DEVICE_TYPE_SOLAR_CHARGER) { 
            sharedMetrics.power     = device->solar.panelPower;
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

// Low-level helper function to execute clean register configurations to the CH422G expander
void writeCH422GRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(CH422G_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    byte err = Wire.endTransmission();
    if (err != 0) {
        Serial.printf("[HARDWARE ERROR] Failed writing to CH422G Reg 0x%02X. I2C Error Code: %d\n", reg, err);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); 
    Serial.println("\n[SYSTEM] Booting Waveshare 7-Inch Matrix Core...");

    // 1. Fire up unified shared I2C bus interface configuration
    Serial.println("[HARDWARE] Instantiating Shared Master I2C Interface (Pins 8/9)...");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000U);

    // 2. Perform Explicit CH422G Expansion Rail Sequencing
    Serial.println("[HARDWARE] Configuring CH422G IO Expander Rails...");
    
    // Set EXIO direction/output config
    // EXIO2 (Backlight Enable) -> HIGH
    // EXIO1 (Touch Screen Reset Pin) -> HIGH
    // EXIO6 (LCD_VDD_EN Display Matrix Power VCOM gate) -> HIGH
    writeCH422GRegister(0x0E, 0xFF); 

    Serial.println("[HARDWARE] Power rails latched. Waiting for panel stabilization...");
    delay(100); // Give display driver matrix time to saturate rails

    // 3. Drive panel setup sequence via RGB Parallel bus lines
    Serial.println("[HARDWARE] Activating Arduino_GFX Panel Matrix...");
    gfx->begin(); 
    
    // Run an instantaneous hardware color swap verification loop
    gfx->fillScreen(RED); delay(150);
    gfx->fillScreen(GREEN); delay(150);
    gfx->fillScreen(BLUE); delay(150);
    gfx->fillScreen(BLACK);

    // 4. Initializing graphic structure layouts
    Serial.println("[SYSTEM] Firing up LVGL Graphics Engine layer...");
    lv_init();
    
    // Connect touch structures over the active, shared 8/9 bus lines
    Serial.println("[HARDWARE] Attaching GT911 Touch Controller Hooks...");
    touch_init();

    screenWidth = gfx->width();
    screenHeight = gfx->height();

    disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("[CRITICAL ERROR] Failed to claim memory buffer arrays. Halting.");
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

    // Define telemetry interface hooks
    lbl_battery = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_14, 0); 
    lv_obj_align(lbl_battery, LV_ALIGN_CENTER, 0, -50);
    lv_label_set_text(lbl_battery, "Waiting for SmartShunt BLE...");

    lbl_solar = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(lbl_solar, &lv_font_montserrat_14, 0); 
    lv_obj_align(lbl_solar, LV_ALIGN_CENTER, 0, 50);
    lv_label_set_text(lbl_solar, "Waiting for MPPT BLE...");

    // 5. Connect Bluetooth Decryption Background Framework
    Serial.println("[BLE] Arming background Victron decryption scanning cores...");
    victron.addDevice("SmartShunt", "aa:bb:cc:dd:ee:ff", "00112233445566778899aabbccddeeff");
    victron.addDevice("SmartMPPT",  "11:22:33:44:55:66", "ffeeddccbbaa99887766554433221100");
    
    victron.setCallback(onVictronBleData);
    victron.begin(); 
    
    Serial.println("[SYSTEM] Initialization cycle fully completed.");
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