# -------------------------------------------------
# Project created by QtCreator 2010-06-13T04:26:52
# -------------------------------------------------
TARGET = QSanguosha
QT += network sql declarative
TEMPLATE = app
CONFIG += warn_on audio

# If you want to enable joystick support, please uncomment the following line:
# CONFIG += joystick
# However, joystick is not supported under Mac OS X temporarily

# If you want enable voice reading for chat content, uncomment the following line:
# CONFIG += chatvoice
# Also, this function can only enabled under Windows system as it make use of Microsoft TTS

SOURCES += \
    src/core/WrappedCard.cpp \
    src/core/util.cpp \
    src/core/structs.cpp \
    src/core/skill.cpp \
    src/core/settings.cpp \
    src/dialog/scenario-overview.cpp \
    src/core/RoomState.cpp \
    src/dialog/roleassigndialog.cpp \
    src/core/record-analysis.cpp \
    src/core/protocol.cpp \
    src/dialog/playercarddialog.cpp \
    src/core/player.cpp \
    src/dialog/packagingeditor.cpp \
    src/dialog/mainwindow.cpp \
    src/core/lua-wrapper.cpp \
    src/core/jsonutils.cpp \
    src/dialog/halldialog.cpp \
    src/dialog/generaloverview.cpp \
    src/core/general.cpp \
    src/core/engine.cpp \
    src/dialog/distanceviewdialog.cpp \
    src/dialog/customassigndialog.cpp \
    src/dialog/connectiondialog.cpp \
    src/dialog/configdialog.cpp \
    src/client/clientstruct.cpp \
    src/client/clientplayer.cpp \
    src/client/client.cpp \
    src/dialog/choosegeneraldialog.cpp \
    src/dialog/cardoverview.cpp \
    src/dialog/cardeditor.cpp \
    src/core/card.cpp \
    src/core/banpair.cpp \
    src/ui/window.cpp \
    src/ui/uiUtils.cpp \
    src/package/touhou-yuki.cpp \
    src/package/touhou-tsuki.cpp \
    src/package/touhou-kaze.cpp \
    src/package/touhou-hana.cpp \
    src/package/touhou.cpp \
    src/ui/TimedProgressBar.cpp \
    src/ui/TablePile.cpp \
    src/ui/startscene.cpp \
    src/package/standard-generals-wind.cpp \
    src/package/standard-generals-snow.cpp \
    src/package/standard-generals-luna.cpp \
    src/package/standard-generals-bloom.cpp \
    src/package/standard-cards.cpp \
    src/package/standard.cpp \
    src/ui/sprite.cpp \
    src/package/sp.cpp \
    src/ui/SkinBank.cpp \
    src/server/serverplayer.cpp \
    src/server/server.cpp \
    src/scenario/scenerule.cpp \
    src/scenario/scenario.cpp \
    src/server/roomthread3v3.cpp \
    src/server/roomthread1v1.cpp \
    src/server/roomthread.cpp \
    src/server/room.cpp \
    src/ui/rolecombobox.cpp \
    src/util/recorder.cpp \
    src/ui/QSanSelectableItem.cpp \
    src/ui/qsanbutton.cpp \
    src/lua/print.c \
    src/ui/pixmapanimation.cpp \
    src/ui/photo.cpp \
    src/package/package.cpp \
    src/util/nativesocket.cpp \
    src/scenario/miniscenarios.cpp \
    src/package/maneuvering.cpp \
    src/main.cpp \
    src/ui/magatamasItem.cpp \
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
    src/lua/lcode.c \
    src/lua/lbaselib.c \
    src/lua/lauxlib.c \
    src/lua/lapi.c \
    src/package/kami.cpp \
    src/jsoncpp/src/json_writer.cpp \
    src/jsoncpp/src/json_value.cpp \
    src/jsoncpp/src/json_reader.cpp \
    src/ui/indicatoritem.cpp \
    src/ui/GenericCardContainerUI.cpp \
    src/server/generalselector.cpp \
    src/server/gamerule.cpp \
    src/package/exppattern.cpp \
    src/util/detector.cpp \
    src/ui/dashboard.cpp \
    src/scenario/couple-scenario.cpp \
    src/server/contestdb.cpp \
    src/ui/clientlogbox.cpp \
    src/ui/chatwidget.cpp \
    src/ui/carditem.cpp \
    src/ui/cardcontainer.cpp \
    src/ui/button.cpp \
    src/scenario/boss-mode-scenario.cpp \
    src/package/bangai.cpp \
    src/server/ai.cpp \
    src/client/aux-skills.cpp \
    src/ui/roomscene.cpp \
    src/package/standard-skillcards.cpp \
    src/jsoncpp/src/json_valueiterator.inl \
    src/jsoncpp/src/json_internalmap.inl \
    src/jsoncpp/src/json_internalarray.inl \
    swig/sanguosha_wrap.cxx

