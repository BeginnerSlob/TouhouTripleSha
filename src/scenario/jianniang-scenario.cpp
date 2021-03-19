#include "jianniang-scenario.h"

#include "client.h"
#include "engine.h"
#include "maneuvering.h"
#include "skill.h"
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
#define YISHIJIU "luna055"

class JnDaizhan : public PhaseChangeSkill
{
public:
    JnDaizhan()
        : PhaseChangeSkill("jndaizhan")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive() && target->getMark("jndaizhan") > 0 && target->getPhase() == Player::Play;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        player->getRoom()->broadcastSkillInvoke("jndaizhan");
        return false;
    }
};

class JnDaizhanProhibit : public ProhibitSkill
{
public:
    JnDaizhanProhibit()
        : ProhibitSkill("#jndaizhan-prohibit")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->getMark("jndaizhan") > 0 && card->isKindOf("Slash");
    }
};

class JnDaizhanInvalidity : public InvaliditySkill
{
public:
    JnDaizhanInvalidity()
        : InvaliditySkill("#jndaizhan-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return player->getMark("jndaizhan") == 0 || skill->objectName() != "ikzhange";
    }
};

class JnChaonu : public TriggerSkill
{
public:
    JnChaonu()
        : TriggerSkill("jnchaonu")
    {
        events << EventPhaseStart;
        frequency = Limited;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->isAlive() && player->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner->isWounded())
                    skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        player->gainAnExtraTurn();
        room->removePlayerCardLimitation(player, "use", "Slash$0");
        room->setPlayerFlag(player, "jnchaonu_flag");
        room->setPlayerMark(player, "jndaizhan", 0);
        room->detachSkillFromPlayer(player, "jndaizhan", true);
        room->detachSkillFromPlayer(player, "jnchaonu", true);
        return false;
    }
};

class JnChaonuTargetMod : public TargetModSkill
{
public:
    JnChaonuTargetMod()
        : TargetModSkill("#jnchaonu-tar")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasFlag("jnchaonu_flag"))
            return 1;
        else
            return 0;
    }
};

