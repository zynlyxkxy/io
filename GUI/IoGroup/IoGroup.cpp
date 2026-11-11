#include "IoGroup.h"
#include "IoWidget/DiWidget.h"
#include "IoWidget/DoWidget.h"
#include "IoWidget/AiWidget.h"
#include "IoWidget/AoWidget.h"
#include "IoWidget/SettingWidget.h"
#include "IoWidget/BufferWidget.h"
#include "io_client.h"

#include <QScrollArea>
#include <QLabel>

// ═══════════════════════════════════════════════════════════════
// 辅助
// ═══════════════════════════════════════════════════════════════

static inline bool isPlc(int varId)
{
    return (varId / 1000) % 2 == 0;
}

template<typename T>
static QList<int> extractIds(const std::vector<T> &entries)
{
    QList<int> ids;
    for (const auto &e : entries)
    {
        ids.append(e.id);
    }
    return ids;
}

template<typename T>
static QMap<int, QString> extractNameMap(const std::vector<T> &entries)
{
    QMap<int, QString> map;
    for (const auto &e : entries)
    {
        map.insert(e.id, QString::fromStdString(e.name));
    }
    return map;
}

static QScrollArea *makeScrollArea(QWidget *parent)
{
    auto *area = new QScrollArea(parent);
    area->setStyleSheet("QWidget{background-color: transparent;}");
    area->setWidgetResizable(true);
    area->setWidget(new QWidget());
    return area;
}

static QLabel *makeHeader(const QString &text)
{
    auto *lbl = new QLabel(text);
    lbl->setStyleSheet("font-weight: bold; font-size: 14px; color: #4C70FA;"
                       "border-bottom: 2px solid #4C70FA;"
                       "padding-bottom: 2px;");
    lbl->setMaximumHeight(24);
    return lbl;
}

// ═══════════════════════════════════════════════════════════════
// 两列 tab — ACS / PLC
// ═══════════════════════════════════════════════════════════════

template<typename TWidget>
static void buildTwoColTab(const QList<int> &varIds,
                            const QMap<int, QString> &nameMap,
                            QGridLayout *layout,
                            QMap<int, TWidget *> &widgetMap)
{
    layout->addWidget(makeHeader("PLC"), 0, 0);
    layout->addWidget(makeHeader("ACS"), 0, 1);

    for (int id : varIds)
    {
        int col = isPlc(id) ? 0 : 1;
        int row = (id % 1000) + 1;

        TWidget *w = new TWidget(id, nameMap.value(id, QString::number(id)));
        w->setFixedHeight(50);
        layout->addWidget(w, row, col);
        widgetMap.insert(id, w);
    }
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(2, 0);
}

// ═══════════════════════════════════════════════════════════════
// 单列 tab
// ═══════════════════════════════════════════════════════════════

template<typename TWidget>
static void buildSingleColTab(const QList<int> &varIds,
                               const QMap<int, QString> &nameMap,
                               QGridLayout *layout,
                               QMap<int, TWidget *> &widgetMap)
{
    for (int i = 0; i < varIds.size(); ++i)
    {
        int id = varIds[i];
        TWidget *w = new TWidget(id, nameMap.value(id, QString::number(id)));
        w->setFixedHeight(50);
        layout->addWidget(w, i, 0);
        widgetMap.insert(id, w);
    }
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
}

// Setting: 左列 Bool，右列 Double
static void buildSettingGrid(const QList<int> &dblIds,
                              const QMap<int, QString> &dblNameMap,
                              const QList<int> &boolIds,
                              const QMap<int, QString> &boolNameMap,
                              QGridLayout *layout,
                              QMap<int, SettingWidget *> &dblMap,
                              QMap<int, SettingWidget *> &boolMap)
{
    layout->addWidget(makeHeader("Bool"), 0, 0);
    layout->addWidget(makeHeader("Double"), 0, 1);

    int rowBool = 1;
    for (int i = 0; i < boolIds.size(); ++i)
    {
        int id = boolIds[i];
        SettingWidget *w = new SettingWidget(id, boolNameMap.value(id, QString::number(id)),
                                              false);
        w->setFixedHeight(50);
        layout->addWidget(w, rowBool + i, 0);
        boolMap.insert(id, w);
    }

    int rowDbl = 1;
    for (int i = 0; i < dblIds.size(); ++i)
    {
        int id = dblIds[i];
        SettingWidget *w = new SettingWidget(id, dblNameMap.value(id, QString::number(id)),
                                              true);
        w->setFixedHeight(50);
        layout->addWidget(w, rowDbl + i, 1);
        dblMap.insert(id, w);
    }
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
}

