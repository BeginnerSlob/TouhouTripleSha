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

FunuanCard::FunuanCard(){
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool FunuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    if (!targets.isEmpty())
        return false;
    if (to_select == Self)
        return false;

    if (Self->getMark("funuantarget") >= 2)
        return to_select->getMark("funuantarget") > 0;
    else
        return true;
}

void FunuanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = NULL;
    if(targets.isEmpty())
        foreach(ServerPlayer *player, room->getOtherPlayers(source))
        {
            target = player;
            break;
        }
    else
        target = targets.first();

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName());
    reason.m_playerId = target->objectName();
    room->obtainCard(target, this, reason, false);

    if (target->getMark("funuantarget") <= 0)
    {
        room->setPlayerMark(target, "funuantarget", 1);
        room->setPlayerMark(source, "funuantarget", source->getMark("funuantarget") + 1);
    }
    int old_value = source->getMark("funuan");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "funuan", new_value);

    if(old_value < 2 && new_value >= 2){
        RecoverStruct recover;
        recover.who = source;
        room->recover(source, recover);
    }
}

class FunuanViewAsSkill:public ViewAsSkill{
public:
    FunuanViewAsSkill():ViewAsSkill("funuan"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(selected.length() + Self->getMark("funuan") >= 3)
            return false;
        else
            return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        FunuanCard *funuan_card = new FunuanCard;
        funuan_card->addSubcards(cards);
        return funuan_card;
    }
};

class Funuan: public TriggerSkill{
public:
    Funuan():TriggerSkill("funuan"){
        events << EventPhaseStart;
        view_as_skill = new FunuanViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
               && target->getPhase() == Player::NotActive
               && target->hasUsed("FunuanCard");
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *target, QVariant &data) const{
        if (data.value<PlayerStar>() != target)
            return false;
        room->setPlayerMark(target, "funuan", 0);
        foreach (ServerPlayer *p, room->getAlivePlayers())
            room->setPlayerMark(p, "funuantarget", 0);
        return false;
    }
};

LiqiCard::LiqiCard(){

}

bool LiqiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash= new Slash(NoSuitNoColor, 0);
    return slash->targetFilter(targets, to_select, Self);
}

void LiqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> lieges = room->getLieges("shu", source);
    const Card *slash = NULL;

    foreach(ServerPlayer *liege, lieges){
        slash = room->askForCard(liege, "slash", "@liqi-slash:" + source->objectName(), QVariant(), Card::MethodResponse, source);
        if(slash){
            CardUseStruct card_use;
            card_use.card = slash;
            card_use.from = source;
            card_use.to << targets;

            room->useCard(card_use);
            return;
        }
    }
    room->setPlayerFlag(source, "liqi_failed");
}

LiqiViewAsSkill::LiqiViewAsSkill(): ZeroCardViewAsSkill("liqi$") {
}

bool LiqiViewAsSkill::isEnabledAtPlay(const Player *player) const{
    return hasShuGenerals(player) && player->hasLordSkill("liqi") && !player->hasFlag("liqi_failed")
           && Slash::IsAvailable(player);
}

bool LiqiViewAsSkill::isEnabledAtResponse(const Player *player, const QString &pattern) const{
    return hasShuGenerals(player)
           && pattern == "slash" && !ClientInstance->hasNoTargetResponding()
           && !player->hasFlag("liqi_failed");
}

const Card *LiqiViewAsSkill::viewAs() const{
    return new LiqiCard;
}

bool LiqiViewAsSkill::hasShuGenerals(const Player *player) {
    foreach (const Player *p, player->getSiblings())
        if (p->getKingdom() == "shu")
            return true;
    return false;
}

class Liqi: public TriggerSkill{
public:
    Liqi():TriggerSkill("liqi$"){
        events << CardAsked;
        default_choice = "ignore";

        view_as_skill = new LiqiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("liqi");
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

        room->broadcastSkillInvoke(objectName());

        QVariant tohelp = QVariant::fromValue((PlayerStar)liubei);
        foreach(ServerPlayer *liege, lieges){
            const Card *slash = room->askForCard(liege, "slash", "@liqi-slash:" + liubei->objectName(), QVariant(), Card::MethodResponse, liubei);
            if(slash){
                room->provide(slash);
                return true;
            }
        }

        return false;
    }
};

class Chilian:public OneCardViewAsSkill{
public:
    Chilian():OneCardViewAsSkill("chilian"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player, NULL);
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

class Zhenhong: public TriggerSkill{
public:
    Zhenhong():TriggerSkill("zhenhong"){
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.from == player && use.card->isKindOf("Slash") && use.card->getSuit() == Card::Diamond) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            foreach(ServerPlayer *p, use.to)
                p->addMark("Qinggang_Armor_Nullified");
        }

        return false;
    }
};

class ZhenhongTargetMod: public TargetModSkill {
public:
    ZhenhongTargetMod(): TargetModSkill("#zhenhong-target") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("zhenhong") && card->getSuit() == Card::Heart)
            return 1000;
        else
            return 0;
    }
};

class Hupao: public TargetModSkill {
public:
    Hupao(): TargetModSkill("hupao") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class Jiuli:public OneCardViewAsSkill{
public:
    Jiuli():OneCardViewAsSkill("jiuli"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player, NULL);
    }

    virtual bool viewFilter(const Card* card) const{
        return (card->isKindOf("Weapon") ||(card->isBlack() && card->isNDTrick()));
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *jiu = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        jiu->addSubcard(originalCard->getId());
        jiu->setSkillName(objectName());
        return jiu;
    }
};

class Yuxi:public TriggerSkill {
public:
    Yuxi(): TriggerSkill("yuxi") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
        if(data.value<PlayerStar>() == player && player->getPhase() == Player::Start &&
           player->askForSkillInvoke(objectName()))
        {
            room->broadcastSkillInvoke(objectName());

            int n = qMin(5, room->alivePlayerCount());
            room->askForYuxi(player, room->getNCards(n, false), false);
        }

        return false;
    }
};

class Jingyou: public ProhibitSkill{
public:
    Jingyou():ProhibitSkill("jingyou"){

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        if(card->isKindOf("Slash") || card->isKindOf("Duel"))
            return to->hasSkill(objectName()) && to->isKongcheng();
        else
            return false;
    }
};

