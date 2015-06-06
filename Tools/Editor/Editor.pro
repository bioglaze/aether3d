QT       += core gui opengl

TARGET = Editor
CONFIG   += console
CONFIG += c++11

TEMPLATE = app
RC_ICONS = glider.ico
ICON = Editor.icns
QMAKE_INFO_PLIST = Info.plist

INCLUDEPATH += $$PWD/../../../aether3d/Engine/Include
DEPENDPATH += $$PWD/../../../aether3d_build

macx
{
    QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
    LIBS += -L$$PWD/../../../aether3d_build/ -laether3d_osx
    LIBS += -framework CoreFoundation -framework OpenAL -framework QuartzCore -framework IOKit -framework Cocoa
    copyfiles.commands = cp -r $$PWD/copy_to_output/* $$OUT_PWD
    PRE_TARGETDEPS += $$PWD/../../../aether3d_build/libaether3d_osx.a
}
win32 {
    DEFINES += AETHER3D_WINDOWS
    PWD_WIN = $${PWD}
    DESTDIR_WIN = $${OUT_PWD}
    PWD_WIN ~= s,/,\\,g
    DESTDIR_WIN ~= s,/,\\,g
    copyfiles.commands = $$quote(cmd /c xcopy /S /I /y $${PWD_WIN}\copy_to_output $${DESTDIR_WIN})
    PRE_TARGETDEPS += $$PWD/../../../aether3d_build/libaether3d_win.a
}
linux {
    QMAKE_CXXFLAGS += -std=c++11
    LIBS += -ldl -fPIC
    DEFINES += AETHER3D_UNIX
    copyfiles.commands = cp -r $$PWD/copy_to_output/* $$OUT_PWD
}

QMAKE_EXTRA_TARGETS += copyfiles
POST_TARGETDEPS += copyfiles

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    SceneWidget.cpp

HEADERS += \
    MainWindow.hpp \
    SceneWidget.hpp
