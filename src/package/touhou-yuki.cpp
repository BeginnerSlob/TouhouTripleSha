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

class ThJianmo: public TriggerSkill {
public:
	ThJianmo(): TriggerSkill("thjianmo") {
		events << EventPhaseStart << EventPhaseChanging << CardUsed << CardEffected;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == EventPhaseChanging)
		{	
			if (player->hasFlag("jianmoinvoke"))
				room->setPlayerFlag(player, "-jianmoinvoke");
			room->removePlayerCardLimitation(player, "use,response", "Slash@0");
		}
		else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play)
		{
			ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
			if (!splayer || player == splayer)
				return false;

			if (player->getHandcardNum() >= player->getMaxHp() && splayer->askForSkillInvoke(objectName()))
				if (room->askForChoice(player, objectName(), "jian+mo") == "jian")
				{
					LogMessage log;
					log.type = "#thjianmochoose1";
					log.from = player;
					log.arg = "1";
					room->sendLog(log);
					player->drawCards(1);
					room->setPlayerCardLimitation(player, "use,response", "Slash", false);
				}
				else
				{
					LogMessage log;
					log.type = "#thjianmochoose2";
					log.from = player;
					log.arg = "2";
					room->sendLog(log);
					room->setPlayerFlag(player,"jianmoinvoke");
				}
		}
		else if (triggerEvent == CardUsed)
		{
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.card->hasFlag("jianmoavoid"))
				use.card->setFlags("-jianmoavoid");
			if (use.card->isKindOf("Slash") && use.from->hasFlag("jianmoinvoke"))
			{
				LogMessage log;
				log.type = "#ThJianmo";
				log.from = player;
				log.arg = objectName();
				log.arg2 = use.card->objectName();
				room->sendLog(log);

				if (!room->askForCard(player, "..", "@thjianmo"))
					use.card->setFlags("jianmoavoid");
			}
		}
		else if (triggerEvent == CardEffected && data.value<CardEffectStruct>().card->hasFlag("jianmoavoid"))
			return true;

		return false;
	}
};

class ThErchong: public TriggerSkill {
public:
	ThErchong(): TriggerSkill("therchong") {
		events << EventPhaseStart;
		frequency = Wake;
	}

	virtual bool triggerable(const ServerPlayer *target) const {
		return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start
												 && target->getHp() <= 2
												 && target->getMark("@erchong") <= 0;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
		LogMessage log;
		log.type = "#ThErchong";
		log.from = player;
		log.arg  = objectName();
		log.arg2 = QString::number(player->getHpPoints());
		room->sendLog(log);

		player->gainMark("@erchong");

		if (player->getCardCount(true) < 2)
			player->throwAllHandCardsAndEquips();
		else
			room->askForDiscard(player, objectName(), 2, 2, false, true);

		room->loseMaxHp(player);
		room->acquireSkill(player, "huanfa");
		room->acquireSkill(player, "zhuji");

		return false;
	}
};

class ThChundu: public TriggerSkill {
public:
	ThChundu(): TriggerSkill("thchundu$") {
		events << CardUsed << CardResponded;
	}

	virtual bool triggerable(const ServerPlayer *target) const {
		return target != NULL && target->getKingdom() == "wu";
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
		const Card *trigger_card;
		if (triggerEvent == CardUsed)
			trigger_card = data.value<CardUseStruct>().card;
		else if (triggerEvent == CardResponded && data.value<CardResponseStruct>().m_isUse)
			trigger_card = data.value<CardResponseStruct>().m_card;

		if (trigger_card == NULL)
			return false;

		if (trigger_card->isKindOf("BasicCard") && trigger_card->getSuit() == Card::Heart)
		{
			QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
			{
                if (p->hasLordSkill(objectName()))
                    targets << p;
            }
            
            while (!targets.isEmpty())
			{
                if (player->askForSkillInvoke(objectName()))
				{
                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                    targets.removeOne(target);
					const Card *card = room->peek();
					target->drawCards(1);
					if (room->askForChoice(target, objectName(), "give+throw") == "give")
					{
						ServerPlayer *tar = room->askForPlayerChosen(target, room->getOtherPlayers(target), objectName());
						room->moveCardTo(card, tar, Player::PlaceHand, false);
					}
					else
					{
						CardMoveReason reason = CardMoveReason(CardMoveReason::S_REASON_NATURAL_ENTER, target->objectName());
						room->moveCardTo(card, NULL, Player::DiscardPile, reason, true);
					}
                }
				else
                    break;
            }
		}

		return false;
	}
};

