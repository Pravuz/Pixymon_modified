#-------------------------------------------------
#
# Project created by QtCreator 2012-07-30T13:51:47
#
#-------------------------------------------------

QT       += core gui widgets xml

TARGET = PixyMon
TEMPLATE = app
RC_FILE = resources.rc

SOURCES += main.cpp\
        mainwindow.cpp \
    videowidget.cpp \
    usblink.cpp \
    console.cpp \
    interpreter.cpp \
    renderer.cpp \
    chirpmon.cpp \
    dfu.cpp \
    connectevent.cpp \
    flash.cpp \
    reader.cpp \
    ../../common/chirp.cpp \
    ../../common/colorlut.cpp \
    ../../common/blob.cpp \
    ../../common/blobs.cpp \
    ../../common/qqueue.cpp \
    ../../common/calc.cpp \
    configdialog.cpp \
    parameters.cpp \
    paramfile.cpp \
    dataexport.cpp \
    monmodule.cpp \
    monparameterdb.cpp \
    debug.cpp \
    urmmodule.cpp \
    disconnectevent.cpp

HEADERS  += mainwindow.h \
    videowidget.h \
    usblink.h \
    console.h \
    interpreter.h \
    renderer.h \
    chirpmon.h \
    dfu.h \
    usb_dfu.h \
    dfu_info.h \
    connectevent.h \
    flash.h \
    reader.h \
    ../../common/pixytypes.h \
    ../../common/pixydefs.h \
    ../../common/chirp.hpp \
    ../../common/colorlut.h \
    ../../common/blobs.h \
    ../../common/blob.h \
    ../../common/blobs.h \
    ../../common/qqueue.h \
    ../../common/link.h \
    ../../common/calc.h \
    ../../common/simplevector.h \
    pixymon.h \
    configdialog.h \
    sleeper.h \
    parameters.h \
    paramfile.h \
    dataexport.h \
    monmodule.h \
    monparameterdb.h \
    debug.h \
    urmmodule.h \
    disconnectevent.h \
    lusb0_usb.h

INCLUDEPATH += ../../common

QMAKE_CXXFLAGS_DEBUG += -O0
QMAKE_CXXFLAGS += -Wno-unused-parameter
FORMS    += mainwindow.ui \
    configdialog.ui

# LIBS += ./libusb-1.0.dll.a

win32 {
    DEFINES += __WINDOWS__
    QMAKE_CXXFLAGS += -mno-ms-bitfields
    LIBS += ../windows/libusb-1.0.dll.a
    HEADERS += ../windows/libusb.h
    INCLUDEPATH += ../windows
    QMAKE_CXXFLAGS += -mno-ms-bitfields
}

macx {
    ICON = pixy.icns
    DEFINES += __MACOS__
    #CONFIG += x86
    #CONFIG -= x86_64
    LIBS += -L/opt/local/lib -lusb-1.0
    INCLUDEPATH += /opt/local/include/libusb-1.0
    #QMAKE_MAC_SDK = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk
    #QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
}

unix:!macx {
    DEFINES += __LINUX__
    PKGCONFIG += libusb-1.0
    LIBS += -lusb-1.0
    INCLUDEPATH += /usr/include/libusb-1.0
   
}

RESOURCES += \
    resources.qrc

DISTFILES += \
    pixyflash.bin.hdr \
    pixy.ico \
    pixymon.pro.user \
    pixymon.pro.user.1db1b83 \
    pixymon.pro.user.82e9e8a \
    VERSION \
    resources.rc \
    plotlut.m \
    plotscript.m \
    pixymon.pro.KR4076







