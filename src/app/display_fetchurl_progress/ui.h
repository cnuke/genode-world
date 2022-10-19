// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.1.0
// LVGL VERSION: 8.2
// PROJECT: SquareLine_Project

#ifndef _SQUARELINE_PROJECT_UI_H
#define _SQUARELINE_PROJECT_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined __has_include
#if __has_include("lvgl.h")
#include "lvgl.h"
#elif __has_include("lvgl/lvgl.h")
#include "lvgl/lvgl.h"
#else
#include "lvgl.h"
#endif
#else
#include "lvgl.h"
#endif

extern lv_obj_t * ui_Screen1;
extern lv_obj_t * ui_ProgressBar;
extern lv_obj_t * ui_UriLabel;
extern lv_obj_t * ui_ActivitySpinner;
extern lv_obj_t * ui_PercentLabel;
extern lv_obj_t * ui_PercentSignLabel;
extern lv_obj_t * ui_DoneLabel;


void ui_init(char const *url);
void ui_update_url(char const *url);
void ui_update_progress(int percent, char const *string);
void ui_show_done(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
