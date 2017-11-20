/*
 * \brief   Audio Player Agent (control panel)
 * \author  Josef Soentgen
 * \date    2016-02-14
 */

/*
 * Copyright (C) 2016 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CONTROL_PANEL_H_
#define _CONTROL_PANEL_H_

/* Qt includes */
#include <QFrame>
#include <QHBoxLayout>

/* Qoost includes */
#include <qoost/compound_widget.h>
#include <qoost/qmember.h>


namespace Control {
	class Panel;
	class Button;
	class Open_button;
	class Prev_button;
	class Play_button;
	class Next_button;
	class Repeat_button;
}

struct Control::Panel : Compound_widget<QFrame, QHBoxLayout>
{
	Q_OBJECT

	private:

		QMember<Open_button>   _open_button;
		QMember<Prev_button>   _prev_button;
		QMember<Play_button>   _play_button;
		QMember<Next_button>   _next_button;
		QMember<Repeat_button> _repeat_button;

	public Q_SLOTS:

		void update_from_model();

	public:

		Panel();
		~Panel();

	Q_SIGNALS:

		void open_clicked();
		void prev_clicked();
		void play_clicked();
		void next_clicked();
		void repeat_clicked();
};

#endif /* _CONTROL_PANEL_H_ */
