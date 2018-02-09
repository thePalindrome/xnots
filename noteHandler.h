#pragma once
#include <QBasicTimer>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextEdit>
#include <QHash>
#include <QStringList>
#include <QTimerEvent>
#include <QFileSystemWatcher>

class noteHandler : public QObject
{
    Q_OBJECT

public:
    noteHandler(QDir *notesDir);
    ~noteHandler();

public slots:
    void onDirChange(const QString &path);
    void onFileChange(const QString &path);

private:
    QDir *dir;
    QHash<QString, QTextEdit*> *windows;
    QFileSystemWatcher watcher;
    void scanDir();
    QString *readNote(const QString filename);
};
