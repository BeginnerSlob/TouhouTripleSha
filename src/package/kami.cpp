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
        if (data.value<PlayerStar>() != player || player->getPhase() != Player::Start || !player->askForSkillInvoke(objectName()))
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

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == Death || (data.value<PlayerStar>() == player && player->getPhase() == Player::RoundStart))
            && !room->getTag("ThJiguang").toString().isEmpty())
        {
            if (triggerEvent == Death)
            {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return false;
            }

            QString name = room->getTag("ThJiguang").toString();
            if (triggerEvent != Death)
                player->loseAllMarks("@" + name);
            if (name == "jgfengyu")
                foreach(ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, "jgfengyu", 0);
            else if (name == "jgtantian")
                foreach(ServerPlayer *p, room->getAllPlayers())
                    room->detachSkillFromPlayer(p, "thjiguanggivenskill", true);
        }
        else if (triggerEvent == EventPhaseStart && data.value<PlayerStar>() == player && player->getPhase() == Player::Start)
        {
            QStringList choices;
            choices << "jglieri" << "jgfengyu" << "jghuangsha" << "jgtantian" << "jgnongwu";
            if (!player->askForSkillInvoke(objectName()))
                room->setTag("ThJiguang", QVariant::fromValue(QString()));
            else
            {
                QString name = room->getTag("ThJiguang").toString();
                choices.removeOne(name);
                QString choice = room->askForChoice(player, objectName(), choices.join("+"));
                room->setTag("ThJiguang", QVariant::fromValue(choice));
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
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageInflicted)
            if (damage.nature == DamageStruct::Fire && room->getTag("ThJiguang").toString() == "jglieri")
            {
                damage.damage++;
                data = QVariant::fromValue(damage);
                return false;
            }
            else if (damage.damage > 1 && room->getTag("ThJiguang").toString() == "jghuangsha")
            {
                damage.damage = 1;
                data = QVariant::fromValue(damage);
                return false;
            }
            else
                return false;
        else if (triggerEvent == Damage)
            if (room->getTag("ThJiguang").toString() == "jgnongwu" && player->distanceTo(damage.to) <= 1
                && damage.card->isKindOf("Slash"))
            {
                RecoverStruct recover;
                recover.who = player;
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
        attached_lord_skill = true;
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

class ThMengsheng: public TriggerSkill {
public:
    ThMengsheng(): TriggerSkill("thmengsheng") {
        events << Damaged << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL && target->hasInnateSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        if (triggerEvent == Damaged && TriggerSkill::triggerable(player))
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to != player || !player->askForSkillInvoke(objectName()))
                return false;

            if (!damage.from->isKongcheng())
                room->obtainCard(player, room->askForCardChosen(player, damage.from, "h", objectName()), false);

            if (damage.from->tag.value("ThMengsheng").isNull())
            {
                QStringList skills;
                foreach(const Skill *skill, damage.from->getVisibleSkillList())
                    if (skill->getLocation() == Skill::Right && !skill->isAttachedLordSkill())
                    {
                        skills << skill->objectName();
                        room->detachSkillFromPlayer(damage.from, skill->objectName());
                    }
                
                damage.from->gainMark("@mengsheng");
                damage.from->tag["ThMengsheng"] = QVariant::fromValue(skills);
                if (player == room->getCurrent())
                    room->setPlayerFlag(player, "thmengshengusing");
            }
        }
        else
        {
            if (triggerEvent == EventPhaseChanging)
            {
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.who != player || player->hasFlag("thmengshengusing") || change.to != Player::RoundEnd)
                    return false;
            }
            if (triggerEvent == Death && data.value<DeathStruct>().who != player)
                return false;

            foreach (ServerPlayer *p, room->getAllPlayers())
                if (!p->tag.value("ThMengsheng").isNull())
                {
                    QStringList skills = p->tag.value("ThMengsheng").toStringList();
                    p->tag.remove("ThMengsheng");
                    room->setPlayerMark(p, "@mengsheng", 0);
                    foreach (QString skill, skills)
                        room->acquireSkill(p, skill);
                }
        }

        return false;
    }
};

class ThQixiang: public TriggerSkill {
public:
    ThQixiang(): TriggerSkill("thqixiang") {
        events << EventPhaseEnd;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *splayer, QVariant &data) const {
        ServerPlayer *player = data.value<PlayerStar>();

        if (splayer == player || splayer->isKongcheng())
            return false;

        if (player->getPhase() == Player::Discard
            && splayer->inMyAttackRange(player)
            && room->askForCard(splayer, ".", "@thqixiang", data, objectName()))
        {
            QString choice = "draw";
            if (!player->isKongcheng())
                choice = room->askForChoice(splayer, objectName(), "draw+discard");

            if (choice == "draw")
                player->drawCards(1);
            else
                room->askForDiscard(player, objectName(), 1, 1);
        }

        return false;
    }
};

