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
        attached_lord_skill = true;
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

class ThLianxie:public TriggerSkill{
public:
	ThLianxie():TriggerSkill("thlianxie"){
		events << EventPhaseStart;
		frequency = Wake;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (player->getPhase() != Player::Start || player->getMark("@lianxie") > 0)
			return false;

		if (player->getEquip(2) || player->getEquip(3))
		{
			LogMessage log;
			log.type = "#ThLianxieWake";
			log.from = player;
			log.arg = objectName();
			room->sendLog(log);

			room->loseMaxHp(player);
			room->acquireSkill(player, "kuanggu");
			player->gainMark("@lianxie");
		}

		return false;
	}
};

class ThKuangqi: public TriggerSkill {
public:
	ThKuangqi(): TriggerSkill("thkuangqi") {
		events << DamageCaused;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (player->getPhase() != Player::Play)
			return false;

		DamageStruct damage = data.value<DamageStruct>();
		if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))
			&& !damage.transfer && !damage.chain
			&& room->askForCard(player, "Peach,Analeptic,EquipCard", "@thkuangqi"))
		{
			LogMessage log;
			log.type = "#ThKuangqi";
			log.from = player;
			log.to << damage.to;
			log.arg  = QString::number(damage.damage);
			log.arg2 = QString::number(++damage.damage);
			room->sendLog(log);
			
			data = QVariant::fromValue(damage);
		}

		return false;
	}
};

ThKaiyunCard::ThKaiyunCard(){
    target_fixed = true;
}

void ThKaiyunCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
}

class ThKaiyunViewAsSkill:public OneCardViewAsSkill{
public:
    ThKaiyunViewAsSkill():OneCardViewAsSkill("thkaiyun"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@thkaiyun";
    }

    virtual bool viewFilter(const Card* to_select) const{
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new ThKaiyunCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThKaiyun: public TriggerSkill {
public:
	ThKaiyun(): TriggerSkill("thkaiyun") {
		view_as_skill = new ThKaiyunViewAsSkill;
		events << AskForRetrial;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		JudgeStar judge = data.value<JudgeStar>();
		if (player->isNude() || !room->askForUseCard(player, "@@thkaiyun", "@thkaiyun"))
			return false;
		
		if (judge->who == player)
			room->broadcastSkillInvoke(objectName(), qrand() % 2 + 3);
		else
			room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);

		QList<int> card_ids = room->getNCards(2, false);
		room->fillAG(card_ids, player);
		int card_id = room->askForAG(player, card_ids, false, objectName());
		card_ids.removeOne(card_id);
		player->invoke("clearAG");
		room->retrial(Sanguosha->getCard(card_id), player, judge, objectName(), false);
		
		room->moveCardTo(Sanguosha->getCard(card_ids.first()), player, Player::PlaceHand, false);

		return false;
	}
};

class Wumou:public TriggerSkill{
public:
    Wumou():TriggerSkill("wumou"){
        frequency = Compulsory;
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardStar card = NULL;
        if(triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }else if(triggerEvent == CardResponded)
            card = data.value<CardResponseStruct>().m_card;

        if(card->isNDTrick()){
            room->broadcastSkillInvoke(objectName());

            int num = player->getMark("@wrath");
            if(num >= 1 && room->askForChoice(player, objectName(), "discard+losehp") == "discard"){
                player->loseMark("@wrath");
            }else
                room->loseHp(player);
        }

        return false;
    }
};

class ThJiaotu: public TriggerSkill {
public:
	ThJiaotu(): TriggerSkill("thjiaotu") {
		events << Damaged << EventPhaseStart;
	}

