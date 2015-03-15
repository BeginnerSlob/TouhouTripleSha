#include "jianniang-scenario.h"

#include "skill.h"
#include "engine.h"
#include "standard.h"

#define DAHE "luna019"
#define ZHENMING "snow039"
#define XIANG "wind010"
#define XILI "wind051"
#define RUIFENG "wind003"
#define JIAHE "wind047"
#define BEISHANG "bloom036"
#define DAOFENG "luna037"
#define AIDANG "bloom052"
#define MISHENG "snow052"
#define CHICHENG "luna052"
#define SHU "wind052"

class JnDaizhan: public PhaseChangeSkill {
public:
    JnDaizhan(): PhaseChangeSkill("jndaizhan") {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && target->getMark("jndaizhan") > 0
            && target->getPhase() == Player::Play;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        player->getRoom()->broadcastSkillInvoke("jndaizhan");
        return false;
    }
};

class JnDaizhanProhibit: public ProhibitSkill {
public:
    JnDaizhanProhibit(): ProhibitSkill("#jndaizhan-prohibit") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->getMark("jndaizhan") > 0 && card->isKindOf("Slash");
    }
};

class JnDaizhanInvalidity: public InvaliditySkill {
public:
    JnDaizhanInvalidity(): InvaliditySkill("#jndaizhan-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return player->getMark("jndaizhan") == 0 || skill->objectName() != "ikzhange";
    }
};

class JnChaonu: public TriggerSkill {
public:
    JnChaonu(): TriggerSkill("jnchaonu") {
        events << EventPhaseStart;
        frequency = Limited;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
        if (player->isAlive() && player->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner->isWounded())
                    skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->gainAnExtraTurn();
        room->removePlayerCardLimitation(player, "use", "Slash$0");
        room->setPlayerFlag(player, "jnchaonu_flag");
        room->setPlayerMark(player, "jndaizhan", 0);
        room->detachSkillFromPlayer(player, "jndaizhan", true);
        room->detachSkillFromPlayer(player, "jnchaonu", true);
        return false;
    }
};

class JnChaonuTargetMod: public TargetModSkill {
public:
    JnChaonuTargetMod(): TargetModSkill("#jnchaonu-tar") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("jnchaonu_flag"))
            return 1;
        else
            return 0;
    }
};

class JnQishui: public PhaseChangeSkill {
public:
    JnQishui(): PhaseChangeSkill("jnqishui") {
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (PhaseChangeSkill::triggerable(player)) {
            if (player->getPhase() == Player::Start && player->getHp() == 1)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();

        LogMessage log;
        log.type = "#JnQishui";
        log.from = player;
        log.arg = QString::number(player->getHp());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        if (player->getWeapon() && player->canDiscard(player, player->getWeapon()->getId()))
            room->throwCard(player->getWeapon(), player);

        room->acquireSkill(player, "ikshenti");

        if (player->getMark("@zhangeused") > 0) {
            room->addPlayerMark(player, "@zhange", player->getMark("@zhangeused"));
            room->setPlayerMark(player, "@zhangeused", 0);
        }

        room->detachSkillFromPlayer(player, objectName(), true);

        return false;
    }
};

class IkShenti: public TargetModSkill {
public:
    IkShenti(): TargetModSkill("ikshenti") {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()) && from->getWeapon() == NULL)
            return 2;
        else
            return 0;
    }
};

class JnXiangwu: public TriggerSkill {
public:
    JnXiangwu(): TriggerSkill("jnxiangwu") {
        events << Dying;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && !player->isKongcheng()) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who == player)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@jnxiangwu", true);
        if (target) {
            player->tag["JnXiangwuTarget"] = QVariant::fromValue(target);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = player->tag["JnXiangwuTarget"].value<ServerPlayer *>();
        player->tag.remove("JnXiangwuTarget");
        if (target) {
            DummyCard *dummy = player->wholeHandCards();
            target->obtainCard(dummy, false);
            delete dummy;
            const Card *peach = room->askForSinglePeach(target, player);
            if (peach) {
                room->useCard(CardUseStruct(peach, target, player));
                room->acquireSkill(player, "iklihui");
                if (target->getRole() == "rebel")
                    room->setPlayerProperty(player, "role", "rebel");
                else {
                    room->setPlayerProperty(player, "role", "loyalist");
                    room->setPlayerProperty(target, "role", "loyalist");
                }
            }
            room->detachSkillFromPlayer(player, objectName(), true);
        }
        return false;
    }
};

