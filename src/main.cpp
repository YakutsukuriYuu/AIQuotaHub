#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AIQuotaHub"));
    QApplication::setOrganizationName(QStringLiteral("AIQuotaHub"));
    QApplication::setApplicationVersion(QStringLiteral("0.2.0"));
    QApplication::setQuitOnLastWindowClosed(false);   // 关窗驻留托盘

    // 字体栈：SF Pro（.AppleSystemUIFont），中文由 CoreText 自动回退苹方
    QFont appFont(QStringLiteral(".AppleSystemUIFont"));
    appFont.setPointSize(13);
    QApplication::setFont(appFont);

    app.setStyleSheet(Theme::appStyleSheet());   // 跟随系统明/暗

    MainWindow window;
    window.show();

    return QApplication::exec();
}