	virtual bool triggerable(const ServerPlayer *target) const {
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == Damaged && TriggerSkill::triggerable(player))
		{
			DamageStruct damage = data.value<DamageStruct>();
			if (!damage.from)
				return false;

			for (int i = 0; i < damage.damage; i++)
			{
				if (!player->askForSkillInvoke(objectName()))
					break;

				room->broadcastSkillInvoke(objectName());

				JudgeStruct judge;
				judge.pattern = QRegExp("(.*):(heart):(.*)");
				judge.good = true;
				judge.reason = objectName();
				judge.who = damage.from;
				room->judge(judge);

				if (judge.isBad() && damage.from->getMark("@jiaotu") <= 0)
				{
					if (room->getCurrent() == damage.from)
						room->setPlayerFlag(damage.from, objectName());
					
					damage.from->gainMark("@jiaotu");
					room->acquireSkill(damage.from, "wumou");
				}
			}
		}
		else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive && player->getMark("@jiaotu") > 0)
		{
			if (!player->hasFlag(objectName()))
			{
				player->loseMark("@jiaotu");
				room->detachSkillFromPlayer(player, "wumou");
			}
		}

		return false;
	}
};

class ThShouye: public TriggerSkill {
public:
	ThShouye(): TriggerSkill("thshouye") {
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

class ThFenghuang: public TriggerSkill {
public:
	ThFenghuang(): TriggerSkill("thfenghuang") {
		events << ChainStateChanged;
		frequency = Frequent;
	}

	virtual bool triggerable(const ServerPlayer *target) const {
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *, QVariant &data) const{
		ServerPlayer *player = room->findPlayerBySkillName(objectName());
		if (!player)
			return false;

		if (player->askForSkillInvoke(objectName()))
			player->drawCards(1);

		return false;
	}
};

class ThKuaiqing: public TriggerSkill {
public:
	ThKuaiqing(): TriggerSkill("thkuaiqing") {
		events << TrickCardCanceling;
		frequency = Compulsory;
	}

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
	
	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.from && effect.from->hasSkill(objectName()) && effect.from->isAlive()
            && !player->isWounded())
		{
			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = effect.from;
			log.arg = objectName();
			room->sendLog(log);
            return true;
		}

		return false;
	}
};

class ThBumie: public TriggerSkill {
public:
	ThBumie(): TriggerSkill("thbumie") {
		events << DamageInflicted;
		frequency = Limited;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
		if (player->getMark("@bumie") <= 0)
			return false;

		DamageStruct damage = data.value<DamageStruct>();
		if (damage.from && damage.from->isAlive() && damage.from != player && player->askForSkillInvoke(objectName()))
		{
			damage.to = damage.from;
			damage.transfer = true;
			player->loseMark("@bumie");
			player->gainMark("@bumieused");
			room->damage(damage);
			if (damage.from->isAlive())
				damage.from->turnOver();
			return true;
		}

		return false;
	}
};

class ThCunjing: public TriggerSkill {
public:
	ThCunjing(): TriggerSkill("thcunjing") {
		events << SlashMissed;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
		SlashEffectStruct effect = data.value<SlashEffectStruct>();
		if (player->getWeapon())
			return false;
		if (room->askForCard(player, "..", "@thcunjing"))
		{
			LogMessage log;
			log.type = "#ThCunjing";
			log.from = player;
			log.to << effect.to;
			log.arg = objectName();
			room->sendLog(log);
			room->broadcastSkillInvoke(objectName());
			room->slashResult(effect, NULL);
			return true;
		}

		return false;
	}
};

ThLianhuaCard::ThLianhuaCard(){
	target_fixed = true;
}

void ThLianhuaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    source->drawCards(1); //It should be preview, not draw
    ServerPlayer *target = room->askForPlayerChosen(source, room->getAllPlayers(), "thlianhua");

	QList<const Card *> miji_cards = source->getHandcards().mid(source->getHandcardNum() - 1);
	foreach(const Card *card, miji_cards){
        CardMoveReason reason(CardMoveReason::S_REASON_PREVIEWGIVE, source->objectName());
        reason.m_playerId == target->objectName();
        room->obtainCard(target, card, reason, false);
    }
}

