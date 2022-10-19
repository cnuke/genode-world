// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.1.0
// LVGL VERSION: 8.2
// PROJECT: SquareLine_Project

#include "ui.h"
#include "ui_helpers.h"

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_Screen1;
lv_obj_t * ui_ProgressBar;
lv_obj_t * ui_UriLabel;
lv_obj_t * ui_ActivitySpinner;
lv_obj_t * ui_PercentLabel;
lv_obj_t * ui_PercentSignLabel;
lv_obj_t * ui_DoneLabel;

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 32
    #error "LV_COLOR_DEPTH should be 32bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=0
    #error "LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////

///////////////////// SCREENS ////////////////////
void ui_Screen1_screen_init(char const *url)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_ProgressBar = lv_bar_create(ui_Screen1);
    lv_bar_set_value(ui_ProgressBar, 0, LV_ANIM_OFF);
    lv_obj_set_width(ui_ProgressBar, 703);
    lv_obj_set_height(ui_ProgressBar, 65);
    lv_obj_set_align(ui_ProgressBar, LV_ALIGN_CENTER);

    ui_UriLabel = lv_label_create(ui_Screen1);
    lv_obj_set_width(ui_UriLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_UriLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_UriLabel, 0);
    lv_obj_set_y(ui_UriLabel, -75);
    lv_obj_set_align(ui_UriLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_UriLabel, url);
    lv_obj_set_style_text_font(ui_UriLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ActivitySpinner = lv_spinner_create(ui_Screen1, 1000, 90);
    lv_obj_set_width(ui_ActivitySpinner, 51);
    lv_obj_set_height(ui_ActivitySpinner, 51);
    lv_obj_set_x(ui_ActivitySpinner, 365);
    lv_obj_set_y(ui_ActivitySpinner, 85);
    lv_obj_set_align(ui_ActivitySpinner, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_ActivitySpinner, LV_OBJ_FLAG_CLICKABLE);      /// Flags

    ui_PercentLabel = lv_label_create(ui_Screen1);
    lv_obj_set_width(ui_PercentLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_PercentLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_PercentLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_PercentLabel, "0");
    lv_obj_set_style_text_align(ui_PercentLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_PercentLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PercentSignLabel = lv_label_create(ui_Screen1);
    lv_obj_set_width(ui_PercentSignLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_PercentSignLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_PercentSignLabel, 40);
    lv_obj_set_y(ui_PercentSignLabel, 0);
    lv_obj_set_align(ui_PercentSignLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_PercentSignLabel, "%");
    lv_obj_set_style_text_font(ui_PercentSignLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_DoneLabel = lv_label_create(ui_Screen1);
    lv_obj_set_width(ui_DoneLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_DoneLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_DoneLabel, 0);
    lv_obj_set_y(ui_DoneLabel, 0);
    lv_obj_set_align(ui_DoneLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_DoneLabel, "Download done");
    lv_obj_add_flag(ui_DoneLabel, LV_OBJ_FLAG_HIDDEN);     /// Flags
    lv_obj_set_style_text_align(ui_DoneLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_DoneLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);

}

void ui_init(char const *url)
{
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_Screen1_screen_init(url);
    lv_disp_load_scr(ui_Screen1);
}


void ui_update_url(char const *url)
{
	lv_label_set_text(ui_UriLabel, url);
}


static char buffer[8];

void ui_update_progress(int percent, char const *string)
{
	int i;

	if      (percent > 100) percent = 100;
	else if (percent < 0)   percent = 0;

	lv_bar_set_value(ui_ProgressBar, percent, LV_ANIM_OFF);

	if (percent < 100) {
		for (i = 0; i < sizeof (buffer) - 1 && (string && *string); i++) {
			buffer[i] = *string++;
		}
		buffer[i] = 0;

		lv_label_set_text(ui_PercentLabel, buffer);

		lv_obj_clear_flag(ui_PercentLabel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(ui_PercentSignLabel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(ui_ActivitySpinner, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_DoneLabel, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(ui_PercentLabel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_PercentSignLabel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_ActivitySpinner, LV_OBJ_FLAG_HIDDEN);

		lv_obj_clear_flag(ui_DoneLabel, LV_OBJ_FLAG_HIDDEN);
	}
}
