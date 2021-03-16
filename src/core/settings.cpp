#include "settings.h"
#include "card.h"
#include "engine.h"
#include "photo.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFontDatabase>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QStringList>

Settings Config;

static const qreal ViewWidth = 1280 * 0.8;
static const qreal ViewHeight = 800 * 0.8;

//consts
const int Settings::S_SURRENDER_REQUEST_MIN_INTERVAL = 5000;
const int Settings::S_PROGRESS_BAR_UPDATE_INTERVAL = 200;
const int Settings::S_SERVER_TIMEOUT_GRACIOUS_PERIOD = 1000;
const int Settings::S_MOVE_CARD_ANIMATION_DURATION = 600;
const int Settings::S_JUDGE_ANIMATION_DURATION = 1200;
const int Settings::S_JUDGE_LONG_DELAY = 800;

const QString Settings::m_autoSpeakString = "BB prevent offline";
const QString Settings::m_defaultMusicPath = "audio/bgm/1.ogg;audio/bgm/2.ogg;audio/bgm/3.ogg;audio/bgm/4.ogg;audio/bgm/"
                                             "5.ogg;audio/bgm/6.ogg;audio/bgm/7.ogg;audio/bgm/8.ogg;audio/bgm/9.ogg;audio/bgm/"
                                             "10.ogg;audio/bgm/11.ogg;audio/bgm/12.ogg";

Settings::Settings()
#ifdef Q_OS_WIN32
    : QSettings("config.ini", QSettings::IniFormat)
    ,
#else
    : QSettings("TouhouTripleSha.org", "TouhouTripleSha")
    ,
#endif
    Rect(-ViewWidth / 2, -ViewHeight / 2, ViewWidth, ViewHeight)
{
}

