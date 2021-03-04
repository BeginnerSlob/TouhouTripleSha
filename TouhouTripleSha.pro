# -------------------------------------------------
# Project created by QtCreator 2010-06-13T04:26:52
# -------------------------------------------------
TARGET = TouhouTripleSha
QT += network widgets
TEMPLATE = app
CONFIG += audio
win32: QT += winextras

CONFIG += lua
#CONFIG += lua53alpha

SOURCES += \
    src/main.cpp \
    src/client/aux-skills.cpp \
    src/client/client.cpp \
    src/client/clientplayer.cpp \
    src/client/clientstruct.cpp \
    src/core/banpair.cpp \
    src/core/card.cpp \
    src/core/engine.cpp \
    src/core/general.cpp \
    src/core/lua-wrapper.cpp \
    src/core/player.cpp \
    src/core/protocol.cpp \
    src/core/record-analysis.cpp \
    src/core/settings.cpp \
    src/core/skill.cpp \
    src/core/structs.cpp \
    src/core/util.cpp \
    src/dialog/cardoverview.cpp \
    src/dialog/configdialog.cpp \
    src/dialog/connectiondialog.cpp \
    src/dialog/customassigndialog.cpp \
    src/dialog/distanceviewdialog.cpp \
    src/dialog/generaloverview.cpp \
    src/dialog/mainwindow.cpp \
    src/package/exppattern.cpp \
    src/package/standard.cpp \
    src/package/package.cpp \
    src/scenario/miniscenarios.cpp \
    src/scenario/scenario.cpp \
    src/server/ai.cpp \
    src/server/gamerule.cpp \
    src/server/generalselector.cpp \
    src/server/room.cpp \
    src/server/roomthread.cpp \
    src/server/server.cpp \
    src/server/serverplayer.cpp \
    src/ui/button.cpp \
    src/ui/cardcontainer.cpp \
    src/ui/carditem.cpp \
    src/ui/chatwidget.cpp \
    src/ui/clientlogbox.cpp \
    src/ui/dashboard.cpp \
    src/ui/indicatoritem.cpp \
    src/ui/magatamas-item.cpp \
    src/ui/photo.cpp \
    src/ui/pixmapanimation.cpp \
    src/ui/qsanbutton.cpp \
    src/ui/rolecombobox.cpp \
    src/ui/roomscene.cpp \
    src/ui/sprite.cpp \
    src/ui/startscene.cpp \
    src/ui/window.cpp \
    src/ui/bubblechatbox.cpp \
    src/util/detector.cpp \
    src/util/nativesocket.cpp \
    src/util/recorder.cpp \
    swig/sanguosha_wrap.cxx \
    src/core/json.cpp \
    src/core/room-state.cpp \
    src/core/wrapped-card.cpp \
    src/dialog/choosegeneraldialog.cpp \
    src/dialog/roleassigndialog.cpp \
    src/dialog/scenario-overview.cpp \
    src/package/bgm.cpp \
    src/package/god.cpp \
    src/package/h-formation.cpp \
    src/package/h-momentum.cpp \
    src/package/hegemony.cpp \
    src/package/ikai-do.cpp \
    src/package/ikai-ka.cpp \
    src/package/ikai-kin.cpp \
    src/package/ikai-moku.cpp \
    src/package/ikai-sui.cpp \
    src/package/ling.cpp \
    src/package/maneuvering.cpp \
    src/package/fantasy.cpp \
    src/package/nostalgia.cpp \
    src/package/sp.cpp \
    src/package/special1v1.cpp \
    src/package/special3v3.cpp \
    src/package/standard-cards.cpp \
    src/package/standard-generals.cpp \
    src/package/standard-skillcards.cpp \
    src/package/touhou-bangai.cpp \
    src/package/touhou-hana.cpp \
    src/package/touhou-kami.cpp \
    src/package/touhou-kaze.cpp \
    src/package/touhou-shin.cpp \
    src/package/touhou-sp.cpp \
    src/package/touhou-tsuki.cpp \
    src/package/touhou-yuki.cpp \
    src/package/wind.cpp \
    src/scenario/couple-scenario.cpp \
    src/scenario/fancheng-scenario.cpp \
    src/scenario/guandu-scenario.cpp \
    src/server/roomthread1v1.cpp \
    src/server/roomthread3v3.cpp \
    src/server/roomthreadxmode.cpp \
    src/ui/generic-cardcontainer-ui.cpp \
    src/ui/qsan-selectable-item.cpp \
    src/ui/skin-bank.cpp \
    src/ui/table-pile.cpp \
    src/ui/timed-progressbar.cpp \
    src/ui/ui-utils.cpp \
    src/package/touhou-story.cpp \
    src/scenario/chunxue-scenario.cpp \
    src/scenario/jianniang-scenario.cpp \
    src/package/tenshi-reihou.cpp \
    src/ui/graphicsbox.cpp \
    src/ui/chooseoptionsbox.cpp \
    src/ui/playercardbox.cpp \
    src/ui/choosetriggerorderbox.cpp \
    src/ui/choosesuitbox.cpp \
    src/package/achievement.cpp \
    src/ui/guanxingbox.cpp \
    swig/sanguosha_wrap.cxx

