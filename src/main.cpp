/*******************************************************************************
 * LVGL 2.8-Inch CYD Victron BLE Dashboard 
 ******************************************************************************/
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <VictronBLE.h> 
#include <Wire.h>
#include "secrets.h"

#define SCREEN_ID_MAIN SCREEN_ID_SCREEN_1 //fix screen ID conflict with generated screens.h
// PicoPixel UI Header mappings
#include "ui.h"
#include "vars.h"
#include "screens.h" 

#define CYD_BACKLIGHT_PIN 21

// CYD Touchscreen Hardware SPI Pins
#define XPT2046_CS   33
#define XPT2046_IRQ  36

// Instantiate Drivers
TFT_eSPI tft = TFT_eSPI(); 
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

static const char* chargeStateName(uint8_t state) {
    switch (state) {
        case CHARGER_OFF:              return "OFF";
        case CHARGER_LOW_POWER:        return "LOW POWER";
        case CHARGER_FAULT:            return "FAULT";
        case CHARGER_BULK:             return "BULK";
        case CHARGER_ABSORPTION:       return "ABSORPTION"; 
        case CHARGER_FLOAT:            return "FLOAT";
        case CHARGER_STORAGE:          return "STORAGE";
        case CHARGER_EQUALIZE:         return "EQUALIZE";
        case CHARGER_INVERTING:        return "INVERTING";
        case CHARGER_POWER_SUPPLY:     return "POwER SUPPLY";
        case CHARGER_EXTERNAL_CONTROL: return "EXTERNAL CONTROL";
        default:                       return "UNKNOWN";
    }
}
/* --- VICTRON BLE RADIO ENGINE --- */
VictronBLE victron;

struct VictronSharedState {
    float voltage;
    float current;
    float soc;
    float power;
    int32_t remainingMinutes;
    uint8_t mpptState;
    uint32_t shuntPacketsReceived;
    uint32_t mpptPacketsReceived;
    bool dataReady;
};
volatile VictronSharedState sharedMetrics = {0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, false};

void onVictronBleData(const VictronDevice* device) {
    if (device->dataValid) {
        if (device->deviceType == DEVICE_TYPE_BATTERY_MONITOR) { 
            sharedMetrics.voltage = device->battery.voltage;
            sharedMetrics.current = device->battery.current;
            sharedMetrics.soc     = device->battery.soc;
            sharedMetrics.remainingMinutes = device->battery.remainingMinutes;
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


/* --- LVGL DISPLAY FLUSH (TFT_eSPI Optimized) --- */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

/* --- LVGL TOUCHPAD READ (XPT2046 Optimized) --- */
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        
        // Basic calibration mapping for CYD landscape orientation (320x240)
        int16_t x = map(p.x, 200, 3700, 0, 320);
        int16_t y = map(p.y, 240, 3800, 0, 240);

        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); 
    Serial.println("[SYSTEM] Starting TFT_eSPI + XPT2046 CYD Framework...");

    // Turn display backlight on
    pinMode(CYD_BACKLIGHT_PIN, OUTPUT);
    digitalWrite(CYD_BACKLIGHT_PIN, HIGH); 

    // Init Display
    tft.init();
    tft.setRotation(1); // Landscape orientation
    tft.fillScreen(TFT_BLACK);

    // Init Touch
    touch.begin();
    touch.setRotation(1);

    lv_init();

    uint32_t screenWidth = 320;
    uint32_t screenHeight = 240;

    // We only register the devices that explicitly map to your UI variables
    victron.addDevice("SmartShunt", SmartShuntMAC, SmartShuntEncryptionKey);
    victron.addDevice("SmartMPPT",  SmartMPPTMAC, SmartMPPTEncryptionKey);
    victron.setCallback(onVictronBleData);
    victron.begin(); 

    // Setup buffer structure for 320x240
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
    Serial.println("[SYSTEM] PicoPixel Layout successfully attached via TFT_eSPI pipeline.");
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
        snap.remainingMinutes = sharedMetrics.remainingMinutes;
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

            lv_label_set_text_fmt(objects.loadsvoltsdata, "%d.%02d V", wholeVolts, milliVolts);
            
            if (snap.current < 0.0f && wholeAmps == 0) {
                lv_label_set_text_fmt(objects.loadsampsdata, "-0.%02d A", milliAmps);
            } else {
                lv_label_set_text_fmt(objects.loadsampsdata, "%d.%02d A", wholeAmps, milliAmps);
            }

            lv_label_set_text_fmt(objects.batterypercentdata, "%d%%", wholeSoc);
            if (objects.battery_bar) {
                lv_bar_set_value(objects.battery_bar, wholeSoc, LV_ANIM_ON);
            }

            if (snap.remainingMinutes == 0xFFFF || snap.remainingMinutes <= 0) {
                lv_label_set_text(objects.timeremainingdata, "Inf.");
            } else {
                int hours = snap.remainingMinutes / 60;
                int mins = snap.remainingMinutes % 60;
                lv_label_set_text_fmt(objects.timeremainingdata, "%dh %dm", hours, mins);
            }
        }

        // 2. POPULATE SMARTMPPT SOLAR METRICS
        if (snap.mpptPacketsReceived > 0) {
            int wholePower = (int)snap.power;
            lv_label_set_text_fmt(objects.totalcharge, "%d W", wholePower);
            
            if (snap.voltage > 1.0f) {
                float calculatedSolarAmps = snap.power / snap.voltage;
                int sAmpsWhole = (int)calculatedSolarAmps;
                int sAmpsMilli = (int)abs((int)((calculatedSolarAmps - sAmpsWhole) * 100));
                
                lv_label_set_text_fmt(objects.solarampsdata, "%d.%02d A", sAmpsWhole, sAmpsMilli);
                lv_label_set_text_fmt(objects.solarvoltsdata, "%d.%02d V", wholeVolts, milliVolts);
            }
            if (objects.chargetypedata) {
                lv_label_set_text(objects.chargetypedata, chargeStateName(snap.mpptState));
            }
        }

        lastWidgetRefresh = millis();
    }
    delay(1); 
}