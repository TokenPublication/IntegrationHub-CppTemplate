/**
 * @file poscommunication.cpp
 * @brief Implementation of the POSCommunication class for integrating with payment terminals.
 *
 * This file contains the implementation of the POSCommunication class, which provides
 * a Qt-based interface for communicating with POS (Point of Sale) payment terminals
 * through the IntegrationHub library. The class is designed primarily for Windows
 * platforms where the necessary DLLs are available, but includes platform checks
 * to provide graceful degradation on other operating systems.
 *
 * The implementation handles:
 * - Dynamic loading of required libraries
 * - Connection management with payment terminals
 * - Transaction processing (sending baskets and payments)
 * - Callback handling for device state changes and serial communications
 * - Thread-safe asynchronous operations
 *
 * Platform: Windows (primary), with limited functionality on other platforms
 */

#include "poscommunication.h"
#include <QDir>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>

// Initialize static instance
POSCommunication* POSCommunication::m_instance = nullptr;

/**
 * @brief Constructor for the POSCommunication class.
 *
 * Initializes a new instance of the POSCommunication class with the specified
 * company name. Attempts to load required libraries and initialize function
 * pointers to the IntegrationHub DLL functions.
 *
 * @param companyName The name of the company using the integration, used for identification
 * @param parent The parent QObject for memory management (can be nullptr)
 */
POSCommunication::POSCommunication(const QString& companyName, QObject* parent)
    : QObject(parent)
    , m_companyName(companyName)
    , m_connection(nullptr)
    , m_isConnected(false)
    , m_isConnecting(false)
{
    // Load required libraries
    if (!loadLibraries()) {
        emit logMessage("Failed to load required libraries");
        return;
    }

    // Initialize function pointers
    initializeFunctions();
}

/**
 * @brief Destructor for the POSCommunication class.
 *
 * Cleans up resources by disconnecting from any active connections,
 * unloading and freeing libraries, and clearing the singleton instance
 * if this object is the current singleton.
 */
POSCommunication::~POSCommunication()
{
    // Clean up connection
    disconnect();

    // Free libraries
    for (QLibrary* lib : m_libraries) {
        lib->unload();
        delete lib;
    }
    m_libraries.clear();

    // Clear singleton instance if this is it
    if (m_instance == this) {
        m_instance = nullptr;
    }
}

/**
 * @brief Gets or creates the singleton instance of POSCommunication.
 *
 * This method implements the singleton pattern, ensuring only one instance
 * of POSCommunication exists. If no instance exists, it creates one with
 * the specified company name.
 *
 * @param companyName The name of the company using the integration
 * @return A pointer to the singleton POSCommunication instance
 */
POSCommunication* POSCommunication::getInstance(const QString& companyName)
{
    if (!m_instance) {
        m_instance = new POSCommunication(companyName);
    }
    return m_instance;
}

/**
 * @brief Checks if a connection to a payment terminal is established.
 *
 * @return true if connected to a payment terminal, false otherwise
 */
bool POSCommunication::isConnected() const
{
    return m_isConnected;
}

/**
 * @brief Checks if a connection attempt is currently in progress.
 *
 * @return true if a connection attempt is in progress, false otherwise
 */
bool POSCommunication::isConnecting() const
{
    return m_isConnecting;
}

/**
 * @brief Initiates a connection to the payment terminal.
 *
 * This method starts a new thread to handle the connection process asynchronously.
 * It sets the connection status flags and emits relevant signals to update the UI.
 */
