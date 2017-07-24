#include "achievement.h"
#include "engine.h"
#include "room.h"

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
    return 9;
}

void AchieveSkill::onGameOver(Room *, ServerPlayer *, QVariant &) const
{
}

void AchieveSkill::onWinOrLose(Room *, ServerPlayer *, bool) const
{
}

#define ACCOUNT "account/docs/data/"

void AchieveSkill::gainAchievement(ServerPlayer *player, Room *room) const
{
    int uid = player->userId();
    if (uid == -1)
        return;
    QStringList list = room->getAchievementData(player, "finished").toString().split("|");
    if (list.contains(key))
        return;
    list << key;
    room->setAchievementData(player, "finished", list.join("|"));
    QStringList translations = getAchievementTranslations(key);
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
    QString location = QString(ACCOUNT "%1_achievement.csv").arg(uid);
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

QStringList AchieveSkill::getAchievementTranslations(QString _key)
{
    QString location = QString(ACCOUNT "achievement.csv");
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
        events << GameOverJudge << BeforeGameOver;
        // only trigger BeforeGameOver when standoff/draw/surrender
        // other achieve skill cannot set to GameOverJudge in events, but set the trigger in AchieveSkill::onGameOver
    }

    virtual QStringList triggerable(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (e == GameOverJudge) {
            QString winner = room->getWinner(player);
            if (!winner.isEmpty())
                return QStringList(objectName());
        } else if (e == BeforeGameOver) {
            if (!data.canConvert<DeathStruct>())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (e == GameOverJudge) {
            const Package *package = Sanguosha->getPackage("achievement");
            if (package) {
                QList<const Skill *> skills = package->getSkills();
                foreach (const Skill *s, skills) {
                    if (s->inherits("AchieveSkill")) {
                        const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                        as->onGameOver(room, player, data);
                    }
                }
            }
        } else if (e == BeforeGameOver) {
            QStringList winners = data.toString().split("+");
            if (!winners.contains(".")) {
                foreach (ServerPlayer *p, room->getPlayers()) {
                    bool is_win = winners.contains(p->objectName()) || winners.contains(p->getRole());

                    const Package *package = Sanguosha->getPackage("achievement");
                    if (package) {
                        QList<const Skill *> skills = package->getSkills();
                        foreach (const Skill *s, skills) {
                            if (s->inherits("AchieveSkill")) {
                                const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                                as->onWinOrLose(room, player, is_win);
                            }
                        }
                    }
                }
            }
        }
        QStringList winners;
        QStringList alive_roles;
        if (e == GameOverJudge) {
            QString winner = room->getWinner(player);
            winners = winner.split("+");
            alive_roles = room->aliveRoles(player);
        } else if (e == BeforeGameOver) {
            QString winner = data.toString();
            winners = winner.split("+");
            alive_roles = room->aliveRoles();
        }
        foreach (ServerPlayer *p, room->getPlayers())
            addPlayerRecord(room, p, winners, alive_roles);
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
        QString role = player->getRole();
        bool is_escape = player->property("run").toBool();
        if (player->isAlive())
            is_escape = player->isOffline();
        bool is_draw = winners.contains(".");
        bool is_win = winners.contains(player->getRole()) || winners.contains(player->objectName());
        bool is_alive = player->isAlive();
        int exp = 0;
        if (!is_escape) {
            if (is_draw) {
                if (is_alive)
                    exp += 1;
            } else
                exp = getExp(room, player, winners, alive_roles);
        }
        int wen = room->getAchievementData(player, "wen").toInt();
        int wu = room->getAchievementData(player, "wu").toInt();
        QString _finished_achieve = room->getAchievementData(player, "finished").toString();
        QStringList finished_achieve = _finished_achieve.split("|");
        QStringList translated_achieve;
        foreach (QString _key, finished_achieve) {
            QStringList trans = getAchievementTranslations(_key);
            if (trans.isEmpty())
                continue;
            translated_achieve << trans[0];
        }
        if (translated_achieve.isEmpty())
            translated_achieve << AchieveSkill::tr("None");
        room->setPlayerProperty(player, "gain", QString("%1,%2,%3,%4").arg(exp).arg(wen).arg(wu).arg(_finished_achieve));
        int uid = player->userId();
        if (uid == -1)
            return;
        updatePlayerData(uid, exp, wen, wu);
        QStringList line;
        line << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
        line << Sanguosha->translate(player->getGeneralName());
        line << Sanguosha->getModeName(room->getMode());
        line << Sanguosha->translate(role);
        line << QString::number(player->getMark("Global_TurnCount"));
        line << (is_alive ? AchieveSkill::tr("Alive") : AchieveSkill::tr("Dead"));
        line << (is_escape ? AchieveSkill::tr("Escape")
                           : (is_draw ? AchieveSkill::tr("Standoff")
                                      : (is_win ? AchieveSkill::tr("Victory") : AchieveSkill::tr("Failure"))));
        line << QString::number(exp);
        line << QString::number(wen);
        line << QString::number(wu);
        line << translated_achieve.join(" ");
        QString location = QString(ACCOUNT "%1_records.csv").arg(uid);
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

    void updatePlayerData(int uid, int exp, int wen, int wu) const
    {
        QString location = QString(ACCOUNT "accounts.csv");
        QFile file(location);
        if (!file.open(QIODevice::ReadWrite))
            return;
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        QStringList lines = stream.readAll().split("\n", QString::SkipEmptyParts);
        stream.seek(0);
        foreach (QString line, lines) {
            QStringList _line = line.split(",");
            if (_line.length() != 6) {
                stream << line << "\n";
            } else if (_line[0] == QString::number(uid)) {
                _line[3] = QString::number(_line[3].toInt() + exp);
                _line[4] = QString::number(_line[4].toInt() + wen);
                _line[5] = QString::number(_line[5].toInt() + wu);
                stream << _line.join(",") << "\n";
            } else
                stream << line << "\n";
        }
        stream.flush();
        file.close();
    }
};

class HFLY : public AchieveSkill
{
public:
    HFLY()
        : AchieveSkill("hfly")
    {
        events << PreDamageDone << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            QVariant value = room->getAchievementData(player, key);
            QVariantList v_list = value.toList();
            QMap<QString, QStringList> map;
            for (int i = 0; i < v_list.length(); ++i) {
                QString s = v_list[i].toString();
                ++i;
                map[s] = v_list[i].toStringList();
            }
            if (!map.value(use.card->toString(), QStringList()).isEmpty())
                map.remove(use.card->toString());
            v_list.clear();
            foreach (QString _key, map.keys()) {
                v_list << _key;
                v_list << QVariant::fromValue(map[_key]);
            }
            room->setAchievementData(player, key, QVariant::fromValue(v_list));
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.card && damage.card->isKindOf("BurningCamps")) {
                QVariant value = room->getAchievementData(damage.from, key);
                QVariantList v_list = value.toList();
                QMap<QString, QStringList> map;
                for (int i = 0; i < v_list.length(); ++i) {
                    QString s = v_list[i].toString();
                    ++i;
                    map[s] = v_list[i].toStringList();
                }
                if (map.value(damage.card->toString(), QStringList()).isEmpty()) {
                    map[damage.card->toString()] = QStringList();
                    map[damage.card->toString()] << player->objectName();
                } else {
                    if (!map[damage.card->toString()].contains(player->objectName()))
                        map[damage.card->toString()] << player->objectName();
                }
                v_list.clear();
                foreach (QString _key, map.keys()) {
                    v_list << _key;
                    v_list << QVariant::fromValue(map[_key]);
                }
                room->setAchievementData(damage.from, key, QVariant::fromValue(v_list));
                if (map[damage.card->toString()].length() == 6)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        gainAchievement(damage.from, room);
        return false;
    }
};

class XQWBJFY : public AchieveSkill
{
public:
    XQWBJFY()
        : AchieveSkill("xqwbjfy")
    {
        events << ChoiceMade << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Drowning"))
                room->setAchievementData(player, key, 0);
        } else {
            QStringList args = data.toString().split(":");
            if (args[0] == "cardChosen") {
                if (args[1] == "drowning")
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        int value = room->getAchievementData(player, key).toInt();
        ++value;
        room->setAchievementData(player, key, value);
        if (value == 8)
            gainAchievement(player, room);
        return false;
    }
};

class YLHS : public AchieveSkill
{
public:
    YLHS()
        : AchieveSkill("ylhs")
    {
        events << ChoiceMade;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QStringList args = data.toString().split(":");
        if (args[0] == "skillInvoke") {
            if (args[1] == "breastplate" && args[2] == "yes") {
                QVariant _data = room->getAchievementData(player, key);
                DamageStruct damage = _data.value<DamageStruct>();
                if (damage.damage > 1)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        gainAchievement(player, room);
        return false;
    }
};

class SSHAHX : public AchieveSkill
{
public:
    SSHAHX()
        : AchieveSkill("sshahx")
    {
        events << ChoiceMade;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QStringList args = data.toString().split(":");
        if (args[0] == "Nullification") {
            if (args[1] == "LureTiger" || args[1] == "KnownBoth") {
                QVariant _data = room->getAchievementData(player, key);
                QStringList list = _data.toStringList();
                if (!list.contains(args[1]))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QStringList args = data.toString().split(":");
        QVariant _data = room->getAchievementData(player, key);
        QStringList list = _data.toStringList();
        list << args[1];
        room->setAchievementData(player, key, QVariant::fromValue(list));
        if (list.length() == 2)
            gainAchievement(player, room);
        return false;
    }
};

class MDZM : public AchieveSkill
{
public:
    MDZM()
        : AchieveSkill("mdzm")
    {
        events << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardResponseStruct resp = data.value<CardResponseStruct>();
        if (resp.m_card->isKindOf("Jink") && resp.m_isUse && resp.m_card->getSkillName() == "ibuki_gourd")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key);
        if (room->getAchievementData(player, key).toInt() == 2)
            gainAchievement(player, room);
        return false;
    }
};

class NZDL : public AchieveSkill
{
public:
    NZDL()
        : AchieveSkill("nzdl")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to == player && move.reason.m_skillName == "maid_suit")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key);
        if (room->getAchievementData(player, key).toInt() == 2)
            gainAchievement(player, room);
        return false;
    }
};

class DBDJ : public AchieveSkill
{
public:
    DBDJ()
        : AchieveSkill("dbdj")
    {
        events << ChoiceMade;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        QStringList args = data.toString().split(":");
        if (args[0] == "AGChosen" && args[1] == "amazing_grace") {
            int id = args[2].toInt();
            if (id != -1 && Sanguosha->getEngineCard(id)->isKindOf("Peach"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key);
        if (room->getAchievementData(player, key).toInt() == 2)
            gainAchievement(player, room);
        return false;
    }
};

class DJSB : public AchieveSkill
{
public:
    DJSB()
        : AchieveSkill("djsb")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player) {
            DamageStruct *damage = death.damage;
            if (damage && damage->card && damage->card->isKindOf("Lightning"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        gainAchievement(player, room);
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage && death.damage->card && death.damage->card->isKindOf("Lightning"))
            gainAchievement(player, room);
    }
};

class TSQB : public AchieveSkill
{
public:
    TSQB()
        : AchieveSkill("tsqb")
    {
        events << CardUsed << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setAchievementData(p, key, 0);
            }
            return QStringList();
        }
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key, 1);
        if (room->getAchievementData(player, key).toInt() == 4)
            gainAchievement(player, room);
        return false;
    }
};

class GYDDW : public AchieveSkill
{
public:
    GYDDW()
        : AchieveSkill("gyddw")
    {
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        QStringList winners = room->getWinner(player).split("+");
        if (!winners.isEmpty()) {
            ServerPlayer *lord = room->getLord();
            if (lord && (winners.contains(lord->objectName()) || winners.contains("lord")))
                onWinOrLose(room, lord, true);
        }
    }

    virtual void onWinOrLose(Room *room, ServerPlayer *player, bool is_win) const
    {
        if (is_win && player && player == room->getLord()) {
            room->addAchievementData(player, key, 1, false);
            if (room->getAchievementData(player, key, false, false) == 10)
                gainAchievement(player, room);
        }
    }
};

class JJDDW : public AchieveSkill
{
public:
    JJDDW()
        : AchieveSkill("jjddw")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player) {
            if (death.who->getRole() == "royalist")
                return QStringList();
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getRole() == "royalist")
                    return QStringList();
            }
            if (death.damage && death.damage->from && death.damage->from == room->getLord())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(room->getLord(), key);
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        QStringList winners = room->getWinner(player).split("+");
        if (!winners.isEmpty()) {
            ServerPlayer *lord = room->getLord();
            if (lord && (winners.contains(lord->objectName()) || winners.contains("lord")))
                onWinOrLose(room, lord, true);
        }
    }

    virtual void onWinOrLose(Room *room, ServerPlayer *player, bool is_win) const
    {
        if (is_win && player && player == room->getLord() && room->getAchievementData(player, key).toInt() >= 2)
            gainAchievement(player, room);
    }
};

