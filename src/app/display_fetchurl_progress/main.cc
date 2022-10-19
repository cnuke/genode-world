/*
 * \brief   Display fetchurl progress via LVGL
 * \author  Josef Soentgen
 * \date    2022-10-19
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <libc/component.h>
#include <base/log.h>

/* library includes */
#include <lvgl.h>
#include <misc/lv_color.h>
#include <lvgl_support.h>

/* local includes */
#include "ui.h"

struct Main
{
	Genode::Env &_env;

	Genode::Attached_rom_dataspace _progress_rom { _env, "progress" };

	Genode::Signal_handler<Main> _progress_sigh {
		_env.ep(), *this, &Main::_handle_progress };

	using Url = Genode::String<512>;
	Url _url { "-" };

	float _download_total { 0.0f };
	float _download_now   { 0.0f };
	float _download_ratio { 0.0f };

	void _handle_progress()
	{
		_progress_rom.update();
		if (!_progress_rom.valid())
			return;

		Genode::Xml_node const node = _progress_rom.xml();
		if (!node.has_sub_node("fetch"))
			return;

		Genode::Xml_node const fetch_node = node.sub_node("fetch");

		Url url = fetch_node.attribute_value("url", Url("-"));
		if (url != _url) {
			_url = url;
			ui_update_url(_url.string());
		}

		_download_total = fetch_node.attribute_value("total",(double const&) _download_total);
		_download_now   = fetch_node.attribute_value("now",  (double const&) _download_now);

		if (_download_total)
			_download_ratio = _download_now / _download_total * 100;
	}

	void handle_component()
	{
		if (_url == "-") {
			return;
		}

		int const percent = int(_download_ratio + 0.5f);
		Genode::String<8> percent_string { percent };
		ui_update_progress(percent, percent_string.string());
	}

	Lvgl::Config config { .title              = "Fetchurl Progress",
	                      .initial_width      = 800,
	                      .initial_height     = 240,
	                      .allow_resize       = false,
	                      .use_refresh_sync   = false,
	                      .use_keyboard       = false,
	                      .use_mouse          = false,
	                      .use_periodic_timer = true,
	                      .periodic_ms        = 50u,
	};

	Main(Genode::Env &env) : _env(env)
	{
		Lvgl::init(_env, config);

		ui_init(_url.string());

		_progress_rom.sigh(_progress_sigh);
		_handle_progress();
	}
};


static Main *_main;


void Lvgl::handle_component()
{
	if (!_main) {
		return;
	}

	_main->handle_component();
}


void Libc::Component::construct(Libc::Env &env)
{
	/* holzhammer */
	Libc::with_libc([&] {
		static Main main(env);
		_main = &main;
	});
}
