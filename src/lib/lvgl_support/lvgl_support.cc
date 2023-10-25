/*
 * \brief   LVGL support library
 * \author  Josef Soentgen
 * \date    2022-10-19
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* local includes */
#include "lvgl_support.h"

/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <base/log.h>
#include <base/heap.h>
#include <gui_session/connection.h>
#include <input_session/connection.h>
#include <timer_session/connection.h>
#include <util/reconstructible.h>
#include <libc/component.h>

/* library includes */
#include <lvgl.h>
#include <misc/lv_color.h>

namespace Util {

	template <typename T1, typename T2>
	static constexpr T1 max(T1 v1, T2 v2) { return v1 > v2 ? v1 : v2; }

	template <typename T1, typename T2>
	static constexpr T1 min(T1 v1, T2 v2) { return v1 < v2 ? v1 : v2; }

	template <typename T1, typename T2>
	static constexpr T1 clamp_greater(T1 v1, T2 v2) { return v1 > v2 ? v2 : v1; }

	template <typename T1, typename T2>
	static constexpr T1 clamp_lesser(T1 v1, T2 v2) { return v1 < v2 ? v2 : v1; }

	template <typename T>
	static constexpr T abs(T value) { return value >= 0 ? value : -value; }

	template <typename T>
	static constexpr bool unsigned_overflow(T x, T y) { return (y && x > (T)-1/y); }

	template <typename T>
	static constexpr T delta(T x, T y) { return max(x, y) - min(x, y); }

	template <typename T, size_t C>
	struct Queue
	{
		unsigned int _head { 0 };
		unsigned int _tail { 0 };

		T _items[C] { };

		template <typename SUCC, typename FAIL>
		void pop(SUCC const &succ, FAIL const &fail)
		{
			unsigned const d = Util::delta(_head, _tail);
			if (d > 0) {
				succ(_items[_head]);
				_head = (_head + 1) % C;
			} else
				fail();
		}

		template <typename SUCC, typename FAIL>
		void push(SUCC const &succ, FAIL const &fail)
		{
			unsigned const d = Util::delta(_tail, _head);
			if (d > 0 || _head == _tail) {
				succ(_items[_tail]);
				_tail = (_tail + 1) % C;
			} else
				fail();
		}
	};
} /* namespace Util */


struct Platform
{
	Genode::Env &_env;

	Gui::Connection           _gui;
	Gui::Session::View_handle _view { _gui.create_view() };
	Framebuffer::Mode         _mode;

	Input::Session_client &_input { *_gui.input() };

	struct Key_event
	{
		uint32_t codepoint;
		bool     pressed;
	};

	Util::Queue<Key_event, 32> _key_events;

	struct Mouse_event
	{
		int  x;
		int  y;
		bool left_pressed;
	};
	Mouse_event _mouse_event { 0, 0, false };

	Genode::Constructible<Genode::Attached_dataspace> _fb_ds { };
	uint8_t *_framebuffer { nullptr };
	size_t _size { 0 };

	Genode::Signal_handler<Platform> _sigh {
		_env.ep(), *this, &Platform::_handle_mode_change };

	void _handle_mode_change()
	{
		/* dummy signal handler to active resizing */
	}

	Platform(Genode::Env       &env,
	         Framebuffer::Mode  mode,
	         bool               allow_resize,
	         char const        *title)
	:
		_env { env },
		_gui { _env, title ? title : "LVGL" },
		_mode { mode }
	{
		/* register handler early, otherwise resizing seems to has issues */
		if (allow_resize)
			_gui.mode_sigh(_sigh);

		_gui.buffer(_mode, false);

		/* XXX deduplicate */
		_fb_ds.construct(_env.rm(), _gui.framebuffer()->dataspace());
		_framebuffer = _fb_ds->local_addr<uint8_t>();
		size_t const size = mode.area.w() * mode.area.h() * 4;
		_size = size;

		using Command = Gui::Session::Command;
		using namespace Gui;

		_gui.enqueue<Command::Geometry>(_view, Gui::Rect(Gui::Point(0, 0),
		                                                 _mode.area));
		_gui.enqueue<Command::To_front>(_view, Gui::Session::View_handle());
		// _gui.enqueue<Command::Title>(_view, title ? title : "unknown-lvgl-window");
		_gui.execute();
	}