class YDSPZ : public AchieveSkill
{
public:
    YDSPZ()
        : AchieveSkill("ydspz")
    {
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        QStringList winners = room->getWinner(player).split("+");
        if (!winners.isEmpty()) {
            foreach (ServerPlayer *loyalist, room->getPlayers())
                if (loyalist->getRole() == "loyalist" && (winners.contains(player->objectName()) || winners.contains("lord")))
                    onWinOrLose(room, loyalist, true);
        }
    }

    virtual void onWinOrLose(Room *room, ServerPlayer *player, bool is_win) const
    {
        if (is_win && player && player->getRole() == "loyalist") {
            QStringList killed_roles = room->getAchievementData(player, "killed_roles").toStringList();
            QStringList mode_lists = Sanguosha->getRoleList(player->getGameMode());
            if (killed_roles.count("rebel") == mode_lists.count("rebel")
                && killed_roles.count("renegade") == mode_lists.count("renegade"))
                gainAchievement(player, room);
        }
    }
};

class DMHB : public AchieveSkill
{
public:
    DMHB()
        : AchieveSkill("dmhb")
    {
        events << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardResponseStruct resp = data.value<CardResponseStruct>();
        if (resp.m_card->isKindOf("Jink"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key);
        if (room->getAchievementData(player, key) == 10)
            gainAchievement(player, room);
        return false;
    }
};

class FSLZ : public AchieveSkill
{
public:
    FSLZ()
        : AchieveSkill("fslz")
    {
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        QStringList winners = room->getWinner(player).split(":");
        foreach (ServerPlayer *p, room->getPlayers()) {
            bool is_win = winners.contains(p->objectName()) || winners.contains(p->getRole());
            onWinOrLose(room, p, is_win);
        }
    }

