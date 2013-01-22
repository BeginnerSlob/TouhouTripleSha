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

class Jianxiong:public MasochismSkill{
public:
    Jianxiong():MasochismSkill("jianxiong"){
    }

    virtual void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const{
        Room *room = caocao->getRoom();
        const Card *card = damage.card;
        if(card && room->getCardPlace(card->getEffectiveId()) == Player::PlaceTable){

            QVariant data = QVariant::fromValue(card);
            if(room->askForSkillInvoke(caocao, "jianxiong", data)){
            room->broadcastSkillInvoke(objectName());
                caocao->obtainCard(card);
            }
        }
    }
};

class Hujia:public TriggerSkill{
public:
    Hujia():TriggerSkill("hujia$"){
        events << CardAsked;
        default_choice = "ignore";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("hujia");
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *caocao, QVariant &data) const{
        QString pattern = data.toStringList().first();
        if(pattern != "jink")
            return false;

        QList<ServerPlayer *> lieges = room->getLieges("wei", caocao);
        if(lieges.isEmpty())
            return false;

        if(!room->askForSkillInvoke(caocao, objectName()))
            return false;

        room->broadcastSkillInvoke(objectName());
        QVariant tohelp = QVariant::fromValue((PlayerStar)caocao);
        foreach(ServerPlayer *liege, lieges){
            const Card *jink = room->askForCard(liege, "jink", "@hujia-jink:" + caocao->objectName(),
                                                tohelp, Card::MethodResponse, caocao);
            if(jink){
                room->provide(jink);
                return true;
            }
        }

        return false;
    }
};

TiansuoCard::TiansuoCard(){
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodResponse;
}

void TiansuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{

}

class TiansuoViewAsSkill:public OneCardViewAsSkill{
public:
    TiansuoViewAsSkill():OneCardViewAsSkill(""){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@tiansuo";
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new TiansuoCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Tiansuo: public TriggerSkill{
public:
    Tiansuo():TriggerSkill("tiansuo"){
        view_as_skill = new TiansuoViewAsSkill;

        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target) && !target->isKongcheng();
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();

        QStringList prompt_list;
        prompt_list << "@tiansuo-card" << judge->who->objectName()
                << objectName() << judge->reason << judge->card->getEffectIdString();
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, "@tiansuo", prompt, data, Card::MethodResponse, judge->who, true);
        if (card != NULL){
            if (player->hasInnateSkill("tiansuo") || !player->hasSkill("jilve"))
                room->broadcastSkillInvoke(objectName());
            else
                room->broadcastSkillInvoke("jilve", 1);
            room->retrial(card, player, judge, objectName());
        }

        return false;
    }
};

class Huanji:public MasochismSkill{
public:
    Huanji():MasochismSkill("huanji"){

    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = simayi->getRoom();
        QVariant data = QVariant::fromValue(from);
        if(from && !from->isAllNude() && room->askForSkillInvoke(simayi, "huanji", data)){
            room->broadcastSkillInvoke(objectName());
            int card_id = room->askForCardChosen(simayi, from, "hej", "huanji");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, simayi->objectName());
            room->obtainCard(simayi, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
        }
    }
};

TuxiCard::TuxiCard(){
}

bool TuxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.length() >= 2)
        return false;

    if(to_select == Self)
        return false;

    return !to_select->isKongcheng();
}

void TuxiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    QList<CardsMoveStruct> moves;
    CardsMoveStruct move1;
    move1.card_ids << room->askForCardChosen(source, targets[0], "h", "tuxi");
    move1.to = source;
    move1.to_place = Player::PlaceHand;
    moves.push_back(move1);
    if(targets.length() == 2)
    {
        CardsMoveStruct move2;
        move2.card_ids << room->askForCardChosen(source, targets[1], "h", "tuxi");
        move2.to = source;
        move2.to_place = Player::PlaceHand;
        moves.push_back(move2);
    }
    room->moveCards(moves, false);
}

class TuxiViewAsSkill: public ZeroCardViewAsSkill{
public:
    TuxiViewAsSkill():ZeroCardViewAsSkill("tuxi"){
    }

    virtual const Card *viewAs() const{
        return new TuxiCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@tuxi";
    }
};

class Tuxi:public PhaseChangeSkill{
public:
    Tuxi():PhaseChangeSkill("tuxi"){
        view_as_skill = new TuxiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *zhangliao) const{
        if(zhangliao->getPhase() == Player::Draw){
            Room *room = zhangliao->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(zhangliao);
            foreach(ServerPlayer *player, other_players){
                if(!player->isKongcheng()){
                    can_invoke = true;
                    break;
                }
            }

            if(can_invoke && room->askForUseCard(zhangliao, "@@tuxi", "@tuxi-card"))
                return true;
        }

        return false;
    }
};

class Ganglie:public MasochismSkill{
public:
    Ganglie():MasochismSkill("ganglie"){

    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();
        QVariant source = QVariant::fromValue(from);

        if(from && from->isAlive() && room->askForSkillInvoke(xiahou, "ganglie", source)){
            room->broadcastSkillInvoke(objectName());

            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(heart):(.*)");
            judge.good = false;
            judge.reason = objectName();
            judge.who = xiahou;

            room->judge(judge);
            if(judge.isGood()){
                if(!room->askForDiscard(from, objectName(), 2, 2, true)){
                    DamageStruct damage;
                    damage.from = xiahou;
                    damage.to = from;

                    room->setEmotion(xiahou, "good");
                    room->damage(damage);
                }
            }else
                room->setEmotion(xiahou, "bad");
        }
    }
};