void POSCommunication::connect()
{
    if (m_connection != nullptr) {
        emit logMessage("Already connected");
        return;
    }

    if (m_isConnecting) {
        emit logMessage("Connection attempt already in progress...");
        return;
    }

    m_isConnecting = true;
    emit logMessage("Connecting...");
    emit connectionStatusChanged(false);

    // Create a new thread for connection
    QThread* thread = QThread::create([this]() {
        try {
            doConnect();
            QMetaObject::invokeMethod(this, [this]() {
                emit logMessage("Connected successfully");
                m_isConnecting = false;
                emit connectionStatusChanged(true);
            });
        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(this, [this, error = QString(e.what())]() {
                emit logMessage("Error connecting: " + error);
                m_isConnecting = false;
                emit connectionStatusChanged(false);
            });
        }
    });

    thread->start(QThread::LowPriority);
    // Thread will self-delete when finished
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

/**
 * @brief Performs the actual connection logic to the payment terminal.
 *
 * This method is platform-specific and only implemented for Windows. It creates
 * the communication instance, sets up callbacks, and updates the connection status.
 *
 * @throws std::runtime_error if the connection fails or the platform is unsupported
 */
void POSCommunication::doConnect()
{
#ifdef Q_OS_WIN
    emit logMessage("Creating communication instance...");
    
    // Create communication instance
    m_connection = m_createCommunication(reinterpret_cast<const wchar_t*>(m_companyName.utf16()));
    if (m_connection == nullptr) {
        throw std::runtime_error("Failed to create communication instance");
    }

    emit logMessage("Setting up callbacks...");
    
    // Set callbacks
    m_setSerialInCallback(m_connection, serialInCallbackHandler);
    m_setDeviceStateCallback(m_connection, deviceStateCallbackHandler);
    
    emit logMessage("Connection setup complete");
#else
    emit logMessage("Windows-specific functionality not available on this platform");
    throw std::runtime_error("Windows-specific functionality not available");
#endif
}

/**
 * @brief Disconnects from the payment terminal.
 *
 * This method is platform-specific and only implemented for Windows. It deletes
 * the communication instance, updates the connection status, and emits relevant signals.
 */
void POSCommunication::disconnect()
{
#ifdef Q_OS_WIN
    if (m_connection != nullptr) {
        m_deleteCommunication(m_connection);
        m_connection = nullptr;
        m_isConnected = false;
        emit connectionStatusChanged(false);
        emit logMessage("Disconnected");
    }
#endif
}

/**
 * @brief Reconnects to the payment terminal.
 *
 * This method is platform-specific and only implemented for Windows. It either
 * initiates a new connection or calls the reconnect function on the existing connection.
 */
void POSCommunication::reconnect()
{
#ifdef Q_OS_WIN
    if (m_connection == nullptr) {
        connect();
    } else {
        m_reconnect(m_connection);
        emit logMessage("Reconnection initiated");
    }
#endif
}

/**
 * @brief Gets the index of the currently active device.
 *
 * This method is platform-specific and only implemented for Windows. It retrieves
 * the active device index from the communication instance.
 *
 * @return The index of the active device
 * @throws std::runtime_error if not connected or the platform is unsupported
 */
int POSCommunication::getActiveDeviceIndex()
{
#ifdef Q_OS_WIN
    if (m_connection == nullptr) {
        throw std::runtime_error("Not connected");
    }
    return m_getActiveDeviceIndex(m_connection);
#else
    throw std::runtime_error("Function not available on this platform");
#endif
}

/**
 * @brief Sends a basket of items to the payment terminal.
 *
 * This method is platform-specific and only implemented for Windows. It sends
 * the basket data in JSON format to the communication instance.
 *
 * @param jsonData The basket data in JSON format
 * @return The result code from the send operation
 * @throws std::runtime_error if not connected or the platform is unsupported
 */
int POSCommunication::sendBasket(const QString& jsonData)
{
#ifdef Q_OS_WIN
    if (m_connection == nullptr) {
        throw std::runtime_error("Not connected");
    }
    return m_sendBasket(m_connection, reinterpret_cast<const wchar_t*>(jsonData.utf16()));
#else
    throw std::runtime_error("Function not available on this platform");
#endif
}

/**
 * @brief Sends a payment request to the payment terminal.
 *
 * This method is platform-specific and only implemented for Windows. It sends
 * the payment data in JSON format to the communication instance.
 *
 * @param jsonData The payment data in JSON format
 * @return The result code from the send operation
 * @throws std::runtime_error if not connected or the platform is unsupported
 */
int POSCommunication::sendPayment(const QString& jsonData)
{
#ifdef Q_OS_WIN
    if (m_connection == nullptr) {
        throw std::runtime_error("Not connected");
    }
    return m_sendPayment(m_connection, reinterpret_cast<const wchar_t*>(jsonData.utf16()));
#else
    throw std::runtime_error("Function not available on this platform");
#endif
}

/**
 * @brief Retrieves fiscal information from the payment terminal.
 *
 * This method is platform-specific and only implemented for Windows. It gets
 * the fiscal information as a string from the communication instance.
 *
 * @return The fiscal information as a QString
 * @throws std::runtime_error if not connected or the platform is unsupported
 */
QString POSCommunication::getFiscalInfo()
{
#ifdef Q_OS_WIN
    if (m_connection == nullptr) {
        throw std::runtime_error("Not connected");
    }
    BSTR result = m_getFiscalInfo(m_connection);
    QString info = QString::fromWCharArray(result);
    SysFreeString(result);
    return info;
#else
    throw std::runtime_error("Function not available on this platform");
#endif
}

/**
 * @brief Loads the required libraries for communication.
 *
 * This method is platform-specific and only implemented for Windows. It attempts
 * to load the necessary DLLs from predefined search paths and logs the results.
 *
 * @return true if all required libraries are loaded successfully, false otherwise
 */
bool POSCommunication::loadLibraries()
{
#ifdef Q_OS_WIN
    const QStringList requiredDlls = {
        "libcrypto-3.dll",
        "libusb-1.0.dll",
        "zlib1.dll",
        "IntegrationHubCpp.dll"
    };

    // Search paths
    QStringList searchPaths = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath()
    };

    int loadedCount = 0;
    for (const QString& dllName : requiredDlls) {
        bool loaded = false;
        
        for (const QString& path : searchPaths) {
            QString dllPath = path + "/" + dllName;
            if (QFile::exists(dllPath)) {
                emit logMessage("Loading " + dllName + " from " + path + "...");
                
                QLibrary* lib = new QLibrary(dllPath);
                if (lib->load()) {
                    m_libraries.append(lib);
                    loadedCount++;
                    emit logMessage("Successfully loaded " + dllName);
                    loaded = true;
                    break;
                } else {
                    emit logMessage("Failed to load " + dllName + ": " + lib->errorString());
                    delete lib;
                }
            }
        }

        if (!loaded) {
            emit logMessage("Failed to load required DLL: " + dllName);
            return false;
        }
    }

    emit logMessage(QString("Successfully loaded %1 DLLs").arg(loadedCount));
    return true;
#else
    emit logMessage("IntegrationHub library is only supported on Windows. Functionality will be limited.");
    return false;
#endif
}

