/*
 * \brief   LVGL support library
 * \author  Josef Soentgen
 * \author  Johannes Schlatow
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

namespace Util {

	template <typename T1, typename T2>
	static constexpr T1 max(T1 v1, T2 v2) { return v1 > v2 ? v1 : v2; }

	template <typename T1, typename T2>
	static constexpr T1 min(T1 v1, T2 v2) { return v1 < v2 ? v1 : v2; }

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


struct Attached_framebuffer
{
	Genode::Env                &_env;
	Genode::Attached_dataspace  _fb_ds;
	size_t                      _size;

	Attached_framebuffer(Genode::Env                & env,
	                     Framebuffer::Mode            mode,
	                     Genode::Dataspace_capability ds)
	: _env(env),
	  _fb_ds(_env.rm(), ds),
	  _size(mode.area.w() * mode.area.h() * 4)
	{ }

	~Attached_framebuffer()
	{
		_env.rm().detach(_fb_ds.local_addr<void>());
	}

	void  *base() { return _fb_ds.local_addr<void>(); }
	size_t size() { return _size; }
};


struct Platform
{
	Genode::Env &_env;

	Gui::Connection         & _gui;
	Gui::Session::View_handle _view { _gui.create_view() };
	Framebuffer::Mode         _mode;

	Input::Session_client &_input { *_gui.input() };

	struct Key_event
	{
		uint32_t codepoint;
		bool     pressed;
	};

	Util::Queue<Key_event, 32> _key_events { };

	struct Mouse_event
	{
		int  x;
		int  y;
		bool left_pressed;
	};
	Mouse_event _mouse_event { 0, 0, false };

	Genode::Constructible<Attached_framebuffer> _fb { };

	Genode::Signal_handler<Platform> _sigh {
		_env.ep(), *this, &Platform::_handle_mode_change };

	void _handle_mode_change()
	{
		/* dummy signal handler to activate resizing */
	}

	Platform(Genode::Env       &env,
	         Gui::Connection   &gui,
	         Framebuffer::Mode  mode,
	         bool               allow_resize,
	         char const        *title)
	:
		_env { env },
		_gui { gui },
		_mode { mode }
	{
		/* register handler early, otherwise resizing seems to has issues */
		if (allow_resize)
			_gui.mode_sigh(_sigh);

		_gui.buffer(_mode, false);

		_fb.construct(_env, mode, _gui.framebuffer()->dataspace());

		using Command = Gui::Session::Command;
		using namespace Gui;

		_gui.enqueue<Command::Geometry>(_view, Gui::Rect(Gui::Point(0, 0),
		                                                 _mode.area));
		_gui.enqueue<Command::To_front>(_view, Gui::Session::View_handle());
		_gui.enqueue<Command::Title>(_view, title ? title : "unknown-lvgl-window");
		_gui.execute();
	}

	void update_mode(Framebuffer::Mode mode)
	{
		if (_fb.constructed())
			_fb.destruct();

		_gui.buffer(mode, false);

		_fb.construct(_env, mode, _gui.framebuffer()->dataspace());
		_mode = mode;

		using Command = Gui::Session::Command;
		using namespace Gui;

		_gui.enqueue<Command::Geometry>(_view, Gui::Rect(Gui::Point(0, 0),
		                                                 mode.area));
		_gui.execute();
	}

	void *swap(unsigned int w, unsigned int h)
	{
		using Command = Gui::Session::Command;

		static int swap = 0;

		Gui::Point const point { 0, int(swap * h) };
		Gui::Area  const area  { w, h };

		void *next_fb = ((char*)_fb->base()) + !swap * (_fb->size() / 2);

		swap ^= 1;

		_gui.enqueue<Command::Geometry>(_view, Gui::Rect(Gui::Point(0, 0),
		                                                 area));
		_gui.enqueue<Command::Offset>(_view, point);
		_gui.execute();

		_gui.framebuffer()->refresh(point.x(), -point.y(),
		                            area.w(), area.h());

		return next_fb;
	}

	void refresh(int x, int y, int w, int h)
	{
		_gui.framebuffer()->refresh(x, y, w, h);
	}

	template <typename FN>
	void framebuffer(FN const & fn)
	{
		fn(_fb->base(), _fb->size());
	}

	bool _sync_pending { false };

	template <typename FN>
	void wait_for_sync(FN const & fn)
	{
		for (;;) {
			_env.ep().wait_and_dispatch_one_io_signal();
			if (_sync_pending)
				break;
		}

		_sync_pending = false;
		fn();
	}

	void sync_pending()
	{
		_sync_pending = true;
	}

	void update_input()
	{
		_input.for_each_event([&] (Input::Event const &curr) {

			/*
			 * Treat touch as mouse for now, only handling the first finger.
			 */
			curr.handle_touch([&] (Input::Touch_id id, float x, float y) {
				if (id.value != 0)
					return;

				_mouse_event.x = (int)x;
				_mouse_event.y = (int)y;
				_mouse_event.left_pressed = true;
			});

			curr.handle_touch_release([&] (Input::Touch_id id) {

				if (id.value != 0)
					return;

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


static void genode_display_flush_wait(lv_disp_drv_t *disp_drv)
{

	Platform &platform = *static_cast<Platform*>(disp_drv->user_data);

	platform.wait_for_sync([&] () {
		lv_disp_t *disp = lv_disp_get_default();
		lv_coord_t const vres = lv_disp_get_ver_res(disp);
		lv_coord_t const hres = lv_disp_get_hor_res(disp);

		platform.refresh(0, 0, hres, vres);
	});
}


static void genode_disp_flush(lv_disp_drv_t       *disp_drv,
                              lv_area_t     const *area,
                              lv_color_t          *color_p)
{
	Platform &platform = *static_cast<Platform*>(disp_drv->user_data);

	lv_disp_t *disp = lv_disp_get_default();
	lv_coord_t const vres = lv_disp_get_ver_res(disp);
	lv_coord_t const hres = lv_disp_get_hor_res(disp);

	void *next_fb = platform.swap(hres, vres);

	// XXX direct mode w/o full refresh: disp->inv_areas, disp->inv_area_joined, disp->inv_p

	lv_disp_flush_ready(disp_drv);

	// genode_display_flush_wait(disp_drv);
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


class Lvgl_support
{
	private:
		
		Genode::Env &_env;
		Genode::Heap _heap { _env.ram(), _env.rm() };

		Gui::Connection   _gui;
		Framebuffer::Mode _mode;
		Platform          _platform;

		Timer::Connection _timer   { _env };
		uint64_t          _last_ms { _timer.elapsed_ms() };

		Lvgl::Config _config { };

		/* lv stuff */
		lv_disp_drv_t  _disp_drv     { };
		lv_disp_t     *_disp         { nullptr };
		lv_indev_drv_t _mouse_drv    { };
		lv_indev_t    *_mouse_indev  { nullptr };
		lv_obj_t      *_mouse_cursor { nullptr };

		lv_disp_draw_buf_t  _disp_buf1 { };

		void _setup_draw_buffer(lv_disp_draw_buf_t &buffer)
		{
			_platform.framebuffer([&] (void *base, size_t size) {
				lv_disp_draw_buf_init(&buffer, base, (char*)base + (size / 2), size / 2);
				// lv_disp_draw_buf_init(&buffer, base, NULL, size / 2);
			});
		}

		lv_coord_t _get_hor_res(Framebuffer::Mode mode)
		{
			return mode.area.w();
		}

		lv_coord_t _get_ver_res(Framebuffer::Mode mode)
		{
			return mode.area.h() / 2;
		}

		lv_indev_drv_t _keyboard_drv   { };
		lv_indev_t    *_keyboard_indev { nullptr };

		Genode::Signal_handler<Lvgl_support> _sigh {
			_env.ep(), *this, &Lvgl_support::_handle_signals };
		
		Genode::Signal_handler<Lvgl_support> _timer_sigh {
			_env.ep(), *this, &Lvgl_support::_handle_timer };

		Genode::Io_signal_handler<Lvgl_support> _sync_sigh {
			_env.ep(), *this, &Lvgl_support::_handle_sync };

		Genode::Signal_handler<Lvgl_support> _mode_sigh {
			_env.ep(), *this, &Lvgl_support::_handle_mode_change };

		void _handle_signals()
		{
			_platform.update_input();

			uint64_t const cur_ms = _timer.elapsed_ms();
			unsigned const diff   = cur_ms - _last_ms;
			_last_ms = cur_ms;

			Libc::with_libc([&] () {
				Lvgl::tick(diff); });
		}

		void _handle_timer()
		{
			if (_config.timer_callback)
				(*_config.timer_callback)();

			_handle_signals();
		}

		void _handle_sync()
		{
			_platform.sync_pending();

			_sigh.local_submit();
		}

		void _handle_mode_change()
		{
			Framebuffer::Mode const req_mode = _gui.mode();

			_platform.update_mode(req_mode);

			_disp_drv.hor_res = _get_hor_res(req_mode);
			_disp_drv.ver_res = _get_ver_res(req_mode);

			_setup_draw_buffer(_disp_buf1);

			lv_disp_drv_update(_disp, &_disp_drv);
			_mode = req_mode;

			if (_config.resize_callback)
				(*_config.resize_callback)();
		}

		void _resume(Lvgl::Config config)
		{
			if (config.use_keyboard || config.use_mouse) {
				_platform._input.sigh(_sigh);
			}

			if (config.use_periodic_timer) {
				_timer.sigh(_timer_sigh);
				_timer.trigger_periodic(config.periodic_ms * 1000);
			}

			if (config.use_refresh_sync) {
				_gui.framebuffer()->sync_sigh(_sync_sigh);
			}

			if (config.allow_resize) {
				_gui.mode_sigh(_mode_sigh);
			}
		}

		static Framebuffer::Mode _initial_mode(Gui::Connection & gui,
		                                       Lvgl::Config & config)
		{
			Framebuffer::Mode mode = gui.mode();

			if (config.allow_resize) {

				/* always prefer actual screen size (only if larger) */
				if (mode.area.w() < config.initial_width)
					mode.area = { config.initial_width, mode.area.h() };

				if (mode.area.h() < config.initial_height)
					mode.area = { mode.area.w(), config.initial_height };

			} else {

				/* prefer initial width/height (if != 0) */
				if (config.initial_width)
					mode.area = { config.initial_width, mode.area.h() };

				if (config.initial_height)
					mode.area = { mode.area.w(), config.initial_height };

			}

			mode.area = { mode.area.w(), mode.area.h() * 2 };

			return mode;
		}

		/* Noncopyable */
		Lvgl_support(Lvgl_support const &) = delete;
		Lvgl_support & operator=(Lvgl_support const &) = delete;

	public:

		Lvgl_support(Genode::Env &env, Lvgl::Config config)
		:
			_env  { env },
			_gui  { env, config.title ? config.title : "LVGL" },
			_mode { _initial_mode(_gui, config) },
			_platform { _env, _gui, _mode, config.allow_resize, config.title },
			_config { config }
		{
			lv_init();

			_setup_draw_buffer(_disp_buf1);

			lv_disp_drv_init(&_disp_drv);
			_disp_drv.draw_buf = &_disp_buf1;
			_disp_drv.flush_cb = genode_disp_flush;
			// _disp_drv.wait_cb = genode_display_flush_wait;
			_disp_drv.hor_res = _get_hor_res(_mode);
			_disp_drv.ver_res = _get_ver_res(_mode);
			_disp_drv.direct_mode = true;
			_disp_drv.full_refresh = true;
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
	lv_tick_inc(ms);
	lv_task_handler();
}