class JnQishui : public PhaseChangeSkill
{
public:
    JnQishui()
        : PhaseChangeSkill("jnqishui")
    {
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (PhaseChangeSkill::triggerable(player)) {
            if (player->getPhase() == Player::Start && player->getHp() == 1)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
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

class IkShenti : public TargetModSkill
{
public:
    IkShenti()
        : TargetModSkill("ikshenti")
    {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(objectName()) && from->getWeapon() == NULL)
            return 2;
        else
            return 0;
    }
};

class JnXiangwu : public TriggerSkill
{
public:
    JnXiangwu()
        : TriggerSkill("jnxiangwu")
    {
        events << Dying;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && !player->isKongcheng()) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who == player)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@jnxiangwu", true);
        if (target) {
            player->tag["JnXiangwuTarget"] = QVariant::fromValue(target);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
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

class JnKongwu : public TriggerSkill
{
public:
    JnKongwu()
        : TriggerSkill("jnkongwu")
    {
        events << QuitDying;
        frequency = Wake;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->acquireSkill(player, "ikkongni");
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class IkKongni : public TriggerSkill
{
public:
    IkKongni()
        : TriggerSkill("ikkongni")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                if (player->getHp() < to->getHp())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive())
                break;
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

class JnHuangqi : public PhaseChangeSkill
{
public:
    JnHuangqi()
        : PhaseChangeSkill("jnhuangqi")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start && target->getMark("@suinieused") > 0;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        player->drawCards(1, objectName());
        room->acquireSkill(player, "ikmopan");
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class IkMopan : public OneCardViewAsSkill
{
public:
    IkMopan()
        : OneCardViewAsSkill("ikmopan")
    {
        response_or_use = true;
        filter_pattern = ".|red|.|hand";
        response_pattern = "peach+analeptic";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Peach *card = new Peach(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName("ikmopan");
        return card;
    }
};

class JnLaishi : public TriggerSkill
{
public:
    JnLaishi()
        : TriggerSkill("jnlaishi")
    {
        events << HpRecover;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getHp() > 0 && player->hasFlag("Global_Dying")) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.who && recover.who->isAlive() && recover.who != player)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        RecoverStruct recover = data.value<RecoverStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(recover.who))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
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

class JnLuomo : public PhaseChangeSkill
{
public:
    JnLuomo()
        : PhaseChangeSkill("jnluomo")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start && target->getHp() == 1;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
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

IkTanyanCard::IkTanyanCard()
{
}

bool IkTanyanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

bool IkTanyanCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetFixed();
}

bool IkTanyanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetsFeasible(targets, Self);
}

const Card *IkTanyanCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    room->showCard(user, getEffectiveId());
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("iktanyan");
    room->setPlayerFlag(user, "IkTanyanUsed");
    return slash;
}

const Card *IkTanyanCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();
    room->showCard(user, getEffectiveId());
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("iktanyan");
    room->setPlayerFlag(user, "IkTanyanUsed");
    return slash;
}

class IkTanyanViewAsSkill : public OneCardViewAsSkill
{
public:
    IkTanyanViewAsSkill()
        : OneCardViewAsSkill("iktanyan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && !player->hasFlag("IkTanyanUsed");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "slash" && !player->hasFlag("IkTanyanUsed");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IkTanyanCard *tanyan_card = new IkTanyanCard;
        tanyan_card->addSubcard(originalCard);
        return tanyan_card;
    }
};

class IkTanyan : public TriggerSkill
{
public:
    IkTanyan()
        : TriggerSkill("iktanyan")
    {
        events << EventPhaseChanging;
        view_as_skill = new IkTanyanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("IkTanyanUsed"))
                    room->setPlayerFlag(p, "-IkTanyanUsed");
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }
};

class JnQiwang : public PhaseChangeSkill
{
public:
    JnQiwang()
        : PhaseChangeSkill("jnqiwang")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive() && target->getMark("jnqiwang") > 0 && target->getPhase() == Player::Play;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        player->getRoom()->broadcastSkillInvoke("jnqiwang");
        return false;
    }
};

class JnQiwangInvalidity : public InvaliditySkill
{
public:
    JnQiwangInvalidity()
        : InvaliditySkill("#jnqiwang-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return player->getMark("jnqiwang") == 0 || (skill->objectName() != "ikyipao" && skill->objectName() != "ikshijiu");
    }
};

class JnLinbing : public TriggerSkill
{
public:
    JnLinbing()
        : TriggerSkill("jnlinbing")
    {
        events << Damaged;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getLostHp() >= 2;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
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

IkXiashanCard::IkXiashanCard()
{
    target_fixed = true;
    mute = true;
}

const Card *IkXiashanCard::validate(CardUseStruct &card_use) const
{
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

const Card *IkXiashanCard::validateInResponse(ServerPlayer *player) const
{
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

void IkXiashanCard::onUse(Room *, const CardUseStruct &) const
{
    // do nothing
}

class IkXiashan : public ZeroCardViewAsSkill
{
public:
    IkXiashan()
        : ZeroCardViewAsSkill("ikxiashan")
    {
        response_pattern = "slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("Global_IkXiashanFailed") && Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return !player->hasFlag("Global_IkXiashanFailed") && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            && pattern == "slash" && !player->hasFlag("IkXiashanUse");
    }

    virtual const Card *viewAs() const
    {
        return new IkXiashanCard;
    }
};

class IkXiashanInvalidity : public InvaliditySkill
{
public:
    IkXiashanInvalidity()
        : InvaliditySkill("#ikxiashan-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return !player->hasFlag("ikxiashan_ikyipao") || skill->objectName() != "ikyipao";
    }
};

class JnBizheng : public TriggerSkill
{
public:
    JnBizheng()
        : TriggerSkill("jnbizheng")
    {
        events << DamageInflicted;
        frequency = Limited;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (player == p)
                continue;
            skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        damage.to = ask_who;
        damage.transfer = true;
        damage.transfer_reason = "jnbizheng";
        player->tag["TransferDamage"] = QVariant::fromValue(damage);

        room->detachSkillFromPlayer(player, objectName(), true);
        return true;
    }
};

JnMingshiCard::JnMingshiCard()
{
}

bool JnMingshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < Self->getHandcardNum() && to_select != Self;
}

void JnMingshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    SkillCard::use(room, source, targets);
    source->throwAllHandCards();
    foreach (ServerPlayer *p, targets)
        slash(room, source, p);
}

void JnMingshiCard::onEffect(const CardEffectStruct &effect) const
{
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

void JnMingshiCard::slash(Room *room, ServerPlayer *from, ServerPlayer *to) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_jnmingshi");
    if (from->canSlash(to, slash, false))
        room->useCard(CardUseStruct(slash, from, to));
    else
        delete slash;
}

class JnMingshiVS : public ZeroCardViewAsSkill
{
public:
    JnMingshiVS()
        : ZeroCardViewAsSkill("jnmingshi")
    {
        response_pattern = "@@jnmingshi";
    }

    virtual const Card *viewAs() const
    {
        return new JnMingshiCard;
    }
};

class JnMingshi : public PhaseChangeSkill
{
public:
    JnMingshi()
        : PhaseChangeSkill("jnmingshi")
    {
        view_as_skill = new JnMingshiVS;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return PhaseChangeSkill::triggerable(player) && !player->isKongcheng() && player->getPhase() == Player::Play;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@jnmingshi", "@jnmingshi");
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class JnYiwu : public TriggerSkill
{
public:
    JnYiwu()
        : TriggerSkill("jnyiwu")
    {
        frequency = Compulsory;
        events << EventPhaseStart << PreCardUsed << CardResponded << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && triggerEvent == EventPhaseStart && player->getPhase() == Player::Play)
            player->setMark(objectName(), player->getHp());
        else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse && resp.m_card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            player->setMark(objectName(), 0);
            player->setMark("yiwu_num", 0);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->addMark("yiwu_num");
        if (player->getMark("yiwu_num") == player->getMark(objectName()) && TriggerSkill::triggerable(player)) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->setPlayerCardLimitation(player, "use", ".", true);
        }
        return false;
    }
};

class JnZhonglei : public PhaseChangeSkill
{
public:
    JnZhonglei()
        : PhaseChangeSkill("jnzhonglei")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        foreach (const Player *p, target->getAliveSiblings()) {
            if (p->getHp() < target->getHp())
                return false;
        }
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        room->detachSkillFromPlayer(player, "jnyiwu", true);
        room->detachSkillFromPlayer(player, objectName(), true);
        room->acquireSkill(player, "ikxunlv");
        return false;
    }
};

class IkXunlv : public OneCardViewAsSkill
{
public:
    IkXunlv()
        : OneCardViewAsSkill("ikxunlv")
    {
        filter_pattern = ".|black";
        response_or_use = true;
        response_pattern = "slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Slash *card = new Slash(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class IkXunlvTargetMod : public TargetModSkill
{
public:
    IkXunlvTargetMod()
        : TargetModSkill("#ikxunlv-target")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (card->getSkillName() == "ikxunlv" || (from->hasSkill("ikxunlv") && card->hasFlag("Global_SlashAvailabilityChecker")))
            return 1000;
        else
            return 0;
    }
};

class JnJicha : public TriggerSkill
{
public: // EightDiagram && distanceTo
    JnJicha()
        : TriggerSkill("jnjicha")
    {
        frequency = Compulsory;
        events << HpChanged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getHp() == 1)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName(), 2);
        return false;
    }
};

class JnYuhe : public TriggerSkill
{
public:
    JnYuhe()
        : TriggerSkill("jnyuhe")
    {
        events << EventPhaseStart << PreCardUsed;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && triggerEvent == EventPhaseStart && player->getPhase() == Player::Play && player->canDiscard(player, "h"))
            return QStringList(objectName());
        else if (triggerEvent == PreCardUsed && player->hasFlag("JnYuheUsed")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == "iklingcha")
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart)
            return room->askForCard(player, ".", "@jnyuhe", QVariant(), objectName());
        else {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@jnyuhe-draw", true, true);
            if (target)
                target->drawCards(1, objectName());
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->setPlayerFlag(player, "JnYuheUsed");
        return false;
    }
};

