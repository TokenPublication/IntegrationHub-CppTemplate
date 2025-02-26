#include <iostream>
#include <string>
#include <functional>
#include <Windows.h>
#include <thread>
#include <comdef.h>
#include <comutil.h>


#define EXPORT __declspec(dllexport)

class POSCommunication {
private:
    // Fonksiyon işaretçileri için farklı isimler kullanalım
    typedef void* (__cdecl* CreateCommunication)(const std::wstring&);
    typedef void (__cdecl* DeleteCommunication)(void*);
    typedef void (__cdecl* Reconnect)(void*);
    typedef int (__cdecl* GetActiveDeviceIndex)(void*);
    typedef int (__cdecl* SendBasket)(void*, const std::wstring&);
    typedef int (__cdecl* SendPayment)(void*, const std::wstring&);
    typedef BSTR (__cdecl* GetFiscalInfo)(void*);
    // Define the function pointer types to match your DLL
    typedef void(__stdcall* SerialInCallback)(int, BSTR);
    typedef void(__stdcall* DeviceStateCallback)(bool, BSTR);
    typedef void (__cdecl* SetSerialInCallback)(void*, SerialInCallback);
    typedef void (__cdecl* SetDeviceStateCallback)(void*, DeviceStateCallback);

    void* _Communication;
    SerialInCallback _serialInCallback;
    DeviceStateCallback _deviceStateCallback;

    // DLL fonksiyon işaretçilerini burada tanımlıyoruz
    CreateCommunication createCommunication;
    DeleteCommunication deleteCommunication;
    Reconnect reconnect;
    GetActiveDeviceIndex getActiveDeviceIndex; 
    SendBasket sendBasket;
    SendPayment sendPayment;
    GetFiscalInfo getFiscalInfo;
    SetSerialInCallback setSerialInCallback;
    SetDeviceStateCallback setDeviceStateCallback;

    HMODULE libraryHandle;

    void LoadNativeLibrary(const std::string& libName) {
        libraryHandle = LoadLibraryA(libName.c_str());
        if (!libraryHandle) {
            throw std::runtime_error("Unable to load DLL");
        }

        createCommunication = (CreateCommunication)GetProcAddress(libraryHandle, "createCommunication");
        deleteCommunication = (DeleteCommunication)GetProcAddress(libraryHandle, "deleteCommunication");
        reconnect = (Reconnect)GetProcAddress(libraryHandle, "reconnect");
        getActiveDeviceIndex = (GetActiveDeviceIndex)GetProcAddress(libraryHandle, "getActiveDeviceIndex");
        sendBasket = (SendBasket)GetProcAddress(libraryHandle, "sendBasket");
        sendPayment = (SendPayment)GetProcAddress(libraryHandle, "sendPayment");
        getFiscalInfo = (GetFiscalInfo)GetProcAddress(libraryHandle, "getFiscalInfo");
        setSerialInCallback = (SetSerialInCallback)GetProcAddress(libraryHandle, "setSerialInCallback");
        setDeviceStateCallback = (SetDeviceStateCallback)GetProcAddress(libraryHandle, "setDeviceStateCallback");

        if (!createCommunication || !deleteCommunication || !reconnect || !getActiveDeviceIndex || !sendBasket || !sendPayment || !getFiscalInfo || !setSerialInCallback || !setDeviceStateCallback) {
            throw std::runtime_error("Failed to load required functions from DLL");
        }
    }

public:
    POSCommunication(const std::wstring& companyName) {
        LoadNativeLibrary("IntegrationHubCpp.dll");
        _Communication = createCommunication(companyName);
    }

    ~POSCommunication() {
        if (_Communication) {
            deleteCommunication(_Communication);
        }
        if (libraryHandle) {
            FreeLibrary(libraryHandle);
        }
    }

    int GetActiveDeviceIndexFunc() {
        return getActiveDeviceIndex(_Communication);  // Hata olan kısmı burasıydı, fonksiyon adı değiştirildi
    }

