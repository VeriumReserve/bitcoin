#include <qt/forumspage.h>
#include <qt/forms/ui_forumspage.h>

#include <qt/clientmodel.h>
#include <qt/walletmodel.h>

#include <qt/guiutil.h>
#include <qt/guiconstants.h>

#include <QDesktopServices>

using namespace GUIUtil;

ForumsPage::ForumsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ForumsPage),
    walletModel(0)
{
    ui->setupUi(this);
    ui->textEdit->setFont(qFontLargerBold);
    // Setup header and styles
    GUIUtil::header(this, QString(""));
    this->layout()->setContentsMargins(0, HEADER_HEIGHT, 0, 0);
}

ForumsPage::~ForumsPage()
{
    delete ui;
}

void ForumsPage::setModel(WalletModel *model)
{
    this->walletModel = model;
}

void ForumsPage::on_explorerButton_clicked()
{
    QDesktopServices::openUrl(QUrl("https://vrmexp.vericoin.info"));
}

void ForumsPage::on_chatButton_clicked()
{
    QDesktopServices::openUrl(QUrl("https://slack.vericoin.info"));
}

void ForumsPage::on_forumButton_clicked()
{
    QDesktopServices::openUrl(QUrl("https://forum.vericoin.info"));
}

void ForumsPage::on_siteButton_clicked()
{
    QDesktopServices::openUrl(QUrl("https://vericoin.info/"));
}