void Settings::init()
{
    lua_State *lua = Sanguosha->getLuaState();
    if (!qApp->arguments().contains("-server")) {
        QVariant app_font = value("AppFont");
        QVariant ui_font = value("UIFont");
        QString font_path = value("DefaultFontPath", "font/wqy-microhei.ttc").toString();
        int font_id = QFontDatabase::addApplicationFont(font_path);
        if (font_id != -1) {
            QString font_family = QFontDatabase::applicationFontFamilies(font_id).first();
            BigFont.setFamily(font_family);
            SmallFont.setFamily(font_family);
            TinyFont.setFamily(font_family);
            if (app_font == QVariant())
                AppFont = QFont(font_family, 9);
            else
                AppFont = app_font.value<QFont>();
            if (ui_font == QVariant())
                UIFont = QFont(font_family, 9);
            else
                UIFont = ui_font.value<QFont>();
        } else
            QMessageBox::warning(NULL, tr("Warning"), tr("Font file %1 could not be loaded!").arg(font_path));

        int big_font = GetConfigFromLuaState(lua, "big_font").toInt();
        int small_font = GetConfigFromLuaState(lua, "small_font").toInt();
        int tiny_font = GetConfigFromLuaState(lua, "tiny_font").toInt();
        BigFont.setPixelSize(big_font);
        SmallFont.setPixelSize(small_font);
        TinyFont.setPixelSize(tiny_font);

        SmallFont.setWeight(QFont::Bold);
        //AppFont = value("AppFont", QApplication::font("QMainWindow")).value<QFont>();
        //UIFont = value("UIFont", QApplication::font("QTextEdit")).value<QFont>();
        TextEditColor = QColor(value("TextEditColor", "white").toString());
    }

    CountDownSeconds = value("CountDownSeconds", 3).toInt();
    GameMode = value("GameMode", "02p").toString();

    QStringList banpackagelist = value("BanPackages").toStringList();
    if (banpackagelist.isEmpty()) {
        banpackagelist << "standard"
                       << "wind"
                       << "fire"
                       << "thicket"
                       << "mountain"
                       << "god"
                       << "Special3v3"
                       << "Special1v1"
                       << "assassins"
                       << "sp-old"
                       << "OL"
                       << "Taiwan_sp"
                       << "miscellaneous"
                       << "BGM"
                       << "BGMDIY"
                       << "ling"
                       << "hegemony"
                       << "h_formation"
                       << "h_momentum"
                       << "hegemony_sp"
                       << "nostal_standard"
                       << "nostal_general"
                       << "nostal_wind"
                       << "nostal_yjcm"
                       << "nostal_yjcm2013"
                       << "test"
                       << "GreenHand"
                       << "GreenHandCard"
                       << "New3v3Card"
                       << "New3v3_2013Card"
                       << "New1v1Card"
                       << "hulaopass"
                       << "touhou-kami_cards";
    }
    setValue("BanPackages", banpackagelist);

    BanPackages = value("BanPackages").toStringList();

    RandomSeat = value("RandomSeat", true).toBool();
    if (value("EnableTest", false).toBool())
        EnableCheat = value("EnableCheat", false).toBool();
    else
        EnableCheat = false;

    FreeChoose = EnableCheat && value("FreeChoose", false).toBool();
    ForbidSIMC = value("ForbidSIMC", false).toBool();
    DisableChat = value("DisableChat", false).toBool();
    FreeAssignSelf = EnableCheat && value("FreeAssignSelf", false).toBool();
    Enable2ndGeneral = value("Enable2ndGeneral", false).toBool();
    EnableSame = value("EnableSame", false).toBool();
    EnableBasara = value("EnableBasara", false).toBool();
    EnableHegemony = value("EnableHegemony", false).toBool();
    MaxHpScheme = value("MaxHpScheme", 0).toInt();
    Scheme0Subtraction = value("Scheme0Subtraction", 3).toInt();
    PreventAwakenBelow3 = value("PreventAwakenBelow3", false).toBool();
    Address = value("Address", QString()).toString();
    EnableAI = value("EnableAI", true).toBool();
    OriginAIDelay = value("OriginAIDelay", 1000).toInt();
    AlterAIDelayAD = value("AlterAIDelayAD", false).toBool();
    AIDelayAD = value("AIDelayAD", 0).toInt();
    SurrenderAtDeath = value("SurrenderAtDeath", false).toBool();
    EnableLuckCard = value("EnableLuckCard", false).toBool();
    ServerPort = value("ServerPort", 9527u).toUInt();
    DisableLua = value("DisableLua", false).toBool();

#ifdef Q_OS_WIN32
    UserName = value("UserName", qgetenv("USERNAME")).toString();
#else
    UserName = value("USERNAME", qgetenv("USER")).toString();
#endif

    if (UserName == "root" || UserName == "Administrator" || UserName.isEmpty())
        UserName = tr("TripleShaFans");
    ServerName = value("ServerName", tr("%1's touhoukifugeki").arg(UserName)).toString();
    DefaultPassword = value("DefaultPassword").toString();

    HostAddress = value("HostAddress", "127.0.0.1").toString();
    UserAvatar = value("UserAvatar", "kaze001").toString();
    HistoryIPs = value("HistoryIPs").toStringList();
    DetectorPort = value("DetectorPort", 9526u).toUInt();
    MaxCards = value("MaxCards", 15).toInt();

    EnableHotKey = value("EnableHotKey", true).toBool();
    NeverNullifyMyTrick = value("NeverNullifyMyTrick", true).toBool();
    EnableMinimizeDialog = value("EnableMinimizeDialog", false).toBool();
    EnableAutoTarget = value("EnableAutoTarget", true).toBool();
    EnableIntellectualSelection = value("EnableIntellectualSelection", true).toBool();
    EnableDoubleClick = value("EnableDoubleClick", false).toBool();
    EnableSuperDrag = value("EnableSuperDrag", false).toBool();
    NullificationCountDown = value("NullificationCountDown", 8).toInt();
    OperationTimeout = value("OperationTimeout", 15).toInt();
    OperationNoLimit = value("OperationNoLimit", false).toBool();
    EnableEffects = value("EnableEffects", true).toBool();
    EnableLastWord = value("EnableLastWord", true).toBool();
    EnableBgMusic = value("EnableBgMusic", true).toBool();
    BGMVolume = value("BGMVolume", 1.0f).toFloat();
    EffectVolume = value("EffectVolume", 1.0f).toFloat();

    BackgroundImage = value("BackgroundImage", "image/system/backdrop/default.jpg").toString();

    BubbleChatBoxKeepTime = value("BubbleChatboxKeepTime", 2000).toInt();

    AutoSpeakWhenWaiting = value("AutoSpeakWhenWaiting", true).toBool();

    QStringList roles_ban, kof_ban, hulao_ban, xmode_ban, basara_ban, hegemony_ban, pairs_ban, cards_ban;

    roles_ban = GetConfigFromLuaState(lua, "roles_ban").toStringList();
    kof_ban = GetConfigFromLuaState(lua, "kof_ban").toStringList();
    hulao_ban = GetConfigFromLuaState(lua, "hulao_ban").toStringList();
    xmode_ban = GetConfigFromLuaState(lua, "xmode_ban").toStringList();
    basara_ban = GetConfigFromLuaState(lua, "basara_ban").toStringList();
    hegemony_ban = GetConfigFromLuaState(lua, "hegemony_ban").toStringList();
    hegemony_ban.append(basara_ban);
    foreach (QString general, Sanguosha->getLimitedGeneralNames()) {
        if (Sanguosha->getGeneral(general)->getKingdom() == "god" && !hegemony_ban.contains(general))
            hegemony_ban << general;
    }
    pairs_ban = GetConfigFromLuaState(lua, "pairs_ban").toStringList();
    cards_ban = GetConfigFromLuaState(lua, "cards_ban").toStringList();

    QStringList banlist = value("Banlist/Roles").toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, roles_ban)
            banlist << ban_general;

        setValue("Banlist/Roles", banlist);
    }

    banlist = value("Banlist/KOF").toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, kof_ban)
            banlist << ban_general;

        setValue("Banlist/KOF", banlist);
    }

    banlist = value("Banlist/Basara").toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, basara_ban)
            banlist << ban_general;

        setValue("Banlist/Basara", banlist);
    }

    banlist = value("Banlist/Hegemony").toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, hegemony_ban)
            banlist << ban_general;
        setValue("Banlist/Hegemony", banlist);
    }

    banlist = value("Banlist/Pairs").toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, pairs_ban)
            banlist << ban_general;

        setValue("Banlist/Pairs", banlist);
    }

    banlist = value("Banlist/Cards").toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_card, cards_ban)
            banlist << ban_card;

        setValue("Banlist/Cards", banlist);
    }

    QStringList forbid_packages = value("ForbidPackages").toStringList();
    if (forbid_packages.isEmpty()) {
        forbid_packages << "GreenHandCard"
                        << "New3v3Card"
                        << "New3v3_2013Card"
                        << "New1v1Card"
                        << "hulaopass"
                        << "touhou-kami_cards";
        setValue("ForbidPackages", forbid_packages);
    }
}