class JingyouEffect: public TriggerSkill{
public:
    JingyouEffect():TriggerSkill("#jingyou-effect"){
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

class Huahuan:public OneCardViewAsSkill{
public:
    Huahuan():OneCardViewAsSkill("huahuan"){

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
        return Slash::IsAvailable(player, NULL);
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

class Qizhou: public TriggerSkill{
public:
    Qizhou():TriggerSkill("qizhou"){
        frequency = Frequent;
        events << TargetConfirmed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player && use.card->isKindOf("Slash") && use.card->isVirtualCard() && use.card->subcardsLength() != 0)
            {
                foreach(ServerPlayer *p, use.to)
                    if(!p->isNude() && player->askForSkillInvoke(objectName()))
                        room->obtainCard(player, room->askForCardChosen(player, p, "h", objectName()), false);
            }
        }
        else if(triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_src == player && resp.m_card->isVirtualCard() && resp.m_card->subcardsLength() != 0
                && resp.m_card->isKindOf("Jink") && player->askForSkillInvoke(objectName()))
                player->drawCards(1);
        }
        
        return false;
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

class Yufeng:public TriggerSkill{
public:
    Yufeng():TriggerSkill("yufeng"){
        events << TargetConfirmed << CardFinished;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!player->isAlive() || player != use.from || !use.card->isKindOf("Slash"))
                return false;
            int count = 1;
            int mark_n = player->getMark("no_jink" + use.card->toString());
            foreach (ServerPlayer *p, use.to) {
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                    room->broadcastSkillInvoke(objectName());

                    p->setFlags("TiejiTarget"); // For AI

                    JudgeStruct judge;
                    judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
                    judge.good = true;
                    judge.reason = objectName();
                    judge.who = player;

                    room->judge(judge);
                    if (judge.isGood()) {
                        LogMessage log;
                        log.type = "#NoJink";
                        log.from = p;
                        room->sendLog(log);

                        mark_n += count;
                        room->setPlayerMark(player, "no_jink" + use.card->toString(), mark_n);
                    }
                    else
                        player->obtainCard(judge.card);

                    p->setFlags("-TiejiTarget");
                }
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

class Huiquan:public TriggerSkill{
public:
    Huiquan():TriggerSkill("huiquan"){
        frequency = Frequent;
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        
        if(use.from == player && use.card->isKindOf("TrickCard")){
            if(room->askForSkillInvoke(player, objectName())){
                room->broadcastSkillInvoke(objectName());
                player->drawCards(1);
            }
        }

        return false;
    }
};

class Jizhi: public TargetModSkill {
public:
    Jizhi(): TargetModSkill("jizhi") {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class Xiagong: public TargetModSkill {
public:
    Xiagong(): TargetModSkill("xiagong") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->getWeapon() == NULL && from->hasSkill(objectName()))
            return 1;
        else
            return 0;
    }
};

class Liegong:public TriggerSkill{
public:
    Liegong():TriggerSkill("liegong"){
        events << TargetConfirmed << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!player->isAlive() || player != use.from || player->getPhase() != Player::Play || !use.card->isKindOf("Slash"))
                return false;
            int count = 1;
            int mark_n = player->getMark("no_jink" + use.card->toString());
            foreach (ServerPlayer *p, use.to) {
                int handcardnum = p->getHandcardNum();
                if ((player->getHp() <= handcardnum || player->getAttackRange() >= handcardnum)
                    && player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                    room->broadcastSkillInvoke(objectName());

                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);

                    mark_n += count;
                    room->setPlayerMark(player, "no_jink" + use.card->toString(), mark_n);
                }
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

class Kuanggu: public TriggerSkill{
public:
    Kuanggu():TriggerSkill("kuanggu"){
        frequency = Compulsory;
        events << Damage << PreHpReduced;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if(triggerEvent == PreHpReduced){
            if (damage.from == player)
                damage.from->tag["InvokeKuanggu"] = damage.from->distanceTo(damage.to) <= 1;
        }else if(triggerEvent == Damage){
            bool invoke = player->tag.value("InvokeKuanggu", false).toBool();
            player->tag["InvokeKuanggu"] = false;
            if(invoke) {
                room->broadcastSkillInvoke(objectName());
                for(int i = 0; i < damage.damage; i++) {

                    LogMessage log;
                    log.type = "#TriggerSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);

                    QStringList choices;

                    choices << "draw";

                    if(player->isWounded())
                        choices << "recover";

                    QString choice;

                    if(choices.length() > 1)
                        choice = room->askForChoice(player, objectName(), choices.join("+"));
                    else
                        choice = "draw";

                    if(choice == "draw")
                        player->drawCards(2);
                    else {
                        RecoverStruct recover;
                        recover.who = player;
                        room->recover(player, recover);
                    }
                }
            }
        }

        return false;
    }
};

class Fuyao: public OneCardViewAsSkill{
public:
    Fuyao():OneCardViewAsSkill("fuyao"){
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->getSuit() == Card::Club;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }
};

class Niepan: public TriggerSkill{
public:
    Niepan():TriggerSkill("niepan"){
        events << AskForPeaches;
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target) && target->getMark("@niepan") > 0;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        if(dying_data.who != player)
            return false;

        if(player->askForSkillInvoke(objectName(), data)){
            //room->broadcastInvoke("animate", "lightbox:$niepan");
            room->broadcastSkillInvoke(objectName());
            
            player->loseMark("@niepan");
            player->gainMark("@niepanused");

            QList<const Card *> tricks = player->getJudgingArea();
            foreach(const Card *trick, tricks)
            {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, this->objectName());
                room->throwCard(trick, reason, NULL);
            }
            
            RecoverStruct recover;
            recover.recover = qMin(3, player->getMaxHp()) - player->getHp();
            recover.who = player;
            room->recover(player, recover);

            player->drawCards(3);

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

class Shengtang: public TriggerSkill{
public:
    Shengtang():TriggerSkill("shengtang"){
        frequency = Compulsory;
        events << CardAsked;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target) && !target->getArmor()
               && target->getMark("Qinggang_Armor_Nullified") == 0;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();

        if(pattern != "jink")
            return false;

        if(player->askForSkillInvoke(objectName())){
            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
            judge.good = true;
            judge.reason = objectName();
            judge.who = player;
            judge.play_animation = true;

            room->judge(judge);

            if(judge.isGood()){
                room->setEmotion(player, "armor/eight_diagram");
                Jink *jink = new Jink(Card::NoSuitNoColor, 0);
                jink->setSkillName(objectName());
                room->provide(jink);
                return true;
            }
        }

        return false;
    }
};

class Cangyan: public OneCardViewAsSkill{
public:
    Cangyan():OneCardViewAsSkill("cangyan"){
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped() && to_select->isRed();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        return fire_attack;
    }
};

class Jinzhou: public OneCardViewAsSkill{
public:
    Jinzhou():OneCardViewAsSkill("jinzhou"){

    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isBlack() && !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "nullification";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());

        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        foreach(const Card *card, player->getHandcards())
            if(card->isBlack() || card->objectName() == "nullification")
                return true;
        
        return false;
    }
};

TiaoxinCard::TiaoxinCard() {
}

bool TiaoxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->inMyAttackRange(Self) && to_select != Self;
}

void TiaoxinCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    bool use_slash = false;
    if (effect.to->canSlash(effect.from, NULL, false))
        use_slash = room->askForUseSlashTo(effect.to, effect.from, "@tiaoxin-slash:" + effect.from->objectName());
    if (!use_slash && !effect.to->isNude())
        room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "tiaoxin"), effect.to, effect.from);
}

