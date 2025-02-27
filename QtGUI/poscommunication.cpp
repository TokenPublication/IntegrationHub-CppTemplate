#include "poscommunication.h"
#include <QDir>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>

// Initialize static instance
POSCommunication* POSCommunication::m_instance = nullptr;

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

POSCommunication* POSCommunication::getInstance(const QString& companyName)
{
    if (!m_instance) {
        m_instance = new POSCommunication(companyName);
    }
    return m_instance;
}

bool POSCommunication::isConnected() const
{
    return m_isConnected;
}

bool POSCommunication::isConnecting() const
{
    return m_isConnecting;
}

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