class LuoyiBuff: public TriggerSkill{
public:
    LuoyiBuff():TriggerSkill("#luoyi"){
        events << ConfirmDamage;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag("luoyi") && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *xuchu, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        const Card *reason = damage.card;
        if(reason == NULL)
            return false;

        if(reason->isKindOf("Slash") || reason->isKindOf("Duel")){
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++ damage.damage);
            xuchu->getRoom()->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

LuoyiCard::LuoyiCard(){
    once = true;
    target_fixed = true;
}

void LuoyiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->setFlags("luoyi");
}

class Luoyi: public OneCardViewAsSkill{
public:
    Luoyi():OneCardViewAsSkill("luoyi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("LuoyiCard") && !player->isNude();
    }

    virtual bool viewFilter(const Card *card) const{
        return card->isKindOf("EquipCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LuoyiCard *card = new LuoyiCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Tiandu:public TriggerSkill{
public:
    Tiandu():TriggerSkill("tiandu"){
        frequency = Frequent;
        events << FinishJudge;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *guojia, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        CardStar card = judge->card;

        QVariant data_card = QVariant::fromValue(card);
        if(guojia->askForSkillInvoke(objectName(), data_card)){
            guojia->obtainCard(judge->card);
            room->broadcastSkillInvoke(objectName());
            room->getThread()->delay(500);

            return true;
        }

        return false;
    }
};

class Yumeng:public MasochismSkill{
public:
    Yumeng():MasochismSkill("yumeng"){
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();

        if(!room->askForSkillInvoke(player, objectName()))
            return;

        room->broadcastSkillInvoke(objectName());

        int x = damage.damage, i;
        for(i = 0; i < x; i++)
        {
            QList<int> yumeng_cards;
            yumeng_cards.append(room->drawCard());
            yumeng_cards.append(room->drawCard());
            CardsMoveStruct move;
            move.card_ids = yumeng_cards;
            move.to = player;
            move.to_place = Player::PlaceHand;
            move.reason = CardMoveReason(CardMoveReason::S_REASON_SHOW, player->objectName(), "yumeng", QString());
            room->moveCards(move, false);

            if(yumeng_cards.isEmpty())
                continue;

            while(room->askForYumeng(player, yumeng_cards));
        }

    }
};

class Mengyang:public TriggerSkill{
public:
    Mengyang():TriggerSkill("mengyang"){
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
                judge.pattern = QRegExp("(.*):(spade|club):(.*)");
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
                if(judge->card->isBlack()){
                    player->obtainCard(judge->card);
                    return true;
                }
            }
        }

        return false;
    }
};

class Zhongyan:public OneCardViewAsSkill{
public:
    Zhongyan():OneCardViewAsSkill("zhongyan"){

    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isBlack() && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(originalCard->getId());
        return jink;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "jink";
    }
};

XunyuCard::XunyuCard(){
    mute = true;
}

bool XunyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->canSlash(to_select, NULL, false);
}

void XunyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    Slash *slash = new Slash(Card::NoSuitNoColor, 0);
    slash->setSkillName("xunyu");
    CardUseStruct use;
    use.card = slash;
    use.from = source;
    use.to = targets;

    room->useCard(use);
}

class XunyuViewAsSkill: public ViewAsSkill{
public:
    XunyuViewAsSkill():ViewAsSkill("xunyu"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1"))
            return false;
        else
            return selected.isEmpty() && !to_select->isKindOf("TrickCard");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        Slash *slash = new Slash(Card::NoSuitNoColor, 0);
        slash->deleteLater();
        if (pattern.startsWith("@@xunyu") && !player->isCardLimited(slash, Card::MethodUse))
            return true;

        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1")){
            if(cards.isEmpty())
                return new XunyuCard;
            else
                return NULL;
        }else{
            if(cards.length() != 1)
                return NULL;

            XunyuCard *card = new XunyuCard;
            card->addSubcards(cards);

            return card;
        }
    }
};

class Xunyu: public TriggerSkill{
public:
    Xunyu():TriggerSkill("xunyu"){
        events << EventPhaseChanging;
        view_as_skill = new XunyuViewAsSkill;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if(change.to == Player::Judge && !player->isSkipped(Player::Judge)
                && !player->isSkipped(Player::Draw)){
            if(room->askForUseCard(player, "@@xunyu1", "@xunyu1", 1)){
                player->skip(Player::Judge);
                player->skip(Player::Draw);
            }
        }else if(change.to == Player::Play && !player->isSkipped(Player::Play)){
            if(room->askForUseCard(player, "@@xunyu2", "@xunyu2", 2)){
                player->skip(Player::Play);
            }
        }
        return false;
    }
};

MancaiCard::MancaiCard(){
    will_throw = false;
    mute = true;
}

bool MancaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    Player::Phase phase = (Player::Phase)Self->getMark("mancaiPhase");
    if(phase == Player::Draw)
        return targets.length() <= 2 && !targets.isEmpty();
    else if(phase == Player::Play)
        return targets.length() == 1;
    return false;
}