class JnYanwang : public TriggerSkill
{
public:
    JnYanwang()
        : TriggerSkill("jnyanwang")
    {
        events << CardFinished;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == "iklingcha") {
                if (room->alivePlayerCount() < player->getHp())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->loseHp(player);
        room->addPlayerMark(player, objectName());
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class JnXianmao : public TriggerSkill
{
public:
    JnXianmao()
        : TriggerSkill("jnxianmao")
    {
        events << EventPhaseStart << DamageDone;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&ask_who) const
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && room->isSomeonesTurn(damage.from))
                player->setMark(objectName(), 1);
        } else {
            if (player->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setMark(objectName(), 0);
            } else if (player->getPhase() == Player::Finish) {
                if (player->hasFlag("JnXianmaoUsed"))
                    return QStringList(objectName());
            } else if (player->getPhase() == Player::NotActive) {
                ServerPlayer *misheng = room->findPlayerBySkillName(objectName());
                if (TriggerSkill::triggerable(misheng) && misheng->getMark(objectName()) > 0) {
                    ask_who = misheng;
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (player->getPhase() == Player::Finish) {
            room->sendCompulsoryTriggerLog(player, objectName());
            return true;
        } else if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (player->getPhase() == Player::Finish) {
            player->turnOver();
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->setPlayerFlag(p, "-JnXianmaoTarget");
        } else {
            room->detachSkillFromPlayer(ask_who, objectName(), true);
            room->setPlayerFlag(ask_who, "JnXianmaoUsed");
            room->setPlayerFlag(player, "JnXianmaoTarget");
            ask_who->gainAnExtraTurn();
        }
        return false;
    }
};

class JnXianmaoProhibit : public ProhibitSkill
{
public:
    JnXianmaoProhibit()
        : ProhibitSkill("#jnxianmao-prohibit")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return card->getTypeId() != Card::TypeSkill && from && from->hasFlag("JnXianmaoUsed") && !to->hasFlag("JnXianmaoTarget");
    }
};

class JnChunyu : public TriggerSkill
{
public:
    JnChunyu()
        : TriggerSkill("jnchunyu")
    {
        events << Death;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *caopi, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(caopi)) {
            DeathStruct death = data.value<DeathStruct>();
            if (caopi != death.who && !death.who->isAllNude())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &, ServerPlayer *) const
    {
        if (caopi->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &data, ServerPlayer *) const
    {
        room->detachSkillFromPlayer(caopi, objectName(), true);
        DeathStruct death = data.value<DeathStruct>();
        DummyCard *dummy = new DummyCard(death.who->handCards());
        QList<const Card *> equips = death.who->getEquips();
        dummy->addSubcards(equips);
        foreach (const Card *cd, death.who->getJudgingArea())
            dummy->addSubcard(cd);

        if (dummy->subcardsLength() > 0)
            room->obtainCard(caopi, dummy, false);

        delete dummy;

        room->acquireSkill(caopi, "thjizhi");

        return false;
    }
};

class JnSongyi : public PhaseChangeSkill
{
public:
    JnSongyi()
        : PhaseChangeSkill("jnsongyi")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start && target->getHp() == 1;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->acquireSkill(player, "ikchilian");
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class JnTaozui : public DrawCardsSkill
{
public:
    JnTaozui()
        : DrawCardsSkill("jntaozui")
    {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName(), qrand() % 2 + 2);
        room->sendCompulsoryTriggerLog(player, objectName());
        return n + 2;
    }
};

class JnTaozuiEffect : public PhaseChangeSkill
{
public:
    JnTaozuiEffect()
        : PhaseChangeSkill("#jntaozui-effect")
    {
        frequency = Compulsory;
        global = true;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive() && target->hasSkill("jntaozui") && target->getPhase() == Player::Play;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        player->getRoom()->broadcastSkillInvoke("jntaozui", 1);
        return false;
    }
};

class JnLinglie : public PhaseChangeSkill
{
public:
    JnLinglie()
        : PhaseChangeSkill("jnlinglie")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return PhaseChangeSkill::triggerable(player) && player->getPhase() == Player::Start && player->getHp() <= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        room->loseMaxHp(player, 3);
        if (player->isWounded() && player->getHp() < 3)
            room->recover(player, RecoverStruct(player, NULL, 3 - player->getHp()));
        room->removePlayerCardLimitation(player, "use,response", "BasicCard+^Slash$0");
        room->detachSkillFromPlayer(player, "jntaozui", true);
        room->detachSkillFromPlayer(player, "jnlinglie", true);
        room->addPlayerMark(player, "jnlinglie");
        return false;
    }
};

class JnDongao : public TriggerSkill
{
public:
    JnDongao()
        : TriggerSkill("jndongao")
    {
        events << DamageInflicted;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->canDiscard(player, "he")) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.damage > 1)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForCard(player, "..", "@jndongao", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        damage.damage = 1;
        data = QVariant::fromValue(damage);
        return false;
    }
};

