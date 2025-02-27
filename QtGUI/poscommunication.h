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
    explicit POSCommunication(const QString& companyName, QObject* parent = nullptr);
    ~POSCommunication();

    // Singleton instance
    static POSCommunication* getInstance(const QString& companyName = QString());

    // API functions
    bool isConnected() const;
    bool isConnecting() const;
    void connect();
    void disconnect();
    void reconnect();
    int getActiveDeviceIndex();
    int sendBasket(const QString& jsonData);
    int sendPayment(const QString& jsonData);
    QString getFiscalInfo();

signals:
    void serialInReceived(int typeCode, const QString& value);
    void deviceStateChanged(bool isConnected, const QString& deviceId);
    void logMessage(const QString& message);
    void connectionStatusChanged(bool isConnected);

private:
#ifdef Q_OS_WIN
    // Function pointer types
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
