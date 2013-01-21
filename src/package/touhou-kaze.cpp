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

class ThQiji:public TriggerSkill{
public:
    ThQiji():TriggerSkill("thqiji"){
        frequency = Frequent;
        events << CardsMoveOneTime << EventPhaseChanging << EventPhaseEnd;
    }
    
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == EventPhaseChanging){
            player->setMark(objectName(), 0);
        }
        else if(triggerEvent == CardsMoveOneTime)
        {
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            if(move->from != player)
                return false;

            if(move->to_place == Player::DiscardPile && player->getPhase() == Player::Discard)
                player->setMark(objectName(), player->getMark(objectName()) + move->card_ids.length());
        }
        else if(triggerEvent == EventPhaseEnd && player->getMark(objectName()) >= 2 
                && player->isWounded() && player->askForSkillInvoke(objectName())){
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        }

        return false;
    }
};

ThJiyiCard::ThJiyiCard(){
    will_throw = false;
}

bool ThJiyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return !to_select->isKongcheng() && to_select!=Self && targets.isEmpty();
}

void ThJiyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    const Card *card = room->askForCardShow(target, source, "@thjiyi");
    room->showCard(target, card->getId());
    if(card->getType() == "trick") {
		const Card *card2 = room->askForCard(source, ".Trick", "@thjiyi", QVariant(), Card::MethodNone);
        if(!card2)
            target->drawCards(1);
        else
            target->obtainCard(card2);
    } else
        source->obtainCard(card);
}

class ThJiyi:public ZeroCardViewAsSkill{
public:
    ThJiyi():ZeroCardViewAsSkill("thjiyi"){
    }
    
    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThJiyiCard");
    }

    virtual const Card *viewAs() const{
        return new ThJiyiCard;
    }
};

class ThQiyuan: public TriggerSkill{
public:
    ThQiyuan():TriggerSkill("thqiyuan$"){
        events << CardAsked;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("thqiyuan");
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();
        if(pattern != "pindiancard")
            return false;
                
        QList<ServerPlayer *> lieges = room->getLieges("shu", player);
        if(lieges.isEmpty())
            return false;

        if(!room->askForSkillInvoke(player, objectName()))
            return false;

        room->broadcastSkillInvoke(objectName(), getEffectIndex(player, NULL));

        QVariant tohelp = QVariant::fromValue((PlayerStar)player);
        foreach(ServerPlayer *liege, lieges){
			const Card *card = room->askForCard(liege, ".", "@thqiyuan-pindiancard:" + player->objectName(), tohelp, Card::MethodNone, player);
            if(card){
                room->provide(card);
                return true;
            }
        }

        return false;
    }
};

class ThJilanwen:public TriggerSkill{
public:
    ThJilanwen():TriggerSkill("thjilanwen"){
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Draw)
            return false;

        bool invoke = false;
        foreach(ServerPlayer *p, room->getAllPlayers())
            if(!p->getCards("ej").isEmpty()) {
                invoke = true;
                break;
            }

        if(!invoke)
            return false;

        if(!player->askForSkillInvoke(objectName()))
            return false;

        room->broadcastSkillInvoke(objectName());
        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(.*):(.*)");
        judge.good = true;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);
        player->obtainCard(judge.card);
        Card::Suit suit=judge.card->getSuit();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getAllPlayers())
            foreach(const Card *card, p->getCards("ej"))
                if(card->getSuit() != suit) {
                    targets << p;
                    break;
                }

        if(targets.isEmpty())
            return true;

        ServerPlayer *target=room->askForPlayerChosen(player, targets, objectName());
        QList<int> card_ids;
        foreach(const Card *card, target->getCards("ej"))
            if(card->getSuit() != suit) 
                card_ids << card->getEffectiveId();

        room->fillAG(card_ids, player);
        int card_id = room->askForAG(player, card_ids, false, objectName());
        player->invoke("clearAG");
        if(card_id != -1)
            room->obtainCard(player, card_id);

        return true;
    }
};

ThNianxieCard::ThNianxieCard(){
    will_throw = false;
}

bool ThNianxieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && (to_select != Self);
}

void ThNianxieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    target->obtainCard(this);
    int card_id=room->askForCardChosen(source, target, "h", "thnianxie");
    room->showCard(target, card_id);
    if(Sanguosha->getCard(card_id)->isKindOf("Jink")) {
           source->drawCards(1);
           target->drawCards(1);
    }
}

