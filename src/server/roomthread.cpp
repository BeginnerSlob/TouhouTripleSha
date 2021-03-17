#include "roomthread.h"
#include "ai.h"
#include "engine.h"
#include "gamerule.h"
#include "room.h"
#include "scenario.h"
#include "settings.h"
#include "standard.h"

#include "json.h"
#include <QTime>

#ifdef QSAN_UI_LIBRARY_AVAILABLE
#pragma message WARN("UI elements detected in server side!!!")
#endif

using namespace JsonUtils;

LogMessage::LogMessage()
    : from(NULL)
{
}

QVariant LogMessage::toVariant() const
{
    QStringList tos;
    foreach (ServerPlayer *player, to)
        if (player != NULL)
            tos << player->objectName();

    QStringList log;
    log << type << (from ? from->objectName() : "") << tos.join("+") << card_str << arg << arg2;
    QVariant json_log = JsonUtils::toJsonArray(log);
    return json_log;
}

QString EventTriplet::toString() const
{
    return QString("event[%1], room[%2], target = %3[%4]\n")
        .arg(_m_event)
        .arg(_m_room->getId())
        .arg(_m_target ? _m_target->objectName() : "NULL")
        .arg(_m_target ? _m_target->getGeneralName() : QString());
}

RoomThread::RoomThread(Room *room)
    : room(room)
{
}

void RoomThread::addPlayerSkills(ServerPlayer *player, bool invoke_game_start)
{
    QVariant void_data;
    bool invoke_verify = false;

    foreach (const TriggerSkill *skill, player->getTriggerSkills()) {
        addTriggerSkill(skill);

        if (invoke_game_start && skill->getTriggerEvents().contains(GameStart))
            invoke_verify = true;
    }

    //We should make someone trigger a whole GameStart event instead of trigger a skill only.
    if (invoke_verify)
        trigger(GameStart, room, player, void_data);
}

void RoomThread::constructTriggerTable()
{
    foreach (ServerPlayer *player, room->getPlayers())
        addPlayerSkills(player, true);
}

ServerPlayer *RoomThread::find3v3Next(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second)
{
    bool all_actioned = true;
    foreach (ServerPlayer *player, room->m_alivePlayers) {
        if (!player->hasFlag("actioned")) {
            all_actioned = false;
            break;
        }
    }

    if (all_actioned) {
        foreach (ServerPlayer *player, room->m_alivePlayers) {
            room->setPlayerFlag(player, "-actioned");
            trigger(ActionedReset, room, player);
        }

        qSwap(first, second);
        QList<ServerPlayer *> first_alive;
        foreach (ServerPlayer *p, first) {
            if (p->isAlive())
                first_alive << p;
        }
        return room->askForPlayerChosen(first.first(), first_alive, "3v3-action", "@3v3-action");
    }

    ServerPlayer *current = room->getCurrent();
    if (current != first.first()) {
        ServerPlayer *another = NULL;
        if (current == first.last())
            another = first.at(1);
        else
            another = first.last();
        if (!another->hasFlag("actioned") && another->isAlive())
            return another;
    }

    QList<ServerPlayer *> targets;
    do {
        targets.clear();
        qSwap(first, second);
        foreach (ServerPlayer *player, first) {
            if (!player->hasFlag("actioned") && player->isAlive())
                targets << player;
        }
    } while (targets.isEmpty());

    return room->askForPlayerChosen(first.first(), targets, "3v3-action", "@3v3-action");
}

