/*
 * \brief   Audio Player Agent (platform)
 * \author  Josef Soentgen
 * \date    2016-02-14
 */

/*
 * Copyright (C) 2016 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

/* Genode includes */
#include <util/xml_node.h>

/* Qt includes */
#include <QObject>
#include <QString>


namespace Platform {
	struct Object;
	struct Model;

	namespace Player {
		enum State { STATE_PLAYING, STATE_PAUSED, STATE_STOPPED };
		class Model;
	}

	namespace Playlist {
		class Model;
	}

	Object &object();
}

struct Platform::Model : QObject { Q_OBJECT Q_SIGNALS: void changed(); };


struct Platform::Player::Model : Platform::Model
{
		virtual unsigned      current_id()       const = 0;
		virtual QString       current_artist()   const = 0;
		virtual QString       current_title()    const = 0;
		virtual unsigned      current_progress() const = 0;
		virtual QString       current_time()     const = 0;
		virtual Player::State current_state()    const = 0;
};

struct QStandardItemModel;

struct Platform::Playlist::Model : Platform::Model
{
	virtual QStandardItemModel const *current() const = 0;
	virtual Genode::Xml_node   const current_xml() const = 0;

	virtual unsigned first_id() const = 0;
	virtual unsigned last_id()  const = 0;
};


struct Platform::Object
{
	virtual Player::Model   const & player_model()   = 0;
	virtual Playlist::Model const & playlist_model() = 0;
};

#endif /* _PLATFORM_H_ */
