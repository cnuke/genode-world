#include <QDebug>
#include <QSettings>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include "linphone.h"


static bool dispatch(int fd, QString const &msg)
{
	QByteArray const msg_array = msg.toUtf8();

	ssize_t const n = ::write(fd,
	                          msg_array.constData(),
	                          msg_array.size());
	return n < 0 ? false : true;
}


Linphone::Linphone()
:
	_write_fd      { open("/dev/terminal", O_WRONLY | O_NONBLOCK) },
	_read_fd       { open("/dev/terminal", O_RDONLY | O_NONBLOCK) },
	_read_notifier { _read_fd, QSocketNotifier::Read, this }
{
	connect(&_read_notifier, SIGNAL(activated(int)),
	        this,            SIGNAL(readStatus()));
}


void Linphone::call(QString address)
{
	QStringList args;
	args << "call" << address << "\n";

	if (!dispatch(_write_fd, args.join(" ")))
		qDebug() << __func__ << "failed";
}


void Linphone::terminate()
{
	QString msg { "terminate\n" };
	if (!dispatch(_write_fd, msg))
		qDebug() << __func__ << "failed";
}


void Linphone::answer()
{
	QString msg { "answer\n" };
	if (!dispatch(_write_fd, msg))
		qDebug() << __func__ << "failed";
}


void Linphone::mute()
{
	QString msg { "mute\n" };
	if (!dispatch(_write_fd, msg))
		qDebug() << __func__ << "failed";
}


void Linphone::unmute()
{
	QString msg { "unmute\n" };
	if (!dispatch(_write_fd, msg))
		qDebug() << __func__ << "failed";
}


void Linphone::registerSIP(QString user, QString domain, QString password)
{
	QString identity = "sip:" + user + "@" + domain;

	QString transport = domain + ";transport=tls";

	QStringList args;
	args << "register" << identity << transport << password << "\n";

	if (!dispatch(_write_fd, args.join(" ")))
		qDebug() << __func__ << "failed";
}


void Linphone::status(QString whatToCheck)
{
	bool const check_register = whatToCheck == "register";

	if (check_register) {
		QString msg { "status register\n" };
		if (!dispatch(_write_fd, msg))
			qDebug() << __func__ << "failed";
	}
}


QString Linphone::readStatusOutput()
{
	static char buffer[8192];
	memset(buffer, 0, sizeof(buffer));

	// XXX we read the dispatched msg
	ssize_t const n = read(_read_fd, buffer, sizeof(buffer)-1);
	if (n < 0) {
		if (errno != EAGAIN)
			qDebug() << __func__ << "errno:" << errno;
		return QString();
	}

	buffer[n] = 0;

	return QString(buffer);
}


void Linphone::command(QStringList userCommand)
{
	bool generic = false;
	if (userCommand.size() > 0)
		generic = userCommand.at(0) == "generic";

	bool calls   = false;
	if (userCommand.size() > 1)
		calls = userCommand.at(1) == "calls";

	if (generic && calls) {

		QString msg { "calls\n" };
		if (!dispatch(_write_fd, msg))
			qDebug() << __func__ << "failed";
	}

	bool unregister = false;
	if (userCommand.size() > 0)
		unregister = userCommand.at(0) == "unregister";

	if (unregister)
		if (!dispatch(_write_fd, "unregister\nproxy remove 0\n"))
			qDebug() << __func__ << "failed";
}


void Linphone::enableSpeaker()
{
	qDebug() << __func__ << ":" << __LINE__;
}


void Linphone::disableSpeaker()
{
	qDebug() << __func__ << ":" << __LINE__;
}


void Linphone::displayOn()
{
	qDebug() << __func__ << ":" << __LINE__;
}


void Linphone::setConfig(QString key, QString value)
{
	qDebug() << __func__ << ":" << __LINE__ << "key: " << key << "value: " << value;

	QSettings settings(m_configFile, QSettings::IniFormat);
	settings.setValue(key, value);
}
