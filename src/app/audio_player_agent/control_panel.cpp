/*
 * \brief   Audio Player Agent (control panel implementation)
 * \author  Josef Soentgen
 * \date    2016-02-14
 */

/*
 * Copyright (C) 2016 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/printf.h>

/* Qt includes */
#include <QPushButton>

/* Qoost includes */
#include <qoost/icon.h>

/* local includes */
#include <platform.h>
#include <control_panel.h>


class Control::Button : public Compound_widget<QPushButton, QHBoxLayout>
{
	Q_OBJECT

	private:

		QMember<Icon> _icon;

	public:

		Button()
		{
			setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			_layout->addWidget(_icon, 0, Qt::AlignCenter);
		}

		~Button() { }
};


struct Control::Open_button   : Control::Button { Q_OBJECT };
struct Control::Prev_button   : Control::Button { Q_OBJECT };
struct Control::Play_button   : Control::Button { Q_OBJECT };
struct Control::Next_button   : Control::Button { Q_OBJECT };
struct Control::Repeat_button : Control::Button { Q_OBJECT };


void Control::Panel::update_from_model()
{
	Platform::Player::Model const &model = Platform::object().player_model();

	switch (model.current_state()) {
	case Platform::Player::STATE_PLAYING:
		_play_button->setObjectName("playing");
		break;
	case Platform::Player::STATE_STOPPED:
	case Platform::Player::STATE_PAUSED:
		_play_button->setObjectName("paused");
		break;
	default: break;
	}

	reapply_style_recursive(_play_button);
}


Control::Panel::Panel()
{
	connect(_open_button,   SIGNAL(clicked()), SIGNAL(open_clicked()));
	connect(_prev_button,   SIGNAL(clicked()), SIGNAL(prev_clicked()));
	connect(_play_button,   SIGNAL(clicked()), SIGNAL(play_clicked()));
	connect(_next_button,   SIGNAL(clicked()), SIGNAL(next_clicked()));
	connect(_repeat_button, SIGNAL(clicked()), SIGNAL(repeat_clicked()));

	_layout->addWidget(_open_button);
	_layout->addWidget(_prev_button);
	_layout->addWidget(_play_button);
	_layout->addWidget(_next_button);
	_layout->addWidget(_repeat_button);
}


Control::Panel::~Panel() { }

#include "control_panel.moc"
