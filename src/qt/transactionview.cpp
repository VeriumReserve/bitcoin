// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionview.h>

#include <qt/addresstablemodel.h>
#include <qt/bitcoinunits.h>
#include <qt/csvmodelwriter.h>
#include <qt/editaddressdialog.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactiondescdialog.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactionrecord.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>

#include <ui_interface.h>

#include <QApplication>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QTableView>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QGridLayout>

TransactionView::TransactionView(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent), model(nullptr), transactionProxyModel(nullptr),
    transactionView(nullptr), columnResizingFixer(nullptr)
{
    // Build filter row
    setContentsMargins(0,0,0,0);

    QWidget *filterBox = new QWidget();
    filterBox->setObjectName("filterBox");
    QVBoxLayout *filterBoxLayout = new QVBoxLayout();
    filterBoxLayout->setMargin(0);
    filterBoxLayout->setSpacing(0);
    filterBox->setLayout(filterBoxLayout);

    QLabel *filterTitle = new QLabel(tr("Filters"));
    filterTitle->setObjectName("filterTitle");
    filterBoxLayout->addWidget(filterTitle);

    QGridLayout *filtersLayout = new QGridLayout();
    filtersLayout->setContentsMargins(4,0,4,4);
    filtersLayout->setSpacing(4);
    filterBoxLayout->addLayout(filtersLayout);

    searchWidget = new QLineEdit(this);
    searchWidget->setObjectName("searchWidget");
    searchWidget->setPlaceholderText(tr("Enter address, transaction id, or label to search"));
    QLabel *searchFilterTitle = new QLabel(tr("&Search: "));
    searchFilterTitle->setObjectName("addressFilterTitle");
    searchFilterTitle->setAlignment(Qt::AlignRight);
    searchFilterTitle->setBuddy(searchWidget);
    filtersLayout->addWidget(searchFilterTitle, 0, 0);
    filtersLayout->addWidget(searchWidget, 0, 1);

    typeWidget = new QComboBox(this);
    typeWidget->setObjectName("typeWidget");
    typeWidget->addItem(tr("All"), TransactionFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Received with"), TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) |
                                        TransactionFilterProxy::TYPE(TransactionRecord::RecvFromOther));
    typeWidget->addItem(tr("Sent to"), TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) |
                                  TransactionFilterProxy::TYPE(TransactionRecord::SendToOther));
    typeWidget->addItem(tr("To yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SendToSelf));
    typeWidget->addItem(tr("Mined"), TransactionFilterProxy::TYPE(TransactionRecord::Generated));
    typeWidget->addItem(tr("Other"), TransactionFilterProxy::TYPE(TransactionRecord::Other));
    QLabel *typeFilterTitle = new QLabel(tr("&Type: "));
    typeFilterTitle->setObjectName("typeFilterTitle");
    typeFilterTitle->setAlignment(Qt::AlignRight);
    typeFilterTitle->setBuddy(typeWidget);
    filtersLayout->addWidget(typeFilterTitle, 1, 0);
    filtersLayout->addWidget(typeWidget, 1, 1);

    amountWidget = new QLineEdit(this);
    amountWidget->setObjectName("amountWidget");
    amountWidget->setPlaceholderText(tr("Min amount"));
    QDoubleValidator *amountValidator = new QDoubleValidator(0, 1e20, 8, this);
    QLocale amountLocale(QLocale::C);
    amountLocale.setNumberOptions(QLocale::RejectGroupSeparator);
    amountValidator->setLocale(amountLocale);
    amountWidget->setValidator(amountValidator);
    QLabel *amountFilterTitle = new QLabel(tr("&Amount: "));
    amountFilterTitle->setObjectName("amountFilterTitle");
    amountFilterTitle->setAlignment(Qt::AlignRight);
    amountFilterTitle->setBuddy(amountWidget);
    filtersLayout->addWidget(amountFilterTitle, 2, 0);
    filtersLayout->addWidget(amountWidget, 2, 1);

    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->setContentsMargins(0,0,0,0);
    dateLayout->setSpacing(0);

    dateWidget = new QComboBox(this);
    dateWidget->setObjectName("dateWidget");
    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);
    QLabel *dateFilterTitle = new QLabel(tr("&Date: "));
    dateFilterTitle->setObjectName("dateFilterTitle");
    dateFilterTitle->setAlignment(Qt::AlignRight);
    dateFilterTitle->setBuddy(dateWidget);

    dateLayout->addWidget(dateWidget);
    dateLayout->addWidget(createDateRangeWidget());

    filtersLayout->addWidget(dateFilterTitle, 3, 0);
    filtersLayout->addLayout(dateLayout, 3, 1);


    // Delay before filtering transactions in ms
    static const int input_filter_delay = 200;

    QTimer* amount_typing_delay = new QTimer(this);
    amount_typing_delay->setSingleShot(true);
    amount_typing_delay->setInterval(input_filter_delay);

    QTimer* prefix_typing_delay = new QTimer(this);
    prefix_typing_delay->setSingleShot(true);
    prefix_typing_delay->setInterval(input_filter_delay);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(6);

    QTableView *view = new QTableView(this);
    vlayout->addWidget(filterBox);
    vlayout->addWidget(view);

    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    view->installEventFilter(this);

    transactionView = view;
    transactionView->setObjectName("transactionView");


    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *copyTxHexAction = new QAction(tr("Copy raw transaction"), this);
    QAction *copyTxPlainText = new QAction(tr("Copy full transaction details"), this);
    QAction *editLabelAction = new QAction(tr("Edit label"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);

    contextMenu = new QMenu(this);
    contextMenu->setObjectName("contextMenu");
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(copyTxHexAction);
    contextMenu->addAction(copyTxPlainText);
    contextMenu->addAction(showDetailsAction);
    contextMenu->addSeparator();
    contextMenu->addAction(editLabelAction);

    connect(dateWidget, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &TransactionView::chooseDate);
    connect(typeWidget, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &TransactionView::chooseType);
    connect(amountWidget, &QLineEdit::textChanged, amount_typing_delay, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(amount_typing_delay, &QTimer::timeout, this, &TransactionView::changedAmount);
    connect(searchWidget, &QLineEdit::textChanged, prefix_typing_delay, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(prefix_typing_delay, &QTimer::timeout, this, &TransactionView::changedSearch);

    connect(view, &QTableView::doubleClicked, this, &TransactionView::doubleClicked);
    connect(view, &QTableView::customContextMenuRequested, this, &TransactionView::contextualMenu);

    connect(copyAddressAction, &QAction::triggered, this, &TransactionView::copyAddress);
    connect(copyLabelAction, &QAction::triggered, this, &TransactionView::copyLabel);
    connect(copyAmountAction, &QAction::triggered, this, &TransactionView::copyAmount);
    connect(copyTxIDAction, &QAction::triggered, this, &TransactionView::copyTxID);
    connect(copyTxHexAction, &QAction::triggered, this, &TransactionView::copyTxHex);
    connect(copyTxPlainText, &QAction::triggered, this, &TransactionView::copyTxPlainText);
    connect(editLabelAction, &QAction::triggered, this, &TransactionView::editLabel);
    connect(showDetailsAction, &QAction::triggered, this, &TransactionView::showDetails);
    // Double-clicking on a transaction on the transaction history page shows details
    connect(this, &TransactionView::doubleClicked, this, &TransactionView::showDetails);
}

void TransactionView::setModel(WalletModel *_model)
{
    this->model = _model;
    if(_model)
    {
        transactionProxyModel = new TransactionFilterProxy(this);
        transactionProxyModel->setSourceModel(_model->getTransactionTableModel());
        transactionProxyModel->setDynamicSortFilter(true);
        transactionProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        transactionProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        transactionProxyModel->setSortRole(Qt::EditRole);

        transactionView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        transactionView->setModel(transactionProxyModel);
        transactionView->setAlternatingRowColors(true);
        transactionView->setSelectionBehavior(QAbstractItemView::SelectRows);
        transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transactionView->horizontalHeader()->setSortIndicator(TransactionTableModel::Date, Qt::DescendingOrder);
        transactionView->setSortingEnabled(true);
        transactionView->verticalHeader()->hide();

        transactionView->setColumnWidth(TransactionTableModel::Status, STATUS_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Watchonly, WATCHONLY_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Date, DATE_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Type, TYPE_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Amount, AMOUNT_MINIMUM_COLUMN_WIDTH);

        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(transactionView, AMOUNT_MINIMUM_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH, this);

        if (_model->getOptionsModel())
        {
            // Add third party transaction URLs to context menu
            QStringList listUrls = _model->getOptionsModel()->getThirdPartyTxUrls().split("|", QString::SkipEmptyParts);
            for (int i = 0; i < listUrls.size(); ++i)
            {
                QString url = listUrls[i].trimmed();
                QString host = QUrl(url, QUrl::StrictMode).host();
                if (!host.isEmpty())
                {
                    QAction *thirdPartyTxUrlAction = new QAction(host, this); // use host as menu item label
                    if (i == 0)
                        contextMenu->addSeparator();
                    contextMenu->addAction(thirdPartyTxUrlAction);
                    connect(thirdPartyTxUrlAction, &QAction::triggered, [this, url] { openThirdPartyTxUrl(url); });
                }
            }
        }

        // show/hide column Watch-only
        updateWatchOnlyColumn(_model->wallet().haveWatchOnly());

        // Watch-only signal
        connect(_model, &WalletModel::notifyWatchonlyChanged, this, &TransactionView::updateWatchOnlyColumn);
    }
}

void TransactionView::chooseDate(int idx)
{
    if (!transactionProxyModel) return;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        transactionProxyModel->setDateRange(
                TransactionFilterProxy::MIN_DATE,
                TransactionFilterProxy::MAX_DATE);
        break;
    case Today:
        transactionProxyModel->setDateRange(
                QDateTime(current),
                TransactionFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        transactionProxyModel->setDateRange(
                QDateTime(startOfWeek),
                TransactionFilterProxy::MAX_DATE);

        } break;
    case ThisMonth:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1)),
                TransactionFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), 1, 1)),
                TransactionFilterProxy::MAX_DATE);
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
}

