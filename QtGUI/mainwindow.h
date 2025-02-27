#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include "poscommunication.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSendBasketClicked();
    void onSendPaymentClicked();
    void onGetFiscalInfoClicked();
    void onLogMessage(const QString& message);
    void onConnectionStatusChanged(bool isConnected);
    void onSerialInReceived(int typeCode, const QString& value);
    void onDeviceStateChanged(bool isConnected, const QString& deviceId);

private:
    void setupUi();
    void log(const QString& message);
    void updateButtons();

    // UI elements
    QPushButton* m_btnSendBasket;
    QPushButton* m_btnSendPayment;
    QPushButton* m_btnGetFiscalInfo;
    QTextEdit* m_textLog;

    // POS Communication
    POSCommunication* m_posComm;
};

#endif // MAINWINDOW_H