ThGugaoCard::ThGugaoCard() {
    will_throw = false;
    handling_method = MethodPindian;
}

bool ThGugaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThGugaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    ServerPlayer *target = targets[0];
    source->pindian(target, "thgugao", this);
}

class ThGugaoViewAsSkill: public OneCardViewAsSkill {
public:
    ThGugaoViewAsSkill(): OneCardViewAsSkill("thgugao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThGugaoCard") || (player->getMark("@huangyi") > 0 && player->hasFlag("thgugao"));
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThGugaoCard *card = new ThGugaoCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThGugao: public TriggerSkill {
public:
    ThGugao(): TriggerSkill("thgugao") {
        events << Pindian;
        view_as_skill = new ThGugaoViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        PindianStar pindian = data.value<PindianStar>();
        if (pindian->reason != objectName())
            return false;

        if (pindian->isSuccess())
        {
            room->setPlayerFlag(pindian->from, "thgugao");
            DamageStruct damage;
            damage.from = pindian->from;
            damage.to = pindian->to;
            room->damage(damage);
        }
        else
        {
            if (pindian->from->hasFlag("thgugao"))
                room->setPlayerFlag(pindian->from, "-thgugao");
            if (player->getMark("@qianyu") < 1
                || pindian->from_card->getSuit() == pindian->to_card->getSuit())
            {
                DamageStruct damage;
                damage.from = pindian->to;
                damage.to = pindian->from;
                room->damage(damage);
            }
        }

        return false;
    }
};

class ThQianyu: public TriggerSkill {
public:
    ThQianyu(): TriggerSkill("thqianyu") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *srcplayer = data.value<PlayerStar>();
        if (player->getHp() != 1)
            return false;

        if (srcplayer->getPhase() == Player::NotActive && player->getMark("@qianyu") <= 0)
        {
            LogMessage log;
            log.type = "#ThQianyu";
            log.from = player;
            log.arg = objectName();
            log.arg2 = QString::number(player->getHp());
            room->sendLog(log);

            player->gainMark("@qianyu");
            if (player->getMaxHp() > 1)
                room->loseMaxHp(player, player->getMaxHp() - 1);
            room->acquireSkill(player, "thkuangmo");
            player->gainAnExtraTurn(srcplayer);
        }

        return false;
    }
};

class ThKuangmo: public TriggerSkill {
public:
    ThKuangmo(): TriggerSkill("thkuangmo") {
        events << Damage;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash"))
            return false;
        if (!damage.from->hasSkill("thgugao") || damage.from == damage.to)
            return false;

        while (damage.damage--)
        {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        }

        return false;
    }
};

class ThHuangyi: public TriggerSkill {
public:
    ThHuangyi(): TriggerSkill("thhuangyi") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return TriggerSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("@huangyi") <= 0
               && target->getMark("@qianyu") > 0
               && target->getMaxHp() > target->getGeneralMaxHp();
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() != player)
            return false;
        player->gainMark("@huangyi");
        int n = player->getMaxHp() - player->getGeneralMaxHp();
        room->loseMaxHp(player, n);
        player->drawCards(n);

        if (player->isWounded())
        {
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        }

        room->detachSkillFromPlayer(player, "thkuangmo");

        return false;
    }
};

class ThFanhun: public TriggerSkill{
public:
    ThFanhun(): TriggerSkill("thfanhun") {
        events << AskForPeaches;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        if(dying_data.who != player)
            return false;

        if(player->hasSkill("thmanxiao") && player->askForSkillInvoke(objectName(), data)){
            room->broadcastSkillInvoke(objectName());
			
			RecoverStruct recover;
            recover.recover = 1 - player->getHp();
			recover.who = player;
            room->recover(player, recover);

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
        if (damage.from && damage.from == player
            && damage.card && damage.card->isRed()
            && !damage.chain && !damage.transfer
            && player->hasSkill("thmanxiao")
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
    dummy_card->addSubcards(effect.to->getCards("he"));
    dummy_card->deleteLater();
    if (!dummy_card->getSubcards().isEmpty())
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

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() != player)
            return false;

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
        events << TurnedOver << ChainStateChanged;
        frequency = Frequent;
    }
    
    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == ChainStateChanged && data.value<PlayerStar>() != player)
            return false;

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

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() != player || player->getPhase() != Player::Draw
            || player->isNude() || !room->askForCard(player, "..", "@thyuxin", QVariant(), objectName()))
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

        return true;
    }
};

ThChuangxinCard::ThChuangxinCard(){
    target_fixed = true;
}

void ThChuangxinCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if (subcardsLength() == 1)
    {
        QString choice = room->askForChoice(source, "thchuangxin", "lingshi+zhuoyue");
        room->acquireSkill(source, choice);
    }
    else
    {
        room->acquireSkill(source, "lingshi");
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

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::Play)
            room->askForUseCard(player, "@@thchuangxin", "@thchuangxin");
        else if (player->getPhase() == Player::NotActive)
        {
            room->detachSkillFromPlayer(player, "lingshi");
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

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::RoundStart)
            room->askForUseCard(player, "@@thtianxin", "@thtianxin");
        else if (player->getPhase() == Player::NotActive)
        {
            room->detachSkillFromPlayer(player, "yuxi");
            room->detachSkillFromPlayer(player, "tiandu");
        }

        return false;
    }
};

class ThWunan: public TriggerSkill{
public:
    ThWunan(): TriggerSkill("thwunan"){
        events << CardUsed << HpRecovered << Damaged << CardResponded << DamageCaused;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *target = NULL;
        if (triggerEvent == CardUsed)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if ((!use.card->isKindOf("GodSalvation") && !use.card->isKindOf("AmazingGrace")))
                return false;
            target = use.from;
        }
        else if (triggerEvent == HpRecovered)
        {
            RecoveredStruct recover = data.value<RecoveredStruct>();
            int hp = recover.target->getHpPoints();
            if (hp < 1 || hp - recover.recover > 0)
                return false;
            target = recover.target;
        }
        else if (triggerEvent == Damaged)
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to == player || damage.nature != DamageStruct::Fire)
                return false;
            target = damage.to;
        }
        else if (triggerEvent == CardResponded)
        {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (!resp.m_card->isKindOf("Jink")
                || resp.m_card->getSkillName() != "EightDiagram")
                return false;
            target = resp.m_src;
        }
        else if (triggerEvent == DamageCaused)
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.from || !damage.card || !damage.card->isKindOf("Slash") || !damage.to->isKongcheng())
                return false;
            target = damage.from;
        }
        else
            return false;

        if (target == player || target->isDead())
            return false;

        if (room->askForCard(player, ".", "@thwunan", QVariant(), objectName()))
        {
            if (player->isWounded())
            {
                RecoverStruct recover;
                recover.who = player;
                room->recover(player, recover);
            }
            if (!target->isKongcheng())
                room->throwCard(room->askForCardChosen(player, target, "h", objectName()), target, player);
        }

        return false;
    }
};

class ThSanling: public TriggerSkill{
public:
    ThSanling(): TriggerSkill("thsanling"){
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *srcplayer = data.value<PlayerStar>();

        if (srcplayer->getPhase() == Player::NotActive && player->isKongcheng())
        {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg  = objectName();
            room->sendLog(log);
            room->killPlayer(player, NULL);
        }

        return false;
    }
};

class ThBingzhang: public TriggerSkill{
public:
    ThBingzhang(): TriggerSkill("thbingzhang"){
        events << DamageForseen << HpLost;
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
                 && move->to && move->to != player
                 && (move->to_place == Player::PlaceHand
                     || move->to_place == Player::PlaceEquip))
                || // 上两行判断第二条件，下两行判断第一条件
                (move->to == NULL 
                 && move->to_place == Player::DiscardPile
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

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::Start
            && player->askForSkillInvoke(objectName()))
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
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.who == player && change.to == Player::Draw)
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

class ThLuli: public TriggerSkill{
public:
    ThLuli(): TriggerSkill("thluli"){
        events << DamageCaused << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(triggerEvent == DamageCaused){
            if (damage.from && damage.from == player && damage.to->getHpPoints() >= player->getHpPoints()
                && damage.to != player && !player->isKongcheng())
                if(room->askForCard(player, ".black", "@ThLuliIncrease", data, objectName())){
                    LogMessage log;
                    log.type = "#ThLuliIncrease";
                    log.from = player;
                    log.arg = QString::number(damage.damage);
                    log.arg2 = QString::number(++damage.damage);
                    room->sendLog(log);

                    data = QVariant::fromValue(damage);
                }
        }else if(triggerEvent == DamageInflicted){
            if(damage.from && damage.from->isAlive()
               && damage.from->getHp() >= player->getHp() && damage.from != player && !player->isKongcheng())
                if(room->askForCard(player, ".red", "@ThLuliDecrease", data, objectName())){
                    LogMessage log;
                    log.type = "#ThLuliDecrease";
                    log.from = player;
                    log.arg = QString::number(damage.damage);
                    log.arg2 = QString::number(--damage.damage);
                    room->sendLog(log);

                    if (damage.damage < 1){
                        LogMessage log;
                        log.type = "#ZeroDamage";
                        log.from = damage.from;
                        log.to << player;
                        room->sendLog(log);
                        return true;
                    }
                    data = QVariant::fromValue(damage);
                }
        }

        return false;
    }
};