    virtual void onWinOrLose(Room *room, ServerPlayer *player, bool is_win) const
    {
        if (is_win) {
            bool update = false;
            QStringList lists = room->getAchievementData(player, key, false).toString().split("\n", QString::SkipEmptyParts);
            QString role = player->getRole();
            if (!lists.contains(role)) {
                lists << role;
                room->setAchievementData(player, key, role, false);
                room->addAchievementData(player, key, 1, false);
                update = true;
            }
            QString kingdom = player->getKingdom();
            if (!lists.contains(kingdom)) {
                lists << kingdom;
                room->setAchievementData(player, key, kingdom, false);
                room->addAchievementData(player, key, 1, false);
                update = true;
            }
            if (update && room->getAchievementData(player, key, false, false).toInt() == 8)
                gainAchievement(player, room);
        }
    }
};

class CDSC : public AchieveSkill
{
public:
    CDSC()
        : AchieveSkill("cdsc")
    {
        events << PreDamageDone;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->addAchievementData(damage.from, key, damage.damage);
        if (room->getAchievementData(damage.from, key).toInt() >= 20)
            gainAchievement(damage.from, room);
        return false;
    }
};

class GLGJSSY : public AchieveSkill
{
public:
    GLGJSSY()
        : AchieveSkill("glgjssy")
    {
        events << QuitDying;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target->isAlive();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key);
        if (room->getAchievementData(player, key).toInt() == 5)
            gainAchievement(player, room);
        return false;
    }
};

