#include <curl/curl.h>

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QTimer>

namespace Ui {
    class Bootstrapdlg;
}
class Bootstrapdlg : public QDialog
{
    Q_OBJECT

public:
    explicit Bootstrapdlg(QWidget *parent = 0);
    ~Bootstrapdlg();
    void setProgress(curl_off_t, curl_off_t);

    Ui::Bootstrapdlg *ui;

private Q_SLOTS:

    void on_startButton_clicked();

};

