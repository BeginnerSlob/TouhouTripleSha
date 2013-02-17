#include "bangai.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"
#include "standard-equips.h"

class ThBianfang: public TriggerSkill{
public:
    ThBianfang():TriggerSkill("thbianfang"){
        events << Damage << Damaged;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (!player->isWounded() || !damage.card || !damage.card->isKindOf("Slash"))
            return false;
        if (triggerEvent == Damage && (damage.chain || damage.transfer))
            return false;
        if (triggerEvent == Damaged && damage.to != player)
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

class ThChuandao: public TriggerSkill{
public:
    ThChuandao(): TriggerSkill("thchuandao"){
        events << DrawNCards << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DrawNCards && player->getMark("@yaoshu") > 0 && player->askForSkillInvoke(objectName()))
            data = data.toInt() + 1;
        else if (triggerEvent == Death && player->hasSkill(objectName()))
        {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;

            ServerPlayer *target;
            QList<ServerPlayer *> players = room->getAllPlayers();
            players << player;
            foreach(ServerPlayer *p, players)
                if (p->getMark("@yaoshu") > 0)
                {
                    target = p;
                    break;
                }

            if (target == NULL)
                return false;
            QList<ServerPlayer *> targets = room->getAllPlayers();
            if (target != player)
                targets.removeOne(target);
            if (targets.isEmpty())
                return false;

            if (player->askForSkillInvoke(objectName()))
            {
                target->loseAllMarks("@yaoshu");
                target = room->askForPlayerChosen(player, targets, objectName());
                target->gainMark("@yaoshu");
            }
            
            if (target != player)
            {
                target->drawCards(2);

                room->acquireSkill(target, objectName());
            }
        }

        return false;
    }
};

ThShoujuanCard::ThShoujuanCard(){
}

bool ThShoujuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void ThShoujuanCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->loseAllMarks("@yaoshu");
    effect.to->gainMark("@yaoshu");
}

class ThShoujuanViewAsSkill: public ZeroCardViewAsSkill{
public:
    ThShoujuanViewAsSkill(): ZeroCardViewAsSkill("thshoujuan"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@yaoshu") > 0;
    }

    virtual const Card *viewAs() const{
        return new ThShoujuanCard;
    }
};

class ThShoujuan: public TriggerSkill{
public:
    ThShoujuan(): TriggerSkill("thshoujuan"){
        events << Dying << DamageCaused;
        view_as_skill = new ThShoujuanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *owner = NULL;
        if (triggerEvent == DamageCaused)
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.from || damage.from->getMark("@yaoshu") <= 0 || !damage.to->hasSkill(objectName()) || damage.from == damage.to)
                return false;
            else
                owner = damage.to;
        }
        else if (triggerEvent == Dying)
        {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who == player || dying.who->getMark("@yaoshu") <= 0)
                return false;
            else
                owner = player;
        }

        if(owner && owner->askForSkillInvoke(objectName()))
        {
            owner->turnOver();
            room->moveCardTo(player->wholeHandCards(), owner, Player::PlaceHand);
            player->loseAllMarks("@yaoshu");
            owner->gainMark("@yaoshu");
        }

        return false;
    }
};

class ThShuling: public TriggerSkill{
public:
    ThShuling(): TriggerSkill("thshuling"){
        events << GameStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const{
        player->gainMark("@yaoshu");
        return false;
    }
};

class ThBanyue: public TriggerSkill{
public:
    ThBanyue():TriggerSkill("thbanyue"){
        events << PreCardUsed << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from != player || !use.card->isKindOf("Slash"))
            return false;

        if (triggerEvent == PreCardUsed)
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
                    
                    LogMessage log;
                    log.type = "#ThBanyue";
                    log.from = player;
                    log.to << target;
                    log.arg = objectName();
                    log.arg2 = use.card->objectName();
                    room->sendLog(log);
                    
                    qSort(use.to.begin(), use.to.end(), ServerPlayer::CompareByActionOrder);
                    data = QVariant::fromValue(use);
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

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Judge && data.value<PlayerStar>() == player)
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

class ThXijing: public TriggerSkill{
public:
    ThXijing(): TriggerSkill("thxijing"){
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive || player->isKongcheng())
            return false;

        CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
        if (move->from != player
            || move->to_place != Player::DiscardPile)
            return false;
        