class ThLianhua: public OneCardViewAsSkill {
public:
	ThLianhua(): OneCardViewAsSkill("thlianhua") {
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThLianhuaCard");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isKindOf("EquipCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new ThLianhuaCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThQishu: public TriggerSkill {
public:
	ThQishu(): TriggerSkill("thqishu") {
		events << EventPhaseStart;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
	
	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
		ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
		if (!splayer || player == splayer)
			return false;

		if (player->getPhase() == Player::NotActive && !splayer->faceUp()
			&& splayer->askForSkillInvoke(objectName()))
		{
			room->broadcastSkillInvoke(objectName());
			splayer->turnOver();
			splayer->gainMark("@shiji");
			splayer->gainAnExtraTurn(player);
		}

		return false;
	}
};

class ThShiting: public TriggerSkill {
public:
	ThShiting(): TriggerSkill("thshiting") {
		events << EventPhaseStart;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
		if (player->getPhase() == Player::Start && player->getMark("@shiji") <= 0
			&& player->askForSkillInvoke(objectName()))
		{
			room->broadcastSkillInvoke(objectName());
			player->turnOver();
			if (player->getHandcardNum() < player->getMaxHp())
				player->drawCards(player->getMaxHp() - player->getHandcardNum());
			
			player->skip(Player::Judge);
			player->skip(Player::Draw);
			player->skip(Player::Play);
			player->skip(Player::Discard);
		}

		return false;
	}
};

class ThHuanzai: public TriggerSkill {
public:
	ThHuanzai(): TriggerSkill("thhuanzai") {
		events << EventPhaseStart;
		frequency = Compulsory;
	}
	
	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
		if (player->getPhase() == Player::Finish && player->getMark("@shiji") > 0)
		{
			room->broadcastSkillInvoke(objectName());
			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = player;
			log.arg  = objectName();
			room->sendLog(log);
			player->loseAllMarks("@shiji");
		}
		return false;
	}
};

ThShennaoCard::ThShennaoCard(){
	target_fixed = true;
	will_throw = false;
}

const Card *ThShennaoCard::validate(const CardUseStruct *card_use) const{
	Nullification *use_card;
	if (subcardsLength() != 1)
	{
		ServerPlayer *player = card_use->from;
		Room *room = player->getRoom();

		player->drawCards(1);
		room->loseHp(player);

		use_card = new Nullification(NoSuitNoColor, 0);
	}
	else
	{
		const Card *card = Sanguosha->getCard(getSubcards().first());
		use_card = new Nullification(card->getSuit(), card->getNumber());
		use_card->addSubcard(card);
	}
	use_card->setSkillName("thshennao");
	use_card->deleteLater();

	return use_card;
}

const Card *ThShennaoCard::validateInResposing(ServerPlayer *player, bool &continuable) const{
	Nullification *use_card;
	if (subcardsLength() != 1)
	{
		Room *room = player->getRoom();

		player->drawCards(1);
		room->loseHp(player);

		use_card = new Nullification(NoSuitNoColor, 0);
	}
	else
	{
		const Card *card = Sanguosha->getCard(getSubcards().first());
		use_card = new Nullification(card->getSuit(), card->getNumber());
		use_card->addSubcard(card);
	}
	use_card->setSkillName("thshennao");
	use_card->deleteLater();
	
	return use_card;
}

class ThShennao: public ViewAsSkill {
public:
	ThShennao(): ViewAsSkill("thshennao") {
	}

	virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
		if(Self->getHandcardNum() >= Self->getHpPoints())
		{
			return selected.isEmpty() && !to_select->isEquipped();
		}
		else
		{
			return false;
		}
	}

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
		if(Self->getHandcardNum() >= Self->getHpPoints() && cards.isEmpty())
		{
			return NULL;
		}
		ThShennaoCard *card = new ThShennaoCard;
		if (!cards.isEmpty())
			card->addSubcards(cards);
		return card;
	}
	
	virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "nullification";
    }

	virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
		return true;
	}
};

class ThMiaoyao: public TriggerSkill {
public:
	ThMiaoyao(): TriggerSkill("thmiaoyao") {
		events << CardResponded;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (player == NULL) return false;
        CardStar card_star = data.value<CardResponseStruct>().m_card;
        if (!card_star->isKindOf("Jink") || player->getHp() != 1)
            return false;

		if (player->isWounded() && player->askForSkillInvoke(objectName()))
		{
			RecoverStruct recover;
			recover.who = player;
			room->recover(player, recover);
		}

		return false;
	}
};