class ThNianxie:public OneCardViewAsSkill{
public:
    ThNianxie():OneCardViewAsSkill("thnianxie"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThNianxieCard");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isKindOf("Jink");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThNianxieCard *card = new ThNianxieCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThJilan:public TriggerSkill{
public:
    ThJilan():TriggerSkill("thjilan"){
        events << Damaged;
    }
    
    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct &damage = data.value<DamageStruct>();
        for(int i = 0; i < damage.damage; i++) {
            if(!player->askForSkillInvoke(objectName(), data))
                break;

            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getAlivePlayers())
                if(!p->isNude())
                    targets << p;

            if(targets.isEmpty())
                break;

            ServerPlayer *target=room->askForPlayerChosen(player, targets, objectName());
            int x = 1;
            if(target->isWounded())
                x = target->getLostHp();

            if(x > target->getCardCount(true))
                x = target->getCardCount(true);

            room->askForDiscard(target, objectName(), x, x, false, true);
        }

        return false;
    }
};

class ThWangshou:public TriggerSkill{
public:
    ThWangshou():TriggerSkill("thwangshou"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct &damage = data.value<DamageStruct>();
        ServerPlayer *victim = damage.to;
        if(victim == NULL || victim->isDead())
            return false;

        if(!player->askForSkillInvoke(objectName()))
            return false;

        room->broadcastSkillInvoke(objectName());

        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(spade|club):(.*)");
        judge.good = false;
        judge.reason = objectName();
        judge.who = player;

        room->judge(judge);
        if(judge.isBad()) {
            if(victim->isNude())
                return false;

            int card_id = room->askForCardChosen(player, victim, "he", objectName());
            room->throwCard(card_id, victim, player);
        }
        return false;
    }
};

class ThHongye:public TriggerSkill{
public:
    ThHongye():TriggerSkill("thhongye"){
        events << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        if(!judge->card->isRed())
            return false;

        if(player->isDead())
            return false;
        
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if(splayer == NULL || player == splayer || splayer->isKongcheng() || splayer->getPhase() != Player::NotActive)
            return false;

        if(splayer->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->askForUseSlashTo(splayer, player, "@thhongye");
        }
        return false;
    }
};

ThEnanCard::ThEnanCard(){

}

bool ThEnanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->inMyAttackRange(to_select);
}

void ThEnanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->loseMaxHp(source);
    room->loseHp(targets.first());
}

class ThEnan:public ZeroCardViewAsSkill{
public:
    ThEnan():ZeroCardViewAsSkill("thenan"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThEnanCard");
    }

    virtual const Card *viewAs() const{
        return new ThEnanCard;
    }
};

class ThBeiyun:public TriggerSkill{
public:
    ThBeiyun():TriggerSkill("thbeiyun"){
        events << AskForPeaches;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DyingStruct &dying = data.value<DyingStruct>();
        if(dying.who != player)
            return false;

        if(!player->askForSkillInvoke(objectName()))
            return false;
        
        QList<int> card_ids = room->getNCards(qMin(4, player->getMaxHp()));
        CardMoveReason reason = CardMoveReason();
        reason.m_reason = CardMoveReason::S_REASON_NATURAL_ENTER;
        int maxtimes = 0;
        while (!card_ids.isEmpty()) {
            QStringList choices;
            choices << "cancel";
            foreach(int id, card_ids) {
                const Card *card = Sanguosha->getCard(id);
                if(card->getSuit() == Card::Diamond) {
                    choices << "red";
                    break;
                }
            }
            foreach(int id, card_ids) {
                const Card *card = Sanguosha->getCard(id);
                if(card->isBlack()) {
                    choices << "black";
                    break;
                }
            }
            room->fillAG(card_ids, NULL);
            QString choice;
            if(choices.size() < 2)
                choice = "cancel";
            else
                choice = room->askForChoice(player, objectName(), choices.join("+"));

            maxtimes = 0;
            if(choice == "red") {
                maxtimes=card_ids.size();
                for (int x = 0; x < maxtimes; x++)
                    foreach(int id, card_ids) {
                        const Card *card = Sanguosha->getCard(id);
                        if(card->isRed()) {
                            room->moveCardTo(card, NULL, Player::DiscardPile, reason, true);
                            card_ids.removeOne(id);
                            break;
                        }
                    }
                RecoverStruct recover;
                recover.who = player;
                room->recover(player, recover);
                foreach(ServerPlayer *p, room->getPlayers())
                    p->invoke("clearAG");
            }
            else if(choice == "black") {
                maxtimes=card_ids.size();
                for(int x = 0; x < maxtimes; x++)
                    foreach(int id, card_ids) {
                        const Card *card = Sanguosha->getCard(id);
                        if(card->isBlack()) {
                            room->moveCardTo(card, NULL, Player::DiscardPile, reason, true);
                            card_ids.removeOne(id);
                            break;
                        }
                    }

                room->setPlayerProperty(player, "maxhp", player->getMaxHp()+1);
                foreach(ServerPlayer *p, room->getPlayers())
                    p->invoke("clearAG");
            }
            else {
                if(room->askForChoice(player, objectName(), "get+discard") == "get") {
                    maxtimes = card_ids.size();
                    for(int x = 0; x < maxtimes; x++) {
                        int id = card_ids.first();
                        const Card *card = Sanguosha->getCard(id);
                        player->obtainCard(card);
                        card_ids.removeOne(id);
                    }
                }
                else {
                    maxtimes=card_ids.size();
                    for(int x = 0; x < maxtimes; x++) {
                        int id = card_ids.first();
                        const Card *card = Sanguosha->getCard(id);
                        room->moveCardTo(card, NULL, Player::DiscardPile, reason, true);
                        card_ids.removeOne(id);
                    }
                }
            }
        }
        foreach(ServerPlayer *p, room->getPlayers())
            p->invoke("clearAG");
        
        return false;
    }
};

