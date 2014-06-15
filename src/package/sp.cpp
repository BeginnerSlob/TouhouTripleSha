#include "sp.h"
#include "general.h"
#include "skill.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

class ThFanshi: public OneCardViewAsSkill {
public:
    ThFanshi(): OneCardViewAsSkill("thfanshi") {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() >= player->getHp() && Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getHandcardNum() < player->getHp())
            return false;
        return (pattern == "slash" && player->getPhase() != Player::NotActive)
            || ((pattern == "jink" || pattern.contains("peach")) && player->getPhase() == Player::NotActive);
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = NULL;
        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
                card = new Slash(originalCard->getSuit(), originalCard->getNumber());
                break;
                                                  }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                if (pattern == "jink")
                    card = new Jink(originalCard->getSuit(), originalCard->getNumber());
                else if (pattern == "peach" || pattern == "peach+analeptic")
                    card = new Peach(originalCard->getSuit(), originalCard->getNumber());
                else if (pattern == "slash")
                    card = new Slash(originalCard->getSuit(), originalCard->getNumber());
                else
                    return NULL;
                break;
                                                          }
        }
        if (card != NULL){
            card->setSkillName(objectName());
            card->addSubcard(originalCard);
            return card;
        }
        return NULL;
    }
};

class ThNichang: public TriggerSkill {
public:
    ThNichang(): TriggerSkill("thnichang") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName(), "@thnichang", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThNichangTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThNichangTarget"].value<PlayerStar>();
        player->tag.remove("ThNichangTarget");
        if (target) {
            target->setChained(!target->isChained());
            room->broadcastProperty(target, "chained");
            room->setEmotion(target, "chain");
            room->getThread()->trigger(ChainStateChanged, room, target);
        }

        return false;
    }
};

class ThQimen: public DistanceSkill {
public:
    ThQimen(): DistanceSkill("thqimen"){
    }

    virtual int getCorrect(const Player *from, const Player *to) const {
        if ((from->hasSkill(objectName()) && to->isChained())
            || (to->hasSkill(objectName()) && from->isChained())) {
            int x = qAbs(from->getSeat() - to->getSeat());
            int y = from->aliveCount() - x;
            return 1 - qMin(x, y);
        } else
            return 0;
    }
};

class ThGuanjia: public TriggerSkill {
public:
    ThGuanjia(): TriggerSkill("thguanjia") {
        events << TargetSpecified << CardFinished;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified)
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                room->addPlayerMark(p, "Armor_Nullified");
        else
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                room->removePlayerMark(p, "Armor_Nullified");

        return QStringList();
    }
};

class ThShenshi: public TriggerSkill {
public:
    ThShenshi(): TriggerSkill("thshenshi") {
        events << EventPhaseSkipped << EventPhaseChanging;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("thshenshiused"))
                        p->setFlags("-thshenshiused");
                }
            }
        } else if (player->isWounded())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner->hasFlag("thshenshiused") || !owner->canDiscard(owner, "he")) continue;
                skill_list.insert(owner, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        if (room->askForCard(ask_who, "..", "@shenshi:" + player->objectName(), data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        room->recover(player, RecoverStruct(ask_who));
        ask_who->setFlags("thshenshiused");
        return false;
    }
};

class ThJiefan: public TriggerSkill {
public:
    ThJiefan(): TriggerSkill("thjiefan") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        int id = room->drawCard();

        CardsMoveStruct move;
        move.card_ids << id;
        move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        move.to_place = Player::PlaceTable;
        room->moveCardsAtomic(move, true);
        room->getThread()->delay();

        bool next = true;
        const Card *card = Sanguosha->getCard(id);
        if (card->isKindOf("BasicCard")) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName());
            room->obtainCard(target, card);
            if (card->isKindOf("Peach"))
                next = false;
        } else {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
            room->throwCard(card, reason, NULL);
            next = false;
        }

        if (next && player->askForSkillInvoke(objectName()))
            effect(NonTrigger, room, player, data, player);
        return false;
    }
};

