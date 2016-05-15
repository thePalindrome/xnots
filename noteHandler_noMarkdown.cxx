#include "noteHandler.h"

noteHandler::noteHandler(QDir *notesDir)
{
    dir = notesDir;
    windows = new QHash<QString, QTextEdit*>();
    scanDir();
    timer.start((1000)*10,this);
}

void noteHandler::timerEvent(QTimerEvent *event)
{
    scanDir();
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
    return noteText;
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
	    if ( readNote(notes.at(i)) == NULL )
	    {
		continue;
	    }
	    textEdit->setHtml(*readNote(notes.at(i)));
	    textEdit->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowStaysOnBottomHint | Qt::WindowCloseButtonHint | Qt::Tool);
	    textEdit->show();
        windows->insert(notes.at(i), textEdit);
	}
    }
}
