#include "touhou.h"

#include "general.h"
#include "skill.h"
#include "room.h"
#include "carditem.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "card.h"

class ThSuoming: public TriggerSkill{
public:
	ThSuoming(): TriggerSkill("thsuoming"){
		events << Damaged;
	}
	
	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		int damage = data.value<DamageStruct>().damage;
		LogMessage log;
		log.type = "#ThSuoming";
		log.from = player;
		while(damage --)
		{
			if (!player->askForSkillInvoke(objectName()))
				return false;
			ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName());
			log.to << target;
			log.arg = objectName();
			log.arg2 = target->isChained() ? "chongzhi" : "hengzhi";
			room->sendLog(log);
			log.to.removeOne(target);
			room->setPlayerProperty(target, "chained", !target->isChained());
		}

		return false;
	}
};

class ThChiwu: public TriggerSkill {
public:
	ThChiwu(): TriggerSkill("thchiwu") {
		events << CardEffected << SlashEffected;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (!player->isChained())
			return false;

		if(triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(effect.nature == DamageStruct::Normal){
				LogMessage log;
				log.from = player;
				log.type = "#ThChiwu";
				log.to << effect.from;
				log.arg  = objectName();
                log.arg2 = effect.slash->objectName();
                room->sendLog(log);

                return true;
            }
        }else if(triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(effect.card->isKindOf("Duel")){
                LogMessage log;
                log.from = player;
				log.type = "#ThChiwu";
				log.to << effect.from;
                log.arg = objectName();
                log.arg2 = effect.card->objectName();
                room->sendLog(log);

                return true;
            }
        }

        return false;
    }
};

ThYewangCard::ThYewangCard(){
    m_skillName = "thyewangv";
    mute = true;
}

void ThYewangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    if(target->hasLordSkill("thyewang")){
        room->setPlayerFlag(target, "ThYewangInvoked");
		room->setPlayerProperty(target, "chained", true);
        QList<ServerPlayer *> lords;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach(ServerPlayer *p, players){
            if(p->hasLordSkill("thyewang") && !p->hasFlag("ThYewangInvoked")){
                lords << p;
            }
        }
        if(lords.empty())
            room->setPlayerFlag(source, "ForbidThYewang");
    }
}

bool ThYewangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("thyewang")
            && to_select != Self && !to_select->hasFlag("ThYewangInvoked");
}

class ThYewangViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThYewangViewAsSkill(): ZeroCardViewAsSkill("thyewangv"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
		if (Self->isChained())
			return false;
		foreach (const Player *p, Self->getSiblings())
			if (p->isChained())
				return false;

        return player->getKingdom() == "qun" && !player->hasFlag("ForbidThYewang");
    }

    virtual const Card *viewAs() const{
        return new ThYewangCard;
    }
};

class ThYewang: public TriggerSkill{
public:
	ThYewang(): TriggerSkill("thyewang$"){
		events << EventPhaseStart << EventPhaseEnd << EventPhaseChanging;
	}
	
	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == EventPhaseEnd && player->hasSkill("thyewangv"))
			room->detachSkillFromPlayer(player, "thyewangv", true);
		else if (triggerEvent == EventPhaseStart)
		{
			bool can_invoke = false;
			foreach(ServerPlayer *p, room->getOtherPlayers(player))
				if (p->hasLordSkill("thyewang"))
				{
					can_invoke = true;
					break;
				}
			if (can_invoke && player->getPhase() == Player::Play && !player->hasSkill("thyewangv") && player->getKingdom() == "qun")
				room->attachSkillToPlayer(player, "thyewangv");
		}
		else if (triggerEvent == EventPhaseChanging)
		{
			PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                  return false;
            if (player->hasFlag("ForbidThYewang")) {
                room->setPlayerFlag(player, "-ForbidThYewang");
            }
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach(ServerPlayer *p, players){
                if(p->hasFlag("ThYewangInvoked")){
                    room->setPlayerFlag(p, "-ThYewangInvoked");
                }
            }
		}

		return false;
	}
};

ThJinguoCard::ThJinguoCard(){
}

bool ThJinguoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	if (subcardsLength() == 0)
		return false;
	else
		return targets.isEmpty() && to_select != Self;
}

bool ThJinguoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
	if (subcardsLength() == 0)
		return targets.isEmpty();
	else
		return !targets.isEmpty();
}

void ThJinguoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	if (subcardsLength() == 0)
		room->setPlayerFlag(source, "thjinguoinvoke");
	else
	{
		ServerPlayer *target = targets.first();
		DummyCard *dummy = new DummyCard;
		if (target->getCardCount(true) < 2)
			dummy->addSubcards(target->getCards("he"));
		else
			dummy = room->askForCardsChosen(source, target, "he", "thjinguo", 2);
        if (dummy->subcardsLength() > 0)
            source->obtainCard(dummy, false);
        dummy->deleteLater();
		RecoverStruct recover;
		recover.who = source;
		room->recover(target, recover);
	}
}

