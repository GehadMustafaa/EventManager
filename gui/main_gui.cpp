#include <QApplication>
#include <QStandardPaths>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("SmartEventManagement");
    QApplication::setApplicationName("SmartEventManagement");

    QString dataPath = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/events.db";

    MainWindow window(dataPath);
    window.show();
    return QApplication::exec();
}