        for (int i = 0; i < move->card_ids.length(); i++)
            if (move->from_places[i] != Player::PlaceSpecial
                && move->from_places[i] != Player::PlaceDelayedTrick
                && !Sanguosha->getEngineCard(move->card_ids[i])->isKindOf("EquipCard"))
            {
                if (player->askForSkillInvoke(objectName()))
                    break;
                else
                    return false;
            }

        //lose
        CardsMoveStruct move1(QList<int>(),
            NULL,
            Player::DiscardPile,
            move->reason);

        //get
        CardMoveReason reason(CardMoveReason::S_REASON_RECYCLE,
            player->objectName(),
            objectName(),
            QString());
        CardsMoveStruct move2(QList<int>(),
            player,
            Player::PlaceHand,
            reason);

        for (int i = 0; i < move->card_ids.length(); i++)
        {
            if (move->from_places[i] != Player::PlaceSpecial
                && move->from_places[i] != Player::PlaceDelayedTrick
                && !Sanguosha->getEngineCard(move->card_ids[i])->isKindOf("EquipCard"))
            {
                const Card *c = Sanguosha->getEngineCard(move->card_ids[i]);
                QString prompt = "@thxijing:" + c->getSuitString()
                    + ":" + QString::number(c->getNumber())
                    + ":" + c->objectName();
                QString pattern = ".";
                if (c->isBlack())
                    pattern = ".black";
                else if (c->isRed())
                    pattern = ".red";

                const Card *card = room->askForCard(player, pattern, prompt, QVariant(), Card::MethodNone);
                
                if (card)
                {
                    move1.card_ids.append(card->getEffectiveId());
                    move2.card_ids.append(move->card_ids[i]);
                }
            }
        }

        if (move1.card_ids.length() == move1.card_ids.length()
            && !move1.card_ids.isEmpty())
        {
            QList<CardsMoveStruct> moves;
            moves.push_back(move1);
            moves.push_back(move2);
            room->setPlayerFlag(player, "ThXijing_InTempMoving");
            room->moveCardsAtomic(moves, true);
            room->setPlayerFlag(player, "-ThXijing_InTempMoving");
        }

        return false;
    }
};

class ThXijingAvoidTriggeringCardsMove: public TriggerSkill{
public:
    ThXijingAvoidTriggeringCardsMove():TriggerSkill("#thxijing"){
        events << CardsMoveOneTime;
    }

    virtual int getPriority() const{
        return 10;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const{
        if (player->hasFlag("ThXijing_InTempMoving"))
            return true;
        return false;
    }
};

class ThMengwei: public TriggerSkill{
public:
    ThMengwei():TriggerSkill("thmengwei"){
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Finish && data.value<PlayerStar>() == player && player->getHandcardNum() < 2)
        {
            if (player->askForSkillInvoke(objectName()))
                player->drawCards(2 - player->getHandcardNum());
        }
        else if (player->getPhase() == Player::Start)
        {
            ServerPlayer *srcplayer = data.value<PlayerStar>();
            if (srcplayer == player)
                return false;

            if (player->isKongcheng() && player->askForSkillInvoke(objectName()))
                player->drawCards(1);
        }

        return false;
    }
};

class ThTianyi: public FilterSkill {
public:
    ThTianyi(): FilterSkill("thtianyi") {
    }

    virtual bool viewFilter(const Card* to_select) const{
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return to_select->isKindOf("Jink") && (place == Player::PlaceHand || place == Player::PlaceEquip);
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        EightDiagram *bagua = new EightDiagram(originalCard->getSuit(), originalCard->getNumber());
        bagua->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(bagua);
        return card;
    }
};

