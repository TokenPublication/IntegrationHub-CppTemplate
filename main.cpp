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
