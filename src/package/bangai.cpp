#include "bangai.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

class Silian: public FilterSkill{
public:
	Silian():FilterSkill("silian"){
	}

	virtual bool viewFilter(const Card* to_select) const{
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return place == Player::PlaceHand && to_select->isKindOf("Weapon");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class SilianGet: public TriggerSkill{
public:
	SilianGet():TriggerSkill("#silian"){
		events << CardsMoveOneTime;
	}

	virtual int getPriority() const{
		return 4;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
		if(!move->to || move->to != player)
			return false;
		if(move->to_place == Player::PlaceEquip){
			QList<CardsMoveStruct> exchangeMove;
			CardsMoveStruct silianMove;
			silianMove.to = player;
			silianMove.to_place = Player::PlaceHand;

			int card_id;
			for(int i = 0; i < move->card_ids.length(); i++) {
				card_id = move->card_ids.at(i);
				if(Sanguosha->getCard(card_id)->isKindOf("Weapon"))
					silianMove.card_ids << card_id;
				}

			if(silianMove.card_ids.isEmpty())
				return false;
	
			exchangeMove.push_back(silianMove);
	        room->moveCardsAtomic(exchangeMove, true);
		}
		return false;
	}
};

class SilianTriggerSkill: public TriggerSkill{
public:
	SilianTriggerSkill():TriggerSkill("#silian-weapon"){
		events << SlashMissed;
	}

	virtual int getPriority() const{
		return -1;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if (!effect.to->isAlive())
            return false;
        if (!effect.from->canSlash(effect.to, NULL, false))
            return false;

        if(room->askForUseSlashTo(player, effect.to, "blade-slash:" + effect.to->objectName()))
            room->setEmotion(player,"weapon/blade");

        return false;
    }
};

BangaiPackage::BangaiPackage()
    :Package("bangai")
{
	General *bangai009 = new General(this, "bangai009", "shu");
	bangai009->addSkill(new Silian);
	bangai009->addSkill(new SilianGet);
	bangai009->addSkill(new SilianTriggerSkill);
    related_skills.insertMulti("silian", "#silian");
    related_skills.insertMulti("silian", "#silian-weapon");
}

ADD_PACKAGE(Bangai)