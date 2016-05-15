#pragma once
#include <QBasicTimer>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextEdit>
#include <QHash>
#include <QStringList>
#include <QTimerEvent>

class noteHandler : public QObject
{
    Q_OBJECT

public:
    noteHandler(QDir *notesDir);
    ~noteHandler();

protected:
    void timerEvent(QTimerEvent *event);

private:
    QDir *dir;
    QHash<QString, QTextEdit*> *windows;
    QBasicTimer timer;
    void scanDir();
    QString *readNote(const QString filename);
};
