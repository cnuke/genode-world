#include <QtQml>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlExtensionPlugin>

#include "linphone.h"

class LinphonePlugin : public QQmlExtensionPlugin {
	Q_OBJECT
		Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

	public:
		void registerTypes(const char *uri);
};


void LinphonePlugin::registerTypes(const char *uri) {
	qmlRegisterSingletonType<Linphone>(uri, 1, 0, "Linphone",
		[](QQmlEngine*, QJSEngine*) -> QObject* { return new Linphone; });
}


#include "plugin.moc"
