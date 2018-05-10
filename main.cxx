#include <QApplication>
#include <QDir>
#include <QFile>
#include "noteHandler.h"

int main(int argv, char **args) {
    QApplication app(argv,args);

    QDir *notesDir = new QDir(QDir::homePath());
    notesDir->cd(".local/xnots");
    QDir::setCurrent(notesDir->absolutePath());
    if ( !notesDir->exists() ) {
        if ( !notesDir->mkdir(notesDir->absolutePath()) ) {
            qFatal("FATAL: Unable to create nonexistant directory %s", qPrintable(notesDir->absolutePath()));
        }
    }

    QStringList nameFilters;
    //nameFilters << "^[^.]" << "[^~]$"; // TODO: Add .swp handler
    notesDir->setNameFilters(nameFilters);

    noteHandler *handler = new noteHandler(notesDir);

    int returnCode = app.exec();
    delete handler;
    return returnCode;
}