HEADERS += \
    src/client/aux-skills.h \
    src/client/client.h \
    src/client/clientplayer.h \
    src/client/clientstruct.h \
    src/core/audio.h \
    src/core/banpair.h \
    src/core/card.h \
    src/core/compiler-specific.h \
    src/core/engine.h \
    src/core/general.h \
    src/core/lua-wrapper.h \
    src/core/player.h \
    src/core/protocol.h \
    src/core/record-analysis.h \
    src/core/settings.h \
    src/core/skill.h \
    src/core/structs.h \
    src/core/util.h \
    src/dialog/cardoverview.h \
    src/dialog/configdialog.h \
    src/dialog/connectiondialog.h \
    src/dialog/customassigndialog.h \
    src/dialog/distanceviewdialog.h \
    src/dialog/generaloverview.h \
    src/dialog/mainwindow.h \
    src/package/exppattern.h \
    src/package/package.h \
    src/package/standard.h \
    src/scenario/miniscenarios.h \
    src/scenario/scenario.h \
    src/server/ai.h \
    src/server/gamerule.h \
    src/server/generalselector.h \
    src/server/room.h \
    src/server/roomthread.h \
    src/server/server.h \
    src/server/serverplayer.h \
    src/ui/button.h \
    src/ui/cardcontainer.h \
    src/ui/carditem.h \
    src/ui/chatwidget.h \
    src/ui/clientlogbox.h \
    src/ui/dashboard.h \
    src/ui/indicatoritem.h \
    src/ui/magatamas-item.h \
    src/ui/photo.h \
    src/ui/pixmapanimation.h \
    src/ui/qsanbutton.h \
    src/ui/rolecombobox.h \
    src/ui/roomscene.h \
    src/ui/sprite.h \
    src/ui/startscene.h \
    src/ui/window.h \
    src/ui/bubblechatbox.h \
    src/util/detector.h \
    src/util/nativesocket.h \
    src/util/recorder.h \
    src/util/socket.h \
    src/core/json.h \
    src/core/room-state.h \
    src/core/wrapped-card.h \
    src/dialog/choosegeneraldialog.h \
    src/dialog/roleassigndialog.h \
    src/dialog/scenario-overview.h \
    src/package/bgm.h \
    src/package/god.h \
    src/package/h-formation.h \
    src/package/h-momentum.h \
    src/package/hegemony.h \
    src/package/ikai-do.h \
    src/package/ikai-ka.h \
    src/package/ikai-kin.h \
    src/package/ikai-moku.h \
    src/package/ikai-sui.h \
    src/package/ling.h \
    src/package/maneuvering.h \
    src/package/fantasy.h \
    src/package/nostalgia.h \
    src/package/sp.h \
    src/package/special1v1.h \
    src/package/special3v3.h \
    src/package/standard-skillcards.h \
    src/package/touhou-bangai.h \
    src/package/touhou-hana.h \
    src/package/touhou-kami.h \
    src/package/touhou-kaze.h \
    src/package/touhou-shin.h \
    src/package/touhou-sp.h \
    src/package/touhou-tsuki.h \
    src/package/touhou-yuki.h \
    src/package/wind.h \
    src/scenario/couple-scenario.h \
    src/scenario/fancheng-scenario.h \
    src/scenario/guandu-scenario.h \
    src/server/roomthread1v1.h \
    src/server/roomthread3v3.h \
    src/server/roomthreadxmode.h \
    src/ui/generic-cardcontainer-ui.h \
    src/ui/qsan-selectable-item.h \
    src/ui/skin-bank.h \
    src/ui/table-pile.h \
    src/ui/timed-progressbar.h \
    src/ui/ui-utils.h \
    src/package/standard-equips.h \
    src/package/touhou-story.h \
    src/scenario/chunxue-scenario.h \
    src/scenario/jianniang-scenario.h \
    src/package/tenshi-reihou.h \
    src/ui/graphicsbox.h \
    src/ui/chooseoptionsbox.h \
    src/ui/playercardbox.h \
    src/ui/choosetriggerorderbox.h \
    src/ui/choosesuitbox.h \
    src/package/achievement.h \
    src/ui/guanxingbox.h

FORMS += \
    src/dialog/cardoverview.ui \
    src/dialog/configdialog.ui \
    src/dialog/generaloverview.ui

