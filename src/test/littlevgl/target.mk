TARGET := test-littlevgl
SRC_CC := main.cc
SRC_C  := mouse_cursor_icon.c
SRC_C  += demo.c img_bubble_pattern.c
SRC_C  += lv_test_theme_1.c
SRC_C  += lv_test_theme_2.c
SRC_C  += lv_tutorial_keyboard.c
LIBS   := base littlevgl libc

INC_DIR += $(PRG_DIR)

CC_CXX_WARN_STRICT :=