void RoomThread::run3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule, ServerPlayer *current)
{
    try {
        forever {
            room->setCurrent(current);
            trigger(TurnStart, room, room->getCurrent());
            room->setPlayerFlag(current, "actioned");
            current = find3v3Next(first, second);
        }
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBroken3v3(first, second, game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBroken3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule)
{
    try {
        ServerPlayer *player = room->getCurrent();
        trigger(TurnBroken, room, player);
        if (player->getPhase() != Player::NotActive) {
            QVariant empty_data;
            game_rule->trigger(EventPhaseEnd, room, player, empty_data);
            player->changePhase(player->getPhase(), Player::NotActive);
        }
        if (!player->hasFlag("actioned"))
            room->setPlayerFlag(player, "actioned");

        ServerPlayer *next = find3v3Next(first, second);
        run3v3(first, second, game_rule, next);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken) {
            _handleTurnBroken3v3(first, second, game_rule);
        } else {
            throw triggerEvent;
        }
    }
}

ServerPlayer *RoomThread::findHulaoPassNext(ServerPlayer *shenlvbu, QList<ServerPlayer *> league, int stage)
{
    ServerPlayer *current = room->getCurrent();
    if (stage == 1) {
        if (current == shenlvbu) {
            foreach (ServerPlayer *p, league) {
                if (p->isAlive() && !p->hasFlag("actioned"))
                    return p;
            }
            foreach (ServerPlayer *p, league) {
                if (p->isAlive())
                    return p;
            }
            Q_ASSERT(false);
            return league.first();
        } else {
            return shenlvbu;
        }
    } else {
        Q_ASSERT(stage == 2);
        return room->findPlayer(current->getNextAlive(1, false)->objectName());
    }
}

void RoomThread::actionHulaoPass(ServerPlayer *shenlvbu, QList<ServerPlayer *> league, GameRule *game_rule, int stage)
{
    try {
        if (stage == 1) {
            forever {
                ServerPlayer *current = room->getCurrent();
                trigger(TurnStart, room, current);

                if (current != shenlvbu) {
                    if (current->isAlive() && !current->hasFlag("actioned"))
                        room->setPlayerFlag(current, "actioned");
                } else {
                    bool all_actioned = true;
                    foreach (ServerPlayer *player, league) {
                        if (player->isAlive() && !player->hasFlag("actioned")) {
                            all_actioned = false;
                            break;
                        }
                    }
                    if (all_actioned) {
                        foreach (ServerPlayer *player, league) {
                            if (player->hasFlag("actioned"))
                                room->setPlayerFlag(player, "-actioned");
                        }
                        foreach (ServerPlayer *player, league) {
                            if (player->isDead())
                                trigger(TurnStart, room, player);
                        }
                    }
                }

                ServerPlayer *regular_next = findHulaoPassNext(shenlvbu, league, 1);
                while (!room->getTag("ExtraTurnList").isNull()) {
                    QStringList extraTurnList = room->getTag("ExtraTurnList").toStringList();
                    if (!extraTurnList.isEmpty()) {
                        QString extraTurnPlayer = extraTurnList.takeFirst();
                        room->setTag("ExtraTurnList", QVariant::fromValue(extraTurnList));
                        ServerPlayer *next = room->findPlayer(extraTurnPlayer);
                        room->setTag("NormalNext", QVariant::fromValue(regular_next));
                        room->setCurrent(next);
                        trigger(TurnStart, room, next);
                        room->removeTag("NormalNext");
                    } else
                        room->removeTag("ExtraTurnList");
                }

                room->setCurrent(regular_next);
            }
        } else {
            Q_ASSERT(stage == 2);
            forever {
                ServerPlayer *current = room->getCurrent();
                trigger(TurnStart, room, current);

                if (current == shenlvbu) {
                    foreach (ServerPlayer *player, league) {
                        if (player->isDead())
                            trigger(TurnStart, room, player);
                    }
                }

                ServerPlayer *regular_next = findHulaoPassNext(shenlvbu, league, 2);
                while (!room->getTag("ExtraTurnList").isNull()) {
                    QStringList extraTurnList = room->getTag("ExtraTurnList").toStringList();
                    if (!extraTurnList.isEmpty()) {
                        QString extraTurnPlayer = extraTurnList.takeFirst();
                        room->setTag("ExtraTurnList", QVariant::fromValue(extraTurnList));
                        ServerPlayer *next = room->findPlayer(extraTurnPlayer);
                        room->setTag("NormalNext", QVariant::fromValue(regular_next));
                        room->setCurrent(next);
                        trigger(TurnStart, room, next);
                        room->removeTag("NormalNext");
                    } else
                        room->removeTag("ExtraTurnList");
                }

                room->setCurrent(regular_next);
            }
        }
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == StageChange) {
            stage = 2;
            trigger(triggerEvent, room, NULL);
            room->removeTag("ExtraTurnList");
            room->removeTag("NormalNext");
            foreach (ServerPlayer *player, room->getPlayers()) {
                if (player != shenlvbu) {
                    if (player->hasFlag("actioned"))
                        room->setPlayerFlag(player, "-actioned");

                    if (player->getPhase() != Player::NotActive) {
                        QVariant empty_data;
                        game_rule->trigger(EventPhaseEnd, room, player, empty_data);
                        player->changePhase(player->getPhase(), Player::NotActive);
                    }
                }
            }

            room->setCurrent(shenlvbu);
            actionHulaoPass(shenlvbu, league, game_rule, 2);
        } else if (triggerEvent == TurnBroken) {
            _handleTurnBrokenHulaoPass(shenlvbu, league, game_rule, stage);
        } else {
            throw triggerEvent;
        }
    }
}

