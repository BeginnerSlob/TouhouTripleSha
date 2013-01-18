#include "bangai.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

class ThBianfang: public TriggerSkill{
public:
	ThBianfang():TriggerSkill("thbianfang"){
		events << Damage << Damaged;
		frequency = Frequent;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if (!player->isWounded() || !damage.card->isKindOf("Slash"))
			return false;
		if (triggerEvent == Damage && (damage.chain || damage.transfer))
			return false;

		if (!player->askForSkillInvoke(objectName()))
			return false;

		int x = qMin(player->getLostHp(), 3);
		QString choice;
		QStringList choices;
		choices << "spade" << "heart" << "club" << "diamond";
		for (int i = 0; i < x; i++)
		{
			choice = room->askForChoice(player, objectName(), choices.join("+"));
			LogMessage log;
			log.type = "#ChooseSuit";
			log.from = player;
			log.arg  = choice;
			room->sendLog(log);
			choices.removeOne(choice);
		}
		JudgeStruct judge;
		judge.pattern = QRegExp("(.*):(.*):(.*)");
		judge.good = true;
		judge.reason = objectName();
		judge.who = player;
		room->judge(judge);
		if (!choices.contains(judge.card->getSuitString()))
		{
			QStringList choices;
			QList<ServerPlayer *> targets;
			foreach(ServerPlayer *p, room->getOtherPlayers(player))
				if (!p->isNude())
					targets << p;
			
			if (!targets.isEmpty())
				choices << "discard";

			if (player->isWounded())
				choices << "recover";

			if (choices.isEmpty())
				return false;
			QString choice = room->askForChoice(player, objectName(), choices.join("+"));

			if (choice == "discard")
			{
				ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
				int card_id = room->askForCardChosen(player, target, "he", objectName());
				room->throwCard(card_id, target, player);
			}
			else
			{
				RecoverStruct recover;
					recover.who = player;
				room->recover(player, recover);
			}
		}

		return false;
	}
};

class ThBanyue: public TriggerSkill{
public:
	ThBanyue():TriggerSkill("thbanyue"){
		events << CardUsed << TargetConfirmed;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		CardUseStruct use = data.value<CardUseStruct>();
		if (!use.card->isKindOf("Slash"))
			return false;

		if (triggerEvent == CardUsed)
		{
			if (player->askForSkillInvoke(objectName()))
			{
				JudgeStruct judge;
				judge.pattern = QRegExp("(.*):(.*):(.*)");
				judge.good = true;
				judge.reason = objectName();
				judge.who = player;
				room->judge(judge);

				if (judge.card->isRed())
					use.card->setFlags("thbanyuered");
				else if (judge.card->isBlack())
				{
					QList<ServerPlayer *> targets;
					foreach(ServerPlayer *p, room->getOtherPlayers(player))
						if (player->canSlash(p, use.card) && !use.to.contains(p))
							targets << p;

					if (targets.isEmpty())
						return false;
					ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
					use.to << target;
					qSort(use.to.begin(), use.to.end(), ServerPlayer::CompareByActionOrder);
				}
			}
		}
		else if (triggerEvent == TargetConfirmed && use.card->hasFlag("thbanyuered"))
			foreach(ServerPlayer *p, use.to)
				if (!p->isNude())
					room->throwCard(room->askForCardChosen(player, p, "he", objectName()), p, player);

		return false;
	}
};

ThZushaCard::ThZushaCard(){
}

bool ThZushaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.isEmpty() && to_select != Self;
}

void ThZushaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	targets.first()->gainMark("@zuzhou");
}

class ThZushaViewAsSkill:public OneCardViewAsSkill{
public:
	ThZushaViewAsSkill():OneCardViewAsSkill("thzusha"){
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThZushaCard");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->getSuit() == Card::Spade && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThZushaCard *card = new ThZushaCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThZusha:public TriggerSkill{
public:
	ThZusha():TriggerSkill("thzusha"){
		events << EventPhaseStart;
		view_as_skill = new ThZushaViewAsSkill;
	}

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && target->getMark("@zuzhou") > 0;
    }

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		if (player->getPhase() == Player::Judge)
		{
			LogMessage log;
			log.type = "#ThZusha";
			log.from = player;
			log.arg = objectName();
			room->sendLog(log);

			JudgeStruct judge;
			judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
			judge.good = true;
			judge.who = player;
			judge.reason = objectName();

			room->judge(judge);

			if (judge.isGood())
				player->loseMark("@zuzhou");
			else
				room->loseHp(player);
		}