class Tiaoxin: public ZeroCardViewAsSkill {
public:
    Tiaoxin(): ZeroCardViewAsSkill("tiaoxin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("TiaoxinCard");
    }

    virtual const Card *viewAs() const{
        return new TiaoxinCard;
    }
};

class Shengtian: public TriggerSkill {
public:
    Shengtian(): TriggerSkill("shengtian") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && TriggerSkill::triggerable(target)
               && target->getMark("@shengtian") == 0
               && target->getPhase() == Player::Start
               && target->isKongcheng();
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
        if (data.value<PlayerStar>() != player)
            return false;
        LogMessage log;
        log.type = "#ShengtianWake";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        player->gainMark("@shengtian");
        if (player->isWounded() && room->askForChoice(player, objectName(), "recover+draw") == "recover")
        {
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        }
        else
        {
            room->drawCards(player, 2);
        }
        room->loseMaxHp(player);

        room->acquireSkill(player, "xuanwu");
        room->acquireSkill(player, "mohua");

        return false;
    }
};

class Mohua: public FilterSkill {
public:
    Mohua(): FilterSkill("mohua") {
    }

    static WrappedCard *changeToClub(int cardId){
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("mohua");
        new_card->setSuit(Card::Club);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->getSuit() == Card::Diamond;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        return changeToClub(originalCard->getEffectiveId());
    }
};

class Manbo: public TriggerSkill {
public:
    Manbo(): TriggerSkill("manbo") {
        events << SlashEffected << TargetConfirming;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        if (event == TargetConfirming) {
            ServerPlayer *target = NULL;
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->getMark("TargetConfirming") > 0)
                {
                    target = p;
                    break;
                }
            if (!target || target != player)
                return false;
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                player->setMark("manbo", 0);
                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.type = "#Manbo";
                log.from = use.from;
                log.to << player;
                log.arg = objectName();
                room->sendLog(log);

                QVariant dataforai = QVariant::fromValue(player);
                if (!room->askForCard(use.from, ".Basic", "@manbo-discard", dataforai, "manbo"))
                    player->addMark("manbo");
            }
        } else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (player->getMark("manbo") > 0)
            {
                player->setMark("manbo", player->getMark("manbo") - 1);
                return true;
            }
        }

        return false;
    }
};

class Baishen: public TriggerSkill {
public:
    Baishen(): TriggerSkill("baishen") {
        events << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.who != player)
            return false;
        switch (change.to) {
        case Player::Play: {
                bool invoked = false;
                if (player->isSkipped(Player::Play))
                    return false;
                invoked = player->askForSkillInvoke(objectName());
                if (invoked) {
                    player->setFlags("baishen");
                    player->skip(Player::Play);
                }
                break;
            }
        case Player::NotActive: {
                if (player->hasFlag("baishen")) {
                    if (player->isKongcheng() || !room->askForDiscard(player, "baishen", 1, 1, true))
                        return false;

                    ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());

                    room->broadcastSkillInvoke("baishen");
                    LogMessage log;
                    log.type = "#Baishen";
                    log.from = player;
                    log.to << target;
                    room->sendLog(log);

                    PlayerStar p = target;
                    room->setTag("BaishenTarget", QVariant::fromValue(p));
                }
                break;
            }
        default:
            break;
        }
        return false;
    }
};

class BaishenGive: public TriggerSkill {
public:
    BaishenGive(): TriggerSkill("#baishen-give") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPhase() == Player::NotActive;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
        if (data.value<PlayerStar>() != player)
            return false;
        if (!room->getTag("BaishenTarget").isNull()) {
            PlayerStar target = room->getTag("BaishenTarget").value<PlayerStar>();
            room->removeTag("BaishenTarget");
            if (target->isAlive())
                target->gainAnExtraTurn();
        }
        return false;
    }
};

class Hunshou: public TriggerSkill {
public:
    Hunshou(): TriggerSkill("hunshou$") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive()
               && target->hasLordSkill("hunshou")
               && target->getPhase() == Player::Start
               && target->getMark("@hunshou") == 0;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
        if (data.value<PlayerStar>() != player)
            return false;
        bool can_invoke = true;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (player->getHpPoints() > p->getHpPoints()) {
                can_invoke = false;
                break;
            }
        }

        if (can_invoke) {
            room->broadcastSkillInvoke(objectName());
            //room->broadcastInvoke("animate", "lightbox:$HunshouAnimate");
            //room->getThread()->delay(1500);

            LogMessage log;
            log.type = "#HunshouWake";
            log.from = player;
            log.arg = QString::number(player->getHp());
            log.arg2 = objectName();
            room->sendLog(log);

            player->gainMark("@hunshou");

            room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);

            if (player->isLord())
                room->acquireSkill(player, "liqi");
        }

        return false;
    }
};

class SavageAssaultAvoid: public TriggerSkill{
public:
    SavageAssaultAvoid(const QString &avoid_skill)
        :TriggerSkill("#sa_avoid_" + avoid_skill), avoid_skill(avoid_skill)
    {
        events << PreCardEffected;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if(effect.to == player && effect.card->isKindOf("SavageAssault")){
            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = avoid_skill;
            log.arg2 = "savage_assault";
            room->sendLog(log);

            return true;
        }else
            return false;
    }

private:
    QString avoid_skill;
};

class Huoshou: public TriggerSkill{
public:
    Huoshou():TriggerSkill("huoshou"){
        events << TargetConfirmed << ConfirmDamage << CardFinished;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player))
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card->isKindOf("SavageAssault") && !use.from->hasSkill(objectName())){
                room->broadcastSkillInvoke(objectName());
                room->setTag("HuoshouSource", QVariant::fromValue((PlayerStar)player));
            }
        }
        else if(triggerEvent == ConfirmDamage && !room->getTag("HuoshouSource").isNull()){
            DamageStruct damage = data.value<DamageStruct>();
            if(!damage.card || !damage.card->isKindOf("SavageAssault"))
                return false;

            ServerPlayer *player = room->getTag("HuoshouSource").value<PlayerStar>();
            if(player->isAlive())
                damage.from = player;
            else
                damage.from = NULL;
            data = QVariant::fromValue(damage);
        }
        else if(triggerEvent == CardFinished && TriggerSkill::triggerable(player)){
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card->isKindOf("SavageAssault"))
                room->removeTag("HuoshouSource");
        }

        return false;
    }
};

