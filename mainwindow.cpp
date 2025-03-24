/**
 * @file mainwindow.cpp
 * @brief Implementation file for the MainWindow class which handles the UI and POS communication
 * @date 2023
 * 
 * This file implements a Qt-based UI application that demonstrates integration with a POS system
 * through the IntegrationHub library. The application provides a simple interface for sending
 * basket data, payment information, and retrieving fiscal information from a connected POS terminal.
 * 
 * Platform: This code is primarily designed for Windows platforms where the IntegrationHub library
 * is supported. While the UI will display on other platforms, the POS communication functionality
 * is disabled outside of Windows environments.
 */

#include "mainwindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QTimer>

/**
 * @brief Constructor for the MainWindow class
 * @param parent The parent widget (default is nullptr for top-level windows)
 * 
 * Initializes the UI components and establishes connections with the POS system.
 * If running on Windows, it automatically attempts to connect to the POS device.
 * On non-Windows platforms, it disables the interactive buttons.
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
 * @brief Destructor for the MainWindow class
 * 
 * Since POSCommunication is implemented as a singleton pattern,
 * no explicit deletion is required here.
 */
MainWindow::~MainWindow()
{
    // POSCommunication is a singleton, no need to delete it here
}

/**
 * @brief Sets up the user interface components
 * 
 * This method creates and arranges all UI elements including:
 * - Window title and size
 * - Button controls for POS operations
 * - Text area for logging
 * It also centers the window on the screen and connects button signals to their handlers.
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
 * @brief Adds a timestamped message to the log display
 * @param message The message text to be logged
 * 
 * Formats the message with a timestamp and appends it to the text log area.
 */
void MainWindow::log(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_textLog->append(QString("[%1] %2").arg(timestamp).arg(message));
}

/**
 * @brief Updates the enabled state of UI buttons based on connection status
 * 
 * Enables or disables the action buttons depending on whether the POS device
 * is connected. Also updates the window title to reflect the current connection status.
 * On non-Windows platforms, all POS-related buttons remain disabled.
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
 * @brief Handler for the "Send Basket" button click
 * 
 * Sends a sample basket in JSON format to the POS device.
 * The sample includes a tax-free transaction with customer information.
 * Logs the result of the operation, including any errors that occur.
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
 * @brief Handler for the "Send Payment" button click
 * 
 * Sends a sample payment request in JSON format to the POS device.
 * The sample includes an amount and payment type.
 * Logs the result of the operation, including any errors that occur.
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
 * @brief Handler for the "Get Fiscal Info" button click
 * 
 * Requests fiscal information from the connected POS device.
 * Logs the received information or any errors that occur during the request.
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
 * @brief Slot handler for log messages from the POS communication module
 * @param message The message to be logged
 * 
 * Receives log messages from the POSCommunication instance and forwards them
 * to the application's logging system.
 */
void MainWindow::onLogMessage(const QString& message)
{
    log(message);
}

/**
 * @brief Slot handler for connection status changes
 * @param isConnected Boolean indicating whether the device is now connected
 * 
 * Updates the UI in response to changes in the connection status of the POS device.
 */
void MainWindow::onConnectionStatusChanged(bool isConnected)
{
    updateButtons();
}

/**
 * @brief Slot handler for serial input received from the POS device
 * @param typeCode The type code of the received data
 * @param value The actual data value received
 * 
 * Logs the serial input received from the POS device, including its type code and value.
 */
void MainWindow::onSerialInReceived(int typeCode, const QString& value)
{
    log(QString("Serial In - Type: %1, Value: %2").arg(typeCode).arg(value));
}

/**
 * @brief Slot handler for device state changes
 * @param isConnected Boolean indicating whether the device is now connected
 * @param deviceId The identifier of the device that changed state
 * 
 * Updates the UI in response to changes in the state of the connected POS device.
 * Also logs the device state change with its ID for diagnostic purposes.
 */
void MainWindow::onDeviceStateChanged(bool isConnected, const QString& deviceId)
{
    log(QString("Device State - Connected: %1, ID: %2")
        .arg(isConnected ? "Yes" : "No").arg(deviceId));
    updateButtons();
}
