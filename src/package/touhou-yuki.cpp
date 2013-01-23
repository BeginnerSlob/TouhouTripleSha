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

class ThDunjia:public TriggerSkill{
public:
	ThDunjia():TriggerSkill("thdunjia"){
		events << Damage;
		frequency = Frequent;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if (!damage.card->isKindOf("Slash"))
			return false;

		int n = qAbs(player->getEquips().size() - damage.to->getEquips().size());
		if (n == 0)
			return false;

		if (!player->askForSkillInvoke(objectName()))
			return false;

		QStringList choices;
		if (player->getCardCount(true) >= n)
			choices << "discard";

		choices << "draw";

		QString choice = room->askForChoice(player, objectName(), choices.join("+"));
		if (choice == "discard")
		{
			DummyCard *card = room->askForCardsChosen(player, damage.to, "he", objectName(), n);
			card->deleteLater();
			room->throwCard(card, damage.to, player);
		}
		else
			player->drawCards(n);

		return false;
	}
};

ThDongmoCard::ThDongmoCard(){
	handling_method = Card::MethodNone;
}

bool ThDongmoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.size() < Self->getLostHp() && to_select != Self;
}

void ThDongmoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	source->turnOver();
	foreach(ServerPlayer *p, targets)
		p->turnOver();

	source->drawCards(1);
	foreach(ServerPlayer *p, targets)
		p->drawCards(1);
}

class ThDongmoViewAsSkill:public ZeroCardViewAsSkill{
public:
	ThDongmoViewAsSkill():ZeroCardViewAsSkill("thdongmo"){
	}

	virtual const Card *viewAs() const{
		return new ThDongmoCard;
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
		return false;
	}

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
		return pattern == "@@thdongmo";
	}
};

class ThDongmo:public TriggerSkill{
public:
	ThDongmo():TriggerSkill("thdongmo"){
		events << EventPhaseStart;
		view_as_skill = new ThDongmoViewAsSkill;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
		if (player->getPhase() == Player::Finish && player->isWounded() && player->askForSkillInvoke(objectName()))
			room->askForUseCard(player, "@@thdongmo", "@thdongmo");

		return false;
	}
};

class ThJidong:public TriggerSkill{
public:
	ThJidong():TriggerSkill("thjidong"){
		events << CardResponded;
		frequency = Frequent;
	}
	
    virtual int getPriority() const{
        return 3;
    }

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (player == NULL) return false;
        CardStar card_star = data.value<CardResponseStruct>().m_card;
        if (!card_star->isKindOf("Jink"))
            return false;
		if (player->askForSkillInvoke(objectName()))
			player->drawCards(1);

        return false;
    }
};

class ThQiebao:public TriggerSkill{
public:
	ThQiebao():TriggerSkill("thqiebao"){
		events << CardUsed << BeforeCardsMoveOneTime;
	}
	
	virtual bool triggerable(ServerPlayer *target) const{
		return target != NULL;
	}

	bool doQiebao(Room* room, ServerPlayer *player, QList<int> card_ids) const {
		if (card_ids.isEmpty())
			return false;

		if (!player->askForSkillInvoke(objectName()))
			return false;

		const Card *card = room->askForCard(player, "slash", "@thqiebao", QVariant(), Card::MethodResponse);

		if (!card)
			return false;

		QList<CardsMoveStruct> exchangeMove;
        CardsMoveStruct qiebaoMove;

		if (card->isRed())
		{
			qiebaoMove.to = player;
			qiebaoMove.to_place = Player::PlaceHand;
		}
		else
		{
			qiebaoMove.to = NULL;
			qiebaoMove.to_place = Player::DiscardPile;
			CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
			qiebaoMove.reason = reason;
		}
		qiebaoMove.card_ids = card_ids;
        exchangeMove.push_back(qiebaoMove);
        room->moveCardsAtomic(exchangeMove, false);

		return true;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == CardUsed)
		{
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.to.isEmpty())
				return false;

			if (use.card->isKindOf("Peach"))
			{	
				QList<int> card_ids;
				if (use.card->isVirtualCard())
					card_ids = use.card->getSubcards();
				else
					card_ids << use.card->getEffectiveId();
				return doQiebao(room, player, card_ids);
			}
		}
		else if (triggerEvent == BeforeCardsMoveOneTime && TriggerSkill::triggerable(player))
		{
			CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
			if (!move->from || !move->to || move->from == move->to)
				return false;

			if (move->to_place == Player::PlaceHand || move->to_place == Player::PlaceEquip)
			{
				QList<int> qiebaolist;
				for(int i = 0; i < move->card_ids.size(); i++){
					int card_id = move->card_ids[i];
					if(move->from_places[i] == Player::PlaceHand || move->from_places[i] == Player::PlaceEquip)
						qiebaolist << card_id;
				}

				return doQiebao(room, player, qiebaolist);
			}
        }
		return false;
	}
};

class ThBaota:public TriggerSkill{
public:
	ThBaota():TriggerSkill("thbaota"){
		events << EventPhaseChanging;
	}

	virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
		PhaseChangeStruct change = data.value<PhaseChangeStruct>();
		if (player->isSkipped(change.to))
			return false;

		if (change.to == Player::Draw || change.to == Player::Play)
			if (player->askForSkillInvoke(objectName(), QVariant::fromValue(QString::number((int)change.to))))
			{
				player->skip(change.to);
				player->gainMark("@fadeng");
			}