win32 {
    FORMS += \
    src/dialog/mainwindow.ui \
    src/dialog/connectiondialog.ui
}
else: linux {
    FORMS += src/dialog/mainwindow.ui
    android {
    FORMS += src/dialog/connectiondialog_android.ui
    } else {
    FORMS += src/dialog/connectiondialog.ui
    }
}
else {
    FORMS += \
    src/dialog/mainwindow.ui \
    src/dialog/connectiondialog.ui
}

INCLUDEPATH += include
INCLUDEPATH += src/client
INCLUDEPATH += src/core
INCLUDEPATH += src/dialog
INCLUDEPATH += src/package
INCLUDEPATH += src/scenario
INCLUDEPATH += src/server
INCLUDEPATH += src/ui
INCLUDEPATH += src/util

win32{
    RC_FILE += resource/icon.rc
}

macx{
    ICON = resource/icon/sgs.icns
}

LIBS += -L.
win32-msvc*{
    DEFINES += _CRT_SECURE_NO_WARNINGS
    !contains(QMAKE_HOST.arch, x86_64) {
        DEFINES += WIN32
        LIBS += -L"$$_PRO_FILE_PWD_/lib/win/x86"
    } else {
        DEFINES += WIN64
        LIBS += -L"$$_PRO_FILE_PWD_/lib/win/x64"
    }
    CONFIG(debug, debug|release) {
        !winrt:INCLUDEPATH += include/vld
    } else {
        QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
    }
}
win32-g++{
    DEFINES += WIN32
    LIBS += -L"$$_PRO_FILE_PWD_/lib/win/MinGW"
    DEFINES += GPP
}
winrt{
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += WINRT
    !winphone {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/winrt/x64"
    } else {
        DEFINES += WINPHONE
        contains($$QMAKESPEC, arm): LIBS += -L"$$_PRO_FILE_PWD_/lib/winphone/arm"
        else : LIBS += -L"$$_PRO_FILE_PWD_/lib/winphone/x86"
    }
}
macx{
    DEFINES += MAC
    LIBS += -L"$$_PRO_FILE_PWD_/lib/mac/lib"
}
ios{
    DEFINES += IOS
    CONFIG(iphonesimulator){
        LIBS += -L"$$_PRO_FILE_PWD_/lib/ios/simulator/lib"
    }
    else {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/ios/device/lib"
    }
}
linux{
    android{
        DEFINES += ANDROID
        ANDROID_LIBPATH = $$_PRO_FILE_PWD_/lib/android/$$ANDROID_ARCHITECTURE/lib
        LIBS += -L"$$ANDROID_LIBPATH"
    }
    else {
        DEFINES += LINUX
        !contains(QMAKE_HOST.arch, x86_64) {
            LIBS += -L"$$_PRO_FILE_PWD_/lib/linux/x86"
            QMAKE_LFLAGS += -Wl,--rpath=lib/linux/x86
        }
        else {
            LIBS += -L"$$_PRO_FILE_PWD_/lib/linux/x64"
            QMAKE_LFLAGS += -Wl,--rpath=lib/linux/x64
        }
    }
}

CONFIG(audio){
    DEFINES += AUDIO_SUPPORT
    INCLUDEPATH += include/fmod
    CONFIG(debug, debug|release): LIBS += -lfmodexL
    else:LIBS += -lfmodex
    SOURCES += src/core/audio.cpp

    android{
        CONFIG(debug, debug|release):ANDROID_EXTRA_LIBS += $$ANDROID_LIBPATH/libfmodexL.so
        else:ANDROID_EXTRA_LIBS += $$ANDROID_LIBPATH/libfmodex.so
    }

    ios: QMAKE_LFLAGS += -framework AudioToolBox
}

CONFIG(lua){

android:DEFINES += "\"getlocaledecpoint()='.'\""

    SOURCES += \
        src/lua/lzio.c \
        src/lua/lvm.c \
        src/lua/lundump.c \
        src/lua/ltm.c \
        src/lua/ltablib.c \
        src/lua/ltable.c \
        src/lua/lstrlib.c \
        src/lua/lstring.c \
        src/lua/lstate.c \
        src/lua/lparser.c \
        src/lua/loslib.c \
        src/lua/lopcodes.c \
        src/lua/lobject.c \
        src/lua/loadlib.c \
        src/lua/lmem.c \
        src/lua/lmathlib.c \
        src/lua/llex.c \
        src/lua/liolib.c \
        src/lua/linit.c \
        src/lua/lgc.c \
        src/lua/lfunc.c \
        src/lua/ldump.c \
        src/lua/ldo.c \
        src/lua/ldebug.c \
        src/lua/ldblib.c \
        src/lua/lctype.c \
        src/lua/lcorolib.c \
        src/lua/lcode.c \
        src/lua/lbitlib.c \
        src/lua/lbaselib.c \
        src/lua/lauxlib.c \
        src/lua/lapi.c
    HEADERS += \
        src/lua/lzio.h \
        src/lua/lvm.h \
        src/lua/lundump.h \
        src/lua/lualib.h \
        src/lua/luaconf.h \
        src/lua/lua.hpp \
        src/lua/lua.h \
        src/lua/ltm.h \
        src/lua/ltable.h \
        src/lua/lstring.h \
        src/lua/lstate.h \
        src/lua/lparser.h \
        src/lua/lopcodes.h \
        src/lua/lobject.h \
        src/lua/lmem.h \
        src/lua/llimits.h \
        src/lua/llex.h \
        src/lua/lgc.h \
        src/lua/lfunc.h \
        src/lua/ldo.h \
        src/lua/ldebug.h \
        src/lua/lctype.h \
        src/lua/lcode.h \
        src/lua/lauxlib.h \
        src/lua/lapi.h
    INCLUDEPATH += src/lua
}