class ThChuangshi: public TriggerSkill {
public:
    ThChuangshi(): TriggerSkill("thchuangshi") {
        events << CardEffected;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (!effect.card->isKindOf("Dismantlement")
            && !effect.card->isKindOf("Collateral")
            && !effect.card->isKindOf("Duel"))
            return skill_list;

        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (effect.from == owner) continue;
            if (owner == player || owner->inMyAttackRange(player))
                skill_list.insert(owner, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_" + objectName());
        room->useCard(CardUseStruct(slash, effect.from, ask_who));
        return true;
    }
};

class ThGaotian: public TriggerSkill {
public:
    ThGaotian(): TriggerSkill("thgaotian") {
        events << CardAsked;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        QString asked_pattern = data.toStringList().first();
        if (TriggerSkill::triggerable(player) && asked_pattern == "jink")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        int n = qMin(4, room->alivePlayerCount());
        QList<int> card_ids = room->getNCards(n, false);

        room->fillAG(card_ids, player);

        QList<int> reds, blacks;
        foreach (int id, card_ids)
            if (Sanguosha->getCard(id)->isRed())
                reds << id;
            else if (Sanguosha->getCard(id)->isBlack())
                blacks << id;
        QString pattern = ".";
        if (reds.isEmpty())
            pattern = ".|black";
        else if (blacks.isEmpty())
            pattern = ".|red";
        const Card *card = room->askForCard(player, pattern, "@gaotian-discard", data, objectName());
        if (card) {
            int card_id = -1;
            if (card->isBlack()) {
                room->clearAG(player);
                room->fillAG(card_ids, player, reds);
                card_id = room->askForAG(player, blacks, false, objectName());
            } else if (card->isRed()) {
                room->clearAG(player);
                room->fillAG(card_ids, player, blacks);
                card_id = room->askForAG(player, reds, false, objectName());
            }
            if (card_id != -1) {
                room->clearAG(player);
                room->fillAG(card_ids, player);
                QList<ServerPlayer *> p_list;
                p_list << player;
                room->takeAG(player, card_id, false, p_list);
                card_ids.removeOne(card_id);
                room->obtainCard(player, card_id);
            }
        }

        DummyCard *dummy = new DummyCard;
        QList<ServerPlayer *> p_list;
        p_list << player;
        while (!card_ids.isEmpty()) {
            int card_id = room->askForAG(player, card_ids, true, objectName());
            if (card_id != -1) {
                room->takeAG(NULL, card_id, false, p_list);
                card_ids.removeOne(card_id);
                dummy->addSubcard(card_id);
            } else
                break;
        }

        room->clearAG(player);
        if (!card_ids.isEmpty())
            room->askForGuanxing(player, card_ids, Room::GuanxingUpOnly);
        if (dummy->subcardsLength() > 0) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
            room->throwCard(dummy, reason, NULL);
        }
        delete dummy;

        return false;
    }
};

class ThWanling: public TriggerSkill {
public:
    ThWanling(): TriggerSkill("thwanling") {
        events << BeforeCardsMove;
    }

    virtual QStringList triggerable(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!TriggerSkill::triggerable(player) || player == move.from) return QStringList();
        if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
            && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
            CardStar card = move.reason.m_extraData.value<CardStar>();
            if (card && card->isRed() && (card->isKindOf("Slash") || card->isNDTrick()))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        if (judge.isGood()) {
            if (player->canDiscard(player, "he") && room->askForCard(player,
                                                                     "..",
                                                                     "@thwanling:" + move.from->objectName(),
                                                                     data,
                                                                     objectName())) {
                player->obtainCard(move.reason.m_extraData.value<CardStar>());
                move.removeCardIds(move.card_ids);
                data = QVariant::fromValue(move);
            } else
                room->drawCards((ServerPlayer *)move.from, 1, objectName());
        }
        return false;
    }
};

class ThZuibu: public TriggerSkill {
public:
    ThZuibu(): TriggerSkill("thzuibu") {
        events << DamageInflicted;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        player->drawCards(1, objectName());
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->askForSkillInvoke(objectName(), "decrease")) {
            damage.from->drawCards(1);
            
            LogMessage log;
            log.type = "#ThZuibu";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(--damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
            if (damage.damage < 1)
                return true;
        }
        return false;
    }
};

SPPackage::SPPackage()
    :Package("sp")
{
    General *sp001 = new General(this, "sp001", "kaze");
    sp001->addSkill(new ThFanshi);

    General *sp002 = new General(this, "sp002", "hana", 4, false);
    sp002->addSkill(new ThNichang);
    sp002->addSkill(new ThQimen);
    sp002->addSkill(new ThGuanjia);

    General *sp003 = new General(this, "sp003", "yuki", 3);
    sp003->addSkill(new ThShenshi);
    sp003->addSkill(new ThJiefan);

    General *sp004 = new General(this, "sp004", "tsuki", 3);
    sp004->addSkill(new ThChuangshi);
    sp004->addSkill(new ThGaotian);

    General *sp005 = new General(this, "sp005", "kaze", 3);
    sp005->addSkill(new ThWanling);
    sp005->addSkill(new ThZuibu);

    /*General *sp999 = new General(this, "sp999", "te", 5, true, true);
    sp999->addSkill("jibu");
    sp999->addSkill(new Skill("thfeiniang", Skill::Compulsory));*/
}

ADD_PACKAGE(SP)