bool MancaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Player::Phase phase = (Player::Phase)Self->getMark("mancaiPhase");
    if(phase == Player::Draw)
        return targets.length() < 2 && to_select != Self && !to_select->isKongcheng();
    else if(phase == Player::Play)
        return targets.isEmpty() &&
                (!to_select->getJudgingArea().isEmpty() || !to_select->getEquips().isEmpty());
    return false;
}

void MancaiCard::use(Room *room, ServerPlayer *zhanghe, QList<ServerPlayer *> &targets) const{
    Player::Phase phase = (Player::Phase)zhanghe->getMark("mancaiPhase");
    if(phase == Player::Draw){
        if(targets.isEmpty())
            return;

        QList<CardsMoveStruct> moves;
        CardsMoveStruct move1;
        move1.card_ids << room->askForCardChosen(zhanghe, targets[0], "h", "mancai");
        move1.to = zhanghe;
        move1.to_place = Player::PlaceHand;
        moves.push_back(move1);
        if(targets.length() == 2)
        {
            CardsMoveStruct move2;
            move2.card_ids << room->askForCardChosen(zhanghe, targets[1], "h", "mancai");
            move2.to = zhanghe;
            move2.to_place = Player::PlaceHand;
            moves.push_back(move2);
        }
        room->moveCards(moves, false);
    }else if(phase == Player::Play){
        if(targets.isEmpty())
            return;

        PlayerStar from = targets.first();
        if(!from->hasEquip() && from->getJudgingArea().isEmpty())
            return;

        int card_id = room->askForCardChosen(zhanghe, from , "ej", "mancai");
        const Card *card = Sanguosha->getCard(card_id);
        Player::Place place = room->getCardPlace(card_id);

        int equip_index = -1;
        if(place == Player::PlaceEquip){
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equip->location());
        }

        QList<ServerPlayer *> tos;
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            if(equip_index != -1){
                if(p->getEquip(equip_index) == NULL)
                    tos << p;
            }else{
                if(!zhanghe->isProhibited(p, card) && !p->containsTrick(card->objectName()))
                    tos << p;
            }
        }

        room->setTag("MancaiTarget", QVariant::fromValue(from));
        ServerPlayer *to = room->askForPlayerChosen(zhanghe, tos, "mancai");
        if(to)
            room->moveCardTo(card, from, to, place,
                CardMoveReason(CardMoveReason::S_REASON_TRANSFER, zhanghe->objectName(), "mancai", QString()));
        room->removeTag("MancaiTarget");
    }
}

class MancaiViewAsSkill: public ZeroCardViewAsSkill{
public:
    MancaiViewAsSkill():ZeroCardViewAsSkill("mancai"){

    }

    virtual const Card *viewAs() const{
        MancaiCard *card = new MancaiCard;
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@mancai";
    }
};

class Mancai: public TriggerSkill{
public:
    Mancai():TriggerSkill("mancai"){
        events << EventPhaseChanging;
        view_as_skill = new MancaiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->isKongcheng();
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        room->setPlayerMark(player, "mancaiPhase", (int)change.to);
        int index = 0;
        switch(change.to){
        case Player::RoundStart:
        case Player::Start:
        case Player::Finish:
        case Player::NotActive: return false;

        case Player::Judge: index = 1;break;
        case Player::Draw: index = 2;break;
        case Player::Play: index = 3;break;
        case Player::Discard: index = 4;break;
        case Player::PhaseNone: Q_ASSERT(false);

        }

        QString discard_prompt = QString("#mancai-%1").arg(index);
        QString use_prompt = QString("@mancai-%1").arg(index);
        if(index > 0 && room->askForDiscard(player, objectName(), 1, 1, true, false, discard_prompt)){
            room->broadcastSkillInvoke("mancai", index);
            if(!player->isSkipped(change.to) && (index == 2 || index == 3))
                room->askForUseCard(player, "@mancai", use_prompt, index);
            player->skip(change.to);
        }
        return false;
    }
};

class Fenghou: public OneCardViewAsSkill{
public:
    Fenghou():OneCardViewAsSkill("fenghou"){

    }

    virtual bool viewFilter(const Card *card) const{
        return card->isBlack() && !card->isKindOf("TrickCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{

        SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
        shortage->setSkillName(objectName());
        shortage->addSubcard(originalCard);

        return shortage;
    }
};

class Zhaihun: public TriggerSkill{
public:
    Zhaihun(): TriggerSkill("zhaihun"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && (target->hasSkill(objectName()) || target->getMark("@zhaihun")>0);
    }

    virtual int getPriority() const{
        return 3;
    }