class ThTianyiTriggerSkill: public TriggerSkill {
public:
    ThTianyiTriggerSkill(): TriggerSkill("#thtianyi") {
        events << CardsMoveOneTime << EventLoseSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player) && !player->hasFlag("ThTianyi_InTempMoving"))
        {
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            if(move->from != player || move->from == NULL)
                return false;

            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct tianyiMove;
            
            for(int i = 0; i < move->card_ids.size(); i++){
                int card_id = move->card_ids[i];
                if((Sanguosha->getEngineCard(card_id)->isKindOf("Jink") || Sanguosha->getEngineCard(card_id)->isKindOf("EightDiagram"))
                    && move->from_places[i] == Player::PlaceEquip)
                    tianyiMove.card_ids << card_id;
            }

            if(tianyiMove.card_ids.empty())
                return false;

            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
            tianyiMove.to = target;
            tianyiMove.to_place = Player::PlaceHand;

            exchangeMove.push_back(tianyiMove);
            // iwillback
            room->setPlayerFlag(player, "ThTianyi_InTempMoving");
            room->moveCardsAtomic(exchangeMove, true);
            room->setPlayerFlag(player, "-ThTianyi_InTempMoving");

            QList<ServerPlayer *> victims;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (!p->isNude())
                    victims << p;

            if (!victims.isEmpty())
            {
                ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
                room->throwCard(room->askForCardChosen(player, victim, "he", objectName()), victim, player);
            }
        }
        else if (triggerEvent == EventLoseSkill && data.toString() == "thtianyi" && player->getArmor())
        {
            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct tianyiMove;
            
            tianyiMove.card_ids << player->getArmor()->getEffectiveId();

            if(tianyiMove.card_ids.first() == -1)
                return false;

            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
            tianyiMove.to = target;
            tianyiMove.to_place = Player::PlaceHand;

            exchangeMove.push_back(tianyiMove);
            // iwillback
            room->setPlayerFlag(player, "ThTianyi_InTempMoving");
            room->moveCardsAtomic(exchangeMove, true);
            room->setPlayerFlag(player, "-ThTianyi_InTempMoving");

            QList<ServerPlayer *> victims;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (!p->isNude())
                    victims << p;

            if (!victims.isEmpty())
            {
                ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
                room->throwCard(room->askForCardChosen(player, victim, "he", objectName()), victim, player);
            }
        }
        return false;
    }
};

class ThHuilun: public TriggerSkill {
public:
    ThHuilun(): TriggerSkill("thhuilun") {
        events << PreCardUsed << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging)
        {
            PhaseChangeStruct change =  data.value<PhaseChangeStruct>();
            if (change.to == Player::Play)
                room->setPlayerMark(change.who, "thhuilun", 0);
        }
        else if (triggerEvent == PreCardUsed)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.from || use.from->getPhase() != Player::Play)
                return false;

            if (use.card->isKindOf("Slash") && room->askForCard(player, ".black", "@thhuilun", QVariant(), objectName()))
            {
                LogMessage log;
                log.type = "#ThHuilun";
                log.from = player;
                log.to << use.from;
                log.arg = objectName();
                log.arg2 = use.card->objectName();
                room->sendLog(log);

                room->setPlayerMark(use.from, "thhuilun", use.from->getMark("thhuilun") + 1);
                if (use.card->isRed())
                    player->drawCards(1);
            }
        }

        return false;
    }
};

class ThHuilunTargetModSkill: public TargetModSkill {
public:
    ThHuilunTargetModSkill(): TargetModSkill("#thhuilun") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->getMark("thhuilun") > 0)
            return from->getMark("thhuilun");
        else
            return 0;
    }
};

ThYuboCard::ThYuboCard(){
}

bool ThYuboCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.length() < 2 && !to_select->isChained();
}

void ThYuboCard::onEffect(const CardEffectStruct &effect) const {
    effect.to->getRoom()->setPlayerProperty(effect.to, "chained", true);
}

class ThYubo: public OneCardViewAsSkill {
public:
    ThYubo(): OneCardViewAsSkill("thyubo") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThYuboCard");
    }

    virtual bool viewFilter(const Card* card) const{
        return card->isBlack() && !card->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThYuboCard *card = new ThYuboCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThQiongfa: public TriggerSkill {
public:
    ThQiongfa(): TriggerSkill("thqiongfa") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *srcplayer = data.value<PlayerStar>();
        if (!srcplayer->isChained() || srcplayer->getPhase() != Player::Start || !player->askForSkillInvoke(objectName()))
            return false;

        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getOtherPlayers(srcplayer))
            if (!p->isNude())
                victims << p;

        ServerPlayer *victim = NULL;
        if (!victims.isEmpty())
        {
            victim = room->askForPlayerChosen(player, victims, objectName());
            LogMessage log;
            log.type = "#ThQiongfa";
            log.from = player;
            log.to << victim;
            room->sendLog(log);
        }

        if (victim && room->askForChoice(srcplayer, objectName(), "discard+cancel") == "discard")
            room->throwCard(room->askForCardChosen(srcplayer, victim, "he", objectName()), victim, srcplayer);
        else
            player->drawCards(1);

        room->setPlayerProperty(srcplayer, "chained", false);

        return false;
    }
};

