#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "carditem.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-generals.h"
#include "maneuvering.h"
#include "ai.h"

class Huichun: public OneCardViewAsSkill{
public:
    Huichun():OneCardViewAsSkill("huichun"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.contains("peach") && player->getPhase() == Player::NotActive;
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isRed();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
        peach->addSubcard(originalCard->getId());
        peach->setSkillName(objectName());
        return peach;
    }
};

QingnangCard::QingnangCard(){
    once = true;
}

bool QingnangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->isWounded();
}

bool QingnangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.value(0, Self)->isWounded();
}

void QingnangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.value(0, source);

    CardEffectStruct effect;
    effect.card = this;
    effect.from = source;
    effect.to = target;

    room->cardEffect(effect);
}

void QingnangCard::onEffect(const CardEffectStruct &effect) const{
    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;
    effect.to->getRoom()->recover(effect.to, recover);
}

class Qingnang: public OneCardViewAsSkill{
public:
    Qingnang():OneCardViewAsSkill("qingnang"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("QingnangCard");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        QingnangCard *qingnang_card = new QingnangCard;
        qingnang_card->addSubcard(originalCard->getId());

        return qingnang_card;
    }
};

class Wushuang:public TriggerSkill{
public:
    Wushuang():TriggerSkill("wushuang"){
        frequency = Compulsory;
        events << TargetConfirmed << SlashProceed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card->isKindOf("Slash") && use.from == player){
                room->setCardFlag(use.card, "WushuangInvke");
            }
            else if(use.card->isKindOf("Duel")){
                room->setCardFlag(use.card, "WushuangInvke");
            }
        }
        else if(triggerEvent == SlashProceed){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(!effect.slash->hasFlag("WushuangInvke"))
                return false;
            room->broadcastSkillInvoke(objectName());

            QString slasher = player->objectName();

            const Card *first_jink = NULL, *second_jink = NULL;
            first_jink = room->askForCard(effect.to, "jink", "@wushuang-jink-1:" + slasher, QVariant(), CardUsed, player);
            if(first_jink)
                second_jink = room->askForCard(effect.to, "jink", "@wushuang-jink-2:" + slasher, QVariant(), CardUsed, player);

            Card *jink = NULL;
            if(first_jink && second_jink){
                jink = new DummyCard;
                jink->addSubcard(first_jink);
                jink->addSubcard(second_jink);
            }

            room->slashResult(effect, jink);

            return true;
        }
        return false;
    }
};

class WudiViewAsSkill:public ViewAsSkill{
public:
	WudiViewAsSkill():ViewAsSkill("wudi"){
	}

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasFlag("wudiused");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(selected.isEmpty())
            return !to_select->isEquipped();
        else if(selected.length() == 1){
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        }else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.length() == 2){
            const Card *first = cards.first();
            Duel *aa = new Duel(first->getSuit(), 0);
            aa->addSubcards(cards);
            aa->setSkillName("wudi");
            return aa;
        }else
            return NULL;
    }
};

class Wudi:public TriggerSkill{
public:
	Wudi():TriggerSkill("wudi"){
		view_as_skill = new WudiViewAsSkill;
		events << CardUsed;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		CardUseStruct &use = data.value<CardUseStruct>();

		if(use.card && use.card->getSkillName() == "wudi")
			room->setPlayerFlag(player, "wudiused");

		return false;
	}
};

MoyuCard::MoyuCard(){
    once = true;
}

bool MoyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!to_select->isMale())
        return false;

    Duel *duel = new Duel(Card::NoSuit, 0);
    if(targets.isEmpty() && Self->isProhibited(to_select, duel)){
        return false;
    }

    return targets.length() < 2;
}

bool MoyuCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void MoyuCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *player = card_use.from;

    LogMessage log;
    log.from = player;
    log.to << card_use.to[1];
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();

    thread->trigger(CardUsed, room, player, data);

    thread->trigger(CardFinished, room, player, data);

}

void MoyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->broadcastSkillInvoke("moyu");
    room->throwCard(this, source);

    ServerPlayer *to = targets.at(0);
    ServerPlayer *from = targets.at(1);

    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->setCancelable(false);
    duel->setSkillName("moyu");

    CardUseStruct use;
    use.from = from;
    use.to << to;
    use.card = duel;
    room->useCard(use);
}