void RoomThread::_handleTurnBrokenHulaoPass(ServerPlayer *shenlvbu, QList<ServerPlayer *> league, GameRule *game_rule,
                                            int stage)
{
    try {
        ServerPlayer *player = room->getCurrent();
        trigger(TurnBroken, room, player);
        if (player->getPhase() != Player::NotActive) {
            QVariant empty_data;
            game_rule->trigger(EventPhaseEnd, room, player, empty_data);
            player->changePhase(player->getPhase(), Player::NotActive);
            if (player != shenlvbu && stage == 1)
                room->setPlayerFlag(player, "actioned");
        }

        ServerPlayer *regular_next = findHulaoPassNext(shenlvbu, league, stage);

        while (!room->getTag("ExtraTurnList").isNull()) {
            QStringList extraTurnList = room->getTag("ExtraTurnList").toStringList();
            if (!extraTurnList.isEmpty()) {
                QString extraTurnPlayer = extraTurnList.takeFirst();
                room->setTag("ExtraTurnList", QVariant::fromValue(extraTurnList));
                ServerPlayer *next = room->findPlayer(extraTurnPlayer);
                room->setTag("NormalNext", QVariant::fromValue(regular_next));
                room->setCurrent(next);
                trigger(TurnStart, room, next);
                room->removeTag("NormalNext");
            } else
                room->removeTag("ExtraTurnList");
        }

        room->setCurrent(regular_next);
        actionHulaoPass(shenlvbu, league, game_rule, stage);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenHulaoPass(shenlvbu, league, game_rule, stage);
        else
            throw triggerEvent;
    }
}