ThBishaCard::ThBishaCard(){
    target_fixed = true;
    will_throw = false;
    mute = true;
}

void ThBishaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    const Card *card = Sanguosha->getCard(getEffectiveId());
    QString cardname;
    if(card->isRed())
        cardname = "indulgence";
    else
        cardname = "supply_shortage";

    Card *newcard = Sanguosha->cloneCard(cardname, card->getSuit(), card->getNumber());
    newcard->addSubcard(card);
    CardUseStruct use;
    use.card = newcard;
    use.from = source;
    use.to << source;
    room->useCard(use, true);

    source->drawCards(1);
    QList<ServerPlayer *> victims;
    foreach(ServerPlayer *p, room->getOtherPlayers(source))
        if(source->canSlash(p, NULL, false))
            victims << p;

    if(!victims.isEmpty()) {
        ServerPlayer *victim = room->askForPlayerChosen(source, victims, "thbisha");
        room->setFixedDistance(source, victim, 1);
        room->setPlayerFlag(victim, "bishatarget");
        Slash *slash = new Slash(Card::NoSuitNoColor, 0);
        slash->setSkillName("thbisha");
        CardUseStruct use;
        use.card = slash;
        use.from = source;
        use.to << victim;
        victim->addMark("Qinggang_Armor_Nullified");
        room->useCard(use, false);
        victim->removeMark("Qinggang_Armor_Nullified");
    }
}

class ThBishaViewAsSkill:public OneCardViewAsSkill{
public:
    ThBishaViewAsSkill():OneCardViewAsSkill("thbisha"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !(player->containsTrick("indulgence") && player->containsTrick("supply_shortage")) && !player->hasUsed("ThBishaCard");
    }
    
    virtual bool viewFilter(const Card* to_select) const{
        return (to_select->isBlack() && !to_select->isKindOf("TrickCard")) || to_select->getSuit() == Card::Diamond;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if(originalCard->isRed() && Self->containsTrick("indulgence"))
            return NULL;

        if(originalCard->isBlack() && Self->containsTrick("supply_shortage"))
            return NULL;

        ThBishaCard *card = new ThBishaCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThBisha:public TriggerSkill {
public:
    ThBisha():TriggerSkill("thbisha"){
        events << EventPhaseChanging;
        view_as_skill = new ThBishaViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct &change = data.value<PhaseChangeStruct>();
        if(change.to == Player::NotActive)
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if(p->hasFlag("bishatarget")) {
                    room->setFixedDistance(player, p, -1);
                    room->setPlayerFlag(p, "-bishatarget");
                }

        return false;
    }
};

class ThShenzhou:public TriggerSkill{
public:
    ThShenzhou():TriggerSkill("thshenzhou") {
        frequency = Frequent;
        events << Damaged;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(!player->askForSkillInvoke(objectName()))
            return false;

        int x = qMin(room->alivePlayerCount(), 5);
        QList<int> card_ids = room->getNCards(x);
        room->fillAG(card_ids, NULL);
        int card_id = room->askForAG(player, card_ids, false, objectName());
        CardMoveReason reason;
        reason.m_reason=CardMoveReason::S_REASON_NATURAL_ENTER;
        QString type = Sanguosha->getCard(card_id)->getType();
        foreach(int id, card_ids)
            if(Sanguosha->getCard(id)->getType() != type) {
                card_ids.removeOne(id);
                room->moveCardTo(Sanguosha->getCard(id), NULL, Player::DiscardPile, reason);
            }

        foreach(ServerPlayer *p, room->getPlayers())
            p->invoke("clearAG");
        
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName());

        LogMessage log;

        log.type = "#thshenzhou";
        log.from = player;
        log.to << target;
        log.arg = QString::number(card_ids.size());
        log.arg2 = type;

        room->sendLog(log);
        foreach(int id, card_ids)
            target->obtainCard(Sanguosha->getCard(id));

        return false;
    }
};

class ThTianliu:public TriggerSkill{
public:
    ThTianliu():TriggerSkill("thtianliu"){
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Draw)
            return false;

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg  = objectName();
        room->sendLog(log);

        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(spade|club):(.*)");
        judge.good = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);
        if(judge.isGood())
            room->setEmotion(player, "good");
        else
            room->setEmotion(player, "bad");

