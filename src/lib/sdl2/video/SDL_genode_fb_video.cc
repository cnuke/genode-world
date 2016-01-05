/*
 * \brief  Genode-specific video backend
 * \author Stefan Kalkowski
 * \date   2008-12-12
 */

/*
 * Copyright (c) <2008> Stefan Kalkowski
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include <base/printf.h>
#include <base/env.h>
#include <framebuffer_session/connection.h>

extern "C" {

#include "../../SDL_internal.h"

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_mouse.h>
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"
#include "SDL_genode_fb_events.h"
#include "SDL_genode_fb_video.h"


struct Genode_Fb_data
{
	Framebuffer::Connection *framebuffer;
	void                    *buffer;

	SDL_Rect *modes[2];
	SDL_Rect df_mode;
};


static inline Uint32 translate_mode(Framebuffer::Mode mode)
{
	switch (mode) {
	case Framebuffer::Mode::RGB565:
		return SDL_PIXELFORMAT_RGB565;
	default:
		return SDL_PIXELFORMAT_UNKNOWN
	}
}


static void Genode_Fb_InitDisplays(_THIS)
{
	Genode_Fb_data      *data = _this->driverdata;
	Framebuffer::Mode fb_mode = data->framebuffer->mode();

	SDL_VideoDisplay display;
	SDL_zero(display);

	SDL_DisplayMode mode;
	SDL_zero(mode);

	mode.format = translate_mode(fb_mode);

	if (mode.format == SDL_PIXELFORMAT_UNKNOWN) {
		PERR("Unknown pixel format");
		return;
	}

	mode.w            = fb_mode.width();
	mode.h            = fb_mode.height();
	mode.refresh_rate = 60;

	SDL_AddDisplayMode(&display, &mode);
	SDL_AddVideoDisplay(&display);
}


static int Genode_Fb_VideoInit(_THIS)
{
	Genode_Fb_data *data = _this->driverdata;

	if (data->framebuffer == nullptr)
		data->framebuffer = new (Genode::env()->heap()) Framebuffer::Connection();

	if (!data->framebuffer->cap().valid())
		return SDL_SetError("Could not get framebuffer session");

	/* Map the buffer */
	Genode::Dataspace_capability fb_ds_cap = framebuffer->dataspace();
	if (!fb_ds_cap.valid()) {
		Genode_Fb_VideoQuit(t);
		return SDL_SetError("Could not request dataspace for framebuffer");
	}

	data->buffer = Genode::env()->rm_session()->attach(fb_ds_cap);

	Genode_Fb_InitDisplays(_this);

	Genode_Fb_InitOSKeymap();

	return 0;
}


static void Genode_Fb_VideoQuit(_THIS)
{
	Genode_Fb_data *data = _this->driverdata;

	Genode_Fb_data::env()->rm_session()->detach(data->buffer);

	Genode::destroy(Genode::env()->heap(), data->framebuffer);
	data->framebuffer = nullptr;
}


static void Genode_Fb_GetDisplayModes(_THIS, SDL_VideoDisplay *sdl_display) {
	PDGB("Not implemented"); }


static void Genode_Fb_SetDisplayMode(_THIS, SDL_VideoDisplay *, SDL_DisplayMode *) {
	PDGB("Not implemented"); }


static void Genode_Fb_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->driverdata);
	SDL_free(device);
}


static int Genode_Fb_CreateWindow(_THIS, SDL_Window *window)
{
	Genode_Fb_data *data = _this->driverdata;

	window->driverdata = data;

	if (window->x == SDL_WINDOWPOS_UNDEFINED)
		window->x = 0;

	if (window->y == SDL_WINDOWPOS_UNDEFINED)
		window->y = 0;

	return 0;
}


static int Genode_Fb_CreateWindowFramebuffer(_THIS, SDL_Window* window, Uint32* format,
                                      void** pixels, int* pitch)
{
	Genode_Fb_data *data = _this->driverdata;
 
	Genode_Fb_CreateWindow(_this, window);

	*format = translate_mode(data->framebuffer.mode());
	if (*format == SDL_PIXELFORMAT_UNKNOWN)
		return SDL_SetError("Unknown pixel format");

	*pitch = (((window->w * SDL_BYTESPERPIXEL(*format)) + 3) & ~3);

	*pixels = SDL_malloc(window->h*(*pitch));
	if (*pixels == NULL)
		return SDL_OutOfMemory();

	return 0;
}