class JnKongwu: public TriggerSkill {
public:
    JnKongwu(): TriggerSkill("jnkongwu") {
        events << QuitDying;
        frequency = Wake;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->acquireSkill(player, "ikkongni");
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class IkKongni: public TriggerSkill {
public:
    IkKongni(): TriggerSkill("ikkongni") {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                if (player->getHp() < to->getHp())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive()) break;
            if (player->getHp() >= p->getHp())
                continue;

            room->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#NoJink";
            log.from = p;
            room->sendLog(log);
            jink_list.replace(index, QVariant(0));

            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class JnHuangqi: public PhaseChangeSkill {
public:
    JnHuangqi(): PhaseChangeSkill("jnhuangqi") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("@suinieused") > 0;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        player->drawCards(1, objectName());
        room->acquireSkill(player, "ikmopan");
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class IkMopan: public OneCardViewAsSkill {
public:
    IkMopan(): OneCardViewAsSkill("ikmopan") {
        response_or_use = true;
        filter_pattern = ".|red|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *) const {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (pattern != "peach")
            return false;
        QString str = player->property("currentdying").toString();
        return player->objectName() == str;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        Peach *card = new Peach(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName("ikmopan");
        return card;
    }
};

class JnLaishi: public TriggerSkill{
public:
    JnLaishi(): TriggerSkill("jnlaishi"){
        events << HpRecover;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && player->getHp() > 0 && player->hasFlag("Global_Dying")) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.who && recover.who->isAlive() && recover.who != player)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        RecoverStruct recover = data.value<RecoverStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(recover.who))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        RecoverStruct recover = data.value<RecoverStruct>();
        recover.who->drawCards(1, objectName());
        if (recover.who->getRole() == "loyalist")
            room->setPlayerProperty(player, "role", "loyalist");
        else {
            room->setPlayerProperty(player, "role", "rebel");
            room->setPlayerProperty(recover.who, "role", "rebel");
        }

        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class JianniangScenarioRule: public ScenarioRule {
public:
    JianniangScenarioRule(Scenario *scenario)
        : ScenarioRule(scenario)
    {
        events << GameStart << GameOverJudge << BuryVictim;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        switch (triggerEvent) {
        case GameStart: {
            if (player == NULL) {
                ServerPlayer *dahe = room->findPlayer(DAHE);
                if (dahe) {
                    room->setPlayerCardLimitation(dahe, "use", "Slash", false);
                    room->addPlayerMark(dahe, "jndaizhan");
                    room->acquireSkill(dahe, "jndaizhan");
                    room->acquireSkill(dahe, "jnchaonu");
                    room->acquireSkill(dahe, "jnqishui");
                }

                ServerPlayer *zhenming = room->findPlayer(ZHENMING);
                if (zhenming) {
                    room->acquireSkill(zhenming, "jnxiangwu");
                    room->acquireSkill(zhenming, "jnkongwu");
                }

                ServerPlayer *xiang = room->findPlayer(XIANG);
                if (xiang) {
                    room->acquireSkill(xiang, "jnhuangqi");
                    room->acquireSkill(xiang, "jnlaishi");
                }
                return false;
            }
            break;
                        }
        case GameOverJudge: {
            QList<ServerPlayer *> players = room->getAlivePlayers();
            if (players.length() == 1)
                room->gameOver(players.first()->objectName());
            else {
                QString first_role = players.first()->getRole();
                if (first_role != "renegade") {
                    bool all_same = true;
                    foreach (ServerPlayer *p, players) {
                        if (p->getRole() == "renegade" || p->getRole() != first_role) {
                            all_same = false;
                            break;
                        }
                    }
                    if (all_same)
                        room->gameOver(first_role);
                }
            }
            return true;
                            }
        case BuryVictim: {
            DeathStruct death = data.value<DeathStruct>();
            player->bury();
            // reward and punishment
            if (death.damage && death.damage->from) {
                ServerPlayer *killer = death.damage->from;
                if (killer == player || killer->hasSkill("iktianzuo"))
                    return false;
                if (killer->getRole() != "renegade" && killer->getRole() == death.who->getRole())
                    killer->throwAllHandCardsAndEquips();
                else
                    killer->drawCards(3, "kill");
            }

            break;
                         }
        default:
            break;
        }

        return false;
    }
};

JianniangScenario::JianniangScenario()
    : Scenario("jianniang")
{
    rule = new JianniangScenarioRule(this);

    skills << new JnDaizhan << new JnDaizhanProhibit << new JnDaizhanInvalidity
           << new JnChaonu << new JnChaonuTargetMod
           << new JnQishui
           << new IkShenti
           << new JnXiangwu
           << new JnKongwu
           << new IkKongni
           << new JnHuangqi
           << new IkMopan
           << new JnLaishi;
    related_skills.insert("jndaizhan", "#jndaizhan-prohibit");
    related_skills.insert("jndaizhan", "#jndaizhan-inv");
    related_skills.insert("jnchaonu", "#jnchaonu-tar");
}

void JianniangScenario::assign(QStringList &generals, QStringList &roles) const{
    generals << DAHE
             << ZHENMING
             << XIANG
             << XILI
             << RUIFENG
             << JIAHE
             << BEISHANG
             << DAOFENG
             << AIDANG
             << MISHENG
             << CHICHENG
             << SHU;
    qShuffle(generals);
    generals.mid(0, 8);

    // roles
    for (int i = 0; i < 8; i++)
        roles << "renegade";
}

int JianniangScenario::getPlayerCount() const{
    return 8;
}

QString JianniangScenario::getRoles() const{
    return "NNNNNNNN";
}

void JianniangScenario::onTagSet(Room *, const QString &) const{
}
