#include "kami.h"

#include "general.h"
#include "skill.h"
#include "room.h"
#include "carditem.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"

class ThKexing: public TriggerSkill{
public:
	ThKexing(): TriggerSkill("thkexing"){
		events << EventPhaseStart;
		frequency = Frequent;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (player->getPhase() != Player::Start || !player->askForSkillInvoke(objectName()))
			return false;

		QList<int> card_ids = room->getNCards(3, false);
		
		while (!card_ids.isEmpty())
		{
			room->fillAG(card_ids, player);
			CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
			bool broken = false;
			foreach (int id, card_ids)
			{
				Card *card = Sanguosha->getCard(id);
				if (card->isKindOf("TrickCard"))
				{
					room->moveCardTo(card, NULL, NULL, Player::DiscardPile, reason, true);
					card_ids.removeOne(id);
					player->invoke("clearAG");
					broken = true;
					break;
				}
			}

			if (broken)
				continue;

			if (card_ids.isEmpty())
				return false;

            int card_id = room->askForAG(player, card_ids, true, objectName());
			if (card_id == -1)
			{
				player->invoke("clearAG");
				break;
			}

			Card *card = Sanguosha->getCard(card_id);
			room->moveCardTo(card, NULL, NULL, Player::DiscardPile, reason, true);

			card_ids.removeOne(card_id);
			player->invoke("clearAG");
		}


		if (card_ids.isEmpty())
			return false;

		room->askForYuxi(player, card_ids, true);

		return false;
	}
};

ThShenfengCard::ThShenfengCard(){
	will_throw = false;
	handling_method = MethodNone;
}

bool ThShenfengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.isEmpty() && to_select->getHpPoints() > Self->getHpPoints();
}

void ThShenfengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	ServerPlayer *target = targets.first();
	target->obtainCard(this);
	QList<ServerPlayer *> victims;
	foreach(ServerPlayer *p, room->getOtherPlayers(target))
		if (target->distanceTo(p) == 1)
			victims << p;

	if (victims.isEmpty())
		return;

	ServerPlayer *victim = room->askForPlayerChosen(source, victims, "thshenfeng");

	if (!victim)
		victim = victims.first();

	DamageStruct damage;
	damage.from = target;
	damage.to = victim;
	room->damage(damage);
}

class ThShenfeng: public ViewAsSkill{
public:
	ThShenfeng(): ViewAsSkill("thshenfeng"){
	}

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
		if (selected.length() >= 2)
			return false;
		else if (!selected.isEmpty())
			return to_select->getSuit() == selected.first()->getSuit();
		else
			return true;
	}

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
		if (cards.length() != 2)
			return NULL;
		else
		{
			ThShenfengCard *card = new ThShenfengCard;
			card->addSubcards(cards);
			return card;
		}
	}
	
	virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("ThShenfengCard");
	}
};

class ThKaihai: public TriggerSkill{
public:
	ThKaihai(): TriggerSkill("thkaihai"){
		events << CardsMoveOneTime;
		frequency = Frequent;		
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
        if (move->from == player && move->from_places.contains(Player::PlaceHand) && player->isKongcheng()){
            if (player->askForSkillInvoke(objectName())){
                room->broadcastSkillInvoke(objectName());
                QList<CardsMoveStruct> moves;
				QList<int> card_ids;
				QList<int> notify_card_ids;
				
				QList<int> &drawPile = room->getDrawPile();
				if(drawPile.isEmpty())
					room->swapPile();
				RoomThread *thread = room->getThread();
				thread->trigger(FetchDrawPileCard, room, NULL);
				int card_id = drawPile.takeLast();
				card_ids << card_id;
				const Card *card = Sanguosha->getCard(card_id);

				QVariant data1 = QVariant::fromValue(card_id);
				if (thread->trigger(CardDrawing, room, player, data1))
					return false;

				player->drawCard(card);

				notify_card_ids << card_id;

				// update place_map & owner_map
				room->setCardMapping(card_id, player, Player::PlaceHand);
				
				if(notify_card_ids.isEmpty())
					return false;

				CardsMoveStruct move1;
				move1.card_ids = card_ids;
				move1.from = NULL; move1.from_place = Player::DrawPile;
				move1.to = player; move1.to_place = Player::PlaceHand; move1.to_player_name = player->objectName();     
				moves.append(move1);

				room->notifyMoveCards(true, moves, false);
				room->updateCardsOnLose(move1);
				room->notifyMoveCards(false, moves, false);
				room->updateCardsOnGet(move1);
				QVariant data2 = QVariant::fromValue(1);
				thread->trigger(CardDrawnDone, room, player, data2);
            }
        }
        return false;
	}
};