void TransactionView::chooseType(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setTypeFilter(
        typeWidget->itemData(idx).toInt());
}

void TransactionView::changedSearch()
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setSearchString(searchWidget->text());
}

void TransactionView::changedAmount()
{
    if(!transactionProxyModel)
        return;
    CAmount amount_parsed = 0;
    if (BitcoinUnits::parse(model->getOptionsModel()->getDisplayUnit(), amountWidget->text(), &amount_parsed)) {
        transactionProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        transactionProxyModel->setMinAmount(0);
    }
}

void TransactionView::exportClicked()
{
    if (!model || !model->getOptionsModel()) {
        return;
    }

    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Transaction History"), QString(),
        tr("Comma separated file (*.csv)"), nullptr);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(transactionProxyModel);
    writer.addColumn(tr("Confirmed"), 0, TransactionTableModel::ConfirmedRole);
    if (model->wallet().haveWatchOnly())
        writer.addColumn(tr("Watch-only"), TransactionTableModel::Watchonly);
    writer.addColumn(tr("Date"), 0, TransactionTableModel::DateRole);
    writer.addColumn(tr("Type"), TransactionTableModel::Type, Qt::EditRole);
    writer.addColumn(tr("Label"), 0, TransactionTableModel::LabelRole);
    writer.addColumn(tr("Address"), 0, TransactionTableModel::AddressRole);
    writer.addColumn(BitcoinUnits::getAmountColumnTitle(model->getOptionsModel()->getDisplayUnit()), 0, TransactionTableModel::FormattedAmountRole);
    writer.addColumn(tr("ID"), 0, TransactionTableModel::TxHashRole);

    if(!writer.write()) {
        Q_EMIT message(tr("Exporting Failed"), tr("There was an error trying to save the transaction history to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
    }
    else {
        Q_EMIT message(tr("Exporting Successful"), tr("The transaction history was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void TransactionView::contextualMenu(const QPoint &point)
{
    QModelIndex index = transactionView->indexAt(point);
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    // check if transaction can be abandoned, disable context menu action in case it doesn't
    uint256 hash;
    hash.SetHex(selection.at(0).data(TransactionTableModel::TxHashRole).toString().toStdString());
    if(index.isValid())
    {
        contextMenu->popup(transactionView->viewport()->mapToGlobal(point));
    }
}


void TransactionView::copyAddress()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::AddressRole);
}

void TransactionView::copyLabel()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::LabelRole);
}

void TransactionView::copyAmount()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::FormattedAmountRole);
}

