#include "ikai-sui.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"

class IkHuahuan: public OneCardViewAsSkill {
public:
    IkHuahuan(): OneCardViewAsSkill("ikhuahuan") {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const{
        const Card *card = to_select;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
                return card->isKindOf("Jink");
            }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                if (pattern == "slash")
                    return card->isKindOf("Jink");
                else if (pattern == "jink")
                    return card->isKindOf("Slash");
            }
        default:
            return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard->isKindOf("Slash")) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        } else if (originalCard->isKindOf("Jink")) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        } else
            return NULL;
    }
};

class IkQizhou: public TriggerSkill {
public:
    IkQizhou(): TriggerSkill("ikqizhou") {
        events << CardResponded << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Jink") && resp.m_card->isVirtualCard() && resp.m_card->subcardsLength() > 0)
                return QStringList(objectName());
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->isVirtualCard() && use.card->subcardsLength() > 0)
                foreach (ServerPlayer *p, use.to)
                    if (!p->isKongcheng())
                        return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (triggerEvent == CardResponded && player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == CardResponded)
            player->drawCards(1);
        else {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to) {
                if (p->isKongcheng()) continue;
                if (!player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) continue;
                int card_id = room->askForCardChosen(player, p, "h", objectName());
                player->obtainCard(Sanguosha->getCard(card_id), false);
            }
        }
        return false;
    }
};

class IkShushen: public TriggerSkill {
public:
    IkShushen(): TriggerSkill("ikshushen") {
        events << HpRecover;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (!TriggerSkill::triggerable(player)) return skill;
        RecoverStruct recover = data.value<RecoverStruct>();
        for (int i = 0; i < recover.recover; i++)
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@ikshushen", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkShushenTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = player->tag["IkShushenTarget"].value<ServerPlayer *>();
        player->tag.remove("IkShushenTarget");
        if (target) {
            target->drawCards(1, objectName());
            target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
            target->drawCards(1, objectName());
        }
        return false;
    }
};

class IkQiaoxia: public PhaseChangeSkill {
public:
    IkQiaoxia(): PhaseChangeSkill("ikqiaoxia") {
    }

    virtual bool triggerable(const ServerPlayer *ganfuren) const{
        foreach (const Card *card, ganfuren->getHandcards())
            if (ganfuren->isJilei(card))
                return false;
        return PhaseChangeSkill::triggerable(ganfuren)
            && ganfuren->getPhase() == Player::Start
            && !ganfuren->isKongcheng();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *ganfuren) const{
        Room *room = ganfuren->getRoom();
        int handcard_num = ganfuren->getHandcardNum();
        ganfuren->throwAllHandCards();
        if (handcard_num >= ganfuren->getHp())
            room->recover(ganfuren, RecoverStruct(ganfuren));
        return false;
    }
};

IkXielunCard::IkXielunCard() {
}

bool IkXielunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() >= Self->getLostHp())
        return false;

    int range_fix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getEffectiveId() == getEffectiveId()) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        range_fix += weapon->getRange() - Self->getAttackRange(false);
    } else if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getEffectiveId() == getEffectiveId())
        range_fix += 1;

    return Self->inMyAttackRange(to_select, range_fix);
}

void IkXielunCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    DamageStruct damage;
    damage.from = source;
    damage.reason = "ikxielun";

    foreach (ServerPlayer *p, targets) {
        damage.to = p;
        room->damage(damage);
    }
    foreach (ServerPlayer *p, targets) {
        if (p->isAlive())
            p->drawCards(1, "ikxielun");
    }
}

class IkXielun: public OneCardViewAsSkill {
public:
    IkXielun(): OneCardViewAsSkill("ikxielun") {
        filter_pattern = ".|red!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getLostHp() > 0 && player->canDiscard(player, "he") && !player->hasUsed("IkXielunCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkXielunCard *first = new IkXielunCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class IkCanyue: public TargetModSkill {
public:
    IkCanyue(): TargetModSkill("ikcanyue") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return from->getMark(objectName());
        else
            return 0;
    }
};

class IkCanyueCount: public TriggerSkill {
public:
    IkCanyueCount(): TriggerSkill("#ikcanyue-count") {
        events << SlashMissed << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player || player->isDead() || !player->hasSkill("ikcanyue")) return QStringList();
        if (triggerEvent == SlashMissed) {
            if (player->getPhase() == Player::Play)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                if (player->getMark("ikcanyue") > 0)
                    room->setPlayerMark(player, "ikcanyue", 0);
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->hasFlag("ikcanyue_disabled"))
                        room->setPlayerFlag(p, "-ikcanyue_disabled");
            }
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->addPlayerMark(player, "ikcanyue");
        return false;
    }
};

class IkCanyueProhibit: public ProhibitSkill {
public:
    IkCanyueProhibit(): ProhibitSkill("#ikcanyue-prohibit") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasFlag("ikcanyue_disabled") && card->isKindOf("Slash");
    }
};

class IkCanyueClear: public DetachEffectSkill {
public:
    IkCanyueClear(): DetachEffectSkill("ikcanyue") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        room->setPlayerMark(player, "ikcanyue", 0);
    }
};

class IkJiuming: public PhaseChangeSkill {
public:
    IkJiuming(): PhaseChangeSkill("ikjiuming") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Finish
               && target->getMark("@jiuming") == 0
               && target->getMark("ikjiuming_damage") >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#IkJiumingWake";
        log.from = player;
        log.arg = QString::number(player->getMark("ikjiuming_damage"));
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(player, "@jiuming", 1);
        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            room->recover(player, RecoverStruct(player));
            room->detachSkillFromPlayer(player, "ikcanyue");
        }

        return false;
    }
};

class IkJiumingCount: public TriggerSkill {
public:
    IkJiumingCount(): TriggerSkill("#ikjiuming-count") {
        events << PreDamageDone << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.from == room->getCurrent() && damage.from->getMark("@jiuming") == 0)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                if (player->getMark("ikjiuming_damage") > 0)
                    room->setPlayerMark(player, "ikjiuming_damage", 0);
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        room->addPlayerMark(damage.from, "ikjiuming_damage", damage.damage);
        return false;
    }
};

IkaiSuiPackage::IkaiSuiPackage()
    :Package("ikai-sui")
{
    General *wind005 = new General(this, "wind005", "kaze", 3);
    wind005->addSkill(new IkHuahuan);
    wind005->addSkill(new IkQizhou);

    General *wind022 = new General(this, "wind022", "kaze", 3, false);
    wind022->addSkill(new IkShushen);
    wind022->addSkill(new IkQiaoxia);

    General *wind023 = new General(this, "wind023", "kaze", 3, false);
    wind023->addSkill(new IkXielun);
    wind023->addSkill(new IkCanyue);
    wind023->addSkill(new IkCanyueCount);
    wind023->addSkill(new IkCanyueProhibit);
    wind023->addSkill(new IkCanyueClear);
    related_skills.insertMulti("ikcanyue", "#ikcanyue-count");
    related_skills.insertMulti("ikcanyue", "#ikcanyue-prohibit");
    related_skills.insertMulti("ikcanyue", "#ikcanyue-clear");
    wind023->addSkill(new IkJiuming);
    wind023->addSkill(new IkJiumingCount);
    related_skills.insertMulti("ikjiuming", "#ikjiuming-count");

    addMetaObject<IkXielunCard>();
}

ADD_PACKAGE(IkaiSui)