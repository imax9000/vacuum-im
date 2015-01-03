#include <QLibrary>
#include <QApplication>
#include <QMap>
#include <QStringList>
#include "pluginmanager.h"

// This is a simple function that parses command-line flags of 2 formats:
//   --flag value
//   --flag=value
// After migration to Qt5 replace this with QCommandLineParser.
QMap<QString, QString> parseArgs(const QStringList& args) {
	QMap<QString, QString> result;
	for (int i = 1; i < args.size(); i++) {
		if (!args.at(i).startsWith("--") || args.at(i) == "--") {
			// End of flags.
			break;
		}
		QString flag = args.at(i);
		flag.remove(0, 2);
		int end = flag.indexOf('=');
		if (flag > 0) {
			result[flag.left(end)] = flag.right(flag.length() - 1 - end);
		} else if (i + 1 < args.size()) {
			result[flag] = args.at(++i);
		}
	}
	return result;
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	app.addLibraryPath(app.applicationDirPath());

	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();

	QMap<QString, QString> flags = parseArgs(app.arguments());
	QString plugins = flags.value("plugins-dir", PLUGINS_DIR);
	QString resources = flags.value("resources-dir", RESOURCES_DIR);
	QString translations = flags.value("translations-dir", TRANSLATIONS_DIR);
	PluginManager pm(&app, plugins, resources, translations);
	pm.restart();

	return app.exec();
}