class Moyu: public OneCardViewAsSkill{
public:
    Moyu():OneCardViewAsSkill("moyu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("MoyuCard");
    }

    virtual bool viewFilter(const Card *) const{
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        MoyuCard *moyu_card = new MoyuCard;
        moyu_card->addSubcard(originalCard->getId());

        return moyu_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 0;
    }
};

class Zhuoyue: public PhaseChangeSkill{
public:
    Zhuoyue():PhaseChangeSkill("zhuoyue"){
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if(player->getPhase() == Player::Finish){
            Room *room = player->getRoom();
            if(room->askForSkillInvoke(player, objectName())){
                room->broadcastSkillInvoke(objectName());
                player->drawCards(1);
            }
        }

        return false;
    }
};

class Xinghuang:public ViewAsSkill{
public:
    Xinghuang():ViewAsSkill("xinghuang"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(selected.isEmpty())
            return !to_select->isEquipped();
        else if(selected.length() == 1){
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        }else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.length() == 2){
            const Card *first = cards.first();
            ArcheryAttack *aa = new ArcheryAttack(first->getSuit(), 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        }else
            return NULL;
    }
};

class Puzhao: public MaxCardsSkill{
public:
    Puzhao():MaxCardsSkill("puzhao$"){
    }
    virtual int getExtra(const Player *target) const{
        int extra = 0;
        QList<const Player *> players = target->getSiblings();
        foreach(const Player *player, players){
            if(player->isAlive() && player->getKingdom() == "qun")
                extra += 2;
        }
        if(target->hasLordSkill(objectName()))
            return extra;
        else
            return 0;
    }
};

class ShuangniangViewAsSkill: public OneCardViewAsSkill{
public:
    ShuangniangViewAsSkill():OneCardViewAsSkill("shuangniang"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("shuangniang") != 0;
    }

    virtual bool viewFilter(const Card *card) const{
        if(card->isEquipped())
            return false;

        int value = Self->getMark("shuangniang");
        if(value == 1)
            return card->isBlack();
        else if(value == 2)
            return card->isRed();

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Duel *duel = new Duel(originalCard->getSuit(), originalCard->getNumber());
        duel->addSubcard(originalCard);
        duel->setSkillName(objectName());
        return duel;
    }
};

class Shuangniang: public TriggerSkill{
public:
    Shuangniang():TriggerSkill("shuangniang"){
        view_as_skill = new ShuangniangViewAsSkill;

        events << EventPhaseStart << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) || (target && target->hasFlag("shuangniang"));
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *shuangxiong, QVariant &data) const{
        if(triggerEvent == EventPhaseStart){
            if(shuangxiong->getPhase() == Player::Start){
                room->setPlayerMark(shuangxiong, "shuangniang", 0);
            }else if(shuangxiong->getPhase() == Player::Draw){
                if(shuangxiong->askForSkillInvoke(objectName())){
                    room->setPlayerFlag(shuangxiong, "shuangniang");

                    room->broadcastSkillInvoke("shuangniang");
                    JudgeStruct judge;
                    judge.pattern = QRegExp("(.*)");
                    judge.good = true;
                    judge.reason = objectName();
                    judge.who = shuangxiong;

                    room->judge(judge);
                    room->setPlayerMark(shuangxiong, "shuangniang", judge.card->isRed() ? 1 : 2);

                    return true;
                }
            }else if(shuangxiong->getPhase() == Player::NotActive && shuangxiong->hasFlag("shuangniang")){
                room->setPlayerFlag(shuangxiong, "-shuangniang");
            }
        }else if(triggerEvent == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if(judge->reason == "shuangniang"){
                shuangxiong->obtainCard(judge->card);
                return true;
            }
        }

        return false;
    }
};

class Sishi: public TriggerSkill{
public:
    Sishi():TriggerSkill("sishi"){
        events << Dying;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if(splayer && splayer->objectName() == room->getCurrent()->objectName()){

            LogMessage log;
            log.from = splayer;
            log.arg = "sishi";
            if(splayer != player){
                log.type = "#SishiTwo";
                log.to << player;
            }else{
                log.type = "#SishiOne";
            }
            room->sendLog(log);
        }
        return false;
    }
};