void RoomThread::actionNormal(GameRule *game_rule)
{
    try {
        forever {
            trigger(TurnStart, room, room->getCurrent());
            ServerPlayer *regular_next = qobject_cast<ServerPlayer *>(room->getCurrent()->getNextAlive(1, false));
            while (!room->getTag("ExtraTurnList").isNull()) {
                QStringList extraTurnList = room->getTag("ExtraTurnList").toStringList();
                if (!extraTurnList.isEmpty()) {
                    QString extraTurnPlayer = extraTurnList.takeFirst();
                    room->setTag("ExtraTurnList", QVariant::fromValue(extraTurnList));
                    ServerPlayer *next = room->findPlayer(extraTurnPlayer);
                    room->setTag("NormalNext", QVariant::fromValue(regular_next));
                    room->setCurrent(next);
                    trigger(TurnStart, room, next);
                    room->removeTag("NormalNext");
                } else
                    room->removeTag("ExtraTurnList");
            }
            room->setCurrent(regular_next);
        }
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenNormal(game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBrokenNormal(GameRule *game_rule)
{
    try {
        ServerPlayer *player = room->getCurrent();
        trigger(TurnBroken, room, player);

        if (player->getPhase() != Player::NotActive) {
            QVariant empty_data;
            game_rule->trigger(EventPhaseEnd, room, player, empty_data);
            player->changePhase(player->getPhase(), Player::NotActive);
        }

        ServerPlayer *regular_next = room->getTag("NormalNext").value<ServerPlayer *>();
        if (!regular_next)
            regular_next = qobject_cast<ServerPlayer *>(room->getCurrent()->getNextAlive(1, false));
        while (!room->getTag("ExtraTurnList").isNull()) {
            QStringList extraTurnList = room->getTag("ExtraTurnList").toStringList();
            if (!extraTurnList.isEmpty()) {
                QString extraTurnPlayer = extraTurnList.takeFirst();
                room->setTag("ExtraTurnList", QVariant::fromValue(extraTurnList));
                ServerPlayer *next = room->findPlayer(extraTurnPlayer);
                room->setTag("NormalNext", QVariant::fromValue(regular_next));
                room->setCurrent(next);
                trigger(TurnStart, room, next);
                room->removeTag("NormalNext");
            } else
                room->removeTag("ExtraTurnList");
        }
        room->setCurrent(regular_next);
        actionNormal(game_rule);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenNormal(game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::run()
{
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    Sanguosha->registerRoom(room);
    GameRule *game_rule;
    if (room->getMode() == "04_1v3")
        game_rule = new HulaoPassMode(this);
    else
        game_rule = new GameRule(this);

    addTriggerSkill(game_rule);
    foreach (const TriggerSkill *triggerSkill, Sanguosha->getGlobalTriggerSkills())
        addTriggerSkill(triggerSkill);
    if (Config.EnableBasara)
        addTriggerSkill(new BasaraMode(this));

    if (room->getScenario() != NULL) {
        const ScenarioRule *rule = room->getScenario()->getRule();
        if (rule)
            addTriggerSkill(rule);
    }

    // start game
    try {
        QString order;
        QList<ServerPlayer *> warm, cool;
        QList<ServerPlayer *> first, second;
        if (room->getMode() == "06_3v3") {
            foreach (ServerPlayer *player, room->m_players) {
                switch (player->getRoleEnum()) {
                case Player::Lord:
                    warm.prepend(player);
                    break;
                case Player::Loyalist:
                    warm.append(player);
                    break;
                case Player::Renegade:
                    cool.prepend(player);
                    break;
                case Player::Rebel:
                    cool.append(player);
                    break;
                }
            }
            order = room->askForOrder(cool.first(), "cool");
            if (order == "warm") {
                first = warm;
                second = cool;
            } else {
                first = cool;
                second = warm;
            }
        }
        constructTriggerTable();
        if (room->getTag("DisableAchievement").toBool()) {
            LogMessage log;
            log.type = "#DisableAchievement";
            room->sendLog(log);
        }
        trigger(GameStart, (Room *)room, NULL);
        if (room->getMode() == "06_3v3") {
            run3v3(first, second, game_rule, first.first());
        } else if (room->getMode() == "04_1v3") {
            ServerPlayer *shenlvbu = room->getLord();
            QList<ServerPlayer *> league = room->getPlayers();
            league.removeOne(shenlvbu);

            room->setCurrent(league.first());
            actionHulaoPass(shenlvbu, league, game_rule, 1);
        } else {
            if (room->getMode() == "02_1v1") {
                ServerPlayer *first = room->getPlayers().first();
                if (first->getRole() != "renegade")
                    first = room->getPlayers().at(1);
                ServerPlayer *second = room->findPlayer(first->getNext()->objectName());
                trigger(Debut, (Room *)room, first);
                trigger(Debut, (Room *)room, second);
                room->setCurrent(first);
            }

            actionNormal(game_rule);
        }
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == GameFinished) {
            terminate();
            Sanguosha->unregisterRoom();
            return;
        } else if (triggerEvent == TurnBroken || triggerEvent == StageChange) { // caused in Debut trigger
            ServerPlayer *first = room->getPlayers().first();
            if (first->getRole() != "renegade")
                first = room->getPlayers().at(1);
            room->setCurrent(first);
            actionNormal(game_rule);
        } else {
            Q_ASSERT(false);
        }
    }
}

static bool CompareByPriority(const TriggerSkill *a, const TriggerSkill *b)
{
    return a->getDynamicPriority() > b->getDynamicPriority();
}

bool RoomThread::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data)
{
    // push it to event stack
    EventTriplet triplet(triggerEvent, room, target);
    event_stack.push_back(triplet);

    bool broken = false;
    QList<const TriggerSkill *> will_trigger;
    QSet<const TriggerSkill *> triggerable_tested;
    QList<const TriggerSkill *> rules; // we can't get a GameRule with Engine::getTriggerSkill() :(
    TriggerList trigger_who;

    try {
        QList<const TriggerSkill *> triggered;
        QList<const TriggerSkill *> &skills = skill_table[triggerEvent];
        foreach (const TriggerSkill *skill, skills) {
            double priority = skill->getPriority(triggerEvent);
            TriggerSkill *mutable_skill = const_cast<TriggerSkill *>(skill);
            mutable_skill->setDynamicPriority(priority);
        }
        qStableSort(skills.begin(), skills.end(), CompareByPriority);

        do {
            trigger_who.clear();
            foreach (const TriggerSkill *skill, skills) {
                if (!triggered.contains(skill)) {
                    if ((skill->objectName() == "game_rule" || skill->objectName() == "hulaopass_mode")
                        || (room->getScenario() && room->getScenario()->objectName() == skill->objectName())) {
                        room->tryPause();
                        if (will_trigger.isEmpty()
                            || skill->getDynamicPriority() == will_trigger.last()->getDynamicPriority()) {
                            will_trigger.append(skill);
                            trigger_who[NULL].append(skill->objectName()); // Don't assign game rule to some player.
                            rules.append(skill);
                        } else if (skill->getDynamicPriority() != will_trigger.last()->getDynamicPriority())
                            break;
                        triggered.prepend(skill);
                    } else {
                        room->tryPause();
                        if (will_trigger.isEmpty()
                            || skill->getDynamicPriority() == will_trigger.last()->getDynamicPriority()) {
                            TriggerList triggerSkillList = skill->triggerable(triggerEvent, room, target, data);
                            foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                                if (triggerSkillList.contains(p) && !triggerSkillList.value(p).isEmpty()) {
                                    foreach (const QString &skill_name, triggerSkillList.value(p)) {
                                        const TriggerSkill *trskill = Sanguosha->getTriggerSkill(skill_name);
                                        if (trskill) { // "yiji"
                                            will_trigger.append(trskill);
                                            trigger_who[p].append(skill_name);
                                        } else {
                                            trskill = Sanguosha->getTriggerSkill(skill_name.split("'").last());
                                            if (trskill) { // "sgs1'iksongwei"
                                                will_trigger.append(trskill);
                                                trigger_who[p].append(skill_name);
                                            } else {
                                                trskill = Sanguosha->getTriggerSkill(skill_name.split("->").first());
                                                if (trskill) { // "tieqi->sgs4+sgs8+sgs1+sgs2"
                                                    will_trigger.append(trskill);
                                                    trigger_who[p].append(skill_name);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else if (skill->getDynamicPriority() != will_trigger.last()->getDynamicPriority())
                            break;

                        triggered.prepend(skill);
                    }
                }
                triggerable_tested << skill;
            }

            if (!will_trigger.isEmpty()) {
                will_trigger.clear();
                foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                    if (!trigger_who.contains(p))
                        continue;
                    QStringList already_triggered;
                    forever {
                        QStringList who_skills = trigger_who.value(p);
                        if (who_skills.isEmpty())
                            break;
                        bool has_compulsory = false;
                        foreach (const QString &skill, who_skills) {
                            const TriggerSkill *trskill = Sanguosha->getTriggerSkill(skill); // "yiji"
                            if (!trskill)
                                trskill = Sanguosha->getTriggerSkill(skill.split("'").last()); // "sgs1'songwei"
                            if (!trskill)
                                trskill = Sanguosha->getTriggerSkill(skill.split("->").first()); // "tieqi->sgs4+sgs8+sgs1+sgs2"
                            if (trskill
                                && (trskill->getFrequency(p) == Skill::Compulsory
                                    || trskill->getFrequency(p) == Skill::NotCompulsory
                                    || trskill->getFrequency(p) == Skill::Wake)) {
                                has_compulsory = true;
                                break;
                            }
                        }

                        will_trigger.clear();
                        QStringList names, back_up;
                        foreach (const QString &skill_name, who_skills) {
                            if (skill_name.contains("->")) { // "tieqi->sgs4+sgs8+sgs1+sgs2"
                                QString realSkillName = skill_name.split("->").first(); // "tieqi"
                                QStringList targets
                                    = skill_name.split("->").last().split("+"); // ["sgs4", "sgs8", "sgs1", "sgs2"]
                                int index = 1; // this index is for UI only
                                bool cannotSkip = false;
                                const TriggerSkill *trskill = Sanguosha->getTriggerSkill(realSkillName);
                                if (trskill
                                    && (trskill->getFrequency(p) == Skill::Compulsory
                                        || trskill->getFrequency(p) == Skill::NotCompulsory
                                        || trskill->getFrequency(p) == Skill::Wake))
                                    cannotSkip = true;
                                foreach (const QString &target, targets) {
                                    QString name = QString("%1->%2&%3").arg(realSkillName).arg(target).arg(index);
                                    if (names.contains(name))
                                        back_up << name;
                                    else
                                        names << name;
                                    if (cannotSkip)
                                        break;
                                    ++index;
                                }
                            } else {
                                if (names.contains(skill_name))
                                    back_up << skill_name;
                                else
                                    names << skill_name;
                            }
                        }

                        if (names.isEmpty())
                            break;

                        QString name;
                        foreach (const QString &sn, names) {
                            const TriggerSkill *skill = Sanguosha->getTriggerSkill(sn);
                            if (skill && skill->isGlobal()
                                && (skill->getFrequency(p) == Skill::Compulsory
                                    || skill->getFrequency(p) == Skill::NotCompulsory
                                    || skill->getFrequency(p) == Skill::Wake)) {
                                name = sn;
                                break;
                            }
                        }
                        if (name.isEmpty()) {
                            if (names.length() == 1 && back_up.isEmpty()) {
                                name = names.first();
                            } else if (p) {
                                QString reason = "GameRule:TriggerOrder";
                                QMap<const ServerPlayer *, QStringList> map;
                                foreach (const QString &skillName, names) {
                                    // codes below for UI shows the rest times of some skill
                                    int n = back_up.count(skillName) + 1;
                                    QString skillName_times = skillName;
                                    if (n > 1) // default means one time.
                                        skillName_times = QString("%1*%2").arg(skillName).arg(n);
                                    if (skillName.contains("'")) { // "sgs1'songwei"
                                        ServerPlayer *owner
                                            = room->findPlayer(skillName.split("'").first()); // someone such as Caopi
                                        Q_ASSERT(owner);
                                        map[owner] << skillName_times;
                                    } else
                                        map[p] << skillName_times;
                                }
                                name = room->askForTriggerOrder(p, reason, map, !has_compulsory, data);
                            } else
                                name = names.first();
                        }

                        if (name == "cancel")
                            break;
                        // "xxxx*yyy", "sgs1:xxxx" or "sgs1:xxxx*yyy"
                        int split = -1;
                        if ((split = name.indexOf('*')) != -1)
                            name = name.left(split);
                        if ((split = name.indexOf(':')) != -1)
                            name = name.mid(split + 1); // "xxxx"

                        ServerPlayer *skill_target = NULL;
                        const TriggerSkill *result_skill = NULL;
                        if (name.contains("'")) { // "sgs1'songwei"
                            skill_target = room->findPlayer(name.split("'").first());
                            result_skill = Sanguosha->getTriggerSkill(name.split("'").last());
                        } else if (name.contains("->")) { // "tieqi->sgs4&1"
                            skill_target = room->findPlayer(name.split("&").first().split("->").last());
                            result_skill = Sanguosha->getTriggerSkill(name.split("->").first());
                        } else { // "yiji"
                            skill_target = target;
                            result_skill = Sanguosha->getTriggerSkill(name);
                        }

                        Q_ASSERT(result_skill);

                        //----------------------------------------------- TriggerSkill::cost
                        already_triggered.append(name);
                        bool do_effect = false;
                        if (result_skill->cost(triggerEvent, room, skill_target, data, p))
                            do_effect = true;
                        //-----------------------------------------------

                        //----------------------------------------------- TriggerSkill::effect
                        if (do_effect) {
                            broken = result_skill->effect(triggerEvent, room, skill_target, data, p);
                            if (broken)
                                break;
                        }
                        //-----------------------------------------------

                        QStringList skill_rule = trigger_who[NULL];
                        trigger_who.clear();
                        trigger_who[NULL] = skill_rule;
                        foreach (const TriggerSkill *skill, triggered) {
                            if ((skill->objectName() == "game_rule" || skill->objectName() == "hulaopass_mode")
                                || (room->getScenario() && room->getScenario()->objectName() == skill->objectName())) {
                                room->tryPause();
                                continue;
                            } else {
                                room->tryPause();
                                if (skill->getDynamicPriority() == triggered.first()->getDynamicPriority()) {
                                    TriggerList triggerSkillList = skill->triggerable(triggerEvent, room, target, data);
                                    foreach (ServerPlayer *player, room->getAllPlayers(true)) {
                                        if (triggerSkillList.contains(player) && !triggerSkillList.value(player).isEmpty()) {
                                            foreach (const QString &skill_name, triggerSkillList.value(player)) {
                                                const TriggerSkill *trskill = Sanguosha->getTriggerSkill(skill_name);
                                                if (trskill) // "yiji"
                                                    trigger_who[player].append(skill_name);
                                                else {
                                                    trskill = Sanguosha->getTriggerSkill(skill_name.split("'").last());
                                                    if (trskill) // "sgs1'songwei"
                                                        trigger_who[player].append(skill_name);
                                                    else {
                                                        trskill = Sanguosha->getTriggerSkill(skill_name.split("->").first());
                                                        if (trskill) { // "tieqi->sgs4+sgs8+sgs1+sgs2"
                                                            trigger_who[player].append(skill_name);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else
                                    break;
                            }
                        }

                        foreach (const QString &s, already_triggered) {
                            if (s.contains("&")) {
                                // s is "skillName->targetObjectName&number"
                                QString skillName_copy = s.split("->").first();
                                QString triggered_target = s.split("->").last().split("&").first();
                                foreach (const QString &skill, trigger_who[p]) {
                                    if (skill.contains("->")) { // "tieqi->sgs4+sgs8+sgs1+sgs2"
                                        if (skill.split("->").first() == skillName_copy) { // check skill
                                            QStringList targets = skill.split("->").last().split("+");
                                            if (!targets.contains(triggered_target)) {
                                                // here may cause a bug, if the first triggerlist contains "triggered_target"
                                                // but this triggerlist doesn't contain "triggered_target". @todo_Slob
                                                continue;
                                            } else {
                                                int n = targets.indexOf(triggered_target);
                                                if (n == targets.length() - 1)
                                                    trigger_who[p].removeOne(skill); // triggered_target is the last one.
                                                else {
                                                    QStringList targets_after = targets.mid(n + 1);
                                                    // remove the targets before triggered_target
                                                    QString skill_after
                                                        = QString("%1->%2").arg(skillName_copy).arg(targets_after.join("+"));
                                                    trigger_who[p].replaceInStrings(skill, skill_after);
                                                }
                                            }
                                        } else
                                            continue;
                                    }
                                }
                            } else { // "sgs1'songwei" or "yiji"
                                if (trigger_who[p].contains(s))
                                    trigger_who[p].removeOne(s);
                            }
                        }

                        if (has_compulsory) {
                            has_compulsory = false;
                            foreach (const QString &sn, trigger_who[p]) {
                                const TriggerSkill *s = Sanguosha->getTriggerSkill(sn); // "yiji"
                                if (!s)
                                    s = Sanguosha->getTriggerSkill(sn.split("'").last()); // "sgs1'songwei"
                                if (!s)
                                    s = Sanguosha->getTriggerSkill(sn.split("->").first()); // "tieqi->sgs4+sgs8+sgs1+sgs2"
                                if (s
                                    && (s->getFrequency(p) == Skill::Compulsory || s->getFrequency(p) == Skill::NotCompulsory
                                        || s->getFrequency(p) == Skill::Wake)) {
                                    has_compulsory = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (broken)
                        break;
                }
                // @todo_Slob: for drawing cards when game starts -- stupid design of triggering no player!
                if (!broken) {
                    if (!trigger_who[NULL].isEmpty()) {
                        foreach (QString skill_name, trigger_who[NULL]) {
                            const TriggerSkill *skill = NULL;
                            foreach (const TriggerSkill *rule, rules) {
                                if (rule->objectName() == skill_name) {
                                    skill = rule;
                                    break;
                                }
                            }
                            Q_ASSERT(skill);
                            if (skill->cost(triggerEvent, room, target, data, NULL)) {
                                broken = skill->effect(triggerEvent, room, target, data, NULL);
                                if (broken)
                                    break;
                            }
                        }
                    }
                }
            }

            if (broken)
                break;
        } while (skills.length() != triggerable_tested.size());

        if (target) {
            foreach (AI *ai, room->ais)
                ai->filterEvent(triggerEvent, target, data);
        }

        // pop event stack
        event_stack.pop_back();
    } catch (TriggerEvent throwed_event) {
        if (target) {
            foreach (AI *ai, room->ais)
                ai->filterEvent(triggerEvent, target, data);
        }

        // pop event stack
        event_stack.pop_back();

        throw throwed_event;
    }

    room->tryPause();
    return broken;
}

const QList<EventTriplet> *RoomThread::getEventStack() const
{
    return &event_stack;
}

bool RoomThread::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target)
{
    QVariant data;
    return trigger(triggerEvent, room, target, data);
}

void RoomThread::addTriggerSkill(const TriggerSkill *skill)
{
    if (skill == NULL || skillSet.contains(skill->objectName()))
        return;

    skillSet << skill->objectName();

    QList<TriggerEvent> events = skill->getTriggerEvents();
    foreach (TriggerEvent triggerEvent, events) {
        QList<const TriggerSkill *> &table = skill_table[triggerEvent];
        if (!table.contains(skill))
            table << skill;
        foreach (const TriggerSkill *askill, table) {
            double priority = askill->getPriority(triggerEvent);
            /*int len = room->getPlayers().length();
            foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                if (p->hasSkill(askill->objectName())) {
                    priority += (double)len / 100;
                    break;
                }
                len--;
            }*/
            TriggerSkill *mutable_skill = const_cast<TriggerSkill *>(askill);
            mutable_skill->setDynamicPriority(priority);
        }
        qStableSort(table.begin(), table.end(), CompareByPriority);
    }

    if (skill->isVisible()) {
        foreach (const Skill *skill, Sanguosha->getRelatedSkills(skill->objectName())) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            if (trigger_skill)
                addTriggerSkill(trigger_skill);
        }
    }
}

void RoomThread::delay(long secs)
{
    if (secs == -1)
        secs = Config.AIDelay;
    Q_ASSERT(secs >= 0);
    if (room->property("to_test").toString().isEmpty() && Config.AIDelay > 0)
        msleep(secs);
}