void TransactionView::copyTxID()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxHashRole);
}

void TransactionView::copyTxHex()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxHexRole);
}

void TransactionView::copyTxPlainText()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxPlainTextRole);
}

void TransactionView::editLabel()
{
    if(!transactionView->selectionModel() ||!model)
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        AddressTableModel *addressBook = model->getAddressTableModel();
        if(!addressBook)
            return;
        QString address = selection.at(0).data(TransactionTableModel::AddressRole).toString();
        if(address.isEmpty())
        {
            // If this transaction has no associated address, exit
            return;
        }
        // Is address in address book? Address book can miss address when a transaction is
        // sent from outside the UI.
        int idx = addressBook->lookupAddress(address);
        if(idx != -1)
        {
            // Edit sending / receiving address
            QModelIndex modelIdx = addressBook->index(idx, 0, QModelIndex());
            // Determine type of address, launch appropriate editor dialog type
            QString type = modelIdx.data(AddressTableModel::TypeRole).toString();

            EditAddressDialog dlg(
                type == AddressTableModel::Receive
                ? EditAddressDialog::EditReceivingAddress
                : EditAddressDialog::EditSendingAddress, this);
            dlg.setModel(addressBook);
            dlg.loadRow(idx);
            dlg.exec();
        }
        else
        {
            // Add sending address
            EditAddressDialog dlg(EditAddressDialog::NewSendingAddress,
                this);
            dlg.setModel(addressBook);
            dlg.setAddress(address);
            dlg.exec();
        }
    }
}

