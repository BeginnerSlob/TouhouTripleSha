#include "achievement.h"
#include "engine.h"

#include <QFile>

AchieveSkill::AchieveSkill(QString objectName)
    : TriggerSkill("#achievement_" + objectName)
{
    key = objectName;
    frequency = Compulsory;
    global = true;
}

int AchieveSkill::getPriority(TriggerEvent) const
{
    return 0;
}

void AchieveSkill::onGameOver(Room *, ServerPlayer *, QVariant &) const
{
    qDebug() << "return";
    return;
}

void AchieveSkill::gainAchievement(ServerPlayer *player, Room *room) const
{
    int uid = player->userId();
    if (uid == -1)
        return;
    QStringList list = room->getAchievementData(player, "finished").toString().split("|");
    list << key;
    room->setAchievementData(player, "finished", list.join("|"));
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

QStringList AchieveSkill::getAchievementTranslations(QString _key) const
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

class AchievementMain : public AchieveSkill
{
public:
    AchievementMain()
        : AchieveSkill("main")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (player->isOffline())
            player->setProperty("run", true);
        if (death.damage && death.damage->from) {
            QStringList killed_roles = room->getAchievementData(death.damage->from, "killed_roles").toStringList();
            killed_roles << player->getRole();
            room->setAchievementData(death.damage->from, "killed_roles", QVariant::fromValue(killed_roles));
        }
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &data) const
    {
        qDebug() << "main";
        DeathStruct death = data.value<DeathStruct>();
        if (player->isOffline())
            player->setProperty("run", true);
        if (death.damage && death.damage->from) {
            QStringList killed_roles = room->getAchievementData(death.damage->from, "killed_roles").toStringList();
            killed_roles << player->getRole();
            room->setAchievementData(death.damage->from, "killed_roles", QVariant::fromValue(killed_roles));
        }
    }
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

class AchievementRecord : public AchieveSkill
{
public:
    AchievementRecord()
        : AchieveSkill("record")
    {
        events << GameOverJudge;
        // other achieve skill cannot set to GameOverJudge in events, but set the trigger in AchieveSkill::onGameOver
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        QString winner = room->getWinner(player);
        if (!winner.isEmpty())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        const Package *package = Sanguosha->getPackage("achievement");
        if (package) {
            QList<const Skill *> skills = package->getSkills();
            foreach (const Skill *s, skills) {
                if (s->inherits("AchieveSkill")) {
                    const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                    qDebug() << as->objectName();
                    as->onGameOver(room, player, data);
                }
            }
        }
        QString winner = room->getWinner(player);
        QStringList alive_roles = room->aliveRoles(player);
        QStringList winners = winner.split("+");
        foreach (ServerPlayer *p, room->getPlayers()) {
            addPlayerRecord(room, p, winners, alive_roles);
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

    void addPlayerRecord(Room *room, ServerPlayer *player, QStringList winners, QStringList alive_roles) const
    {
        int uid = player->userId();
        if (uid == -1)
            return;
        QString role = player->getRole();
        bool is_escape = player->property("run").toBool();
        if (player->isAlive())
            is_escape = player->isOffline();
        bool is_win = winners.contains(player->getRole()) || winners.contains(player->objectName());
        int exp = is_escape ? 0 : getExp(room, player, winners, alive_roles);
        int wen = room->getAchievementData(player, "wen").toInt();
        int wu = room->getAchievementData(player, "wu").toInt();
        updatePlayerData(uid, role, is_win, is_escape, exp, wen, wu);
        QStringList finished_achieve = room->getAchievementData(player, "finished").toString().split("|");
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
        line << Sanguosha->translate(player->getGeneralName());
        line << Sanguosha->getModeName(room->getMode());
        line << Sanguosha->translate(role);
        line << QString::number(player->getMark("Global_TurnCount"));
        line << (player->isAlive() ? tr("Alive") : tr("Dead"));
        line << (is_escape ? tr("Escape") : (is_win ? tr("Victory") : tr("Failure")));
        line << QString::number(exp);
        line << QString::number(wen);
        line << QString::number(wu);
        line << translated_achieve.join(" ");
        QString location = QString("account/%1_records.csv").arg(uid);
        QFile file(location);
        if (!file.open(QIODevice::ReadWrite))
            return;
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream.seek(file.size());
        stream << line.join(",") << "\n";
        stream.flush();
        file.close();
    }

    void updatePlayerData(int uid, const QString &role, bool is_win, bool is_escape, int exp, int wen, int wu) const
    {
        QString location = QString("account/accounts.csv");
        QFile file(location);
        if (!file.open(QIODevice::ReadWrite))
            return;
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        QStringList lines = stream.readAll().split("\n", QString::SkipEmptyParts);
        stream.seek(0);
        foreach (QString line, lines) {
            QStringList _line = line.split(",");
            if (_line.length() != 12) {
                stream << line << "\n";
            } else if (_line[0] == QString::number(uid)) {
                if (is_win) {
                    if (role == "lord")
                        _line[3] = QString::number(_line[3].toInt() + 1);
                    else if (role == "loyalist")
                        _line[4] = QString::number(_line[4].toInt() + 1);
                    else if (role == "rebel")
                        _line[5] = QString::number(_line[5].toInt() + 1);
                    else if (role == "renegade")
                        _line[6] = QString::number(_line[6].toInt() + 1);
                }
                if (is_escape)
                    _line[7] = QString::number(_line[7].toInt() + 1);
                _line[8] = QString::number(_line[8].toInt() + 1);
                _line[9] = QString::number(_line[9].toInt() + exp);
                _line[10] = QString::number(_line[10].toInt() + wen);
                _line[11] = QString::number(_line[11].toInt() + wu);
                stream << _line.join(",") << "\n";
            } else
                stream << line << "\n";
        }
        stream.flush();
        file.close();
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
        if (use.card->isKindOf("Slash"))
            return QStringList(objectName());
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

class GLGJSSY : public AchieveSkill
{
public:
    GLGJSSY()
        : AchieveSkill("glgjssy")
    {
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key, 1, false);
        if (room->getAchievementData(player, key, false) == 7)
            gainAchievement(player, room);
        return false;
    }
};

class QYHFBQZ : public AchieveSkill
{
public:
    QYHFBQZ()
        : AchieveSkill("qyhfbqz")
    {
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key, 1, false);
        if (room->getAchievementData(player, key, false) == 7)
            gainAchievement(player, room);
        return false;
    }
};

AchievementPackage::AchievementPackage()
    : Package("achievement", SpecialPack)
{
    skills << new AchievementMain << new WenGongWuGong << new AchievementRecord;
    skills << new NDXS << new GLGJSSY << new QYHFBQZ;
}

ADD_PACKAGE(Achievement)
