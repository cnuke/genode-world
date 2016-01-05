/*
 * \brief  Genode-specific shared-object backend
 * \author Josef Soentgen
 * \date   2016-01-05
 */

/*
 * Copyright (C) 2015 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/env.h>
#include <base/shared_object.h>

/* SDL2 includes */
#include "SDL_config.h"
#include "SDL_loadso.h"

using namespace Genode;


void *SDL_LoadObject(const char *sofile)
{
	Shared_object *obj = nullptr;
	try {
		obj = new (Genode::env()->heap()) Shared_object(sofile, Shared_object::LAZY);
	} catch (...) {
		PERR("Could not open '%s'", sofile);
	}

	return obj;
}


void *SDL_LoadFunction(void *handle, const char *name)
{
	try         { return static_cast<Shared_object *>(handle)->lookup(name); }
	catch (...) { PERR("Could not lookup function '%s'"); }
	return nullptr;
}


void SDL_UnloadObject(void* handle)
{
	destroy(env()->heap(), static_cast<Shared_object *>(handle));
}