static int Genode_Fb_Available(void) { return 1; }


static SDL_VideoDevice *Genode_Fb_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;
	Genode_Fb_data  *data;

	device = (SDL_VideoDevice*) SDL_calloc(1, sizeof(SDL_VideoDevice));
	if (!device) {
		SDL_OutOfMemory();
		return nullptr;
	}

	data = (Genode_Fb_data*) SDL_calloc(1, sizeof(Genode_Fb_data));
	if (!data) {
		SDL_free(device);
		SDL_OutOfMemory();
		return nullptr;
	}

	device->driverdata = data;

	/* video */
	device->VideoInit        = Genode_Fb_VideoInit;
	device->VideoQuit        = Genode_Fb_VideoQuit;
	device->GetDisplayModes  = Genode_Fb_GetDisplayModes;
	device->SetDisplayMode   = Genode_Fb_SetDisplayMode;
	device->free             = Genode_Fb_DeleteDevice;

	device->InitOSKeymap     = Genode_Fb_InitOSKeymap;

	/* framebuffer */
	device->CreateWindowFramebuffer  = Genode_Fb_CreateWindowFramebuffer;
	device->UpdateWindowFramebuffer  = Genode_Fb_UpdateWindowFramebuffer;
	device->DestroyWindowFramebuffer = Genode_Fb_DestroyWindowFramebuffer;

	device->PumpEvents       = Genode_Fb_PumpEvents;

	/* window */
	device->CreateWindow         = nullptr;
	device->DestroyWindow        = nullptr;
	device->GetWindowWMInfo      = nullptr;
	device->SetWindowFullscreen  = nullptr;
	device->MaximizeWindow       = nullptr;
	device->MinimizeWindow       = nullptr;
	device->RestoreWindow        = nullptr;

	device->CreateWindowFrom     = nullptr;
	device->SetWindowTitle       = nullptr;
	device->SetWindowIcon        = nullptr;
	device->SetWindowPosition    = nullptr;
	device->SetWindowSize        = nullptr;
	device->SetWindowMinimumSize = nullptr;
	device->SetWindowMaximumSize = nullptr;
	device->ShowWindow           = nullptr;
	device->HideWindow           = nullptr;
	device->RaiseWindow          = nullptr;
	device->SetWindowBordered    = nullptr;
	device->SetWindowGammaRamp   = nullptr;
	device->GetWindowGammaRamp   = nullptr;
	device->SetWindowGrab        = nullptr;
	device->OnWindowEnter        = nullptr;

	/* opengl */
	device->GL_SwapWindow      = nullptr;
	device->GL_MakeCurrent     = nullptr;
	device->GL_CreateContext   = nullptr;
	device->GL_DeleteContext   = nullptr;
	device->GL_LoadLibrary     = nullptr;
	device->GL_UnloadLibrary   = nullptr;
	device->GL_GetSwapInterval = nullptr;
	device->GL_SetSwapInterval = nullptr;
	device->GL_GetProcAddress  = nullptr;

	device->SuspendScreenSaver = nullptr;

	device->StartTextInput   = nullptr;
	device->StopTextInput    = nullptr;
	device->SetTextInputRect = nullptr;

	device->HasScreenKeyboardSupport = nullptr;
	device->ShowScreenKeyboard       = nullptr;
	device->HideScreenKeyboard       = nullptr;
	device->IsScreenKeyboardShown    = nullptr;

	device->SetClipboardText = nullptr;
	device->GetClipboardText = nullptr;
	device->HasClipboardText = nullptr;

	device->ShowMessageBox = nullptr;

	return device;
}


VideoBootStrap Genode_fb_bootstrap = {
	"Genode_Fb", "SDL Genode Framebuffer video driver",
	Genode_Fb_Available, Genode_Fb_CreateDevice
};

} /* extern "C" */
