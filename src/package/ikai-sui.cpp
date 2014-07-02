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

IkaiSuiPackage::IkaiSuiPackage()
    :Package("ikai-sui")
{
    General *wind005 = new General(this, "wind005", "kaze", 3);
    wind005->addSkill(new IkHuahuan);
    wind005->addSkill(new IkQizhou);
}

ADD_PACKAGE(IkaiSui)