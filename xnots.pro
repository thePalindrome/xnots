# Custom .pro, try not to explode it!

no_markdown {
} else {
    LIBS += -lmarkdown
    DEFINES += USE_MARKDOWN
}

QT += widgets
target.path = /usr/local/bin/
INSTALLS += target
TEMPLATE = app
TARGET = xnots
DEPENDPATH += .
INCLUDEPATH += .
#CONFIG += debug

# Input
HEADERS += noteHandler.h
SOURCES += main.cxx noteHandler.cxx
