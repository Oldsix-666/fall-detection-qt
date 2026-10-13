QT += widgets

CONFIG += c++17

# 让 #include "ui/mainwindow.h"、"detector/xxx.h" 这类相对 src 的写法能找到文件
INCLUDEPATH += src

# ==================== OpenCV ====================
OPENCV_DIR = D:/Tool/Opencv/opencv/build

INCLUDEPATH += $$OPENCV_DIR/include

CONFIG(debug, debug|release) {
    LIBS += -L$$OPENCV_DIR/x64/vc16/lib -lopencv_world500d
    OPENCV_DLL = opencv_world500d.dll
} else {
    LIBS += -L$$OPENCV_DIR/x64/vc16/lib -lopencv_world500
    OPENCV_DLL = opencv_world500.dll
}

# 链接完成后，把 OpenCV 的运行时 DLL 拷到 exe 旁边，免去手工配 PATH
win32 {
    OPENCV_BIN = $$shell_path($$OPENCV_DIR/x64/vc16/bin)
    CONFIG(debug, debug|release) {
        QMAKE_POST_LINK += $$quote(cmd /c xcopy /Y /D "$$OPENCV_BIN\\$$OPENCV_DLL" "$$shell_path($$OUT_PWD/debug)")
        QMAKE_POST_LINK += $$quote(& cmd /c xcopy /Y /D "$$OPENCV_BIN\\opencv_videoio_*.dll" "$$shell_path($$OUT_PWD/debug)")
    } else {
        QMAKE_POST_LINK += $$quote(cmd /c xcopy /Y /D "$$OPENCV_BIN\\$$OPENCV_DLL" "$$shell_path($$OUT_PWD/release)")
        QMAKE_POST_LINK += $$quote(& cmd /c xcopy /Y /D "$$OPENCV_BIN\\opencv_videoio_*.dll" "$$shell_path($$OUT_PWD/release)")
    }
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    src/detector/detectionworker.cpp \
    src/detector/falljudge.cpp \
    src/source/camerasource.cpp \
    src/source/framesourcefactory.cpp \
    src/source/rtspsource.cpp \
    src/ui/mainwindow.cpp \
    src/ui/videowidget.cpp

HEADERS += \
    src/core/types.h \
    src/detector/detectionworker.h \
    src/detector/falljudge.h \
    src/source/camerasource.h \
    src/source/framesourcefactory.h \
    src/source/iframesource.h \
    src/source/rtspsource.h \
    src/ui/mainwindow.h \
    src/ui/videowidget.h

FORMS += \
    MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