        if(judge.card->getSuit() == Card::Heart)
            player->drawCards(3);
        else if(judge.card->getSuit() == Card::Diamond)
            player->drawCards(2);
        else if(judge.card->getSuit() == Card::Club)
            player->drawCards(1);
        
        return true;
    }
};

class ThQianyi:public TriggerSkill{
public:
    ThQianyi():TriggerSkill("thqianyi"){
        events << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::NotActive)
            return false;

        const Card *card = data.value<CardResponseStruct>().m_card;
        
        if(!card || !card->isKindOf("Jink"))
            return false;

        if(!player->askForSkillInvoke(objectName()))
            return false;

        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());

        LogMessage log;
        log.type = "$ShowAllCardsToOther";
        log.from = target;
        log.to << player;
        room->sendLog(log);

        QList<int> card_ids = target->handCards();
        room->fillAG(card_ids, player);
        int card_id;
        bool sucess = false;
        const Card *newcard;
        while(!sucess) {
            card_id = room->askForAG(player, card_ids, true, objectName());
            if(card_id == -1)
                sucess = true;
            else {
                newcard = Sanguosha->getCard(card_id);
                if(newcard->getType() == "trick") 
                    sucess = true;
            }
        }
        player->invoke("clearAG");
        if(card_id == -1)
            return false;

        target = room->askForPlayerChosen(player, room->getOtherPlayers(target), objectName());
        target->obtainCard(newcard);

        return false;
    }
};

ThHuosuiCard::ThHuosuiCard(){
}

bool ThHuosuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->inMyAttackRange(to_select) && to_select != Self;
}

void ThHuosuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
	if(!room->askForCard(target, "jink", "@thhuosuijink", QVariant(), Card::MethodResponse))
        if(!room->askForUseSlashTo(source, target, "@thhuosui-slash", false))
            source->drawCards(1);
}

class ThHuosui:public OneCardViewAsSkill{
public:
    ThHuosui():OneCardViewAsSkill("thhuosui"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThHuosuiCard");
    }
    
    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThHuosuiCard *card = new ThHuosuiCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThTiandi:public TriggerSkill{
public:
    ThTiandi():TriggerSkill("thtiandi"){
        frequency = Frequent;
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use=data.value<CardUseStruct>();
        if(!use.card->isKindOf("Slash"))
            return false;

        if(player->askForSkillInvoke(objectName()))
            player->drawCards(1);

        return false;
    }
};

ThKunyiCard::ThKunyiCard(){
}

bool ThKunyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->inMyAttackRange(to_select);
}

void ThKunyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    source->loseMark("@kunyi");
    source->gainMark("@kunyiused");
    //room->broadcastInvoke("animate", "lightbox:$kunyi");

    source->turnOver();

    DamageStruct damage;
    damage.from = source;
    damage.to = target;
    room->damage(damage);
}

class ThKunyiViewAsSkill: public ZeroCardViewAsSkill{
public:
    ThKunyiViewAsSkill():ZeroCardViewAsSkill("thkunyi"){
        frequency = Limited;
    }

    virtual const Card *viewAs() const{
        return new ThKunyiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@kunyi") > 0;
    }
};

class ThKunyi: public TriggerSkill{
public:
	ThKunyi():TriggerSkill("thkunyi") {
		events << GameStart;
		view_as_skill = new ThKunyiViewAsSkill;
	}

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
		player->gainMark("@kunyi");

		return false;
	}
};

ThCannueCard::ThCannueCard(){
    will_throw = false;
}

