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

ThJinguoCard::ThJinguoCard(){
	handling_method = Card::MethodNone;
	will_throw = false;
}

bool ThJinguoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.isEmpty() && to_select->isWounded() && to_select != Self;
}

void ThJinguoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	room->showCard(source, getEffectiveId());
	ServerPlayer *target = targets.first();
	QStringList choices;
	choices << "give";
	if (target->getHandcardNum() > 1)
		choices << "discard";
	QString choice = room->askForChoice(target, "thjinguo", choices.join("+"));
	if (choice == "discard")
	{
		room->throwCard(this, source);
        DummyCard *dummy = room->askForCardsChosen(source, target, "h", "thjinguo", 2);
        if (dummy->subcardsLength() > 0)
            source->obtainCard(dummy, false);
        dummy->deleteLater();
		RecoverStruct recover;
		recover.who = source;
		room->recover(target, recover);
	}
	else
		room->setPlayerFlag(source, "thjinguoinvoke");
}

class ThJinguo:public OneCardViewAsSkill{
public:
	ThJinguo():OneCardViewAsSkill("thjinguo"){
	}

	virtual bool viewFilter(const Card *to_select) const{
		if(!Self->hasFlag("thjinguoinvoke"))
		{
			return to_select->getSuit() == Card::Heart;
		}
		else
		{
			return to_select->getSuit() == Card::Heart && to_select->isKindOf("BasicCard");
		}
	}

	virtual const Card *viewAs(const Card *originalCard) const{
		if(!Self->hasFlag("thjinguoinvoke"))
		{
			ThJinguoCard *card = new ThJinguoCard;
			card->addSubcard(originalCard);
			return card;
		}
		else
		{
			Indulgence *le = new Indulgence(Card::NoSuitNoColor, 0);
			le->addSubcard(originalCard);
			le->setSkillName(objectName());
			return le;
		}
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("ThJinguoCard") || player->hasFlag("thjinguoinvoke");
	}
};

class ThShixue:public TriggerSkill{
public:
	ThShixue():TriggerSkill("thshixue"){
		events << EventPhaseStart;
		frequency = Wake;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (player->getPhase() != Player::Start || player->getMark(objectName()) > 0)
			return false;

		if (player->getEquip(2) || player->getEquip(3))
		{
			LogMessage log;
			log.type = "#ThShixueWake";
			log.from = player;
			log.arg = objectName();
			room->sendLog(log);

			if (player->getCardCount(true) < 2 || !room->askForDiscard(player, objectName(), 2, 2, true, true))
				room->loseHp(player);

			room->loseMaxHp(player);
			room->acquireSkill(player, "kuanggu");
			player->gainMark("@waked");
			player->addMark(objectName());
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
	General *tsuki002 = new General(this, "tsuki002", "qun");
	tsuki002->addSkill(new ThJinguo);
	tsuki002->addSkill(new ThShixue);

	General *tsuki005 = new General(this, "tsuki005", "qun", 3);
	tsuki005->addSkill(new ThShouye);
	tsuki005->addSkill(new ThXushi);

    addMetaObject<ThJinguoCard>();
}