    int SendBasketFunc(const std::wstring& jsonData) {
        return sendBasket(_Communication, jsonData);
    }

    int SendPaymentFunc(const std::wstring& jsonData) {
        return sendPayment(_Communication, jsonData);
    }

    BSTR GetFiscalInfoFunc() {
        return getFiscalInfo(_Communication);
    }

    void SetSerialInCallbackFunc(SerialInCallback callback) {
        _serialInCallback = callback;
        setSerialInCallback(_Communication, _serialInCallback);
    }

    void SetDeviceStateCallbackFunc(DeviceStateCallback callback) {
        _deviceStateCallback = callback;
//        setDeviceStateCallback(_Communication, _deviceStateCallback);
        
        // Run the callback setting in a separate thread
        std::thread callbackThread([this]() {
            try {
                setDeviceStateCallback(_Communication, _deviceStateCallback);
                std::wcout << L"Device State Callback has been set successfully in the background thread." << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Error setting Device State Callback: " << e.what() << std::endl;
            }
            });
        callbackThread.detach();  // Detach the thread so it runs independently
        
    }

    static POSCommunication* instance;

    static POSCommunication* GetInstance(const std::wstring& companyName) {
        if (!instance) {
            instance = new POSCommunication(companyName);
        }
        return instance;
    }

    void ReconnectFunc() {
        reconnect(_Communication);
    }

    void DeleteCommunicationFunc() {
        deleteCommunication(_Communication);
    }
};

// Initialize the static instance pointer
POSCommunication* POSCommunication::instance = nullptr;

void __stdcall serialInHandler(int value, BSTR data) {
    std::wcout << L"+++++++++++++++++++++++++++++++" << std::endl;
//    _bstr_t bstr(data);
    std::wcout << L"SerialInCallback called with value: " << value << L", data: " << data << std::endl;
    std::wcout << L"+++++++++++++++++++++++++++++++" << std::endl;
}

void __stdcall deviceStateHandler(bool state, BSTR message) {
    std::wcout << L"a+++++++++++++++++++++++++++++++" << std::endl;
//    _bstr_t bstr(message);
    std::wcout << L"DeviceStateCallback called with state: " << state << L", message: " << message << std::endl;
    std::wcout << L"b+++++++++++++++++++++++++++++++" << std::endl;
    if (state) {
       

    }
}

int main() {
    try {

        LoadLibrary((WCHAR*)"libusb-1.0.dll");
        LoadLibrary((WCHAR*)"libcrypto-3.dll");
        LoadLibrary((WCHAR*)"zlib1.dll");

        // Example usage of the POSCommunication class
        POSCommunication* posComm = POSCommunication::GetInstance(L"CompanyName");

        
        // Set callbacks
        posComm->SetSerialInCallbackFunc(serialInHandler);
        posComm->SetDeviceStateCallbackFunc(deviceStateHandler);


        /*
        posComm->SetSerialInCallbackFunc([](int type, BSTR value) {
            std::wcout << L"Serial In Callback: " << value << std::endl;
        });

        posComm->SetDeviceStateCallbackFunc([](bool isDeviceConnected,  BSTR id) {
            std::wcout << L"Device State Callback: " << (isDeviceConnected ? L"Connected" : L"Disconnected") << L", ID: " << id << std::endl;

            });
          */  
    
        std::cin.get();

        std::wcout << L"c+++++++++++++++++++++++++++++++" << std::endl;
        std::wcout << POSCommunication::instance << std::endl;
        std::wcout << posComm << std::endl;
        BSTR fiscal = posComm->GetFiscalInfoFunc();
        std::wcout << L"d+++++++++++++++++++++++++++++++" << std::endl;
        std::wcout << L"GetFiscalInfoFunc: " << fiscal << std::endl;

        std::wcout << L"e+++++++++++++++++++++++++++++++" << std::endl;

        std::cin.get();

    
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    return 0;
}
