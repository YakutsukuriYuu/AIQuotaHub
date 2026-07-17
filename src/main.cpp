#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AIQuotaHub"));
    QApplication::setOrganizationName(QStringLiteral("AIQuotaHub"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setQuitOnLastWindowClosed(false);   // 关窗驻留托盘

    MainWindow window;
    window.show();

    return QApplication::exec();
}