bool ThCannueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void ThCannueCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(this);
    Room *room = effect.to->getRoom();
    QString choice = room->askForChoice(effect.to, "thcannue", "slash+other");
    if(choice == "slash") {
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *player, room->getOtherPlayers(effect.to))
            if(effect.to->canSlash(player) && player != effect.from)
                targets << player;

        if(!targets.isEmpty()) {
            ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "thcannue");
            if(!room->askForUseSlashTo(effect.to, target, "@thcannue-slash"))
                choice = "other";
        }
        else
            choice="other";
    }
    if(choice == "other") {
        choice = room->askForChoice(effect.from, "thcannue", "get+hit");
        if(choice == "get") {
            int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thcannue");
            effect.from->obtainCard(Sanguosha->getCard(card_id));
        }
        else {
            DamageStruct damage;
            damage.from = effect.from;
            damage.to = effect.to;
            room->damage(damage);
        }
    }
};

class ThCannue:public OneCardViewAsSkill{
public:
    ThCannue():OneCardViewAsSkill("thcannue"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThCannueCard");
    }
    
    virtual bool viewFilter(const Card* to_select) const{
        return to_select->getSuit() == Card::Diamond;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThCannueCard *card = new ThCannueCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThSibao:public OneCardViewAsSkill{
public:
    ThSibao():OneCardViewAsSkill("thsibao"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "peach+analeptic";
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isKindOf("EquipCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *jiu = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        jiu->addSubcard(originalCard->getId());
        jiu->setSkillName(objectName());
        return jiu;
    }
};

ThWangminCard::ThWangminCard(){
    will_throw = false;
}

bool ThWangminCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void ThWangminCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    source->addToPile("wangminpile", this, false);
    targets.first()->gainMark("@zhusi");
}

class ThWangminViewAsSkill:public OneCardViewAsSkill{
public:
    ThWangminViewAsSkill():OneCardViewAsSkill("thwangmin"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getPile("wangminpile").isEmpty();
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThWangminCard *card = new ThWangminCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThWangmin:public TriggerSkill{
public:
    ThWangmin():TriggerSkill("thwangmin"){
        events << EventPhaseStart;
        view_as_skill = new ThWangminViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Play)
            return false;

        if(player->getMark("@zhusi") < 1)
            return false;

        player->loseAllMarks("@zhusi");
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if(splayer == NULL || splayer->getPile("wangminpile").isEmpty())
            return false;

        const Card *card = room->askForCardShow(player, splayer, objectName());
        room->showCard(player, card->getId());
        room->showCard(splayer, splayer->getPile("wangminpile").first());
        const Card *newcard = Sanguosha->getCard(splayer->getPile("wangminpile").first());
        int ninus = qAbs(card->getNumber() - newcard->getNumber());
        QString choice = "cancel";
        if(ninus > splayer->getHp() + 2)
            if(!player->isNude())
                choice = room->askForChoice(splayer, objectName(), "discard+get+cancel");
            else
                choice = room->askForChoice(splayer, objectName(), "discard+cancel");

        if(choice == "discard") {
            room->throwCard(newcard, splayer);
            DamageStruct damage;
            damage.from = splayer;
            damage.to = player;
            room->damage(damage);
        }
        else if(choice == "get") {
            splayer->obtainCard(newcard);
            int card_id = room->askForCardChosen(splayer, player, "he", objectName());
            room->throwCard(card_id, player, splayer);
        }
        else {
            player->obtainCard(newcard);
            player->drawCards(1);
        }
        return false;
    }
};

ThGelongCard::ThGelongCard(){
    will_throw = false;
}

bool ThGelongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThGelongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    bool success = source->pindian(target, "thgelong", Sanguosha->getCard(getEffectiveId()));
    QString choice;
    if(success) {
        choice="losehp";
        if(!source->isKongcheng())
            choice = room->askForChoice(source, "thgelong", "give+losehp");
    }
    else {
        choice="damage";
        if(!target->isNude())
            choice = room->askForChoice(source, "thgelong", "damage+get");
    }
    int card_id;
    if(choice == "give") {
        card_id = room->askForCardChosen(source, source, "h", "thgelong");
        room->moveCardTo(Sanguosha->getCard(card_id), target, Player::PlaceHand, false);
    }
    else if(choice == "losehp")
        room->loseHp(source);
    else if(choice == "damage") {
        DamageStruct damage;
        damage.from = source;
        damage.to = target;
        room->damage(damage);
    }
    else {
        card_id = room->askForCardChosen(source, target, "he", "thgelong");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, source->objectName());
        room->obtainCard(source, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
    }
}

class ThGelong:public OneCardViewAsSkill{
public:
    ThGelong():OneCardViewAsSkill("thgelong"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThGelongCard");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThGelongCard *card = new ThGelongCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThYuanzhou:public TriggerSkill{
public:
    ThYuanzhou():TriggerSkill("thyuanzhou"){
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Finish)
            return false;

        bool invoke = true;
        int maxnum = 0;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if(p->getHandcardNum() < player->getHandcardNum()) {
                invoke = false;
                break;
            }

            if(p->getHandcardNum() > maxnum)
                maxnum = p->getHandcardNum();
        }
        if(!invoke)
            return false;

        if(player->askForSkillInvoke(objectName())) {
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getAllPlayers())
                if(p->getHandcardNum() == maxnum && !p->isAllNude())
                    targets << p;

            if(!targets.isEmpty()) {
                ServerPlayer *target = room->askForPlayerChosen(player, targets, "thyuanzhou");
                int card_id = room->askForCardChosen(player, target, "hej", "thyuanzhou");
                room->throwCard(card_id, target, player);
            }
        }
        return false;
    }
};

