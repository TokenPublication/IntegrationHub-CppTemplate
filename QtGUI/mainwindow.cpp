/**
 * @file mainwindow.cpp
 * @brief Implementation of the MainWindow class for the IntegrationHub POS Communication Demo
 * 
 * This file implements a Qt-based GUI application in C++ that demonstrates communication
 * with Integration Hub.

 * Dependencies: Qt Framework, POSCommunication library
 */

#include "mainwindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QTimer>

/**
 * @brief Constructs the main window and initializes the POS communication
 * 
 * Sets up the UI components and attempts to establish a connection with the
 * POS system if running on Windows. On other platforms, buttons are disabled
 * as the IntegrationHub library is Windows-specific.
 * 
 * @param parent The parent widget
 */
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

/**
 * @brief Destructor for MainWindow
 * 
 * POSCommunication is implemented as a singleton, so it doesn't need
 * to be explicitly deleted here.
 */
MainWindow::~MainWindow()
{
    // POSCommunication is a singleton, no need to delete it here
}

/**
 * @brief Sets up the user interface components
 * 
 * Creates and arranges the UI elements including buttons and text area,
 * connects button signals to appropriate slots, and centers the window on screen.
 */
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

/**
 * @brief Logs a message to the text area with a timestamp
 * 
 * @param message The message to log
 */
void MainWindow::log(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_textLog->append(QString("[%1] %2").arg(timestamp).arg(message));
}

/**
 * @brief Updates the state of the buttons based on connection status
 * 
 * Enables or disables the buttons depending on whether the POS communication
 * is connected, connecting, or disconnected. On non-Windows platforms, all
 * buttons are disabled.
 */
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

/**
 * @brief Slot for handling Send Basket button click
 * 
 * Sends a sample basket data to the POS system and logs the response.
 */
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

/**
 * @brief Slot for handling Send Payment button click
 * 
 * Sends a sample payment data to the POS system and logs the response.
 */
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

/**
 * @brief Slot for handling Get Fiscal Info button click
 * 
 * Retrieves fiscal information from the POS system and logs it.
 */
void MainWindow::onGetFiscalInfoClicked()
{
    try {
        QString info = m_posComm->getFiscalInfo();
        log("Fiscal Info: " + info);
    } catch (const std::exception& e) {
        log(QString("Error getting fiscal info: %1").arg(e.what()));
    }
}

/**
 * @brief Slot for handling log messages from POS communication
 * 
 * Logs the received message.
 * 
 * @param message The log message
 */
void MainWindow::onLogMessage(const QString& message)
{
    log(message);
}

/**
 * @brief Slot for handling connection status changes
 * 
 * Updates the state of the buttons based on the new connection status.
 * 
 * @param isConnected True if connected, false otherwise
 */
void MainWindow::onConnectionStatusChanged(bool isConnected)
{
    updateButtons();
}

/**
 * @brief Slot for handling serial input received from POS communication
 * 
 * Logs the received serial input.
 * 
 * @param typeCode The type code of the serial input
 * @param value The value of the serial input
 */
void MainWindow::onSerialInReceived(int typeCode, const QString& value)
{
    log(QString("Serial In - Type: %1, Value: %2").arg(typeCode).arg(value));
}

/**
 * @brief Slot for handling device state changes
 * 
 * Logs the new device state and updates the state of the buttons.
 * 
 * @param isConnected True if the device is connected, false otherwise
 * @param deviceId The ID of the device
 */
void MainWindow::onDeviceStateChanged(bool isConnected, const QString& deviceId)
{
    log(QString("Device State - Connected: %1, ID: %2")
        .arg(isConnected ? "Yes" : "No").arg(deviceId));
    updateButtons();
}
