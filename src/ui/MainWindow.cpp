#include "MainWindow.h"

#include "../core/Provider.h"
#include "../core/RefreshScheduler.h"
#include "../providers/DemoProvider.h"
#include "../providers/DeepSeekProvider.h"
#include "../providers/GlmProvider.h"
#include "ProviderCard.h"
#include "SettingsDialog.h"
#include "TrayIcon.h"
#include "UiColors.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QScrollArea>
#include <QSettings>
#include <QToolBar>
#include <utility>

namespace {

Provider *createProvider(const ProviderConfig &config, QObject *parent)
{
    if (config.type == QStringLiteral("demo"))
        return new DemoProvider(config, parent);
    if (config.id == QStringLiteral("glm"))
        return new GlmProvider(config, parent);
    if (config.id == QStringLiteral("deepseek"))
        return new DeepSeekProvider(config, parent);
    qWarning() << "未知提供商，已跳过:" << config.id << config.type;
    return nullptr;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_scheduler(new RefreshScheduler(this))
{
    setWindowTitle(QStringLiteral("AI Quota Hub"));
    setUnifiedTitleAndToolBarOnMac(true);

    QString configError;
    m_configs = ProvidersConfig::load(&configError);
    if (!configError.isEmpty())
        qWarning().noquote() << configError;

    createProviders();
    buildUi();

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
        // 无托盘时关窗即退出，避免进程驻留无法退出
        QApplication::setQuitOnLastWindowClosed(true);
    }

    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("ui/geometry")).toByteArray());

    m_scheduler->refreshAll();
}

void MainWindow::createProviders()
{
    for (const ProviderConfig &config : std::as_const(m_configs)) {
        if (!config.enabled)
            continue;
        Provider *provider = createProvider(config, this);
        if (!provider)
            continue;
        m_providers.append(provider);
        m_names.insert(config.id, config.name);
        m_scheduler->addProvider(provider);
        connect(provider, &Provider::finished, this, &MainWindow::onSnapshot);
    }
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

    // 侧栏：全部 + 每个提供商
    m_sidebar = new QListWidget;
    m_sidebar->setFixedWidth(150);
    auto *allItem = new QListWidgetItem(QStringLiteral("全部"));
    allItem->setData(Qt::UserRole, QString());   // 空 id = 不过滤
    m_sidebar->addItem(allItem);
    for (Provider *provider : m_providers)
        m_sidebar->addItem(new QListWidgetItem(provider->displayName()));
    for (int row = 1; row < m_sidebar->count(); ++row)
        m_sidebar->item(row)->setData(Qt::UserRole, m_providers.at(row - 1)->id());
    m_sidebar->setCurrentRow(0);
    connect(m_sidebar, &QListWidget::currentRowChanged,
            this, [this](int) { relayoutCards(); });
    rootLayout->addWidget(m_sidebar);

    // 卡片滚动区
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    auto *container = new QWidget;
    m_grid = new QGridLayout(container);
    m_grid->setSpacing(12);

    if (m_providers.isEmpty()) {
        m_grid->addWidget(new QLabel(QStringLiteral(
            "没有可用的提供商。请检查 providers.json 配置。")), 0, 0);
    } else {
        for (Provider *provider : m_providers) {
            auto *card = new ProviderCard(provider->displayName());
            m_cards.insert(provider->id(), card);
        }
        relayoutCards();
    }

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea, 1);
    setCentralWidget(central);
    resize(1024, 640);
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
    for (Provider *provider : m_providers) {
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
    for (Provider *provider : m_providers) {
        const ProviderSnapshot snapshot = m_latest.value(provider->id());
        if (snapshot.providerId.isEmpty())
            continue;

        const QString name = m_names.value(provider->id());
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

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_configs, this);
    dialog.exec();
    m_scheduler->refreshAll();   // Key 可能变了，立即重刷
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
