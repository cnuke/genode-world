/*
 * \brief   LittlevGL test
 * \author  Josef Soentgen
 * \date    2019-02-04
 */

/*
 * Copyright (C) 2019 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <libc/component.h>
#include <base/log.h>
#include <gui_session/connection.h>
#include <input_session/connection.h>
#include <timer_session/connection.h>
#include <util/reconstructible.h>

/* library includes */
#include <lvgl.h>
#include <misc/lv_color.h>

struct Platform
{
	Genode::Env &_env;

	Gui::Connection           _gui  { _env, "test-lvgl" };
	Gui::Session::View_handle _view { _gui.create_view() };
	Framebuffer::Mode         _mode;

	Input::Session_client &_input { *_gui.input() };

	Genode::Constructible<Genode::Attached_dataspace> _fb_ds { };
	uint8_t *_framebuffer { nullptr };

	Platform(Genode::Env &env) : _env(env)
	{
		_mode = Framebuffer::Mode { .area = { 1024, 768 } };

		_gui.buffer(_mode, false);

		_fb_ds.construct(_env.rm(), _gui.framebuffer()->dataspace());
		_framebuffer = _fb_ds->local_addr<uint8_t>();

		using Command = Gui::Session::Command;
		using namespace Gui;

		_gui.enqueue<Command::Geometry>(_view, Gui::Rect(Gui::Point(0, 0), _mode.area));
		_gui.enqueue<Command::To_front>(_view, Gui::Session::View_handle());
		_gui.enqueue<Command::Title>(_view, "webcam");
		_gui.execute();

	}

	~Platform()
	{
		_env.rm().detach(_framebuffer);
	}

	void refresh(int , int , int , int )
	{
		_gui.framebuffer()->refresh(0, 0, _mode.area.w(), _mode.area.h());
	}

	unsigned int *buffer() const { return (unsigned int*)_framebuffer; }
};


static Platform *global_platform;

static lv_disp_drv_t *global_display;

static void global_disp_flush(lv_disp_drv_t * disp_drv,
                              lv_area_t const * area,
                              lv_color_t * color_p)
{
	unsigned int *dst = global_platform->buffer();
	unsigned width    = global_platform->_mode.area.w();
	int32_t y;
	int32_t x;
	for(y = area->y1; y <= area->y2; y++) {
		for(x = area->x1; x <= area->x2; x++) {
			unsigned int c = lv_color_to32(*color_p);
			dst[y * width + x] = c;
			color_p++;
		}
	}

	global_platform->refresh(area->x1, area->y1, x, y);

	lv_disp_flush_ready(global_display);
}


static int  mouse_x;
static int  mouse_y;
static bool mouse_lb;
static uint32_t keyboard_key;
static bool keyboard_state;


static void update_input()
{
	/* XXX the events need to be buffered, we will lose key events */
	global_platform->_input.for_each_event([&] (Input::Event const &curr) {

		curr.handle_absolute_motion([&] (int x, int y) {
			mouse_x = x;
			mouse_y = y;
		});

		auto mouse_button = [] (Input::Keycode key) {
		return key >= Input::BTN_MISC && key <= Input::BTN_GEAR_UP; };

		curr.handle_press([&] (Input::Keycode key, Genode::Codepoint codepoint) {

			if (mouse_button(key)) {
				mouse_lb = (key == Input::BTN_LEFT);
			} else {
				keyboard_key = codepoint.value ? codepoint.value : Genode::Codepoint::INVALID;
				keyboard_state = true;
			}
		});

		curr.handle_release([&] (Input::Keycode key) {

			if (mouse_button(key)) {
				mouse_lb = !(key == Input::BTN_LEFT);
			} else {
				/* AFAICT lvgl does not core about the code when releasing */
				keyboard_key = Genode::Codepoint::INVALID;
				keyboard_state = false;
			}
		});
	});
}


void global_keyboard_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
	update_input();

	data->key   = keyboard_key;
	data->state = keyboard_state ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}


