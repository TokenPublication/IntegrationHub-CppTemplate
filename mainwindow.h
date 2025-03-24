#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/**
 * @file mainwindow.h
 * @brief Main window class for the Integration Hub application
 * 
 * This file contains the declaration of the MainWindow class which serves as the
 * primary user interface for the Integration Hub application. The application is
 * built using the Qt framework for cross-platform GUI development and is designed
 * to facilitate communication with Point of Sale (POS) systems.
 * 
 * Platform: Qt-based C++ application for Windows/Linux/macOS
 * 
 * @copyright (c) 2023 Payosy Integration Hub
 */

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include "poscommunication.h"

/**
 * @class MainWindow
 * @brief The main window of the Integration Hub application
 * 
 * This class provides the main user interface for the Integration Hub application.
 * It contains controls for sending basket information, processing payments, retrieving
 * fiscal information, and displaying log messages related to POS communication.
 * The window manages the connection status with POS devices and updates the UI accordingly.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for the MainWindow class
     * @param parent The parent widget (default: nullptr)
     * 
     * Initializes the main window, sets up the UI elements, and establishes
     * connections to the POS communication system.
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor for the MainWindow class
     * 
     * Cleans up resources and handles proper shutdown of connections.
     */
    ~MainWindow();

private slots:
    /**
     * @brief Handles the Send Basket button click event
     * 
     * Triggered when the user clicks the Send Basket button. Initiates the
     * process of sending basket information to the connected POS device.
     */
    void onSendBasketClicked();
    
    /**
     * @brief Handles the Send Payment button click event
     * 
     * Triggered when the user clicks the Send Payment button. Initiates the
     * payment process through the connected POS device.
     */
    void onSendPaymentClicked();
    
    /**
     * @brief Handles the Get Fiscal Info button click event
     * 
     * Triggered when the user clicks the Get Fiscal Info button. Requests
     * fiscal information from the connected POS device.
     */
    void onGetFiscalInfoClicked();
    
    /**
     * @brief Handles log messages received from the system
     * @param message The log message to be displayed
     * 
     * Displays log messages in the application's log window.
     */
    void onLogMessage(const QString& message);
    
    /**
     * @brief Handles connection status changes
     * @param isConnected True if connected, false otherwise
     * 
     * Updates the UI based on the connection status with the POS device.
     */
    void onConnectionStatusChanged(bool isConnected);
    
    /**
     * @brief Handles serial data received from the POS device
     * @param typeCode The type code of the received data
     * @param value The value of the received data
     * 
     * Processes serial communication data received from the POS device.
     */
    void onSerialInReceived(int typeCode, const QString& value);
    
    /**
     * @brief Handles device state changes
     * @param isConnected True if the device is connected, false otherwise
     * @param deviceId The identifier of the device that changed state
     * 
     * Updates the UI and internal state based on device connection changes.
     */
    void onDeviceStateChanged(bool isConnected, const QString& deviceId);

private:
    /**
     * @brief Sets up the user interface
     * 
     * Initializes and arranges all UI elements in the main window.
     */
    void setupUi();
    
    /**
     * @brief Logs a message to the application's log window
     * @param message The message to be logged
     * 
     * Formats and displays a message in the log window with a timestamp.
     */
    void log(const QString& message);
    
    /**
     * @brief Updates the enabled/disabled state of buttons
     * 
     * Enables or disables buttons based on the current connection state.
     */
    void updateButtons();

    // UI elements
    /**
     * @brief Button for sending basket information to the POS device
     */
    QPushButton* m_btnSendBasket;
    
    /**
     * @brief Button for sending payment information to the POS device
     */
    QPushButton* m_btnSendPayment;
    
    /**
     * @brief Button for requesting fiscal information from the POS device
     */
    QPushButton* m_btnGetFiscalInfo;
    
    /**
     * @brief Text area for displaying log messages
     */
    QTextEdit* m_textLog;

    // POS Communication
    /**
     * @brief Handler for communication with POS devices
     * 
     * Manages all communication between the application and connected POS devices.
     */
    POSCommunication* m_posComm;
};

#endif // MAINWINDOW_H
