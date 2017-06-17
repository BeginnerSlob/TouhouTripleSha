#include "achievement.h"

#include <QFile>

class ArchieveSkill : public TriggerSkill
{
public:
    ArchieveSkill(QString objectName)
        : TriggerSkill("#achievement_" + objectName)
    {
        key = objectName;
        frequency = Compulsory;
        global = true;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 0;
    }

    void gainAchievement(ServerPlayer *player, Room *room) const
    {
        int uid = player->userId();
        if (uid == -1)
            return;
        QStringList translations = getAchievementTranslations();
        if (translations.length() == 2) {
            LogMessage log;
            log.type = "#GainAchievement";
            log.arg = player->screenName();
            log.arg2 = translations[0];
            room->sendLog(log);
            log.type = "#AchievementDescription";
            log.arg = translations[1];
            room->sendLog(log);
        }
        QString location = QString("account/%1_achievement.csv").arg(uid);
        QFile file(location);
        file.open(QIODevice::ReadWrite);
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        QStringList lines = stream.readAll().split("\n", QString::SkipEmptyParts);
        stream.seek(0);
        bool modified = false;
        foreach (QString line, lines) {
            QStringList _line = line.split(",");
            if (_line.length() != 4) {
                stream << line << "\n";
            } else if (_line[0] == key) {
                int num = _line[1].toInt() + 1;
                QString date = _line[2];
                if (date == "1990-01-01") {
                    QDateTime time = QDateTime::currentDateTime();
                    date = time.toString("yyyy-MM-dd");
                }
                stream << QString("%1,%2,%3,%4").arg(key).arg(num).arg(date).arg(_line[3]) << "\n";
                modified = true;
            } else
                stream << line << "\n";
        }
        if (!modified) {
            stream.seek(file.size());
            QDateTime time = QDateTime::currentDateTime();
            stream << QString("%1,1,%2,-1").arg(key).arg(time.toString("yyyy-MM-dd")) << "\n";
        }
        stream.flush();
        file.close();
    }

    QStringList getAchievementTranslations() const
    {
        QString location = QString("account/achievement.csv");
        QFile file(location);
        file.open(QIODevice::ReadOnly);
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList _line = line.split(",");
            if (_line.length() != 5)
                continue;
            if (_line[0] == key) {
                stream.flush();
                file.close();
                QStringList lists;
                lists << _line[1] << _line[2];
                return lists;
            }
        }
        stream.flush();
        file.close();
        return QStringList();
    }

    QString key;
};

class NDXS : public ArchieveSkill
{
public:
    NDXS()
        : ArchieveSkill("ndxs")
    {
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key, 1, false);
        if (room->getAchievementData(player, key, false) == 3)
            gainAchievement(player, room);
        return false;
    }
};

AchievementPackage::AchievementPackage()
    : Package("achievement", SpecialPack)
{
    skills << new NDXS;
}

ADD_PACKAGE(Achievement)
