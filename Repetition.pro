QT       += core gui network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
include("PullFlow/PullFlow.pri")
include("FileRecord/FileRecord.pri")
include("TCPSocket/TCPSocket.pri")
include("TextAnalysis/TextAnalysis.pri")
include("QsLog/QsLog.pri")
include("Log/Log.pri")

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    TextDisplay.cpp \
    Universal.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    TextDisplay.h \
    Universal.h \
    mainwindow.h

FORMS += \
    TextDisplay.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



INCLUDEPATH += $$PWD/include/ffmpeg \
               $$PWD/include/sdl
DEPENDPATH += $$PWD/include/ffmpeg \
              $$PWD/include/sdl

LIBS += $$PWD/lib/ffmpeg/avdevice.lib \
        $$PWD/lib/ffmpeg/avfilter.lib \
        $$PWD/lib/ffmpeg/avformat.lib \
        $$PWD/lib/ffmpeg/avutil.lib \
        $$PWD/lib/ffmpeg/avcodec.lib \
        $$PWD/lib/ffmpeg/postproc.lib \
        $$PWD/lib/ffmpeg/swresample.lib \
        $$PWD/lib/ffmpeg/swscale.lib \
        $$PWD/lib/sdl/SDL2.lib

#        $$PWD/lib/sdl/SDL2main.lib
