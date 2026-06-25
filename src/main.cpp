/*******************************************************************************
 * LVGL Victron BLE Dashboard (NO TOUCH - STABLE BUILD)
 ******************************************************************************/

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <VictronBLE.h>
#include "secrets.h"

#include "ui.h"
#include "vars.h"
#include "screens.h"

/* ---------------- DISPLAY ---------------- */
TFT_eSPI tft;

/* ---------------- VICTRON ---------------- */
VictronBLE victron;

/* ---------------- THREAD SAFE STATE ---------------- */
portMUX_TYPE victronMux = portMUX_INITIALIZER_UNLOCKED;

struct VictronSharedState {
    float voltage;
    float current;
    float soc;
    float power;
    int32_t remainingMinutes;

    uint8_t mpptState;

    uint32_t shuntPackets;
    uint32_t mpptPackets;

    bool dataReady;
};

VictronSharedState sharedMetrics = {0};

/* ---------------- DISPLAY FLUSH ---------------- */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {

    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

/* ---------------- UI UPDATE ---------------- */
void updateUI(const VictronSharedState& s) {

    if (objects.loadsvoltsdata)
        lv_label_set_text_fmt(objects.loadsvoltsdata, "%0.2f V", s.voltage);

    if (objects.loadsampsdata)
        lv_label_set_text_fmt(objects.loadsampsdata, "%0.2f A", s.current);

    if (objects.batterypercentdata)
        lv_label_set_text_fmt(objects.batterypercentdata, "%d%%", (int)s.soc);

    if (objects.battery_bar)
        lv_bar_set_value(objects.battery_bar, (int)s.soc, LV_ANIM_ON);

    if (objects.totalcharge)
        lv_label_set_text_fmt(objects.totalcharge, "%d W", (int)s.power);

    if (objects.chargetypedata) {
        lv_label_set_text(objects.chargetypedata,
            (s.mpptState == 0) ? "OFF" : "ACTIVE");
    }
}

/* ---------------- BLE CALLBACK ---------------- */
void onVictronBleData(const VictronDevice* device) {

    if (!device->dataValid) return;

    portENTER_CRITICAL(&victronMux);

    if (device->deviceType == DEVICE_TYPE_BATTERY_MONITOR) {
        sharedMetrics.voltage = device->battery.voltage;
        sharedMetrics.current = device->battery.current;
        sharedMetrics.soc     = device->battery.soc;
        sharedMetrics.remainingMinutes = device->battery.remainingMinutes;
        sharedMetrics.shuntPackets++;
    }

    if (device->deviceType == DEVICE_TYPE_SOLAR_CHARGER) {
        sharedMetrics.power = device->solar.panelPower;
        sharedMetrics.mpptState = device->solar.chargeState;
        sharedMetrics.mpptPackets++;
    }

    sharedMetrics.dataReady = true;

    portEXIT_CRITICAL(&victronMux);
}

/* ---------------- SETUP ---------------- */
void setup() {

    Serial.begin(115200);
    delay(500);

    /* BACKLIGHT (toggle HIGH/LOW if blank screen) */
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);

    /* TFT INIT (ONLY ONCE) */
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    /* LVGL INIT (ONLY ONCE) */
    lv_init();

    static lv_color_t buf[320 * 20];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* UI (MUST NOT INIT HARDWARE) */
    ui_init();

    /* VICTRON */
    victron.addDevice("SmartShunt", SmartShuntMAC, SmartShuntEncryptionKey);
    victron.addDevice("SmartMPPT", SmartMPPTMAC, SmartMPPTEncryptionKey);
    victron.setCallback(onVictronBleData);
    victron.begin();

    Serial.println("SYSTEM READY");
}

/* ---------------- LOOP ---------------- */
void loop() {

    victron.loop();
    lv_timer_handler();

    static uint32_t lastUI = 0;

    if (millis() - lastUI > 1000) {

        VictronSharedState snap;

        portENTER_CRITICAL(&victronMux);
        snap = sharedMetrics;
        sharedMetrics.dataReady = false;
        portEXIT_CRITICAL(&victronMux);

        if (snap.dataReady) {
            updateUI(snap);
        }

        lastUI = millis();
    }
}