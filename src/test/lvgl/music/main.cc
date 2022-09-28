/*
 * \brief   LVGL Demo Music Player
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

/* demo includes */
#include <demos/lv_demos.h>
#include <lv_demo_music.h>


struct Main
{
	Genode::Env &_env;

	Lvgl::Config config { .title              = "LVGL Demo Music Player",
	                      .initial_width      = 800,
	                      .initial_height     = 600,
	                      .allow_resize       = false,
	                      .use_refresh_sync   = true,
	                      .use_keyboard       = false,
	                      .use_mouse          = true,
	                      .use_periodic_timer = false,
	                      .periodic_ms        = 0u,
	};

	Main(Genode::Env &env) : _env(env)
	{
		Lvgl::init(_env, config);

		lv_demo_music();
	}
};


void Lvgl::handle_component() { }


void Libc::Component::construct(Libc::Env &env)
{
	/* holzhammer */
	Libc::with_libc([&] {
		static Main main(env);
	});
}
