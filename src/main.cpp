#include "app/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FinInsight");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("FinInsight");

    MainWindow window;
    window.resize(1280, 800);
    window.show();

    return app.exec();
}
