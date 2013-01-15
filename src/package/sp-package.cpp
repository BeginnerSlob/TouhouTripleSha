#include "sp-package.h"
#include "general.h"
#include "skill.h"
#include "standard-generals.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"
#include "wisdompackage.h"

class ThChuangshi:public TriggerSkill{
public:
	ThChuangshi():TriggerSkill("thchuangshi"){
		events << TargetConfirmed << CardEffected;
	}
	
	virtual bool triggerable(ServerPlayer *target) const {
		return (target != NULL);
	}
	
	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
		if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player))
		{
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.from == player)
				return false;
			if (!use.card->isKindOf("Dismantlement")
				&& !use.card->isKindOf("Collateral")
				&& !use.card->isKindOf("Duel"))
				return false;
			QList<ServerPlayer *> targets;
			foreach (ServerPlayer *p, use.to)
				if (player->inMyAttackRange(p))
					targets << p;

			if (targets.isEmpty())
				return false;

			while (!targets.isEmpty() && player->askForSkillInvoke(objectName()))
			{
				ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
				if (target)
				{
					target->addMark("chuangshitarget");
					use.card->setFlags("chuangshi" + target->objectName());
					targets.removeOne(target);
					Slash *slash = new Slash(Card::NoSuitNoColor, 0);
					slash->setSkillName(objectName());
					slash->deleteLater();
					CardUseStruct newuse;
					newuse.card = slash;
					newuse.from = use.from;
					newuse.to << player;
					room->useCard(newuse, false);
				}
			}
		}
		else if (triggerEvent == CardEffected)
		{
			CardEffectStruct effect = data.value<CardEffectStruct>();
			if (effect.card->hasFlag("chuangshi" + effect.to->objectName())
				&& effect.to->getMark("chuangshitarget") > 0)
			{
				effect.to->removeMark("chuangshitarget");
				if (effect.to->getMark("chuangshitarget") <= 0)
					effect.card->setFlags("-chuangshi" + effect.to->objectName());

				return true;
			}
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
		QString pattern = data.toStringList().first();
        if(pattern != "jink" || !player->askForSkillInvoke(objectName()))
            return false;

        int n = qMin(4, room->alivePlayerCount());
        
		QList<int> card_ids = room->getNCards(n, false);
		
		bool red_can = true;
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
			if (card->isRed() && red_can)
				choices << "get";

			QString choice = room->askForChoice(player, objectName(), choices.join("+"));

			if (choice == "get")
			{
				room->obtainCard(player, card_id);
				red_can = false;
			}
			else
			{
				CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
				room->moveCardTo(card, NULL, NULL, Player::DiscardPile, reason);
			}

			card_ids.removeOne(card_id);
			player->invoke("clearAG");
		}


		if (card_ids.size() <= 1)
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
