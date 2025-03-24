#ifndef POSCOMMUNICATION_H
#define POSCOMMUNICATION_H

/**
 * @file poscommunication.h
 * @brief POS Communication Interface for Integration Hub
 *
 * This header file defines the POSCommunication class which serves as an interface
 * between a Point of Sale (POS) application and payment terminal devices. It is designed
 * to work with the Payosy Integration Hub on Windows platforms, providing a Qt-based
 * wrapper around native Windows libraries for payment processing and device communication.
 *
 * The class implements a singleton pattern for system-wide access to payment functionality
 * and handles the dynamic loading of required libraries, callback registration, and
 * asynchronous communication with payment devices.
 *
 * Platform: Windows with Qt framework
 */

#include <QString>
#include <QObject>
#include <QLibrary>
#include <QThread>
#include <QDebug>
#include <functional>
#include <memory>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <comdef.h>
#include <comutil.h>
#endif

/**
 * @class POSCommunication
 * @brief Handles communication between POS software and payment terminals
 *
 * This class provides a high-level API for POS applications to interact with
 * payment terminals. It manages the connection state, handles payment and basket
 * transactions, and provides status information about connected devices.
 *
 * It uses a singleton pattern to ensure only one instance manages device communication
 * throughout the application lifecycle.
 */
class POSCommunication : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for POSCommunication
     * @param companyName The merchant/company identifier for the POS system
     * @param parent The parent QObject (for memory management)
     *
     * Initializes a new POSCommunication instance with the specified company name.
     * The company name is used for identification with the payment service provider.
     */
    explicit POSCommunication(const QString& companyName, QObject* parent = nullptr);
    
    /**
     * @brief Destructor
     *
     * Cleans up resources and disconnects from payment devices before destroying the instance.
     */
    ~POSCommunication();

    /**
     * @brief Returns the singleton instance of POSCommunication
     * @param companyName Optional company name parameter (used only during first initialization)
     * @return Pointer to the singleton POSCommunication instance
     *
     * If the instance doesn't exist, it will be created with the provided company name.
     * Subsequent calls will return the existing instance regardless of the company name parameter.
     */
    static POSCommunication* getInstance(const QString& companyName = QString());

    /**
     * @brief Checks if the POS system is currently connected to a payment device
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Checks if the POS system is currently attempting to connect
     * @return true if connecting, false otherwise
     */
    bool isConnecting() const;
    
    /**
     * @brief Initiates a connection to the payment device
     *
     * This method starts an asynchronous connection process. Results are reported
     * through the connectionStatusChanged and deviceStateChanged signals.
     */
    void connect();
    
    /**
     * @brief Terminates the connection to the payment device
     *
     * Releases resources and closes the connection to any active payment device.
     */
    void disconnect();
    
    /**
     * @brief Reinitializes the connection to the payment device
     *
     * Attempts to re-establish connection with the payment device. Useful when
     * connection states become inconsistent or after a device timeout.
     */
    void reconnect();
    
    /**
     * @brief Gets the index of the currently active payment device
     * @return The device index or a negative value if no device is active
     */
    int getActiveDeviceIndex();
    
    /**
     * @brief Sends basket information to the payment device
     * @param jsonData JSON-formatted string containing basket details (items, prices, etc.)
     * @return Result code indicating success (0) or failure (error code)
     *
     * Transmits basket data to the payment device for processing or display.
     */
    int sendBasket(const QString& jsonData);
    
    /**
     * @brief Initiates a payment transaction
     * @param jsonData JSON-formatted string containing payment details (amount, currency, etc.)
     * @return Result code indicating success (0) or failure (error code)
     *
     * Starts a payment transaction with the specified parameters. Results are
     * reported asynchronously through callback signals.
     */
    int sendPayment(const QString& jsonData);
    
    /**
     * @brief Retrieves fiscal information from the connected device
     * @return JSON-formatted string containing fiscal details
     *
     * Gets fiscal information such as transaction history, device status, etc.
     */
    QString getFiscalInfo();

signals:
    /**
     * @brief Signal emitted when data is received from the device
     * @param typeCode Code indicating the type of received data
     * @param value The data value as a string
     */
    void serialInReceived(int typeCode, const QString& value);
    
    /**
     * @brief Signal emitted when device connection state changes
     * @param isConnected Whether the device is now connected
     * @param deviceId Identifier of the affected device
     */
    void deviceStateChanged(bool isConnected, const QString& deviceId);
    
    /**
     * @brief Signal for debug and information logging
     * @param message The log message text
     */
    void logMessage(const QString& message);
    
    /**
     * @brief Signal emitted when the overall connection status changes
     * @param isConnected Whether the system is now connected to any device
     */
    void connectionStatusChanged(bool isConnected);

private:
#ifdef Q_OS_WIN
    /**
     * @brief Windows-specific function pointer types for dynamic library loading
     *
     * These typedefs define the function signatures used to interface with
     * the native Windows libraries that provide the actual device communication.
     */
    typedef void* (__cdecl* CreateCommunication)(const wchar_t*);
    typedef void (__cdecl* DeleteCommunication)(void*);
    typedef void (__cdecl* Reconnect)(void*);
    typedef int (__cdecl* GetActiveDeviceIndex)(void*);
    typedef int (__cdecl* SendBasket)(void*, const wchar_t*);
    typedef int (__cdecl* SendPayment)(void*, const wchar_t*);
    typedef BSTR (__cdecl* GetFiscalInfo)(void*);
    typedef void (__stdcall* SerialInCallback)(int, BSTR);
    typedef void (__stdcall* DeviceStateCallback)(bool, BSTR);
    typedef void (__cdecl* SetSerialInCallback)(void*, SerialInCallback);
    typedef void (__cdecl* SetDeviceStateCallback)(void*, DeviceStateCallback);

    // DLL function pointers
    CreateCommunication m_createCommunication;
    DeleteCommunication m_deleteCommunication;
    Reconnect m_reconnect;
    GetActiveDeviceIndex m_getActiveDeviceIndex;
    SendBasket m_sendBasket;
    SendPayment m_sendPayment;
    GetFiscalInfo m_getFiscalInfo;
    SetSerialInCallback m_setSerialInCallback;
    SetDeviceStateCallback m_setDeviceStateCallback;

    // Static callback handlers
    static void __stdcall serialInCallbackHandler(int typeCode, BSTR value);
    static void __stdcall deviceStateCallbackHandler(bool isConnected, BSTR deviceId);
#endif

    /**
     * @brief Loads required DLL libraries for device communication
     * @return true if all libraries were loaded successfully, false otherwise
     */
    bool loadLibraries();
    
    /**
     * @brief Initializes function pointers from loaded libraries
     *
     * Maps function pointers to exported functions in the loaded DLLs.
     */
    void initializeFunctions();
    
    /**
     * @brief Performs actual connection to the payment device
     *
     * Internal method that implements the connection logic.
     */
    void doConnect();

    // Member variables
    QString m_companyName;           ///< Company identifier for POS system
    QList<QLibrary*> m_libraries;    ///< List of dynamically loaded libraries
    void* m_connection;              ///< Pointer to the native connection object
    bool m_isConnected;              ///< Current connection state
    bool m_isConnecting;             ///< Whether connection is in progress
    
    // Singleton instance
    static POSCommunication* m_instance;  ///< Static pointer to singleton instance
};

#endif // POSCOMMUNICATION_H