class ZCYX : public AchieveSkill
{
public:
    ZCYX()
        : AchieveSkill("zcyx")
    {
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Peach")) {
            ServerPlayer *dying = room->getCurrentDyingPlayer();
            if (dying && dying != player && use.to.contains(dying))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key);
        if (room->getAchievementData(player, key).toInt() == 5)
            gainAchievement(player, room);
        return false;
    }
};

class ZJDFZ : public AchieveSkill
{
public:
    ZJDFZ()
        : AchieveSkill("zjdfz")
    {
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        QStringList winners = room->getWinner(player).split(":");
        foreach (ServerPlayer *p, room->getPlayers()) {
            bool is_win = winners.contains(p->objectName()) || winners.contains(p->getRole());
            if (is_win && p->getRole() == "renegade")
                onWinOrLose(room, p, true);
        }
    }

    virtual void onWinOrLose(Room *room, ServerPlayer *player, bool is_win) const
    {
        if (is_win && player->getRole() == "renegade") {
            ServerPlayer *lord = room->getLord();
            if (lord) {
                QStringList list = room->getAchievementData(lord, "killed_roles").toStringList();
                if (list.contains("loyalist"))
                    gainAchievement(player, room);
            }
        }
    }
};

class ZLPCCZ : public AchieveSkill
{
public:
    ZLPCCZ()
        : AchieveSkill("zlpccz")
    {
        events << CardUsed << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setAchievementData(p, key, 0);
            }
            return QStringList();
        }
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("EquipCard"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key, 1);
        if (room->getAchievementData(player, key).toInt() == 5)
            gainAchievement(player, room);
        return false;
    }
};

class GSTY : public AchieveSkill
{
public:
    GSTY()
        : AchieveSkill("gsty")
    {
        events << PreDamageDone;
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->addAchievementData(player, key, damage.damage);
        if (damage.from)
            room->addAchievementData(damage.from, key, damage.damage);
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        QStringList winners = room->getWinner(player).split("+");
        foreach (ServerPlayer *p, room->getPlayers()) {
            if (winners.contains(p->objectName()) || winners.contains(p->getRole()))
                onWinOrLose(room, p, true);
        }
    }