JnChunsuCard::JnChunsuCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

void JnChunsuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "jnchunsu", QString());
    room->obtainCard(effect.to, this, reason, false);
    if (effect.to->getKingdom() != "kaze")
        room->setPlayerProperty(effect.to, "kingdom", "kaze");
    room->setPlayerFlag(effect.from, "jnchunshu_" + effect.to->objectName());
    room->detachSkillFromPlayer(effect.from, "jnchunsu", true);
}

class JnChunsuVS : public ViewAsSkill
{
public:
    JnChunsuVS()
        : ViewAsSkill("jnchunsu")
    {
        response_pattern = "@@jnchunsu";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < Self->getHp() && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != Self->getHp())
            return NULL;

        JnChunsuCard *card = new JnChunsuCard;
        card->addSubcards(cards);
        return card;
    }
};

class JnChunsu : public TriggerSkill
{
public:
    JnChunsu()
        : TriggerSkill("jnchunsu")
    {
        events << EventPhaseStart;
        frequency = Limited;
        view_as_skill = new JnChunsuVS;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Start && player->getHandcardNum() >= player->getHp();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@jnchunsu", "@jnchunsu", -1, Card::MethodNone);
    }
};

class JnChunsuTrigger : public TriggerSkill
{
public:
    JnChunsuTrigger()
        : TriggerSkill("#jnchunsu")
    {
        events << DamageCaused;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to && player->hasFlag("jnchunsu_" + damage.to->objectName()))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "jnchunsu");
        room->broadcastSkillInvoke("jnchunsu");
        return true;
    }
};