void TransactionView::showDetails()
{
    if(!transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog *dlg = new TransactionDescDialog(selection.at(0));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}

void TransactionView::openThirdPartyTxUrl(QString url)
{
    if(!transactionView || !transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
         QDesktopServices::openUrl(QUrl::fromUserInput(url.replace("%s", selection.at(0).data(TransactionTableModel::TxHashRole).toString())));
}

QWidget *TransactionView::createDateRangeWidget()
{
    dateRangeWidget = new QWidget();
    dateRangeWidget->setObjectName("dateRangeWidget");
    dateRangeWidget->setContentsMargins(0,0,0,0);
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    QLabel *fromTitle = new QLabel(tr("From:"));
    fromTitle->setObjectName("fromTitle");
    fromTitle->setContentsMargins(0,0,0,0);
    layout->addWidget(fromTitle);

    dateFrom = new QDateTimeEdit(this);
    dateFrom->setObjectName("dateFrom");
    dateFrom->setDisplayFormat("dd/MM/yy");
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    dateFrom->setDate(QDate::currentDate().addDays(-7));
    dateFrom->setContentsMargins(0,0,0,0);
    layout->addWidget(dateFrom);

    QLabel *toTitle = new QLabel(tr("To:"));
    toTitle->setContentsMargins(0,0,0,0);
    toTitle->setObjectName("toTitle");
    layout->addWidget(toTitle);

    dateTo = new QDateTimeEdit(this);
    dateTo->setObjectName("dateTo");
    dateTo->setDisplayFormat("dd/MM/yy");
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    dateTo->setDate(QDate::currentDate());
    dateTo->setContentsMargins(0,0,0,0);
    layout->addWidget(dateTo);

    // Hide by default
    dateRangeWidget->setVisible(false);
    dateRangeWidget->setFixedHeight(25);

    // Notify on change
    connect(dateFrom, &QDateTimeEdit::dateChanged, this, &TransactionView::dateRangeChanged);
    connect(dateTo, &QDateTimeEdit::dateChanged, this, &TransactionView::dateRangeChanged);

    return dateRangeWidget;
}

void TransactionView::dateRangeChanged()
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setDateRange(
            QDateTime(dateFrom->date()),
            QDateTime(dateTo->date()).addDays(1));
}

void TransactionView::focusTransaction(const QModelIndex &idx)
{
    if(!transactionProxyModel)
        return;
    QModelIndex targetIdx = transactionProxyModel->mapFromSource(idx);
    transactionView->scrollTo(targetIdx);
    transactionView->setCurrentIndex(targetIdx);
    transactionView->setFocus();
}

void TransactionView::focusTransaction(const uint256& txid)
{
    if (!transactionProxyModel)
        return;

    const QModelIndexList results = this->model->getTransactionTableModel()->match(
        this->model->getTransactionTableModel()->index(0,0),
        TransactionTableModel::TxHashRole,
        QString::fromStdString(txid.ToString()), -1);

    transactionView->setFocus();
    transactionView->selectionModel()->clearSelection();
    for (const QModelIndex& index : results) {
        const QModelIndex targetIndex = transactionProxyModel->mapFromSource(index);
        transactionView->selectionModel()->select(
            targetIndex,
            QItemSelectionModel::Rows | QItemSelectionModel::Select);
        // Called once per destination to ensure all results are in view, unless
        // transactions are not ordered by (ascending or descending) date.
        transactionView->scrollTo(targetIndex);
        // scrollTo() does not scroll far enough the first time when transactions
        // are ordered by ascending date.
        if (index == results[0]) transactionView->scrollTo(targetIndex);
    }
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void TransactionView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(TransactionTableModel::ToAddress);
}

// Need to override default Ctrl+C action for amount as default behaviour is just to copy DisplayRole text
bool TransactionView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers().testFlag(Qt::ControlModifier))
        {
             GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxPlainTextRole);
             return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// show/hide column Watch-only
void TransactionView::updateWatchOnlyColumn(bool fHaveWatchOnly)
{
    transactionView->setColumnHidden(TransactionTableModel::Watchonly, !fHaveWatchOnly);
}
