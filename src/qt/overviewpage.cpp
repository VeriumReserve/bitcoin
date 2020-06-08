// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/overviewpage.h>
#include <qt/forms/ui_overviewpage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>

#include <miner.h>
#include <pow.h>
#include <rpc/blockchain.h>
#include <util/system.h>
#include <validation.h>
#include <wallet/wallet.h>

#include <thread>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

#define DECORATION_SIZE 35
#define ICON_SIZE 16
#define MARGIN_SIZE 6
#define NUM_ITEMS 7

Q_DECLARE_METATYPE(interfaces::WalletBalances)

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(BitcoinUnits::BTC),
        platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;

        // Anti aliasing + border
        painter->setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor(217, 217, 217), 1);
        painter->setPen(pen);

        // create rounded rectangle that contain the TX
        QPainterPath mainPath;
        QRect txRect(mainRect.left() + MARGIN_SIZE, mainRect.top() + MARGIN_SIZE, mainRect.width() - MARGIN_SIZE*2, DECORATION_SIZE);
        mainPath.addRoundedRect(txRect, 5, 5);
        painter->fillPath(mainPath, QColor(230, 230, 230));
        painter->drawPath(mainPath);

        // Add icon
        QRect iconRect(txRect.left() + 5, txRect.top() + ((DECORATION_SIZE - ICON_SIZE)/2), ICON_SIZE, ICON_SIZE );
        icon.paint(painter, iconRect);

        // Create Text Rect
        QRect textRect(iconRect.right() + 5, txRect.top(), txRect.width() - iconRect.right() - 5, DECORATION_SIZE);

        // Set default font
        QFont font("Lato", 9);
        painter->setPen(QColor(102, 102, 102));
        painter->setFont(font);

        // Write Date & Address
        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        painter->drawText(textRect, Qt::AlignLeft|Qt::AlignVCenter, QString("%1  -  %2").arg(GUIUtil::dateTimeStr(date)).arg(address));

        // Write amount
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();

        QColor foreground(53, 155, 55);
        if(amount < 0)
        {
            foreground = QColor(233, 58, 93);
        }
        else if(!confirmed)
        {
            foreground = QColor(81, 177, 242);
        }

        painter->setPen(foreground);
        QFont fontHeavy("Lato", 9, QFont::ExtraBold);
        painter->setFont(fontHeavy);

        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(textRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        // if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        // {
        //     QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
        //     QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
        //     iconWatchonly.paint(painter, watchonlyRect);
        // }


        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE+MARGIN_SIZE*2);
    }

    int unit;
    const PlatformStyle *platformStyle;

};


#include <qt/overviewpage.moc>

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    miningState(false),
    maxThread(std::thread::hardware_concurrency()),
    ui(new Ui::OverviewPage),
    clientModel(nullptr),
    walletModel(nullptr),
    txdelegate(new TxViewDelegate(platformStyle, this))
{
    ui->setupUi(this);

    for (int i = 0; i < ui->topBoxes->count(); ++i)
    {
        QWidget *box = ui->topBoxes->itemAt(i)->widget();
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
        shadow->setOffset(QPointF(5, 5));
        shadow->setBlurRadius(20.0);
        box->setGraphicsEffect(shadow);
    }


    m_balances.balance = -1;

    // Set mining thread at Max thread - 1
    ui->minerThreadNumber->setRange(1,maxThread);
    int procDefault = (maxThread-1);
    ui->minerThreadNumber->setValue(procDefault);

    // Recent transactions
    ui->lastTransactionsContent->setItemDelegate(txdelegate);
    ui->lastTransactionsContent->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE + (MARGIN_SIZE * 2)));
    ui->lastTransactionsContent->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + (MARGIN_SIZE * 2)));
    ui->lastTransactionsContent->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->lastTransactionsContent, &QListView::clicked, this, &OverviewPage::handleTransactionClicked);

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->availableWarn, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);
    connect(ui->unconfirmedWarn, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);
    connect(ui->immatureWarn, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);
    connect(ui->totalWarn, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);

    // Prepare update miner statistics
    updateMiningStatsTimer = new QTimer(this);
    connect(updateMiningStatsTimer, &QTimer::timeout, this, &OverviewPage::updateMiningStatistics);

    // Prepare mining animation
    miningOnMovie = new QMovie(":/movies/mining_on", "gif", this);
    connect(miningOnMovie, &QMovie::frameChanged, [this](int frame) {
      if(miningState)
        ui->mineButton->setIcon(QIcon(miningOnMovie->currentPixmap()));
    });
    if (miningOnMovie->loopCount() != -1)
        connect(miningOnMovie, &QMovie::finished, miningOnMovie, &QMovie::start);

    // manage receive/send button
    ui->receiveBox->installEventFilter(this);
    ui->sendBox->installEventFilter(this);
}

