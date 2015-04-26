#include "jianniang-scenario.h"

#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "maneuvering.h"

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
#define YISHIJIU "luna054"

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

class JnLuomo: public PhaseChangeSkill {
public:
    JnLuomo(): PhaseChangeSkill("jnluomo") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getHp() == 1;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        QStringList choices;
        if (player->isWounded())
            choices << "recover";
        choices << "draw";
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "recover")
            room->recover(player, RecoverStruct(player));
        else
            player->drawCards(2, objectName());

        room->acquireSkill(player, "iktanyan");
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

IkTanyanCard::IkTanyanCard() {
}

bool IkTanyanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

bool IkTanyanCard::targetFixed() const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetFixed();
}

bool IkTanyanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetsFeasible(targets, Self);
}

const Card *IkTanyanCard::validateInResponse(ServerPlayer *user) const{
    Room *room = user->getRoom();
    room->showCard(user, getEffectiveId());
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("iktanyan");
    room->setPlayerFlag(user, "IkTanyanUsed");
    return slash;
}

const Card *IkTanyanCard::validate(CardUseStruct &cardUse) const{
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();
    room->showCard(user, getEffectiveId());
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("iktanyan");
    room->setPlayerFlag(user, "IkTanyanUsed");
    return slash;
}

class IkTanyanViewAsSkill: public OneCardViewAsSkill {
public:
    IkTanyanViewAsSkill(): OneCardViewAsSkill("iktanyan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player) && !player->hasFlag("IkTanyanUsed");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" && !player->hasFlag("IkTanyanUsed");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkTanyanCard *tanyan_card = new IkTanyanCard;
        tanyan_card->addSubcard(originalCard);
        return tanyan_card;
    }
};

class IkTanyan: public TriggerSkill {
public:
    IkTanyan(): TriggerSkill("iktanyan") {
        events << EventPhaseChanging;
        view_as_skill = new IkTanyanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("IkTanyanUsed"))
                    room->setPlayerFlag(p, "-IkTanyanUsed");
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }
};

class JnQiwang: public PhaseChangeSkill {
public:
    JnQiwang(): PhaseChangeSkill("jnqiwang") {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && target->getMark("jnqiwang") > 0
            && target->getPhase() == Player::Play;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        player->getRoom()->broadcastSkillInvoke("jnqiwang");
        return false;
    }
};

class JnQiwangInvalidity: public InvaliditySkill {
public:
    JnQiwangInvalidity(): InvaliditySkill("#jnqiwang-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return player->getMark("jnqiwang") == 0 || (skill->objectName() != "ikyipao" && skill->objectName() != "ikshijiu");
    }
};

class JnLinbing: public TriggerSkill {
public:
    JnLinbing(): TriggerSkill("jnlinbing") {
        events << Damaged;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getLostHp() >= 2;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        room->recover(player, RecoverStruct(player, NULL, player->getLostHp()));
        room->setPlayerMark(player, "jnqiwang", 0);
        room->detachSkillFromPlayer(player, "jnqiwang", true);
        room->acquireSkill(player, "ikxiashan");
        room->detachSkillFromPlayer(player, "jnlinbing", true);
        return false;
    }
};

IkXiashanCard::IkXiashanCard() {
    target_fixed = true;
    mute = true;
}

const Card *IkXiashanCard::validate(CardUseStruct &card_use) const{
    ServerPlayer *player = card_use.from;
    Room *room = player->getRoom();
    room->setPlayerFlag(player, "IkXiashanUse");
    bool use = room->askForUseCard(player, "slash", "@ikxiashan");
    if (!use) {
        room->setPlayerFlag(player, "Global_IkXiashanFailed");
        room->setPlayerFlag(player, "-IkXiashanUse");
        return NULL;
    }
    return this;
}

const Card *IkXiashanCard::validateInResponse(ServerPlayer *player) const{
    Room *room = player->getRoom();
    room->setPlayerFlag(player, "IkXiashanUse");
    bool use = room->askForUseCard(player, "slash", "@ikxiashan");
    if (!use) {
        room->setPlayerFlag(player, "Global_IkXiashanFailed");
        room->setPlayerFlag(player, "-IkXiashanUse");
        return NULL;
    }
    return this;
}

void IkXiashanCard::onUse(Room *, const CardUseStruct &) const{
    // do nothing
}

class IkXiashan: public ZeroCardViewAsSkill {
public:
    IkXiashan(): ZeroCardViewAsSkill("ikxiashan") {
        response_pattern = "slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasFlag("Global_IkXiashanFailed") && Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return !player->hasFlag("Global_IkXiashanFailed")
            && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            && pattern == "slash"
            && !player->hasFlag("IkXiashanUse");
    }

