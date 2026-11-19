#include <QApplication>
#include <QSettings>
#include <QStandardPaths>
#include "MainWindow.h"
#include "Theme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("SmartEventManagement");
    QApplication::setApplicationName("SmartEventManagement");

    // Follow the system appearance until the user picks one under View > Dark Mode.
    const QString saved = QSettings().value("appearance").toString();
    const Theme::Mode mode = saved == "dark"    ? Theme::Mode::Dark
                             : saved == "light" ? Theme::Mode::Light
                                                : Theme::systemMode();
    Theme::apply(app, mode);

    QString dataPath = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/events.db";

    MainWindow window(dataPath);
    window.show();
    return QApplication::exec();
}
