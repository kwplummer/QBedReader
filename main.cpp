#include <QtGui/QApplication>
#include "qbedreader.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QBedReader w;
    w.setWindowIcon(QIcon(":/Ico/bed_icon.png"));
    w.show();
    return a.exec();
}
