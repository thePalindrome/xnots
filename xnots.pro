# Custom .pro, try not to explode it!

no_markdown {
    SOURCES += noteHandler_noMarkdown.cxx
} else {
    LIBS += -lmarkdown
    SOURCES += noteHandler.cxx
}

QT += widgets
TEMPLATE = app
TARGET = xnots
DEPENDPATH += .
INCLUDEPATH += .
#CONFIG += debug

# Input
HEADERS += noteHandler.h
SOURCES += main.cxx