	void update_mode(Framebuffer::Mode mode)
	{
		if (_fb_ds.constructed())
			_fb_ds.destruct();
		_framebuffer = nullptr;

		_gui.buffer(mode, false);

		/* XXX deduplicate */
		_fb_ds.construct(_env.rm(), _gui.framebuffer()->dataspace());
		_framebuffer = _fb_ds->local_addr<uint8_t>();
		_mode = mode;
		size_t const size = mode.area.w() * mode.area.h() * 4;
		_size = size;

		using Command = Gui::Session::Command;
		using namespace Gui;

		_gui.enqueue<Command::Geometry>(_view, Gui::Rect(Gui::Point(0, 0),
		                                                 mode.area));
		_gui.execute();
	}

	~Platform()
	{
		_env.rm().detach(_framebuffer);
	}

	void refresh(int x, int y, int w, int h)
	{
		_gui.framebuffer()->refresh(x, y, w, h);
	}

	unsigned int *buffer() const { return (unsigned int*)_framebuffer; }

	template <typename FN>
	void with_framebuffer(FN const &fn)
	{
		fn((unsigned int*)_framebuffer, _size);
	}

	void update_input()
	{
		_input.for_each_event([&] (Input::Event const &curr) {

			curr.handle_touch([&] (Input::Touch_id id, float x, float y) {
				_mouse_event.x = (int)x;
				_mouse_event.y = (int)y;
				_mouse_event.left_pressed = true;
			});

			curr.handle_touch_release([&] (Input::Touch_id id) {
				_mouse_event.left_pressed = false;
			});

			curr.handle_absolute_motion([&] (int x, int y) {
				_mouse_event.x = x;
				_mouse_event.y = y;
			});

			auto mouse_button = [] (Input::Keycode key) {
			return key >= Input::BTN_MISC && key <= Input::BTN_GEAR_UP; };

			curr.handle_press([&] (Input::Keycode key, Genode::Codepoint codepoint) {

				if (mouse_button(key)) {
					_mouse_event.left_pressed = (key == Input::BTN_LEFT);
				} else {

					auto success = [&] (Platform::Key_event &event) {
						event.codepoint = codepoint.value;
						event.pressed   = true;
					};

					auto fail = [&] () {
						Genode::warning("dropped pressed ", Genode::Hex(codepoint.value));
					};

					_key_events.push(success, fail);
				}
			});

			curr.handle_release([&] (Input::Keycode key) {

				if (mouse_button(key)) {
					_mouse_event.left_pressed = !(key == Input::BTN_LEFT);
				} else {

					/* AFAICT lvgl does not core about the code when releasing */
					auto success = [&] (Platform::Key_event &event) {
						event.codepoint = Genode::Codepoint::INVALID;
						event.pressed   = false;
					};

					auto fail = [&] () {
						Genode::warning("dropped released ", Genode::Hex((unsigned)key));
					};

					_key_events.push(success, fail);
				}
			});
		});
	}

	template <typename FN>
	void key_event(FN const &fn)
	{
		auto success = [&] (Platform::Key_event event) {
			fn(event);
		};
		auto fail = [&] () { /* Genode::warning("key event queue underrun"); */ };
		_key_events.pop(success, fail);
	}

	template <typename FN>
	void mouse_event(FN const &fn)
	{
		fn(_mouse_event);
	}
};


