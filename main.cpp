#include "PRDisplay.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PRDisplay w;
    w.show();
    return a.exec();
}