class ThJiguang: public TriggerSkill{
public:
	ThJiguang(): TriggerSkill("thjiguang"){
		events << EventPhaseStart << Death;
		frequency = Frequent;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		if ((triggerEvent == Death || player->getPhase() == Player::RoundStart) && !player->tag.value("ThJiguang").toString().isEmpty())
		{
			QString name = player->tag.value("ThJiguang").toString();
			if (triggerEvent != Death)
				player->loseAllMarks("@" + name);
			if (name == "jgfengyu")
				foreach(ServerPlayer *p, room->getAllPlayers())
					room->setPlayerMark(p, "jgfengyu", 0);
			else if (name == "jgtantian")
				foreach(ServerPlayer *p, room->getAllPlayers())
					room->detachSkillFromPlayer(p, "thjiguanggivenskill", true);
		}
		else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start)
		{
			QStringList choices;
			choices << "jglieri" << "jgfengyu" << "jghuangsha" << "jgtantian" << "jgnongwu";
			if (!player->askForSkillInvoke(objectName()))
				player->tag.remove("ThJiguang");
			else
			{
				QString name = player->tag.value("ThJiguang").toString();
				choices.removeOne(name);
				QString choice = room->askForChoice(player, objectName(), choices.join("+"));
				player->tag["ThJiguang"] = QVariant::fromValue(choice);
				player->gainMark("@" + choice);
				if (choice == "jgfengyu")
					foreach(ServerPlayer *p, room->getAllPlayers())
						room->setPlayerMark(p, "jgfengyu", 1);
				if (choice == "jgtantian")
					foreach(ServerPlayer *p, room->getAllPlayers())
						room->attachSkillToPlayer(p, "thjiguanggivenskill");
			}
		}

		return false;
	}
};

class ThJiguangBuff: public TriggerSkill{
public:
	ThJiguangBuff():TriggerSkill("#thjiguang"){
		events << DamageInflicted << Damage;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		ServerPlayer *splayer = room->findPlayerBySkillName("thjiguang");
		if (splayer == NULL)
			return false;
		DamageStruct damage = data.value<DamageStruct>();
		if (triggerEvent == DamageInflicted)
			if (damage.nature == DamageStruct::Fire && splayer->getMark("@jglieri") > 0)
			{
				damage.damage++;
                data = QVariant::fromValue(damage);
				return false;
			}
			else if (damage.damage > 1 && splayer->getMark("@jghuangsha") > 0)
			{
				damage.damage = 1;
                data = QVariant::fromValue(damage);
				return false;
			}
			else
				return false;
		else if (triggerEvent == Damage)
			if (splayer->getMark("@jgnongwu") > 0 && player->distanceTo(damage.to) <= 1
				&& damage.card->isKindOf("Slash"))
			{
				RecoverStruct recover;
				recover.who = splayer;
				room->recover(player, recover);
			}

		return false;
	}
};

class ThJiguangDistanceSkill: public DistanceSkill{
public:
	ThJiguangDistanceSkill(): DistanceSkill("thjiguangdis"){
	}

	virtual int getCorrect(const Player *from, const Player *to) const{
        if (from->getMark("jgfengyu") > 0)
			return -1;
		else
			return 0;
    }
};

class ThJiguangGivenSkill: public OneCardViewAsSkill{
public:
	ThJiguangGivenSkill(): OneCardViewAsSkill("thjiguanggivenskill"){
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
		Peach *peach = new Peach(Card::NoSuitNoColor, 0);
		return peach->isAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "peach" || pattern == "peach+analeptic";
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isKindOf("Jink") && to_select->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Peach *card = new Peach(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard->getId());
        card->setSkillName(objectName());
        return card;
    }
};

