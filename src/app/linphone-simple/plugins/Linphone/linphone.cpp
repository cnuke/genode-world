#include <QDebug>
#include <QSettings>

#include "linphone.h"

static bool out_call = false;
static bool in_call = false;
static bool in_call_active = false;
static bool check_register = false;
static bool check_calls = false;

static unsigned status_ticks = 0;


Linphone::Linphone()
{
}


void Linphone::call(QString address)
{
	QStringList args;
	args << "call " << address;

	if (!out_call)
		out_call = true;
	else
		qDebug() << "call while already calling";

	qDebug() << args;
}


void Linphone::terminate()
{
	QStringList args;
	args << "terminate";

	if (!out_call && !in_call_active)
		qDebug() << "terminate while not calling";

	if (out_call)
		out_call = false;

	if (in_call_active)
		in_call_active = false;

	qDebug() << args;
}


void Linphone::answer()
{
	QStringList args;
	args << "answer";

	if (in_call)
		in_call = false;

	if (!in_call_active)
		in_call_active = true;

	qDebug() << args;
}


void Linphone::mute()
{
	QStringList args;
	args << "mute";

	qDebug() << args;
}


void Linphone::unmute()
{
	QStringList args;
	args << "unmute";

	qDebug() << args;
}


void Linphone::registerSIP(QString user, QString domain, QString password)
{
	QStringList args;
	QString userUser;
	QString domainHost;
	QString passwordPassword;
	//address = "sip:" + user + "@" + domain;
	//
	userUser = " --username " + user;
	domainHost = " --host " + domain;
	passwordPassword = " --password " + password;

	args << "register" << "--username" << user << "--host" << domain << "--password" << password;

	qDebug() << args;

	status("register");
}


void Linphone::status(QString whatToCheck)
{
	QStringList args;
	args << "status" << whatToCheck;

	bool const registered = whatToCheck == "register";
	if (registered && !check_register)
		check_register = true;

	qDebug() << "LINPHONECSH: status on " << whatToCheck;

	if (++status_ticks >= 20) {
		if (!in_call)
			in_call = true;
		check_calls = true;
		status_ticks = 0;
	}

	emit readStatus();
}


QString Linphone::readStatusOutput()
{
	// check first to inject incoming calls
	if (check_calls) {
		check_calls = false;
		bool const active = out_call || in_call_active;

		QString result;
		if (active || in_call) {
			if (active)  result += "StreamsRunning\n";
			if (in_call) result += "sip:fnord@10.0.0.8  IncomingReceived";
		} else
			result = "No active calls";

		qDebug() << __func__ << ":" << __LINE__ << " check_calls" << result;
		return result;
	}

	if (check_register) {
		qDebug() << __func__ << ":" << __LINE__ << " check_register";

		check_register = false;
		return QString("registered, identity=sip:pinephone@10.0.0.8 duration=600");
	}

	qDebug() << __func__ << ":" << __LINE__ << " called";
	return QString();
}


void Linphone::command(QStringList userCommand)
{
	QStringList args;
	args << userCommand;

	bool generic = false;
	bool calls   = false;
	if (userCommand.size() > 0)
		generic = userCommand.at(0) == "generic";

	if (userCommand.size() > 1)
		calls = userCommand.at(1) == "calls";

	if (generic && calls && !check_calls)
		check_calls = true;

	qDebug() << "LINPHONECSH: command " << userCommand;

	emit readStatus();
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