ThDasuiCard::ThDasuiCard(){
    target_fixed = true;
    will_throw = false;
}

void ThDasuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    source->addToPile("dasuipile", this);
}

class ThDasuiViewAsSkill:public ViewAsSkill{
public:
    ThDasuiViewAsSkill():ViewAsSkill("thdasui"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThDasuiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.size() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.size() != 0) {
            ThDasuiCard *card = new ThDasuiCard;
            card->addSubcards(cards);
            return card;
        }
        else
            return NULL;
    }
};

class ThDasui:public TriggerSkill{
public:
    ThDasui():TriggerSkill("thdasui") {
        events << EventPhaseStart;
        view_as_skill = new ThDasuiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Play)
            return false;

        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if(splayer == NULL || player == splayer || splayer->getPile("dasuipile").isEmpty())
            return false;

        if(splayer->askForSkillInvoke(objectName())) {
            QList<int> card_ids = splayer->getPile("dasuipile");
            room->fillAG(card_ids, NULL);
            int card_id = room->askForAG(player, card_ids, true, objectName());
            foreach(ServerPlayer *p, room->getPlayers())
                p->invoke("clearAG");

            if(card_id == -1)
                return false;

            room->obtainCard(player, card_id);
        }
        return false;
    }
};

class ThFengrang:public TriggerSkill{
public:
    ThFengrang():TriggerSkill("thfengrang"){
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Start)
            return false;

        QList<int> card_ids = player->getPile("dasuipile");
        if(card_ids.isEmpty())
            return false;

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg  = objectName();
        room->sendLog(log);

        foreach(int card_id, card_ids)
            room->obtainCard(player, card_id);

        return false;
    }
};

class ThFuli:public TriggerSkill{
public:
    ThFuli():TriggerSkill("thfuli"){
        frequency = Frequent;
        events << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct &use = data.value<CardUseStruct>();
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if(use.from != splayer && use.to.contains(splayer) && use.card->isKindOf("Slash")
                && splayer->askForSkillInvoke(objectName()))
            splayer->addToPile("dasuipile", use.card);

        return false;
    }
};

ThYanlunCard::ThYanlunCard(){
    will_throw = false;
    mute = true;
}

bool ThYanlunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThYanlunCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    bool success = source->pindian(target, "thyanlun", Sanguosha->getCard(getEffectiveId()));
    room->broadcastSkillInvoke("yanlun", 1);
    if(success)
        room->setPlayerFlag(source, "yanlun");
    else {
        DamageStruct damage;
        damage.from = target;
        damage.to = source;
        damage.nature = DamageStruct::Fire;
        room->damage(damage);
    }
}

class ThYanlunViewAsSkill:public OneCardViewAsSkill{
public:
    ThYanlunViewAsSkill():OneCardViewAsSkill("thyanlun"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThYanlunCard") || player->hasFlag("yanlun");
    }

    virtual bool viewFilter(const Card* to_select) const{
        if(Self->hasFlag("yanlun"))
            return !to_select->isEquipped() && to_select->isRed();
        else
            return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if(Self->hasFlag("yanlun")) {
            FireAttack *huogong = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
            huogong->addSubcard(originalCard);
            huogong->setSkillName("thyanlun_fire_attack");
            return huogong;
        }
        else {
            ThYanlunCard *card = new ThYanlunCard;
            card->addSubcard(originalCard);
            return card;
        }
    }
};

class ThYanlun:public TriggerSkill{
public:
    ThYanlun():TriggerSkill("thyanlun"){
        events << EventPhaseChanging << Damage;
        view_as_skill = new ThYanlunViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == Damage) {
            DamageStruct &damage = data.value<DamageStruct>();
            if(player->hasFlag("yanlun") && damage.card->isKindOf("FireAttack")
                    && player->askForSkillInvoke(objectName()))
                player->drawCards(1);

            return false;
        }
        else if(triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct &change = data.value<PhaseChangeStruct>();
            if(change.to == Player::NotActive && player->hasFlag("yanlun"))
                room->setPlayerFlag(player, "-yanlun");

            return false;
        }

