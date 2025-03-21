/**
 * @file poscommunication.h
 * @brief POS Communication Interface for Integration Hub
 * 
 * This header file defines the POSCommunication class, which serves as an interface
 * between a Qt application and Integration Hub (TokenX Connect (Wired)). It is implemented in C++
 * using the Qt framework.
 * 
 * The class provides a singleton pattern interface to handle communication with
 * POS devices, manage connections, and process transactions. It dynamically loads
 * the required libraries and exposes methods for sending basket data, processing
 * payments, and retrieving fiscal information.
 * 
 * Platform: Windows (with conditional compilation)
 * Language: C++ with Qt framework
 * Design Pattern: Singleton
 * 
 * Features:
 * - Dynamic library loading
 * - Connection management (connect, disconnect, reconnect)
 * - Transaction processing (baskets, payments)
 * - Device state monitoring
 * - Callback handling for device events
 * 
 * @note This class requires Windows-specific COM libraries when compiled for Windows
 */

#ifndef POSCOMMUNICATION_H
#define POSCOMMUNICATION_H

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

class POSCommunication : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for POSCommunication
     * @param companyName The name of the company using the POS system
     * @param parent The parent QObject (default: nullptr)
     * 
     * Initializes a new instance of the POSCommunication class with the specified
     * company name. The company name is used to identify the integration.
     */
    explicit POSCommunication(const QString& companyName, QObject* parent = nullptr);
    
    /**
     * @brief Destructor for POSCommunication
     * 
     * Cleans up resources and releases the connection to the POS system.
     */
    ~POSCommunication();

    /**
     * @brief Get the singleton instance of POSCommunication
     * @param companyName The name of the company (optional if already initialized)
     * @return Pointer to the singleton instance
     * 
     * This method implements the singleton pattern. If the instance doesn't exist,
     * it creates one with the provided company name.
     */
    static POSCommunication* getInstance(const QString& companyName = QString());

    /**
     * @brief Check if the POS system is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Check if the POS system is in the process of connecting
     * @return true if connecting, false otherwise
     */
    bool isConnecting() const;
    
    /**
     * @brief Initiate connection to the POS system
     * 
     * Begins the connection process to the POS device.
     */
    void connect();
    
    /**
     * @brief Disconnect from the POS system
     * 
     * Terminates the connection with the POS device.
     */
    void disconnect();
    
    /**
     * @brief Reconnect to the POS system
     * 
     * Attempts to reestablish connection with the POS device.
     */
    void reconnect();
    
    /**
     * @brief Get the index of the currently active device
     * @return The index of the active device or an error code
     */
    int getActiveDeviceIndex();
    
    /**
     * @brief Send basket data to the POS system
     * @param jsonData JSON-formatted basket data
     * @return Result code indicating success or failure
     * 
     * Sends product/basket information to the POS device in JSON format.
     */
    int sendBasket(const QString& jsonData);
    
    /**
     * @brief Send payment information to the POS system
     * @param jsonData JSON-formatted payment data
     * @return Result code indicating success or failure
     * 
     * Initiates a payment transaction with the provided payment details.
     */
    int sendPayment(const QString& jsonData);
    
    /**
     * @brief Get fiscal information from the POS system
     * @return JSON-formatted fiscal information
     * 
     * Retrieves fiscal data from the connected POS device.
     */
    QString getFiscalInfo();

signals:
    /**
     * @brief Signal emitted when serial data is received
     * @param typeCode Type of the received data
     * @param value The data content
     */
    void serialInReceived(int typeCode, const QString& value);
    
    /**
     * @brief Signal emitted when device state changes
     * @param isConnected Connection state
     * @param deviceId Identifier of the device
     */
    void deviceStateChanged(bool isConnected, const QString& deviceId);
    
    /**
     * @brief Signal for logging messages
     * @param message The log message
     */
    void logMessage(const QString& message);
    
    /**
     * @brief Signal emitted when connection status changes
     * @param isConnected New connection state
     */
    void connectionStatusChanged(bool isConnected);

private:
#ifdef Q_OS_WIN
    /**
     * @brief Windows-specific function pointer types
     * 
     * These typedefs define the function signatures for the dynamically loaded
     * library functions that interface with the POS system on Windows.
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

    // Private methods
    bool loadLibraries();
    void initializeFunctions();
    void doConnect();

    // Member variables
    QString m_companyName;
    QList<QLibrary*> m_libraries;
    void* m_connection;
    bool m_isConnected;
    bool m_isConnecting;
    
    // Singleton instance
    static POSCommunication* m_instance;
};

#endif // POSCOMMUNICATION_H
