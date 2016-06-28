#include <QtWidgets/QApplication>
#include "graph.h"
#include "tabdialog.h"
#include "window.h"

int main(int argc, char *argv[])
{
//    QApplication a(argc, argv);

    QApplication app(argc, argv);
    QString fileName;

    if (argc >= 2)
        fileName = argv[1];
    else
        fileName = ".";

    TabDialog tabdialog(fileName);
    tabdialog.show();

    return app.exec();


//    Window w;
//    w.show();

//    return a.exec();
}