static void genode_disp_flush(lv_disp_drv_t       *disp_drv,
                              lv_area_t     const *area,
                              lv_color_t          *color_p)
{
	Platform &platform = *static_cast<Platform*>(disp_drv->user_data);

	platform.with_framebuffer([&] (unsigned int *dst, size_t size) {

		unsigned const width  = disp_drv->hor_res;
		unsigned const height = disp_drv->ver_res;

		if (size < (width * height * sizeof(unsigned int))) {
			Genode::warning("do not update display, framebuffer too small");
			return;
		}

		int32_t const act_x1 = Util::clamp_lesser(area->x1, 0);
		int32_t const act_y1 = Util::clamp_lesser(area->y1, 0);
		int32_t const act_x2 = Util::clamp_greater(area->x2, (int32_t)width - 1);
		int32_t const act_y2 = Util::clamp_greater(area->y2, (int32_t)height - 1);
		lv_coord_t const line_width = (act_x2 - act_x1 + 1);

		for(int32_t y = act_y1; y <= act_y2; y++) {
			long int const location = y * width /* line */ + act_x1 /* offset */;
			memcpy(&dst[location], (unsigned int *)color_p, line_width * 4);
			color_p += line_width;
		}
	});

	platform.refresh(area->x1, area->y1, area->x2, area->y2);
	lv_disp_flush_ready(disp_drv);
}


static void genode_keyboard_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
	Platform &platform = *static_cast<Platform*>(indev_drv->user_data);

	auto query = [&] (Platform::Key_event event) {
		data->key   = event.codepoint;
		data->state = event.pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
	};
	platform.key_event(query);
}


static void genode_mouse_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
	Platform &platform = *static_cast<Platform*>(indev_drv->user_data);

	auto query = [&] (Platform::Mouse_event event) {
		data->point.x = event.x;
		data->point.y = event.y;
		data->state   = event.left_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
	};
	platform.mouse_event(query);
}


struct Lvgl_support
{
	Genode::Env &_env;
	Genode::Heap _heap { _env.ram(), _env.rm() };

	Framebuffer::Mode _mode;
	Platform          _platform;

	/* lv stuff */
	lv_disp_drv_t  _disp_drv;
	lv_disp_t     *_disp;
    lv_indev_drv_t _mouse_drv;
	lv_indev_t    *_mouse_indev { nullptr };
	lv_obj_t      *_mouse_cursor { nullptr };

	lv_disp_draw_buf_t disp_buf1;
	lv_color_t *disp_buf1_backing = nullptr;

	Genode::Signal_handler<Lvgl_support> _mode_sigh {
		_env.ep(), *this, &Lvgl_support::_handle_mode_change };

	void _handle_mode_change()
	{
		Framebuffer::Mode const req_mode = _platform._gui.mode();

		_platform.update_mode(req_mode);

		// XXX disp_buf1 resizing?
		_disp_drv.hor_res = req_mode.area.w();
		_disp_drv.ver_res = req_mode.area.h();

		lv_disp_drv_update(_disp, &_disp_drv);
		_mode = req_mode;
	}

    lv_indev_drv_t _keyboard_drv;
	lv_indev_t    *_keyboard_indev { nullptr };

	Timer::Connection _timer { _env };

	uint64_t _last_ms { _timer.elapsed_ms() };

	Genode::Signal_handler<Lvgl_support> _sigh {
		_env.ep(), *this, &Lvgl_support::_handle_signals };

	bool _disarm_next { false };
	void _disarm_signal_handler()
	{
		_timer.sigh(Genode::Signal_context_capability());
		_platform._gui.framebuffer()->sync_sigh(Genode::Signal_context_capability());
		_platform._input.sigh(Genode::Signal_context_capability());
		_platform._gui.mode_sigh(Genode::Signal_context_capability());
	}

	void _handle_signals()
	{
		_platform.update_input();

		Lvgl::handle_component();

		uint64_t const cur_ms = _timer.elapsed_ms();
		unsigned const diff   = cur_ms - _last_ms;
		_last_ms = cur_ms;

		Lvgl::tick(diff);
	}

	Lvgl::Config _config { };

	void _resume(Lvgl::Config config)
	{
		if (config.use_keyboard || config.use_mouse) {
			_platform._input.sigh(_sigh);
		}

		if (config.use_periodic_timer) {
			_timer.sigh(_sigh);
			_timer.trigger_periodic(config.periodic_ms * 1000);
		}

		if (config.use_refresh_sync) {
			_platform._gui.framebuffer()->sync_sigh(_sigh);
		}

		if (config.allow_resize) {
			_platform._gui.mode_sigh(_mode_sigh);
		}
	}