HEADERS += \
    src/core/WrappedCard.h \
    src/core/util.h \
    src/core/structs.h \
    src/core/skill.h \
    src/core/settings.h \
    src/dialog/scenario-overview.h \
    src/core/RoomState.h \
    src/dialog/roleassigndialog.h \
    src/core/record-analysis.h \
    src/jsoncpp/include/json/reader.h \
    src/core/protocol.h \
    src/dialog/playercarddialog.h \
    src/core/player.h \
    src/dialog/packagingeditor.h \
    src/dialog/mainwindow.h \
    src/core/lua-wrapper.h \
    src/core/jsonutils.h \
    src/jsoncpp/include/json/json.h \
    src/dialog/halldialog.h \
    src/dialog/generaloverview.h \
    src/core/general.h \
    src/jsoncpp/include/json/forwards.h \
    src/jsoncpp/include/json/features.h \
    src/core/engine.h \
    src/dialog/distanceviewdialog.h \
    src/dialog/customassigndialog.h \
    src/dialog/connectiondialog.h \
    src/dialog/configdialog.h \
    src/jsoncpp/include/json/config.h \
    src/core/compiler-specific.h \
    src/client/clientstruct.h \
    src/client/clientplayer.h \
    src/client/client.h \
    src/dialog/choosegeneraldialog.h \
    src/dialog/cardoverview.h \
    src/dialog/cardeditor.h \
    src/core/card.h \
    src/core/banpair.h \
    src/client/aux-skills.h \
    src/jsoncpp/include/json/autolink.h \
    src/core/audio.h \
    src/jsoncpp/include/json/assertions.h \
    src/jsoncpp/include/json/writer.h \
    src/ui/window.h \
    src/jsoncpp/include/json/value.h \
    src/ui/uiUtils.h \
    src/package/touhou.h \
    src/ui/TimedProgressBar.h \
    src/ui/TablePile.h \
    src/ui/startscene.h \
    src/package/standard-generals.h \
    src/package/standard-equips.h \
    src/package/standard.h \
    src/ui/sprite.h \
    src/package/sp.h \
    src/util/socket.h \
    src/ui/SkinBank.h \
    src/server/serverplayer.h \
    src/server/server.h \
    src/scenario/scenerule.h \
    src/scenario/scenario.h \
    src/server/roomthread3v3.h \
    src/server/roomthread1v1.h \
    src/server/roomthread.h \
    src/ui/roomscene.h \
    src/server/room.h \
    src/ui/rolecombobox.h \
    src/util/recorder.h \
    src/ui/QSanSelectableItem.h \
    src/ui/qsanbutton.h \
    src/ui/pixmapanimation.h \
    src/ui/photo.h \
    src/package/package.h \
    src/util/nativesocket.h \
    src/scenario/miniscenarios.h \
    src/package/maneuvering.h \
    src/ui/magatamasItem.h \
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
    src/lua/lcode.h \
    src/lua/lauxlib.h \
    src/lua/lapi.h \
    src/package/kami.h \
    src/jsoncpp/src/json_tool.h \
    src/jsoncpp/src/json_batchallocator.h \
    src/ui/indicatoritem.h \
    src/ui/GenericCardContainerUI.h \
    src/server/generalselector.h \
    src/server/gamerule.h \
    src/package/exppattern.h \
    src/util/detector.h \
    src/ui/dashboard.h \
    src/scenario/couple-scenario.h \
    src/server/contestdb.h \
    src/ui/clientlogbox.h \
    src/ui/chatwidget.h \
    src/ui/carditem.h \
    src/ui/cardcontainer.h \
    src/ui/button.h \
    src/scenario/boss-mode-scenario.h \
    src/package/bangai.h \
    src/server/ai.h

FORMS += \
    src/dialog/cardoverview.ui \
    src/dialog/configdialog.ui \
    src/dialog/connectiondialog.ui \
    src/dialog/generaloverview.ui \
    src/dialog/mainwindow.ui 
    
INCLUDEPATH += include
INCLUDEPATH += src/client
INCLUDEPATH += src/core
INCLUDEPATH += src/dialog
INCLUDEPATH += src/package
INCLUDEPATH += src/scenario
INCLUDEPATH += src/server
INCLUDEPATH += src/ui
INCLUDEPATH += src/util
INCLUDEPATH += src/lua
INCLUDEPATH += src/jsoncpp/include

win32{
    RC_FILE += resource/icon.rc
}

macx{
    ICON = resource/icon/sgs.icns
}


LIBS += -L.

CONFIG(audio){
    DEFINES += AUDIO_SUPPORT
    INCLUDEPATH += include/fmod
    LIBS += -lfmodex
    SOURCES += src/core/audio.cpp
}

CONFIG(joystick){
    DEFINES += JOYSTICK_SUPPORT
    HEADERS += src/ui/joystick.h
    SOURCES += src/ui/joystick.cpp
    win32: LIBS += -lplibjs -lplibul -lwinmm
    unix: LIBS += -lplibjs -lplibul
}

CONFIG(chatvoice){
    win32{
        CONFIG += qaxcontainer
        DEFINES += CHAT_VOICE
    }
}

TRANSLATIONS += sanguosha.ts

OTHER_FILES += \
    sanguosha.qss \
    acknowledgement/main.qml \
    acknowledgement/list.png \
    acknowledgement/back.png

symbian: LIBS += -lfreetype
else:unix|win32: LIBS += -L$$PWD/lib/ -lfreetype

INCLUDEPATH += $$PWD/include/freetype
DEPENDPATH += $$PWD/include/freetype
