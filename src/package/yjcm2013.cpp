#include "yjcm2013.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"

class Juece: public PhaseChangeSkill {
public:
    Juece(): PhaseChangeSkill("juece") {
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Finish) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> kongcheng_players;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->isKongcheng())
                kongcheng_players << p;
        }
        if (kongcheng_players.isEmpty()) return false;

        ServerPlayer *to_damage = room->askForPlayerChosen(target, kongcheng_players, objectName(),
                                                           "@juece", true, true);
        if (to_damage) {
            room->broadcastSkillInvoke(objectName());
            room->damage(DamageStruct(objectName(), target, to_damage));
        }
        return false;
    }
};

MiejiCard::MiejiCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool MiejiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void MiejiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, effect.from->objectName(), QString(), "mieji", QString());
    room->moveCardTo(this, effect.from, NULL, Player::DrawPile, reason, true);

    int trick_num = 0, nontrick_num = 0;
    foreach (const Card *c, effect.to->getCards("he")) {
        if (effect.to->canDiscard(effect.to, c->getId())) {
            if (c->isKindOf("TrickCard"))
                trick_num++;
            else
                nontrick_num++;
        }
    }
    bool discarded = room->askForDiscard(effect.to, "mieji", 1, qMin(1, trick_num), nontrick_num > 1, true, "@mieji-trick", "TrickCard");
    if (trick_num == 0 || !discarded)
        room->askForDiscard(effect.to, "mieji", 2, 2, false, true, "@mieji-nontrick", "^TrickCard");
}

class Mieji: public OneCardViewAsSkill {
public:
    Mieji(): OneCardViewAsSkill("mieji") {
        filter_pattern = "TrickCard|black";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("MiejiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        MiejiCard *card = new MiejiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Fencheng: public ZeroCardViewAsSkill {
public:
    Fencheng(): ZeroCardViewAsSkill("fencheng") {
        frequency = Limited;
        limit_mark = "@burn";
    }

    virtual const Card *viewAs() const{
        return new FenchengCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@burn") >= 1;
    }
};

class FenchengMark: public TriggerSkill {
public:
    FenchengMark(): TriggerSkill("#fencheng") {
        events << ChoiceMade;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        QStringList data_str = data.toString().split(":");
        if (data_str.length() != 3 || data_str.first() != "cardDiscard" || data_str.at(1) != "fencheng")
            return false;
        room->setTag("FenchengDiscard", data_str.last().split("+").length());
        return false;
    }
};

FenchengCard::FenchengCard() {
    mute = true;
    target_fixed = true;
}

void FenchengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@burn");
    room->broadcastSkillInvoke("fencheng");
    room->doLightbox("$FenchengAnimate", 3000);
    room->setTag("FenchengDiscard", 0);

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    source->setFlags("FenchengUsing");
    try {
        foreach (ServerPlayer *player, players) {
            if (player->isAlive())
                room->cardEffect(this, source, player);
                room->getThread()->delay();
        }
        source->setFlags("-FenchengUsing");
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            source->setFlags("-FenchengUsing");
        throw triggerEvent;
    }
}

void FenchengCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    int length = room->getTag("FenchengDiscard").toInt() + 1;
    if (!effect.to->canDiscard(effect.to, "he") || effect.to->getCardCount(true) < length
        || !room->askForDiscard(effect.to, "fencheng", 1000, length, true, true, "@fencheng:::" + QString::number(length))) {
        room->setTag("FenchengDiscard", 0);
        room->damage(DamageStruct("fencheng", effect.from, effect.to, 2, DamageStruct::Fire));
    }
}

YJCM2013Package::YJCM2013Package()
    : Package("YJCM2013")
{

    General *liru = new General(this, "liru", "qun", 3); // YJ 206
    liru->addSkill(new Juece);
    liru->addSkill(new Mieji);
    liru->addSkill(new Fencheng);
    liru->addSkill(new FenchengMark);
    related_skills.insertMulti("fencheng", "#fencheng");

    addMetaObject<MiejiCard>();
    addMetaObject<FenchengCard>();
}

ADD_PACKAGE(YJCM2013)