class JnXiamu : public TriggerSkill
{
public:
    JnXiamu()
        : TriggerSkill("jnxiamu")
    {
        events << Dying;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who == player && player->getHp() < 1)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->recover(player, RecoverStruct(player, NULL, 2));
        player->drawCards(qMin(room->alivePlayerCount(), 4), objectName());
        room->detachSkillFromPlayer(player, objectName(), true);
        return false;
    }
};

class JnQiuling : public TriggerSkill
{
public:
    JnQiuling()
        : TriggerSkill("jnqiuling")
    {
        events << Death;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player && player->isDead() && player->hasSkill(objectName())) {
            DeathStruct death = data.value<DeathStruct>();
            if (player == death.who)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        ServerPlayer *killer = NULL;
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage)
            killer = death.damage->from;
        if (killer && killer->isAlive() && killer->canDiscard(killer, "h"))
            room->askForDiscard(killer, objectName(), 1, 1);
        QList<ServerPlayer *> players = room->getAllPlayers();
        if (killer && players.contains(killer))
            players.removeOne(killer);
        room->drawCards(players, 1, objectName());
        return false;
    }
};

JnAngongCard::JnAngongCard()
{
    will_throw = false;
    handling_method = MethodNone;
    target_fixed = true;
}

void JnAngongCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addToPile("jnangongpile", this);
}

class JnAngong : public OneCardViewAsSkill
{
public:
    JnAngong()
        : OneCardViewAsSkill("jnangong")
    {
        filter_pattern = "BasicCard";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getPile("jnangongpile").length() < 3 && !player->hasUsed("JnAngongCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        JnAngongCard *card = new JnAngongCard;
        card->addSubcard(originalCard);
        return card;
    }
};

JnHuojiCard::JnHuojiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool JnHuojiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->setSkillName("_jnhuoji");
        }
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *card = Sanguosha->cloneCard(Self->tag.value("jnhuoji").value<const Card *>()->objectName());
    card->addSubcards(subcards);
    card->setSkillName("_jnhuoji");
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool JnHuojiCard::targetFixed() const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("jnhuoji").value<const Card *>();
    return card && card->targetFixed();
}

