/*******************************************************************************
 * LVGL 7-Inch Waveshare Victron BLE Integrated Dashboard
 * Preconfigured for Arduino_GFX RGB panel & Standalone Bluetooth Decryption
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <VictronBLE.h> // Standalone BLE library
#include "touch.h"

#define TFT_BL 2

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

/* LVGL Engine Globals */
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

/* --- VICTRON INTEGRATION ENGINE --- */
VictronBLE victron;

// Structure to pass data safely across processor cores
struct VictronSharedState {
    float voltage;
    float current;
    float soc;
    float power;
    bool dataReady;
};
volatile VictronSharedState sharedMetrics = {0.0f, 0.0f, 0.0f, 0.0f, false};

// Temporary placeholder pointers for verifying the BLE incoming data stream
lv_obj_t *lbl_battery;
lv_obj_t *lbl_solar;

// Background Core 0 Callback: Executes when a matching encrypted payload is captured
void onVictronBleData(const VictronDevice* device) {
    if (device->getType() == VICTRON_SMART_SHUNT) {
        sharedMetrics.voltage   = device->getBatteryVoltage();
        sharedMetrics.current   = device->getBatteryCurrent();
        sharedMetrics.soc       = device->getSOC();
        sharedMetrics.dataReady = true;
    } else if (device->getType() == VICTRON_MPPT) {
        sharedMetrics.power     = device->getSolarPower();
        sharedMetrics.dataReady = true;
    }
}
/* --------------------------------- */

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

/* Touch Input driver */
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Victron 7-Inch Standalone Screen Init...");

  gfx->begin();
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  ledcSetup(0, 300, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 255); 
#endif

  // Clear splash screens quickly
  gfx->fillScreen(BLACK);
  
  lv_init();

  // Init touch device
  pinMode(TOUCH_GT911_RST, OUTPUT);
  digitalWrite(TOUCH_GT911_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_GT911_RST, HIGH);
  delay(10);
  touch_init();

  screenWidth = gfx->width();
  screenHeight = gfx->height();

#ifdef ESP32
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
  disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4);
#endif

  if (!disp_draw_buf)
  {
    Serial.println("LVGL Buffer Allocation Failed!");
    while(1) delay(100);
  }

  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 4);

  /* Setup Display Driver */
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* Setup Input Touch Driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  /* --- HACK IN YOUR CUSTOM UI / FILES HERE --- */
  // If using an exported SquareLine studio framework, comment out the lines below and call:
  // ui_init(); 
  
  // Temporary placeholders for validation:
  lbl_battery = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_28, 0);
  lv_obj_align(lbl_battery, LV_ALIGN_CENTER, 0, -50);
  lv_label_set_text(lbl_battery, "Waiting for SmartShunt BLE...");

  lbl_solar = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(lbl_solar, &lv_font_montserrat_28, 0);
  lv_obj_align(lbl_solar, LV_ALIGN_CENTER, 0, 50);
  lv_label_set_text(lbl_solar, "Waiting for MPPT BLE...");
  /* ------------------------------------------- */

  /* --- INITIALIZE VICTRON DECRYPTION --- */
  // Replace with your real Victron MAC addresses and extracted 32-character Hex AES keys
  victron.addDevice("SmartShunt", "aa:bb:cc:dd:ee:ff", "00112233445566778899aabbccddeeff");
  victron.addDevice("SmartMPPT",  "11:22:33:44:55:66", "ffeeddccbbaa99887766554433221100");
  
  victron.setCallback(onVictronBleData);
  victron.begin(); // Fires up background parsing tasks asynchronously on Core 0
  
  Serial.println("System Engine Live.");
}

void loop()
{
  lv_timer_handler(); /* Let the GUI engine refresh objects */
  
  // Thread-safely pipe the background data metrics to your active screen widgets
  static uint32_t lastWidgetRefresh = 0;
  if (millis() - lastWidgetRefresh > 1000) {
      if (sharedMetrics.dataReady) {
          
          // --- HACK IN YOUR LVGL LABELS/METERS DATA RETRIEVAL HERE ---
          // Replace these placeholder calls with your custom UI setters (e.g. lv_bar_set_value)
          lv_label_set_text_fmt(lbl_battery, "Battery: %.2fV  |  %.2fA  |  %.1f%%", 
                                sharedMetrics.voltage, sharedMetrics.current, sharedMetrics.soc);
                                
          lv_label_set_text_fmt(lbl_solar, "Solar Production: %.0f Watts", 
                                sharedMetrics.power);
          
          sharedMetrics.dataReady = false; // reset transaction flag
      }
      lastWidgetRefresh = millis();
  }
  
  delay(5);
}