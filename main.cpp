#include "mainwindow.h"
#include <QApplication>
#include "Log/log.h"

#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //QMyLog* log = new QMyLog;
    //log->initLogger();
    MainWindow w;

    w.show();
    return a.exec();
}