/**
 * @brief Initializes function pointers to the IntegrationHub DLL functions.
 *
 * This method is platform-specific and only implemented for Windows. It resolves
 * the function pointers from the loaded IntegrationHub DLL and logs the results.
 */
void POSCommunication::initializeFunctions()
{
#ifdef Q_OS_WIN
    // Find the main DLL in our loaded libraries
    QLibrary* mainDll = nullptr;
    for (QLibrary* lib : m_libraries) {
        if (lib->fileName().contains("IntegrationHubCpp.dll", Qt::CaseInsensitive)) {
            mainDll = lib;
            break;
        }
    }

    if (!mainDll) {
        emit logMessage("Failed to find main DLL in loaded libraries");
        return;
    }

    // Get function pointers
    m_createCommunication = reinterpret_cast<CreateCommunication>(mainDll->resolve("createCommunication"));
    m_deleteCommunication = reinterpret_cast<DeleteCommunication>(mainDll->resolve("deleteCommunication"));
    m_reconnect = reinterpret_cast<Reconnect>(mainDll->resolve("reconnect"));
    m_getActiveDeviceIndex = reinterpret_cast<GetActiveDeviceIndex>(mainDll->resolve("getActiveDeviceIndex"));
    m_sendBasket = reinterpret_cast<SendBasket>(mainDll->resolve("sendBasket"));
    m_sendPayment = reinterpret_cast<SendPayment>(mainDll->resolve("sendPayment"));
    m_getFiscalInfo = reinterpret_cast<GetFiscalInfo>(mainDll->resolve("getFiscalInfo"));
    m_setSerialInCallback = reinterpret_cast<SetSerialInCallback>(mainDll->resolve("setSerialInCallback"));
    m_setDeviceStateCallback = reinterpret_cast<SetDeviceStateCallback>(mainDll->resolve("setDeviceStateCallback"));

    // Check that all functions were resolved
    if (!m_createCommunication || !m_deleteCommunication || !m_reconnect || 
        !m_getActiveDeviceIndex || !m_sendBasket || !m_sendPayment || 
        !m_getFiscalInfo || !m_setSerialInCallback || !m_setDeviceStateCallback) {
        emit logMessage("Failed to resolve one or more functions from DLL");
    } else {
        emit logMessage("Successfully initialized all DLL functions");
    }
#endif
}