bool JnHuojiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->setSkillName("_jnhuoji");
        }
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    Card *card = Sanguosha->cloneCard(Self->tag.value("jnhuoji").value<const Card *>()->objectName());
    card->addSubcards(subcards);
    card->setSkillName("_jnhuoji");
    return card && card->targetsFeasible(targets, Self);
}

const Card *JnHuojiCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *wenyang = card_use.from;
    Room *room = wenyang->getRoom();

    QString to_ikshidao = user_string;
    if (user_string == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList ikshidao_list;
        ikshidao_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            ikshidao_list << "thunder_slash"
                          << "fire_slash";
        to_ikshidao = room->askForChoice(wenyang, "jnhuoji_slash", ikshidao_list.join("+"));
    }

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "jnhuoji", QString());
    room->throwCard(this, reason, NULL);

    Card *use_card = Sanguosha->cloneCard(to_ikshidao, NoSuit, 0);
    use_card->setSkillName("_jnhuoji");
    use_card->deleteLater();
    return use_card;
}

const Card *JnHuojiCard::validateInResponse(ServerPlayer *wenyang) const
{
    Room *room = wenyang->getRoom();

    QString to_ikshidao;
    if (user_string == "peach+analeptic") {
        QStringList ikshidao_list;
        ikshidao_list << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            ikshidao_list << "analeptic";
        to_ikshidao = room->askForChoice(wenyang, "jnhuoji_saveself", ikshidao_list.join("+"));
    } else if (user_string == "slash") {
        QStringList ikshidao_list;
        ikshidao_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            ikshidao_list << "thunder_slash"
                          << "fire_slash";
        to_ikshidao = room->askForChoice(wenyang, "jnhuoji_slash", ikshidao_list.join("+"));
    } else
        to_ikshidao = user_string;

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "jnhuoji", QString());
    room->throwCard(this, reason, NULL);

    Card *use_card = Sanguosha->cloneCard(to_ikshidao, NoSuit, 0);
    use_card->setSkillName("_jnhuoji");
    use_card->deleteLater();
    return use_card;
}

#include "touhou-hana.h"
class JnHuoji : public OneCardViewAsSkill
{
public:
    JnHuoji()
        : OneCardViewAsSkill("jnhuoji")
    {
        filter_pattern = ".|.|.|jnangongpile";
        expand_pile = "jnangongpile";
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPile("jnangongpile").isEmpty())
            return false;
        if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic") || pattern == "slash" || pattern == "jink")
            return true;
        return false;
    }

    virtual SkillDialog *getDialog() const
    {
        return ThMimengDialog::getInstance("jnhuoji", true, false);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            JnHuojiCard *tianyan_card = new JnHuojiCard;
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";
            tianyan_card->setUserString(pattern);
            tianyan_card->addSubcard(originalCard);
            return tianyan_card;
        }

        const Card *c = Self->tag["jnhuoji"].value<const Card *>();
        if (c) {
            JnHuojiCard *card = new JnHuojiCard;
            card->setUserString(c->objectName());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("jnangongpile").isEmpty();
    }
};

class JnHuojiDraw : public TriggerSkill
{
public:
    JnHuojiDraw()
        : TriggerSkill("#jnhuoji")
    {
        events << CardResponded << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player && player->isAlive()) {
            if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_card && resp.m_card->getSkillName() == "jnhuoji" && !resp.m_isUse)
                    return QStringList(objectName());
            } else if (triggerEvent == CardFinished) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card && use.card->getSkillName() == "jnhuoji")
                    return QStringList(objectName());
            }
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1, "jnhuoji");
        return false;
    }
};

