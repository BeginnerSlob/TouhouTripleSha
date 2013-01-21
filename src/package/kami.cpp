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
			if (splayer->getMark("@jgnongwu") < 1 && player->distanceTo(damage.to) <= 1
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

        if(player->askForSkillInvoke(objectName(), data)){
            room->broadcastSkillInvoke(objectName());

            if(player->isChained()){
                if(dying_data.damage == NULL || dying_data.damage->nature == DamageStruct::Normal)
                    room->setPlayerProperty(player, "chained", false);
            }
            if(!player->faceUp())
                player->turnOver();

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
		if (damage.card && damage.card->isKindOf("Slash")
			&& damage.card->isRed() && !damage.chain && !damage.transfer
			&& player->askForSkillInvoke(objectName()))
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

bool ThYouyaCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const{
	return true;
}

bool ThYouyaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
	return targets.length() == Self->getMark("@yingxiao");
}

void ThYouyaCard::onEffect(const CardEffectStruct &effect) const{
	Room *room = effect.from->getRoom();
	if (!room->askForCard(effect.to, "jink", "@thyouya-jink:" + effect.from->objectName(), QVariant(), Card::MethodResponse, effect.from))
		effect.from->drawCards(1);
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
			return false;
	}
};

KamiPackage::KamiPackage()
    :Package("kami")
{

	General *kami002 = new General(this, "kami002", "god");
	kami002->addSkill(new ThJiguang);
	kami002->addSkill(new ThJiguangBuff);

	General *kami007 = new General(this, "kami007", "god", 2, false);
	kami007->addSkill(new ThFanhun);
	kami007->addSkill(new ThYoushang);
	kami007->addSkill(new ThYouya);
	kami007->addSkill(new ThManxiao);

    addMetaObject<ThYouyaCard>();

	skills << new ThJiguangDistanceSkill << new ThJiguangGivenSkill;
}

ADD_PACKAGE(Kami)