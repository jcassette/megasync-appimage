#-------------------------------------------------
#
# Project created by QtCreator 2013-10-22T17:12:03
#
#-------------------------------------------------

CONFIG -= qt

TARGET = MEGAShellExt
TEMPLATE = lib

LIBS += -luser32 -lole32 -loleaut32 -lgdi32 -luuid -lAdvapi32 -lShell32

DEF_FILE = GlobalExportFunctions.def
RC_FILE = MEGAShellExt.rc

OTHER_FILES += GlobalExportFunctions.def MEGAShellExt.rc

HEADERS += \
    ClassFactoryShellExtNotFound.h \
    ShellExt.h \
    ShellExtNotASync.h \
    resource.h \
    RegUtils.h \
    ContextMenuExt.h \
    ClassFactoryShellExtSyncing.h \
    ClassFactoryShellExtSynced.h \
    ClassFactoryShellExtPending.h \
    ClassFactoryContextMenuExt.h \
    ClassFactory.h \
    MegaInterface.h

SOURCES += \
    ClassFactoryShellExtNotFound.cpp \
    ShellExt.cpp \
    RegUtils.cpp \
    ShellExtNotASync.cpp \
    dllmain.cpp \
    ContextMenuExt.cpp \
    ClassFactoryShellExtSyncing.cpp \
    ClassFactoryShellExtSynced.cpp \
    ClassFactoryShellExtPending.cpp \
    ClassFactoryContextMenuExt.cpp \
    ClassFactory.cpp \
    MegaInterface.cpp

win32:RC_FILE = MEGAShellExt.rc

QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

QMAKE_CXXFLAGS_RELEASE += -MT
QMAKE_CXXFLAGS_DEBUG += -MTd

QMAKE_CXXFLAGS_RELEASE -= -MD
QMAKE_CXXFLAGS_DEBUG -= -MDd

DEFINES += UNICODE _UNICODE
