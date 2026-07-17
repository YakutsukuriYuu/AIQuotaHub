#include "MainWindow.h"

#include "../core/Provider.h"
#include "../core/ProviderManager.h"
#include "../core/RefreshScheduler.h"
#include "ProviderCard.h"
#include "SettingsDialog.h"
#include "Theme.h"
#include "TrayIcon.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QScrollArea>
#include <QSettings>
#include <QStyleHints>
#include <QToolBar>
#include <utility>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_manager(new ProviderManager(this))
    , m_scheduler(new RefreshScheduler(this))
{
    setWindowTitle(QStringLiteral("AI Quota Hub"));
    setUnifiedTitleAndToolBarOnMac(true);

    buildUi();

    connect(m_manager, &ProviderManager::providersChanged, this, [this] {
        rebuildDashboard();
        m_scheduler->refreshAll();
    });
    rebuildDashboard();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray = new TrayIcon(this);
        connect(m_tray, &TrayIcon::showWindowRequested, this, [this] {
            showNormal();
            raise();
            activateWindow();
        });
        connect(m_tray, &TrayIcon::refreshRequested,
                m_scheduler, &RefreshScheduler::refreshAll);
        m_tray->show();
    } else {
        QApplication::setQuitOnLastWindowClosed(true);
    }

    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("ui/geometry")).toByteArray());

    // 系统外观切换（明/暗）时实时换肤
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged,
            this, &MainWindow::applyThemeToUi);

    m_scheduler->refreshAll();
}

void MainWindow::buildUi()
{
    auto *toolbar = addToolBar(QStringLiteral("main"));
    toolbar->setMovable(false);
    QAction *refreshAction = toolbar->addAction(QStringLiteral("刷新"));
    connect(refreshAction, &QAction::triggered,
            m_scheduler, &RefreshScheduler::refreshAll);
    QAction *settingsAction = toolbar->addAction(QStringLiteral("设置"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    auto *central = new QWidget;
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(8, 8, 8, 8);

    m_sidebar = new QListWidget;
    m_sidebar->setFixedWidth(150);
    connect(m_sidebar, &QListWidget::currentRowChanged,
            this, [this](int) { relayoutCards(); });
    rootLayout->addWidget(m_sidebar);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    auto *container = new QWidget;
    m_grid = new QGridLayout(container);
    m_grid->setSpacing(12);
    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea, 1);

    setCentralWidget(central);
    resize(1024, 640);
}

void MainWindow::rebuildDashboard()
{
    // 旧 Provider 实例由 ProviderManager 销毁；
    // RefreshScheduler 通过 destroyed 信号自动摘除，无需手动清理
    while (QLayoutItem *item = m_grid->takeAt(0))
        delete item;
    qDeleteAll(m_cards);
    m_cards.clear();
    m_latest.clear();

    const auto &providers = m_manager->providers();
    for (Provider *provider : providers) {
        auto *card = new ProviderCard(provider->displayName());
        m_cards.insert(provider->id(), card);
        m_scheduler->addProvider(provider);
        connect(provider, &Provider::finished, this, &MainWindow::onSnapshot);
    }

    m_sidebar->blockSignals(true);
    m_sidebar->clear();
    auto *allItem = new QListWidgetItem(QStringLiteral("全部"));
    allItem->setData(Qt::UserRole, QString());
    m_sidebar->addItem(allItem);
    for (Provider *provider : providers) {
        auto *item = new QListWidgetItem(provider->displayName());
        item->setData(Qt::UserRole, provider->id());
        m_sidebar->addItem(item);
    }
    m_sidebar->setCurrentRow(0);
    m_sidebar->blockSignals(false);

    if (providers.isEmpty())
        m_grid->addWidget(new QLabel(QStringLiteral(
            "没有启用的提供商。点右上角「设置」添加或启用。")), 0, 0);
    else
        relayoutCards();

    updateTraySummary();
}

void MainWindow::relayoutCards()
{
    while (QLayoutItem *item = m_grid->takeAt(0))
        delete item;   // 只移除布局项，卡片本身由 m_cards 持有

    // 移出布局的控件不会自动隐藏，先全部藏起来
    for (ProviderCard *card : std::as_const(m_cards))
        card->setVisible(false);

    QString filter;
    if (QListWidgetItem *item = m_sidebar->currentItem())
        filter = item->data(Qt::UserRole).toString();

    int index = 0;
    for (Provider *provider : m_manager->providers()) {
        if (!filter.isEmpty() && provider->id() != filter)
            continue;
        ProviderCard *card = m_cards.value(provider->id());
        if (!card)
            continue;
        m_grid->addWidget(card, index / 2, index % 2);
        card->setVisible(true);
        ++index;
    }
}

void MainWindow::onSnapshot(const ProviderSnapshot &snapshot)
{
    m_latest.insert(snapshot.providerId, snapshot);
    if (ProviderCard *card = m_cards.value(snapshot.providerId))
        card->updateSnapshot(snapshot);
    updateTraySummary();
}

void MainWindow::updateTraySummary()
{
    if (!m_tray)
        return;

    double worst = -1.0;
    QStringList lines;
    for (Provider *provider : m_manager->providers()) {
        const ProviderSnapshot snapshot = m_latest.value(provider->id());
        if (snapshot.providerId.isEmpty())
            continue;

        const QString name = m_manager->configFor(provider->id()).name;
        if (!snapshot.ok()) {
            lines.append(name + QStringLiteral("：抓取失败"));
            continue;
        }
        const double percent = snapshot.worstQuotaPercent();
        if (percent >= 0) {
            worst = qMax(worst, percent);
            lines.append(QStringLiteral("%1 %2%").arg(name).arg(qRound(percent * 100)));
        } else if (snapshot.api && snapshot.api->isValid) {
            lines.append(QStringLiteral("%1 %2 %3")
                             .arg(name, QString::number(snapshot.api->balance, 'f', 1),
                                  snapshot.api->currency));
        }
    }
    m_tray->setStatus(worst, lines);
}

void MainWindow::applyThemeToUi()
{
    qApp->setStyleSheet(Theme::appStyleSheet());
    for (ProviderCard *card : std::as_const(m_cards))
        card->applyTheme();
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_manager, this);
    connect(&dialog, &SettingsDialog::credentialsChanged,
            m_scheduler, &RefreshScheduler::refreshAll);
    dialog.exec();   // 增删改/启停由 ProviderManager 即时生效，无需收尾处理
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("ui/geometry"), saveGeometry());

    if (m_tray && m_tray->isVisible()) {
        hide();           // 关窗不退出，驻留托盘
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}