class Zailuan: public TriggerSkill {
public:
    Zailuan():TriggerSkill("zailuan") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::Draw
            && player->isWounded())
        {
            if(player->askForSkillInvoke(objectName()))
            {
                int x = player->getLostHp();

                room->broadcastSkillInvoke(objectName(), 1);
                bool has_heart = false;

                for (int i = 0; i < x; i++)
                {
                    int card_id = room->drawCard();
                    room->moveCardTo(Sanguosha->getCard(card_id), NULL, NULL, Player::PlaceTable,
                        CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), QString(), objectName(), QString()), true);
                    room->getThread()->delay();

                    const Card *card = Sanguosha->getCard(card_id);
                    if(card->getSuit() == Card::Heart){
                        QStringList choices;

                        choices << "recover" << "draw";

                        QString choice = room->askForChoice(player, objectName(), choices.join("+"));

                        if(choice == "draw")
                            player->drawCards(2);
                        else {
                            RecoverStruct recover;
                            recover.card = card;
                            recover.who = player;
                            room->recover(player, recover);
                            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
                            room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
                        }

                        has_heart = true;
                    }else{
                        CardMoveReason reason(CardMoveReason::S_REASON_GOTBACK, player->objectName());
                        room->obtainCard(player, Sanguosha->getCard(card_id), reason);
                    }
                }

                if(has_heart)
                    room->broadcastSkillInvoke(objectName(), 2);
                else
                    room->broadcastSkillInvoke(objectName(), 3);

                return true;
            }
        }

        return false;
    }
};

class Jugui: public TriggerSkill{
public:
    Jugui():TriggerSkill("jugui"){
        events << PostCardEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && !target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.card->isKindOf("SavageAssault") &&
                ((!use.card->isVirtualCard()) ||
                  (use.card->getSubcards().length() == 1 &&
                  Sanguosha->getCard(use.card->getSubcards().first())->isKindOf("SavageAssault")))){
            if (player == NULL) return false;
            if(room->getCardPlace(use.card->getEffectiveId()) == Player::DiscardPile){
                // finding zhurong;
                QList<ServerPlayer *> players = room->getAllPlayers();
                foreach(ServerPlayer *p, players){
                    if(p->hasSkill(objectName())){
                        room->broadcastSkillInvoke(objectName());
                        p->obtainCard(use.card);
                        break;
                    }
                }
            }
        }

        return false;
    }
};

class Lieren: public TriggerSkill{
public:
    Lieren():TriggerSkill("lieren"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if(damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))
            && !player->isKongcheng() && !target->isKongcheng() && target != player && !damage.chain && !damage.transfer){
            if(room->askForSkillInvoke(player, objectName(), data)){
                room->broadcastSkillInvoke(objectName(), 1);

                bool success = player->pindian(target, objectName(), NULL);
                if(success)
                    room->broadcastSkillInvoke(objectName(), 2);
                else{
                    room->broadcastSkillInvoke(objectName(), 3);
                    return false;
                }

                if(!target->isNude()){
                    int card_id = room->askForCardChosen(player, target, "he", objectName());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                    room->obtainCard(player, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
                }
            }
        }

        return false;
    }
};

XuanhuiCard::XuanhuiCard() {
}

bool XuanhuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void XuanhuiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->drawCards(effect.to, 2);
    if (!effect.from->isAlive() || !effect.to->isAlive())
        return;

    bool can_use = false;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
        if (effect.to->canSlash(p)) {
            can_use = true;
            break;
        }
    }
    ServerPlayer *victim = NULL;
    if (can_use) {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *victim, room->getOtherPlayers(effect.to)) {
            if (effect.to->canSlash(victim))
                targets << victim;
        }
        victim = room->askForPlayerChosen(effect.from, targets, "xuanhui");

        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = effect.from;
        log.to << victim;
        room->sendLog(log);

        QString prompt = QString("xuanhui-slash:%1:%2").arg(effect.from->objectName()).arg(victim->objectName());
        if (!room->askForUseSlashTo(effect.to, victim, prompt)) {
            DummyCard *dummy = room->askForCardsChosen(effect.from, effect.to, "he", "xuanhui", 2);
            room->moveCardTo(dummy, effect.from, Player::PlaceHand, false);
            delete dummy;
        }
    } else {
        if (effect.to->isNude())
            return;
        DummyCard *dummy = room->askForCardsChosen(effect.from, effect.to, "he", "xuanhui", 2);
        room->moveCardTo(dummy, effect.from, Player::PlaceHand, false);
        delete dummy;
    }
}

class XuanhuiViewAsSkill: public ZeroCardViewAsSkill {
public:
    XuanhuiViewAsSkill(): ZeroCardViewAsSkill("xuanhui") {
    }

    virtual const Card *viewAs() const{
        return new XuanhuiCard;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return  pattern == "@@xuanhui";
    }
};

class Xuanhui: public TriggerSkill {
public:
    Xuanhui(): TriggerSkill("xuanhui") {
        events << EventPhaseStart;
        view_as_skill = new XuanhuiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() != player)
            return false;
        if (player->getPhase() == Player::Draw && room->askForUseCard(player, "@@xuanhui", "@xuanhui-card"))
            return true;

        return false;
    }
};

class Enyuan: public TriggerSkill {
public:
    Enyuan(): TriggerSkill("enyuan") {
        events << CardsMoveOneTime << Damaged;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        if (event == CardsMoveOneTime) {
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            if (move->to == player && move->from && move->from->isAlive() && move->from != move->to
                && move->card_ids.size() >= 2
                && move->reason.m_reason != CardMoveReason::S_REASON_PREVIEWGIVE
                && room->askForSkillInvoke(player, objectName(), data)) {
                room->drawCards((ServerPlayer *)move->from, 1);
                room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
            }
        } else if (event == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (!source || source == player) return false;
            int x = damage.damage;
            for (int i = 0; i < x; i++) {
                if (room->askForSkillInvoke(player, objectName(), data)) {
                    room->broadcastSkillInvoke(objectName(), qrand() % 2 + 3);
                    const Card *card = NULL;
                    if (!source->isKongcheng())
                        card = room->askForExchange(source, objectName(), 1, false, "EnyuanGive", true);
                    if (card) {
                        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
                                              player->objectName(), objectName(), QString());
                        reason.m_playerId = player->objectName();
                        room->moveCardTo(card, source, player, Player::PlaceHand, reason);
                    } else {
                        room->loseHp(source);
                    }
                } else {
                    break;
                }
            }
        }
        return false;
    }
};