class ThFanhun: public TriggerSkill{
public:
    ThFanhun(): TriggerSkill("thfanhun") {
        events << AskForPeaches;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target);
    }

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        if(dying_data.who != player)
            return false;

        if(player->hasSkill("thmanxiao") && player->askForSkillInvoke(objectName(), data)){
            room->broadcastSkillInvoke(objectName());

            room->setPlayerProperty(player, "hp", qMin(1, player->getMaxHp()));
			player->gainMark("@yingxiao");
			if (player->getMark("@yingxiao") >= 4 && player->hasSkill("thmanxiao"))
			{
				LogMessage log;
				log.type = "#TriggerSkill";
				log.from = player;
				log.arg = "thmanxiao";
				room->sendLog(log);
				player->getRoom()->killPlayer(player, NULL);
			}

            if(player->isChained()){
                if(dying_data.damage == NULL || dying_data.damage->nature == DamageStruct::Normal)
                    room->setPlayerProperty(player, "chained", false);
            }
            if(!player->faceUp())
                player->turnOver();
        }

        return false;
    }
};

class ThYoushang: public TriggerSkill{
public:
	ThYoushang(): TriggerSkill("thyoushang"){
		events << DamageCaused;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
		if (damage.card && damage.card->isRed() && !damage.chain && !damage.transfer
			&& player->hasSkill("thmanxiao") && player->askForSkillInvoke(objectName()))
		{
			room->loseMaxHp(damage.to);
			player->gainMark("@yingxiao");
			if (player->getMark("@yingxiao") >= 4 && player->hasSkill("thmanxiao"))
			{
				LogMessage log;
				log.type = "#TriggerSkill";
				log.from = player;
				log.arg = "thmanxiao";
				room->sendLog(log);
				player->getRoom()->killPlayer(player, NULL);
			}

			return true;
        }

        return false;
    }
};

ThYouyaCard::ThYouyaCard(){
}

bool ThYouyaCard::targetFilter(const QList<const Player *> &selected, const Player *, const Player *) const{
	return selected.length() < Self->getMark("@yingxiao");
}

void ThYouyaCard::onEffect(const CardEffectStruct &effect) const{
	Room *room = effect.from->getRoom();
	if (!room->askForCard(effect.to, "jink", "@thyouya-jink:" + effect.from->objectName(), QVariant(), Card::MethodResponse, effect.from))
	{
		int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thyouya");
		room->moveCardTo(Sanguosha->getCard(card_id),
						 effect.from,
						 Player::PlaceHand,
						 room->getCardPlace(card_id) == Player::PlaceEquip);
	}
}

class ThYouyaViewAsSkill: public OneCardViewAsSkill{
public:
    ThYouyaViewAsSkill():OneCardViewAsSkill("thyouya"){

    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@thyouya";
    }

    virtual bool viewFilter(const Card* to_select) const{
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThYouyaCard *card = new ThYouyaCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThYouya: public TriggerSkill{
public:
    ThYouya(): TriggerSkill("thyouya"){
		events << DamageInflicted;
        view_as_skill = new ThYouyaViewAsSkill;
    }

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
		if (player->getMark("@yingxiao") > 0)
			room->askForUseCard(player, "@@thyouya", "@thyouya");

        return false;
    }
};

class ThManxiao: public MaxCardsSkill{
public:
	ThManxiao(): MaxCardsSkill("thmanxiao"){
	}

	virtual int getExtra(const Player *target) const{
		if (target->hasSkill(objectName()))
			return target->getMark("@yingxiao");
		else
			return 0;
	}
};

ThJinluCard::ThJinluCard(){
}

bool ThJinluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void ThJinluCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();

    DummyCard *dummy_card = new DummyCard;
    foreach(const Card *cd, effect.to->getCards("he")){
        dummy_card->addSubcard(cd);
    }
    if (!effect.to->isKongcheng())
    {
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, effect.from->objectName(),
            effect.to->objectName(), "thjinlu", QString());
        room->moveCardTo(dummy_card, effect.to, effect.from, Player::PlaceHand, reason, false);
    }
    effect.to->setFlags("ThJinluTarget");
	room->setPlayerFlag(effect.from, "ThJinluUsed");
}

class ThJinluViewAsSkill: public OneCardViewAsSkill{
public:
    ThJinluViewAsSkill():OneCardViewAsSkill("thjinlu"){

    }

