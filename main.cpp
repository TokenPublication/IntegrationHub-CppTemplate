/**
 * @file main.cpp
 * @brief Entry point for the POS Communication Demo application
 * 
 * This file contains the main function that initializes and launches the Qt-based
 * POS Communication Demo application. It sets up the application environment,
 * creates the main window, and handles top-level exception management.
 * 
 * Platform: Qt C++ cross-platform framework
 * This application demonstrates integration between point-of-sale systems
 * and payment services through the IntegrationHub system.
 */

#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>

/**
 * @brief Application entry point
 * 
 * Initializes the Qt application, sets application metadata, creates and shows
 * the main window, and establishes global exception handling.
 * 
 * @param argc Command line argument count
 * @param argv Command line argument values
 * @return Application exit code (0 for normal exit, 1 for error)
 */
int main(int argc, char *argv[])
{
    // Create the Qt application object
    QApplication app(argc, argv);
    
    // Set application information
    // These values are used for settings storage and display purposes
    QApplication::setApplicationName("POS Communication Demo");
    QApplication::setOrganizationName("YourCompanyName");
    QApplication::setApplicationVersion("1.0.0");
    
    try {
        // Create and display the main application window
        MainWindow mainWindow;
        mainWindow.show();
        
        // Start the Qt event loop
        // This will block until the application is closed
        return app.exec();
    } catch (const std::exception& e) {
        // Global exception handler for unhandled exceptions
        // Displays a message box with the exception details
        QMessageBox::critical(nullptr, "Critical Error", 
                             QString("An unhandled exception occurred: %1").arg(e.what()));
        return 1; // Return error code
    }
}