XinchaoCard::XinchaoCard(){
    target_fixed = true;
    once = true;
}

void XinchaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    QList<int> cards = room->getNCards(3), left;
    left = cards;

    QList<int> hearts;
    foreach(int card_id, cards){
        const Card *card = Sanguosha->getCard(card_id);
        if(card->getSuit() == Card::Heart)
            hearts << card_id;
    }

    if(!hearts.isEmpty()){
        room->fillAG(cards, source);

        while(!hearts.isEmpty()){
            int card_id = room->askForAG(source, hearts, true, "xinchao");
            if(card_id == -1)
                break;

            if(!hearts.contains(card_id))
                continue;

            hearts.removeOne(card_id);
            left.removeOne(card_id);

            source->obtainCard(Sanguosha->getCard(card_id));
        }

        source->invoke("clearAG");
    }

    if(!left.isEmpty())
        room->askForYuxi(source, left, true);
 }

class Xinchao: public ZeroCardViewAsSkill{
public:
    Xinchao():ZeroCardViewAsSkill("xinchao"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("XinchaoCard") && player->getHandcardNum() > player->getHp();
    }

    virtual const Card *viewAs() const{
        return new XinchaoCard;
    }
};

class Shangshi: public TriggerSkill {
public:
    Shangshi():TriggerSkill("shangshi") {
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (killer && player->askForSkillInvoke(objectName()))
            killer->throwAllHandCardsAndEquips();

        return false;
    }
};

class Mitu: public TriggerSkill{
public:
    Mitu():TriggerSkill("mitu"){
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.card && damage.card->getTypeId() == Card::TypeTrick){
            if((triggerEvent == DamageInflicted)
                    || (triggerEvent == DamageCaused && damage.from && damage.from == player))
            {
                LogMessage log;
                log.type = "#Mitu";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->broadcastSkillInvoke(objectName());

                return true;
            }
        }

        return false;
    }
};

Sishi2Card::Sishi2Card(){
}

bool Sishi2Card::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    return true;
}

void Sishi2Card::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    QStringList choicelist;
    choicelist << "draw";
    if (effect.to->isWounded())
        choicelist << "recover";
    if (!effect.to->faceUp() || effect.to->isChained())
        choicelist << "reset";
        QString choice;
    if (choicelist.length() >= 2)
        choice = room->askForChoice(effect.to, "sishi2", choicelist.join("+"));
    else
        choice = "draw";
    if(choice == "draw")
        effect.to->drawCards(2);
    else if(choice == "recover"){
        RecoverStruct recover;
        recover.who = effect.to;
        room->recover(effect.to, recover);
    }
    else if(choice == "reset"){
        if(!effect.to->faceUp())
            effect.to->turnOver();

        room->setPlayerProperty(effect.to, "chained", false);
    }
}

class Sishi2: public OneCardViewAsSkill{
public:
    Sishi2():OneCardViewAsSkill("sishi2"){
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Sishi2Card *sishiCard = new Sishi2Card;
        sishiCard->addSubcard(originalCard);
        return sishiCard;
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isKindOf("BasicCard");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("Sishi2Card");
    }
};

class Wanhun: public TriggerSkill {
public:
    Wanhun(): TriggerSkill("wanhun") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.from && damage.from == player
            && player->distanceTo(damage.to) <= 2
            && damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))
            && !damage.chain && !damage.transfer
            && player->askForSkillInvoke(objectName(), data))
        {
            room->broadcastSkillInvoke(objectName(), 1);
            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(heart):(.*)");
            judge.good = false;
            judge.who = player;
            judge.reason = objectName();

            room->judge(judge);
            if (judge.isGood()) {
                room->broadcastSkillInvoke(objectName(), 2);
                room->loseMaxHp(damage.to);
                return true;
            } else
                room->broadcastSkillInvoke(objectName(), 3);
        }
        return false;
    }
};

class Meiying: public TriggerSkill {
public:
    Meiying(): TriggerSkill("meiying") {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::RoundStart) {
            room->broadcastSkillInvoke(objectName());
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (!p->isKongcheng())
                    targets << p;

            if (!targets.isEmpty() && room->askForChoice(player, objectName(), "play+look") == "look")
            {
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                room->showAllCards(target, player);
            }
            else
            {
                player->setPhase(Player::Play);
                room->broadcastProperty(player, "phase");
            }
        }
        return false;
    }
};

class Fansheng: public TriggerSkill {
public:
    Fansheng(): TriggerSkill("fansheng") {
        events << AskForPeaches;
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("@fansheng") > 0;
    }

    int getKingdoms(Room *room) const{
        QSet<QString> kingdom_set;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            kingdom_set << p->getKingdom();
        return kingdom_set.size();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != player)
            return false;
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            
            player->loseMark("@fansheng");
            player->loseMark("@fanshengused");

            RecoverStruct recover;
            recover.recover = qMin(getKingdoms(room), player->getMaxHp()) - player->getHp();
            room->recover(player, recover);

            player->turnOver();
        }
        return false;
    }
};

class XieyongViewAsSkill: public OneCardViewAsSkill {
public:
    XieyongViewAsSkill(): OneCardViewAsSkill("xieyong") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        Slash *slash = new Slash(Card::Diamond, 0);
        slash->deleteLater();
        return slash->isAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "slash";
    }

    virtual bool viewFilter(const Card* card) const{
        if(card->getSuit() != Card::Diamond)
            return false;

        if(Self->getWeapon() && card->getEffectiveId() == Self->getWeapon()->getId() && card->objectName() == "Crossbow")
        {
            Slash *slash = new Slash(Card::Diamond, card->getNumber());
            slash->deleteLater();
            return Self->canSlashWithoutCrossbow() || Self->canUseExtraSlash(slash);
        }
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

class Xieyong: public TriggerSkill {
public:
    Xieyong(): TriggerSkill("xieyong") {
        events << CardUsed;
        view_as_skill = new XieyongViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
            room->setPlayerMark(player, objectName(), use.card->getColor() + 1);

        return false;
    }
};

class Shushen: public TriggerSkill{
public:
    Shushen():TriggerSkill("shushen"){
        events << HpRecovered;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        RecoverStruct recover = data.value<RecoverStruct>();
        for (int i = 0; i < recover.recover; i++) {
            if (room->askForSkillInvoke(player, objectName())) {
                ServerPlayer *target1 = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName());
                target1->drawCards(1);
                ServerPlayer *target2 = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
                target2->drawCards(1);
            } else
                break;
        }
        return false;
    }
};