WenyueCard::WenyueCard(){
    target_fixed = true;
}

void WenyueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->loseMark("@wenyue");
    room->broadcastInvoke("animate", "lightbox:$wenyue");

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach(ServerPlayer *player, players){
        if(player->isAlive())
            room->cardEffect(this, source, player);
    }
}

void WenyueCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    QList<ServerPlayer *> players = room->getOtherPlayers(effect.to);
    QList<int> distance_list;
    int nearest = 1000;
    foreach(ServerPlayer *player, players){
        int distance = effect.to->distanceTo(player);
        distance_list << distance;

        nearest = qMin(nearest, distance);
    }

    QList<ServerPlayer *> wenyue_targets;
    for(int i = 0; i < distance_list.length(); i++){
        if(distance_list.at(i) == nearest && effect.to->canSlash(players.at(i))){
            wenyue_targets << players.at(i);
        }
    }

    if(!wenyue_targets.isEmpty()){
        if(!room->askForUseSlashTo(effect.to, wenyue_targets, "@wenyue-slash"))
           room->loseHp(effect.to);
    }else
        room->loseHp(effect.to);
}

class Wenyue: public ZeroCardViewAsSkill{
public:
    Wenyue():ZeroCardViewAsSkill("wenyue"){
        frequency = Limited;
    }

    virtual const Card *viewAs() const{
        return new WenyueCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@wenyue") >= 1;
    }
};

class Moyu2: public ProhibitSkill{
public:
    Moyu2():ProhibitSkill("moyu2"){

    }

    virtual bool isProhibited(const Player *, const Player *, const Card *card) const{
        return card->isKindOf("TrickCard") && card->isBlack();
    }
};

class Jibu: public DistanceSkill{
public:
    Jibu():DistanceSkill("jibu")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(from->hasSkill(objectName()))
            return -1;
        else
            return 0;
    }
};

class Mengjin: public TriggerSkill{
public:
    Mengjin():TriggerSkill("mengjin"){
        events << SlashMissed << Damage;
    }

    virtual int getPriority() const{
        return 2;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == Damage) {
			DamageStruct &damage = data.value<DamageStruct>();

			if(damage.card && damage.card->isKindOf("Slash") && damage.card->isRed()
					&& !damage.chain && !damage.transfer)
				if(player->askForSkillInvoke(objectName()))
					player->drawCards(1);
			
			return false;
		}
		SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if(effect.to->isAlive() && !effect.to->isNude()){
            if(player->askForSkillInvoke(objectName(), data)){
                room->broadcastSkillInvoke(objectName());
                int to_throw = room->askForCardChosen(player, effect.to, "he", objectName());
                room->throwCard(Sanguosha->getCard(to_throw), effect.to, player);
            }
        }

        return false;
    }
};

void StandardPackage::addLunaGenerals(){
	General *luna001 = new General(this, "luna001", "qun", 3);
    luna001->addSkill(new Huichun);
    luna001->addSkill(new Qingnang);

	General *luna002 = new General(this, "luna002", "qun");
    luna002->addSkill(new Wushuang);
    luna002->addSkill(new Wudi);

	General *luna003 = new General(this, "luna003", "qun", 3, false);
    luna003->addSkill(new Moyu);
    luna003->addSkill(new Zhuoyue);

    General *luna004 = new General(this, "luna004$", "qun");
    luna004->addSkill(new Xinghuang);
    luna004->addSkill(new Puzhao);

    General *luna005 = new General(this, "luna005", "qun");
    luna005->addSkill(new Shuangniang);

    General *luna007 = new General(this, "luna007", "qun", 3);
    luna007->addSkill(new Sishi);
    luna007->addSkill(new MarkAssignSkill("@wenyue", 1));
    luna007->addSkill(new Wenyue);
    luna007->addSkill(new Moyu2);
    related_skills.insertMulti("wenyue", "#@wenyue-1");

    General *luna008 = new General(this, "luna008", "qun");
    luna008->addSkill(new Jibu);
    luna008->addSkill(new Mengjin);
	
    addMetaObject<QingnangCard>();
    addMetaObject<MoyuCard>();
    addMetaObject<WenyueCard>();
}