class ThMozhuang: public TriggerSkill {
public:
    ThMozhuang(): TriggerSkill("thmozhuang") {
        events << DrawNCards << TargetConfirmed << CardFinished;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        if (triggerEvent == DrawNCards && player->getWeapon() && TriggerSkill::triggerable(player))
        {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            int n = qMax((qobject_cast<const Weapon*>(player->getWeapon()->getRealCard()))->getRange(), 2);
            player->drawCards(n);
            return true;
        }
        else if (triggerEvent == TargetConfirmed && player->getArmor()) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!player->isAlive() || player != use.from || !use.card->isKindOf("Slash") || player->getPhase() != Player::Play)
                return false;
            int count = 1;
            int mark_n = player->getMark("no_jink" + use.card->toString());
            foreach (ServerPlayer *p, use.to) {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);
                
                LogMessage log2;
                log2.type = "#NoJink";
                log2.from = p;
                room->sendLog(log2);

                mark_n += count;
                room->setPlayerMark(player, "no_jink" + use.card->toString(), mark_n);

                count *= 10;
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player && use.card->isKindOf("Slash"))
                room->setPlayerMark(player, "no_jink" + use.card->toString(), 0);
        }

        return false;
    }
};

class ThMozhuangMaxCardsSkill: public MaxCardsSkill {
public:
    ThMozhuangMaxCardsSkill(): MaxCardsSkill("#thmozhuang") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill("thmozhuang"))
        {
            int horses = 0;
            if (target->getOffensiveHorse())
                horses++;
            if (target->getDefensiveHorse())
                horses++;
            return horses;
        }
        else
            return 0;
    }
};

class ThWeide: public TriggerSkill{
public:
    ThWeide():TriggerSkill("thweide"){
        events << DrawNCards << EventPhaseEnd;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DrawNCards && player->isWounded())
        {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (p->isWounded())
                    targets << p;

            if (targets.isEmpty() || !player->askForSkillInvoke(objectName()))
                return false;

            room->setPlayerFlag(player, "thweideused");
            int x = qMin(player->getLostHp(), 2);
            data = data.toInt() - x;

            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            target->drawCards(x);
        }
        else if (triggerEvent == EventPhaseEnd && data.value<PlayerStar>() == player
                 && player->getPhase() == Player::Draw && player->hasFlag("thweideused"))
        {
            room->setPlayerFlag(player, "-thweideused");
            QList<ServerPlayer *> victims;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (p->getHpPoints() >= player->getHpPoints())
                    victims << p;

            if (victims.isEmpty())
                return false;

            ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
            DummyCard *dummy = room->askForCardsChosen(player, victim, "h", objectName(), qMin(player->getLostHp(), 2));
            if (dummy->subcardsLength() > 0)
                player->obtainCard(dummy, false);
            dummy->deleteLater();
        }

        return false;
    }
};

ThGuijuanCard::ThGuijuanCard() {
    target_fixed = true;
}

void ThGuijuanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    source->drawCards(1);
    const Card *card = source->getHandcards().last();
    room->showCard(source, card->getId());
    if (!card->isAvailable(source) || !room->askForUseCard(source, card->getEffectIdString(), "@thguijuan"))
        room->loseHp(source);
}

class ThGuijuan: public ZeroCardViewAsSkill {
public:
    ThGuijuan(): ZeroCardViewAsSkill("thguijuan") {
    }

    virtual const Card *viewAs() const{
        return new ThGuijuanCard;
    }
};

class ThZhayou: public TriggerSkill {
public:
    ThZhayou(): TriggerSkill("thzhayou") {
        events << SlashMissed;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isAlive() && effect.to->hasSkill(objectName()) && effect.to->askForSkillInvoke(objectName()))
        {    
            effect.to->drawCards(1);
            if (!room->askForUseSlashTo(effect.from, effect.to, "@thzhayou:" + effect.to->objectName()))
            {
                DamageStruct damage;
                damage.from = effect.to;
                damage.to = effect.from;
                room->damage(damage);
            }
        }

        return false;
    }
};

ThSixiangCard::ThSixiangCard(){
}