		return false;
	}
};

ThYaomeiCard::ThYaomeiCard(){
}

bool ThYaomeiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.length() < 2 && to_select->isWounded();
}

bool ThYaomeiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void ThYaomeiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	source->loseMark("@yaomei");
	source->gainMark("@yaomeiused");
	int a = targets.at(0)->getLostHp();
	int b = targets.at(1)->getLostHp();
	ServerPlayer *ta;
	ServerPlayer *tb;
	if (a < b)
	{
		ta = targets.at(0);
		tb = targets.at(1);
	}
	else if (a > b)
	{
		ta = targets.at(1);
		tb = targets.at(0);
	}
	else
	{
		ta = room->askForPlayerChosen(source, targets, "thyaomei");
		targets.removeOne(ta);
		tb = targets.first();
	}

	if (!ta || !tb)
		return;

	room->loseHp(ta);
	RecoverStruct recover;
	recover.who = source;
	room->recover(tb, recover);
}

class ThYaomeiViewAsSkill:public OneCardViewAsSkill{
public:
	ThYaomeiViewAsSkill():OneCardViewAsSkill("thyaomei"){
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@yaomei") > 0;
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isRed() && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThYaomeiCard *card = new ThYaomeiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThYaomei:public TriggerSkill{
public:
	ThYaomei():TriggerSkill("thyaomei"){
		view_as_skill = new ThYaomeiViewAsSkill;
		frequency = Limited;
		events << GameStart;
	}

	virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const{
		player->gainMark("@yaomei");

		return false;
	}
};

class ThZhongjie:public TriggerSkill{
public:
	ThZhongjie():TriggerSkill("thzhongjie"){
		events << DamageInflicted;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (player->getArmor())
			return false;

		DamageStruct damage = data.value<DamageStruct>();
        if(damage.damage > 1){
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

			damage.damage = 1;
            data = QVariant::fromValue(damage);
        }
		return false;
	}
};

class ThWeide: public TriggerSkill{
public:
	ThWeide():TriggerSkill("thweide"){
		events << EventPhaseStart;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		if(player->getPhase() != Player::Draw || !player->isWounded() || !player->askForSkillInvoke(objectName()))
			return false;

		int x = player->getLostHp();
		ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
		target->drawCards(x);

		int n = 0;
		foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
			if(p->getHandcardNum() > n)
				n = p->getHandcardNum();
		}

		if(n == 0)
			return true;

		QList<ServerPlayer *> targets;
		foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
			if(p->getHandcardNum() == n)
				targets << p;
		}

		target = room->askForPlayerChosen(player, targets, objectName());
        DummyCard *dummy = room->askForCardsChosen(player, target, "h", objectName(), x);
        if (dummy->subcardsLength() > 0)
            player->obtainCard(dummy, false);
        dummy->deleteLater();

		return false;
	}
};

class ThXiangrui: public TriggerSkill{
public:
	ThXiangrui():TriggerSkill("thxiangrui"){
		events << EventPhaseStart;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		if (player->getPhase() != Player::Discard || player->getHandcardNum() == player->getHpPoints())
			return false;

		int x = player->getHandcardNum() - player->getHpPoints();
		if (player->askForSkillInvoke(objectName()))
		{
			if (x > 0)
			{
				ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
				target->drawCards(x);
			}
			else if (x < 0)
				player->drawCards(-x);
		}

		return false;
	}
};

ThXingxieCard::ThXingxieCard(){
	target_fixed = true;
}

void ThXingxieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	source->loseMark("@xingxie");
	source->gainMark("@xingxieused");

	foreach(ServerPlayer *p, room->getOtherPlayers(source))
	{
		DummyCard *card = new DummyCard;
		card->addSubcards(p->getEquips());

		if (card->subcardsLength() == 0)
			break;

		p->obtainCard(card);
	}
}

class ThXingxieViewAsSkill: public ViewAsSkill{
public:
	ThXingxieViewAsSkill():ViewAsSkill("thxingxie"){
	}

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped();
    }

	virtual const Card *viewAs(const QList<const Card *> &cards) const {
		if (cards.length() < Self->getHandcardNum())
            return NULL;

		ThXingxieCard *card = new ThXingxieCard;
        card->addSubcards(cards);
        return card;
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
		foreach(const Player *p, player->getSiblings())
			if (p->getEquips().isEmpty())
				return false;
		return player->getMark("@xingxie") > 0 && !player->isKongcheng();
	}
};

class ThXingxie: public TriggerSkill{
public:
	ThXingxie():TriggerSkill("thxingxie"){
		events << GameStart;
		frequency = Limited;
		view_as_skill = new ThXingxieViewAsSkill;
	}

	virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const{
		player->gainMark("@xingxie");

		return false;
	}
};

ThLuanshenCard::ThLuanshenCard(){
	will_throw = false;
	handling_method = MethodNone;
}

bool ThLuanshenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.isEmpty() && to_select != Self;
}

void ThLuanshenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	room->fillAG(getSubcards());
	room->getThread()->delay(2000);
	ServerPlayer *target = targets.first();
	QStringList choices;
	if (target->getCardCount(true) >= subcardsLength())
		choices << "discard";

	choices << "turnover";
	
	QString choice = room->askForChoice(target, "thluanshen", choices.join("+"));

	room->broadcastInvoke("clearAG");

	if (choice == "discard")
	{
		room->throwCard(this, source, target);
		room->askForDiscard(target, "thluanshen", subcardsLength(), subcardsLength(), false, true);
	}
	else
	{
		target->obtainCard(this);
		if (target->getHandcardNum() < target->getMaxHp())
			target->drawCards(target->getMaxHp() - target->getHandcardNum());

		target->turnOver();
	}
};

class ThLuanshen: public ViewAsSkill{
public:
	ThLuanshen():ViewAsSkill("thluanshen"){
	}

	virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped();
    }

	virtual const Card *viewAs(const QList<const Card *> &cards) const {
		if (cards.isEmpty())
            return NULL;

		ThLuanshenCard *card = new ThLuanshenCard;
        card->addSubcards(cards);
        return card;
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("ThLuanshenCard") && player->getHandcardNum() > player->getHpPoints();
	}
};

