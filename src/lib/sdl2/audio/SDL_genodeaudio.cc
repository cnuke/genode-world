/*
 * \brief  Genode-specific audio backend
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
#include <base/allocator_avl.h>
#include <base/printf.h>
#include <base/thread.h>
#include <os/config.h>
#include <audio_out_session/connection.h>

using Genode::env;
using Genode::Allocator_avl;
using Genode::Signal_context;
using Genode::Signal_receiver;


enum {
	LEFT = 0, RIGHT, MAX_AUDIO_CHANNELS,
};

static const char *channel_names[] = { "front left", "front right" };

extern "C" {

#include "SDL_internal.h"
#include "SDL_audio.h"
#include "SDL_audiomem.h"
#include "SDL_audio_c.h"
#include "SDL_audiodev_c.h"
#include "SDL_genodeaudio.h"


struct SDL_PrivateAudioData {
	Uint8                 *mixbuf;
	Uint32                 mixlen;
	Audio_out::Connection *audio[MAX_AUDIO_CHANNELS];
	Audio_out::Packet     *packet[MAX_AUDIO_CHANNELS];
	bool                   initialized;
};


static void GENODEAUDIO_CloseDevice(_THIS)
{
	if (_this->hidden != nullptr) {
		SDL_FreeAudioMem(_this->hidden->mixbuf);
		_this->hidden->mixbuf = nullptr;

		for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
			Audio_out::Connection *c = _this->hidden->audio[i];
			c->stop();
			Genode::destroy(env()->heap(), c);
			c = nullptr;
		}

		SDL_free(_this->hidden);
		_this->hidden = nullptr;
	}
}


static Uint8 *GENODEAUDIO_GetDeviceBuf(_THIS) {
	return _this->hidden->mixbuf; }


static int GENODEAUDIO_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
	if (iscapture)
		return SDL_SetError("Capture devices not supported");

	/* Initialize all variables that we clean on shutdown */
	_this->hidden = (struct SDL_PrivateAudioData *)
		SDL_malloc((sizeof *_this->hidden));

	if ((_this->hidden == nullptr)) {
		return SDL_OutOfMemory();
	}

	SDL_memset(_this->hidden, 0, (sizeof *_this->hidden));

	/* connect to 'Audio_out' service */
	for (int channel = 0; channel < MAX_AUDIO_CHANNELS; channel++) {
		try {
			_this->hidden->audio[channel] = new (env()->heap())
				Audio_out::Connection(channel_names[channel],
				                      false, channel == 0 ? true : false);
			_this->hidden->audio[channel]->start();
		} catch(Genode::Parent::Service_denied) {
			PERR("Could not connect to 'Audio_out' service.");

			while(--channel > 0)
				destroy(env()->heap(), _this->hidden->audio[channel]);

			return SDL_SetError("No Audio_out session available.");;
		}
	}

	PDBG("requested freq    = %d",   _this->spec.freq);
	PDBG("requested format  = 0x%x", _this->spec.format);
	PDBG("requested samples = %u",   _this->spec.samples);
	PDBG("requested size    = %u",   _this->spec.size);

	/* we currently only support these settings */
	_this->spec.channels = MAX_AUDIO_CHANNELS;
	_this->spec.format   = AUDIO_S16LSB;
	_this->spec.freq     = Audio_out::SAMPLE_RATE;
	_this->spec.samples  = Audio_out::PERIOD;

	/* Calculate the final parameters for this audio specification */
	SDL_CalculateAudioSpec(&_this->spec);

	/* Allocate mixing buffer */
	_this->hidden->mixlen = _this->spec.size;
	_this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(_this->hidden->mixlen);
	if (_this->hidden->mixbuf == nullptr) {
		GENODEAUDIO_CloseDevice(_this);
		return SDL_OutOfMemory();
	}
	SDL_memset(_this->hidden->mixbuf, _this->spec.silence, _this->spec.size);

	/* We're ready to rock and roll. :-) */
	return 0;
}


static void GENODEAUDIO_PlayDevice(_THIS)
{
	Audio_out::Connection *c[MAX_AUDIO_CHANNELS];
	Audio_out::Packet     *p[MAX_AUDIO_CHANNELS];
	for (int channel = 0; channel < MAX_AUDIO_CHANNELS; channel++) {
		c[channel] = _this->hidden->audio[channel];
		p[channel] = _this->hidden->packet[channel];
	}

	/* get the currently played packet initially */
	if (!_this->hidden->initialized) {
		p[0] = c[0]->stream()->next();
		_this->hidden->initialized = true;
	}

	/* get new packet for left channel and use it to synchronize the right channel */
	p[0]          = c[0]->stream()->next(p[0]);
	unsigned ppos = c[0]->stream()->packet_position(p[0]);
	p[1]          = c[1]->stream()->get(ppos);

	Uint8 * const mixbuf = _this->hidden->mixbuf;
	for (int s = 0; s < Audio_out::PERIOD; s++)
		for (int ch = 0; ch < MAX_AUDIO_CHANNELS; ch++)
			p[ch]->content()[s] = (float)(((int16_t*)mixbuf)[s * MAX_AUDIO_CHANNELS + ch]) / 32768;

	for (int ch = 0; ch < MAX_AUDIO_CHANNELS; ch++) {
		_this->hidden->audio[ch]->submit(p[ch]);

		/* save current packet for querying packet position later */
		_this->hidden->packet[ch] = p[ch];
	}
}


static void GENODEAUDIO_WaitDevice(_THIS)
{
	Audio_out::Connection *c = _this->hidden->audio[0];
	Audio_out::Packet     *p = _this->hidden->packet[0];

	unsigned const packet_pos = c->stream()->packet_position(p);
	unsigned const play_pos   = c->stream()->pos();
	unsigned queued           = packet_pos < play_pos
	                            ? ((Audio_out::QUEUE_SIZE + packet_pos) - play_pos)
	                            : packet_pos - play_pos;

	/* wait until there is only one packet left to play */
	while (queued > 1) {
		c->wait_for_progress();
		queued--;
	}
}


static int GENODEAUDIO_Init(SDL_AudioDriverImpl *impl)
{
	impl->CloseDevice  = GENODEAUDIO_CloseDevice;
	impl->GetDeviceBuf = GENODEAUDIO_GetDeviceBuf;
	impl->OpenDevice   = GENODEAUDIO_OpenDevice;
	impl->PlayDevice   = GENODEAUDIO_PlayDevice;
	impl->WaitDevice   = GENODEAUDIO_WaitDevice;

	impl->AllowsArbitraryDeviceNames = 1;

	return 1;
}


AudioBootStrap GENODEAUDIO_bootstrap = {
	"genode", "Genode audio driver", GENODEAUDIO_Init, 0
};

} /* extern "C" */
