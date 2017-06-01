#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QTranslator>

#include <cstring>

#include "audio.h"
#include "banpair.h"
#include "mainwindow.h"
#include "server.h"
#include "settings.h"

#ifdef USE_BREAKPAD
#include <QProcess>
#include <client/windows/handler/exception_handler.h>

using namespace google_breakpad;

static bool callback(const wchar_t *, const wchar_t *id, void *, EXCEPTION_POINTERS *, MDRawAssertionInfo *, bool succeeded)
{
    if (succeeded && QFile::exists("QSanSMTPClient.exe")) {
        char ID[16000];
        memset(ID, 0, sizeof(ID));
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
        wcstombs(ID, id, wcslen(id));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        QProcess *process = new QProcess(qApp);
        QStringList args;
        args << QString(ID) + ".dmp";
        process->start("QSanSMTPClient", args);
    }
    return succeeded;
}
#endif

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "-server") == 0)
        new QCoreApplication(argc, argv);
    else {
        new QApplication(argc, argv);
#ifndef Q_OS_ANDROID
        QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/plugins");
#endif
    }

#ifdef USE_BREAKPAD
    //showSplashMessage(QSplashScreen::tr("Loading BreakPad..."));
    ExceptionHandler eh(L"./dmp", NULL, callback, NULL, ExceptionHandler::HANDLER_ALL);
#endif

#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG
    //showSplashMessage(QSplashScreen::tr("Setting game path..."));
    QDir::setCurrent(qApp->applicationDirPath());
#endif
#endif

#ifdef Q_OS_LINUX
    QDir dir(QString("lua"));
    if (dir.exists() && (dir.exists(QString("config.lua")))) {
        // things look good and use current dir
    } else {
#ifndef Q_OS_ANDROID
        QDir::setCurrent(qApp->applicationFilePath().replace("games", "share"));
#else
        bool found = false;
        QDir storageDir("/storage");
        QStringList sdcards = storageDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &sdcard, sdcards) {
            QDir root(QString("/storage/%1/Android/data/org.slob.TouhouTripleSha").arg(sdcard));
            if (root.exists("lua/config.lua")) {
                QDir::setCurrent(root.absolutePath());
                found = true;
                break;
            }
        }
        if (!found) {
            QDir root("/sdcard/Android/data/org.slob.TouhouTripleSha");
            if (root.exists("lua/config.lua")) {
                QDir::setCurrent(root.absolutePath());
                found = true;
            }
        }

        if (!found) {
            QString m
                = QObject::tr("Game data not found, please download TouhouTripleSha PC version, and put the files and folders into /sdcard/Android/data/org.slob.TouhouTripleSha");
            puts(m.toLatin1().constData());

            return -2;
        }
#endif
    }
#endif

    // initialize random seed for later use
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

    QTranslator qt_translator, translator;
    qt_translator.load("qt_zh_CN.qm");
    translator.load("sanguosha.qm");

    qApp->installTranslator(&qt_translator);
    qApp->installTranslator(&translator);

    Sanguosha = new Engine;
    Config.init();
    QFont appFont = Config.AppFont;
#ifdef Q_OS_ANDROID
    appFont = QFont("DroidSansFallback", 6);
#endif

    BanPair::loadBanPairs();

    if (qApp->arguments().contains("-server")) {
        Server *server = new Server(qApp);
        printf("Server is starting on port %u\n", Config.ServerPort);

        if (server->listen())
            printf("Starting successfully\n");
        else {
            delete server;
            printf("Starting failed!\n");
        }

        return qApp->exec();
    } else
        qApp->setFont(appFont);

#if (defined Q_OS_WIN) || (defined Q_OS_MAC) || (defined Q_OS_LINUX)
    QFile winFile("qss/sanguosha.qss");
    if (winFile.open(QIODevice::ReadOnly)) {
        QTextStream winStream(&winFile);
        qApp->setStyleSheet(winStream.readAll());
    }
#endif

#ifdef AUDIO_SUPPORT
    Audio::init();
#endif

    MainWindow *main_window = new MainWindow;

    Sanguosha->setParent(main_window);
    main_window->show();

    main_window->checkUpdate();

    foreach (QString arg, qApp->arguments()) {
        if (arg.startsWith("-connect:")) {
            arg.remove("-connect:");
            Config.HostAddress = arg;
            Config.setValue("HostAddress", arg);

            main_window->startConnection();
            break;
        }
    }

    return qApp->exec();
}
