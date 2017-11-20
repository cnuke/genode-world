/*
 * \brief   Audio Player Agent (platform implementation)
 * \author  Josef Soentgen
 * \date    2016-02-14
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/printf.h>
#include <base/thread.h>
#include <os/attached_rom_dataspace.h>
#include <os/reporter.h>
#include <rom_session/connection.h>

/* Qt includes */
#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QStandardItemModel>
#include <QTableView>
#include <QWidget>

/* Qoost includes */
#include <qoost/icon.h> /* pulls the rest */

/* local includes */
#include <platform.h>
#include <control_panel.h>
#include <util.h>


namespace Platform {
	class Genode_object;
	class Genode_signal_proxy;

	class Info_model;
	class Playlist_model;
}

struct Platform::Genode_signal_proxy : QObject,
                                       Genode::Signal_dispatcher<Platform::Genode_signal_proxy>
{
	Q_OBJECT public:

		Genode_signal_proxy(Genode::Signal_receiver &sig_rec)
		:
			Genode::Signal_dispatcher<Platform::Genode_signal_proxy>(
				sig_rec, *this, &Genode_signal_proxy::handle_genode_signal)
	{
		connect(this, SIGNAL(internal_signal()),
		        this, SIGNAL(signal()), Qt::QueuedConnection);
	}

	/* called by signal dispatcher / emits internal signal in context of
	 *          * signal-dispatcher thread */
	void handle_genode_signal(unsigned = 0) { Q_EMIT internal_signal(); }

Q_SIGNALS:

	/* internal_signal() is Qt::QueuedConnection to signal() */
	void internal_signal();

	/* finally signal() is emitted in the context of the Qt main thread */
	void signal();
};


/*******************
 ** Current track **
 *******************/

class Platform::Info_model : public Platform::Player::Model
{
	Q_OBJECT

	private:

		Genode::Attached_rom_dataspace _current_track_rom { "current_track" };
		QMember<Genode_signal_proxy>   _current_track_proxy;

		unsigned _current_id;
		QString  _current_artist;
		QString  _current_title;
		unsigned _current_progress;
		QString  _current_time;

		Player::State _current_state;

		Player::State _import_state(QString const &state)
		{
			if (state == "playing") return Player::STATE_PLAYING;
			if (state == "paused")  return Player::STATE_PAUSED;
			if (state == "stopped") return Player::STATE_STOPPED;

			return Player::STATE_STOPPED;
		}

	typedef Genode::String<256> Artist;
	typedef Genode::String<256> Title;
	typedef Genode::String<16>  State;

	private Q_SLOTS:

		void _update_model_current_track()
		{
			_current_track_rom.update();

			if (!_current_track_rom.is_valid()) { return; }

			try {
				using Genode::Xml_node;

				Xml_node node { Xml_node(_current_track_rom.local_addr<char>(),
				                         _current_track_rom.size()) };

				_current_id = node.attribute_value<unsigned long>("id", 0);

				/* current track report is empty or id is invalid */
				if (_current_id == 0) throw 1;

				Artist artist { Util::string_attribute(node, "artist", Artist("-")) };
				Title  title  { Util::string_attribute(node, "title",  Title("-")) };
				State  state  { Util::string_attribute(node, "state",  State("")) };

				unsigned long p = node.attribute_value<unsigned long>("progress", 0);
				unsigned long d = node.attribute_value<unsigned long>("duration", 1);

				_current_artist   = QString(artist.string());
				_current_title    = QString(title.string());
				_current_progress = (float)p / d * 100;
				_current_time     = Util::time_to_string(p);
				_current_state    = _import_state(QString(state.string()));
			} catch (...) {
				_current_state    = Player::STATE_STOPPED;
				_current_id       = 0;
				_current_artist   = "-";
				_current_title    = "-";
				_current_progress = 0;
				_current_time     = "00:00:00";
			}

			Q_EMIT changed();
		}

	public:

		Info_model(Genode::Signal_receiver &sig_rec)
		: _current_track_proxy(sig_rec)
		{
			connect(_current_track_proxy, SIGNAL(signal()),
			        SLOT(_update_model_current_track()));
			_current_track_rom.sigh(*_current_track_proxy);
		}

		/***********
		 ** Model **
		 ***********/

		unsigned       current_id()       const { return _current_id;       }
		QString        current_artist()   const { return _current_artist;   }
		QString        current_title()    const { return _current_title;    }
		unsigned       current_progress() const { return _current_progress; }
		QString        current_time()     const { return _current_time;     }
		Player::State  current_state()    const { return _current_state;    }
};


/**************
 ** Playlist **
 **************/

struct Platform::Playlist_model : Platform::Playlist::Model
{
	Q_OBJECT

	private:

		Genode::Attached_rom_dataspace _playlist_rom { "playlist" };
		QMember<Genode_signal_proxy>   _playlist_proxy;

        QStandardItemModel _model;

        unsigned _last_current_id { 0 };

	private Q_SLOTS:

