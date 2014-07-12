#include "ikai-ka.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"

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

IkJilunCard::IkJilunCard() {
}

bool IkJilunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasEquip() && to_select != Self;
}

void IkJilunCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (effect.to->hasEquip()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "e", "ikjilun");
        room->obtainCard(effect.to, card_id);
    }
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_ikjilun");
    if (effect.from->canSlash(effect.to, slash, false) && !effect.from->isCardLimited(slash, Card::MethodUse))
        room->useCard(CardUseStruct(slash, effect.from, effect.to), false);
    else
        delete slash;
}

class IkJilun: public OneCardViewAsSkill {
public:
    IkJilun(): OneCardViewAsSkill("ikjilun") {
        filter_pattern = ".|.|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkJilunCard *card = new IkJilunCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkJilunCard") && player->canDiscard(player, "h");
    }
};

class IkJiqiao: public TriggerSkill {
public:
    IkJiqiao(): TriggerSkill("ikjiqiao") {
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Draw
            && player->getHandcardNum() < player->getMaxHp();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        const Card *card = room->askForCard(player, "..", "@ikjiqiao", QVariant(), objectName());
        if (card) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkJiqiaoCard"] = QVariant::fromValue(card);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        const Card *card = player->tag["IkJiqiaoCard"].value<const Card *>();
        player->tag.remove("IkJiqiaoCard");
        if (card) {
            switch (card->getTypeId()) {
            case Card::TypeBasic: {
                    if (Slash::IsAvailable(player)) {
                        QList<ServerPlayer *> targets;
                        foreach (ServerPlayer *p, room->getOtherPlayers(player))
                            if (player->canSlash(p, false))
                                targets << p;

                        if (targets.isEmpty())
                            return false;

                        ServerPlayer *victim = room->askForPlayerChosen(player, targets, objectName(), "@ikjiqiao-basic");
                        Slash *slash = new Slash(Card::NoSuit, 0);
                        slash->setSkillName("_ikjiqiao");
                        room->useCard(CardUseStruct(slash, player, victim));
                    }
                    break;
                }
            case Card::TypeEquip: {
                    ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikjiqiao-equip");
                    target->drawCards(2, objectName());
                    break;
                }
            case Card::TypeTrick: {
                    QList<ServerPlayer *> targets;
                    foreach (ServerPlayer *p, room->getOtherPlayers(player))
                        if (p->isWounded())
                            targets << p;

                    if (targets.isEmpty())
                        return false;
                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@ikjiqiao-trick");
                    room->recover(target, RecoverStruct(player));
                    break;
                }
            default:
                    break;
            }
        }
        return false;
    }
};

IkaiKaPackage::IkaiKaPackage()
    :Package("ikai-ka")
{
    General *wind025 = new General(this, "wind025", "kaze", 3);
    wind025->addSkill(new IkZhiju);
    wind025->addSkill(new IkYingqi);

    General *wind033 = new General(this, "wind033", "kaze");
    wind033->addSkill(new IkJilun);

    General *wind034 = new General(this, "wind034", "kaze");
    wind034->addSkill(new IkJiqiao);

    addMetaObject<IkZhijuCard>();
    addMetaObject<IkJilunCard>();
}

ADD_PACKAGE(IkaiKa)