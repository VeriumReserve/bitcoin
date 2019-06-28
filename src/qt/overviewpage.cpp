// Copyright (c) 2011-2017 The Bitcoin Core developers
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
#include <util.h>
#include <thread>
#include <miner.h>
#include <wallet/wallet.h>
#include <rpc/blockchain.h>
#include <pow.h>
#include <validation.h>

#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 54
#define NUM_ITEMS 5

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
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon = platformStyle->SingleColorIcon(icon);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include <qt/overviewpage.moc>

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    mining(gArgs.GetBoolArg("-gen",false)),
    processors(std::thread::hardware_concurrency()),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    txdelegate(new TxViewDelegate(platformStyle, this))
{
    // Setup header and styles
    GUIUtil::header(this, QString(""));

    ui->setupUi(this);
    this->layout()->setContentsMargins(0, 0 + HEADER_HEIGHT, 0, 0);

    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)
 // XXX   ui->labelTransactionsStatus->setIcon(icon);
 // XXX   ui->labelWalletStatus->setIcon(icon);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->labelWalletStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelTransactionsStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));

    // set initial state of processor spin box
    ui->spinBox->setRange(1,processors);
    int procDefault = (processors-1);
    ui->spinBox->setValue(procDefault);

    ui->labelBalance->setFont(qFontLargerBold);
    ui->labelTransactions->setFont(qFontLargerBold);
    ui->labelNetwork->setFont(qFontLargerBold);
    ui->labelValue->setFont(qFontLargerBold);

    ui->labelSpendableText->setFont(qFont);
    ui->labelSpendable->setFont(qFont);
    ui->labelImmatureText->setFont(qFont);
    ui->labelImmature->setFont(qFont);
    ui->labelUnconfirmedText->setFont(qFont);
    ui->labelUnconfirmed->setFont(qFont);
    ui->labelTotalText->setFont(qFont);
    ui->labelTotal->setFont(qFont);

    // minersection
    ui->miningLabel->setFont(qFont);
    ui->proclabel->setFont(qFont);
    ui->spinBox->setFont(qFont);

    // statistics section
    ui->difficultyText->setFont(qFont);
    ui->difficulty->setFont(qFont);
    ui->blocktimeText->setFont(qFont);
    ui->blocktime->setFont(qFont);
    ui->blockrewardText->setFont(qFont);
    ui->blockreward->setFont(qFont);
    ui->nethashrateText->setFont(qFont);
    ui->nethashrate->setFont(qFont);
    ui->hashrateText->setFont(qFont);
    ui->hashrate->setFont(qFont);
    ui->mineRateText->setFont(qFont);
    ui->mineRate->setFont(qFont);
    ui->blocknumberText->setFont(qFont);
    ui->blocknumber->setFont(qFont);
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

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    ui->labelSpendable->setText(BitcoinUnits::formatWithUnit(unit, balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorAlways));
    // XXX ui->labelWatchAvailable->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    // XXX ui->labelWatchPending->setText(BitcoinUnits::formatWithUnit(unit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    // XXX ui->labelWatchImmature->setText(BitcoinUnits::formatWithUnit(unit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    // XXX ui->labelWatchTotal->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    // XXX ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    // XXX ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    // XXX ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    // XXX ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    // XXX ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    // XXX ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    // XXX ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    // XXX ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    // XXX ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    // XXX if (!showWatchOnly)
    // XXX    ui->labelWatchImmature->hide();
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
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

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    // XXX this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    // XXX this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::on_mineButton_clicked()
{
    // check client is in sync
    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    int count = clientModel->getNumBlocks();
    int nTotalBlocks = GetNumBlocksOfPeers();
    int peers = clientModel->getNumConnections();
    ui->mineButton->clearFocus();

    if((secs > 90*60 && count < nTotalBlocks && !mining) || (peers < 1 && !mining))
    {
        QMessageBox::warning(this, tr("Mining"),
            tr("Please wait until fully in sync with network to mine."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    // check for recommended processor usage and warn
    if (ui->spinBox->value() == processors && !mining)
    {
        QMessageBox::warning(this, tr("Mining"),
            tr("For optimal performace and stability, it is recommended to keep one processor free for the operating system. Please reduce processor by one."),
            QMessageBox::Ok, QMessageBox::Ok);
    }

    // toggle mining
    CWallet * const pwallet = vpwallets[0];
    if (!mining)
    {
        // XXX miningMovie->start();
        GenerateVerium(true, pwallet, ui->spinBox->value());
        mining = true;
        MilliSleep(100);
        ui->miningLabel->setText("Click to stop:");
        ui->mineButton->setIcon(QIcon(":/icons/miningon"));
    }
    else
    {
        // XXX miningMovie->stop();
        GenerateVerium(false, pwallet, ui->spinBox->value());
        mining = false;
        ui->miningLabel->setText("Click to start:");
        ui->mineButton->setIcon(QIcon(":/icons/miningoff"));
    }
}

void OverviewPage::on_spinBox_valueChanged(int procs)
{
    CWallet * const pwallet = vpwallets[0];
    if (mining)
    {
        bool onOrOff = false;
        // XXX miningMovie->stop();
        GenerateVerium(onOrOff, pwallet, ui->spinBox->value());
        mining = onOrOff;
        ui->miningLabel->setText("Click to start:");
        ui->mineButton->setIcon(QIcon(":/icons/miningoff"));
    }
    QString qSprocs = QString::number(procs);
    std::string Sprocs = qSprocs.toStdString();
    gArgs.SoftSetArg("-genproclimit", Sprocs);
}

void OverviewPage::setStatistics()
{
    // calculate stats
    int minerate;
    double nethashrate = GetPoWKHashPM();
    double blocktime = (double)calculateBlocktime(chainActive.Tip())/60;
    double totalhashrate = hashrate;
    if (totalhashrate == 0.0){ minerate = 0;}
    else{
        minerate = 0.694*(nethashrate*blocktime)/(totalhashrate);  //((100/((totalhashrate_Hpm/(nethashrate_kHpm*1000))*100))*blocktime_min)/60*24

        // display stats
        ui->difficulty->setText(QString::number(GetDifficulty()));
        ui->blocktime->setText(QString::number(blocktime));
        ui->blocknumber->setText(QString::number(chainActive.Tip()->nHeight));
        ui->nethashrate->setText(QString::number(nethashrate));
        ui->hashrate->setText(QString::number(totalhashrate));
        ui->mineRate->setText(QString::number(minerate));
    }
}