		return false;
	}
};

class ThWeiguang:public TriggerSkill{
public:
	ThWeiguang():TriggerSkill("thweiguang"){
		events << EventPhaseChanging;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (player->getMark("@fadeng") <= 0)
			return false;

		PhaseChangeStruct change = data.value<PhaseChangeStruct>();
		
		if (change.from == Player::Play || change.from == Player::Draw)
		{
			int n = (int)change.from;
			if (!player->hasFlag(objectName() + QString::number(n)) && player->askForSkillInvoke(objectName(), QVariant::fromValue(QString::number((int)change.from))))
			{
				room->setPlayerFlag(player, objectName() + QString::number(n));
				player->loseMark("@fadeng");
				player->insertPhase(change.from);
				change.to = change.from;
				data = QVariant::fromValue(change);
				return false;
			}
		}
		if (change.to == Player::Judge || change.to == Player::Discard)
		{
			int n = (int)change.to;
			if (!player->hasFlag(objectName() + QString::number(n)) && player->askForSkillInvoke(objectName(), QVariant::fromValue(QString::number((int)change.to))))
			{
				room->setPlayerFlag(player, objectName() + QString::number(n));
				player->loseMark("@fadeng");
				player->skip(change.to);
			}
		}

		return false;
	}
};

class ThChuhui:public TriggerSkill{
public:
	ThChuhui():TriggerSkill("thchuhui"){
		events << GameStart;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		LogMessage log;
		log.type = "#TriggerSkill";
		log.from = player;
		log.arg = objectName();
		room->sendLog(log);
		player->gainMark("@fadeng");

		return false;
	}
};

ThKujieCard::ThKujieCard(){
}

bool ThKujieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.isEmpty() && to_select->hasSkill("thkujie") && Self != to_select;
}

void ThKujieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	ServerPlayer *target = targets.first();
	room->loseHp(target);
	if (target->isAlive())
		room->setPlayerFlag(target, "thkujieused");
}

class ThKujieViewAsSkill:public OneCardViewAsSkill{
public:
	ThKujieViewAsSkill():OneCardViewAsSkill("thkujievs"){
	}

    virtual bool viewFilter(const Card *to_select) const{
		return to_select->isRed() && to_select->isKindOf("BasicCard");
	}

    virtual const Card *viewAs(const Card *originalCard) const{
		ThKujieCard *card = new ThKujieCard();
		card->addSubcard(originalCard);
		return card;
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("ThKujieCard");
	}
};

class ThKujie:public TriggerSkill{
public:
	ThKujie():TriggerSkill("thkujie"){
		events << EventPhaseStart << EventPhaseEnd;
	}
	
	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		ServerPlayer *splayer = room->findPlayerBySkillName("thkujie", true);
		if (!splayer)
			return false;

		if (triggerEvent == EventPhaseEnd && player->hasSkill("thkujievs"))
			room->detachSkillFromPlayer(player, "thkujievs", true);
		else if (triggerEvent == EventPhaseStart)
		{
			if (player->getPhase() == Player::Play && !player->hasSkill("thkujievs") && splayer->isAlive() && player->inMyAttackRange(splayer) && player != splayer)
				room->attachSkillToPlayer(player, "thkujievs");
			else if (player->getPhase() == Player::NotActive && splayer->isAlive() && splayer->hasFlag("thkujieused"))
			{
				room->setPlayerFlag(splayer, "-thkujieused");
				RecoverStruct recover;
				recover.recover = 2;
				recover.who = splayer;
				room->recover(splayer, recover);
			}
		}

		return false;
	}
};

class ThYinbi:public TriggerSkill{
public:
	ThYinbi():TriggerSkill("thyinbi"){
		events << EventPhaseEnd << PreHpReduced;
	}
	
	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
		ServerPlayer *player = room->findPlayerBySkillName(objectName());
		if (!player)
			return false;

		if (triggerEvent == EventPhaseEnd)
			room->setPlayerMark(player, objectName(), 0);
		else if (triggerEvent == PreHpReduced)
		{
			DamageStruct damage = data.value<DamageStruct>();

			if (damage.damage >= 2 && damage.to != player && player->getMark(objectName()) < 1 && player->askForSkillInvoke(objectName()))
			{
				player->addMark(objectName());
				damage.transfer = true;
				damage.to = player;
				room->damage(damage);
				return true;
			}
		}

		return false;
	}
};

void TouhouPackage::addYukiGenerals(){
	General *yuki006 = new General(this, "yuki006", "wu");
	yuki006->addSkill("jibu");
	yuki006->addSkill(new ThDunjia);

	General *yuki011 = new General(this, "yuki011", "wu", 3);
	yuki011->addSkill(new ThDongmo);
	yuki011->addSkill(new ThJidong);

	General *yuki014 = new General(this, "yuki014", "wu");
	yuki014->addSkill(new ThQiebao);

	General *yuki015 = new General(this, "yuki015", "wu");
	yuki015->addSkill(new ThBaota);
	yuki015->addSkill(new ThWeiguang);
	yuki015->addSkill(new ThChuhui);

	General *yuki016 = new General(this, "yuki016", "wu");
	yuki016->addSkill(new ThKujie);
	yuki016->addSkill(new ThYinbi);
	
    addMetaObject<ThDongmoCard>();
    addMetaObject<ThKujieCard>();

	skills << new ThKujieViewAsSkill;
}

