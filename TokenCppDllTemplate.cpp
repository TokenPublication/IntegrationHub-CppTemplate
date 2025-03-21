/**
 * @file TokenCppDllTemplate.cpp
 * @brief Windows C++ wrapper for IntegrationHubCpp.dll - POS Communication Library
 * 
 * This file implements a C++ wrapper around Token Integration Hub library.
 * 
 * @platform Windows (requires Windows.h and COM support)
 * @language C++
 * @dependencies IntegrationHubCpp.dll, libusb-1.0.dll, libcrypto-3.dll, zlib1.dll
 * 
 * The POSCommunication class implements a singleton pattern to ensure only one
 * instance of the communication interface is active. It handles loading the native
 * library dynamically and provides managed access to its functions.
 * 
 * Key functionalities:
 * - Device connection and state management
 * - Sending basket information to payment terminals
 * - Processing payments
 * - Retrieving fiscal information
 * - Handling device callbacks for serial input and device state changes
 */

#include <iostream>
#include <string>
#include <functional>
#include <Windows.h>
#include <thread>
#include <comdef.h>
#include <comutil.h>


#define EXPORT __declspec(dllexport)

/**
 * @class POSCommunication
 * @brief Singleton wrapper class for POS terminal integration
 * 
 * This class provides a C++ interface to the IntegrationHubCpp.dll functions.
 * It loads the DLL dynamically at runtime and maps function pointers to the
 * exported functions from the DLL. Implements the singleton pattern to ensure
 * only one communication instance exists.
 */
class POSCommunication {
private:
    // Function pointer typedefs for DLL functions
    typedef void* (__cdecl* CreateCommunication)(const std::wstring&);
    typedef void (__cdecl* DeleteCommunication)(void*);
    typedef void (__cdecl* Reconnect)(void*);
    typedef int (__cdecl* GetActiveDeviceIndex)(void*);
    typedef int (__cdecl* SendBasket)(void*, const std::wstring&);
    typedef int (__cdecl* SendPayment)(void*, const std::wstring&);
    typedef BSTR (__cdecl* GetFiscalInfo)(void*);
    // Callback function pointer types
    typedef void(__stdcall* SerialInCallback)(int, BSTR);
    typedef void(__stdcall* DeviceStateCallback)(bool, BSTR);
    typedef void (__cdecl* SetSerialInCallback)(void*, SerialInCallback);
    typedef void (__cdecl* SetDeviceStateCallback)(void*, DeviceStateCallback);

    // Communication object handle from the DLL
    void* _Communication;
    // Callback function pointers
    SerialInCallback _serialInCallback;
    DeviceStateCallback _deviceStateCallback;

    // DLL function pointers
    CreateCommunication createCommunication;
    DeleteCommunication deleteCommunication;
    Reconnect reconnect;
    GetActiveDeviceIndex getActiveDeviceIndex; 
    SendBasket sendBasket;
    SendPayment sendPayment;
    GetFiscalInfo getFiscalInfo;
    SetSerialInCallback setSerialInCallback;
    SetDeviceStateCallback setDeviceStateCallback;

    // Handle to the loaded DLL
    HMODULE libraryHandle;

    /**
     * @brief Loads the native DLL and maps function pointers
     * 
     * This method loads the specified DLL and maps all required function
     * pointers to their corresponding exports in the DLL.
     * 
     * @param libName The name/path of the DLL to load
     * @throws std::runtime_error if the DLL cannot be loaded or required functions not found
     */
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
    /**
     * @brief Constructs a POSCommunication instance
     * 
     * Initializes the communication library with the specified company name.
     * Loads the required DLL and creates a communication instance.
     * 
     * @param companyName The company identifier to use with the POS system
     * @throws std::runtime_error if DLL loading or initialization fails
     */
    POSCommunication(const std::wstring& companyName) {
        LoadNativeLibrary("IntegrationHubCpp.dll");
        _Communication = createCommunication(companyName);
    }

    /**
     * @brief Destructor - cleans up resources
     * 
     * Frees the communication object and unloads the DLL.
     */
    ~POSCommunication() {
        if (_Communication) {
            deleteCommunication(_Communication);
        }
        if (libraryHandle) {
            FreeLibrary(libraryHandle);
        }
    }