bool OverviewPage::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        if(object->objectName() == ui->receiveBox->objectName())
            Q_EMIT receiveClicked();
        else if(object->objectName() == ui->sendBox->objectName())
            Q_EMIT sendClicked();
    }
    return QWidget::eventFilter(object, event);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;
    if (walletModel->privateKeysDisabled()) {
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balances.watch_only_balance, false, BitcoinUnits::separatorAlways));
        ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, balances.unconfirmed_watch_only_balance, false, BitcoinUnits::separatorAlways));
        ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, balances.immature_watch_only_balance, false, BitcoinUnits::separatorAlways));
        ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balances.watch_only_balance + balances.unconfirmed_watch_only_balance + balances.immature_watch_only_balance, false, BitcoinUnits::separatorAlways));
    } else {
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balances.balance, false, BitcoinUnits::separatorAlways));
        ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, balances.unconfirmed_balance, false, BitcoinUnits::separatorAlways));
        ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, balances.immature_balance, false, BitcoinUnits::separatorAlways));
        ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balances.balance + balances.unconfirmed_balance + balances.immature_balance, false, BitcoinUnits::separatorAlways));
        ui->labelWatchAvailable->setText(BitcoinUnits::formatWithUnit(unit, balances.watch_only_balance, false, BitcoinUnits::separatorAlways));
        ui->labelWatchUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, balances.unconfirmed_watch_only_balance, false, BitcoinUnits::separatorAlways));
        ui->labelWatchImmature->setText(BitcoinUnits::formatWithUnit(unit, balances.immature_watch_only_balance, false, BitcoinUnits::separatorAlways));
        ui->labelWatchTotal->setText(BitcoinUnits::formatWithUnit(unit, balances.watch_only_balance + balances.unconfirmed_watch_only_balance + balances.immature_watch_only_balance, false, BitcoinUnits::separatorAlways));
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->watchAvailableBox->setVisible(showWatchOnly);
    ui->watchUnconfirmedBox->setVisible(showWatchOnly);
    ui->watchImmatureBox->setVisible(showWatchOnly);
    ui->watchTotalBox->setVisible(showWatchOnly);
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (model) {
        // Show warning, for example if this is a prerelease version
        connect(model, &ClientModel::alertsChanged, this, &OverviewPage::updateAlerts);
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->lastTransactionsContent->setModel(filter.get());
        ui->lastTransactionsContent->setModelColumn(TransactionTableModel::ToAddress);

        if( filter->rowCount() != 0 ) {
            ui->lastTransactionsContent->setVisible(true);
            ui->lastTransactionsEmpty->setVisible(false);
        } else {
            ui->lastTransactionsContent->setVisible(false);
            ui->lastTransactionsEmpty->setVisible(true);
        }

        // Keep up to date with wallet
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, &WalletModel::balanceChanged, this, &OverviewPage::setBalance);

        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &OverviewPage::updateDisplayUnit);

        updateWatchOnlyLabels(wallet.haveWatchOnly() && !model->privateKeysDisabled());
        connect(model, &WalletModel::notifyWatchonlyChanged, [this](bool showWatchOnly) {
            updateWatchOnlyLabels(showWatchOnly && !walletModel->privateKeysDisabled());
        });
    }

    // update the display unit, to not use the default ("VRM")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->lastTransactionsContent->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->availableWarn->setVisible(fShow);
    ui->unconfirmedWarn->setVisible(fShow);
    ui->immatureWarn->setVisible(fShow);
    ui->totalWarn->setVisible(fShow);
}