    virtual void onWinOrLose(Room *room, ServerPlayer *player, bool is_win) const
    {
        if (is_win && room->getAchievementData(player, key).toInt() == 0)
            gainAchievement(player, room);
    }
};

class WHKS : public AchieveSkill
{
public:
    WHKS()
        : AchieveSkill("whks")
    {
        events << PreDamageDone;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Lightning"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key, 1, false);
        if (room->getAchievementData(player, key, false, false).toInt() == 10)
            gainAchievement(player, room);
        return false;
    }
};

class ALHAKB : public AchieveSkill
{
public:
    ALHAKB()
        : AchieveSkill("alhakb")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player && room->getCurrent() == player && player->getPhase() != Player::NotActive)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        gainAchievement(player, room);
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player && player == room->getCurrent() && player->getPhase() != Player::NotActive)
            gainAchievement(player, room);
    }
};

class DJYD : public AchieveSkill
{
public:
    DJYD()
        : AchieveSkill("djyd")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player && player->getMark("Global_TurnCount") == 0)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        gainAchievement(player, room);
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player && player->getMark("Global_TurnCount") == 0)
            gainAchievement(player, room);
    }
};

class RMSH : public AchieveSkill
{
public:
    RMSH()
        : AchieveSkill("rmsh")
    {
        events << GameStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&ask_who) const
    {
        if (player == NULL) {
            foreach (ServerPlayer *p, room->getPlayers()) {
                if (p->isMale())
                    return QStringList();
            }
            ask_who = room->getLord();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        foreach (ServerPlayer *p, room->getPlayers())
            gainAchievement(p, room);
        return false;
    }
};

class YYWM : public AchieveSkill
{
public:
    YYWM()
        : AchieveSkill("yywm")
    {
        events << PreDamageDone;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Duel") && !damage.by_user)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key, 1, false);
        if (room->getAchievementData(player, key, false, false).toInt() == 10)
            gainAchievement(player, room);
        return false;
    }
};

class NWSSSZQ : public AchieveSkill
{
public:
    NWSSSZQ()
        : AchieveSkill("nwssszq")
    {
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &) const
    {
        QStringList winners = room->getWinner(player).split(":");
        foreach (ServerPlayer *p, room->getPlayers()) {
            bool is_win = winners.contains(p->objectName()) || winners.contains(p->getRole());
            onWinOrLose(room, p, is_win);
        }
    }

