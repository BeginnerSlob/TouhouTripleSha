#include "achievement.h"
#include "engine.h"

#include <QFile>

class AchieveSkill : public TriggerSkill
{
public:
    AchieveSkill(QString objectName)
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

    QStringList getAchievementTranslations(QString _key = QString()) const
    {
        if (_key.isEmpty())
            _key = key;
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
            if (_line[0] == _key) {
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

class WenGongWuGong : public AchieveSkill
{
public:
    WenGongWuGong()
        : AchieveSkill("wengongwugong")
    {
        events << DamageDone << EventPhaseProceeding;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&ask_who) const
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from) {
                ask_who = damage.from;
                return QStringList(objectName());
            }
        } else if (triggerEvent == EventPhaseProceeding) {
            if (player->getPhase() == Player::Play)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                        ServerPlayer *ask_who) const
    {
        if (triggerEvent == DamageDone) {
            room->addAchievementData(ask_who, "wu", data.value<DamageStruct>().damage);
        } else if (triggerEvent == EventPhaseProceeding) {
            room->addAchievementData(player, "wen");
        }
        return false;
    }
};

class Zhanji : public AchieveSkill
{
public:
    Zhanji()
        : AchieveSkill("zhanji")
    {
        events << GameOverJudge;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        QString winner = room->getWinner(player);
        if (!winner.isEmpty())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QString winner = room->getWinner(player);
        QStringList alive_roles = room->aliveRoles(player);
        QStringList winners = winner.split("+");
        foreach (ServerPlayer *p, room->getPlayers()) {
            int uid = p->userId();
            if (uid == -1)
                continue;
            qDebug() << p->screenName();
            bool is_escape = p->property("run").toBool();
            bool is_win = winners.contains(p->getRole()) || winners.contains(p->objectName());
            QStringList finished_achieve = room->getAchievementData(p, "finished").toString().split("|");
            QStringList translated_achieve;
            foreach (QString _key, finished_achieve) {
                QStringList trans = getAchievementTranslations(_key);
                if (trans.isEmpty())
                    continue;
                translated_achieve << trans[0];
            }
            if (translated_achieve.isEmpty())
                translated_achieve << tr("None");
            QStringList line;
            line << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
            line << Sanguosha->translate(p->getGeneralName());
            line << Sanguosha->getModeName(room->getMode());
            line << Sanguosha->translate(p->getRole());
            line << QString::number(p->getMark("Global_TurnCount"));
            line << (p->isAlive() ? tr("Alive") : tr("Dead"));
            line << (is_escape ? tr("Escape") : (is_win ? tr("Victory") : tr("Failure")));
            line << (is_escape ? "0" : QString::number(getExp(room, p, winners, alive_roles)));
            line << room->getAchievementData(p, "wen").toString();
            line << room->getAchievementData(p, "wu").toString();
            line << translated_achieve.join(" ");
            qDebug() << line;
            QString location = QString("account/%1_records.csv").arg(uid);
            QFile file(location);
            if (!file.open(QIODevice::ReadWrite))
                continue;
            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            stream.seek(file.size());
            stream << line.join(",") << "\n";
            stream.flush();
            file.close();
        }
        return false;
    }

    int getExp(Room *room, ServerPlayer *player, QStringList winners, QStringList alive_roles) const
    {
        int exp = 5;
        QStringList roles = room->getAchievementData(player, "killed_roles").toStringList();
        switch (player->getRoleEnum()) {
        case Player::Lord:
            if (winners.contains("lord"))
                exp += 4 + 2 * alive_roles.count("loyalist");
            else if (alive_roles.length() == 1 && alive_roles.first() == "renegade")
                exp += 1;
            exp += roles.count("rebel") + roles.count("renegade");
            break;
        case Player::Loyalist:
            if (winners.contains("loyalist"))
                exp += 5 + 2 * alive_roles.count("loyalist");
            exp += roles.count("rebel") + roles.count("renegade");
            break;
        case Player::Rebel:
            if (winners.contains("rebel"))
                exp += 3 * alive_roles.count("rebel");
            exp += roles.count("loyalist") + 2 * roles.count("lord");
            break;
        case Player::Renegade:
            if (player->property("1v1").toBool())
                exp += 8;
            if (winners.contains("rebel") && player->isAlive())
                exp += 1;
            if (winners.contains(player->objectName()))
                exp += 20;
            break;
        }
        return exp;
    }
};

class NDXS : public AchieveSkill
{
public:
    NDXS()
        : AchieveSkill("ndxs")
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
    skills << new WenGongWuGong << new Zhanji;
    skills << new NDXS;
}

ADD_PACKAGE(Achievement)
