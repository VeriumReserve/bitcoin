#include <qt/bootstrapdlg.h>
#include <qt/forms/ui_bootstrapdlg.h>

#include <qt/guiutil.h>
#include <qt/guiconstants.h>

#include <rpc/server.h> /// JSONRequest

#include <QDesktopServices>

using namespace GUIUtil;

Bootstrapdlg::Bootstrapdlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Bootstrapdlg)
{
    ui->setupUi(this);
    // Setup header and styles
    GUIUtil::header(this, QString(""));

    ui->progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
}

Bootstrapdlg::~Bootstrapdlg()
{
    delete ui;
}


Bootstrapdlg* xfer_callback_instance;
static void xfer_callback(curl_off_t total, curl_off_t now)
{
    xfer_callback_instance->setProgress(total, now);
}

void Bootstrapdlg::on_startButton_clicked()
{
    extern UniValue bootstrap(const JSONRPCRequest& request);
    extern void set_xferinfo_data(void*);

    xfer_callback_instance = this;
    set_xferinfo_data((void*)xfer_callback);

    QMessageBox::information(this, "Bootstrap", "The client will now bootstrap the chain. Please be patient.", QMessageBox::Ok, QMessageBox::Ok);
    auto req = JSONRPCRequest();
    bootstrap(req);

    set_xferinfo_data(nullptr);
    xfer_callback_instance = nullptr;
}

void Bootstrapdlg::setProgress(curl_off_t total, curl_off_t now)
{
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(total - 1);
    ui->progressBar->setValue(now);
}