    int getWeaponCount(ServerPlayer *player) const{
        int n = 0;
        foreach(ServerPlayer *p, player->getRoom()->getAlivePlayers()){
            if(p->getWeapon())
                n ++;
        }

        return n;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
        if(player->hasSkill(objectName()) && player->getPhase() == Player::Finish){
            if(!player->askForSkillInvoke(objectName()))
                return false;

            int n = getWeaponCount(player);
            player->drawCards(n+2);
            player->turnOver();
            if(player->getMark("@zhaihun") == 0)
                player->gainMark("@zhaihun");
        }
        else if(player->getPhase() == Player::Draw){
            if(player->getMark("@zhaihun") == 0)
                return false;

            int n = getWeaponCount(player);
            if(n > 0){
                LogMessage log;
                log.type = "#ZhaihunDiscard";
                log.from = player;
                log.arg = QString::number(n);
                log.arg2 = objectName();
                room->sendLog(log);
                if(player->getCards("he").length() <= n){
                    player->throwAllHandCardsAndEquips();
                }
                else{
                    room->askForDiscard(player, objectName(), n, n, false, true);
                }
            }

            player->loseMark("@zhaihun");
        }
        return false;
    }
};

class Fojiao: public OneCardViewAsSkill{
public:
    Fojiao():OneCardViewAsSkill("fojiao"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "nullification" && player->getHandcardNum() > player->getHp();
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());

        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        foreach(const Card *card, player->getHandcards()){
            if(card->objectName() == "nullification")
                return true;
        }

        return player->getHandcardNum() > player->getHp() && !player->getEquips().isEmpty();
    }
};

QiangxiCard::QiangxiCard(){
    once = true;
}

bool QiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first())
        return Self->distanceTo(to_select) <= 1;

    return Self->inMyAttackRange(to_select);
}

void QiangxiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    if(subcards.isEmpty())
        room->loseHp(effect.from);

    DamageStruct damage;
    damage.card = NULL;
    damage.from = effect.from;
    damage.to = effect.to;

    room->damage(damage);
}

class Qiangxi: public ViewAsSkill{
public:
    Qiangxi():ViewAsSkill("qiangxi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("QiangxiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.isEmpty() && to_select->isKindOf("Weapon");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.isEmpty())
            return new QiangxiCard;
        else if(cards.length() == 1){
            QiangxiCard *card = new QiangxiCard;
            card->addSubcards(cards);

            return card;
        }else
            return NULL;
    }
};

class Tanwan: public TriggerSkill{
public:
    Tanwan():TriggerSkill("tanwan"){
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && !target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->isNude())
            return false;
        QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *p, players){
            if(p->isAlive() && room->askForSkillInvoke(p, objectName(), data)){

                p->obtainCard(player->getWeapon());
                p->obtainCard(player->getArmor());
                p->obtainCard(player->getDefensiveHorse());
                p->obtainCard(player->getOffensiveHorse());

                DummyCard *all_cards = player->wholeHandCards();
                if(all_cards){
                    CardMoveReason reason(CardMoveReason::S_REASON_RECYCLE, p->objectName());
                    room->obtainCard(p, all_cards, reason, false);
                    delete all_cards;
                }
                break;
            }
        }
        return false;
    }
};

BisuoCard::BisuoCard(){
    mute = true;
}

void BisuoCard::onEffect(const CardEffectStruct &effect) const{
    int x = effect.from->getLostHp();

    effect.to->drawCards(x);

    effect.to->turnOver();
}

class BisuoViewAsSkill: public ZeroCardViewAsSkill{
public:
    BisuoViewAsSkill():ZeroCardViewAsSkill("bisuo"){

    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@bisuo";
    }

    virtual const Card *viewAs() const{
        return new BisuoCard;
    }
};

class Bisuo: public MasochismSkill{
public:
    Bisuo():MasochismSkill("bisuo"){
        view_as_skill = new BisuoViewAsSkill;
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        room->askForUseCard(player, "@@bisuo", "@bisuo");
    }
};

class DummyViewAsSkill: public ViewAsSkill{
public:
    DummyViewAsSkill(): ViewAsSkill(""){
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *) const{
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &) const{
        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }
};

class Songwei: public TriggerSkill{
public:
    Songwei():TriggerSkill("songwei$"){
        events << FinishJudge;
        view_as_skill = new DummyViewAsSkill;
    }

    virtual int getPriority() const{
        return 2;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getKingdom() == "wei";
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        CardStar card = judge->card;

        if(card->isBlack()){
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if(p->hasLordSkill(objectName()))
                    targets << p;
            }
            
            while(!targets.isEmpty()){
                if(player->askForSkillInvoke(objectName())){
                    ServerPlayer *caopi = room->askForPlayerChosen(player, targets, objectName());
                    if(player->isMale())
                        room->broadcastSkillInvoke(objectName(), 1);
                    else
                        room->broadcastSkillInvoke(objectName(), 2);
                    caopi->drawCards(1);
                    caopi->setFlags("songweiused");      //for AI
                    targets.removeOne(caopi);
                }else
                    break;
            }
                    
            foreach(ServerPlayer *p, room->getAllPlayers()){        //for AI
                if(p->hasFlag("songweiused"))
                    p->setFlags("-songweiused");
            }
        }

        return false;
    }
};