void global_mouse_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    (void) indev_drv;      /*Unused*/
	update_input();

	data->point.x = mouse_x;
	data->point.y = mouse_y;
	data->state   = mouse_lb ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}


#include <demos/lv_demos.h>
#include <lv_demo_music.h>

struct Main
{
	Genode::Env &_env;

	Platform _platform { _env };

	/* lv stuff */
	lv_disp_drv_t  _disp_drv;
    lv_indev_drv_t _mouse_drv;
	lv_indev_t    *_mouse_indev { nullptr };
	lv_obj_t      *_mouse_cursor { nullptr };

	enum {
		WIDTH = 1024,
		HEIGHT = 768,
	};

	lv_disp_draw_buf_t disp_buf1;
	lv_color_t buf1_1[WIDTH * 100];

    lv_indev_drv_t _keyboard_drv;
	lv_indev_t    *_keyboard_indev { nullptr };

	Timer::Connection _timer { _env };

	Genode::Signal_handler<Main> _timer_sigh {
		_env.ep(), *this, &Main::_handle_timer };

	enum { TIMER_US = 30000u };

	unsigned _last_ms { 0 };

	void _handle_timer()
	{
        _handle_signals();
	}

	Genode::Signal_handler<Main> _sigh {
		_env.ep(), *this, &Main::_handle_signals };

	void _handle_signals()
	{
		unsigned cur_ms = _timer.elapsed_ms();
		unsigned diff_ms = cur_ms - _last_ms;
		_last_ms = cur_ms;

		Libc::with_libc([&]() {
			lv_tick_inc(diff_ms);
			lv_task_handler();
		});
	}

	Main(Genode::Env &env) : _env(env)
	{
		global_platform = &_platform;
		lv_init();

		lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, WIDTH * 100);

		lv_disp_drv_init(&_disp_drv);
		_disp_drv.draw_buf = &disp_buf1;
		_disp_drv.flush_cb = global_disp_flush;
		_disp_drv.hor_res = WIDTH;
		_disp_drv.ver_res = HEIGHT;

		lv_disp_t *disp = lv_disp_drv_register(&_disp_drv);
		global_display = &_disp_drv;

		lv_theme_t * th = lv_theme_default_init(disp,
		                                        lv_palette_main(LV_PALETTE_BLUE),
		                                        lv_palette_main(LV_PALETTE_RED),
		                                        LV_THEME_DEFAULT_DARK,
		                                        LV_FONT_DEFAULT);
		lv_disp_set_theme(disp, th);

		lv_group_t * g = lv_group_create();
		lv_group_set_default(g);

		lv_indev_drv_init(&_mouse_drv);
		_mouse_drv.type = LV_INDEV_TYPE_POINTER;
		_mouse_drv.read_cb = global_mouse_read;
		_mouse_indev = lv_indev_drv_register(&_mouse_drv);
		// lv_indev_set_group(_mouse_indev, g);

		LV_IMG_DECLARE(mouse_cursor_icon);
		_mouse_cursor = lv_img_create(lv_scr_act());
		lv_img_set_src(_mouse_cursor, &mouse_cursor_icon);
		lv_indev_set_cursor(_mouse_indev, _mouse_cursor);

		lv_indev_drv_init(&_keyboard_drv);
		_keyboard_drv.type = LV_INDEV_TYPE_KEYPAD;;
		_keyboard_drv.read_cb = global_keyboard_read;
		_keyboard_indev = lv_indev_drv_register(&_keyboard_drv);
		lv_indev_set_group(_keyboard_indev, g);

		_timer.sigh(_timer_sigh);
		// _timer.trigger_periodic(TIMER_US);

		_last_ms = _timer.elapsed_ms();
		_platform._gui.framebuffer()->sync_sigh(_sigh);
		_platform._input.sigh(_sigh);

		lv_demo_music();
	}
};


void Libc::Component::construct(Libc::Env &env)
{
	/* holzhammer */
	Libc::with_libc([&] {
		static Main main(env);
	});
}