ThHeiguanCard::ThHeiguanCard() {
	will_throw = false;
	handling_method = MethodNone;
}

bool ThHeiguanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	if (!targets.isEmpty())
		return false;

	if (subcardsLength() == 0)
		return !to_select->isKongcheng() && to_select != Self;
	else
		return to_select != Self;
}

void ThHeiguanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
	ServerPlayer *target = targets.first();
	if (subcardsLength() == 0)
	{
		room->obtainCard(source,
			             room->askForCardChosen(source, target, "h", objectName()),
						 false);
		target->gainMark("@heiguan2");
	}
	else
	{
		target->obtainCard(this);
		target->gainMark("@heiguan1");
	}
}

class ThHeiguanViewAsSkill: public ViewAsSkill {
public:
	ThHeiguanViewAsSkill(): ViewAsSkill("thheiguan") {
	}

	virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
		return selected.isEmpty() && to_select->isBlack() && !to_select->isEquipped();
	}

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
		ThHeiguanCard *card = new ThHeiguanCard;
		if (!cards.isEmpty())
			card->addSubcards(cards);
		return card;
	}
	
	virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThHeiguanCard");
    }
};

class ThHeiguan: public ProhibitSkill {
public:
	ThHeiguan(): ProhibitSkill("thheiguan") {
		view_as_skill = new ThHeiguanViewAsSkill;
		frequency = NotFrequent;
	}

	virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        if (!card->isKindOf("Slash"))
			return false;

		if (to->hasSkill(objectName()) && from->getMark("@heiguan1") > 0)
			return true;
		else if (to->getMark("@heiguan2") > 0)
			return true;
		else
			return false;
    }
};

class ThHeiguanClear: public TriggerSkill {
public:
	ThHeiguanClear(): TriggerSkill("#thheiguan") {
		events << EventPhaseStart << Death;
	}

	virtual bool triggerable(const ServerPlayer *target) const {
		return target != NULL && target->hasSkill(objectName());
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == Death || player->getPhase() == Player::RoundStart)
		{
			foreach (ServerPlayer *p, room->getOtherPlayers(player))
			{
				room->setPlayerMark(p, "@heiguan1", 0);
				room->setPlayerMark(p, "@heiguan2", 0);
			}
		}

		return false;
	}
};

class ThAnyue: public FilterSkill{
public:
    ThAnyue():FilterSkill("thanyue"){

    }

    static WrappedCard *changeToHeart(int cardId){
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("thanyue");
        new_card->setSuit(Card::Spade);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        return changeToHeart(originalCard->getEffectiveId());
    }
};

class ThGeyong: public TriggerSkill {
public:
	ThGeyong(): TriggerSkill("thgeyong") {
		events << HpChanged;
		frequency = Frequent;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (player->askForSkillInvoke(objectName()))
		{
			JudgeStruct judge;
			judge.pattern = QRegExp("(.*):(heart):(.*)");
			judge.good = false;
			judge.reason = objectName();
			judge.who = player;
			room->judge(judge);
			if (judge.isGood())
				player->drawCards(1);
		}
		
		return false;
	}
};

ThMiquCard::ThMiquCard(){
	target_fixed = true;
	handling_method = MethodNone;
}

void ThMiquCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const {
	room->showAllCards(source);
    int samecount = 0,unsamecount = 0;
    foreach (const Card *card1, source->getHandcards())
		foreach (const Card *card2, source->getHandcards())
			if(card1 != card2)
				if(card1->getSuit() == card2->getSuit())
					samecount = samecount + 1;
				else
					unsamecount = unsamecount + 1;

	if (unsamecount == 0)
	{
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(source))
            if (!p->isNude())
                targets << p;

       	if(!targets.isEmpty())
		{
			ServerPlayer *target = room->askForPlayerChosen(source, targets, "thmiqu");
            int card_id = room->askForCardChosen(source, target, "he", "thmiqu");
			CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, source->objectName());
            room->obtainCard(source, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
		}
	}
	else if (samecount == 0)
	{
       	 ServerPlayer *target = room->askForPlayerChosen(source, room->getAlivePlayers(), "thmiqu");
		 room->loseHp(target);
	}
};

