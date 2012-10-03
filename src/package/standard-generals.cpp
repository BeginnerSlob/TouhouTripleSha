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
        QString pattern = data.toString();
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
            const Card *jink = room->askForCard(liege, "jink", "@hujia-jink:" + caocao->objectName(), tohelp, CardResponsed, caocao);
            if(jink){
                room->provide(jink);
                return true;
            }
        }

        return false;
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

class Tiandu:public TriggerSkill{
public:
    Tiandu():TriggerSkill("tiandu"){
        frequency = Frequent;
        default_choice = "no";
        events << FinishJudge;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *guojia, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        CardStar card = judge->card;

        QVariant data_card = QVariant::fromValue(card);
        if(guojia->askForSkillInvoke(objectName(), data_card)){
            if(card->objectName() == "shit"){
                QString result = room->askForChoice(guojia, objectName(), "yes+no");
                if(result == "no")
                    return false;
            }

            guojia->obtainCard(judge->card);
            room->broadcastSkillInvoke(objectName());
            room->getThread()->delay(500);

            return true;
        }

        return false;
    }
};

class Yiji:public MasochismSkill{
public:
    Yiji():MasochismSkill("yiji"){
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *guojia, const DamageStruct &damage) const{
        Room *room = guojia->getRoom();

        if(!room->askForSkillInvoke(guojia, objectName()))
            return;

        room->broadcastSkillInvoke(objectName());

        int x = damage.damage, i;
        for(i = 0; i < x; i++)
        {
            QList<int> yiji_cards;
            yiji_cards.append(room->drawCard());
            yiji_cards.append(room->drawCard());
            CardsMoveStruct move;
            move.card_ids = yiji_cards;
            move.to = guojia;
            move.to_place = Player::PlaceHand;
            move.reason = CardMoveReason(CardMoveReason::S_REASON_SHOW, guojia->objectName(), "yiji", QString());
            room->moveCards(move, false);

            if(yiji_cards.isEmpty())
                continue;

            while(room->askForYiji(guojia, yiji_cards));
        }

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

TiansuoCard::TiansuoCard(){
    target_fixed = true;
    will_throw = false;
    can_jilei = true;
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
        const Card *card = room->askForCard(player, "@tiansuo", prompt, data, AskForRetrial);
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
            log.arg2 = QString::number(damage.damage + 1);
            xuchu->getRoom()->sendLog(log);

            damage.damage ++;
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

class Luoshen:public TriggerSkill{
public:
    Luoshen():TriggerSkill("luoshen"){
        events << EventPhaseStart << FinishJudge;

        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *zhenji, QVariant &data) const{
        if(triggerEvent == EventPhaseStart && zhenji->getPhase() == Player::Start){
            int n = 0;
            while(zhenji->askForSkillInvoke("luoshen")){
                if(n == 0)
                    room->broadcastSkillInvoke(objectName());
                n++;

                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(spade|club):(.*)");
                judge.good = true;
                judge.reason = objectName();
                judge.who = zhenji;
                judge.time_consuming = true;

                room->judge(judge);
                if(judge.isBad())
                    break;
            }

        }else if(triggerEvent == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if(judge->reason == objectName()){
                if(judge->card->isBlack()){
                    zhenji->obtainCard(judge->card);
                    return true;
                }
            }
        }

        return false;
    }
};

class Qingguo:public OneCardViewAsSkill{
public:
    Qingguo():OneCardViewAsSkill("qingguo"){

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

RendeCard::RendeCard(){
    will_throw = false;
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

class RendeViewAsSkill:public ViewAsSkill{
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
        slash = room->askForCard(liege, "slash", "@jijiang-slash:" + liubei->objectName(), tohelp, CardResponsed, liubei);
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
        return player->hasLordSkill("jijiang") && Slash::IsAvailable(player);
    }

    virtual const Card *viewAs() const{
        return new JijiangCard;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" && !ClientInstance->hasNoTargetResponsing() && !player->hasFlag("jijiang_failed");
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
        QString pattern = data.toString();
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
            const Card *slash = room->askForCard(liege, "slash", "@jijiang-slash:" + liubei->objectName(), tohelp, CardResponsed, liubei);
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

class Wusheng:public OneCardViewAsSkill{
public:
    Wusheng():OneCardViewAsSkill("wusheng"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "slash";
    }

    virtual bool viewFilter(const Card* card) const{
        if(!card->isRed())
            return false;

        if(Self->getWeapon() && card->getEffectiveId() == Self->getWeapon()->getId() && card->objectName() == "Crossbow")
            return Self->canSlashWithoutCrossbow();
        else
            return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        
        Card *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard->getId());
        slash->setSkillName(objectName());
        return slash;
    }
};

class Longdan:public OneCardViewAsSkill{
public:
    Longdan():OneCardViewAsSkill("longdan"){

    }

    virtual bool viewFilter(const Card* to_select) const{
        const Card *card = to_select;

        switch(Sanguosha->currentRoomState()->getCurrentCardUseReason()){
        case CardUseStruct::CARD_USE_REASON_PLAY:{
                // jink as slash
                return card->isKindOf("Jink");
            }

        case CardUseStruct::CARD_USE_REASON_RESPONSE:{
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                if(pattern == "slash")
                    return card->isKindOf("Jink");
                else if(pattern == "jink")
                    return card->isKindOf("Slash");
            }

        default:
            return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if(originalCard->isKindOf("Slash")){
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        }else if(originalCard->isKindOf("Jink")){
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        }else
            return NULL;
    }
};

class Tieji:public TriggerSkill{
public:
    Tieji():TriggerSkill("tieji"){
        events << TargetConfirmed << SlashProceed << CardFinished;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{

        if(triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if(!use.card->isKindOf("Slash") || use.from != player)
                   return false;

            foreach(ServerPlayer *target, use.to){
                bool invoke = player->askForSkillInvoke("tieji", QVariant::fromValue(target));
                JudgeStruct judge;
                if(invoke){
                    room->broadcastSkillInvoke(objectName());

                    judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
                    judge.good = true;
                    judge.reason = objectName();
                    judge.who = player;
                    judge.play_animation = true;

                    room->judge(judge);
                }
                QVariantList tiejiList = target->tag["Tieji"].toList();
                tiejiList << (invoke && judge.isGood());
                target->tag["Tieji"] = tiejiList;
                target->setFlags("TiejiTarget");
            }
        }
        else if(triggerEvent == SlashProceed){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.to->setFlags("-TiejiTarget");
            QVariantList tiejiList = effect.to->tag["Tieji"].toList();
            if(!tiejiList.isEmpty()){
                bool hit = tiejiList.takeFirst().toBool();
                effect.to->tag["Tieji"] = tiejiList;
                if(hit){
                    room->slashResult(effect, NULL);
                    return true;
                }
            }
        }
        else if(triggerEvent == CardFinished){
            CardUseStruct use = data.value<CardUseStruct>();
            foreach(ServerPlayer *target, use.to){
                if(target->hasFlag("TiejiTarget"))
                    target->tag.remove("Tieji");
            }
        }
        return false;
    }
};

class Guanxing:public PhaseChangeSkill{
public:
    Guanxing():PhaseChangeSkill("guanxing"){
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *zhuge) const{
        if(zhuge->getPhase() == Player::Start &&
           zhuge->askForSkillInvoke(objectName()))
        {
            Room *room = zhuge->getRoom();
            int index = qrand() % 2 + 1;
            if (!zhuge->hasInnateSkill(objectName()) && zhuge->hasSkill("zhiji"))
                index += 2;
            room->broadcastSkillInvoke(objectName(), index);

            int n = qMin(5, room->alivePlayerCount());
            room->askForGuanxing(zhuge, room->getNCards(n, false), false);
        }

        return false;
    }
};

class Kongcheng: public ProhibitSkill{
public:
    Kongcheng():ProhibitSkill("kongcheng"){

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        if(card->isKindOf("Slash") || card->isKindOf("Duel"))
            return to->isKongcheng();
        else
            return false;
    }
};

class KongchengEffect: public TriggerSkill{
public:
    KongchengEffect():TriggerSkill("#kongcheng-effect"){
        frequency = Compulsory;

        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->isKongcheng()){
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            if(move->from == player && move->from_places.contains(Player::PlaceHand))
                room->broadcastSkillInvoke("kongcheng");
        }

        return false;
    }
};

class Jizhi:public TriggerSkill{
public:
    Jizhi():TriggerSkill("jizhi"){
        frequency = Frequent;
        events << CardUsed << CardResponsed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *yueying, QVariant &data) const{
        if (yueying == NULL) return false;
        CardStar card = NULL;
        if(triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }else if(triggerEvent == CardResponsed)
            card = data.value<ResponsedStruct>().m_card;

        if(card->isNDTrick()){            
            if(room->askForSkillInvoke(yueying, objectName())){
                room->broadcastSkillInvoke(objectName());
                yueying->drawCards(1);
            }
        }

        return false;
    }
};

ZhihengCard::ZhihengCard(){
    target_fixed = true;
    once = true;
    mute = true;
    will_throw = false;
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

class Yingzi:public DrawCardsSkill{
public:
    Yingzi():DrawCardsSkill("yingzi"){
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *zhouyu, int n) const{
        Room *room = zhouyu->getRoom();
        if(room->askForSkillInvoke(zhouyu, objectName())){
            int index = qrand() % 2 + 1;
            if (!zhouyu->hasInnateSkill(objectName())) {
                if (zhouyu->hasSkill("mouduan"))
                    index += 2;
                else if (zhouyu->hasSkill("hunzi"))
                    index += 4;
            }

            room->broadcastSkillInvoke(objectName(), index);
            return n + 1;
        }else
            return n;
    }
};

FanjianCard::FanjianCard(){
    once = true;
}

void FanjianCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    int card_id = zhouyu->getRandomHandCardId();
    const Card *card = Sanguosha->getCard(card_id);
    Card::Suit suit = room->askForSuit(target, "fanjian");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = target;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->getThread()->delay();
    target->obtainCard(card);

    if(card->getSuit() != suit){
        DamageStruct damage;
        damage.card = NULL;
        damage.from = zhouyu;
        damage.to = target;

        room->damage(damage);
    }
}

class Fanjian:public ZeroCardViewAsSkill{
public:
    Fanjian():ZeroCardViewAsSkill("fanjian"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && ! player->hasUsed("FanjianCard");
    }

    virtual const Card *viewAs() const{
        return new FanjianCard;
    }
};

class Keji: public TriggerSkill{
public:
    Keji():TriggerSkill("keji"){
        events << EventPhaseChanging << CardResponsed;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &data) const{
        if(triggerEvent == CardResponsed && lvmeng->getPhase() == Player::Play){
            CardStar card_star = data.value<ResponsedStruct>().m_card;
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

class Lianying: public TriggerSkill{
public:
    Lianying():TriggerSkill("lianying"){
        events << CardsMoveOneTime;

        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *luxun, QVariant &data) const{
        CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
        if(move->from == luxun && move->from_places.contains(Player::PlaceHand) && luxun->isKongcheng()){
            if(room->askForSkillInvoke(luxun, objectName(), data)){
                room->broadcastSkillInvoke(objectName());
                luxun->drawCards(1);
            }
        }
        return false;
    }
};

class Qixi: public OneCardViewAsSkill{
public:
    Qixi():OneCardViewAsSkill("qixi"){

    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isBlack();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Dismantlement *dismantlement = new Dismantlement(originalCard->getSuit(), originalCard->getNumber());
        dismantlement->addSubcard(originalCard->getId());
        dismantlement->setSkillName(objectName());
        return dismantlement;
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
};

class Guose: public OneCardViewAsSkill{
public:
    Guose():OneCardViewAsSkill("guose"){

    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->getSuit() == Card::Diamond;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Indulgence *indulgence = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
        indulgence->addSubcard(originalCard->getId());
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

LiuliCard::LiuliCard()
{
}


bool LiuliCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select->hasFlag("slash_source"))
        return false;

    CardStar slash = Self->tag["liuli-card"].value<CardStar>();
    if(!Self->canSlash(to_select, slash))
        return false;

    int card_id = subcards.first();
    if(Self->getWeapon() && Self->getWeapon()->getId() == card_id)
        return Self->distanceTo(to_select) <= 1;
    else if(Self->getOffensiveHorse() && Self->getOffensiveHorse()->getId() == card_id){
        int distance = 1;
        if(Self->getWeapon()){
            const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
            distance = weapon->getRange();
        }
        return Self->distanceTo(to_select, 1) <= distance;
    }
    else
        return true;
}

void LiuliCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    room->setPlayerFlag(effect.to, "liuli_target");
}

class LiuliViewAsSkill: public OneCardViewAsSkill{
public:
    LiuliViewAsSkill():OneCardViewAsSkill("liuli"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@liuli";
    }

    virtual bool viewFilter(const Card *) const{
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LiuliCard *liuli_card = new LiuliCard;
        liuli_card->addSubcard(originalCard);

        return liuli_card;
    }
};

class Liuli: public TriggerSkill{
public:
    Liuli():TriggerSkill("liuli"){
        view_as_skill = new LiuliViewAsSkill;

        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *daqiao, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if(use.card && use.card->isKindOf("Slash") && use.to.contains(daqiao) && !daqiao->isNude() && room->alivePlayerCount() > 2){
            QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
            players.removeOne(use.from);

            bool can_invoke = false;
            foreach(ServerPlayer *p, players){
                if(daqiao->canSlash(p, use.card)){
                    can_invoke = true;
                    break;
                }
            }

            if(can_invoke){
                QString prompt = "@liuli:" + use.from->objectName();
                room->setPlayerFlag(use.from, "slash_source");
                daqiao->tag["liuli-card"] = QVariant::fromValue((CardStar)use.card);
                if(room->askForUseCard(daqiao, "@@liuli", prompt)){
                    daqiao->tag.remove("liuli-card");
                    foreach(ServerPlayer *p, players){
                        if(p->hasFlag("liuli_target")){
                            use.to.insert(use.to.indexOf(daqiao), p);
                            use.to.removeOne(daqiao);

                            data = QVariant::fromValue(use);

                            room->setPlayerFlag(use.from, "-slash_source");
                            room->setPlayerFlag(p, "-liuli_target");
                            return true;
                        }
                    }
                }
				daqiao->tag.remove("liuli-card");
            }
        }

        return false;
    }
};

JieyinCard::JieyinCard(){
    once = true;
    mute = true;
}

bool JieyinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    return to_select->isMale() && to_select->isWounded();
}

void JieyinCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;

    room->recover(effect.from, recover, true);
    room->recover(effect.to, recover, true);

    int index = -1;
    if(effect.from->isMale()){
        if(effect.from == effect.to)
            index = 5;
        else if(effect.from->getHp() >= effect.to->getHp())
            index = 3;
        else
            index = 4;
    }else{
        index = 1 + qrand() % 2;
    }

    room->broadcastSkillInvoke("jieyin", index);
}

class Jieyin: public ViewAsSkill{
public:
    Jieyin():ViewAsSkill("jieyin"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("JieyinCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(selected.length() > 1)
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.length() != 2)
            return NULL;

        JieyinCard *jieyin_card = new JieyinCard();
        jieyin_card->addSubcards(cards);

        return jieyin_card;
    }
};

class Xiaoji: public TriggerSkill{
public:
    Xiaoji():TriggerSkill("xiaoji"){
        events << CardsMoveOneTime;

        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *sunshangxiang, QVariant &data) const{
        CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
        if(move->from == sunshangxiang && move->from_places.contains(Player::PlaceEquip)){
            int n = 0;
            foreach(Player::Place place, move->from_places)
                if(place == Player::PlaceEquip)
                    n ++;

            for(int i = 0; i < n; i++)
            if(room->askForSkillInvoke(sunshangxiang, objectName())){
                if(i == 0)
                    room->broadcastSkillInvoke(objectName());
                sunshangxiang->drawCards(2);
            }
        }

        return false;
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

LijianCard::LijianCard(){
    once = true;
}

bool LijianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!to_select->isMale())
        return false;

    Duel *duel = new Duel(Card::NoSuit, 0);
    if(targets.isEmpty() && Self->isProhibited(to_select, duel)){
        return false;
    }

    return targets.length() < 2;
}

bool LijianCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void LijianCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *diaochan = card_use.from;

    LogMessage log;
    log.from = diaochan;
    log.to << card_use.to[1];
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();

    thread->trigger(CardUsed, room, diaochan, data);

    thread->trigger(CardFinished, room, diaochan, data);

}

void LijianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->broadcastSkillInvoke("lijian");
    room->throwCard(this, source);

    ServerPlayer *to = targets.at(0);
    ServerPlayer *from = targets.at(1);

    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->setCancelable(false);
    duel->setSkillName("lijian");

    CardUseStruct use;
    use.from = from;
    use.to << to;
    use.card = duel;
    room->useCard(use);
}

class Lijian: public OneCardViewAsSkill{
public:
    Lijian():OneCardViewAsSkill("lijian"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("LijianCard");
    }

    virtual bool viewFilter(const Card *) const{
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LijianCard *lijian_card = new LijianCard;
        lijian_card->addSubcard(originalCard->getId());

        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 0;
    }
};

class Biyue: public PhaseChangeSkill{
public:
    Biyue():PhaseChangeSkill("biyue"){
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *diaochan) const{
        if(diaochan->getPhase() == Player::Finish){
            Room *room = diaochan->getRoom();
            if(room->askForSkillInvoke(diaochan, objectName())){
                room->broadcastSkillInvoke(objectName());
                diaochan->drawCards(1);
            }
        }

        return false;
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

class Jijiu: public OneCardViewAsSkill{
public:
    Jijiu():OneCardViewAsSkill("jijiu"){

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

class Qianxun: public ProhibitSkill{
public:
    Qianxun():ProhibitSkill("qianxun"){

    }

    virtual bool isProhibited(const Player *, const Player *, const Card *card) const{
        return card->isKindOf("Snatch") || card->isKindOf("Indulgence");
    }
};

class Mashu: public DistanceSkill{
public:
    Mashu():DistanceSkill("mashu")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(from->hasSkill(objectName()))
            return -1;
        else
            return 0;
    }
};

void StandardPackage::addGenerals(){
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
    zhangliao->addSkill(new Tuxi);

    xuchu = new General(this, "xuchu", "wei");
    xuchu->addSkill(new Luoyi);
    xuchu->addSkill(new LuoyiBuff);
    related_skills.insertMulti("luoyi", "#luoyi");

    guojia = new General(this, "guojia", "wei", 3);
    guojia->addSkill(new Tiandu);
    guojia->addSkill(new Yiji);

    zhenji = new General(this, "zhenji", "wei", 3, false);
    zhenji->addSkill(new Luoshen);
    zhenji->addSkill(new Qingguo);

    General *liubei, *guanyu, *zhangfei, *zhaoyun, *machao, *zhugeliang, *huangyueying;
    liubei = new General(this, "liubei$", "shu");
    liubei->addSkill(new Rende);
    liubei->addSkill(new Jijiang);

    guanyu = new General(this, "guanyu", "shu");
    guanyu->addSkill(new Wusheng);

    zhangfei = new General(this, "zhangfei", "shu");
    zhangfei->addSkill(new Skill("paoxiao", Skill::Compulsory));

    zhugeliang = new General(this, "zhugeliang", "shu", 3);
    zhugeliang->addSkill(new Guanxing);
    zhugeliang->addSkill(new Kongcheng);
    zhugeliang->addSkill(new KongchengEffect);
    related_skills.insertMulti("kongcheng", "#kongcheng-effect");

    zhaoyun = new General(this, "zhaoyun", "shu");
    zhaoyun->addSkill(new Longdan);

    machao = new General(this, "machao", "shu");
    machao->addSkill(new Tieji);
    machao->addSkill(new Mashu);
    machao->addSkill(new SPConvertSkill("fanqun", "machao", "sp_machao"));

    huangyueying = new General(this, "huangyueying", "shu", 3, false);
    huangyueying->addSkill(new Jizhi);
    huangyueying->addSkill(new Skill("qicai", Skill::Compulsory));

    General *sunquan, *zhouyu, *lvmeng, *luxun, *ganning, *huanggai, *daqiao, *sunshangxiang;
    sunquan = new General(this, "sunquan$", "wu");
    sunquan->addSkill(new Zhiheng);
    sunquan->addSkill(new Jiuyuan);

    ganning = new General(this, "ganning", "wu");
    ganning->addSkill(new Qixi);

    lvmeng = new General(this, "lvmeng", "wu");
    lvmeng->addSkill(new Keji);

    huanggai = new General(this, "huanggai", "wu");
    huanggai->addSkill(new Kurou);

    zhouyu = new General(this, "zhouyu", "wu", 3);
    zhouyu->addSkill(new Yingzi);
    zhouyu->addSkill(new Fanjian);

    daqiao = new General(this, "daqiao", "wu", 3, false);
    daqiao->addSkill(new Guose);
    daqiao->addSkill(new Liuli);

    luxun = new General(this, "luxun", "wu", 3);
    luxun->addSkill(new Qianxun);
    luxun->addSkill(new Lianying);

    sunshangxiang = new General(this, "sunshangxiang", "wu", 3, false);
    sunshangxiang->addSkill(new SPConvertSkill("chujia", "sunshangxiang", "sp_sunshangxiang"));
    sunshangxiang->addSkill(new Jieyin);
    sunshangxiang->addSkill(new Xiaoji);

    General *lvbu, *huatuo, *diaochan;

    huatuo = new General(this, "huatuo", "qun", 3);
    huatuo->addSkill(new Qingnang);
    huatuo->addSkill(new Jijiu);

    lvbu = new General(this, "lvbu", "qun");
    lvbu->addSkill(new Wushuang);

    diaochan = new General(this, "diaochan", "qun", 3, false);
    diaochan->addSkill(new Lijian);
    diaochan->addSkill(new Biyue);
    diaochan->addSkill(new SPConvertSkill("tuoqiao", "diaochan", "sp_diaochan"));*/

    //new General(this, "anjiang", "god", 4,true, true, true);

    // for skill cards
    addMetaObject<ZhihengCard>();
    addMetaObject<LuoyiCard>();
    addMetaObject<RendeCard>();
    addMetaObject<TuxiCard>();
    addMetaObject<JieyinCard>();
    addMetaObject<KurouCard>();
    addMetaObject<LijianCard>();
    addMetaObject<FanjianCard>();
    addMetaObject<TiansuoCard>();
    addMetaObject<QingnangCard>();
    addMetaObject<LiuliCard>();
    addMetaObject<JijiangCard>();
}
