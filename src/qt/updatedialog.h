#ifndef BITCOIN_QT_UPDATEDIALOG_H
#define BITCOIN_QT_UPDATEDIALOG_H

#include <curl/curl.h>
#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QTimer>

std::string getUpdatedClient();
void processUpdate(QString qClientName);

namespace Ui {
    class UpdateDialog;
}
class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget *parent = 0);
    ~UpdateDialog();
    void setProgress(curl_off_t, curl_off_t);

    Ui::UpdateDialog *ui;
    std::string clientName;

private Q_SLOTS:

    void on_updateButton_clicked();
    void on_closeButton_clicked();

};
#endif // BITCOIN_QT_UPDATEDIALOG_H
