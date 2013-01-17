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
        events << TargetConfirmed << CardFinished;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct &use = data.value<CardUseStruct>();

        if(triggerEvent == CardFinished){
            foreach(ServerPlayer *p, use.to)
                p->removeMark("qinggang");

            return false;
        }

        if(use.card && use.card->isKindOf("Slash") && use.card->getSuit() == Card::Diamond) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            foreach(ServerPlayer *p, use.to)
                p->addMark("qinggang");
        }

        return false;
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

class Yuxi:public PhaseChangeSkill{
public:
    Yuxi():PhaseChangeSkill("yuxi"){
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if(player->getPhase() == Player::Start &&
           player->askForSkillInvoke(objectName()))
        {
            Room *room = player->getRoom();
            room->broadcastSkillInvoke(objectName());

            int n = qMin(5, room->alivePlayerCount());
            room->askForYuxi(player, room->getNCards(n, false), false);
        }

        return false;
    }
};

class Jingmo: public ProhibitSkill{
public:
    Jingmo():ProhibitSkill("jingmo"){

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        if(card->isKindOf("Slash") || card->isKindOf("Duel"))
            return to->isKongcheng();
        else
            return false;
    }
};

class JingmoEffect: public TriggerSkill{
public:
    JingmoEffect():TriggerSkill("#jingmo-effect"){
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
        events << TargetConfirmed << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        const Card *card;
        if(triggerEvent == CardUsed || triggerEvent == TargetConfirmed) {
            CardUseStruct &use = data.value<CardUseStruct>();
            card = use.card;
        }
        else if(triggerEvent == CardResponded) {
            CardResponseStruct &resp = data.value<CardResponseStruct>();
            card = resp.m_card;
        }

        if(card == NULL || !card->isVirtualCard())
            return false;

        if((triggerEvent == CardUsed || triggerEvent == CardResponded) && card->isKindOf("Jink"))
            if(player->askForSkillInvoke(objectName()))
                player->drawCards(1);

        if(triggerEvent == TargetConfirmed && card->isKindOf("Slash")) {
            CardUseStruct &use = data.value<CardUseStruct>();
            foreach(ServerPlayer *p, use.to)
                if(!p->isNude() && player->askForSkillInvoke(objectName()))
                    room->obtainCard(player, room->askForCardChosen(player, p, "he", objectName()), false);
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
        events << TargetConfirmed << SlashProceed << CardFinished;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{

        if(triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if(!use.card->isKindOf("Slash") || use.from != player)
                   return false;

            foreach(ServerPlayer *target, use.to){
                bool invoke = player->askForSkillInvoke(objectName(), QVariant::fromValue(target));
                JudgeStruct judge;
                if(invoke){
                    room->broadcastSkillInvoke(objectName());

                    judge.pattern = QRegExp("(.*):(.*):(.*)");
                    judge.good = true;
                    judge.reason = objectName();
                    judge.who = player;

                    room->judge(judge);
                }

                if(invoke && judge.card->isBlack())
                    player->obtainCard(judge.card);

                QVariantList yufengList = target->tag["Yufeng"].toList();
                yufengList << (invoke && judge.card->isRed());
                target->tag["Yufeng"] = yufengList;
                target->setFlags("YufengTarget");
            }
        }
        else if(triggerEvent == SlashProceed){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.to->setFlags("-YufengTarget");
            QVariantList yufengList = effect.to->tag["Yufeng"].toList();
            if(!yufengList.isEmpty()){
                bool hit = yufengList.takeFirst().toBool();
                effect.to->tag["Yufeng"] = yufengList;
                if(hit){
                    room->slashResult(effect, NULL);
                    return true;
                }
            }
        }
        else if(triggerEvent == CardFinished){
            CardUseStruct use = data.value<CardUseStruct>();
            foreach(ServerPlayer *target, use.to){
                if(target->hasFlag("YufengTarget"))
                    target->tag.remove("Yufeng");
            }
        }
        return false;
    }
};

class Huiquan:public TriggerSkill{
public:
    Huiquan():TriggerSkill("huiquan"){
        frequency = Frequent;
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (player == NULL) return false;
        CardStar card = NULL;
        if(triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }else if(triggerEvent == CardResponded)
            card = data.value<CardResponseStruct>().m_card;

        if(card->isNDTrick()){            
            if(room->askForSkillInvoke(player, objectName())){
                room->broadcastSkillInvoke(objectName());
                player->drawCards(1);
            }
        }

        return false;
    }
};

class Liegong:public TriggerSkill{
public:
    Liegong():TriggerSkill("liegong"){
        events << TargetConfirmed << SlashProceed << CardFinished;
    }

    virtual bool trigger(TriggerEvent triggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if(!use.card->isKindOf("Slash") || use.from != player || player->getPhase() != Player::Play)
                   return false;

            foreach(ServerPlayer *target, use.to){
                int handcardnum = target->getHandcardNum();
                if(handcardnum >= player->getHp() || handcardnum <= player->getAttackRange()){
                    bool invoke = player->askForSkillInvoke("liegong", QVariant::fromValue(target));
                    QVariantList liegongList = target->tag["Liegong"].toList();
                    liegongList << invoke;
                    target->tag["Liegong"] = liegongList;
                    target->setFlags("LiegongTarget");
                }
            }
        }
        else if(triggerEvent == SlashProceed){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.to->setFlags("-LiegongTarget");
            QVariantList liegongList = effect.to->tag["Liegong"].toList();
            if(!liegongList.isEmpty()){
                bool invoke = liegongList.takeFirst().toBool();
                effect.to->tag["Liegong"] = liegongList;
                if(invoke){
                    room->broadcastSkillInvoke(objectName());
                    room->slashResult(effect, NULL);
                    return true;
                }
            }
        }
        else if(triggerEvent == CardFinished){
            CardUseStruct use = data.value<CardUseStruct>();
            foreach(ServerPlayer *target, use.to){
                if(target->hasFlag("LiegongTarget"))
                    target->tag.remove("Liegong");
            }
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

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if(triggerEvent == PreHpReduced && TriggerSkill::triggerable(damage.from)){
            damage.from->tag["InvokeKuanggu"] = damage.from->distanceTo(damage.to) <= 1;
        }else if(triggerEvent == Damage && TriggerSkill::triggerable(player)){
            bool invoke = player->tag.value("InvokeKuanggu", false).toBool();
            player->tag["InvokeKuanggu"] = false;
            if(damage.to != player && invoke) {
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

            QList<const Card *> tricks = player->getJudgingArea();
            foreach(const Card *trick, tricks)
            {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, this->objectName());
                room->throwCard(trick, reason, NULL);
            }
            room->setPlayerProperty(player, "hp", qMin(3, player->getMaxHp()));
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
               && target->getMark("qinggang") == 0;
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

class Longxi: public OneCardViewAsSkill{
public:
    Longxi():OneCardViewAsSkill("longxi"){
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

class SavageAssaultAvoid: public TriggerSkill{
public:
    SavageAssaultAvoid(const QString &avoid_skill)
        :TriggerSkill("#sa_avoid_" + avoid_skill), avoid_skill(avoid_skill)
    {
        events << CardEffected;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if(effect.card->isKindOf("SavageAssault")){
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
        if(triggerEvent == TargetConfirmed && player->hasSkill(objectName()) && player->isAlive())
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
        else if(triggerEvent == CardFinished){
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card->isKindOf("SavageAssault"))
                room->removeTag("HuoshouSource");
        }

        return false;
    }
};

class Zaiqi: public PhaseChangeSkill{
public:
    Zaiqi():PhaseChangeSkill("zaiqi"){

    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if(player->getPhase() == Player::Draw && player->isWounded()){
            Room *room = player->getRoom();
            if(room->askForSkillInvoke(player, objectName())){
                int x = player->getLostHp();

                room->broadcastSkillInvoke(objectName(), 1);
                bool has_heart = false;

                for(int i = 0; i < x; i++){
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
        events << Damage << CardUsed << CardFinished;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        DamageStruct damage = data.value<DamageStruct>();
        if(triggerEvent == CardUsed) {
            if(use.card->isKindOf("Duel"))
                player->tag["LierenDuelInvoke"] = true;

            return false;
        }

        if(triggerEvent == CardFinished) {
            if(use.card->isKindOf("Duel") && player->tag.value("LierenDuelInvoke", false).toBool())
                player->tag["LierenDuelInvoke"] = false;

            return false;
        }
        ServerPlayer *target = damage.to;
        if(damage.card && (damage.card->isKindOf("Slash") || (damage.card->isKindOf("Duel") 
                && player->tag.value("LierenDuelInvoke", false).toBool())) && !player->isKongcheng()
                && !target->isKongcheng() && target != player && !damage.chain && !damage.transfer){
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

class Shangshi: public TriggerSkill{
public:
    Shangshi():TriggerSkill("shangshi"){
        events << Death;
        frequency = NotFrequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStar damage = data.value<DamageStar>();
        ServerPlayer *killer = damage ? damage->from : NULL;
        if(killer && player->askForSkillInvoke(objectName())) {
            killer->throwAllHandCardsAndEquips();

            QString killer_name = killer->getGeneralName();
            room->broadcastSkillInvoke(objectName());
        }

        return false;
    }
};

class Mitu: public TriggerSkill{
public:
    Mitu():TriggerSkill("mitu"){
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.card && damage.card->getTypeId() == Card::TypeTrick){
            if((triggerEvent == DamageInflicted && player->hasSkill(objectName()))
                    || (triggerEvent == DamageCaused && damage.from && damage.from->isAlive() 
                    && damage.from->hasSkill(objectName()))) {
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

class Shushen: public TriggerSkill{
public:
    Shushen():TriggerSkill("shushen"){
        events << HpRecover;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        RecoverStruct &recover = data.value<RecoverStruct>();
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

class Qiaoxia:public PhaseChangeSkill{
public:
    Qiaoxia():PhaseChangeSkill("qiaoxia"){
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (player->getPhase() != Player::Start || player->isKongcheng())
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

void StandardPackage::addWindGenerals(){
    General *wind002 = new General(this, "wind002", "shu");
    wind002->addSkill(new Chilian);
    wind002->addSkill(new Zhenhong);

    General *wind003 = new General(this, "wind003", "shu");
    wind003->addSkill(new Skill("hupao"));
    wind003->addSkill(new Jiuli);

    General *wind004 = new General(this, "wind004", "shu", 3);
    wind004->addSkill(new Yuxi);
    wind004->addSkill(new Jingmo);
    wind004->addSkill(new JingmoEffect);
    related_skills.insertMulti("jingmo", "#jingmo-effect");

    General *wind005 = new General(this, "wind005", "shu", 3);
    wind005->addSkill(new Huahuan);
    wind005->addSkill(new Qizhou);

    General *wind006 = new General(this, "wind006", "shu");
    wind006->addSkill(new Jibu);
    wind006->addSkill(new Yufeng);

    General *wind007 = new General(this, "wind007", "shu", 3, false);
    wind007->addSkill(new Huiquan);
    wind007->addSkill(new Skill("jizhi", Skill::Compulsory));

    General *wind008 = new General(this, "wind008", "shu");
    wind008->addSkill(new Skill("xiagong"));
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
    wind011->addSkill(new Longxi);
    wind011->addSkill(new Jinzhou);

    General *wind014 = new General(this, "wind014", "shu");
    wind014->addSkill(new SavageAssaultAvoid("huoshou"));
    wind014->addSkill(new Huoshou);
    wind014->addSkill(new Zaiqi);
    related_skills.insertMulti("huoshou", "#sa_avoid_huoshou");

    General *wind015 = new General(this, "wind015", "shu", 4, false);
    wind015->addSkill(new SavageAssaultAvoid("jugui"));
    wind015->addSkill(new Jugui);
    related_skills.insertMulti("jugui", "#sa_avoid_jugui");
    wind015->addSkill(new Lieren);

    General *wind017 = new General(this, "wind017", "shu", 3);
    wind017->addSkill(new Xinchao);
    wind017->addSkill(new Shangshi);

    General *wind018 = new General(this, "wind018", "shu", 3);
    wind018->addSkill(new Mitu);
    wind018->addSkill(new Sishi2);

    General *wind022 = new General(this, "wind022", "shu", 3, false);
    wind022->addSkill(new Shushen);
    wind022->addSkill(new Qiaoxia);
    
    addMetaObject<XinchaoCard>();
    addMetaObject<Sishi2Card>();
}