class ThSilian: public FilterSkill{
public:
	ThSilian():FilterSkill("thsilian"){
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

class ThSilianGet: public TriggerSkill{
public:
	ThSilianGet():TriggerSkill("#thsilian"){
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

class ThSilianTriggerSkill: public TriggerSkill{
public:
	ThSilianTriggerSkill():TriggerSkill("#thsilian-weapon"){
		events << SlashMissed;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (player->getWeapon())
			return false;

		SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if (!effect.to->isAlive() || effect.to->getMark("Equips_of_Others_Nullified_to_You") > 0)
            return false;
        if (!effect.from->canSlash(effect.to, NULL, false))
            return false;

        const Card *card = NULL;
        card = room->askForCard(player, "slash", "blade-slash:" + effect.to->objectName(), QVariant(), Card::MethodUse, effect.to);
        if(card){
            room->setEmotion(player, "weapon/blade");
            // if player is drank, unset his flag
            if(player->hasFlag("drank"))
                room->setPlayerFlag(player, "-drank");

            LogMessage log;
            log.type = "#BladeUse";
            log.from = effect.from;
            log.to << effect.to;
            room->sendLog(log);

            CardUseStruct use;
            use.card = card;
            use.from = player;
            use.to << effect.to;
            room->useCard(use, false);
        }

        return false;
    }
};

ThLingzhanCard::ThLingzhanCard(){
}

bool ThLingzhanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	Slash *slash = new Slash(NoSuitNoColor, 0);
    return slash->targetFilter(targets, to_select, Self);
}

const Card *ThLingzhanCard::validate(const CardUseStruct *card_use) const{
	ServerPlayer *player = card_use->from;
	Room *room = player->getRoom();

	QList<int> card_ids = player->getPile("lingzhanpile");
	if(card_ids.isEmpty())
		return NULL;
	room->fillAG(card_ids, NULL);
	int card_id = room->askForAG(player, card_ids, true, "thlingzhan");
	foreach(ServerPlayer *p, room->getPlayers())
		p->invoke("clearAG");

	if(card_id == -1)
		return NULL;

	const Card *card = Sanguosha->getCard(card_id);
	Card *use_card = Sanguosha->cloneCard("slash", card->getSuit(), card->getNumber());
	use_card->addSubcard(card);
	use_card->setSkillName("thlingzhan");
	use_card->deleteLater();
	foreach(ServerPlayer *p, card_use->to)
		if(player->isProhibited(p, use_card))
			return NULL;
	
	return use_card;
}

const Card *ThLingzhanCard::validateInResposing(ServerPlayer *player, bool &continuable) const{
	continuable = true;
	Room *room = player->getRoom();

	QList<int> card_ids = player->getPile("lingzhanpile");
	if(card_ids.isEmpty())
		return NULL;
	room->fillAG(card_ids, NULL);
	int card_id = room->askForAG(player, card_ids, true, "thlingzhan");
	foreach(ServerPlayer *p, room->getPlayers())
		p->invoke("clearAG");

	if(card_id == -1)
		return NULL;

	const Card *card = Sanguosha->getCard(card_id);
	Card *use_card = Sanguosha->cloneCard("slash", card->getSuit(), card->getNumber());
	use_card->addSubcard(card);
	use_card->setSkillName("thlingzhan");
	use_card->deleteLater();
	
	return use_card;
}

class ThLingzhanViewAsSkill: public ZeroCardViewAsSkill{
public:
	ThLingzhanViewAsSkill():ZeroCardViewAsSkill("thlingzhan"){
	}

	virtual const Card *viewAs() const{
		return new ThLingzhanCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("lingzhanpile").isEmpty() && Slash::IsAvailable(player, NULL);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" && !ClientInstance->hasNoTargetResponding() && !player->getPile("lingzhanpile").isEmpty();
    }
};

class ThLingzhan: public TriggerSkill{
public:
	ThLingzhan():TriggerSkill("thlingzhan"){
		events << Damage << CardAsked;
		view_as_skill = new ThLingzhanViewAsSkill;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if(triggerEvent == CardAsked) {
			QString pattern = data.toStringList().first();
			if(pattern != "slash")
				return false;
                
			QList<int> card_ids = player->getPile("lingzhanpile");
			if(card_ids.isEmpty())
				return false;

			if(!player->askForSkillInvoke(objectName()))
				return false;

			room->broadcastSkillInvoke(objectName());

			room->fillAG(card_ids, NULL);
			int card_id = room->askForAG(player, card_ids, true, "thlingzhan");
			foreach(ServerPlayer *p, room->getPlayers())
				p->invoke("clearAG");

			if(card_id == -1)
				return false;

			const Card *card = Sanguosha->getCard(card_id);
			Card *slash = Sanguosha->cloneCard("slash", card->getSuit(), card->getNumber());
			slash->addSubcard(card);
			slash->setSkillName("thlingzhan");
			room->provide(slash);
		}
		else if(triggerEvent == Damage) {
			DamageStruct damage = data.value<DamageStruct>();
			if(damage.card->isKindOf("Slash")
					&& !damage.chain
					&& !damage.transfer
					&& player->askForSkillInvoke(objectName()))
			{
				JudgeStruct judge;
				judge.pattern = QRegExp("(.*):(heart):(.*)");
				judge.good = false;
				judge.who = player;
				judge.reason = objectName();

				room->judge(judge);

				if (judge.isGood())
					player->addToPile("lingzhanpile", judge.card, true);
			}
		}
		
		return false;
	}
};

class ThZhanfu: public TriggerSkill{
public:
	ThZhanfu():TriggerSkill("thzhanfu"){
		events << EventPhaseStart;
	}

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data) const{
		ServerPlayer *player = room->findPlayerBySkillName(objectName());
		if(!player || player == target || target->getPhase() != Player::Play || target->isKongcheng() || !player->askForSkillInvoke(objectName()))
			return false;

		const Card *card = room->askForCardShow(target, player, "@thzhanfuchoose:" + player->getGeneralName());
		if(card != NULL) {
			QString choice = room->askForChoice(player, objectName(), "basic+equip+trick");
			room->showCard(target, card->getId());
			if(card->getType() == choice)
				room->loseHp(target);
			else if(!player->isNude())
				room->askForDiscard(player, objectName(), 1, 1, false, true);
		}
		
		int id = card->getId();
		room->setPlayerMark(target, "zhanfumark", id + 1);
		room->setPlayerCardLimitation(target, "use", "^" + QString::number(id), true);

		return false;
	}
};

class ThZhanfuClear: public TriggerSkill{
public:
	ThZhanfuClear():TriggerSkill("#thzhanfu"){
		events << CardsMoveOneTime;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
		CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
		if(!move->from)
			return false;

		ServerPlayer *player = (ServerPlayer *)move->from;
		if (player->getMark("zhanfumark") <= 0)
			return false;

		int id = player->getMark("zhanfumark") - 1;
		
		int reason = move->reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
		if (reason != CardMoveReason::S_REASON_USE
			&& reason != CardMoveReason::S_REASON_RESPONSE
			&& reason != CardMoveReason::S_REASON_DISCARD)
			return false;

		bool invoke = false;
		for (int i = 0; i < move->card_ids.length(); i++)
			if (move->card_ids.at(i) == id) {
				invoke = true;
				break;
			}

		if (invoke) {
			room->setPlayerMark(player, "zhanfumark", 0);
			room->removePlayerCardLimitation(player, "use", "^" + QString::number(id) + "@1");
		}

		return false;
	}
};

BangaiPackage::BangaiPackage()
    :Package("bangai")
{
	General *bangai001 = new General(this, "bangai001", "shu");
	bangai001->addSkill(new ThBianfang);

	General *bangai003 = new General(this, "bangai003", "wu");
	bangai003->addSkill(new ThBanyue);

	General *bangai004 = new General(this, "bangai004", "qun", 3, false);
	bangai004->addSkill(new ThZusha);
	bangai004->addSkill(new ThYaomei);
	bangai004->addSkill(new ThZhongjie);

	General *bangai010 = new General(this, "bangai010", "wei");
	bangai010->addSkill(new ThWeide);

	General *bangai011 = new General(this, "bangai011", "wu");
	bangai011->addSkill(new ThXiangrui);
	bangai011->addSkill(new ThXingxie);

	General *bangai013 = new General(this, "bangai013", "shu");
	bangai013->addSkill(new ThLuanshen);

	General *bangai014 = new General(this, "bangai014", "wei");
	bangai014->addSkill(new ThSilian);
	bangai014->addSkill(new ThSilianGet);
	bangai014->addSkill(new ThSilianTriggerSkill);
	bangai014->addSkill(new ThLingzhan);
    related_skills.insertMulti("thsilian", "#thsilian");
    related_skills.insertMulti("thsilian", "#thsilian-weapon");

	General *bangai016 = new General(this, "bangai016", "qun");
	bangai016->addSkill(new ThZhanfu);
	bangai016->addSkill(new ThZhanfuClear);
    related_skills.insertMulti("thzhanfu", "#thzhanfu");
	
    addMetaObject<ThZushaCard>();
    addMetaObject<ThYaomeiCard>();
    addMetaObject<ThLingzhanCard>();
    addMetaObject<ThXingxieCard>();
    addMetaObject<ThLuanshenCard>();
}

ADD_PACKAGE(Bangai)