class JnNiying : public TriggerSkill
{
public:
    JnNiying()
        : TriggerSkill("jnniying")
    {
        events << EventPhaseStart << CardEffect << CardEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish) {
                if (player->getMark("@qianhang") > 0)
                    return QStringList(objectName());
            }
        } else {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (player && player->isAlive() && player->getMark("@qianhang") > 0 && effect.to != effect.from) {
                if (effect.card && !((effect.card->isKindOf("AOE") && !effect.card->isKindOf("BurningCamps")) || effect.card->isKindOf("GlobalEffect")))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        if (triggerEvent == EventPhaseStart) {
            room->broadcastSkillInvoke(objectName(), 3);
            player->loseAllMarks("@qianhang");
        } else {
            room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
            CardEffectStruct effect = data.value<CardEffectStruct>();
            effect.nullified = true;
            data = QVariant::fromValue(effect);
            return true;
        }
        return false;
    }
};

JnTaoxiCard::JnTaoxiCard()
{
    will_throw = false;
    handling_method = MethodNone;
    target_fixed = true;
}

void JnTaoxiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "jntaoxi", QString());
    room->throwCard(this, reason, NULL);
    source->gainMark("@qianhang");
}

class JnTaoxiVS : public ViewAsSkill
{
public:
    JnTaoxiVS()
        : ViewAsSkill("jntaoxi")
    {
        expand_pile = "jnangongpile";
        response_pattern = "@@jntaoxi";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < Self->getHp() && Self->getPile("jnangongpile").contains(to_select->getId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != Self->getHp())
            return NULL;

        JnTaoxiCard *card = new JnTaoxiCard;
        card->addSubcards(cards);
        return card;
    }
};

class JnTaoxi : public TriggerSkill
{
public:
    JnTaoxi()
        : TriggerSkill("jntaoxi")
    {
        events << EventPhaseChanging << EventPhaseStart;
        view_as_skill = new JnTaoxiVS;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skills;
        if (triggerEvent == EventPhaseChanging && TriggerSkill::triggerable(player)) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->getPile("jnangongpile").length() >= player->getHp())
                skills.insert(player, QStringList(objectName()));
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p == player)
                    continue;
                if (p->getMark("@qianhang") > 0)
                    skills.insert(p, QStringList(objectName()));
            }
        }
        return skills;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventPhaseChanging)
            room->askForUseCard(ask_who, "@@jntaoxi", "@jntaoxi", -1, Card::MethodNone);
        else if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player)))
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        player->loseAllMarks("@qianhang");
        if (player->getHandcardNum() > ask_who->getHandcardNum())
            room->askForUseCard(ask_who, "@@ikqianshe", "@ikqianshe:" + player->objectName(), -1, Card::MethodNone);
        else if (player->getHandcardNum() < ask_who->getHandcardNum())
            room->askForUseCard(ask_who, "@@ikdaolei", "@ikdaolei:" + player->objectName(), -1, Card::MethodNone);
        return false;
    }
};

