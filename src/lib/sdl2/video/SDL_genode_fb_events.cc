/*
 * \brief  Genode-specific event backend
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

/* Genode includes */
#include <base/printf.h>
#include <input_session/connection.h>
#include <input/event.h>
#include <input/keycodes.h>

/* SDL2 includes */
#include "SDL_internal.h"
#include "SDL_keycode.h"
#include "SDL_events_c.h"
#include "SDL_keyboard_c.h"
#include "SDL_sysevents.h"
#include "SDL_genode_fb_events.h"

static Input::Connection *input;
static Input::Event *ev_buf;
static const int KEYNUM_MAX = 512;
static SDL_Scancode keymap[KEYNUM_MAX];
static int buttonmap[KEYNUM_MAX];


static inline SDL_Scancode translate_key(int keycode) {
	return keymap[keycode]; }


void Genode_Fb_PumpEvents(SDL_VideoDevice *t)
{
	if (!input->is_pending()) return;

	SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
	window;

	int num_ev = input->flush();
	for (int src_ev_cnt = 0; src_ev_cnt < num_ev; src_ev_cnt++) {
		Input::Event curr = ev_buf[src_ev_cnt];

		switch(curr.type()) {
		case Input::Event::MOTION:
			if (curr.is_absolute_motion())
				SDL_SendMouseMotion(window, 0, 0, curr.ax(), curr.ay());
			else
				SDL_SendMouseMotion(window, 0, 1, curr.rx(), curr.ry());
			break;
		case Input::Event::PRESS:
			if(curr.code() >= Input::BTN_MISC &&
			   curr.code() <= Input::BTN_GEAR_UP)
				SDL_SendMouseButton(SDL_PRESSED, buttonmap[curr.code()], 0, 0);
			else
				SDL_SendKeyboardKey(SDL_PRESSED, translate_key(curr.code());
			break;
		case Input::Event::RELEASE:
			if(curr.code() >= Input::BTN_MISC &&
			   curr.code() <= Input::BTN_GEAR_UP)
				SDL_SendMouseButton(window, SDL_RELEASED, buttonmap[curr.code()]);
			else
				SDL_SendKeyboardKey(SDL_RELEASED, translate_key(curr.code());
			break;
		case Input::Event::WHEEL:
			PWRN("Mouse wheel, not implemented yet!");
			// SDL_SendMouseWheel(window, 0, curr.ry(), curr.rx(), SDL_MOUSEWHEEL_NORMAL);
			break;
		default:
			break;
		}
	}
}


void Genode_Fb_InitOSKeymap(SDL_VideoDevice *t)
{
	using namespace Input;
	input = new (Genode::env()->heap()) Connection();

	if (!input->cap().valid()) {
		PERR("No input driver available!");
		return;
	}

	/* Attach event buffer to address space */
	ev_buf = Genode::env()->rm_session()->attach(input->dataspace());

	/* Prepare button mappings */
	for (int i=0; i<KEYNUM_MAX; i++) {
		switch(i) {
		case BTN_LEFT: buttonmap[i]=SDL_BUTTON_LEFT; break;
		case BTN_RIGHT: buttonmap[i]=SDL_BUTTON_RIGHT; break;
		case BTN_MIDDLE: buttonmap[i]=SDL_BUTTON_MIDDLE; break;
		case BTN_0:
		case BTN_1:
		case BTN_2:
		case BTN_3:
		case BTN_4:
		case BTN_5:
		case BTN_6:
		case BTN_7:
		case BTN_8:
		case BTN_9:
		case BTN_SIDE:
		case BTN_EXTRA:
		case BTN_FORWARD:
		case BTN_BACK:
		case BTN_TASK:
		case BTN_TRIGGER:
		case BTN_THUMB:
		case BTN_THUMB2:
		case BTN_TOP:
		case BTN_TOP2:
		case BTN_PINKIE:
		case BTN_BASE:
		case BTN_BASE2:
		case BTN_BASE3:
		case BTN_BASE4:
		case BTN_BASE5:
		case BTN_BASE6:
		case BTN_DEAD:
		case BTN_A:
		case BTN_B:
		case BTN_C:
		case BTN_X:
		case BTN_Y:
		case BTN_Z:
		case BTN_TL:
		case BTN_TR:
		case BTN_TL2:
		case BTN_TR2:
		case BTN_SELECT:
		case BTN_START:
		case BTN_MODE:
		case BTN_THUMBL:
		case BTN_THUMBR:
		case BTN_TOOL_PEN:
		case BTN_TOOL_RUBBER:
		case BTN_TOOL_BRUSH:
		case BTN_TOOL_PENCIL:
		case BTN_TOOL_AIRBRUSH:
		case BTN_TOOL_FINGER:
		case BTN_TOOL_MOUSE:
		case BTN_TOOL_LENS:
		case BTN_TOUCH:
		case BTN_STYLUS:
		case BTN_STYLUS2:
		case BTN_TOOL_DOUBLETAP:
		case BTN_TOOL_TRIPLETAP:
		case BTN_GEAR_DOWN:
		case BTN_GEAR_UP:
		default: buttonmap[i]=0; break;
		}
	}

	/* Prepare key mappings */
	for (int i=0; i<KEYNUM_MAX; i++) {
		switch(i) {
		case KEY_RESERVED: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_ESC: keymap[i]=SDL_SCANCODE_ESCAPE; break;
		case KEY_1: keymap[i]=SDL_SCANCODE_1; break;
		case KEY_2: keymap[i]=SDL_SCANCODE_2; break;
		case KEY_3: keymap[i]=SDL_SCANCODE_3; break;
		case KEY_4: keymap[i]=SDL_SCANCODE_4; break;
		case KEY_5: keymap[i]=SDL_SCANCODE_5; break;
		case KEY_6: keymap[i]=SDL_SCANCODE_6; break;
		case KEY_7: keymap[i]=SDL_SCANCODE_7; break;
		case KEY_8: keymap[i]=SDL_SCANCODE_8; break;
		case KEY_9: keymap[i]=SDL_SCANCODE_9; break;
		case KEY_0: keymap[i]=SDL_SCANCODE_0; break;
		case KEY_MINUS: keymap[i]=SDL_SCANCODE_MINUS; break;
		case KEY_EQUAL: keymap[i]=SDL_SCANCODE_EQUALS; break;
		case KEY_BACKSPACE: keymap[i]=SDL_SCANCODE_BACKSPACE; break;
		case KEY_TAB: keymap[i]=SDL_SCANCODE_TAB; break;
		case KEY_Q: keymap[i]=SDL_SCANCODE_Q; break;
		case KEY_W: keymap[i]=SDL_SCANCODE_W; break;
		case KEY_E: keymap[i]=SDL_SCANCODE_E; break;
		case KEY_R: keymap[i]=SDL_SCANCODE_R; break;
		case KEY_T: keymap[i]=SDL_SCANCODE_T; break;
		case KEY_Y: keymap[i]=SDL_SCANCODE_Y; break;
		case KEY_U: keymap[i]=SDL_SCANCODE_U; break;
		case KEY_I: keymap[i]=SDL_SCANCODE_I; break;
		case KEY_O: keymap[i]=SDL_SCANCODE_O; break;
		case KEY_P: keymap[i]=SDL_SCANCODE_P; break;
		case KEY_LEFTBRACE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_RIGHTBRACE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_ENTER: keymap[i]=SDL_SCANCODE_RETURN; break;
		case KEY_LEFTCTRL: keymap[i]=SDL_SCANCODE_LCTRL; break;
		case KEY_A: keymap[i]=SDL_SCANCODE_A; break;
		case KEY_S: keymap[i]=SDL_SCANCODE_S; break;
		case KEY_D: keymap[i]=SDL_SCANCODE_D; break;
		case KEY_F: keymap[i]=SDL_SCANCODE_F; break;
		case KEY_G: keymap[i]=SDL_SCANCODE_G; break;
		case KEY_H: keymap[i]=SDL_SCANCODE_H; break;
		case KEY_J: keymap[i]=SDL_SCANCODE_J; break;
		case KEY_K: keymap[i]=SDL_SCANCODE_K; break;
		case KEY_L: keymap[i]=SDL_SCANCODE_L; break;
		case KEY_SEMICOLON: keymap[i]=SDL_SCANCODE_SEMICOLON; break;
		case KEY_APOSTROPHE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_GRAVE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_LEFTSHIFT: keymap[i]=SDL_SCANCODE_LSHIFT; break;
		case KEY_BACKSLASH: keymap[i]=SDL_SCANCODE_BACKSLASH; break;
		case KEY_Z: keymap[i]=SDL_SCANCODE_Z; break;
		case KEY_X: keymap[i]=SDL_SCANCODE_X; break;
		case KEY_C: keymap[i]=SDL_SCANCODE_C; break;
		case KEY_V: keymap[i]=SDL_SCANCODE_V; break;
		case KEY_B: keymap[i]=SDL_SCANCODE_B; break;
		case KEY_N: keymap[i]=SDL_SCANCODE_N; break;
		case KEY_M: keymap[i]=SDL_SCANCODE_M; break;
		case KEY_COMMA: keymap[i]=SDL_SCANCODE_COMMA; break;
		case KEY_DOT: keymap[i]=SDL_SCANCODE_PERIOD; break;
		case KEY_SLASH: keymap[i]=SDL_SCANCODE_SLASH; break;
		case KEY_RIGHTSHIFT: keymap[i]=SDL_SCANCODE_RSHIFT; break;
		case KEY_KPASTERISK: keymap[i]=SDL_SCANCODE_ASTERISK; break;
		case KEY_LEFTALT: keymap[i]=SDL_SCANCODE_LALT; break;
		case KEY_SPACE: keymap[i]=SDL_SCANCODE_SPACE; break;
		case KEY_CAPSLOCK: keymap[i]=SDL_SCANCODE_CAPSLOCK; break;
		case KEY_F1: keymap[i]=SDL_SCANCODE_F1; break;
		case KEY_F2: keymap[i]=SDL_SCANCODE_F2; break;
		case KEY_F3: keymap[i]=SDL_SCANCODE_F3; break;
		case KEY_F4: keymap[i]=SDL_SCANCODE_F4; break;
		case KEY_F5: keymap[i]=SDL_SCANCODE_F5; break;
		case KEY_F6: keymap[i]=SDL_SCANCODE_F6; break;
		case KEY_F7: keymap[i]=SDL_SCANCODE_F7; break;
		case KEY_F8: keymap[i]=SDL_SCANCODE_F8; break;
		case KEY_F9: keymap[i]=SDL_SCANCODE_F9; break;
		case KEY_F10: keymap[i]=SDL_SCANCODE_F10; break;
		case KEY_NUMLOCK: keymap[i]=SDL_SCANCODE_NUMLOCK; break;
		case KEY_SCROLLLOCK: keymap[i]=SDL_SCANCODE_SCROLLOCK; break;
		case KEY_KP7: keymap[i]=SDL_SCANCODE_KP7; break;
		case KEY_KP8: keymap[i]=SDL_SCANCODE_KP8; break;
		case KEY_KP9: keymap[i]=SDL_SCANCODE_KP9; break;
		case KEY_KPMINUS: keymap[i]=SDL_SCANCODE_KP_MINUS; break;
		case KEY_KP4: keymap[i]=SDL_SCANCODE_KP4; break;
		case KEY_KP5: keymap[i]=SDL_SCANCODE_KP5; break;
		case KEY_KP6: keymap[i]=SDL_SCANCODE_KP6; break;
		case KEY_KPPLUS: keymap[i]=SDL_SCANCODE_KP_PLUS; break;
		case KEY_KP1: keymap[i]=SDL_SCANCODE_KP1; break;
		case KEY_KP2: keymap[i]=SDL_SCANCODE_KP2; break;
		case KEY_KP3: keymap[i]=SDL_SCANCODE_KP3; break;
		case KEY_KP0: keymap[i]=SDL_SCANCODE_KP0; break;
		case KEY_KPDOT: keymap[i]=SDL_SCANCODE_KP_PERIOD; break;
		case KEY_ZENKAKUHANKAKU: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_102ND: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F11: keymap[i]=SDL_SCANCODE_F11; break;
		case KEY_F12: keymap[i]=SDL_SCANCODE_F12; break;
		case KEY_RO: keymap[i]=SDL_SCANCODE_EURO; break;
		case KEY_KATAKANA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_HIRAGANA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_HENKAN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_KATAKANAHIRAGANA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MUHENKAN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_KPJPCOMMA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_KPENTER: keymap[i]=SDL_SCANCODE_KP_ENTER; break;
		case KEY_RIGHTCTRL: keymap[i]=SDL_SCANCODE_RCTRL; break;
		case KEY_KPSLASH: keymap[i]=SDL_SCANCODE_KP_DIVIDE; break;
		case KEY_SYSRQ: keymap[i]=SDL_SCANCODE_SYSREQ; break;
		case KEY_RIGHTALT: keymap[i]=SDL_SCANCODE_RALT; break;
		case KEY_LINEFEED: keymap[i]=SDL_SCANCODE_RETURN; break;
		case KEY_HOME: keymap[i]=SDL_SCANCODE_HOME; break;
		case KEY_UP: keymap[i]=SDL_SCANCODE_UP; break;
		case KEY_PAGEUP: keymap[i]=SDL_SCANCODE_PAGEUP; break;
		case KEY_LEFT: keymap[i]=SDL_SCANCODE_LEFT; break;
		case KEY_RIGHT: keymap[i]=SDL_SCANCODE_RIGHT; break;
		case KEY_END: keymap[i]=SDL_SCANCODE_END; break;
		case KEY_DOWN: keymap[i]=SDL_SCANCODE_DOWN; break;
		case KEY_PAGEDOWN: keymap[i]=SDL_SCANCODE_PAGEDOWN; break;
		case KEY_INSERT: keymap[i]=SDL_SCANCODE_INSERT; break;
		case KEY_DELETE: keymap[i]=SDL_SCANCODE_DELETE; break;
		case KEY_MACRO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MUTE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_VOLUMEDOWN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_VOLUMEUP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_POWER: keymap[i]=SDL_SCANCODE_POWER; break;
		case KEY_KPEQUAL: keymap[i]=SDL_SCANCODE_KP_EQUALS; break;
		case KEY_KPPLUSMINUS: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PAUSE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_KPCOMMA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_HANGUEL: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_HANJA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_YEN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_LEFTMETA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_RIGHTMETA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_COMPOSE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_STOP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_AGAIN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PROPS: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_UNDO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_FRONT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_COPY: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_OPEN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PASTE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_FIND: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CUT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_HELP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MENU: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CALC: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SETUP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SLEEP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_WAKEUP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_FILE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SENDFILE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_DELETEFILE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_XFER: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PROG1: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PROG2: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_WWW: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MSDOS: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_COFFEE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_DIRECTION: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CYCLEWINDOWS: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MAIL: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_BOOKMARKS: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_COMPUTER: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_BACK: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_FORWARD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CLOSECD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_EJECTCD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_EJECTCLOSECD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_NEXTSONG: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PLAYPAUSE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PREVIOUSSONG: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_STOPCD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_RECORD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_REWIND: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PHONE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_ISO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CONFIG: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_HOMEPAGE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_REFRESH: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_EXIT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MOVE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_EDIT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SCROLLUP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SCROLLDOWN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_KPLEFTPAREN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_KPRIGHTPAREN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F13: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F14: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F15: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F16: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F17: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F18: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F19: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F20: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F21: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F22: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F23: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_F24: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PLAYCD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PAUSECD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PROG3: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PROG4: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SUSPEND: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CLOSE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PLAY: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_FASTFORWARD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_BASSBOOST: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PRINT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_HP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CAMERA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SOUND: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_QUESTION: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_EMAIL: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CHAT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SEARCH: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CONNECT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_FINANCE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SPORT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SHOP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_ALTERASE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CANCEL: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_BRIGHTNESSDOWN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_BRIGHTNESSUP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MEDIA: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_OK: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SELECT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_GOTO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CLEAR: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_POWER2: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_OPTION: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_INFO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TIME: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_VENDOR: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_ARCHIVE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PROGRAM: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CHANNEL: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_FAVORITES: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_EPG: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PVR: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MHP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_LANGUAGE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TITLE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SUBTITLE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_ANGLE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_ZOOM: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MODE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_KEYBOARD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SCREEN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PC: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TV: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TV2: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_VCR: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_VCR2: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SAT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SAT2: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TAPE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_RADIO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TUNER: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PLAYER: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TEXT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_DVD: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_AUX: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MP3: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_AUDIO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_VIDEO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_DIRECTORY: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_LIST: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MEMO: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CALENDAR: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_RED: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_GREEN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_YELLOW: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_BLUE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CHANNELUP: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_CHANNELDOWN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_FIRST: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_LAST: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_AB: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_NEXT: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_RESTART: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SLOW: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_SHUFFLE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_BREAK: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_PREVIOUS: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_DIGITS: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TEEN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_TWEN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_DEL_EOL: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_DEL_EOS: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_INS_LINE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_DEL_LINE: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_UNKNOWN: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		case KEY_MAX: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		default: keymap[i]=SDL_SCANCODE_UNKNOWN; break;
		}
	}
}
