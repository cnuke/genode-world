/*
 * \brief  Genode-specific timer backend
 * \author Josef Soentgen
 * \date   2016-01-05
 */

/*
 * Copyright (C) 2016 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <timer_session/connection.h>

/* SDL2 includes */
#include "SDL_internal.h"
#include "SDL_timer.h"


static unsigned long      _start_ms = 0;
static Timer::Connection *_timer;


extern "C" void SDL_TicksInit(void)
{
	if (_timer) return;

	_timer    = new (Genode::env()->heap()) Timer::Connection();
	_start_ms = _timer->elapsed_ms();
}


extern "C" void SDL_TicksQuit(void)
{
	Genode::destroy(Genode::env()->heap(), _timer);
	_timer = nullptr;
}


extern "C" Uint32 SDL_GetTicks(void)
{
	if (!_timer) SDL_TicksInit();

	return _timer->elapsed_ms() - _start_ms;
}


extern "C" Uint64 SDL_GetPerformanceCounter(void)
{
	return SDL_GetTicks();
}


extern "C" Uint64 SDL_GetPerformanceFrequency(void)
{
	return 1000;
}


extern "C" void SDL_Delay(Uint32 ms)
{
	if (!_timer) SDL_TicksInit();

	_timer->msleep(ms);
}