// ═══════════════════════════════════════════════════════════════
// IoGroup
// ═══════════════════════════════════════════════════════════════

IoGroup::IoGroup(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // 固定 6 个空 tab，连上 RPC 后由 buildTabs 填充
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("AI"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("DI"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("AO"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("DO"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("Setting"));
    m_tabWidget->addTab(makeScrollArea(this), QStringLiteral("Buffer"));

    connect(this, &IoGroup::sig_ObserverReceived,
            this, &IoGroup::onObserverData, Qt::QueuedConnection);
}

IoGroup::~IoGroup()
{
}

// ═══════════════════════════════════════════════════════════════
// buildTabs — 从 IoServerInfo 填充内容
// ═══════════════════════════════════════════════════════════════

void IoGroup::buildTabs(const IoServerInfo &info)
{
    m_tabWidget->clear();
    m_aiWidgets.clear();
    m_aoWidgets.clear();
    m_diWidgets.clear();
    m_doWidgets.clear();
    m_settingDoubleWidgets.clear();
    m_settingBoolWidgets.clear();
    m_bufferWidgets.clear();

    // AI
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto nameMap = extractNameMap(info.io.AI);
        buildTwoColTab<AiWidget>(extractIds(info.io.AI), nameMap, lay, m_aiWidgets);

        m_tabWidget->addTab(area, QStringLiteral("AI"));
    }

    // DI
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto nameMap = extractNameMap(info.io.DI);
        buildTwoColTab<DiWidget>(extractIds(info.io.DI), nameMap, lay, m_diWidgets);

        m_tabWidget->addTab(area, QStringLiteral("DI"));
    }

    // AO
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto nameMap = extractNameMap(info.io.AO);
        buildTwoColTab<AoWidget>(extractIds(info.io.AO), nameMap, lay, m_aoWidgets);

        m_tabWidget->addTab(area, QStringLiteral("AO"));
    }

    // DO
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto nameMap = extractNameMap(info.io.DO);
        buildTwoColTab<DoWidget>(extractIds(info.io.DO), nameMap, lay, m_doWidgets);

        m_tabWidget->addTab(area, QStringLiteral("DO"));
    }

    // Setting — Double + Bool 合在一个 tab
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto dblNameMap = extractNameMap(info.io.SettingDouble);
        auto boolNameMap = extractNameMap(info.io.SettingBool);
        buildSettingGrid(extractIds(info.io.SettingDouble), dblNameMap,
                         extractIds(info.io.SettingBool), boolNameMap,
                         lay, m_settingDoubleWidgets, m_settingBoolWidgets);

        m_tabWidget->addTab(area, QStringLiteral("Setting"));
    }

    // Buffer
    {
        QScrollArea *area = makeScrollArea(this);
        QGridLayout *lay = new QGridLayout(area->widget());
        lay->setSpacing(0);
        lay->setHorizontalSpacing(20);
        lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        lay->addWidget(makeHeader("Buffer"), 0, 0);

        auto nameMap = extractNameMap(info.io.Buffer);
        auto ids = extractIds(info.io.Buffer);
        for (int i = 0; i < ids.size(); ++i)
        {
            int id = ids[i];
            BufferWidget *w = new BufferWidget(id, nameMap.value(id, QString::number(id)));
            w->setFixedHeight(50);
            lay->addWidget(w, i + 1, 0);
            m_bufferWidgets.insert(id, w);
        }
        lay->setColumnStretch(0, 1);
        lay->setColumnStretch(1, 1);

        m_tabWidget->addTab(area, QStringLiteral("Buffer"));
    }

    if (m_rpc)
    {
        connectSignals();
    }
}

