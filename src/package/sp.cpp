#include "sp.h"
#include "general.h"
#include "skill.h"
#include "standard-generals.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

class ThFanshi: public OneCardViewAsSkill {
public:
    ThFanshi(): OneCardViewAsSkill("thfanshi") {
    }
    
    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isRed() && !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() >= player->getHpPoints() && Slash::IsAvailable(player, NULL);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getHandcardNum() < player->getHpPoints())
            return false;
        return pattern == "jink" || (pattern == "slash" && player->getPhase() == Player::Play) || pattern.contains("peach");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = NULL;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "jink")
                card = new Jink(originalCard->getSuit(), originalCard->getNumber());
            else if (pattern == "peach" || pattern == "peach+analeptic")
                card = new Peach(originalCard->getSuit(), originalCard->getNumber());
            else if (pattern == "slash")
                card = new Slash(originalCard->getSuit(), originalCard->getNumber());
            else
                return NULL;
        }
        else
            card = new Slash(originalCard->getSuit(), originalCard->getNumber());

        if (card != NULL)
        {
            card->setSkillName(objectName());
            card->addSubcard(originalCard);
            return card;
        }
        else
            return NULL;
    }
};

class ThNichang: public TriggerSkill {
public:
    ThNichang(): TriggerSkill("thnichang") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::Start
            && player->askForSkillInvoke(objectName()))
        {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName());
            
            LogMessage log;
            log.type = "#ThSuoming";
            log.from = player;
            log.to << target;
            log.arg = objectName();
            log.arg2 = target->isChained() ? "chongzhi" : "hengzhi";
            room->sendLog(log);

            room->setPlayerProperty(target, "chained", !target->isChained());
        }

        return false;
    }
};

class ThQimen: public DistanceSkill {
public:
    ThQimen(): DistanceSkill("thqimen") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if ((from->hasSkill(objectName()) && to->isChained()) || (to->hasSkill(objectName()) && from->isChained()))
        {
            int x = qAbs(from->getSeat() - to->getSeat());
            int y = from->aliveCount() - x;
            return 1 - qMin(x, y);
        }
        else
            return 0;
    }
};

class ThGuanjia: public TriggerSkill {
public:
    ThGuanjia(): TriggerSkill("thguanjia") {
        events << TargetConfirmed << CardFinished;
        frequency = Compulsory;
    }
    
    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const    {
        if (triggerEvent == TargetConfirmed)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (TriggerSkill::triggerable(use.from) && use.from == player)
                foreach (ServerPlayer *p, use.to.toSet())
                    p->addMark("Armor_Nullified");
        }
        else
        {
            foreach(ServerPlayer *p,room->getAlivePlayers())
                p->setMark("Armor_Nullified", 0);
        }
        return false;
    }
};

class ThShenshi: public TriggerSkill {
public:
    ThShenshi(): TriggerSkill("thshenshi") {
        events << PhaseSkipped;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if (!splayer || !player->isWounded())
            return false;

        if (room->askForCard(splayer, "..", "@shenshi:" + player->objectName(), data, objectName()))
        {
            LogMessage log;
            log.type = "#ThShenshi";
            log.from = splayer;
            log.to << player;
            log.arg = objectName();
            room->sendLog(log);

            RecoverStruct recover;
            recover.who = splayer;
            room->recover(player, recover);
        }

        return false;
    }
};

class ThJiefan: public TriggerSkill {
public:
    ThJiefan(): TriggerSkill("thjiefan") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::Start)
            while (player->askForSkillInvoke(objectName()))
            {
                int card1 = room->drawCard();
                CardsMoveStruct move;
                move.card_ids.append(card1);
                move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
                move.to_place = Player::PlaceTable;
                room->moveCardsAtomic(move, true);
                room->getThread()->delay();
                if (Sanguosha->getCard(card1)->isKindOf("BasicCard"))
                {
                    room->obtainCard(room->askForPlayerChosen(player, room->getAlivePlayers(), objectName()), card1);
                    if (Sanguosha->getCard(card1)->isKindOf("Peach"))
                        break;
                }
                else
                {
                    room->moveCardTo(Sanguosha->getCard(card1), NULL, Player::DiscardPile, true);
                    break;
                }
            }

        return false;
    }
};