class Shihua: public TriggerSkill{
public:
    Shihua():TriggerSkill("shihua"){
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual int getPriority() const{
        return 4;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
        if(move->from == player || move->from == NULL)
            return false;
        if(move->to_place == Player::DiscardPile
                && ((move->reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                    ||move->reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE)){
            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct shihuaMove;
            shihuaMove.to = player;
            shihuaMove.to_place = Player::PlaceHand;

            for(int i = 0; i < move->card_ids.size(); i++){
                int card_id = move->card_ids[i];
                if(Sanguosha->getCard(card_id)->getSuit() != Card::Club
                        || move->from_places[i] == Player::PlaceDelayedTrick
                        || move->from_places[i] == Player::PlaceSpecial
                        || room->getCardPlace(card_id) != Player::DiscardPile)
                    continue;

                shihuaMove.card_ids << card_id;
            }

            if(shihuaMove.card_ids.empty() || !player->askForSkillInvoke(objectName(), data))
                return false;

            if(shihuaMove.card_ids.size() > 1){
                while(!shihuaMove.card_ids.isEmpty()){
                    room->fillAG(shihuaMove.card_ids, player);
                    int card_id = room->askForAG(player, shihuaMove.card_ids, true, "shihua");
                    player->invoke("clearAG");
                    if(card_id == -1)
                        break;
                    shihuaMove.card_ids.removeOne(card_id);
                }
                if(shihuaMove.card_ids.empty())
                    return false;
            }

            exchangeMove.push_back(shihuaMove);
            // iwillback
            room->moveCardsAtomic(exchangeMove, true);
        }
        return false;
    }
};

class Jiushi: public ZeroCardViewAsSkill{
public:
    Jiushi():ZeroCardViewAsSkill("jiushi"){
        Analeptic *analeptic = new Analeptic(Card::NoSuitNoColor, 0);
        analeptic->setSkillName("jiushi");

        this->analeptic = analeptic;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player, NULL) && player->faceUp();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.contains("analeptic") && player->faceUp();
    }

    virtual const Card *viewAs() const{
        return analeptic;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return qrand() % 2 + 1;
    }

private:
    const Analeptic *analeptic;
};

class JiushiFlip: public TriggerSkill{
public:
    JiushiFlip():TriggerSkill("#jiushi-flip"){
        events << CardUsed << PreHpReduced << DamageComplete;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card->getSkillName() == "jiushi")
                player->turnOver();
        }else if(triggerEvent == PreHpReduced){
            player->tag["PredamagedFace"] = player->faceUp();
        }else if(triggerEvent == DamageComplete){
            bool faceup = player->tag.value("PredamagedFace", true).toBool();
            player->tag.remove("PredamagedFace");
            if(!faceup && !player->faceUp() && player->askForSkillInvoke("jiushi", data)){
                room->broadcastSkillInvoke("jiushi", 3);
                player->turnOver();
            }
        }

        return false;
    }
};

class Zhuyan: public TriggerSkill{
public:
    Zhuyan():TriggerSkill("zhuyan"){
        events << SlashEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target) && target->getArmor() == NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if(effect.slash->isBlack() && effect.nature == DamageStruct::Normal){

            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();

            player->getRoom()->sendLog(log);

            return true;
        }

        return false;
    }
};

class Piaonian: public TriggerSkill{
public:
    Piaonian():TriggerSkill("piaonian"){
        events << TargetConfirming;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *victim, QVariant &data) const{
        ServerPlayer *player = room->findPlayerBySkillName(objectName());

        if(player == NULL)
            return false;

        if(player == victim)
            return false;

        if(player->getPhase() != Player::NotActive)
            return false;

        CardUseStruct use = data.value<CardUseStruct>();

        if(use.card && use.card->isKindOf("Slash") && use.to.contains(victim) && player->inMyAttackRange(victim) && player->askForSkillInvoke(objectName())){
            if(!room->askForCard(player, "Armor", "@piaonian")){
                room->setPlayerProperty(victim, "chained", true);
                room->setPlayerProperty(player, "chained", true);
            }
            
            use.to.insert(use.to.indexOf(victim), player);
            use.to.removeOne(victim);

            data = QVariant::fromValue(use);

            return true;
        }

        return false;
    }
};

class Xuwu: public TriggerSkill{
public:
    Xuwu():TriggerSkill("xuwu")
    {
        frequency = Compulsory;
        events << Predamage;
    }

    virtual bool trigger(TriggerEvent ,  Room* room, ServerPlayer *player, QVariant &data) const
    {
        LogMessage log;
        DamageStruct damage = data.value<DamageStruct>();
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->loseHp(damage.to, damage.damage);
        return true;
    }
};

Nvelian::Nvelian(const QString &name, int n)
    :TriggerSkill(name), n(n)
{
}

QString Nvelian::getEffectName()const{
    return objectName();
}

bool Nvelian::trigger(TriggerEvent triggerEvent,  Room* room, ServerPlayer *player, QVariant &data) const
{
    if(triggerEvent == EventPhaseChanging && data.value<PhaseChangeStruct>().to != Player::Finish)
        return false;

    if(triggerEvent == EventAcquireSkill && data.toString() != getEffectName())
        return false;

    if(triggerEvent == HpRecover && player->getHp() <= 0)
        return false;

    if(triggerEvent == PostHpReduced || triggerEvent == HpLost){
        int delta = 0;
        if (triggerEvent == PostHpReduced)
            delta = data.value<DamageStruct>().damage;
        else
            delta = data.toInt();

        if (delta + player->getHp() <= 0)
            return false;
    }

    if(triggerEvent == CardsMoveOneTime)
    {
        if(player->getPhase() == Player::Discard)
            return false;
        CardsMoveOneTimeStar cards_move = data.value<CardsMoveOneTimeStar>();
        bool canInvoke = false;
        if(cards_move->from == player)
            canInvoke = cards_move->from_places.contains(Player::PlaceHand);
        else if(cards_move->to == player)
            canInvoke = cards_move->to_place == Player::PlaceHand;
        if(!canInvoke)
            return false;
    }

    const int count = qMin(qMin(player->getLostHp(), player->getMaxHp()), n) - player->getHandcardNum();

    if(count <= 0)
        return false;

    if(frequency == Compulsory || player->askForSkillInvoke(getEffectName())){
        if(frequency == Compulsory){
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = "nvelian";
            room->sendLog(log);
        }
        room->broadcastSkillInvoke("nvelian");
        player->drawCards(count);
    }

    return false;
}

