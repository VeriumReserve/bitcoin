#ifndef BITCOIN_QT_BOOTSTRAPDIALOG_H
#define BITCOIN_QT_BOOTSTRAPDIALOG_H

#include <curl/curl.h>
#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QTimer>

namespace Ui {
    class Bootstrapdialog;
}
class Bootstrapdialog : public QDialog
{
    Q_OBJECT

public:
    explicit Bootstrapdialog(QWidget *parent = 0);
    ~Bootstrapdialog();
    void setProgress(curl_off_t, curl_off_t);

    Ui::Bootstrapdialog *ui;

private Q_SLOTS:

    void on_startButton_clicked();

};
#endif // BITCOIN_QT_BOOTSTRAPDIALOG_H
