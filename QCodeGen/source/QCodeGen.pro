QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# adding icon
RC_ICONS = resources/TAE_Logo_Icons2.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdlg.cpp \
    actionbutton.cpp \
    appexception.cpp \
    appsettings.cpp \
    cblockdata.cpp \
    codegenerator.cpp \
    main.cpp \
    mainwindow.cpp \
    parser.cpp \
    prefsdlg.cpp \
    tabbar.cpp

HEADERS += \
    aboutdlg.h \
    actionbutton.h \
    appexception.h \
    appsettings.h \
    cblockdata.h \
    codegenerator.h \
    mainwindow.h \
    parser.h \
    prefsdlg.h \
    strings.h \
    tabbar.h

FORMS += \
    aboutdlg.ui \
    mainwindow.ui \
    prefsdlg.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
