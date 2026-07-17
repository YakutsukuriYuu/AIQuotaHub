#include <QApplication>
#include <QLabel>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("AIQuotaHub");
    QApplication::setOrganizationName("AIQuotaHub");

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("AI Quota Hub"));

    auto *placeholder = new QLabel(QStringLiteral("AI Quota Hub — 项目骨架就绪"));
    placeholder->setAlignment(Qt::AlignCenter);
    window.setCentralWidget(placeholder);
    window.resize(960, 640);
    window.show();

    return QApplication::exec();
}
