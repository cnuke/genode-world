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
#include <lv_misc/lv_color.h>


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

static void global_disp_flush(int x1, int y1, int x2, int y2,
                              const lv_color_t * color_p)
{
	unsigned int *dst = global_platform->buffer();
	unsigned width    = global_platform->_mode.area.w();
	int32_t y;
	int32_t x;
	for(y = y1; y <= y2; y++) {
		for(x = x1; x <= x2; x++) {
			unsigned int c = lv_color_to32(*color_p);
			dst[y * width + x] = c;
			color_p++;
		}
	}

	global_platform->refresh(x1, y1, x, y2);

	lv_flush_ready();
}


static void global_disp_fill(int x1, int y1, int x2, int y2, lv_color_t color)
{
	Genode::warning(__func__, ":", __LINE__, " not implemented");
}



static void global_disp_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            const lv_color_t * color_p)
{
	Genode::warning(__func__, ":", __LINE__, " not implemented");
}


static void global_memory_monitor(void * param)
{
    (void) param; /*Unused*/

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);

	Genode::log("used: ",  (int)mon.total_size - mon.free_size, " ", mon.used_pct,
	            " frag: ", mon.frag_pct, " biggeset free: ", (int)mon.free_biggest_size);
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


bool global_keyboard_read(lv_indev_data_t * data)
{
	update_input();

	data->key   = keyboard_key;
	data->state = keyboard_state ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

	return false;
}


bool global_mouse_read(lv_indev_data_t * data)
{
	update_input();

	data->point.x = mouse_x;
	data->point.y = mouse_y;
	data->state   = mouse_lb ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

	return false;
}

#include <demo.h>
#include <lv_test_theme_1.h>
#include <lv_test_theme_2.h>
#include <lv_tutorial_keyboard.h>

struct Main
{
	Genode::Env &_env;

	Platform _platform { _env };

	/* lv stuff */
	lv_disp_drv_t  _disp_drv;
    lv_indev_drv_t _mouse_drv;
	lv_indev_t    *_mouse_indev { nullptr };
	lv_obj_t      *_mouse_cursor { nullptr };

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

		lv_disp_drv_init(&_disp_drv);
		_disp_drv.disp_flush = global_disp_flush;
		_disp_drv.disp_fill  = global_disp_fill;
		_disp_drv.disp_map   = global_disp_map;
		lv_disp_drv_register(&_disp_drv);

		lv_indev_drv_init(&_mouse_drv);
		_mouse_drv.type = LV_INDEV_TYPE_POINTER;
		_mouse_drv.read = global_mouse_read;
		_mouse_indev = lv_indev_drv_register(&_mouse_drv);
		LV_IMG_DECLARE(mouse_cursor_icon);
		_mouse_cursor = lv_img_create(lv_scr_act(), NULL);
		lv_img_set_src(_mouse_cursor, &mouse_cursor_icon);
		lv_indev_set_cursor(_mouse_indev, _mouse_cursor);

		lv_indev_drv_init(&_keyboard_drv);
		_keyboard_drv.type = LV_INDEV_TYPE_KEYPAD;;
		_keyboard_drv.read = global_keyboard_read;
		_keyboard_indev = lv_indev_drv_register(&_keyboard_drv);

		lv_task_create(global_memory_monitor, 3000, LV_TASK_PRIO_MID, NULL);

		_timer.sigh(_timer_sigh);
		// _timer.trigger_periodic(TIMER_US);

		_last_ms = _timer.elapsed_ms();
		_platform._gui.framebuffer()->sync_sigh(_sigh);
		_platform._input.sigh(_sigh);

		Libc::with_libc([&] {
			demo_create(_keyboard_indev);
			// lv_test_theme_1(lv_theme_night_init(15, NULL));
			// lv_test_theme_2();
			// lv_tutorial_keyboard(_keyboard_indev);
		});
	}
};

void Libc::Component::construct(Libc::Env &env)
{
	static Main main(env);
}
