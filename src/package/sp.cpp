#include "sp.h"
#include "general.h"
#include "skill.h"
#include "standard-generals.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

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

				if (room->askForCard(player, pattern, objectName()))
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

	General *sp004 = new General(this, "sp004", "qun", 3);
	sp004->addSkill(new ThChuangshi);
	sp004->addSkill(new ThGaotian);
}

ADD_PACKAGE(SP)