class Qiaoxia: public TriggerSkill {
public:
    Qiaoxia(): TriggerSkill("qiaoxia") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
        if (data.value<PlayerStar>() != player || player->getPhase() != Player::Start || player->isKongcheng())
            return false;

        if(player->askForSkillInvoke(objectName())){
            int handcard_num = player->getHandcardNum();
            player->throwAllHandCards();
            if(handcard_num >= player->getHp()){
                RecoverStruct recover;
                recover.who = player;
                room->recover(player, recover);
            }
        }
        return false;
    }
};

XielunCard::XielunCard() {
}

bool XielunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() >= Self->getLostHp())
        return false;

    if (to_select == Self)
        return false;

    int range_fix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getEffectiveId() == getEffectiveId()) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        range_fix += weapon->getRange() - 1;
    } else if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getEffectiveId() == getEffectiveId())
        range_fix += 1;

    return Self->distanceTo(to_select, range_fix) <= Self->getAttackRange();
}

void XielunCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    DamageStruct damage;
    damage.from = source;

    foreach (ServerPlayer *p, targets) {
        damage.to = p;
        room->damage(damage);
    }
    foreach (ServerPlayer *p, targets) {
        if (p->isAlive())
            p->drawCards(1);
    }
}

class Xielun: public OneCardViewAsSkill {
public:
    Xielun(): OneCardViewAsSkill("xielun") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getLostHp() > 0 && !player->isNude() && !player->hasUsed("XielunCard");
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->isRed() && !Self->isHuyin(to_select);
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        XielunCard *first = new XielunCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Canyue: public TargetModSkill {
public:
    Canyue(): TargetModSkill("canyue") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return from->getMark(objectName());
        else
            return 0;
    }
};

class CanyueCount: public TriggerSkill {
public:
    CanyueCount(): TriggerSkill("#canyue-count") {
        events << SlashMissed << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        if (event == SlashMissed) {
            if (player->getPhase() == Player::Play)
                room->setPlayerMark(player, "canyue", player->getMark("canyue") + 1);
        } else if (event == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.who != player)
                return false;

            if (change.from == Player::Play)
                if (player->getMark("canyue") > 0)
                    room->setPlayerMark(player, "canyue", 0);
        }

        return false;
    }
};

class CanyueClear: public TriggerSkill {
public:
    CanyueClear(): TriggerSkill("#canyue-clear") {
        events << EventLoseSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && !target->hasSkill("canyue") && target->getMark("canyue") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->setPlayerMark(player, "canyue", 0);
        return false;
    }
};

class JiumingCount: public TriggerSkill {
public:
    JiumingCount(): TriggerSkill("#jiuming-count") {
        events << DamageDone << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        if (event == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.from == room->getCurrent() && damage.from->getMark("jiuming") == 0)
                room->setPlayerMark(damage.from, "jiuming_damage", damage.from->getMark("jiuming_damage") + damage.damage);
        } else if (event == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.who != player)
                return false;

            if (change.to == Player::NotActive)
                if (player->getMark("jiuming_damage") > 0)
                    room->setPlayerMark(player, "jiuming_damage", 0);
        }

        return false;
    }
};

class Jiuming: public TriggerSkill {
public:
    Jiuming(): TriggerSkill("jiuming") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
               && target->getPhase() == Player::Finish
               && target->getMark("@jiuming") == 0
               && target->getMark("jiuming_damage") >= 3;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const {
        if (data.value<PlayerStar>() != player)
            return false;
        LogMessage log;
        log.type = "#JiumingWake";
        log.from = player;
        log.arg = QString::number(player->getMark("jiuming_damage"));
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        player->gainMark("@jiuming");

        room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
        RecoverStruct recover;
        recover.who = player;
        room->recover(player, recover);

        room->detachSkillFromPlayer(player, "canyue");
        
        return false;
    }
};

class Xiewang: public TriggerSkill {
public:
    Xiewang(): TriggerSkill("xiewang") {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    static void XiewangChange(Room *room, ServerPlayer *player, int hp, const QString &skill_name) {
        QStringList xiewang_skills = player->tag["XiewangSkills"].toStringList();
        if (player->getHpPoints() <= hp) {
            if (!xiewang_skills.contains(skill_name)) {
                room->broadcastSkillInvoke("xiewang");
                room->acquireSkill(player, skill_name);
                xiewang_skills << skill_name;
            }
        } else {
            room->detachSkillFromPlayer(player, skill_name);
            xiewang_skills.removeOne(skill_name);
        }
        player->tag["XiewangSkills"] = QVariant::fromValue(xiewang_skills);
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName()) {
                QStringList xiewang_skills = player->tag["XiewangSkills"].toStringList();
                foreach (QString skill_name, xiewang_skills)
                    room->detachSkillFromPlayer(player, skill_name);
                player->tag["XiewangSkills"] = QVariant();
            }
            return false;
        }

        if (!TriggerSkill::triggerable(player)) return false;

        XiewangChange(room, player, 1, "thshenyou");
        XiewangChange(room, player, 2, "thkuangqi");
        XiewangChange(room, player, 3, "tiaoxin");
        return false;
    }
};

class Qiyao: public TriggerSkill {
public:
    Qiyao(): TriggerSkill("qiyao") {
        frequency = Frequent;
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getPile("qiyaopile").length() > 0
               && target->getPhase() == Player::Draw;
    }

    static void Exchange(ServerPlayer *shenzhuge) {
        QList<int> ids = shenzhuge->getPile("qiyaopile");
        if (ids.isEmpty()) return;

        shenzhuge->getRoom()->broadcastSkillInvoke("qiyao");
        shenzhuge->exchangeFreelyFromPrivatePile("qiyao", "qiyaopile");
    }

    static void DiscardStar(ServerPlayer *shenzhuge, int n, QString skillName) {
        Room *room = shenzhuge->getRoom();
        QList<int> ids = shenzhuge->getPile("qiyaopile");

        for (int i = 0; i < n; i++) {
            room->fillAG(ids, shenzhuge);
            int card_id = room->askForAG(shenzhuge, ids, false, "qiyao-discard");
            shenzhuge->invoke("clearAG");
            ids.removeOne(card_id);
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), skillName, QString());
            room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
        }
    }

    virtual bool trigger(TriggerEvent , Room *, ServerPlayer *shenzhuge, QVariant &data) const{
        if (data.value<PlayerStar>() != shenzhuge)
            return false;
        Exchange(shenzhuge);
        return false;
    }
};

class QiyaoStart: public GameStartSkill {
public:
    QiyaoStart(): GameStartSkill("#qiyao") {
    }