    virtual bool viewFilter(const Card* to_select) const{
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThJinluCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new ThJinluCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThJinlu: public TriggerSkill{
public:
    ThJinlu():TriggerSkill("thjinlu"){
        events << EventPhaseEnd << EventPhaseStart;
        view_as_skill = new ThJinluViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
        if(triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play && player->hasFlag("ThJinluUsed")){
            ServerPlayer *target = NULL;
            foreach(ServerPlayer *other, room->getOtherPlayers(player)){
                if(other->hasFlag("ThJinluTarget")){
                    other->setFlags("-ThJinluTarget");
                    target = other;
                    break;
                }
            }

            if(!target || target->getHpPoints() < 1 || player->isNude())
                return false;

            DummyCard *to_goback;
            if(player->getCardCount(true) <= target->getHp())
            {
                to_goback = new DummyCard;
				if (!player->isKongcheng())
					to_goback->addSubcards(player->getCards("he"));
            }
            else
                to_goback = (DummyCard *)room->askForExchange(player, objectName(), target->getHp(), true, "ThJinluGoBack");

            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(),
                                  target->objectName(), objectName(), QString());
            reason.m_playerId = target->objectName();
            room->moveCardTo(to_goback, player, target, Player::PlaceHand, reason);
            delete to_goback;
        }
		else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish && player->hasFlag("ThJinluUsed"))
			player->turnOver();

        return false;
    }
};

class ThKuangli: public TriggerSkill{
public:
	ThKuangli(): TriggerSkill("thkuangli"){
		events << TurnedOver << HpChanged;
		frequency = Frequent;
	}
	
    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
		if (player->askForSkillInvoke(objectName()))
			player->drawCards(1);

		return false;
	}
};

class ThYuxin: public TriggerSkill{
public:
	ThYuxin(): TriggerSkill("thyuxin"){
		events << EventPhaseStart;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
		if (player->getPhase() != Player::Draw || player->isNude()
			|| !player->askForSkillInvoke(objectName())
			|| !room->askForDiscard(player, objectName(), 1, 1, true, true))
			return false;
		
		int card1 = room->drawCard();
        int card2 = room->drawCard();
        CardsMoveStruct move, move2;
        move.card_ids.append(card1);
        move.card_ids.append(card2);
        move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        move.to_place = Player::PlaceTable;
        room->moveCardsAtomic(move, true);
        room->getThread()->delay();
        move2 = move;
        move2.to_place = Player::PlaceHand;
        move2.to = player;
        move2.reason.m_reason = CardMoveReason::S_REASON_DRAW;
        room->moveCardsAtomic(move2, true);
		if (Sanguosha->getEngineCard(card1)->getSuit() == Card::Heart
			&& Sanguosha->getEngineCard(card2)->getSuit() == Card::Heart)
		{
			QList<ServerPlayer *> targets;
			foreach(ServerPlayer *p, room->getAllPlayers())
				if (p->isWounded())
					targets << p;

			if (!targets.isEmpty())
			{
				ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
				RecoverStruct recover;
				recover.who = player;
				room->recover(target, recover);
			}
		}

		return false;
	}
};

GongxinCard::GongxinCard(){
    once = true;
}

bool GongxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty(); 
}

void GongxinCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->doGongxin(effect.from, effect.to);
}

class Gongxin: public ZeroCardViewAsSkill{
public:
    Gongxin():ZeroCardViewAsSkill("gongxin"){
        default_choice = "discard";
    }

    virtual const Card *viewAs() const{
        return new GongxinCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("GongxinCard");
    }
};

ThChuangxinCard::ThChuangxinCard(){
	target_fixed = true;
}

void ThChuangxinCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	if (subcardsLength() == 1)
	{
		QString choice = room->askForChoice(source, "thchuangxin", "gongxin+zhuoyue");
		room->acquireSkill(source, choice);
	}
	else
	{
		room->acquireSkill(source, "gongxin");
		room->acquireSkill(source, "zhuoyue");
	}
};