    virtual void onWinOrLose(Room *room, ServerPlayer *player, bool is_win) const
    {
        if (is_win) {
            room->addAchievementData(player, key, 1, false);
            if (room->getAchievementData(player, key, false, false).toInt() == 10)
                gainAchievement(player, room);
        } else {
            int n = room->getAchievementData(player, key, false, false).toInt();
            room->addAchievementData(player, key, -n, false);
        }
    }
};

class LZZX : public AchieveSkill
{
public:
    LZZX()
        : AchieveSkill("lzzx")
    {
        events << ChoiceMade;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        QStringList args = data.toString().split(":");
        if (args[0] == "cardResponded" && args[2].startsWith("@fire-attack"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QStringList args = data.toString().split(":");
        if (args.last() == "_nil_") {
            room->addAchievementData(player, key);
            if (room->getAchievementData(player, key).toInt() == 3)
                gainAchievement(player, room);
        } else
            room->setAchievementData(player, key, 0);
        return false;
    }
};

class MYDNL : public AchieveSkill
{
public:
    MYDNL()
        : AchieveSkill("mydnl")
    {
        events << FinishJudge << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Judge)
                room->setAchievementData(player, key, 0);
            else if (change.from == Player::Judge) {
                if (room->getAchievementData(player, key).toInt() >= 2)
                    gainAchievement(player, room);
            }
        } else {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (player->getPhase() == Player::Judge && (judge->reason == "indulgence" || judge->reason == "supply_shortage"
                                                        || judge->reason == "lightning" || judge->reason == "purple_song"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->isBad())
            room->addAchievementData(player, key);
        else
            room->setAchievementData(player, key, -998);
        return false;
    }
};

class JDYZL : public AchieveSkill
{
public:
    JDYZL()
        : AchieveSkill("jdyzl")
    {
        events << PreDamageDone;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->addAchievementData(player, key, damage.damage);
        if (room->getAchievementData(player, key).toInt() >= 5)
            gainAchievement(player, room);
        return false;
    }
};

class SHWDDY : public AchieveSkill
{
public:
    SHWDDY()
        : AchieveSkill("shwddy")
    {
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("JadeCard") && use.to.length() >= 3)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        gainAchievement(player, room);
        return false;
    }
};

class TJWDDR : public AchieveSkill
{
public:
    TJWDDR()
        : AchieveSkill("tjwddr")
    {
        events << Death << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            QVariant value = room->getAchievementData(player, key);
            QVariantList v_list = value.toList();
            QMap<QString, QStringList> map;
            for (int i = 0; i < v_list.length(); ++i) {
                QString s = v_list[i].toString();
                ++i;
                map[s] = v_list[i].toStringList();
            }
            if (!map.value(use.card->toString(), QStringList()).isEmpty())
                map.remove(use.card->toString());
            v_list.clear();
            foreach (QString _key, map.keys()) {
                v_list << _key;
                v_list << QVariant::fromValue(map[_key]);
            }
            room->setAchievementData(player, key, QVariant::fromValue(v_list));
        } else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player && death.damage) {
                DamageStruct *damage = death.damage;
                if (damage->card && damage->card->getTypeId() != Card::TypeSkill && damage->from) {
                    QVariant value = room->getAchievementData(damage->from, key);
                    QVariantList v_list = value.toList();
                    QMap<QString, QStringList> map;
                    for (int i = 0; i < v_list.length(); ++i) {
                        QString s = v_list[i].toString();
                        ++i;
                        map[s] = v_list[i].toStringList();
                    }
                    if (map.value(damage->card->toString(), QStringList()).isEmpty()) {
                        map[damage->card->toString()] = QStringList();
                        map[damage->card->toString()] << player->objectName();
                    } else {
                        if (!map[damage->card->toString()].contains(player->objectName()))
                            map[damage->card->toString()] << player->objectName();
                    }
                    v_list.clear();
                    foreach (QString _key, map.keys()) {
                        v_list << _key;
                        v_list << QVariant::fromValue(map[_key]);
                    }
                    room->setAchievementData(damage->from, key, QVariant::fromValue(v_list));
                    if (map[damage->card->toString()].length() == 3)
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        DeathStruct death = data.value<DeathStruct>();
        gainAchievement(death.damage->from, room);
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        DamageStruct *damage = death.damage;
        if (damage && damage->card && damage->card->getTypeId() != Card::TypeSkill && damage->from) {
            QVariant value = room->getAchievementData(damage->from, key);
            QVariantList v_list = value.toList();
            QMap<QString, QStringList> map;
            for (int i = 0; i < v_list.length(); ++i) {
                QString s = v_list[i].toString();
                ++i;
                map[s] = v_list[i].toStringList();
            }
            if (map.value(damage->card->toString(), QStringList()).isEmpty()) {
                map[damage->card->toString()] = QStringList();
                map[damage->card->toString()] << player->objectName();
            } else {
                if (!map[damage->card->toString()].contains(player->objectName()))
                    map[damage->card->toString()] << player->objectName();
            }
            v_list.clear();
            foreach (QString _key, map.keys()) {
                v_list << _key;
                v_list << QVariant::fromValue(map[_key]);
            }
            room->setAchievementData(damage->from, key, QVariant::fromValue(v_list));
            if (map[damage->card->toString()].length() == 3)
                gainAchievement(damage->from, room);
        }
    }
};

class BWSDKLR : public AchieveSkill
{
public:
    BWSDKLR()
        : AchieveSkill("bwsdklr")
    {
        events << CardFinished << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill) {
                foreach (ServerPlayer *to, use.to)
                    room->setAchievementData(to, key, -1);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            room->addAchievementData(p, key);
            if (room->getAchievementData(p, key).toInt() == 5)
                gainAchievement(p, room);
        }
        return false;
    }
};

class JZTTXDY : public AchieveSkill
{
public:
    JZTTXDY()
        : AchieveSkill("jzttxdy")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to == player && move.to_place == Player::PlaceEquip && player->getEquips().length() == 5)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        int n = 0;
        foreach (const Card *c, player->getEquips()) {
            if (c->getSuit() == Card::Spade)
                ++n;
        }
        if (n == 5)
            gainAchievement(player, room);
        return false;
    }
};

class NDJSWD : public AchieveSkill
{
public:
    NDJSWD()
        : AchieveSkill("ndjswd")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    foreach (ServerPlayer *other, room->getOtherPlayers(p)) {
                        QString _key = QString("%1_%2").arg(key).arg(other->objectName());
                        room->setAchievementData(p, _key, 0);
                    }
                }
            }
            return QStringList();
        }
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from && move.to && move.to == player && move.to_place == Player::PlaceHand
            && (move.from_places.contains(Player::PlaceDelayedTrick) || move.from_places.contains(Player::PlaceEquip)
                || move.from_places.contains(Player::PlaceHand))) {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QString _key = QString("%1_%2").arg(key).arg(move.from->objectName());
        foreach (Player::Place place, move.from_places) {
            if (place == Player::PlaceDelayedTrick || place == Player::PlaceEquip || place == Player::PlaceHand) {
                room->addAchievementData(player, _key);
                if (move.from->isAllNude() && room->getAchievementData(player, _key).toInt() >= 3)
                    gainAchievement(player, room);
            }
        }
        return false;
    }
};