    virtual void onGameStart(ServerPlayer *shenzhuge) const{
        Room *room = shenzhuge->getRoom();

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = shenzhuge;
        log.arg = "qiyao";
        room->sendLog(log);

        shenzhuge->drawCards(7);

        room->broadcastSkillInvoke("qiyao");
        const Card *exchange_card = room->askForExchange(shenzhuge, "qiyao", 7);
        shenzhuge->addToPile("qiyaopile", exchange_card->getSubcards(), false);
        delete exchange_card;
    }
};

LiefengCard::LiefengCard() {
    handling_method = Card::MethodNone;
}

bool LiefengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void LiefengCard::onEffect(const CardEffectStruct &effect) const{
    Qiyao::DiscardStar(effect.from, 1, "liefeng");
    effect.from->tag["Qiyao_user"] = true;
    effect.to->gainMark("@liefeng");
}

class LiefengViewAsSkill: public ZeroCardViewAsSkill {
public:
    LiefengViewAsSkill(): ZeroCardViewAsSkill("liefeng") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@liefeng";
    }

    virtual const Card *viewAs() const{
        return new LiefengCard;
    }
};

class Liefeng: public TriggerSkill {
public:
    Liefeng(): TriggerSkill("liefeng") {
        events << DamageForseen;
        view_as_skill = new LiefengViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getMark("@liefeng") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire) {
            LogMessage log;
            log.type = "#LiefengPower";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class QiyaoAsk: public TriggerSkill {
public:
    QiyaoAsk(): TriggerSkill("#qiyao-ask") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::Finish) {
            if (player->getPile("qiyaopile").length() > 0 && player->hasSkill("liefeng"))
                room->askForUseCard(player, "@@liefeng", "@liefeng-card", -1, Card::MethodNone);

            if (player->getPile("qiyaopile").length() > 0 && player->hasSkill("miaowu"))
                room->askForUseCard(player, "@@miaowu", "@miaowu-card", -1, Card::MethodNone);
        }

        return false;
    }
};

class QiyaoClear: public TriggerSkill {
public:
    QiyaoClear(): TriggerSkill("#qiyao-clear") {
        events << EventPhaseStart << Death << EventLoseSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        if (event == EventPhaseStart || event == Death) {
            if (event == EventPhaseStart && data.value<PlayerStar>() != player)
                return false;
            if (event == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return false;
            }
            if (!player->tag.value("Qiyao_user", false).toBool())
                return false;
            bool invoke = false;
            if ((event == EventPhaseStart && player->getPhase() == Player::RoundStart) || event == Death)
                invoke = true;
            if (!invoke)
                return false;
            QList<ServerPlayer *> players = room->getAllPlayers();
            foreach (ServerPlayer *player, players) {
                player->loseAllMarks("@liefeng");
                player->loseAllMarks("@miaowu");
            }
            player->tag.remove("Qiyao_user");
        } else if (event == EventLoseSkill) {
            if (!player->hasSkill("qiyao") && player->getPile("qiyaopile").length() > 0)
                player->clearOnePrivatePile("qiyaopile");
        }

        return false;
    }
};

MiaowuCard::MiaowuCard() {
    handling_method = Card::MethodNone;
}

bool MiaowuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < Self->getPile("qiyaopile").length();
}

void MiaowuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    int n = targets.length();
    Qiyao::DiscardStar(source, n, "miaowu");
    source->tag["Qiyao_user"] = true;

    foreach (ServerPlayer *target, targets)
        target->gainMark("@miaowu");
}

class MiaowuViewAsSkill: public ZeroCardViewAsSkill {
public:
    MiaowuViewAsSkill(): ZeroCardViewAsSkill("miaowu") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@miaowu";
    }

    virtual const Card *viewAs() const{
        return new MiaowuCard;
    }
};

class Miaowu: public TriggerSkill {
public:
    Miaowu(): TriggerSkill("miaowu") {
        events << DamageForseen;
        view_as_skill = new MiaowuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getMark("@miaowu") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Thunder) {
            LogMessage log;
            log.type = "#MiaowuProtect";
            log.from = player;
            log.arg = QString::number(damage.damage);
            if (damage.nature == DamageStruct::Normal)
                log.arg2 = "normal_nature";
            else if (damage.nature == DamageStruct::Fire)
                log.arg2 = "fire_nature";
            room->sendLog(log);

            return true;
        } else
            return false;
    }
};

class Juejing: public MaxCardsSkill {
public:
    Juejing(): MaxCardsSkill("juejing") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return 2;
        else
            return 0;
    }
};

class JuejingDrawCardsSkill: public DrawCardsSkill {
public:
    JuejingDrawCardsSkill(): DrawCardsSkill("#juejing") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        if (player->isWounded()) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = "juejing";
            room->sendLog(log);
            room->broadcastSkillInvoke("juejing");
        }
        return n + player->getLostHp();
    }
};

class Zhihun: public ViewAsSkill {
public:
    Zhihun(): ViewAsSkill("zhihun") {
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const {
        return pattern == "slash"
               || pattern == "jink"
               || pattern.contains("peach")
               || pattern == "nullification";
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return player->isWounded() || Slash::IsAvailable(player);
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *card) const {
        int n = qMax(1, Self->getHp());

        if (selected.length() >= n || card->hasFlag("using"))
            return false;

        if (n > 1 && !selected.isEmpty()) {
            Card::Suit suit = selected.first()->getSuit();
            return card->getSuit() == suit;
        }

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
                if (Self->isWounded() && card->getSuit() == Card::Heart)
                    return true;
                else if (Slash::IsAvailable(Self) && card->getSuit() == Card::Spade)
                    return true;
                else
                    return false;
            }
        case CardUseStruct::CARD_USE_REASON_RESPONSE: {
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                if (pattern == "jink")
                    return card->getSuit() == Card::Club;
                else if (pattern == "nullification")
                    return card->getSuit() == Card::Diamond;
                else if (pattern == "peach" || pattern == "peach+analeptic")
                    return card->getSuit() == Card::Heart;
                else if (pattern == "slash")
                    return card->getSuit() == Card::Spade;
            }
        default:
            break;
        }

        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        int n = qMax(1, Self->getHp());

        if (cards.length() != n)
            return NULL;

        const Card *card = cards.first();
        Card *new_card = NULL;

        switch (card->getSuit()) {
        case Card::Diamond: {
                new_card = new Nullification(Card::SuitToBeDecided, 0);
                break;
            }
        case Card::Heart: {
                new_card = new Peach(Card::SuitToBeDecided, 0);
                break;
            }
        case Card::Club: {
                new_card = new Jink(Card::SuitToBeDecided, 0);
                break;
            }
        case Card::Spade: {
                new_card = new FireSlash(Card::SuitToBeDecided, 0);
                break;
            }
        default:
            break;
        }