class ThChuangxinViewAsSkill :public ViewAsSkill{
public:
	ThChuangxinViewAsSkill():ViewAsSkill("thchuangxin"){
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

	virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern=="@@thchuangxin";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const{
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 0) 
            return NULL;

        ThChuangxinCard *card = new ThChuangxinCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThChuangxin: public TriggerSkill{
public:
	ThChuangxin(): TriggerSkill("thchuangxin"){
		events << EventPhaseStart;
		view_as_skill = new ThChuangxinViewAsSkill;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
		if (player->getPhase() == Player::Play)
			room->askForUseCard(player, "@@thchuangxin", "@thchuangxin");
		else if (player->getPhase() == Player::NotActive)
		{
			room->detachSkillFromPlayer(player, "gongxin");
			room->detachSkillFromPlayer(player, "zhuoyue");
		}

		return false;
	}
};

ThTianxinCard::ThTianxinCard(){
	target_fixed = true;
}

void ThTianxinCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	if (subcardsLength() == 1)
	{
		QString choice = room->askForChoice(source, "thtianxin", "yuxi+tiandu");
		room->acquireSkill(source, choice);
	}
	else
	{
		room->acquireSkill(source, "yuxi");
		room->acquireSkill(source, "tiandu");
	}
};

class ThTianxinViewAsSkill :public ViewAsSkill{
public:
	ThTianxinViewAsSkill():ViewAsSkill("thtianxin"){
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

	virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern=="@@thtianxin";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const{
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 0) 
            return NULL;

        ThTianxinCard *card = new ThTianxinCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThTianxin: public TriggerSkill{
public:
	ThTianxin(): TriggerSkill("thtianxin"){
		events << EventPhaseStart;
		view_as_skill = new ThTianxinViewAsSkill;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
		if (player->getPhase() == Player::RoundStart)
			room->askForUseCard(player, "@@thtianxin", "@thtianxin");
		else if (player->getPhase() == Player::NotActive)
		{
			room->detachSkillFromPlayer(player, "yuxi");
			room->detachSkillFromPlayer(player, "tiandu");
		}

		return false;
	}
};

/*class ThWunan: public TriggerSkill{
public:
	ThWunan(): TriggerSkill("thwunan"){
		events << CardUsed << HpRecovered << Damaged << CardResponded << Damage << PreHpReduced;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
		if (splayer == NULL)
			return false;

		if (triggerEvent == CardUsed)
		{
			CardUseStruct use = data.value<CardUseStruct>();
			if (!use.card->isKindOf("GodSalvation") && !use.card->isKindOf("AmazingGrace"))
				return false;
		}
		else if (triggerEvent == HpRecovered)
		{
			RecoverStruct recover = data.value<RecoverStruct>();
			int hp = player->getHpPoints();
			if (hp < 1 || hp - recover.recover > 0)
				return false;
		}
		else if (triggerEvent == Damaged)
		{
			DamageStruct damage = data.value<DamageStruct>();
			if (damage.nature != DamageStruct::Fire)
				return false;
		}
		else if (triggerEvent == CardResponded)
		{
			CardResponseStruct resp = data.value<CardResponseStruct>();
			if (!resp.m_card->isKindOf("Jink")
				|| resp.m_card->getSkillName() != "EightDiagram")
				return false;
		}
		else if (triggerEvent == Damage)
		{
			DamageStruct damage = data.value<DamageStruct>();
			if ()
		return false;
	}
};*/

class ThSanling: public TriggerSkill{
public:
	ThSanling(): TriggerSkill("thsanling"){
		events << EventPhaseStart;
		frequency = Compulsory;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
		if (splayer == NULL)
			return false;

		if (player->getPhase() == Player::NotActive && splayer->isKongcheng())
		{
			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = splayer;
			log.arg  = objectName();
			room->sendLog(log);
			room->loseMaxHp(splayer, 2);
		}

		return false;
	}
};

class ThBingzhang: public TriggerSkill{
public:
	ThBingzhang(): TriggerSkill("thbingzhang"){
		events << DamageInflicted << HpLost;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		if (player->getCardCount(true) >= 2 && player->askForSkillInvoke(objectName())
			&& room->askForDiscard(player, objectName(), 2, 2, true, true))
			return true;

		return false;
	}
};

class ThJiwu: public FilterSkill{
public:
	ThJiwu(): FilterSkill("thjiwu"){
	}

	virtual bool viewFilter(const Card* to_select) const{
		return to_select->isKindOf("Peach") || to_select->isKindOf("Analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(jink);
        return card;
    }
};

class ThJiwuTriggerSkill: public TriggerSkill{
public:
	ThJiwuTriggerSkill(): TriggerSkill("#thjiwu"){
		events << CardsMoveOneTime;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
		if (move->from == NULL || move->from != player)
			return false;

		if (player->getPhase() == Player::Discard)
			return false;

		LogMessage log;
		log.type = "#TriggerSkill";
		log.from = player;
		log.arg  = "thjiwu";
		for (int i = 0; i < move->card_ids.length(); i++)
			if ((move->from_places[i] == Player::PlaceHand
				&& move->to && move->to != player && (move->to_place == Player::PlaceHand || move->to_place == Player::PlaceEquip))
				|| // 上两行判断第二条件，下两行判断第一条件
				(move->to == NULL && move->to_place == Player::DiscardPile
				&& (Sanguosha->getCard(move->card_ids[i])->isKindOf("Jink")
					|| Sanguosha->getCard(move->card_ids[i])->isKindOf("Peach")
					|| Sanguosha->getCard(move->card_ids[i])->isKindOf("Analeptic"))))
			{
				room->sendLog(log);
				player->drawCards(1);
			}

		return false;
	}
};

class ThSisui: public TriggerSkill{
public:
	ThSisui(): TriggerSkill("thsisui"){
		events << EventPhaseStart;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		if (player->getPhase() == Player::Start && player->askForSkillInvoke(objectName()))
		{
			QList<int> card_ids;
			foreach (ServerPlayer *p, room->getOtherPlayers(player))
				if (!p->isKongcheng())
				{
					const Card *card = room->askForCard(p, ".", "@thsisui");
					if (!card)
					{
						card = p->getRandomHandCard();
						room->throwCard(card, p);
					}
					card_ids << card->getEffectiveId();
				}

			foreach (int card_id, card_ids)
				if (room->getCardPlace(card_id) != Player::DiscardPile)
					card_ids.removeOne(card_id);

			if (card_ids.isEmpty())
				return false;

			for (int i = 0; i < 3; i++)
			{
				room->fillAG(card_ids, NULL);
				int card_id = room->askForAG(player, card_ids, true, "thsisui");
				room->broadcastInvoke("clearAG");
				if (card_id == -1)
					break;
				card_ids.removeOne(card_id);
				ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName());
				room->obtainCard(target, card_id);
				if (card_ids.isEmpty())
					break;
			}
		}

		return false;
	}
};

class ThZhanying: public MaxCardsSkill{
public:
	ThZhanying(): MaxCardsSkill("thzhanying"){
	}

	virtual int getExtra(const Player *target) const{
		if (target->hasSkill(objectName()))
			return 4;
		else
			return 0;
	}
};

class ThZhanyingTriggerSkill: public TriggerSkill{
public:
	ThZhanyingTriggerSkill(): TriggerSkill("#thzhanying"){
		events << EventPhaseChanging;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (data.value<PhaseChangeStruct>().to == Player::Draw)
		{
			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = player;
			log.arg  = "thzhanying";
			room->sendLog(log);
			player->skip(Player::Draw);
		}
		return false;
	}
};

KamiPackage::KamiPackage()
    :Package("kami")
{

	General *kami001 = new General(this, "kami001", "god", 3);
	kami001->addSkill(new ThKexing);
	kami001->addSkill(new ThShenfeng);
	kami001->addSkill(new ThKaihai);

	General *kami002 = new General(this, "kami002", "god");
	kami002->addSkill(new ThJiguang);
	kami002->addSkill(new ThJiguangBuff);
    related_skills.insertMulti("thjiguang", "#thjiguang");

	General *kami007 = new General(this, "kami007", "god", 2, false);
	kami007->addSkill(new ThFanhun);
	kami007->addSkill(new ThYoushang);
	kami007->addSkill(new ThYouya);
	kami007->addSkill(new ThManxiao);

	General *kami008 = new General(this, "kami008", "god", 3);
	kami008->addSkill(new ThJinlu);
	kami008->addSkill(new ThKuangli);

	General *kami009 = new General(this, "kami009", "god");
	kami009->addSkill(new ThYuxin);
	kami009->addSkill(new ThChuangxin);
	kami009->addSkill(new ThTianxin);

	General *kami013 = new General(this, "kami013", "god", 1, false);
	kami013->addSkill(new ThSanling);
	kami013->addSkill(new ThBingzhang);
	kami013->addSkill(new ThJiwu);
	kami013->addSkill(new ThJiwuTriggerSkill);
    related_skills.insertMulti("thjiwu", "#thjiwu");
	kami013->addSkill(new ThSisui);
	kami013->addSkill(new ThZhanying);
	kami013->addSkill(new ThZhanyingTriggerSkill);
    related_skills.insertMulti("thzhanying", "#thzhanying");
	
    addMetaObject<ThShenfengCard>();
    addMetaObject<ThYouyaCard>();
	addMetaObject<ThJinluCard>();
    addMetaObject<GongxinCard>();
    addMetaObject<ThChuangxinCard>();
    addMetaObject<ThTianxinCard>();

	skills << new ThJiguangDistanceSkill << new ThJiguangGivenSkill << new Gongxin;
}

ADD_PACKAGE(Kami)