class ThGuihuan: public TriggerSkill {
public:
    ThGuihuan(): TriggerSkill("thguihuan") {
        events << BeforeGameOverJudge;
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        if (!room->getMode().endsWith("p") && !room->getMode().endsWith("pd") && !room->getMode().endsWith("pz"))
            return false;
        DamageStar damage = data.value<DamageStar>();
        if (damage == NULL)
            return false;
        ServerPlayer *killer = damage->from;
        if (killer == NULL || killer->isLord() || player->isLord() || player->getHp() > 0)
            return false;
        if (!TriggerSkill::triggerable(killer) || killer->getMark("@guihuan") == 0)
            return false;
        if (room->askForSkillInvoke(killer, objectName())) {
            room->broadcastSkillInvoke(objectName());
            killer->loseMark("@guihuan");
            killer->gainMark("@guihuanused");
            QString role1 = killer->getRole();
            killer->setRole(player->getRole());
            room->setPlayerProperty(killer, "role", player->getRole());
            player->setRole(role1);
            room->setPlayerProperty(player, "role", role1);
        }
        return false;
    }
};

class ThZhizun: public TriggerSkill {
public:
    ThZhizun(): TriggerSkill("thzhizun"){
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        if(data.value<PlayerStar>() != player || player->getPhase() != Player::Finish)
            return false;

        if(!player->askForSkillInvoke(objectName()))
            return false;
        QStringList choices;
        choices << "modify";
        //===========================
        QStringList lord_skills;
        lord_skills << "liqi" << "huanwei" << "jiyuan" << "yuji" 
            << "thqiyuan" << "songwei" << "thchundu" << "wuhua";

        foreach(QString lord_skill, lord_skills)
            if (room->findPlayerBySkillName(lord_skill))
                lord_skills.removeOne(lord_skill);

        if(!lord_skills.isEmpty())
            choices << "obtain";
        //===========================

        QString choice = room->askForChoice(player, objectName(), choices.join("+"));

        if(choice == "modify")
        {
            ServerPlayer *to_modify = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
            QStringList kingdoms;
            kingdoms << "shu" << "wei" << "wu" << "qun";
            kingdoms.removeOne(to_modify->getKingdom());
            QString kingdom = room->askForChoice(player, objectName(), kingdoms.join("+"));
            QString old_kingdom = to_modify->getKingdom();
            room->setPlayerProperty(to_modify, "kingdom", kingdom);

            LogMessage log;
            log.type = "#ChangeKingdom";
            log.from = player;
            log.to << to_modify;
            log.arg = old_kingdom;
            log.arg2 = kingdom;
            room->sendLog(log);

        }
        else if(choice == "obtain")
        {
            QString skill_name = room->askForChoice(player, objectName(), lord_skills.join("+"));

            if (skill_name.isEmpty())
                return false;
            
            const Skill *skill = Sanguosha->getSkill(skill_name);
            room->acquireSkill(player, skill);

            if(skill->inherits("GameStartSkill")){
                const GameStartSkill *game_start_skill = qobject_cast<const GameStartSkill *>(skill);
                game_start_skill->onGameStart(player);
            }
        }

        room->broadcastSkillInvoke(objectName());

        return false;
    }
};

class ThFeiying: public DistanceSkill{
public:
    ThFeiying(): DistanceSkill("thfeiying"){

    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(to->hasSkill(objectName()))
            return 1;
        else
            return 0;
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

    General *kami003 = new General(this, "kami003", "god", 3);
    kami003->addSkill(new ThMengsheng);
    kami003->addSkill(new ThQixiang);

    General *kami004 = new General(this, "kami004", "god");
    kami004->addSkill(new ThGugao);
    kami004->addSkill(new ThQianyu);
    kami004->addRelateSkill("thkuangmo");
    kami004->addSkill(new ThHuangyi);

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

    General *kami012 = new General(this, "kami012", "god");
    kami012->addSkill(new ThWunan);

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

    General *kami014 = new General(this, "kami014", "god", 3, false);    
    kami014->addSkill(new ThLuli);
    kami014->addSkill(new ThGuihuan);
    kami014->addSkill(new MarkAssignSkill("@guihuan", 1));
    related_skills.insertMulti("thguihuan", "#@guihuan-1");

    General *kami015 = new General(this, "kami015", "god", 3);
    kami015->addSkill(new ThZhizun);
    kami015->addSkill(new ThFeiying);
    
    addMetaObject<ThShenfengCard>();
    addMetaObject<ThGugaoCard>();
    addMetaObject<ThYouyaCard>();
    addMetaObject<ThJinluCard>();
    addMetaObject<ThChuangxinCard>();
    addMetaObject<ThTianxinCard>();

    skills << new ThJiguangDistanceSkill << new ThJiguangGivenSkill << new ThKuangmo;
}

ADD_PACKAGE(Kami)