        if (new_card) {
            new_card->setSkillName(objectName());
            new_card->addSubcards(cards);
            return new_card;
        }

        return NULL;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const{
        int n = qMax(1, player->getHp());
        int count = 0;
        foreach (const Card *card, player->getCards("he")) {
            if (card->objectName() == "nullification")
                return true;

            if (card->getSuit() == Card::Diamond)
                count++;
        }

        return count >= n;
    }
};

void StandardPackage::addWindGenerals(){
    General *wind001 = new General(this, "wind001$", "shu");
    wind001->addSkill(new Funuan);
    wind001->addSkill(new Liqi);

    General *wind002 = new General(this, "wind002", "shu");
    wind002->addSkill(new Chilian);
    wind002->addSkill(new Zhenhong);
    wind002->addSkill(new ZhenhongTargetMod);
    related_skills.insertMulti("zhenhong", "#zhenhong-target");

    General *wind003 = new General(this, "wind003", "shu");
    wind003->addSkill(new Hupao);
    wind003->addSkill(new Jiuli);

    General *wind004 = new General(this, "wind004", "shu", 3);
    wind004->addSkill(new Yuxi);
    wind004->addSkill(new Jingyou);
    wind004->addSkill(new JingyouEffect);
    related_skills.insertMulti("jingyou", "#jingyou-effect");

    General *wind005 = new General(this, "wind005", "shu", 3);
    wind005->addSkill(new Huahuan);
    wind005->addSkill(new Qizhou);

    General *wind006 = new General(this, "wind006", "shu");
    wind006->addSkill(new Jibu);
    wind006->addSkill(new Yufeng);

    General *wind007 = new General(this, "wind007", "shu", 3, false);
    wind007->addSkill(new Huiquan);
    wind007->addSkill(new Jizhi);

    General *wind008 = new General(this, "wind008", "shu");
    wind008->addSkill(new Xiagong);
    wind008->addSkill(new Liegong);

    General *wind009 = new General(this, "wind009", "shu");
    wind009->addSkill(new Kuanggu);

    General *wind010 = new General(this, "wind010", "shu", 3);
    wind010->addSkill(new Fuyao);
    wind010->addSkill(new MarkAssignSkill("@niepan", 1));
    wind010->addSkill(new Niepan);
    related_skills.insertMulti("niepan", "#@niepan-1");

    General *wind011 = new General(this, "wind011", "shu", 3);
    wind011->addSkill(new Shengtang);
    wind011->addSkill(new Cangyan);
    wind011->addSkill(new Jinzhou);

    General *wind012 = new General(this, "wind012", "shu");
    wind012->addSkill(new Tiaoxin);
    wind012->addSkill(new Shengtian);
    wind012->addRelateSkill("mohua");

    General *wind013 = new General(this, "wind013", "shu");
    wind013->addSkill(new SavageAssaultAvoid("huoshou"));
    wind013->addSkill(new Huoshou);
    wind013->addSkill(new Zailuan);
    related_skills.insertMulti("huoshou", "#sa_avoid_huoshou");

    General *wind014 = new General(this, "wind014$", "shu", 3);
    wind014->addSkill(new Manbo);
    wind014->addSkill(new Baishen);
    wind014->addSkill(new BaishenGive);
    related_skills.insertMulti("baishen", "#baishen-give");
    wind014->addSkill(new Hunshou);

    General *wind015 = new General(this, "wind015", "shu", 4, false);
    wind015->addSkill(new SavageAssaultAvoid("jugui"));
    wind015->addSkill(new Jugui);
    related_skills.insertMulti("jugui", "#sa_avoid_jugui");
    wind015->addSkill(new Lieren);

    General *wind016 = new General(this, "wind016", "shu", 3);
    wind016->addSkill(new Xuanhui);
    wind016->addSkill(new Enyuan);

    General *wind017 = new General(this, "wind017", "shu", 3);
    wind017->addSkill(new Xinchao);
    wind017->addSkill(new Shangshi);

    General *wind018 = new General(this, "wind018", "shu", 3);
    wind018->addSkill(new Mitu);
    wind018->addSkill(new Sishi2);

    General *wind019 = new General(this, "wind019", "shu");
    wind019->addSkill("xiagong");
    wind019->addSkill(new Wanhun);

    General *wind020 = new General(this, "wind020", "shu");
    wind020->addSkill(new Meiying);
    wind020->addSkill(new Fansheng);
    wind020->addSkill(new MarkAssignSkill("@fansheng", 1));
    related_skills.insertMulti("fansheng", "#@fansheng-1");

    General *wind021 = new General(this, "wind021", "shu");
    wind021->addSkill(new Xieyong);

    General *wind022 = new General(this, "wind022", "shu", 3, false);
    wind022->addSkill(new Shushen);
    wind022->addSkill(new Qiaoxia);

    General *wind023 = new General(this, "wind023", "shu", 3, false);
    wind023->addSkill(new Xielun);
    wind023->addSkill(new Canyue);
    wind023->addSkill(new CanyueCount);
    wind023->addSkill(new CanyueClear);
    wind023->addSkill(new Jiuming);
    wind023->addSkill(new JiumingCount);
    related_skills.insertMulti("jiuming", "#jiuming-count");
    related_skills.insertMulti("canyue", "#canyue-count");
    related_skills.insertMulti("canyue", "#canyue-clear");

    General *wind024 = new General(this, "wind024", "shu");
    wind024->addSkill(new Xiewang);
    wind024->addSkill("shengzun");

    General *wind029 = new General(this, "wind029", "shu", 3);
    wind029->addSkill(new Qiyao);
    wind029->addSkill(new QiyaoStart);
    wind029->addSkill(new QiyaoAsk);
    wind029->addSkill(new QiyaoClear);
    wind029->addSkill(new Liefeng);
    wind029->addSkill(new Miaowu);
    related_skills.insertMulti("qiyao", "#qiyao");
    related_skills.insertMulti("qiyao", "#qiyao-ask");
    related_skills.insertMulti("qiyao", "#qiyao-clear");

    General *wind030 = new General(this, "wind030", "shu", 2);
    wind030->addSkill(new Juejing);
    wind030->addSkill(new JuejingDrawCardsSkill);
    wind030->addSkill(new Zhihun);
    related_skills.insertMulti("juejing", "#juejing");
    
    addMetaObject<FunuanCard>();
    addMetaObject<LiqiCard>();
    addMetaObject<TiaoxinCard>();
    addMetaObject<XinchaoCard>();
    addMetaObject<Sishi2Card>();
    addMetaObject<XielunCard>();
    addMetaObject<LiefengCard>();
    addMetaObject<MiaowuCard>();

    skills << new Mohua;
}
