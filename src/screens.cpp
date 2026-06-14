// Clean, manual coordinate layout definition
#include <Arduino_GFX_Library.h>
extern Arduino_GFX *gfx; 
#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts/fonts.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

#define ICON_SOLAR               "\xEE\x91\xB2"  // Sun icon
#define ICON_DCDC                "\xEE\x84\xA2"  // Caret circle arrow icon
#define ICON_LOADS               "\xEE\x8B\x9E"  // Lightning icon

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    
    // FIXED: Removed the two problematic flex layout engine configuration lines here
    // to allow absolute pixel positioning (lv_obj_set_pos) to align everything.

    {
        lv_obj_t *parent_obj = obj;
        {
            // energymonitortitle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.energymonitortitle = obj;
            lv_obj_set_pos(obj, 107, 17);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff9c9c9c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "ENERGY MONITOR");
        }
        {
            // solar_data_box
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.solar_display = obj;
            lv_obj_set_pos(obj, 19, 266);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff9c9c9c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // solarvoltagevolts
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.solarvoltagevolts = obj;
                    lv_obj_set_pos(obj, 20, 134);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276475c22ce3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Volts");
                }
                {
                    // solaramps
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.solaramps = obj;
                    lv_obj_set_pos(obj, 20, 96);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276475c22ce3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "AMPS");
                }
                {
                    // solarwattschange
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.solarwattschange = obj;
                    lv_obj_set_pos(obj, 20, 58);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276475c22ce3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "WATTS");
                }
                {
                    // solarvoltslabel
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.solarvoltslabel = obj;
                    lv_obj_set_pos(obj, 118, 141);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "VOLTS");
                }
                {
                    // solarwattlabel
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.solarwlabel = obj;
                    lv_obj_set_pos(obj, 118, 67);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "W");
                }
                {   //solar_icon
                    lv_obj_t *icon_obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(icon_obj, 30, 13); // Positioned to the left of the text
                    lv_obj_set_size(icon_obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(icon_obj, &Phosphor_Energy, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(icon_obj, ICON_SOLAR);
                } 
                {
                    // SOLAR_TITLE
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.mppt = obj;
                    lv_obj_set_pos(obj, 62, 17);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276894e81300, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "SOLAR");
                }
                {
                    // solarampslabel
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.solarampslabel = obj;
                    lv_obj_set_pos(obj, 118, 103);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "AMPS");
                }
            }
        }
        {
            // time_remaining_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.time_remaining_label = obj;
            lv_obj_set_pos(obj, 284, 397);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &font_f885947061f3803d80082776dbb5ffeb, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "TIME REMAINING: --:--");
        }
        {
            // batterylevel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.batterylevel = obj;
            lv_obj_set_pos(obj, 263, 100);
            lv_obj_set_size(obj, 275, 275);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // arc_1
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.arc_1 = obj;
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 250, 250);
                    lv_arc_set_bg_angles(obj, 135, 45);
                    lv_arc_set_range(obj, 0, 100);
                    lv_arc_set_value(obj, 50);
                }
                {
                // batterylevelpercent
                lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.battery = obj;
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d80082776dbb5ffeb, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "....%");
    
    // Dynamically centers the text within the bounds of the arc
    lv_obj_align_to(obj, objects.arc_1, LV_ALIGN_CENTER, 0, 0);
}
            }
        }
        {
            // load_data_box
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.loaddisplaycontainer = obj;
            lv_obj_set_pos(obj, 574, 143);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff9c9c9c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // loadwattslabel
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.loadwattslabel = obj;
                    lv_obj_set_pos(obj, 117, 67);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "W");
                }
                {
                    // loadwattsdisplay
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.loadwattsdisplay = obj;
                    lv_obj_set_pos(obj, 21, 60);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276475c22ce3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Watts");
                }
                {
                    // loadampslabel
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.loadampslabel = obj;
                    lv_obj_set_pos(obj, 117, 106);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "AMPS");
                }
                {// LOADS ICON
                    lv_obj_t *icon_obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(icon_obj, 24, 15);
                    lv_obj_set_size(icon_obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(icon_obj, &Phosphor_Energy, LV_PART_MAIN | LV_STATE_DEFAULT); // FIXED NAME
                    lv_label_set_text(icon_obj, ICON_LOADS);
                }
                {
                    // loadslabel
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.loadslabel = obj;
                    lv_obj_set_pos(obj, 63, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276894e81300, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "LOADS");
                }
                {
                    // loadamps
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.loadamps = obj;
                    lv_obj_set_pos(obj, 21, 100);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276475c22ce3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "AMPS");
                }
                {
                    // loadsvoltsdisplay
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.loadsvoltsdisplay = obj;
                    lv_obj_set_pos(obj, 31, 147);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276475c22ce3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "VOLTS");
                }
                {
                    // loadsvoltslabel
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.loadsvoltslabel = obj;
                    lv_obj_set_pos(obj, 117, 154);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Volts");
                }
            }
        }
        {
            // victrontitlelabel
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.victrontitlelabel = obj;
            lv_obj_set_pos(obj, 19, 17);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "VICTRON");
        }
        {
            // dcdc_data_box
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.dcdc_display = obj;
            lv_obj_set_pos(obj, 19, 50);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(obj, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff9c9c9c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_spread(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_x(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_ofs_y(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // dcdcampslabel
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.dcdcampslabel = obj;
                    lv_obj_set_pos(obj, 131, 114);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "AMPS");
                }
                {
                    // dcvolts
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.dcvolts = obj;
                    lv_obj_set_pos(obj, 131, 64);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d800827698f876eb1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "VOLTS");
                }
                {
                    // dcvoltsdisplay
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.dcvoltsdisplay = obj;
                    lv_obj_set_pos(obj, 8, 56);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276475c22ce3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "DCVolts");
                }
                {// DCDC ICON
                    lv_obj_t *icon_obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(icon_obj, 30, 16);
                    lv_obj_set_size(icon_obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(icon_obj, &Phosphor_Energy, LV_PART_MAIN | LV_STATE_DEFAULT); 
                    lv_label_set_text(icon_obj, ICON_DCDC);
                }
                {
                    // dc_dc title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.dc_dc = obj;
                    lv_obj_set_pos(obj, 70, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276894e81300, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "DCDC");
                }
                {
                    // dcamps
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.dcamps = obj;
                    lv_obj_set_pos(obj, 3, 108);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &font_f885947061f3803d8008276475c22ce3, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "DCAmps");
                }
            }
        }
    }

    tick_screen_main();
}