class NvelianStateChanged: public Nvelian{
public:
    NvelianStateChanged():Nvelian("nvelian", 2)
    {
        frequency = Frequent;
        events << PostHpReduced << HpLost << HpRecover << MaxHpChanged << EventPhaseChanging << EventAcquireSkill;
    }

    virtual int getPriority() const{
        return -1;
    }
};

class NvelianCardMove: public Nvelian{
public:
    NvelianCardMove():Nvelian("#nvelian", 2)
    {
        frequency = Frequent;
        events << CardsMoveOneTime << CardDrawnDone;
    }

    virtual int getPriority() const{
        return 2;
    }

    virtual QString getEffectName() const{
        return "nvelian";
    }
};

class Lundao: public TriggerSkill{
public:
    Lundao():TriggerSkill("lundao"){
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();

        if(player->askForSkillInvoke(objectName(), data)){
            int card_id = room->drawCard();
            room->getThread()->delay();
            const Card *card = Sanguosha->getCard(card_id);

            room->retrial(card, player, judge, objectName());
        }
        return false;
    }
};

class Xuanwu: public PhaseChangeSkill{
public:
    Xuanwu():PhaseChangeSkill("xuanwu"){
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if(player->getPhase() == Player::Finish){
            if(!player->askForSkillInvoke(objectName()))
                return false;
            Room *room = player->getRoom();
            room->broadcastSkillInvoke(objectName(), 1);
            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(club|spade):(.*)");
            judge.good = true;
            judge.reason = objectName();
            judge.who = player;

            room->judge(judge);

            if(judge.isGood()){
                int x = player->getLostHp();
                player->drawCards(x + 1); //It should be preview, not draw
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName());

                QList<const Card *> miji_cards = player->getHandcards().mid(player->getHandcardNum() - x);
                foreach(const Card *card, miji_cards){
                    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName());
                    reason.m_playerId == target->objectName();
                    room->obtainCard(target, card, reason, false);
                }
            }
        }
        return false;
    }
};

class Xiaorui: public TriggerSkill{
public:
    Xiaorui():TriggerSkill("xiaorui"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());

        if(!splayer || player == splayer)
            return false;

        if(player->getPhase() == Player::Finish)
            if(splayer->askForSkillInvoke(objectName()))
                if(room->askForCard(splayer, "BasicCard", "@xiaoruiask"))
                    if(!room->askForCard(player, "EquipCard#TrickCard", "@xiaoruiresp")){
                        DamageStruct damage;
                        damage.from = splayer;
                        damage.to = player;
                        room->damage(damage);
                    }

        return false;
    }
};

RendeCard::RendeCard(){
    will_throw = false;
    handling_method = Card::MethodNone;
}

void RendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = NULL;
    if(targets.isEmpty()){
        foreach(ServerPlayer *player, room->getAlivePlayers()){
            if(player != source){
                target = player;
                break;
            }
        }
    }else
        target = targets.first();

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName());
    reason.m_playerId = target->objectName();
    room->obtainCard(target, this, reason, false);

    int old_value = source->getMark("rende");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "rende", new_value);

    if(old_value < 2 && new_value >= 2){
        RecoverStruct recover;
        recover.card = this;
        recover.who = source;
        room->recover(source, recover);
    }
}