    virtual const Card *viewAs() const{
        return new IkXiashanCard;
    }
};

class IkXiashanInvalidity: public InvaliditySkill {
public:
    IkXiashanInvalidity(): InvaliditySkill("#ikxiashan-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return !player->hasFlag("ikxiashan_ikyipao") || skill->objectName() != "ikyipao";
    }
};

class JnBizheng: public TriggerSkill {
public:
    JnBizheng(): TriggerSkill("jnbizheng") {
        events << DamageInflicted;
        frequency = Limited;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (player == p)
                continue;
            skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        DamageStruct damage = data.value<DamageStruct>();
        damage.to = ask_who;
        damage.transfer = true;
        damage.transfer_reason = "jnbizheng";
        player->tag["TransferDamage"] = QVariant::fromValue(damage);

        room->detachSkillFromPlayer(player, objectName(), true);
        return true;
    }
};

JnMingshiCard::JnMingshiCard() {
}

bool JnMingshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < Self->getHandcardNum() && to_select != Self;
}

void JnMingshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    SkillCard::use(room, source, targets);
    source->throwAllHandCards();
    foreach (ServerPlayer *p, targets)
        slash(room, source, p);
}

void JnMingshiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    const Card *card = room->askForCard(effect.from, ".", "@jnmingshi-card:" + effect.to->objectName(), QVariant(), MethodNone);

    SupplyShortage *shortage = new SupplyShortage(card->getSuit(), card->getNumber());
    shortage->setSkillName("jnmingshi");
    WrappedCard *wrapped_card = Sanguosha->getWrappedCard(card->getId());
    wrapped_card->takeOver(shortage);
    room->broadcastUpdateCard(room->getPlayers(), wrapped_card->getId(), wrapped_card);
    room->moveCardTo(wrapped_card, effect.to, Player::PlaceDelayedTrick, true);
    shortage->deleteLater();
}

void JnMingshiCard::slash(Room *room, ServerPlayer *from, ServerPlayer *to) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_jnmingshi");
    if (from->canSlash(to, slash, false))
        room->useCard(CardUseStruct(slash, from, to));
    else
        delete slash;
}

class JnMingshiVS: public ZeroCardViewAsSkill {
public:
    JnMingshiVS(): ZeroCardViewAsSkill("jnmingshi") {
        response_pattern = "@@jnmingshi";
    }

    virtual const Card *viewAs() const{
        return new JnMingshiCard;
    }
};

class JnMingshi: public PhaseChangeSkill {
public:
    JnMingshi(): PhaseChangeSkill("jnmingshi") {
        view_as_skill = new JnMingshiVS;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return PhaseChangeSkill::triggerable(player)
            && !player->isKongcheng()
            && player->getPhase() == Player::Play;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        return room->askForUseCard(player, "@@jnmingshi", "@jnmingshi");
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
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

                ServerPlayer *xili = room->findPlayer(XILI);
                if (xili)
                    room->acquireSkill(xili, "jnluomo");

                ServerPlayer *ruifeng = room->findPlayer(RUIFENG);
                if (ruifeng) {
                    room->addPlayerMark(ruifeng, "jnqiwang");
                    room->acquireSkill(ruifeng, "jnqiwang");
                    room->sendCompulsoryTriggerLog(ruifeng, "jnqiwang");
                    room->acquireSkill(ruifeng, "ikqingnang");
                    room->acquireSkill(ruifeng, "jnlinbing");
                }

                ServerPlayer *jiahe = room->findPlayer(JIAHE);
                if (jiahe) {
                    room->acquireSkill(jiahe, "jnbizheng");
                    room->acquireSkill(jiahe, "jnmingshi");
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
           << new JnLaishi
           << new JnLuomo
           << new IkTanyan
           << new JnQiwang << new JnQiwangInvalidity
           << new JnLinbing
           << new IkXiashan << new IkXiashanInvalidity
           << new JnBizheng
           << new JnMingshi << new SlashNoDistanceLimitSkill("jnmingshi");
    related_skills.insert("jndaizhan", "#jndaizhan-prohibit");
    related_skills.insert("jndaizhan", "#jndaizhan-inv");
    related_skills.insert("jnchaonu", "#jnchaonu-tar");
    related_skills.insert("jnqiwang", "#jnqiwang-inv");
    related_skills.insert("ikxiashan", "#ikxiashan-inv");
    related_skills.insert("jnmingshi", "#jnmingshi-slash-ndl");

    addMetaObject<IkTanyanCard>();
    addMetaObject<IkXiashanCard>();
    addMetaObject<JnMingshiCard>();
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
             << SHU
             << YISHIJIU;
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
