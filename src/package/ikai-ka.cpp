#include "ikai-ka.h"

#include "general.h"
#include "skill.h"
#include "engine.h"

IkZhijuCard::IkZhijuCard() {
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void IkZhijuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, source->objectName(), QString(), "ikzhiju", QString());
    room->moveCardsAtomic(CardsMoveStruct(subcards, NULL, Player::DrawPile, reason), false);
    if (source->isAlive()) {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (source->canDiscard(p, "ej"))
                targets << p;
        if (targets.isEmpty()) return;
        ServerPlayer *target = room->askForPlayerChosen(source, targets, "ikzhiju", "@ikzhiju");
        int card_id = room->askForCardChosen(source, target, "ej", "ikzhiju", false, MethodDiscard);
        room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : target, source);
    }
}

class IkZhiju: public ViewAsSkill {
public:
    IkZhiju(): ViewAsSkill("ikzhiju") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        IkZhijuCard *card = new IkZhijuCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() >= 2;
    }
};

class IkYingqi: public TriggerSkill {
public:
    IkYingqi(): TriggerSkill("ikyingqi") {
        events << EventPhaseStart;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() == Player::Discard && player->getHandcardNum() >= player->getHp()) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->drawCards(1);
        return false;
    }
};

IkaiKaPackage::IkaiKaPackage()
    :Package("ikai-ka")
{
    General *wind025 = new General(this, "wind025", "kaze", 3);
    wind025->addSkill(new IkZhiju);
    wind025->addSkill(new IkYingqi);

    addMetaObject<IkZhijuCard>();
}

ADD_PACKAGE(IkaiKa)