class JianniangScenarioRule : public ScenarioRule
{
public:
    JianniangScenarioRule(Scenario *scenario)
        : ScenarioRule(scenario)
    {
        events << GameStart << GameOverJudge << BuryVictim;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
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

                ServerPlayer *beishang = room->findPlayer(BEISHANG);
                if (beishang) {
                    room->acquireSkill(beishang, "jnyiwu");
                    room->acquireSkill(beishang, "jnzhonglei");
                }

                ServerPlayer *daofeng = room->findPlayer(DAOFENG);
                if (daofeng)
                    room->acquireSkill(daofeng, "jnjicha");

                ServerPlayer *aidang = room->findPlayer(AIDANG);
                if (aidang) {
                    room->acquireSkill(aidang, "jnyuhe");
                    room->acquireSkill(aidang, "jnyanwang");
                }

                ServerPlayer *misheng = room->findPlayer(MISHENG);
                if (misheng) {
                    room->acquireSkill(misheng, "jnxianmao");
                    room->acquireSkill(misheng, "jnchunyu");
                    room->acquireSkill(misheng, "jnsongyi");
                }

                ServerPlayer *chicheng = room->findPlayer(CHICHENG);
                if (chicheng) {
                    room->setPlayerCardLimitation(chicheng, "use,response", "BasicCard+^Slash", false);
                    room->acquireSkill(chicheng, "jntaozui");
                    room->sendCompulsoryTriggerLog(chicheng, "jntaozui");
                    LogMessage log;
                    log.type = "#GainMaxHp";
                    log.from = chicheng;
                    log.arg = "2";
                    room->sendLog(log);

                    room->setPlayerProperty(chicheng, "maxhp", chicheng->getMaxHp() + 2);
                    room->recover(chicheng, RecoverStruct(chicheng, NULL, 2));

                    room->acquireSkill(chicheng, "jnlinglie");
                }

                ServerPlayer *shu = room->findPlayer(SHU);
                if (shu) {
                    room->acquireSkill(shu, "jndongao");
                    room->acquireSkill(shu, "jnchunsu");
                    room->acquireSkill(shu, "jnxiamu");
                    room->acquireSkill(shu, "jnqiuling");
                }

                ServerPlayer *yi19 = room->findPlayer(YISHIJIU);
                if (yi19) {
                    room->acquireSkill(yi19, "jnangong");
                    room->acquireSkill(yi19, "jnhuoji");
                    room->acquireSkill(yi19, "jnniying");
                    room->sendCompulsoryTriggerLog(yi19, "jnniying");
                    yi19->gainMark("@qianhang");
                    room->acquireSkill(yi19, "jntaoxi");
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
                if (killer == player || killer->hasSkill("iktianzuoyounai"))
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

    skills << new JnDaizhan << new JnDaizhanProhibit << new JnDaizhanInvalidity << new JnChaonu << new JnChaonuTargetMod << new JnQishui << new IkShenti << new JnXiangwu
           << new JnKongwu << new IkKongni << new JnHuangqi << new IkMopan << new JnLaishi << new JnLuomo << new IkTanyan << new JnQiwang << new JnQiwangInvalidity << new JnLinbing
           << new IkXiashan << new IkXiashanInvalidity << new JnBizheng << new JnMingshi << new SlashNoDistanceLimitSkill("jnmingshi") << new JnYiwu << new JnZhonglei
           << new IkXunlv << new IkXunlvTargetMod << new JnJicha << new JnYuhe << new JnYanwang << new JnXianmao << new JnXianmaoProhibit << new JnChunyu << new JnSongyi
           << new JnTaozui << new JnTaozuiEffect << new JnLinglie << new JnDongao << new JnChunsu << new JnChunsuTrigger << new JnXiamu << new JnQiuling << new JnAngong
           << new JnHuoji << new JnHuojiDraw << new JnNiying << new JnTaoxi;
    related_skills.insert("jndaizhan", "#jndaizhan-prohibit");
    related_skills.insert("jndaizhan", "#jndaizhan-inv");
    related_skills.insert("jnchaonu", "#jnchaonu-tar");
    related_skills.insert("jnqiwang", "#jnqiwang-inv");
    related_skills.insert("ikxiashan", "#ikxiashan-inv");
    related_skills.insert("jnmingshi", "#jnmingshi-slash-ndl");
    related_skills.insert("ikxunlv", "#ikxunlv-target");
    related_skills.insert("jnxianmao", "#jnxianmao-prohibit");
    related_skills.insert("jntaozui", "#jntaozui-effect");
    related_skills.insert("jnchunsu", "#jnchunsu");
    related_skills.insert("jnhuoji", "#jnhuoji");

    addMetaObject<IkTanyanCard>();
    addMetaObject<IkXiashanCard>();
    addMetaObject<JnMingshiCard>();
    addMetaObject<JnChunsuCard>();
    addMetaObject<JnAngongCard>();
    addMetaObject<JnHuojiCard>();
    addMetaObject<JnTaoxiCard>();
}

void JianniangScenario::assign(QStringList &generals, QStringList &roles) const
{
    generals << DAHE << ZHENMING << XIANG << XILI << RUIFENG << JIAHE << BEISHANG << DAOFENG << AIDANG << MISHENG << CHICHENG << SHU << YISHIJIU;
    qShuffle(generals);
    generals.mid(0, 8);

    // roles
    for (int i = 0; i < 8; i++)
        roles << "renegade";
}

int JianniangScenario::getPlayerCount() const
{
    return 8;
}

QString JianniangScenario::getRoles() const
{
    return "NNNNNNNN";
}

void JianniangScenario::onTagSet(Room *, const QString &) const
{
}