class ThMiqu: public ZeroCardViewAsSkill {
public:
	ThMiqu():ZeroCardViewAsSkill("thmiqu") {
	}

    virtual const Card *viewAs() const{
        return new ThMiquCard;
    }

	virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThMiquCard") && player->getHandcardNum() >= player->getHpPoints() && !player->isKongcheng();
    }
};

class ThZhehui: public TriggerSkill {
public:
	ThZhehui(): TriggerSkill("thzhehui") {
		events << Damaged << DamageInflicted << EventPhaseStart << EventLoseSkill;
		frequency = Compulsory;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == Damaged && TriggerSkill::triggerable(player))
		{
			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = player;
			log.arg = objectName();
			room->sendLog(log);
            player->addMark("zhehui");
        }
		else if (triggerEvent == DamageInflicted && TriggerSkill::triggerable(player))
		{
            if (player->getMark("zhehui") > 0)
			{
                LogMessage log;
				log.type = "#TriggerSkill";
				log.from = player;
				log.arg = objectName();
				room->sendLog(log);
                return true;
            }
        }
		else if (triggerEvent == EventLoseSkill)
		{
			if (player->getMark("zhehui") > 0 && data.toString() == objectName())
				player->removeMark("zhehui");
		}
		else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive)
		{
			ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
			if (!splayer || splayer->getMark("zhehui") == 0)
				return false;
			
			player->removeMark("zhehui");
		}
		else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish)
		{
			ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
			if (!splayer || splayer->getMark("zhehui") == 0)
				return false;

			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = splayer;
			log.arg = objectName();
			room->sendLog(log);

			splayer->removeMark("zhehui");
			QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(splayer))
                if (splayer->canSlash(p, NULL, false))
                    targets << p;
            QString choice;
            if (targets.isEmpty())
                choice = "discard";
            else
                choice = room->askForChoice(splayer, objectName(), "discard+slash");

            if (choice == "slash") {
                ServerPlayer *victim = room->askForPlayerChosen(splayer, targets, objectName());

                Slash *slash = new Slash(Card::NoSuitNoColor, 0);
                slash->setSkillName(objectName());
                CardUseStruct card_use;
                card_use.from = splayer;
                card_use.to << victim;
                card_use.card = slash;
                room->useCard(card_use, false);
            }
			else
			{
				int card_ids = room->askForCardChosen(splayer, player, "he", objectName());
                room->throwCard(card_ids, player, splayer);
            }
		}

		return false;
	}
};

ThExiCard::ThExiCard(){
}

bool ThExiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	if (targets.length() >= 2)
		return false;

	if (to_select == Self)
		return Self->getHandcardNum() > 1;
	else
		return !to_select->isKongcheng();
}

bool ThExiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
	return targets.length() == 2;
}

void ThExiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	QList<const Card *> cards;
	foreach (ServerPlayer *p, targets)
		cards << room->askForCardShow(p, source, "thexi");
	
	room->showCard(targets[0], cards[0]->getId());
	room->showCard(targets[1], cards[1]->getId());
	const Card *big, *small;
	ServerPlayer *target;
	if (cards[0]->getNumber() == cards[1]->getNumber())
		source->addMark("exi");
	else
	{
		if (cards[0]->getNumber() > cards[1]->getNumber())
		{
			big = cards[0];
			small = cards[1];
			target = targets[0];
		}
		else
		{
			big = cards[1];
			small = cards[0];
			target = targets[1];
		}

		if (target == source)
		{
			source->obtainCard(big);
			source->obtainCard(small);
		}
		else if (room->askForChoice(source, "thexi", "big+small") == "big")
		{
			source->obtainCard(big);
			target->obtainCard(small);
		}
		else
		{
			source->obtainCard(small);
			target->obtainCard(big);
		}
	}
}

