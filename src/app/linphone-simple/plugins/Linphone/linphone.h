#ifndef LINPHONE_H
#define LINPHONE_H

#include <QObject>
#include <QSocketNotifier>

class Linphone: public QObject {
	Q_OBJECT

	public:
		Linphone();
		~Linphone() = default;

		Q_INVOKABLE void call(QString address);
		Q_INVOKABLE void terminate();
		Q_INVOKABLE void answer();
		Q_INVOKABLE void mute();
		Q_INVOKABLE void unmute();
		Q_INVOKABLE void registerSIP(QString user, QString domain, QString password);
		Q_INVOKABLE void status(QString whatToCheck);
		Q_INVOKABLE void command(QStringList userCommand);
		Q_INVOKABLE void setConfig(QString key, QString value);
		Q_INVOKABLE void enableSpeaker();
		Q_INVOKABLE void disableSpeaker();
		Q_INVOKABLE void displayOn();

	Q_SIGNALS:
		void readStatus();

	public Q_SLOTS:
		QString readStatusOutput();

	private:

		QString m_configFile = "/home/phablet/.linphonerc";

		int             _write_fd;
		int             _read_fd;
		QSocketNotifier _read_notifier;
};

#endif
