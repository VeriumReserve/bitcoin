#ifndef BITCOIN_QT_BOOTSTRAPDIALOG_H
#define BITCOIN_QT_BOOTSTRAPDIALOG_H

#include <curl/curl.h>
#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QTimer>

namespace Ui {
    class BootstrapDialog;
}
class BootstrapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BootstrapDialog(QWidget *parent = 0);
    ~BootstrapDialog();
    void setProgress(curl_off_t, curl_off_t);

    Ui::BootstrapDialog *ui;

private Q_SLOTS:

    void on_startButton_clicked();
    void on_closeButton_clicked();

};
#endif // BITCOIN_QT_BOOTSTRAPDIALOG_H