        return false;
    }
};

class ThYanlunFireAttack:public TriggerSkill{
public:
    ThYanlunFireAttack():TriggerSkill("#thyanlun"){
        frequency = Compulsory;
        events << CardEffect;
    }

    virtual int getPriority() const{
        return -2;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct &effect = data.value<CardEffectStruct>();
        if(!player->hasFlag("yanlun") || !effect.card->isKindOf("FireAttack"))
            return false;

        if(effect.card->getSkillName() == "thyanlun_fire_attack")
            room->broadcastSkillInvoke("thyanlun", qrand() % 2 + 2);

        RoomThread *thread=room->getThread();
        QVariant new_data = QVariant::fromValue((CardEffectStruct)effect);
        room->setTag("SkipGameRule", true);
        bool avoid = thread->trigger(CardEffected, room, effect.to, new_data);
        if(avoid)
            return true;

        bool canceled = room->isCanceled(effect);
        if(!canceled) {
            if(effect.to->isKongcheng())
                return true;

            const Card *card = room->askForCardShow(effect.to, effect.from, effect.card->objectName());
            room->showCard(effect.to, card->getEffectiveId());
            QString color = "";
            if(card->isRed())
                color = "red";
            else
                color = "black";

            QString pattern = "."+color;
            QString prompt = QString("@fire-attackex:%1::%2").arg(effect.to->getGeneralName()).arg(color);
            if(room->askForCard(effect.from, pattern, prompt)) {
                DamageStruct damage;
                damage.card = effect.card;
                damage.from = effect.from;
                damage.to = effect.to;
                damage.nature = DamageStruct::Fire;
                room->damage(damage);
            }

            return true;
        }
        return true;
    }
};

ThYanxingCard::ThYanxingCard(){
    target_fixed = true;
    will_throw = false;
}

void ThYanxingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if(room->askForChoice(source, "thyanxing", "hp+maxhp") == "hp")
        room->loseHp(source);
    else
        room->loseMaxHp(source);

    if(source->isAlive())
        room->setPlayerFlag(source, "thyanxing");
}

class ThYanxingViewAsSkill:public ZeroCardViewAsSkill{
public:
    ThYanxingViewAsSkill():ZeroCardViewAsSkill("thyanxing"){
    }
    
    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThYanxingCard");
    }

    virtual const Card *viewAs() const{
        return new ThYanxingCard;
    }
};

class ThYanxing:public TriggerSkill{
public:
    ThYanxing():TriggerSkill("thyanxing"){
        events << DamageComplete;
        view_as_skill = new ThYanxingViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct &damage = data.value<DamageStruct>();
        if(!damage.from || !damage.from->hasFlag("thyanxing"))
            return false;
        
		if(damage.card && damage.card->isKindOf("NatureSlash") && damage.to->isAlive() 
				&& damage.from->askForSkillInvoke(objectName())) {
            DamageStruct newdamage;
            newdamage.from = damage.from;
            newdamage.to = damage.to;
            room->damage(newdamage);
        }

        return false;
    }
};

class ThHere:public FilterSkill{
public:
    ThHere():FilterSkill("thhere"){
    }

    virtual bool viewFilter(const Card* to_select) const{
		if (!to_select->isKindOf("Lightning") && !(to_select->isKindOf("Jink") && to_select->getSuit() == Card::Diamond))
			return false;

		Room *room = Sanguosha->currentRoom();
		ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        foreach(const Player *p, room->getAllPlayers())
            if (splayer->getHpPoints() > p->getHpPoints())
                return true;

		return false;
	}
    
    virtual const Card *viewAs(const Card *originalCard) const{
        FireSlash *huosha = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        huosha->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(huosha);
        return card;
    }
};

class ThHereStateChange:public TriggerSkill{
public:
    ThHereStateChange():TriggerSkill("#thhere"){
        frequency = Compulsory;
        events << HpChanged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
		ServerPlayer *p = room->findPlayerBySkillName("thhere");
        room->filterCards(p, p->getCards("he"), true);
        return false;
    }
};

ThMaihuoCard::ThMaihuoCard(){
    will_throw = false;
}

bool ThMaihuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void ThMaihuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    target->obtainCard(this);
    Card::Suit suit = room->askForSuit(source, "thmaihuo");
    
    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = source;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->showAllCards(target, NULL);
    room->getThread()->delay(2000);
    QList<int> card_ids = target->handCards();
    int num = 0;
    foreach(int card_id, card_ids)
        if(Sanguosha->getCard(card_id)->getSuit() == suit)
            num = num + 1;

    if(num != 0)
        target->drawCards(qMin(3, num));
}

