#include <QtQml>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlExtensionPlugin>

#include "servicecontrol.h"

class LinphonePlugin : public QQmlExtensionPlugin {
	Q_OBJECT
		Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

	public:
		void registerTypes(const char *uri);
};


void LinphonePlugin::registerTypes(const char *uri) {
	qmlRegisterType<ServiceControl>(uri, 1, 0, "ServiceControl");
}

#include "plugin.moc"