/*class RendeViewAsSkill:public ViewAsSkill{
public:
    RendeViewAsSkill():ViewAsSkill("rende"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(ServerInfo.GameMode == "04_1v3"
           && selected.length() + Self->getMark("rende") >= 2)
           return false;
        else
            return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        RendeCard *rende_card = new RendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class Rende: public PhaseChangeSkill{
public:
    Rende():PhaseChangeSkill("rende"){
        view_as_skill = new RendeViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && PhaseChangeSkill::triggerable(target)
                && target->getPhase() == Player::NotActive
                && target->hasUsed("RendeCard");
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        target->getRoom()->setPlayerMark(target, "rende", 0);

        return false;
    }
};

JijiangCard::JijiangCard(){

}

bool JijiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash= new Slash(NoSuit, 0);
    return slash->targetFilter(targets, to_select, Self);
}

void JijiangCard::use(Room *room, ServerPlayer *liubei, QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> lieges = room->getLieges("shu", liubei);
    const Card *slash = NULL;

    QVariant tohelp = QVariant::fromValue((PlayerStar)liubei);
    foreach(ServerPlayer *liege, lieges){
        slash = room->askForCard(liege, "slash", "@jijiang-slash:" + liubei->objectName(), toslash, Card::MethodResponse, liubei);
        if(slash){
            CardUseStruct card_use;
            card_use.card = slash;
            card_use.from = liubei;
            card_use.to << targets.first();

            room->useCard(card_use);
            return;
        }
    }
    room->setPlayerFlag(liubei, "jijiang_failed");
}

class JijiangViewAsSkill:public ZeroCardViewAsSkill{
public:
    JijiangViewAsSkill():ZeroCardViewAsSkill("jijiang$"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->hasLordSkill("jijiang") && Slash::IsAvailable(player, NULL);
    }

    virtual const Card *viewAs() const{
        return new JijiangCard;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" && !ClientInstance->hasNoTargetResponding() && !player->hasFlag("jijiang_failed");
    }
};

class Jijiang: public TriggerSkill{
public:
    Jijiang():TriggerSkill("jijiang$"){
        events << CardAsked;
        default_choice = "ignore";

        view_as_skill = new JijiangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("jijiang");
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *liubei, QVariant &data) const{
        QString pattern = data.toStringList().first();
        if(pattern != "slash")
            return false;
                
        QList<ServerPlayer *> lieges = room->getLieges("shu", liubei);
        if(lieges.isEmpty())
            return false;

        if(!room->askForSkillInvoke(liubei, objectName()))
            return false;

        room->broadcastSkillInvoke(objectName(), getEffectIndex(liubei, NULL));

        QVariant tohelp = QVariant::fromValue((PlayerStar)liubei);
        foreach(ServerPlayer *liege, lieges){
            const Card *slash = room->askForCard(liege, "slash", "@jijiang-slash:" + liubei->objectName(), QVariant(), Card::MethodResponse, liubei);
            if(slash){
                room->provide(slash);
                return true;
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int r = 1 + qrand() % 2;
        if(player->getGeneralName() == "liushan" || player->getGeneral2Name() == "liushan")
            r += 2;

        return r;
    }
};

ZhihengCard::ZhihengCard(){
    target_fixed = true;
    once = true;
    mute = true;
    will_throw = false;
    handling_method = Card::MethodDiscard;
}

void ZhihengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->throwCard(this, source);
    if(!source->hasSkill("jilve"))
        room->broadcastSkillInvoke("zhiheng");
    if(source->isAlive())
        room->drawCards(source, subcards.length());
}

class Zhiheng:public ViewAsSkill{
public:
    Zhiheng():ViewAsSkill("zhiheng"){

    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *) const{
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        ZhihengCard *zhiheng_card = new ZhihengCard;
        zhiheng_card->addSubcards(cards);
        zhiheng_card->setSkillName(objectName());
        return zhiheng_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ZhihengCard");
    }
};

class Jiuyuan: public TriggerSkill{
public:
    Jiuyuan():TriggerSkill("jiuyuan$"){
        events << Dying << AskForPeachesDone << TargetConfirmed << HpRecover;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *sunquan, QVariant &data) const{
        switch(triggerEvent){
        case Dying: {
                foreach(ServerPlayer *wu, room->getOtherPlayers(sunquan)){
                    if(wu->getKingdom() == "wu"){
                        room->broadcastSkillInvoke("jiuyuan", 1);
                        break;
                    }
                }

                break;
            }

        case TargetConfirmed: {
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card->isKindOf("Peach") && use.from && use.from->getKingdom() == "wu"
                    && sunquan != use.from && sunquan->hasFlag("dying"))
            {
                room->setPlayerFlag(sunquan, "jiuyuan");
                room->setCardFlag(use.card, "jiuyuan");
            }
            break;
        }

        case HpRecover: {
            RecoverStruct rec = data.value<RecoverStruct>();
            if(rec.card && rec.card->hasFlag("jiuyuan"))
            {
                int index = rec.who->isMale() ? 2 : 3;

                room->broadcastSkillInvoke("jiuyuan", index);

                LogMessage log;
                log.type = "#JiuyuanExtraRecover";
                log.from = sunquan;
                log.to << rec.who;
                log.arg = objectName();
                room->sendLog(log);

                rec.recover ++;

                data = QVariant::fromValue(rec);
            }
            break;
        }

        case AskForPeachesDone:{
                if(sunquan->hasFlag("jiuyuan")){
                    room->setPlayerFlag(sunquan, "-jiuyuan");
                    if(sunquan->getHp() > 0){
                        room->getThread()->delay(2000);
                        room->broadcastSkillInvoke("jiuyuan", 4);
                    }
                }

                break;
            }

        default:
            break;
        }

        return false;
    }
};

class Keji: public TriggerSkill{
public:
    Keji():TriggerSkill("keji"){
        events << EventPhaseChanging << CardResponded;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &data) const{
        if(triggerEvent == CardResponded && lvmeng->getPhase() == Player::Play){
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if(card_star->isKindOf("Slash"))
                lvmeng->setFlags("keji_use_slash");
        }
        else if(triggerEvent == EventPhaseChanging)
        {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if(change.to == Player::Discard){
                if(!lvmeng->hasFlag("keji_use_slash") &&
                        lvmeng->getSlashCount() == 0 &&
                        lvmeng->askForSkillInvoke("keji"))
                {
                    if (lvmeng->getHandcardNum() > lvmeng->getMaxCards()) {
                        int index = qrand() % 2 + 1;
                        if (!lvmeng->hasInnateSkill(objectName()) && lvmeng->hasSkill("mouduan"))
                            index += 2;
                        room->broadcastSkillInvoke(objectName(), index);
                    }

                    lvmeng->skip(Player::Discard);
                }
            }
        }
        return false;
    }
};

KurouCard::KurouCard(){
    target_fixed = true;
}

void KurouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->loseHp(source);
    if(source->isAlive())
        room->drawCards(source, 2);
}

class Kurou: public ZeroCardViewAsSkill{
public:
    Kurou():ZeroCardViewAsSkill("kurou"){

    }

    virtual const Card *viewAs() const{
        return new KurouCard;
    }
};*/

