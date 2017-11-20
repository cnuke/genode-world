/*
 * \brief   Audio Player Agent
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
#include <base/attached_rom_dataspace.h>
#include <base/log.h>
#include <base/thread.h>
#include <os/reporter.h>
#include <rom_session/connection.h>
#include <util/retry.h>

/* Qt includes */
#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QKeyEvent>
#include <QItemDelegate>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QStandardItemModel>
#include <QTableView>
#include <QWidget>

/* Qoost includes */
#include <qoost/compound_widget.h>

/* local includes */
#include <platform.h>
#include <control_panel.h>
#include <util.h>

/* keep dummies quiet */
extern "C" long pathconf(const char*, int) { return -1; }
extern "C" long getpid() { return 0; }
extern "C" long chmod(const char*, int) { return 0; }


static char const * const config_file   = "/audio_player.config";
static char const * const playlist_file = "/playlist";

struct Artist_label : QLabel       { Q_OBJECT };
struct Title_label  : QLabel       { Q_OBJECT };
struct Time_label   : QLabel       { Q_OBJECT };
struct Progress_bar : QProgressBar { Q_OBJECT };


namespace View {
	struct Info;
	struct Playlist;
}


struct View::Info : Compound_widget<QFrame, QVBoxLayout>
{
	Q_OBJECT

		QMember<QHBoxLayout>  _title_time;

		unsigned              _id;
		QMember<Artist_label> _artist;
		QMember<Title_label>  _title;
		QMember<Time_label>   _time;
		QMember<Progress_bar> _progress;

	public Q_SLOTS:

		void update_from_model()
		{
			Platform::Player::Model const &model = Platform::object().player_model();

			_id = model.current_id();
			_artist->setText(model.current_artist());
			_title->setText(model.current_title());
			_time->setText(model.current_time());
			_progress->setValue(model.current_progress());
		}

	public:

		Info()
		{
			_progress->setMinimum(0);
			_progress->setMaximum(100);
			_progress->setTextVisible(false);

			_time->setAlignment(Qt::AlignRight|Qt::AlignCenter);

			_title_time->addWidget(_title);
			_title_time->addWidget(_time);
			_title_time->setStretchFactor(_title, 4);
			_title_time->setStretchFactor(_time,  1);

			_layout->addWidget(_artist);
			_layout->addLayout(_title_time);
			_layout->addWidget(_progress);
		}

		~Info() { }
};


struct View::Playlist : Compound_widget<QTableView, QVBoxLayout, 10>
{
	Q_OBJECT

	public Q_SLOTS:

		void update_from_model()
		{
			QStandardItemModel const *model = Platform::object().playlist_model().current();

			setModel(const_cast<QStandardItemModel*>(model));
			resizeColumnsToContents();
			horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
			horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
		}

	Q_SIGNALS:

		void remove_tracks(QModelIndexList);

	public:

		Playlist()
		{
			setShowGrid(false);
			verticalHeader()->setVisible(false);
			horizontalHeader()->setVisible(false);
			setEditTriggers(QAbstractItemView::NoEditTriggers);
			setFocusPolicy(Qt::StrongFocus);
			setSelectionBehavior(QAbstractItemView::SelectRows);
			setSelectionMode(QAbstractItemView::ExtendedSelection);
			setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
			setAlternatingRowColors(true);
		}

		~Playlist() { }

		void keyPressEvent(QKeyEvent *event)
		{
			QTableView::keyPressEvent(event);

			switch (event->key()) {
			case Qt::Key_Delete:
			case Qt::Key_Backspace:
				Q_EMIT remove_tracks(selectionModel()->selectedRows());
				break;
			default: break;
			}
		}
};


class Agent : public Compound_widget<QFrame, QVBoxLayout>
{
	Q_OBJECT

	private:

		QMember<View::Info>     _info_view;
		QMember<View::Playlist> _playlist_view;
		QMember<Control::Panel> _control_panel;

	private Q_SLOTS:

		void _remove_rows(QModelIndexList rows);
		void _write_config(int button, unsigned sel_id);
		void _view_clicked(const QModelIndex &);
		void _open_clicked();
		void _prev_clicked();
		void _play_clicked();
		void _next_clicked();