void tick_screen_main() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void delete_screen_main() {
    if (objects.main != NULL) {
        lv_obj_del(objects.main);
    }
    objects.main = 0;
    objects.energymonitortitle = 0;
    objects.solar_display = 0;
    objects.solarvoltagevolts = 0;
    objects.solaramps = 0;
    objects.solarwattschange = 0;
    objects.solarvoltslabel = 0;
    objects.solarwlabel = 0;
    objects.mppt = 0;
    objects.solarampslabel = 0;
    objects.time_remaining_label = 0;
    objects.batterylevel = 0;
    objects.battery = 0;
    objects.arc_1 = 0;
    objects.loaddisplaycontainer = 0;
    objects.loadwattslabel = 0;
    objects.loadwattsdisplay = 0;
    objects.loadampslabel = 0;
    objects.loadslabel = 0;
    objects.loadamps = 0;
    objects.loadsvoltsdisplay = 0;
    objects.loadsvoltslabel = 0;
    objects.victrontitlelabel = 0;
    objects.dcdc_display = 0;
    objects.dcdcampslabel = 0;
    objects.dcvolts = 0;
    objects.dcvoltsdisplay = 0;
    objects.dc_dc = 0;
    objects.dcamps = 0;
}

typedef void (*delete_screen_func_t)();
delete_screen_func_t delete_screen_funcs[] = {
    delete_screen_main,
};
void delete_screen_by_id(enum ScreensEnum screenId) {
    delete_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    create_screen_main();
}