CONFIG(lua53alpha){

android:DEFINES += "\"l_getlocaledecpoint()='.'\""

    SOURCES += \
        src/lua53alpha/lzio.c \
        src/lua53alpha/lvm.c \
        src/lua53alpha/lundump.c \
        src/lua53alpha/ltm.c \
        src/lua53alpha/ltablib.c \
        src/lua53alpha/ltable.c \
        src/lua53alpha/lstrlib.c \
        src/lua53alpha/lstring.c \
        src/lua53alpha/lstate.c \
        src/lua53alpha/lparser.c \
        src/lua53alpha/loslib.c \
        src/lua53alpha/lopcodes.c \
        src/lua53alpha/lobject.c \
        src/lua53alpha/loadlib.c \
        src/lua53alpha/lmem.c \
        src/lua53alpha/lmathlib.c \
        src/lua53alpha/llex.c \
        src/lua53alpha/liolib.c \
        src/lua53alpha/linit.c \
        src/lua53alpha/lgc.c \
        src/lua53alpha/lfunc.c \
        src/lua53alpha/ldump.c \
        src/lua53alpha/ldo.c \
        src/lua53alpha/ldebug.c \
        src/lua53alpha/ldblib.c \
        src/lua53alpha/lctype.c \
        src/lua53alpha/lcorolib.c \
        src/lua53alpha/lcode.c \
        src/lua53alpha/lbitlib.c \
        src/lua53alpha/lbaselib.c \
        src/lua53alpha/lauxlib.c \
        src/lua53alpha/lapi.c \
        src/lua53alpha/lutf8lib.c
    HEADERS += \
        src/lua53alpha/lzio.h \
        src/lua53alpha/lvm.h \
        src/lua53alpha/lundump.h \
        src/lua53alpha/lualib.h \
        src/lua53alpha/luaconf.h \
        src/lua53alpha/lua.hpp \
        src/lua53alpha/lua.h \
        src/lua53alpha/ltm.h \
        src/lua53alpha/ltable.h \
        src/lua53alpha/lstring.h \
        src/lua53alpha/lstate.h \
        src/lua53alpha/lparser.h \
        src/lua53alpha/lopcodes.h \
        src/lua53alpha/lobject.h \
        src/lua53alpha/lmem.h \
        src/lua53alpha/llimits.h \
        src/lua53alpha/llex.h \
        src/lua53alpha/lgc.h \
        src/lua53alpha/lfunc.h \
        src/lua53alpha/ldo.h \
        src/lua53alpha/ldebug.h \
        src/lua53alpha/lctype.h \
        src/lua53alpha/lcode.h \
        src/lua53alpha/lauxlib.h \
        src/lua53alpha/lapi.h
    INCLUDEPATH += src/lua53alpha
}

TRANSLATIONS += builds/sanguosha.ts

!build_pass{
    system("lrelease $$_PRO_FILE_PWD_/builds/sanguosha.ts -qm $$_PRO_FILE_PWD_/sanguosha.qm")

    SWIG_bin = "swig"
    contains(QMAKE_HOST.os, "Windows"): SWIG_bin = "$$_PRO_FILE_PWD_/tools/swig/swig.exe"

    system("$$SWIG_bin -c++ -lua $$_PRO_FILE_PWD_/swig/sanguosha.i")
}

OTHER_FILES += \
    sanguosha.qss

CONFIG(debug, debug|release): LIBS += -lfreetype_D
else:LIBS += -lfreetype

INCLUDEPATH += $$_PRO_FILE_PWD_/include/freetype
DEPENDPATH += $$_PRO_FILE_PWD_/include/freetype

ANDROID_PACKAGE_SOURCE_DIR = $$_PRO_FILE_PWD_/resource/android