	public:

		Agent();
		~Agent();
};


enum { BUTTON_PREV = 0x1, BUTTON_PLAY = 0x2, BUTTON_NEXT = 0x4, BUTTON_SEL = 0x8 };


void Agent::_remove_rows(QModelIndexList rows)
{
	Genode::Xml_node const old      = Platform::object().playlist_model().current_xml();
	size_t                 old_size = old.size();

	char *xml_data = new char[old_size];
	Util::Buffer_guard g(xml_data, true);

	try {
		Genode::Xml_generator xml(xml_data, old_size, "playlist",
		[&] {
			try {
				Genode::String<64> tmp;
				old.attribute("mode").value(&tmp);
				xml.attribute("mode", tmp);
			} catch (...) { xml.attribute("mode", "once"); }

			/* copy other nodes */
			old.for_each_sub_node("track", [&] (Genode::Xml_node &node) {
				unsigned long const track_id = node.attribute_value<unsigned long>("id", 0);

				for (int i = 0; i < rows.size(); ++i) {
					int const id = rows.at(i).row() + 1;
					if ((int)track_id == id)
						return;
				}

				xml.append(node.addr(), node.size());
				xml.append("\n");
			});
		});

		Util::write_file(playlist_file, xml_data, xml.used());
	} catch (...) { Genode::warning("could not generate 'playlist'"); }
}


void Agent::_write_config(int action, unsigned sel = 0)
{
	char buffer[4096];
	int64_t const result = Util::read_file(config_file, buffer, sizeof(buffer));
	if (result < 0) {
		Genode::error("could not read 'audio_player.config' file");
		return;
	}

	Genode::Xml_node old(buffer, result);

	try {
		char xml_data[4096];
		Genode::Xml_generator xml(xml_data, sizeof(xml_data), "config",
		[&] {
			xml.attribute("verbose",
			              old.attribute_value<bool>("verbose", false));
			Genode::String<64> tmp;
			old.attribute("state").value(&tmp);
			if (action & BUTTON_PLAY) {
				if (tmp == "playing") xml.attribute("state", "paused");
				else                  xml.attribute("state", "playing");
			} else { xml.attribute("state", tmp); }

			unsigned const last_id = Platform::object().playlist_model().last_id();
			unsigned const cur     = Platform::object().player_model().current_id();
			unsigned t = cur;

			if (action & BUTTON_NEXT)
				t = (cur == 0 || cur == last_id) ? 1 : cur + 1;

			if (action & BUTTON_PREV)
				t = (cur == 0 || cur == 1) ? last_id : cur - 1;

			if (action & BUTTON_SEL) {
				if (sel > last_id) {
					Genode::warning("track selection ", sel, " invalid");
				}
				t = sel;

				xml.attribute("state", "playing"); /* force */
			}

			xml.attribute("selected_track", t);

			/* copy other nodes */
			old.for_each_sub_node([&] (Genode::Xml_node &node) {
				xml.append(node.addr(), node.size());
			});
		});

		Util::write_file(config_file, xml_data, xml.used());
	} catch (...) { Genode::error("could generate 'audio_player.config'"); }
}


void Agent::_view_clicked(const QModelIndex &index) {
	_write_config(BUTTON_SEL, index.row()+1); }


