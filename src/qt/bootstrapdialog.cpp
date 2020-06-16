#include <qt/bootstrapdialog.h>
#include <qt/forms/ui_bootstrapdialog.h>
#include <qt/guiutil.h>
#include <qt/guiconstants.h>
#include <downloader.h>
#include <QDesktopServices>

BootstrapDialog::BootstrapDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BootstrapDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowTitle(tr("Chain Bootstrap"));
    ui->checkBox->setVisible(false);
}

BootstrapDialog::~BootstrapDialog()
{
    delete ui;
}

BootstrapDialog* bootstrap_callback_instance;
static void xfer_callback(curl_off_t total, curl_off_t now)
{
    bootstrap_callback_instance->setProgress(total, now);
}

void BootstrapDialog::on_startButton_clicked()
{
    extern void set_xferinfo_data(void*);

    bootstrap_callback_instance = this;
    set_xferinfo_data((void*)xfer_callback);

    QMessageBox::information(this, "Bootstrap", "The client will now bootstrap the chain. \n\nThe Verium vault will exit after extracting the bootstrap and need to be restarted.", QMessageBox::Ok, QMessageBox::Ok);
    try {
        downloadBootstrap();
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(this, tr("Bootstrap failed"), e.what());
    }
    set_xferinfo_data(nullptr);
    bootstrap_callback_instance = nullptr;
    this->close();
    QApplication::quit();
}

void BootstrapDialog::on_closeButton_clicked()
{
    this->close();
}

void BootstrapDialog::setProgress(curl_off_t total, curl_off_t now)
{
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(total - 1);
    ui->progressBar->setValue(now);
}
