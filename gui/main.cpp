#include "mainwindow.h"
#include <QApplication>
// solo sirve para iniciar la aplicacion desde qt`
// y mostrar la ventana principal
// y ejecutar el loop de eventos
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
