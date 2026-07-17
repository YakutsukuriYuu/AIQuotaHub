#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AIQuotaHub"));
    QApplication::setOrganizationName(QStringLiteral("AIQuotaHub"));
    QApplication::setApplicationVersion(QStringLiteral("0.2.0"));
    QApplication::setQuitOnLastWindowClosed(false);   // 关窗驻留托盘

    app.setStyleSheet(Theme::appStyleSheet());   // 跟随系统明/暗

    MainWindow window;
    window.show();

    return QApplication::exec();
}