void Agent::_open_clicked()
{
	QStringList files = QFileDialog::getOpenFileNames(nullptr,
		tr("Open audio files"), "/",
		tr("Audio  Files (*.flac *.mp3 *.ogg *.opus);;All Files (*)"));

	enum { STEP_SIZE = 4096 };
	char *buffer       = nullptr;
	size_t buffer_size = STEP_SIZE;

	Util::Buffer_guard g(buffer, true);

	int64_t size = -1;
	Genode::retry<int>(
	[&] {
		buffer = new char[buffer_size];
		size = Util::read_file(playlist_file, buffer, buffer_size);
	},
	[&] {
		delete[] buffer;
		buffer_size += STEP_SIZE;
	});

	if (size < 0) return;

	/* always try to add all new files */
	bool append[files.size()];
	for (bool &a : append) a = true;

	Genode::Xml_node playlist(buffer, size);
	for (int i = 0; i < files.size(); ++i) {
		char const *file = files.at(i).toUtf8().constData();

		auto check_track = [&] (Genode::Xml_node &track) {
			try {
				Genode::String<2048> path;
				track.attribute("path").value(&path);

				if (Util::equal_path(path.string(), file)) {
					append[i] = false;
					throw -1; /* "fast" exit */
				}
			} catch (Genode::Xml_node::Nonexistent_sub_node) {
				Genode::warning("invalid track in playlist");
				return;
			}
		};

		try { playlist.for_each_sub_node("track", check_track); } catch (int) { }
	}

	/* check if append */
	bool result = false;
	for (bool a : append) result |= a;
	if (!result) return;

	char xml_data[buffer_size+4096];
	try {
		Genode::Xml_generator xml(xml_data, sizeof(xml_data), "playlist",
		[&] {
			try {
				Genode::String<64> tmp;
				playlist.attribute("mode").value(&tmp);
				xml.attribute("mode", tmp);
			} catch (...) { xml.attribute("mode", "once"); }

			/* copy other nodes */
			playlist.for_each_sub_node("track", [&] (Genode::Xml_node &node) {
				xml.append(node.addr(), node.size());
				xml.append("\n");
			});

			/* append the new ones */
			for (int i = 0; i < files.size(); ++i) {
				if (!append[i]) continue;

				char const *file = files.at(i).toUtf8().constData();
				file = Util::normalize_path(file);

				xml.node("track", [&] { xml.attribute("path", file); });
			}
		});

		Util::write_file(playlist_file, xml_data, xml.used());
	} catch (...) { Genode::warning("could not generate 'playlist'"); }
}


void Agent::_prev_clicked() { _write_config(BUTTON_PREV, 0); }
void Agent::_play_clicked() { _write_config(BUTTON_PLAY, 0); }
void Agent::_next_clicked() { _write_config(BUTTON_NEXT, 0); }


Agent::Agent()
{
	Platform::object();

	setWindowTitle("Audio Player Agent");
	setObjectName("Frame");

	_layout->addWidget(_info_view);
	_layout->addWidget(_control_panel);
	_layout->addWidget(_playlist_view);

	connect(&Platform::object().player_model(), SIGNAL(changed()),
	        _info_view,                         SLOT(update_from_model()));
	connect(&Platform::object().player_model(), SIGNAL(changed()),
	        _control_panel,                     SLOT(update_from_model()));
	connect(&Platform::object().playlist_model(), SIGNAL(changed()),
	        _playlist_view,                       SLOT(update_from_model()));
	connect(&Platform::object().player_model(),   SIGNAL(changed()),
	        &Platform::object().playlist_model(), SLOT(update_current_track()));

	connect(_control_panel, SIGNAL(open_clicked()), SLOT(_open_clicked()));
	connect(_control_panel, SIGNAL(prev_clicked()), SLOT(_prev_clicked()));
	connect(_control_panel, SIGNAL(play_clicked()), SLOT(_play_clicked()));
	connect(_control_panel, SIGNAL(next_clicked()), SLOT(_next_clicked()));

	connect(_playlist_view, SIGNAL(doubleClicked(const QModelIndex&)),
	                        SLOT(_view_clicked(const QModelIndex&)));
	connect(_playlist_view, SIGNAL(remove_tracks(QModelIndexList)),
	                        SLOT(_remove_rows(QModelIndexList)));

	show();
}


Agent::~Agent() { }


/******************
 ** Main program **
 ******************/

static inline void load_stylesheet()
{
	QFile file("/qt/lib/theme/style.qss");
	if (!file.open(QFile::ReadOnly)) {
		qWarning() << "Warning:"     << file.errorString()
		           << "opening file" << file.fileName();
		return;
	}

	qApp->setStyleSheet(QLatin1String(file.readAll()));
}


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	load_stylesheet();

	QMember<Agent> agent;

	app.connect(&app, SIGNAL(lastWindowClosed()), SLOT(quit()));

	return app.exec();
}

#include "agent.moc"