class PDDDFL : public AchieveSkill
{
public:
    PDDDFL()
        : AchieveSkill("pdddfl")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player && death.damage && death.damage->reason == "moon_spear" && death.damage->from)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        DeathStruct death = data.value<DeathStruct>();
        gainAchievement(death.damage->from, room);
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage && death.damage->reason == "moon_spear" && death.damage->from)
            gainAchievement(death.damage->from, room);
    }
};

class FHFDFGJ : public AchieveSkill
{
public:
    FHFDFGJ()
        : AchieveSkill("fhfdfgj")
    {
        events << ChoiceMade;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        QStringList args = data.toString().split(":");
        if (args[0] == "Nullification") {
            if (args[1] == "Snatch" || args[1] == "FireAttack" || args[1] == "BurningCamps")
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QStringList args = data.toString().split(":");
        QVariant _data = room->getAchievementData(player, key, false);
        QStringList list = _data.toString().split("\n", QString::SkipEmptyParts);
        QMap<QString, int> times_map;
        foreach (QString line, list) {
            QStringList temp = line.split(",");
            times_map[temp[0]] = temp[1].toInt();
        }
        if (times_map.isEmpty()) {
            times_map["Steal"] = 0;
            times_map["Fire"] = 0;
        } else if (times_map["Steal"] == 10 && times_map["Fire"] == 10)
            return false;
        if (args[1] == "Snatch" && times_map["Steal"] < 10) {
            ++times_map["Steal"];
        } else if ((args[1] == "FireAttack" || args[1] == "BurningCamps") && times_map["Fire"] < 10) {
            ++times_map["Fire"];
        }
        int n = room->getAchievementData(player, key, false, false).toInt();
        if (times_map["Steal"] + times_map["Fire"] != n) {
            n = times_map["Steal"] + times_map["Fire"] - n;
            room->addAchievementData(player, key, n, false);
        }
        list.clear();
        foreach (QString name, times_map.keys())
            list << QString("%1,%2").arg(name).arg(times_map[name]);
        room->setAchievementData(player, key, QVariant::fromValue(list), false, FullFile);
        return false;
    }
};

class SSJNYSQ : public AchieveSkill
{
public:
    SSJNYSQ()
        : AchieveSkill("ssjnysq")
    {
        events << EventMarksGot;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (data.toString() == "Global_TurnCount" && player->getMark("Global_TurnCount") == 17) {
            QStringList shahu = Sanguosha->translate(key).split(",");
            foreach (QString s, shahu) {
                if (Sanguosha->translate(player->getGeneralName()).contains(s))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        gainAchievement(player, room);
        return false;
    }
};

class SZBNQB : public AchieveSkill
{
public:
    SZBNQB()
        : AchieveSkill("szbnqb")
    {
        events << CardUsed << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    foreach (ServerPlayer *p2, room->getAlivePlayers())
                        room->setAchievementData(p, key + "_" + p2->objectName(), 0);
                }
            }
            return QStringList();
        }
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Rout"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *to, use.to) {
            QString s("%1_%2");
            s.arg(key).arg(to->objectName());
            room->addAchievementData(player, s);
            if (room->getAchievementData(player, s).toInt() == 2)
                gainAchievement(player, room);
        }
        return false;
    }
};