bool ThSixiangCard::targetFixed() const {
    return Sanguosha->getCard(getEffectiveId())->getSuit() == Card::Spade;
}
    
bool ThSixiangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if(Sanguosha->getCard(getEffectiveId())->getSuit() != Card::Spade)
        return targets.size() == 1;
    else
        return targets.isEmpty();
}

bool ThSixiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Card::Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    if(suit == Card::Spade)
        return false;
    else if(suit == Card::Diamond)
        return targets.isEmpty() && !to_select->isAllNude() && to_select != Self;
    else
        return targets.isEmpty() && to_select != Self;
}

void ThSixiangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    Card::Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    ServerPlayer *target = NULL;
    if(!targets.isEmpty())
        target = targets.first();

    if(suit == Card::Spade)
    {
        RecoverStruct recover;
        recover.who = source;
        room->recover(source, recover);
    }
    else if(suit == Card::Heart)
    {
        target->drawCards(1);
        target->turnOver();
    }
    else if(suit == Card::Club)
    {
        target->drawCards(2);
        if(!target->isNude())
            room->askForDiscard(target, "thsixiang", 1, 1, false, true);
    }
    else if(suit == Card::Diamond)
    {
        int card_id = room->askForCardChosen(source, target, "hej", "thsixiang");
        room->throwCard(card_id, target, source);
    }
}

class ThSixiang:public ViewAsSkill{
public:
    ThSixiang():ViewAsSkill("thsixiang"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(selected.isEmpty())
            return !(!Self->isWounded() && to_select->getSuit() == Card::Spade);
        else if(selected.length() == 1)
            return to_select->getSuit() == selected.first()->getSuit();
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.length() == 2){
            ThSixiangCard *card = new ThSixiangCard;
            card->addSubcards(cards);
            return card;
        }else
            return NULL;
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
        return !player->hasUsed("ThLuanshenCard") && player->getHandcardNum() >= player->getHpPoints();
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

    virtual bool triggerable(const ServerPlayer *target) const {
        if (target->getMark("Equips_Nullified_to_Yourself") > 0) return false;
        return !target->getWeapon() && target->hasSkill("thsilian");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!effect.to->isAlive() || effect.to->getMark("Equips_of_Others_Nullified_to_You") > 0)
            return false;
        if (!effect.from->canSlash(effect.to, NULL, false))
            return false;

        room->setPlayerFlag(effect.from, "BladeUse");
        room->askForUseSlashTo(effect.from, effect.to, QString("blade-slash:%1").arg(effect.to->objectName()), false, false, true);
        room->setPlayerFlag(effect.from, "-BladeUse");
        
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

class ThXiangrui: public TriggerSkill{
public:
    ThXiangrui():TriggerSkill("thxiangrui"){
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() != player || player->getPhase() != Player::Discard || player->getHandcardNum() == player->getHpPoints())
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
        bool invoke = false;
        foreach(const Player *p, player->getSiblings())
            if (!p->getEquips().isEmpty())
            {
                invoke = true;
                break;
            }
        return invoke && player->getMark("@xingxie") > 0 && !player->isKongcheng();
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

class ThWuqi: public TriggerSkill {
public:
    ThWuqi(): TriggerSkill("thwuqi") {
        events << TargetConfirming << CardEffected;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming)
        {
            ServerPlayer *target = NULL;
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->getMark("TargetConfirming"))
                {
                    target = p;
                    break;
                }
            if (!target || target != player)
                return false;
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isNDTrick() && use.card->isBlack())
            {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);
                if (room->askForChoice(use.from, objectName(), "show+cancel") == "show")
                {
                    room->showAllCards(use.from, player);
                    player->drawCards(1);
                }
                else
                    player->tag["ThWuqi"] = use.card->toString();
            }
        }
        else if (triggerEvent == CardEffected)
        {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.to != player || player->tag["ThWuqi"].isNull()
                || player->tag["ThWuqi"].toString() != effect.card->toString())
                return false;

            player->tag["ThWuqi"] = QVariant(QString());

            LogMessage log;
            log.type = "#ThWuqiAvoid";
            log.from = player;
            log.arg = effect.card->objectName();
            log.arg2 = objectName();
            room->sendLog(log);

            return true;
        }

        return false;
    }
};