	void _suspend(Lvgl::Config config)
	{
		if (config.use_keyboard || config.use_mouse) {
			_platform._input.sigh(_sigh);
		}

		if (config.use_periodic_timer) {
			_timer.sigh(_sigh);
			_timer.trigger_periodic(config.periodic_ms);
		}

		if (config.use_refresh_sync) {
			_platform._gui.framebuffer()->sync_sigh(_sigh);
		}

		if (config.allow_resize) {
			_platform._gui.mode_sigh(_mode_sigh);
		}
	}

	Lvgl_support(Genode::Env &env, Lvgl::Config config)
	:
		_env  { env },
		_mode { .area = { config.initial_width,
		                  config.initial_height } },
		_platform { _env, _mode, config.allow_resize, config.title },
		_config { config }
	{
		disp_buf1_backing = (lv_color_t*)_heap.alloc(config.initial_width * 10);

		lv_init();

		lv_disp_draw_buf_init(&disp_buf1, disp_buf1_backing, NULL,
		                      config.initial_width * 10);

		lv_disp_drv_init(&_disp_drv);
		_disp_drv.draw_buf = &disp_buf1;
		_disp_drv.flush_cb = genode_disp_flush;
		_disp_drv.hor_res = config.initial_width;
		_disp_drv.ver_res = config.initial_height;
		_disp_drv.user_data = &_platform;

		_disp = lv_disp_drv_register(&_disp_drv);

		lv_theme_t * th = lv_theme_default_init(_disp,
		                                         lv_palette_main(LV_PALETTE_BLUE),
		                                         lv_palette_main(LV_PALETTE_RED),
		                                         LV_THEME_DEFAULT_DARK,
		                                         LV_FONT_DEFAULT);
		lv_disp_set_theme(_disp, th);

		lv_group_t * g = lv_group_create();
		lv_group_set_default(g);

		if (_config.use_mouse) {
			lv_indev_drv_init(&_mouse_drv);
			_mouse_drv.type = LV_INDEV_TYPE_POINTER;
			_mouse_drv.read_cb = genode_mouse_read;
			_mouse_drv.user_data = &_platform;
			_mouse_indev = lv_indev_drv_register(&_mouse_drv);
			// lv_indev_set_group(_mouse_indev, g);

			_mouse_cursor = lv_img_create(lv_scr_act());
			/* do not show mouse cursor */
			// LV_IMG_DECLARE(mouse_cursor_icon);
			// lv_img_set_src(_mouse_cursor, &mouse_cursor_icon);
			lv_indev_set_cursor(_mouse_indev, _mouse_cursor);
		}

		if (_config.use_keyboard) {
			lv_indev_drv_init(&_keyboard_drv);
			_keyboard_drv.type = LV_INDEV_TYPE_KEYPAD;
			_keyboard_drv.read_cb = genode_keyboard_read;
			_keyboard_drv.user_data = &_platform;

			_keyboard_indev = lv_indev_drv_register(&_keyboard_drv);
			lv_indev_set_group(_keyboard_indev, g);
		}

		_resume(_config);
	}

	void resume()
	{
		_resume(_config);
	}

	void suspend()
	{
		_suspend(_config);
	}
};


static Lvgl_support *_lvgl;


void Lvgl::init(Genode::Env &env, Lvgl::Config config)
{
	if (_lvgl) {
		struct Already_initialized { };
		throw Already_initialized();
	}

	static Lvgl_support inst(env, config);
	_lvgl = &inst;
}


void Lvgl::tick(unsigned ms)
{
	Libc::with_libc([&] {
		lv_tick_inc(ms);
		lv_task_handler();
	});
}


void Lvgl::resume()
{
	if (!_lvgl)
		return;

	_lvgl->resume();
}


void Lvgl::suspend()
{
	if (!_lvgl)
		return;

	_lvgl->suspend();
}
