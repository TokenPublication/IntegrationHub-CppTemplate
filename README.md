# POS Communication Demo

A cross-platform Qt GUI application that demonstrates communication with the IntegrationHub library.

## Platform Support

- **Windows**: Full functionality is available, including POS communication.
- **macOS/Linux**: The application will build and run, but POS communication functionality is not available as the IntegrationHub library is Windows-specific.

## Prerequisites

- CMake 3.14 or higher
- Qt 5.15 or Qt 6.x
- C++17 compatible compiler
- IntegrationHub library and its dependencies (Windows only)

## Required DLLs (Windows)

The following DLLs must be available in the application directory or in the system path:
- IntegrationHubCpp.dll
- libcrypto-3.dll
- libusb-1.0.dll
- zlib1.dll

## Building the Application

### Windows

```bash
# Create a build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --config Release

# Copy the required DLLs to the output directory
# (This is done automatically by the CMake script)
```

### macOS/Linux

```bash
# Create a build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .
```

Note: On macOS/Linux, the application will build but the POS communication functionality will be disabled.

## Usage

1. Launch the application
2. On Windows, the application will automatically attempt to connect to the POS device
3. Once connected, you can:
   - Send a basket
   - Send a payment
   - Get fiscal information

## Features

- Cross-platform GUI using Qt
- Windows: Full POS communication functionality
- macOS/Linux: UI only (POS communication not available)
- Logging of all events and communications

## Architecture

The application consists of three main components:

1. **POSCommunication**: A singleton class that handles communication with the IntegrationHub library
2. **MainWindow**: The main GUI window that provides user interaction
3. **Main Application**: Sets up the Qt application and handles global exceptions