class ThCuimeng: public TriggerSkill {
public:
	ThCuimeng():TriggerSkill("thcuimeng"){
        events << EventPhaseStart << FinishJudge;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == EventPhaseStart && player->getPhase() == Player::Start){
            int n = 0;
            while(player->askForSkillInvoke(objectName())){
                if(n == 0)
                    room->broadcastSkillInvoke(objectName());
                n++;

                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;
                judge.time_consuming = true;

                room->judge(judge);
                if(judge.isBad())
                    break;
            }

        }else if(triggerEvent == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if(judge->reason == objectName()){
                if(judge->card->isRed()){
                    player->obtainCard(judge->card);
                    return true;
                }
            }
        }

        return false;
    }
};

class ThZuimeng: public FilterSkill {
public:
	ThZuimeng(): FilterSkill("thzuimeng") {
	}

	virtual bool viewFilter(const Card* to_select) const{
        return to_select->isKindOf("BasicCard") && to_select->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *jiu = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        jiu->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(jiu);
        return card;
    }
};

class ThZuimengTriggerSkill: public TriggerSkill {
public:
	ThZuimengTriggerSkill(): TriggerSkill("#thzuimeng") {
		events << CardUsed;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (data.value<CardUseStruct>().card->isKindOf("Analeptic"))
		{
			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = player;
			log.arg = "thzuimeng";
			room->sendLog(log);

			player->drawCards(1);
		}

		return false;
	}
};

class ThMengwu: public MaxCardsSkill {
public:
	ThMengwu(): MaxCardsSkill("thmengwu") {
	}

	virtual int getExtra(const Player *target) const{
		if (target->hasSkill(objectName()))
			return 1;
		else
			return 0;
	}
};

class ThCihang: public TriggerSkill {
public:
	ThCihang(): TriggerSkill("thcihang") {
		events << SlashMissed;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (player->askForSkillInvoke(objectName()))
		{
			if (room->askForChoice(player, objectName(), "discard+draw") == "discard")
			{
				int x = effect.to->getLostHp();
				if (x == 0)
					return false;

				if (player->getCardCount(true) < x)
					player->throwAllHandCardsAndEquips();
				else
					room->askForDiscard(player, objectName(), x, x, false, true);
			}
            else
			{
				int x = qMin(effect.to->getHpPoints(), 5);
				if (x == 0)
					return false;
				effect.to->drawCards(x);
			}

            room->broadcastSkillInvoke(objectName());
            room->slashResult(effect, NULL);
						
			return true;
		}

        return false;
	}
};