// ═══════════════════════════════════════════════════════════════
// setRpcClient
// ═══════════════════════════════════════════════════════════════

void IoGroup::setRpcClient(IO_Client_Inherit *rpc)
{
    m_rpc = rpc;
    connectSignals();
}

void IoGroup::connectSignals()
{
    for (auto it = m_aoWidgets.begin(); it != m_aoWidgets.end(); ++it)
    {
        connect(it.value(), &AoWidget::setAo, this, [this](int varId, double value)
        {
            if (!m_rpc)
            {
                return;
            }
            IoAoInput in;
            in.varId = varId;
            in.value = value;
            in.verify = false;
            in.expectedValue = value;
            m_rpc->IOSetAo(in);
        });
    }

    for (auto it = m_doWidgets.begin(); it != m_doWidgets.end(); ++it)
    {
        connect(it.value(), &DoWidget::setDo, this, [this](int varId, bool value)
        {
            if (!m_rpc)
            {
                return;
            }
            IoDoInput in;
            in.varId = varId;
            in.value = value;
            in.verify = false;
            in.expectedValue = value;
            m_rpc->IOSetDo(in);
        });
    }

    for (auto it = m_settingDoubleWidgets.begin(); it != m_settingDoubleWidgets.end(); ++it)
    {
        connect(it.value(), &SettingWidget::setSettingDouble, this,
                [this](int varId, double value)
        {
            if (!m_rpc)
            {
                return;
            }
            IoSettingDoubleInput in;
            in.varId = varId;
            in.value = value;
            m_rpc->IOSetSettingDouble(in);
        });
    }

    for (auto it = m_settingBoolWidgets.begin(); it != m_settingBoolWidgets.end(); ++it)
    {
        connect(it.value(), &SettingWidget::setSettingBool, this,
                [this](int varId, bool value)
        {
            if (!m_rpc)
            {
                return;
            }
            IoSettingBoolInput in;
            in.varId = varId;
            in.value = value;
            m_rpc->IOSetSettingBool(in);
        });
    }

    for (auto it = m_bufferWidgets.begin(); it != m_bufferWidgets.end(); ++it)
    {
        connect(it.value(), &BufferWidget::startBuffer, this, [this](int varId)
        {
            if (!m_rpc)
            {
                return;
            }
            IoVarInput in;
            in.varId = varId;
            m_rpc->IOStartBuffer(in);
        });
        connect(it.value(), &BufferWidget::stopBuffer, this, [this](int varId)
        {
            if (!m_rpc)
            {
                return;
            }
            IoVarInput in;
            in.varId = varId;
            m_rpc->IOStopBuffer(in);
        });
    }
}

// ═══════════════════════════════════════════════════════════════
// Observer 数据分发
// ═══════════════════════════════════════════════════════════════

void IoGroup::onObserverData(const IoObserverData &data)
{
    for (const auto &e : data.acs.AI)
    {
        if (m_aiWidgets.contains(e.varId))
        {
            m_aiWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.acs.DI)
    {
        if (m_diWidgets.contains(e.varId))
        {
            m_diWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.acs.AO)
    {
        if (m_aoWidgets.contains(e.varId))
        {
            m_aoWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.acs.DO)
    {
        if (m_doWidgets.contains(e.varId))
        {
            m_doWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.acs.Buf)
    {
        if (m_bufferWidgets.contains(e.varId))
        {
            m_bufferWidgets[e.varId]->display(e.value);
        }
    }

    for (const auto &e : data.plc.AI)
    {
        if (m_aiWidgets.contains(e.varId))
        {
            m_aiWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.plc.DI)
    {
        if (m_diWidgets.contains(e.varId))
        {
            m_diWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.plc.AO)
    {
        if (m_aoWidgets.contains(e.varId))
        {
            m_aoWidgets[e.varId]->display(e.value);
        }
    }
    for (const auto &e : data.plc.DO)
    {
        if (m_doWidgets.contains(e.varId))
        {
            m_doWidgets[e.varId]->display(e.value);
        }
    }
}