#ifdef Q_OS_WIN
/**
 * @brief Callback handler for serial input events.
 *
 * This static method is called by the IntegrationHub library when a serial input
 * event occurs. It forwards the event to the singleton instance of POSCommunication.
 *
 * @param typeCode The type code of the serial input event
 * @param value The value of the serial input event as a BSTR
 */
void __stdcall POSCommunication::serialInCallbackHandler(int typeCode, BSTR value)
{
    if (m_instance) {
        QString valueStr = QString::fromWCharArray(value);
        QMetaObject::invokeMethod(m_instance, "serialInReceived", Qt::QueuedConnection,
                                 Q_ARG(int, typeCode), Q_ARG(QString, valueStr));
        QMetaObject::invokeMethod(m_instance, "logMessage", Qt::QueuedConnection,
                                 Q_ARG(QString, QString("Serial In - Type: %1, Value: %2").arg(typeCode).arg(valueStr)));
    }
}

/**
 * @brief Callback handler for device state changes.
 *
 * This static method is called by the IntegrationHub library when the device state
 * changes (e.g., connected or disconnected). It forwards the event to the singleton
 * instance of POSCommunication and handles auto-reconnection if necessary.
 *
 * @param isConnected true if the device is connected, false otherwise
 * @param deviceId The ID of the device as a BSTR
 */
void __stdcall POSCommunication::deviceStateCallbackHandler(bool isConnected, BSTR deviceId)
{
    if (m_instance) {
        QString deviceIdStr = QString::fromWCharArray(deviceId);
        m_instance->m_isConnected = isConnected;
        
        QMetaObject::invokeMethod(m_instance, "deviceStateChanged", Qt::QueuedConnection,
                                 Q_ARG(bool, isConnected), Q_ARG(QString, deviceIdStr));
        QMetaObject::invokeMethod(m_instance, "connectionStatusChanged", Qt::QueuedConnection,
                                 Q_ARG(bool, isConnected));
        QMetaObject::invokeMethod(m_instance, "logMessage", Qt::QueuedConnection,
                                 Q_ARG(QString, QString("Device State - Connected: %1, ID: %2")
                                      .arg(isConnected ? "Yes" : "No").arg(deviceIdStr)));
        
        // Auto-reconnect if disconnected
        if (!isConnected && !m_instance->m_isConnecting) {
            QMetaObject::invokeMethod(m_instance, "logMessage", Qt::QueuedConnection,
                                     Q_ARG(QString, "Connection lost. Attempting to reconnect..."));
            QMetaObject::invokeMethod(m_instance, "connect", Qt::QueuedConnection);
        }
    }
}
#endif