class ThMaihuo:public OneCardViewAsSkill{
public:
    ThMaihuo():OneCardViewAsSkill("thmaihuo"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThMaihuoCard");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThMaihuoCard *card = new ThMaihuoCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThWunian:public TriggerSkill{
public:
    ThWunian():TriggerSkill("thwunian"){
        frequency = Compulsory;
        events << Predamage << CardEffected;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        LogMessage log;

        log.type = "#TriggerSkill";
        log.from = player;
        log.arg  = objectName();

        if(triggerEvent == Predamage) {
            room->sendLog(log);
            DamageStruct &damage = data.value<DamageStruct>();
            damage.from = NULL;
            data = QVariant::fromValue((DamageStruct)damage);
            return false;
        }
        else if(triggerEvent == CardEffected) {
            CardEffectStruct &effect = data.value<CardEffectStruct>();
            if((effect.card->isNDTrick() || effect.card->isKindOf("Slash")) && !effect.from->isWounded()) {
                room->sendLog(log);
                return true;
            }
        }
        return false;
    }
};

void TouhouPackage::addKazeGenerals(){
    General *kaze001 = new General(this, "kaze001$", "shu", 3);
    kaze001->addSkill(new ThQiji);
    kaze001->addSkill(new ThJiyi);
    kaze001->addSkill(new ThQiyuan);

    General *kaze002 = new General(this, "kaze002", "shu");
    kaze002->addSkill(new ThJilanwen);

    General *kaze003 = new General(this, "kaze003", "shu", 3);
    kaze003->addSkill(new ThNianxie);
    kaze003->addSkill(new ThJilan);

    General *kaze004 = new General(this, "kaze004", "shu");
    kaze004->addSkill(new ThWangshou);
    kaze004->addSkill(new ThHongye);
    
    General *kaze005 = new General(this, "kaze005", "shu", 3, false);
    kaze005->addSkill(new ThEnan);
    kaze005->addSkill(new ThBeiyun);
    
    General *kaze007 = new General(this, "kaze007", "shu");
    kaze007->addSkill(new ThBisha);
    
    General *kaze008 = new General(this, "kaze008", "shu");
    kaze008->addSkill(new ThShenzhou);
    kaze008->addSkill(new ThTianliu);
    kaze008->addSkill(new ThQianyi);
    
    General *kaze009 = new General(this, "kaze009", "shu", 3, false);
    kaze009->addSkill(new ThHuosui);
    kaze009->addSkill(new ThTiandi);
    kaze009->addSkill(new ThKunyi);

    General *kaze010 = new General(this, "kaze010", "shu", 3);
    kaze010->addSkill(new ThCannue);
    kaze010->addSkill(new ThSibao);

    General *kaze011 = new General(this, "kaze011", "shu");
    kaze011->addSkill(new ThWangmin);

    General *kaze012 = new General(this, "kaze012", "shu");
    kaze012->addSkill(new ThGelong);
    kaze012->addSkill(new ThYuanzhou);

    General *kaze013 = new General(this, "kaze013", "shu", 3);
    kaze013->addSkill(new ThDasui);
    kaze013->addSkill(new ThFengrang);
    kaze013->addSkill(new ThFuli);

    General *kaze015 = new General(this, "kaze015", "shu");
    kaze015->addSkill(new ThYanlun);
    kaze015->addSkill(new ThYanlunFireAttack);
    related_skills.insertMulti("thyanlun", "#thyanlun");

    General *kaze016 = new General(this, "kaze016", "shu", 5);
    kaze016->addSkill(new ThYanxing);
    kaze016->addSkill(new ThHere);
    kaze016->addSkill(new ThHereStateChange);
    related_skills.insertMulti("thhere", "#thhere");

    General *kaze017 = new General(this, "kaze017", "shu", 3, false);
    kaze017->addSkill(new ThMaihuo);
    kaze017->addSkill(new ThWunian);

    addMetaObject<ThJiyiCard>();
    addMetaObject<ThNianxieCard>();
    addMetaObject<ThEnanCard>();
    addMetaObject<ThBishaCard>();
    addMetaObject<ThHuosuiCard>();
    addMetaObject<ThKunyiCard>();
    addMetaObject<ThCannueCard>();
    addMetaObject<ThWangminCard>();
    addMetaObject<ThGelongCard>();
    addMetaObject<ThDasuiCard>();
    addMetaObject<ThYanlunCard>();
    addMetaObject<ThYanxingCard>();
    addMetaObject<ThMaihuoCard>();
}