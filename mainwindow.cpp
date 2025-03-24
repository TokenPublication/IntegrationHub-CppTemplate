#include "mainwindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_btnSendBasket(nullptr)
    , m_btnSendPayment(nullptr)
    , m_btnGetFiscalInfo(nullptr)
    , m_textLog(nullptr)
    , m_posComm(nullptr)
{
    setupUi();

    // Initialize POS Communication
    log("Initializing POS Communication...");
    try {
        m_posComm = POSCommunication::getInstance("YourCompanyName");
        
        // Connect signals
        connect(m_posComm, &POSCommunication::logMessage, this, &MainWindow::onLogMessage);
        connect(m_posComm, &POSCommunication::connectionStatusChanged, this, &MainWindow::onConnectionStatusChanged);
        connect(m_posComm, &POSCommunication::serialInReceived, this, &MainWindow::onSerialInReceived);
        connect(m_posComm, &POSCommunication::deviceStateChanged, this, &MainWindow::onDeviceStateChanged);
        
#ifdef Q_OS_WIN
        // Attempt initial connection on Windows
        log("Attempting initial connection...");
        m_posComm->connect();
#else
        // On non-Windows platforms, just disable the buttons
        log("IntegrationHub library is only supported on Windows. Buttons will be disabled.");
        updateButtons();
#endif
    } catch (const std::exception& e) {
        log(QString("Error initializing: %1").arg(e.what()));
        QMessageBox::critical(this, "Initialization Error", 
                             QString("Failed to initialize POS Communication: %1").arg(e.what()));
        QTimer::singleShot(0, qApp, &QApplication::quit);
    }
}

MainWindow::~MainWindow()
{
    // POSCommunication is a singleton, no need to delete it here
}

void MainWindow::setupUi()
{
    // Set window properties
    setWindowTitle("POS Communication Demo");
    resize(640, 480);
    
    // Center the window on screen
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);

    // Create central widget and layout
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // Create button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // Create buttons
    m_btnSendBasket = new QPushButton("Send Basket", this);
    m_btnSendPayment = new QPushButton("Send Payment", this);
    m_btnGetFiscalInfo = new QPushButton("Get Fiscal Info", this);
    
    // Add buttons to layout
    buttonLayout->addWidget(m_btnSendBasket);
    buttonLayout->addWidget(m_btnSendPayment);
    buttonLayout->addWidget(m_btnGetFiscalInfo);
    buttonLayout->addStretch();
    
    // Create log text area
    m_textLog = new QTextEdit(this);
    m_textLog->setReadOnly(true);
    
    // Add layouts to main layout
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_textLog);
    
    // Connect button signals
    connect(m_btnSendBasket, &QPushButton::clicked, this, &MainWindow::onSendBasketClicked);
    connect(m_btnSendPayment, &QPushButton::clicked, this, &MainWindow::onSendPaymentClicked);
    connect(m_btnGetFiscalInfo, &QPushButton::clicked, this, &MainWindow::onGetFiscalInfoClicked);
    
    // Disable buttons initially
    updateButtons();
}

void MainWindow::log(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_textLog->append(QString("[%1] %2").arg(timestamp).arg(message));
}

void MainWindow::updateButtons()
{
#ifdef Q_OS_WIN
    bool enabled = m_posComm && m_posComm->isConnected() && !m_posComm->isConnecting();
    
    m_btnSendBasket->setEnabled(enabled);
    m_btnSendPayment->setEnabled(enabled);
    m_btnGetFiscalInfo->setEnabled(enabled);
    
    if (m_posComm) {
        if (m_posComm->isConnected()) {
            setWindowTitle("POS Communication Demo - Connected");
        } else if (m_posComm->isConnecting()) {
            setWindowTitle("POS Communication Demo - Connecting...");
        } else {
            setWindowTitle("POS Communication Demo - Disconnected");
        }
    }
#else
    // On non-Windows platforms, disable all buttons
    m_btnSendBasket->setEnabled(false);
    m_btnSendPayment->setEnabled(false);
    m_btnGetFiscalInfo->setEnabled(false);
    setWindowTitle("POS Communication Demo - Not Available on this Platform");
#endif
}

void MainWindow::onSendBasketClicked()
{
    const QString sampleBasket = R"({
        "documentType": 9008,
        "taxFreeAmount": 5000,
        "customerInfo": {
            "taxID": "11111111111"
        },
        "paymentItems": [
            {
                "amount": 5000,
                "description": "Nakit",
                "type": 1
            }
        ]
    })";
    
    try {
        int response = m_posComm->sendBasket(sampleBasket);
        log(QString("Basket sent successfully. Response: %1").arg(response));
    } catch (const std::exception& e) {
        log(QString("Error sending basket: %1").arg(e.what()));
    }
}

void MainWindow::onSendPaymentClicked()
{
    const QString samplePayment = R"({
        "amount": 10.99,
        "type": "credit"
    })";
    
    try {
        int response = m_posComm->sendPayment(samplePayment);
        log(QString("Payment sent successfully. Response: %1").arg(response));
    } catch (const std::exception& e) {
        log(QString("Error sending payment: %1").arg(e.what()));
    }
}

void MainWindow::onGetFiscalInfoClicked()
{
    try {
        QString info = m_posComm->getFiscalInfo();
        log("Fiscal Info: " + info);
    } catch (const std::exception& e) {
        log(QString("Error getting fiscal info: %1").arg(e.what()));
    }
}

void MainWindow::onLogMessage(const QString& message)
{
    log(message);
}

void MainWindow::onConnectionStatusChanged(bool isConnected)
{
    updateButtons();
}

void MainWindow::onSerialInReceived(int typeCode, const QString& value)
{
    log(QString("Serial In - Type: %1, Value: %2").arg(typeCode).arg(value));
}

void MainWindow::onDeviceStateChanged(bool isConnected, const QString& deviceId)
{
    log(QString("Device State - Connected: %1, ID: %2")
        .arg(isConnected ? "Yes" : "No").arg(deviceId));
    updateButtons();
}
