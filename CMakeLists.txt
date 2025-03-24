cmake_minimum_required(VERSION 3.14)
project(POSCommunicationDemo LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt 5 packages
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# Source files
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    poscommunication.cpp
    poscommunication.h
)

# Create executable
add_executable(POSCommunicationDemo ${PROJECT_SOURCES})

# Link Qt libraries
target_link_libraries(POSCommunicationDemo PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

# Copy DLLs to output directory
if(WIN32)
    add_custom_command(TARGET POSCommunicationDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/IntegrationHubCpp.dll"
            "${CMAKE_SOURCE_DIR}/libcrypto-3.dll"
            "${CMAKE_SOURCE_DIR}/libusb-1.0.dll"
            "${CMAKE_SOURCE_DIR}/zlib1.dll"
            $<TARGET_FILE_DIR:POSCommunicationDemo>
    )
endif()
