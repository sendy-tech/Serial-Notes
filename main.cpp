#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/app.ico"));
    MainWindow w;
    w.show();
    return app.exec();
}