class ThExiViewAsSkill: public OneCardViewAsSkill {
public:
	ThExiViewAsSkill(): OneCardViewAsSkill("thexi") {
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@thexi";
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new ThExiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThExi: public TriggerSkill {
public:
	ThExi(): TriggerSkill("thexi") {
		events << EventPhaseStart;
		view_as_skill = new ThExiViewAsSkill;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		if (player->getPhase() == Player::NotActive)
		{
			if (player->getMark("exi") > 0)
			{	
				player->removeMark("exi");
				player->addMark("exicount");
				player->gainAnExtraTurn(player);
			}
			else if (player->getMark("exicount") > 0)
				player->removeMark("exicount");
		}
		else if (player->getPhase() == Player::Finish && player->getMark("exicount") <= 0)
			room->askForUseCard(player, "@@thexi", "@thexi");

		return false;
	}
};

class ThXinglu: public TriggerSkill {
public:
	ThXinglu(): TriggerSkill("thxinglu") {
		events << Dying;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		QList<ServerPlayer *> targets;
		foreach (ServerPlayer *p, room->getOtherPlayers(player))
			if (!p->isKongcheng())
				targets << p;
		
		if (targets.isEmpty() || player->isKongcheng())
			return false;

		if (player->askForSkillInvoke(objectName()))
		{
			ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
			bool win = player->pindian(target, objectName());
			if (win)
			{
				RecoverStruct recover;
				recover.who = player;
				room->recover(player, recover);
			}
		}

		return false;
	}
};

class ThShenyou: public TriggerSkill {
public:
	ThShenyou(): TriggerSkill("thshenyou"){
		events << EventPhaseStart << Predamage;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (player->getPhase() != Player::Draw)
			return false;
		
		if (triggerEvent == Predamage)
		{
			DamageStruct damage = data.value<DamageStruct>();
			if (damage.card && damage.card->isKindOf("Slash")
				&& player->getMark(objectName()) > 0)
			{
				room->loseHp(damage.to, damage.damage);
				return true;
			}
		}
		else
		{
			QList<ServerPlayer *> targets;
			foreach (ServerPlayer *p, room->getOtherPlayers(player))
				if (!p->isAllNude())
					targets << p;

			if (targets.isEmpty())
				return false;

			if (player->askForSkillInvoke(objectName()))
			{
				ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
				int card_id = room->askForCardChosen(player, target, "hej", objectName());
				room->moveCardTo(Sanguosha->getCard(card_id), player, Player::PlaceHand, room->getCardPlace(card_id) != Player::PlaceHand);
				player->addMark(objectName());
				room->askForUseSlashTo(player, target, "@thshenyou:" + target->objectName(), true, false);
				player->removeMark(objectName());
				return true;
			}
		}

		return false;
	}
};

class ThGuixu: public TriggerSkill {
public:
	ThGuixu(): TriggerSkill("thguixu") {
		events << CardUsed << CardEffected;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == CardUsed)
		{
			CardUseStruct use = data.value<CardUseStruct>();
			ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
			if (!splayer)
				return false;
			if (!splayer->tag.value("ThGuixu").isNull())
				splayer->tag["ThGuixu"] = QVariant(QString());
			if (use.from == splayer || splayer->getPile("guixupile").length() > 1
				|| splayer == room->getCurrent())
				return false;
			if (use.card->isKindOf("TrickCard") && !use.card->isKindOf("Nullification"))
			{
				if (splayer->askForSkillInvoke(objectName()))
				{
					JudgeStruct judge;
					judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
					judge.good = true;
					judge.reason = objectName();
					judge.who = splayer;

					room->judge(judge);

					if (judge.isGood())
					{
						splayer->tag["ThGuixu"] = use.card->toString();
						splayer->addToPile("guixupile", use.card);
					}
				}
			}
		}
		else if (triggerEvent == CardEffected)
		{
			CardEffectStruct effect = data.value<CardEffectStruct>();
			ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
			if (!splayer)
				return false;
			if (!splayer->tag.value("ThGuixu").isNull()
				&& splayer->tag.value("ThGuixu").toString() == effect.card->toString())
				return true;
		}

		return false;
	}
};

ThTianqueCard::ThTianqueCard(){
	target_fixed = true;
}

void ThTianqueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	QList<int> card_ids = source->getPile("guixupile");
	room->fillAG(card_ids);
	int card_id = room->askForAG(source, card_ids, true, "thtianque");
	room->broadcastInvoke("clearAG");
	if (card_id == -1)
		return ;
	QString pattern = "." + Sanguosha->getCard(card_id)->getSuitString().left(1).toUpper();
	if (room->askForCard(source, pattern, "@thtianque:::" + Sanguosha->getCard(card_id)->getSuitString()))
		room->obtainCard(source, card_id);
}

