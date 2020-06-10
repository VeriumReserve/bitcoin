// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/receiverequestdialog.h>
#include <qt/forms/ui_receiverequestdialog.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>

#include <QClipboard>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h> /* for USE_QRCODE */
#endif

ReceiveRequestDialog::ReceiveRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveRequestDialog),
    model(nullptr)
{
    ui->setupUi(this);
    for (int i = 0; i < ui->verticalLayout->count(); ++i)
    {
        QWidget *box = ui->verticalLayout->itemAt(i)->widget();
        if( box->isHidden())
            continue;
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
        shadow->setOffset(QPointF(5, 5));
        shadow->setBlurRadius(20.0);
        box->setGraphicsEffect(shadow);
    }

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setOffset(QPointF(5, 5));
    shadow->setBlurRadius(20.0);
    ui->lblQRCode->setGraphicsEffect(shadow);


#ifndef USE_QRCODE
    ui->btnSaveAs->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    connect(ui->btnSaveAs, &QPushButton::clicked, ui->lblQRCode, &QRImageWidget::saveImage);
}

ReceiveRequestDialog::~ReceiveRequestDialog()
{
    delete ui;
}

void ReceiveRequestDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if (_model)
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &ReceiveRequestDialog::update);

    // update the display unit if necessary
    update();
}

void ReceiveRequestDialog::setInfo(const SendCoinsRecipient &_info)
{
    this->info = _info;
    update();
}

void ReceiveRequestDialog::update()
{
    if(!model)
        return;

    ui->amountBox->hide();
    ui->labelBox->hide();
    ui->messageBox->hide();

    QString target = info.label;
    if(target.isEmpty())
        target = info.address;
    setWindowTitle(tr("Request payment to %1").arg(target));

    QString uri = GUIUtil::formatBitcoinURI(info);
    ui->btnSaveAs->setEnabled(false);
    ui->textUri->setText("<a href=\""+uri+"\">" + GUIUtil::HtmlEscape(uri) + "</a>");
    ui->textUri->setAlignment(Qt::AlignCenter);
    ui->textAddress->setText(GUIUtil::HtmlEscape(info.address));
    ui->textAddress->setAlignment(Qt::AlignCenter);

    if(info.amount) {
        ui->amountBox->show();
        ui->textAmount->setText(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), info.amount));
        ui->textAmount->setAlignment(Qt::AlignCenter);
    }

    if(!info.label.isEmpty()) {
        ui->labelBox->show();
        ui->textLabel->setText(GUIUtil::HtmlEscape(info.label));
        ui->textLabel->setAlignment(Qt::AlignCenter);
    }

    if(!info.message.isEmpty()) {
        ui->messageBox->show();
        ui->textMessage->setText(GUIUtil::HtmlEscape(info.message));
        ui->textMessage->setAlignment(Qt::AlignCenter);
    }

    if (ui->lblQRCode->setQR(uri, info.address)) {
        ui->btnSaveAs->setEnabled(true);
    }
}

void ReceiveRequestDialog::on_btnCopyURI_clicked()
{
    GUIUtil::setClipboard(GUIUtil::formatBitcoinURI(info));
}

void ReceiveRequestDialog::on_btnCopyAddress_clicked()
{
    GUIUtil::setClipboard(info.address);
}
