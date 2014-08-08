#include <QDebug>
#include "tdt4255board.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    int ret = a.exec();

    TDT4255Board::destroyInstance();

    return ret;
}