class ThZhanfu: public TriggerSkill{
public:
    ThZhanfu():TriggerSkill("thzhanfu"){
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *target = data.value<PlayerStar>();
        if(player == target || target->getPhase() != Player::Start || target->isKongcheng() || !player->askForSkillInvoke(objectName()))
            return false;

        const Card *card = room->askForCardShow(target, player, "@thzhanfuchoose:" + player->objectName());
        if(card != NULL) {
            QString choice = room->askForChoice(player, objectName(), "basic+equip+trick");
            
            LogMessage log;
            log.type = "#ThZhanfu";
            log.from = player;
            log.arg = choice;
            room->sendLog(log);

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
            if (move->card_ids[i] == id) {
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

    General *bangai002 = new General(this, "bangai002", "wei", 3);
    bangai002->addSkill(new ThChuandao);
    bangai002->addSkill(new ThShoujuan);
    bangai002->addSkill(new ThShuling);

    General *bangai003 = new General(this, "bangai003", "wu");
    bangai003->addSkill(new ThBanyue);

    General *bangai004 = new General(this, "bangai004", "qun", 3, false);
    bangai004->addSkill(new ThZusha);
    bangai004->addSkill(new ThYaomei);
    bangai004->addSkill(new ThZhongjie);

    General *bangai005 = new General(this, "bangai005", "shu", 3);
    bangai005->addSkill(new ThXijing);
    bangai005->addSkill(new ThXijingAvoidTriggeringCardsMove);
    bangai005->addSkill(new ThMengwei);
    related_skills.insertMulti("thxijing", "#thxijing");

    General *bangai006 = new General(this, "bangai006", "wei");
    bangai006->addSkill(new ThTianyi);
    bangai006->addSkill(new ThTianyiTriggerSkill);
    related_skills.insertMulti("thtianyi", "#thtianyi");

    General *bangai007 = new General(this, "bangai007", "wu");
    bangai007->addSkill(new ThHuilun);
    bangai007->addSkill(new ThHuilunTargetModSkill);
    related_skills.insertMulti("thhuilun", "#thhuilun");

    General *bangai008 = new General(this, "bangai008", "qun", 3);
    bangai008->addSkill(new ThYubo);
    bangai008->addSkill(new ThQiongfa);

    General *bangai009 = new General(this, "bangai009", "shu");
    bangai009->addSkill(new ThMozhuang);
    bangai009->addSkill(new ThMozhuangMaxCardsSkill);
    related_skills.insertMulti("thmozhuang", "#thmozhuang");

    General *bangai010 = new General(this, "bangai010", "wei");
    bangai010->addSkill(new ThWeide);

    General *bangai011 = new General(this, "bangai011", "wu", 3, false);
    bangai011->addSkill(new ThGuijuan);
    bangai011->addSkill(new ThZhayou);

    General *bangai012 = new General(this, "bangai012", "qun");
    bangai012->addSkill(new ThSixiang);

    General *bangai013 = new General(this, "bangai013", "shu");
    bangai013->addSkill(new ThLuanshen);

    General *bangai014 = new General(this, "bangai014", "wei");
    bangai014->addSkill(new ThSilian);
    bangai014->addSkill(new ThSilianGet);
    bangai014->addSkill(new ThSilianTriggerSkill);
    bangai014->addSkill(new ThLingzhan);
    related_skills.insertMulti("thsilian", "#thsilian");
    related_skills.insertMulti("thsilian", "#thsilian-weapon");

    General *bangai015 = new General(this, "bangai015", "wu");
    bangai015->addSkill(new ThXiangrui);
    bangai015->addSkill(new ThXingxie);

    General *bangai016 = new General(this, "bangai016", "qun", 3);
    bangai016->addSkill(new ThWuqi);
    bangai016->addSkill(new ThZhanfu);
    bangai016->addSkill(new ThZhanfuClear);
    related_skills.insertMulti("thzhanfu", "#thzhanfu");
    
    addMetaObject<ThShoujuanCard>();
    addMetaObject<ThZushaCard>();
    addMetaObject<ThYaomeiCard>();
    addMetaObject<ThYuboCard>();
    addMetaObject<ThGuijuanCard>();
    addMetaObject<ThSixiangCard>();
    addMetaObject<ThLuanshenCard>();
    addMetaObject<ThLingzhanCard>();
    addMetaObject<ThXingxieCard>();
}

ADD_PACKAGE(Bangai)