void StandardPackage::addBloomGenerals(){
    /*General *caocao, *zhangliao, *guojia, *xiahoudun, *simayi, *xuchu, *zhenji;

    caocao = new General(this, "caocao$", "wei");
    caocao->addSkill(new Jianxiong);
    caocao->addSkill(new Hujia);*/

    General *bloom002 = new General(this, "bloom002", "wei", 3);
    bloom002->addSkill(new Tiansuo);
    bloom002->addSkill(new Huanji);

    /*xiahoudun = new General(this, "xiahoudun", "wei");
    xiahoudun->addSkill(new Ganglie);

    zhangliao = new General(this, "zhangliao", "wei");
    zhangliao->addSkill(new Tuxi);*/

    General *bloom005 = new General(this, "bloom005", "wei");
    bloom005->addSkill(new Luoyi);
    bloom005->addSkill(new LuoyiBuff);
    related_skills.insertMulti("luoyi", "#luoyi");

    General *bloom006 = new General(this, "bloom006", "wei", 3);
    bloom006->addSkill(new Tiandu);
    bloom006->addSkill(new Yumeng);

    General *bloom007 = new General(this, "bloom007", "wei", 3, false);
    bloom007->addSkill(new Mengyang);
    bloom007->addSkill(new Zhongyan);

    General *bloom008 = new General(this, "bloom008", "wei");
    bloom008->addSkill(new Xunyu);

    General *bloom009 = new General(this, "bloom009", "wei");
    bloom009->addSkill(new Mancai);

    General *bloom010 = new General(this, "bloom010", "wei");
    bloom010->addSkill(new Fenghou);

    General *bloom011 = new General(this, "bloom011", "wei");
    bloom011->addSkill(new Zhaihun);
    bloom011->addSkill(new Fojiao);

    General *bloom012 = new General(this, "bloom012", "wei");
    bloom012->addSkill(new Qiangxi);

    General *bloom014 = new General(this, "bloom014$", "wei", 3);
    bloom014->addSkill(new Tanwan);
    bloom014->addSkill(new Bisuo);
    bloom014->addSkill(new Songwei);

    General *bloom016 = new General(this, "bloom016", "wei", 3);
    bloom016->addSkill(new Shihua);
    bloom016->addSkill(new Jiushi);
    bloom016->addSkill(new JiushiFlip);
    related_skills.insertMulti("jiushi", "#jiushi-flip");

    General *bloom017 = new General(this, "bloom017", "wei");
    bloom017->addSkill(new Zhuyan);
    bloom017->addSkill(new Piaonian);

    General *bloom018 = new General(this, "bloom018", "wei", 3, false);
    bloom018->addSkill(new Xuwu);
    bloom018->addSkill(new NvelianStateChanged);
    bloom018->addSkill(new NvelianCardMove);
    related_skills.insertMulti("nvelian", "#nvelian");

    General *bloom022 = new General(this, "bloom022", "wei", 3, false);
    bloom022->addSkill(new Lundao);
    bloom022->addSkill(new Xuanwu);

    General *bloom023 = new General(this, "bloom023", "wei");
    bloom023->addSkill(new Xiaorui);

    /*General *liubei, *guanyu, *zhangfei, *zhaoyun, *machao, *zhugeliang, *huangyueying;
    liubei = new General(this, "liubei$", "shu");
    liubei->addSkill(new Rende);
    liubei->addSkill(new Jijiang);

    General *sunquan, *zhouyu, *lvmeng, *luxun, *ganning, *huanggai, *daqiao, *sunshangxiang;
    sunquan = new General(this, "sunquan$", "wu");
    sunquan->addSkill(new Zhiheng);
    sunquan->addSkill(new Jiuyuan);

    lvmeng = new General(this, "lvmeng", "wu");
    lvmeng->addSkill(new Keji);

    huanggai = new General(this, "huanggai", "wu");
    huanggai->addSkill(new Kurou);

    //new General(this, "anjiang", "god", 4,true, true, true);*/

    // for skill cards
    //addMetaObject<ZhihengCard>();
    addMetaObject<LuoyiCard>();
    addMetaObject<TuxiCard>();
    //addMetaObject<JieyinCard>();
    //addMetaObject<KurouCard>();
    //addMetaObject<LijianCard>();
    addMetaObject<TiansuoCard>();
    //addMetaObject<LiuliCard>();
    //addMetaObject<JijiangCard>();
    addMetaObject<XunyuCard>();
    addMetaObject<MancaiCard>();
    addMetaObject<QiangxiCard>();
    addMetaObject<BisuoCard>();

    addMetaObject<RendeCard>();
}