class ThChuangshi:public TriggerSkill{
public:
    ThChuangshi():TriggerSkill("thchuangshi"){
        events << CardEffected;
    }
    
    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL;
    }
    
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if (!splayer)
            return false;

        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.from == splayer)
            return false;
        if (!effect.card->isKindOf("Dismantlement")
            && !effect.card->isKindOf("Collateral")
            && !effect.card->isKindOf("Duel"))
            return false;
        
        if (!splayer->inMyAttackRange(player))
            return false;

        if (splayer->askForSkillInvoke(objectName()))
        {
            Slash *slash = new Slash(Card::NoSuitNoColor, 0);
            slash->setSkillName(objectName());
            slash->deleteLater();
            CardUseStruct newuse;
            newuse.card = slash;
            newuse.from = effect.from;
            newuse.to << splayer;
            room->useCard(newuse, false);
            return true;
        }

        return false;
    }
};

class ThGaotian:public TriggerSkill{
public:
    ThGaotian():TriggerSkill("thgaotian"){
        events << CardAsked;
        frequency = Frequent;
    }
    
    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        QString asked_pattern = data.toStringList().first();
        if(asked_pattern != "jink" || !player->askForSkillInvoke(objectName()))
            return false;

        int n = qMin(4, room->alivePlayerCount());
        
        QList<int> card_ids = room->getNCards(n, false);
        
        bool can_change = true;
        while (!card_ids.isEmpty())
        {
            room->fillAG(card_ids, player);
            int card_id = room->askForAG(player, card_ids, true, objectName());
            if (card_id == -1)
            {
                player->invoke("clearAG");
                break;
            }

            Card *card = Sanguosha->getCard(card_id);
            QStringList choices;
            choices << "discard";
            if (can_change)
                choices << "get";

            QString choice = room->askForChoice(player, objectName(), choices.join("+"));

            if (choice == "get")
            {
                QString pattern = ".";
                if (card->isRed())
                    pattern = ".|.|.|.|red";
                else if (card->isBlack())
                    pattern = ".|.|.|.|black";

                if (room->askForCard(player, pattern, "@gaotian", data, objectName()))
                {
                    room->obtainCard(player, card_id);
                    card_ids.removeOne(card_id);
                    can_change = false;
                }
            }
            else
            {
                card_ids.removeOne(card_id);
                CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
                room->moveCardTo(card, NULL, NULL, Player::DiscardPile, reason);
            }

            player->invoke("clearAG");
        }


        if (card_ids.isEmpty())
            return false;

        room->askForYuxi(player, card_ids, true);

        return false;
    }
};

SPPackage::SPPackage()
    :Package("sp")
{

    General *sp001 = new General(this, "sp001", "shu");
    sp001->addSkill(new ThFanshi);

    General *sp002 = new General(this, "sp002", "wei", 4, false);
    sp002->addSkill(new ThNichang);
    sp002->addSkill(new ThQimen);
    sp002->addSkill(new ThGuanjia);

    General *sp003 = new General(this, "sp003", "wu", 3);
    sp003->addSkill(new ThShenshi);
    sp003->addSkill(new ThJiefan);
    
    General *sp004 = new General(this, "sp004", "qun", 3);
    sp004->addSkill(new ThChuangshi);
    sp004->addSkill(new ThGaotian);

    General *sp999 = new General(this, "sp999", "te", 5, true, true);
    sp999->addSkill("jibu");
    sp999->addSkill(new Skill("thfeiniang", Skill::Compulsory));
}

ADD_PACKAGE(SP)
