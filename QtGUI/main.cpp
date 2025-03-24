/**
 * @file main.cpp
 * @brief Entry point for the POS Communication Demo application
 * 
 * This file contains the main function that initializes and launches the POS Communication
 * Demo application. The application is built using C++ with the Qt framework for the
 * graphical user interface.
 * 
 * @details
 * Language: C++
 * Platform: Qt Framework (Cross-platform)
 * 
 * This application demonstrates communication with Point of Sale (POS) systems
 * through the Integration Hub. It creates a graphical interface that allows
 * users to interact with POS systems, send commands, and receive responses.
 * 
 * The main function performs the following operations:
 * 1. Initializes the Qt application
 * 2. Sets application metadata (name, organization, version)
 * 3. Creates and displays the main application window
 * 4. Implements exception handling to catch and display any unhandled exceptions
 * 
 * @note The application uses try-catch blocks to prevent crashes from unhandled
 * exceptions and displays user-friendly error messages.
 */

#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application information
    QApplication::setApplicationName("POS Communication Demo");
    QApplication::setOrganizationName("YourCompanyName");
    QApplication::setApplicationVersion("1.0.0");
    
    try {
        MainWindow mainWindow;
        mainWindow.show();
        
        return app.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Critical Error", 
                             QString("An unhandled exception occurred: %1").arg(e.what()));
        return 1;
    }
}
