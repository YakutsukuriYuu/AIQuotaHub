#include "MainWindow.h"

#include "../core/Provider.h"
#include "../core/ProviderManager.h"
#include "../core/RefreshScheduler.h"
#include "ProviderCard.h"
#include "SettingsDialog.h"
#include "Theme.h"
#include "TrayIcon.h"
#include "UiColors.h"

#include <QApplication>
#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QStyleHints>
#include <QVBoxLayout>
#include <utility>

namespace {

// 侧栏状态点小圆点图标
QIcon makeDotIcon(const QColor &color)
{
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(4, 4, 12, 12);
    return QIcon(pixmap);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_manager(new ProviderManager(this))
    , m_scheduler(new RefreshScheduler(this))
{
    setWindowTitle(QStringLiteral("AI Quota Hub"));

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
    auto *central = new QWidget;
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(18, 14, 18, 14);
    rootLayout->setSpacing(14);

    // ---- 头部栏：标题 + 副标题 | 刷新 / 设置 ----
    auto *header = new QHBoxLayout;
    auto *titleBox = new QVBoxLayout;
    titleBox->setSpacing(2);
    auto *title = new QLabel(QStringLiteral("数据总览"));
    title->setObjectName(QStringLiteral("appHeaderTitle"));
    m_headerSubtitle = new QLabel;
    m_headerSubtitle->setObjectName(QStringLiteral("appHeaderSubtitle"));
    titleBox->addWidget(title);
    titleBox->addWidget(m_headerSubtitle);
    header->addLayout(titleBox);
    header->addStretch();

    m_refreshButton = new QPushButton(QStringLiteral("刷新"));
    m_refreshButton->setObjectName(QStringLiteral("primaryButton"));
    connect(m_refreshButton, &QPushButton::clicked,
            m_scheduler, &RefreshScheduler::refreshAll);
    auto *settingsButton = new QPushButton(QStringLiteral("设置"));
    settingsButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    header->addWidget(m_refreshButton);
    header->addWidget(settingsButton);
    rootLayout->addLayout(header);

    // ---- 内容区：侧栏 + 卡片滚动区 ----
    auto *content = new QHBoxLayout;
    content->setSpacing(14);

    auto *sidebarBox = new QVBoxLayout;
    sidebarBox->setContentsMargins(0, 0, 0, 0);
    sidebarBox->setSpacing(6);
    auto *section = new QLabel(QStringLiteral("数据源"));
    section->setObjectName(QStringLiteral("sidebarSection"));
    sidebarBox->addWidget(section);

    m_sidebar = new QListWidget;
    m_sidebar->setFixedWidth(172);
    m_sidebar->setIconSize(QSize(12, 12));
    connect(m_sidebar, &QListWidget::currentRowChanged,
            this, [this](int) { relayoutCards(); });
    sidebarBox->addWidget(m_sidebar, 1);

    auto *version = new QLabel(QStringLiteral("v%1 · 单机开源")
                                   .arg(QApplication::applicationVersion()));
    version->setObjectName(QStringLiteral("appHeaderSubtitle"));
    sidebarBox->addWidget(version);
    content->addLayout(sidebarBox);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    auto *container = new QWidget;
    m_grid = new QGridLayout(container);
    m_grid->setSpacing(14);
    scrollArea->setWidget(container);
    content->addWidget(scrollArea, 1);

    rootLayout->addLayout(content, 1);
    setCentralWidget(central);
    resize(1060, 680);
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

    if (providers.isEmpty()) {
        m_grid->addWidget(new QLabel(QStringLiteral(
            "没有启用的数据源。点右上角「设置」添加或启用。")), 0, 0);
    } else {
        relayoutCards();
    }

    updateHeader();
    updateSidebarStatus();
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
    updateHeader();
    updateSidebarStatus();
    updateTraySummary();
}

void MainWindow::updateHeader()
{
    const int total = m_manager->providers().size();
    QDateTime latest;
    for (const ProviderSnapshot &snapshot : std::as_const(m_latest)) {
        if (snapshot.fetchedAt > latest)
            latest = snapshot.fetchedAt;
    }
    const QString timeText = latest.isValid()
        ? latest.toString(QStringLiteral("HH:mm:ss"))
        : QStringLiteral("—");
    m_headerSubtitle->setText(QStringLiteral("共 %1 个数据源 · 最近更新 %2")
                                  .arg(total).arg(timeText));
}

void MainWindow::updateSidebarStatus()
{
    for (int row = 1; row < m_sidebar->count(); ++row) {
        QListWidgetItem *item = m_sidebar->item(row);
        const QString id = item->data(Qt::UserRole).toString();
        const ProviderSnapshot snapshot = m_latest.value(id);

        QColor color = statusColorForPercent(-1);          // 无数据：灰
        if (!snapshot.providerId.isEmpty()) {
            color = snapshot.ok()
                ? statusColorForPercent(snapshot.worstQuotaPercent())
                : statusColorForPercent(1.0);              // 失败：红
        }
        item->setIcon(makeDotIcon(color));
    }
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