class HXXLYHJH : public AchieveSkill
{
public:
    HXXLYHJH()
        : AchieveSkill("hxxlyhjh")
    {
        events << GameStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&ask_who) const
    {
        if (player == NULL) {
            ask_who = room->getLord();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        int sum = 0;
        foreach (const General *g, Sanguosha->getGeneralList()) {
            if (g->getPackage().startsWith("touhou"))
                ++sum;
        }
        foreach (ServerPlayer *player, room->getPlayers()) {
            if (!player->getGeneral()->getPackage().startsWith("touhou"))
                continue;
            QVariant data = room->getAchievementData(player, key, false);
            QStringList list = data.toString().split("\n", QString::SkipEmptyParts);
            if (list.contains(player->getGeneralName()))
                return false;
            room->setAchievementData(player, key, player->getGeneralName(), false);
            room->addAchievementData(player, key, 1, false);
            if (list.length() + 1 == sum)
                gainAchievement(player, room);
        }
        return false;
    }
};

class WYZSWZH : public AchieveSkill
{
public:
    WYZSWZH()
        : AchieveSkill("wyzswzh")
    {
        events << CardsMoveOneTime << PreDamageDone << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent te, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&aw) const
    {
        if (te == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setAchievementData(p, key, 0);
            }
            return QStringList();
        } else if (te == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature == DamageStruct::Thunder && damage.from)
                return QStringList(objectName());
            return QStringList();
        }
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player == move.to && move.reason.m_reason == CardMoveReason::S_REASON_RETRIAL) {
            JudgeStruct *judge = move.reason.m_extraData.value<JudgeStruct *>();
            if (judge && judge->reason == "lightning") {
                const Card *card = Sanguosha->getCard(move.card_ids.first());
                if (card->getSuit() == Card::Spade && card->getNumber() >= 2 && card->getNumber() <= 10) {
                    aw = room->findPlayer(move.reason.m_playerId);
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent te, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        if (te == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            room->addAchievementData(damage.from, key, damage.damage);
            if (room->getAchievementData(damage.from, key).toInt() >= 3)
                gainAchievement(damage.from, room);
        } else
            gainAchievement(ask_who, room);
        return false;
    }
};

class NJDZJCGDSPMBM : public AchieveSkill
{
public:
    NJDZJCGDSPMBM()
        : AchieveSkill("njdzjcgdspmbm")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player && death.damage) {
            DamageStruct *damage = death.damage;
            if (damage && damage->from)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *player = death.damage->from;
        room->addAchievementData(player, key, 1, false);
        if (room->getAchievementData(player, key, false, false).toInt() == 100)
            gainAchievement(player, room);
        return false;
    }

    virtual void onGameOver(Room *room, ServerPlayer *, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        DamageStruct *damage = death.damage;
        if (damage && damage->from) {
            ServerPlayer *player = death.damage->from;
            room->addAchievementData(player, key, 1, false);
            if (room->getAchievementData(player, key, false, false).toInt() == 100)
                gainAchievement(player, room);
        }
    }
};

class WHHQ : public AchieveSkill
{
public:
    WHHQ()
        : AchieveSkill("whhq")
    {
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("KnownBoth")) {
            QStringList list = room->getAchievementData(player, key).toStringList();
            if (list.length() < 3) {
                foreach (ServerPlayer *to, use.to) {
                    if (list.contains(to->objectName()))
                        continue;
                    list << to->objectName();
                }
                room->setAchievementData(player, key, QVariant::fromValue(list));
                if (list.length() >= 3)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        gainAchievement(player, room);
        return false;
    }
};

class HYLDZZZD : public AchieveSkill
{
public:
    HYLDZZZD()
        : AchieveSkill("hyldzzzd")
    {
        events << GameStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&ask_who) const
    {
        if (player == NULL) {
            if (room->getMode() == "02_1v1") {
                ask_who = room->getLord();
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        foreach (ServerPlayer *player, room->getPlayers())
            gainAchievement(player, room);
        return false;
    }
};

class ZSYB : public AchieveSkill
{
public:
    ZSYB()
        : AchieveSkill("zsyb")
    {
        events << GameStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&ask_who) const
    {
        if (player == NULL) {
            if (room->getMode() == "chunxue") {
                ask_who = room->getLord();
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        foreach (ServerPlayer *player, room->getPlayers())
            gainAchievement(player, room);
        return false;
    }
};

AchievementPackage::AchievementPackage()
    : Package("achievement", SpecialPack)
{
    skills << new AchievementMain << new WenGongWuGong << new AchievementRecord;
    skills << new HFLY << new XQWBJFY << new YLHS << new SSHAHX << new MDZM << new NZDL << new DBDJ << new DJSB << new TSQB
           << new GYDDW << new JJDDW << new YDSPZ << new DMHB << new FSLZ << new CDSC << new GLGJSSY << new ZCYX << new ZJDFZ
           << new ZLPCCZ << new GSTY << new WHKS << new ALHAKB << new DJYD << new RMSH << new YYWM << new NWSSSZQ << new LZZX
           << new MYDNL << new JDYZL << new SHWDDY << new TJWDDR << new BWSDKLR << new JZTTXDY << new NDJSWD
           << new AchieveSkill("pmdx") << new PDDDFL << new FHFDFGJ << new SSJNYSQ << new SZBNQB << new HXXLYHJH << new WYZSWZH
           << new NJDZJCGDSPMBM << new WHHQ << new HYLDZZZD << new ZSYB;
}

ADD_PACKAGE(Achievement)