    /**
     * @brief Gets the index of the currently active device
     * @return The device index or an error code
     */
    int GetActiveDeviceIndexFunc() {
        return getActiveDeviceIndex(_Communication);  // Hata olan kısmı burasıydı, fonksiyon adı değiştirildi
    }

    /**
     * @brief Sends basket information to the POS device
     * 
     * @param jsonData JSON string containing the basket data
     * @return Status code indicating success (0) or an error code
     */
    int SendBasketFunc(const std::wstring& jsonData) {
        return sendBasket(_Communication, jsonData);
    }

    /**
     * @brief Sends payment information to process a transaction
     * 
     * @param jsonData JSON string containing payment details
     * @return Status code indicating success (0) or an error code
     */
    int SendPaymentFunc(const std::wstring& jsonData) {
        return sendPayment(_Communication, jsonData);
    }

    /**
     * @brief Retrieves fiscal information from the connected device
     * 
     * @return BSTR containing fiscal data in XML or JSON format
     */
    BSTR GetFiscalInfoFunc() {
        return getFiscalInfo(_Communication);
    }

    /**
     * @brief Sets a callback for serial input from the device
     * 
     * @param callback Function to be called when serial data is received
     */
    void SetSerialInCallbackFunc(SerialInCallback callback) {
        _serialInCallback = callback;
        setSerialInCallback(_Communication, _serialInCallback);
    }

    /**
     * @brief Sets a callback for device state changes
     * 
     * This method sets up a callback for device connection/disconnection events.
     * The callback is set in a separate thread to avoid blocking the main thread.
     * 
     * @param callback Function to be called when device state changes
     */
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

    /**
     * @brief Singleton instance pointer
     */
    static POSCommunication* instance;

    /**
     * @brief Gets the singleton instance of POSCommunication
     * 
     * Creates the instance if it doesn't exist yet, otherwise returns the existing instance.
     * 
     * @param companyName Company identifier to use when creating the instance
     * @return Pointer to the singleton POSCommunication instance
     */
    static POSCommunication* GetInstance(const std::wstring& companyName) {
        if (!instance) {
            instance = new POSCommunication(companyName);
        }
        return instance;
    }

    /**
     * @brief Attempts to reconnect to the POS device
     */
    void ReconnectFunc() {
        reconnect(_Communication);
    }

    /**
     * @brief Deletes the communication instance in the DLL
     */
    void DeleteCommunicationFunc() {
        deleteCommunication(_Communication);
    }
};

// Initialize the static instance pointer
POSCommunication* POSCommunication::instance = nullptr;

/**
 * @brief Serial input event handler callback function
 * 
 * This function is called by the DLL when serial data is received from the device.
 * 
 * @param value Type or category of the serial data
 * @param data The actual data received in BSTR format
 */
void __stdcall serialInHandler(int value, BSTR data) {
    std::wcout << L"+++++++++++++++++++++++++++++++" << std::endl;
//    _bstr_t bstr(data);
    std::wcout << L"SerialInCallback called with value: " << value << L", data: " << data << std::endl;
    std::wcout << L"+++++++++++++++++++++++++++++++" << std::endl;
}

/**
 * @brief Device state change event handler callback function
 * 
 * This function is called by the DLL when the device state changes
 * (connected/disconnected).
 * 
 * @param state True if device is connected, false otherwise
 * @param message Additional information about the device state
 */
void __stdcall deviceStateHandler(bool state, BSTR message) {
    std::wcout << L"a+++++++++++++++++++++++++++++++" << std::endl;
//    _bstr_t bstr(message);
    std::wcout << L"DeviceStateCallback called with state: " << state << L", message: " << message << std::endl;
    std::wcout << L"b+++++++++++++++++++++++++++++++" << std::endl;
    if (state) {
       

    }
}

/**
 * @brief Main entry point of the application
 * 
 * Demonstrates the usage of the POSCommunication class by:
 * 1. Loading required DLLs
 * 2. Creating a POSCommunication instance
 * 3. Setting up callbacks for device events
 * 4. Retrieving fiscal information
 * 
 * @return Exit code (0 for success)
 */
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