class ThTianque: public ZeroCardViewAsSkill {
public:
	ThTianque(): ZeroCardViewAsSkill("thtianque") {
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->isKongcheng() && !player->getPile("guixupile").isEmpty();
    }

    virtual const Card *viewAs() const{
        return new ThTianqueCard;
    }
};

void TouhouPackage::addTsukiGenerals(){
	General *tsuki001 = new General(this, "tsuki001$", "qun");
	tsuki001->addSkill(new ThSuoming);
	tsuki001->addSkill(new ThChiwu);
	tsuki001->addSkill(new ThYewang);

	General *tsuki002 = new General(this, "tsuki002", "qun");
	tsuki002->addSkill(new ThJinguo);
	tsuki002->addSkill(new ThLianxie);

	General *tsuki003 = new General(this, "tsuki003", "qun");
	tsuki003->addSkill(new ThKuangqi);

	General *tsuki004 = new General(this, "tsuki004", "qun", 3, false);
	tsuki004->addSkill(new ThKaiyun);
	tsuki004->addSkill(new ThJiaotu);

	General *tsuki005 = new General(this, "tsuki005", "qun", 3);
	tsuki005->addSkill(new ThShouye);
	tsuki005->addSkill(new ThXushi);
	
	General *tsuki006 = new General(this, "tsuki006", "qun", 3);
	tsuki006->addSkill(new ThFenghuang);
	tsuki006->addSkill(new ThKuaiqing);
	tsuki006->addSkill(new ThBumie);
    tsuki006->addSkill(new MarkAssignSkill("@bumie", 1));
    related_skills.insertMulti("thbumie", "#@bumie-1");
	
	General *tsuki007 = new General(this, "tsuki007", "qun");
	tsuki007->addSkill(new ThCunjing);
	tsuki007->addSkill(new ThLianhua);
	
	General *tsuki008 = new General(this, "tsuki008", "qun", 3);
	tsuki008->addSkill(new ThQishu);
	tsuki008->addSkill(new ThShiting);
	tsuki008->addSkill(new ThHuanzai);

	General *tsuki009 = new General(this, "tsuki009", "qun", 3);
	tsuki009->addSkill(new ThShennao);
	tsuki009->addSkill(new ThMiaoyao);

	General *tsuki010 = new General(this, "tsuki010", "qun");
	tsuki010->addSkill(new ThHeiguan);
	tsuki010->addSkill(new ThHeiguanClear);
	tsuki010->addSkill(new ThAnyue);
    related_skills.insertMulti("thheiguan", "#thheiguan");

	General *tsuki011 = new General(this, "tsuki011", "qun", 3);
	tsuki011->addSkill(new ThGeyong);
	tsuki011->addSkill(new ThMiqu);
	
	General *tsuki012 = new General(this, "tsuki012", "qun");
	tsuki012->addSkill(new ThZhehui);

    General *tsuki014 = new General(this, "tsuki014", "qun", 3);
	tsuki014->addSkill(new ThExi);
	tsuki014->addSkill(new ThXinglu);

    General *tsuki016 = new General(this, "tsuki016", "qun");
	tsuki016->addSkill(new ThShenyou);

	General *tsuki017 = new General(this, "tsuki017", "qun");
	tsuki017->addSkill(new ThGuixu);
	tsuki017->addSkill(new ThTianque);

    addMetaObject<ThYewangCard>();
    addMetaObject<ThJinguoCard>();
    addMetaObject<ThKaiyunCard>();
    addMetaObject<ThLianhuaCard>();
    addMetaObject<ThShennaoCard>();
    addMetaObject<ThHeiguanCard>();
    addMetaObject<ThMiquCard>();
    addMetaObject<ThExiCard>();
    addMetaObject<ThTianqueCard>();

	skills << new ThYewangViewAsSkill << new Wumou;
}