		void _update_model_playlist()
		{
			_playlist_rom.update();
			if (!_playlist_rom.is_valid())
				return;

			try {
				using Genode::Xml_node;
				using Genode::String;

				Xml_node node { Xml_node(_playlist_rom.local_addr<char>(),
				                         _playlist_rom.size()) };

				_model.clear();
				_model.setHorizontalHeaderItem(0, new QStandardItem("ID"));
				_model.setHorizontalHeaderItem(1, new QStandardItem("Artist"));
				_model.setHorizontalHeaderItem(2, new QStandardItem("Album"));
				_model.setHorizontalHeaderItem(3, new QStandardItem("Track"));
				_model.setHorizontalHeaderItem(4, new QStandardItem("Title"));
				_model.setHorizontalHeaderItem(5, new QStandardItem("Duration"));

				node.for_each_sub_node("track", [&] (Genode::Xml_node &track) {

					QList<QStandardItem*> _row_list;

					try {

						QStandardItem *item = nullptr;
						Genode::String<256> tmp;

						Qt::Alignment left  = Qt::AlignLeft|Qt::AlignVCenter;
						Qt::Alignment right = Qt::AlignRight|Qt::AlignVCenter;
						track.attribute("id").value(&tmp);
						item = new QStandardItem(tmp.string());
						item->setTextAlignment(right);
						_row_list.append(item);

						track.attribute("artist").value(&tmp);
						item = new QStandardItem(tmp.string());
						item->setTextAlignment(left);
						_row_list.append(item);

						track.attribute("album").value(&tmp);
						item = new QStandardItem(tmp.string());
						item->setTextAlignment(left);
						_row_list.append(item);

						track.attribute("track").value(&tmp);
						item = new QStandardItem(tmp.string());
						item->setTextAlignment(right);
						_row_list.append(item);

						track.attribute("title").value(&tmp);
						item = new QStandardItem(tmp.string());
						item->setTextAlignment(left);
						_row_list.append(item);

						unsigned long d = track.attribute_value<unsigned long>("duration", 0);

						item = new QStandardItem(Util::time_to_string(d));
						item->setTextAlignment(right);
						_row_list.append(item);

					} catch (...) { }

					_model.appendRow(_row_list);
					_row_list.clear();
				});

				/* forcefully update playing track even when unchanged */
				update_current_track(true);

				Q_EMIT changed();
			} catch (...) { PWRN("Could update playlist model"); }
		}

	public Q_SLOTS:

		void update_current_track(bool force = false)
		{
			unsigned const cur_id = Platform::object().player_model().current_id();

			if (!force && _last_current_id == cur_id) return;

			if (_last_current_id != 0) {
				QStandardItem *last = _model.item(_last_current_id - 1, 0);
				if (last) last->setIcon(QIcon());
			}

			if (cur_id == 0) return;

			QStandardItem *cur = _model.item(cur_id - 1, 0);
			if (cur) cur->setIcon(QIcon("/qt/lib/theme/playing.png"));

			_last_current_id = cur_id;
		}

	public:

		Playlist_model(Genode::Signal_receiver &sig_rec)
		: _playlist_proxy(sig_rec)
		{
			connect(_playlist_proxy, SIGNAL(signal()),
			        this, SLOT(_update_model_playlist()));

			_playlist_rom.sigh(*_playlist_proxy);
		}

		~Playlist_model() { }

		/***********
		 ** Model **
		 ***********/

		unsigned first_id() const { return _model.rowCount() ? 1 : 0; }
		unsigned  last_id() const { return _model.rowCount(); }

		QStandardItemModel const *current() const { return &_model; }
		Genode::Xml_node const current_xml() const
		{
			/* evïl */
			Playlist_model *_this = const_cast<Playlist_model*>(this);
			return Genode::Xml_node(_this->_playlist_rom.local_addr<char>(),
			                        _playlist_rom.size());
		}
};


enum { THREAD_STACK_SIZE = 2 * 1024 * sizeof(long) };


class Platform::Genode_object : public Platform::Object,
                                public Genode::Thread_deprecated<THREAD_STACK_SIZE>
{
	private:

		Genode::Signal_receiver _sig_rec;

		void entry() override
		{
			using namespace Genode;
			while (true) {
				Signal sig = _sig_rec.wait_for_signal();
				Signal_dispatcher_base *dispatcher {
					dynamic_cast<Signal_dispatcher_base *>(sig.context()) };
				if (dispatcher) dispatcher->dispatch(sig.num());
			}
		}

		QMember<Platform::Info_model>     _player   { _sig_rec };
		QMember<Platform::Playlist_model> _playlist { _sig_rec };

	public:

		Genode_object() : Genode::Thread_deprecated<THREAD_STACK_SIZE>("signal_dispatcher")
		{
			start();
		}

		Player::Model   const &player_model()   { return *_player;   }
		Playlist::Model const &playlist_model() { return *_playlist; }
};


Platform::Object &Platform::object()
{
	static Platform::Genode_object inst;
	return inst;
}

#include "platform.moc"