class ThZhancao: public TriggerSkill {
public:
	ThZhancao(): TriggerSkill("thzhancao") {
		events << TargetConfirming << PostCardEffected << SlashEffected << CardFinished;
	}

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == TargetConfirming)
		{
			ServerPlayer *splayer = room->findPlayerBySkillName(objectName());

			if(splayer == NULL)
				return false;

			if(splayer->getPhase() != Player::NotActive)
				return false;

			CardUseStruct use = data.value<CardUseStruct>();

			if(use.card->isKindOf("Slash") && use.to.contains(player) && splayer->inMyAttackRange(player) && splayer->askForSkillInvoke(objectName())){
				if(!room->askForCard(splayer, "EquipCard|.|.|equipped", "@thzhancao")){
					room->loseHp(splayer);
					splayer->addMark("zhancaoget");
				}
			
				player->addMark("zhancaotarget");
			}
		}
		else if (triggerEvent == CardFinished)
		{
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.card->isKindOf("Slash"))
				foreach (ServerPlayer *p, room->getAllPlayers())
				{
					if (p->getMark("zhancaotarget") > 0)
						room->setPlayerMark(p, "zhancaotarget", 0);
					if (p->getMark("zhancaoget") > 0)
						room->setPlayerMark(p, "zhancaoget", 0);
				}
		}
		else if (triggerEvent == PostCardEffected)
		{
			CardUseStruct use = data.value<CardUseStruct>();
			if(use.card->isKindOf("Slash") &&
					(!use.card->isVirtualCard() ||
					 (use.card->getSubcards().length() == 1 &&
					  Sanguosha->getCard(use.card->getSubcards().first())->isKindOf("Slash")))){
				if (player == NULL) return false;
				if(room->getCardPlace(use.card->getEffectiveId()) == Player::DiscardPile){
					ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
					if (splayer == NULL || splayer->getMark("zhancaoget") < 1)
						return false;

					splayer->obtainCard(use.card);
				}
			}

			return false;
		}
		else if (triggerEvent == SlashEffected)
            if (player->getMark("zhancaotarget") > 0) {
                player->removeMark("zhancaotarget");
				return true;
			}
        return false;
	}
};

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
		events << CardUsed << CardEffected << BeforeCardsMoveOneTime;
	}
	
	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	bool doQiebao(Room* room, ServerPlayer *player, QList<int> card_ids, bool discard) const {
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
			qiebaoMove.card_ids = card_ids;
			exchangeMove.push_back(qiebaoMove);
			room->moveCardsAtomic(exchangeMove, false);
		}
		else if (discard)
		{
			qiebaoMove.to = NULL;
			qiebaoMove.to_place = Player::DiscardPile;
			CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
			qiebaoMove.reason = reason;
			qiebaoMove.card_ids = card_ids;
			exchangeMove.push_back(qiebaoMove);
			room->moveCardsAtomic(exchangeMove, false);
		}

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
				if (doQiebao(room, player, card_ids, false))
					use.card->setFlags("qiebaoinvoke");
			}
		}
		else if (triggerEvent == CardEffected)
		{
			CardEffectStruct effect = data.value<CardEffectStruct>();
			if (effect.card->hasFlag("qiebaoinvoke"))
				return true;
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

				return doQiebao(room, player, qiebaolist, true);
			}
        }

		return false;
	}
};

class ThLingta:public TriggerSkill{
public:
	ThLingta():TriggerSkill("thlingta"){
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
	return targets.isEmpty() && Self->inMyAttackRange(to_select) && to_select->hasSkill("thkujie") && Self != to_select;
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
			if (player->getPhase() == Player::Play && !player->hasSkill("thkujievs") && splayer->isAlive() && player != splayer)
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
	General *yuki001 = new General(this, "yuki001$", "wu");
	yuki001->addSkill(new ThJianmo);
	yuki001->addSkill(new ThErchong);
	yuki001->addSkill(new ThChundu);

	General *yuki002 = new General(this, "yuki002", "wu", 3);
	yuki002->addSkill(new ThCuimeng);
	yuki002->addSkill(new ThZuimeng);
	yuki002->addSkill(new ThZuimengTriggerSkill);
    related_skills.insertMulti("thzuimeng", "#thzuimeng");
	yuki002->addSkill(new ThMengwu);
	
	General *yuki003 = new General(this, "yuki003", "wu");
	yuki003->addSkill(new ThCihang);
	
	General *yuki004 = new General(this, "yuki004", "wu");
	yuki004->addSkill(new ThZhancao);

	General *yuki006 = new General(this, "yuki006", "wu");
	yuki006->addSkill("jibu");
	yuki006->addSkill(new ThDunjia);

	General *yuki011 = new General(this, "yuki011", "wu", 3);
	yuki011->addSkill(new ThDongmo);
	yuki011->addSkill(new ThJidong);

	General *yuki014 = new General(this, "yuki014", "wu");
	yuki014->addSkill(new ThQiebao);

	General *yuki015 = new General(this, "yuki015", "wu");
	yuki015->addSkill(new ThLingta);
	yuki015->addSkill(new ThWeiguang);
	yuki015->addSkill(new ThChuhui);

	General *yuki016 = new General(this, "yuki016", "wu");
	yuki016->addSkill(new ThKujie);
	yuki016->addSkill(new ThYinbi);
	
    addMetaObject<ThDongmoCard>();
    addMetaObject<ThKujieCard>();

	skills << new ThKujieViewAsSkill;
}

