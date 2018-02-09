#include "noteHandler.h"
extern "C" {
    #include <mkdio.h>
}

noteHandler::noteHandler(QDir *notesDir)
{
    dir = notesDir;
    windows = new QHash<QString, QTextEdit*>();
    scanDir();
    watcher.addPath(QDir::currentPath());
    watcher.addPaths(dir->entryList(QDir::Files));
    connect(&watcher, SIGNAL(directoryChanged(const QString &)),
            this, SLOT(onDirChange(const QString &)));
    connect(&watcher, SIGNAL(fileChanged(const QString &)),
            this, SLOT(onFileChange(const QString &)));
}

noteHandler::~noteHandler()
{
    delete windows;
}

void noteHandler::onFileChange(const QString &path)
{
    if (QFile::exists(path) == false)
    {
        windows->value(path)->close();
        windows->remove(path);
    }
    else
    {
        QString *text = readNote(path);
        windows->value(path)->setHtml(*text);
        delete text;
    }
}

void noteHandler::onDirChange(const QString &path)
{
    // path is the dir, I'll have to scan for files
    dir->refresh();
    QStringList files = dir->entryList(QDir::Files);
    QStringList::const_iterator iter;
    QStringList watched = watcher.files();
    for (iter = files.constBegin(); iter != files.constEnd(); ++iter)
    {
        if ( watched.contains(*iter) )
        {
            continue;
        }
        watcher.addPath(*iter);
        QTextEdit *textEdit = new QTextEdit();
        QString *noteText = readNote(*iter);
        textEdit->setHtml(*noteText);
        textEdit->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowStaysOnBottomHint | Qt::WindowCloseButtonHint | Qt::Tool);
        textEdit->show();
        windows->insert(*iter, textEdit);
        delete noteText;
    }
}

QString *noteHandler::readNote(const QString filename)
{
    QFile *file = new QFile(filename);
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text ))
    {
        qWarning("ERROR: File %s unreadable", qPrintable(filename));
        return NULL;
    }

    QTextStream in(file);
    bool handlingOptions = true;
    QString *noteText = new QString();
    while ( !in.atEnd() ) {
        QString line = in.readLine();
        if ( handlingOptions && line.startsWith("*") )
        {
            // TODO: Write handler for in-note options
        }
        else if ( handlingOptions && !line.isEmpty() )
        {
            handlingOptions = false;
        }
        if ( !handlingOptions )
        {
            noteText->append(line);
            noteText->append("\n");
        }
    }

    char *htmlBuf = 0;
    MMIOT *doc = mkd_string(qPrintable(*noteText),noteText->length(),0);
    mkd_compile(doc,0);
    mkd_document(doc,&htmlBuf);
    delete noteText;
    delete file;
    return new QString(htmlBuf);
}

void noteHandler::scanDir()
{
    dir->refresh();
    QStringList notes = dir->entryList(QDir::Files);
    
    QHash<QString, QTextEdit *>::iterator iter = windows->begin();
    while ( iter != windows->end() )
    {
      if ( !notes.contains(iter.key()) )
      {
	iter.value()->close();
	iter = windows->erase(iter);
      }
      else
      {
	iter.value()->setHtml(*readNote(iter.key()));
	iter++;
      }
    }

    for ( int i=0; i < notes.size(); i++ )
    {
        if ( !windows->contains(notes.at(i)) )
        {
	    QTextEdit *textEdit = new QTextEdit();
        QString *noteText = readNote(notes.at(i));
	    if ( noteText == NULL )
	    {
		continue;
	    }
	    textEdit->setHtml(*noteText);
	    textEdit->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowStaysOnBottomHint | Qt::WindowCloseButtonHint | Qt::Tool);
	    textEdit->show();
        windows->insert(notes.at(i), textEdit);
        delete noteText;
	}
    }
}