// Verium Mining
// manageMiningState will be the entry point for start / stop mining. It will also udpdate the view
void OverviewPage::manageMiningState(bool state, int procs)
{
    // get the current wallet
    // XXX: What should we do when we change to another wallet ?
    // Currently, you can mine with multiple wallet in same time
    if ( ! walletModel )
      return;

    std::string walletName = walletModel->getWalletName().toStdString();
    std::shared_ptr<CWallet> const wallet = GetWallet(walletName);
    CWallet* const pwallet = wallet.get();

    QString qSprocs = QString::number(procs);
    std::string Sprocs = qSprocs.toStdString();
    gArgs.SoftSetArg("-genproclimit", Sprocs);

    if( state != miningState )
    {
        miningState = state;
        GenerateVerium(miningState, pwallet, procs);
    }

    // Verium Mining is OFF, let's update view
    if ( ! miningState )
    {
        updateMiningStatsTimer->stop();
        ui->mineButton->setIcon(QIcon(":/icons/verium"));
        miningOnMovie->stop();
        ui->labelMinerHashrate->setText("--- H/m");
        ui->labelEstNextReward->setText("--- Day(s)");
        ui->labelMinerButton->setText(tr("Click to start:"));
    }
    else
    {
        // Update stats every 1s
        miningOnMovie->start();
        updateMiningStatsTimer->start(1000);
        ui->labelMinerButton->setText(tr("Click to stop:"));
    }
}

void OverviewPage::on_mineButton_clicked()
{
    // check client is in sync
    QDateTime lastBlockDate = QDateTime::fromTime_t(clientModel->getHeaderTipTime());
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    int count = clientModel->getHeaderTipHeight();
    int nTotalBlocks = GetNumBlocksOfPeers();
    int peers = clientModel->getNumConnections();

    if((secs > 90*60 && count < nTotalBlocks && !miningState) || (peers < 1 && !miningState))
    {
        QMessageBox::warning(this, tr("Mining"),
            tr("Please wait until fully in sync with network to mine."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    // check for recommended processor usage and warn
    if (ui->minerThreadNumber->value() == maxThread && !miningState)
    {
        QMessageBox::warning(this, tr("Mining"),
            tr("For optimal performace and stability, it is recommended to keep one processor free for the operating system. Please reduce processor by one."),
            QMessageBox::Ok, QMessageBox::Ok);
    }

    if( ! miningState )
        OverviewPage::manageMiningState(true, ui->minerThreadNumber->value());
    else
        OverviewPage::manageMiningState(false);

}

void OverviewPage::on_minerThreadNumber_valueChanged(int procs)
{
    OverviewPage::manageMiningState(false, procs);
}

void OverviewPage::updateMiningStatistics()
{
    // XXX: Refacto to stop calling rpc, the active chain & co ...
    double totalhashrate = hashrate;
    double nethashrate = GetPoWKHashPM();
    double blocktime = (double)calculateBlocktime(::ChainActive().Tip())/60;
    double minerate;
    if (totalhashrate == 0.0){minerate = 0.0;}
    else{
        minerate = 0.694*(nethashrate*blocktime)/(totalhashrate);  //((100/((totalhashrate_Hpm/(nethashrate_kHpm*1000))*100))*blocktime_min)/60*24
    }

    ui->labelMinerHashrate->setText(QString("%1 H/m").arg(QString::number(totalhashrate, 'f', 3)));
    ui->labelEstNextReward->setText(QString("%1 Day(s)").arg(QString::number(minerate, 'f', 1)));
}