class ThJinguo:public ViewAsSkill{
public:
	ThJinguo():ViewAsSkill("thjinguo"){
	}

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
		if(!Self->hasFlag("thjinguoinvoke"))
		{
			return selected.isEmpty() && to_select->getSuit() == Card::Heart;
		}
		else
		{
			return selected.isEmpty() && to_select->getSuit() == Card::Heart && !to_select->isKindOf("TrickCard");
		}
	}

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
		if(!Self->hasFlag("thjinguoinvoke"))
		{
			ThJinguoCard *card = new ThJinguoCard;
			if (!cards.isEmpty())
				card->addSubcards(cards);
			return card;
		}
		else if (!cards.isEmpty())
		{
			Indulgence *le = new Indulgence(cards.first()->getSuit(), cards.first()->getNumber());
			le->addSubcards(cards);
			le->setSkillName(objectName());
			return le;
		}
		else
			return NULL;
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("ThJinguoCard") || player->hasFlag("thjinguoinvoke");
	}
};

class ThLianxue:public TriggerSkill{
public:
	ThLianxue():TriggerSkill("thlianxue"){
		events << EventPhaseStart;
		frequency = Wake;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (player->getPhase() != Player::Start || player->getMark("@lianxue") > 0)
			return false;

		if (player->getEquip(2) || player->getEquip(3))
		{
			LogMessage log;
			log.type = "#ThLianxueWake";
			log.from = player;
			log.arg = objectName();
			room->sendLog(log);

			QStringList choices;
			choices << "draw";
			if (player->isWounded())
				choices << "recover";

			QString choice = room->askForChoice(player, objectName(), choices.join("+"));
			
			if (choice == "recover")
			{
				RecoverStruct recover;
				recover.who = player;
				room->recover(player, recover);
			}
			else
				player->drawCards(2);

			room->loseMaxHp(player);
			room->acquireSkill(player, "kuanggu");
			player->gainMark("@lianxue");
		}

		return false;
	}
};

class ThShouye:public TriggerSkill{
public:
	ThShouye():TriggerSkill("thshouye"){
		events << DrawNCards << EventPhaseStart;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == DrawNCards)
		{
			if (player->askForSkillInvoke(objectName()))
			{
				data = data.toInt() - 1;
				ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
				room->setPlayerMark(target, "shouyetarget", 1);
			}

			return false;
		}
		else if (triggerEvent == EventPhaseStart)
		{
			if (player->getPhase() == Player::NotActive)
				foreach(ServerPlayer *p, room->getOtherPlayers(player))
				{
					if (p->getMark("shouyetarget") > 0)
					{
						room->setPlayerMark(p, "shouyetarget", 0);
						QList<Player::Phase> phases;
						phases << Player::Draw;
						p->play(phases);
						return false;
					}
				}
		}
		return false;
	}
};

class ThXushi:public TriggerSkill{
public:
	ThXushi():TriggerSkill("thxushi"){
		events << CardAsked;
		frequency = Frequent;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (player->getPhase() != Player::NotActive)
			return false;
		QString pattern = data.toStringList().first();
		if (pattern != "slash" && pattern != "jink")
			return false;

		if (!player->askForSkillInvoke(objectName()))
			return false;
		int card_id = room->drawCard();
		CardsMoveStruct move, move2;
        move.card_ids.append(card_id);
        move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        move.to_place = Player::PlaceTable;
        room->moveCardsAtomic(move, true);
		move2 = move;
		const Card *card = Sanguosha->getCard(card_id);
		if (card->isKindOf("Slash") || card->isKindOf("Jink"))
		{
			move2.to_place = Player::PlaceHand;
			move2.to = player;
			move2.reason.m_reason = CardMoveReason::S_REASON_DRAW;
		}
		else
		{
			move2.to_place = Player::DiscardPile;
			move2.to = NULL;
			move2.reason.m_reason = CardMoveReason::S_REASON_PUT;
		}

        room->moveCardsAtomic(move2, true);

		return false;
	}
};

void TouhouPackage::addTsukiGenerals(){
	General *tsuki001 = new General(this, "tsuki001$", "qun");
	tsuki001->addSkill(new ThSuoming);
	tsuki001->addSkill(new ThChiwu);
	tsuki001->addSkill(new ThYewang);

	General *tsuki002 = new General(this, "tsuki002", "qun");
	tsuki002->addSkill(new ThJinguo);
	tsuki002->addSkill(new ThLianxue);

	General *tsuki005 = new General(this, "tsuki005", "qun", 3);
	tsuki005->addSkill(new ThShouye);
	tsuki005->addSkill(new ThXushi);
	
    addMetaObject<ThYewangCard>();
    addMetaObject<ThJinguoCard>();

	skills << new ThYewangViewAsSkill;
}