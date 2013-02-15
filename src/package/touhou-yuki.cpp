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
        if (triggerEvent == EventPhaseChanging && TriggerSkill::triggerable(player))
        {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.who->hasFlag("jianmoinvoke"))
                room->setPlayerFlag(change.who, "-jianmoinvoke");
            room->removePlayerCardLimitation(change.who, "use,response", "Slash@0");
        }
        else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play && TriggerSkill::triggerable(player))
        {
            ServerPlayer *srcplayer = data.value<PlayerStar>();
            if (srcplayer == player)
                return false;

            if (srcplayer->getHandcardNum() >= srcplayer->getMaxHp() && player->askForSkillInvoke(objectName()))
                if (room->askForChoice(srcplayer, objectName(), "jian+mo") == "jian")
                {
                    LogMessage log;
                    log.type = "#thjianmochoose1";
                    log.from = srcplayer;
                    log.arg = "1";
                    room->sendLog(log);
                    player->drawCards(1);
                    room->setPlayerCardLimitation(srcplayer, "use,response", "Slash", false);
                }
                else
                {
                    LogMessage log;
                    log.type = "#thjianmochoose2";
                    log.from = srcplayer;
                    log.arg = "2";
                    room->sendLog(log);
                    room->setPlayerFlag(srcplayer,"jianmoinvoke");
                }
        }
        else if (triggerEvent == CardUsed && TriggerSkill::triggerable(player))
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->hasFlag("jianmoavoid"))
                use.card->setFlags("-jianmoavoid");
            if (use.card->isKindOf("Slash") && use.from->hasFlag("jianmoinvoke"))
            {
                LogMessage log;
                log.type = "#ThJianmo";
                log.from = use.from;
                log.arg = objectName();
                log.arg2 = use.card->objectName();
                room->sendLog(log);

                if (!room->askForCard(use.from, "..", "@thjianmo"))
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
        if (data.value<PlayerStar>() != player)
            return false;

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
        room->acquireSkill(player, "thhuanfa");
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
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        const Card *trigger_card;
        ServerPlayer *trigger_player = NULL;
        if (triggerEvent == CardUsed && TriggerSkill::triggerable(player))
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.from || use.from->getKingdom() != "wu")
                return false;
            trigger_card = use.card;
            trigger_player = use.from;
        }
        else if (triggerEvent == CardResponded)
        {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (player->getKingdom() != "wu" || !resp.m_isUse)
                return false;
            trigger_card = resp.m_card;
            trigger_player = player;
        }
        else
            return false;

        if (trigger_card == NULL)
            return false;

        if (trigger_card->isKindOf("BasicCard") && trigger_card->getSuit() == Card::Heart)
        {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(trigger_player))
            {
                if (p->hasLordSkill(objectName()))
                    targets << p;
            }
            
            while (!targets.isEmpty())
            {
                if (trigger_player->askForSkillInvoke(objectName()))
                {
                    ServerPlayer *target = room->askForPlayerChosen(trigger_player, targets, objectName());
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
        if (triggerEvent == EventPhaseStart && data.value<PlayerStar>()
            && player->getPhase() == Player::Start)
        {
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
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == player && use.card->isKindOf("Analeptic"))
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
                if (x != 0)
                    if (player->getCardCount(true) < x)
                        player->throwAllHandCardsAndEquips();
                    else
                        room->askForDiscard(player, objectName(), x, x, false, true);
            }
            else
            {
                int x = qMin(effect.to->getHpPoints(), 5);
                if (x != 0)
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

ThYuanqiCard::ThYuanqiCard(){
    target_fixed = true;
}

void ThYuanqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    JudgeStruct judge;
    judge.pattern = QRegExp("(.*):(.*):(.*)");
    judge.good = true;
    judge.reason = objectName();
    judge.who = source;
    room->judge(judge);

    QList<int> card_ids;
    foreach (int card_id, source->handCards())
        if (Sanguosha->getCard(card_id)->sameColorWith(judge.card))
            card_ids << card_id;

    if (card_ids.isEmpty())
        return ;

    room->fillAG(card_ids, source);
    int card_id = room->askForAG(source, card_ids, true, "thyuanqi");
    source->invoke("clearAG");

    if (card_id == -1)
        return ;

    ServerPlayer *target = room->askForPlayerChosen(source, room->getOtherPlayers(source), "thyuanqi");
    room->obtainCard(target, card_id);

    QString choice = "draw";
    if (!target->isNude())
        choice = room->askForChoice(target, "thyuanqi", "draw+throw");

    if (choice == "draw")
    {
        target->drawCards(1);
        room->loseHp(target);
    }
    else
    {
        room->throwCard(card_id, target);
        int id = room->askForCardChosen(source, target, "he", "thyuanqi");
        room->obtainCard(source, id, room->getCardPlace(id) != Player::PlaceHand);
    }
}

class ThYuanqi: public ZeroCardViewAsSkill {
public:
    ThYuanqi(): ZeroCardViewAsSkill("thyuanqi") {
    }

    virtual const Card *viewAs() const{
        return new ThYuanqiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThYuanqiCard");
    }
};

class ThMoji: public TriggerSkill {
public:
    ThMoji(): TriggerSkill("thmoji") {
        events << EventPhaseStart << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        bool invoke = triggerEvent == EventPhaseStart && data.value<PlayerStar>() == player
                      && player->getHandcardNum() < 2;
        if (triggerEvent == CardsMoveOneTime)
        {
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            invoke = move->from == player && move->from_places.contains(Player::PlaceHand) && player->getHandcardNum() < 2;
        }
        if (invoke && player->getPhase() == Player::NotActive)
        {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            
            player->drawCards(2 - player->getHandcardNum());
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

ThChouceCard::ThChouceCard(){
    will_throw = false;
}

bool ThChouceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    const Card *card = Sanguosha->getCard(getSubcards().first());
    if (!card)
        return false;

    return targets.isEmpty() && !Self->isProhibited(to_select, card);
}

bool ThChouceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const {
    return targets.length() == 1;
}

const Card *ThChouceCard::validate(const CardUseStruct *card_use) const{
    const Card *card = Sanguosha->getCard(getSubcards().first());
    Card *use_card = Sanguosha->cloneCard(card->objectName(), card->getSuit(), card->getNumber());
    use_card->addSubcard(card);
    use_card->setSkillName("thchouce");
    Room *room = card_use->from->getRoom();
    
    room->setPlayerMark(card_use->from, "ThChouce", use_card->getNumber());
    card_use->from->addMark("choucecount");

    return use_card;
}

class ThChouceViewAsSkill: public OneCardViewAsSkill {
public:
    ThChouceViewAsSkill(): OneCardViewAsSkill("thchouce") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return true;
    }

    virtual bool viewFilter(const Card* to_select) const{
        if (to_select->getNumber() <= Self->getMark("ThChouce"))
            return false;

        if (to_select->isKindOf("Slash"))
            return Slash::IsAvailable(Self, to_select);
        else if (to_select->isKindOf("Analeptic"))
            return Analeptic::IsAvailable(Self, to_select);
        else
            return !to_select->isKindOf("AmazingGrace")
                && !to_select->isKindOf("EquipCard")
                && !to_select->isKindOf("Jink")
                && !to_select->isKindOf("Nullification")
                && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThChouceCard *card = new ThChouceCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThChouce: public TriggerSkill {
public:
    ThChouce(): TriggerSkill("thchouce"){
        events << CardUsed << CardResponded << EventPhaseStart;
        view_as_skill = new ThChouceViewAsSkill;
    }

    virtual int getPriority() const{
        return 5;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == EventPhaseStart && data.value<PlayerStar>() == player){
            if(player->getPhase() == Player::RoundStart){
                room->setPlayerMark(player, "ThChouce", 0);
                room->setPlayerMark(player, "choucecount", 0);
                room->setPlayerFlag(player, "-ThChouce_failed");
                return false;
            }
        }
        else if (triggerEvent == CardUsed || triggerEvent == CardResponded)
        {
            if(player->getPhase() != Player::Play)
                return false;
            const Card *usecard = NULL;
            if (triggerEvent == CardUsed)
            {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.from != player)
                    return false;
                usecard = use.card;
            }
            else if (triggerEvent == CardResponded)
            {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    usecard = resp.m_card;
                else
                    usecard = NULL;
            }
            if (!usecard || usecard->getTypeId() == Card::TypeSkill || usecard->getSkillName() == objectName())
                return false;

            if (usecard->isKindOf("Jink") || usecard->isKindOf("Nullification"))
            {
                room->setPlayerFlag(player, "ThChouce_failed");

                if (usecard->getNumber() == 0)
                    room->setPlayerMark(player, "ThChouce", 13);
                else
                    room->setPlayerMark(player, "ThChouce", usecard->getNumber());

                return false;
            }
            
            CardUseStruct use = data.value<CardUseStruct>();    
            int precardnum = player->getMark("ThChouce"); //the cardnumber store of thchouce
            if(!player->hasFlag("ThChouce_failed") && usecard->getNumber() > precardnum)
            {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (!player->isProhibited(p, use.card))
                        targets << p;

                if (!targets.isEmpty() && player->askForSkillInvoke(objectName(), data))
                {
                    ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName());
                    use.to.clear();
                    use.to << target;
                    
                    LogMessage log;
                    log.type = "$ThChouce";
                    log.from = player;
                    log.to = use.to;
                    log.arg = objectName();
                    log.card_str = usecard->getEffectIdString();
                    room->sendLog(log);
                    player->addMark("choucecount"); //the count of thchouce
                }
            }
            else
                room->setPlayerFlag(player, "ThChouce_failed");

            if (usecard->getNumber() == 0)
                room->setPlayerMark(player, "ThChouce", 13);
            else
                room->setPlayerMark(player, "ThChouce", usecard->getNumber());
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class ThZhanshi: public TriggerSkill{
public:
    ThZhanshi(): TriggerSkill("thzhanshi"){
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
               && target->getPhase() == Player::NotActive
               && !target->hasFlag("ThChouce_failed");
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *target, QVariant &data) const {
        if (data.value<PlayerStar>() != target)
            return false;

        if(target->getMark("choucecount") >= 3){
            target->setMark("choucecount", 0);
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = target;
            log.arg = objectName();
            target->getRoom()->sendLog(log);
            target->gainMark("@tianji");
            target->gainAnExtraTurn(target);
        }
        return false;
    }
};

class ThHuanzang: public TriggerSkill{
public:
    ThHuanzang(): TriggerSkill("thhuanzang"){
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("@tianji") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() != player)
            return false;

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        
        player->loseMark("@tianji");

        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(spade):(.*)");
        judge.good = false;
        judge.reason = objectName();
        judge.who = player;
    
        room->judge(judge);

        if (judge.isBad())
            room->loseHp(player);

        return false;
    }
};

class ThZiyun: public ProhibitSkill {
public:
    ThZiyun(): ProhibitSkill("thziyun") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const {
        if (to->hasSkill(objectName()))
            return card->isKindOf("SupplyShortage") || card->isKindOf("Lightning");
        
        return false;
    }
};

class ThChuiji: public TriggerSkill {
public:
    ThChuiji(): TriggerSkill("thchuiji") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
        if (move->from == player && (move->from_places.contains(Player::PlaceHand) || move->from_places.contains(Player::PlaceEquip)))
        {
            bool invoke = false;
            if (player->isWounded())
                invoke = true;
            else
                foreach (ServerPlayer *p, room->getOtherPlayers(player))
                    if (!p->isNude() || p->isWounded())
                    {
                        invoke = true;
                        break;
                    }

            if (!invoke)
                return false;

            if (player->getPhase() == Player::NotActive && player->askForSkillInvoke(objectName()))
            {
                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(.*):(.*)");
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;
                room->judge(judge);

                QList<ServerPlayer *> targets;
                if (judge.card->isRed())
                {
                    foreach (ServerPlayer *p, room->getAlivePlayers())
                        if (p->isWounded())
                            targets << p;

                    if (targets.isEmpty())
                        return false;

                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                    RecoverStruct recover;
                    recover.who = player;
                    room->recover(target, recover);
                }
                else if (judge.card->isBlack())
                {
                    foreach (ServerPlayer *p, room->getOtherPlayers(player))
                        if (!p->isNude())
                            targets << p;

                    if (targets.isEmpty())
                        return false;

                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                    room->askForDiscard(target, objectName(), 1, 1, false, true);
                }
            }
        }

        return false;
    }
};

class ThLingya: public TriggerSkill {
public:
    ThLingya(): TriggerSkill("thlingya") {
        events << CardFinished;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *current = room->getCurrent();
        CardUseStruct use = data.value<CardUseStruct>();
        if (current->hasSkill(objectName()) && current != player && use.card->isRed()
            && current->askForSkillInvoke(objectName()))
        {
            QStringList choices;
            choices << "letdraw";
            if (!player->isNude())
                choices << "discard";

            QString choice = room->askForChoice(player, objectName(), choices.join("+"));
            if (choice == "discard")
            {
                int card_id = room->askForCardChosen(current, player, "he", objectName());
                room->throwCard(card_id, player, current);
            }
            else
                current->drawCards(1);
        }

        return false;
    }
};

class ThHeimu: public TriggerSkill {
public:
    ThHeimu(): TriggerSkill("thheimu") {
        events << CardUsed << EventPhaseChanging;
    }

    virtual int getPriority() const{
        return 5;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardUsed && player->getMark(objectName()) == 0)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player && player->getPhase() == Player::Play && use.m_reason == CardUseStruct::CARD_USE_REASON_PLAY
                && use.card->getTypeId() != Card::TypeSkill && player->askForSkillInvoke(objectName()))
            {
                room->setPlayerMark(player, objectName(), 1);
                const CardMoveReason &reason = CardMoveReason(CardMoveReason::S_REASON_PUT, player->objectName());
                room->moveCardTo(use.card, NULL, Player::DiscardPile, reason, true);
                QString key = use.card->getClassName();
                player->addHistory(key, -1);
                player->invoke("addHistory", key + ":-1");
                if (use.to.isEmpty())
                    use.to << use.from;

                ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
                use.from = target;
                room->useCard(use);

                return true;
            }
        }
        else if (triggerEvent == EventPhaseChanging && data.value<PhaseChangeStruct>().who == player)
            room->setPlayerMark(player, objectName(), 0);

        return false;
    }
};

class ThHanpo: public TriggerSkill {
public:
    ThHanpo(): TriggerSkill("thhanpo") {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused && (damage.to == player || damage.nature != DamageStruct::Fire))
            return false;
        else if (triggerEvent == DamageInflicted && damage.nature != DamageStruct::Fire)
            return false;

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        return true;
    }
};

class ThZhengguan: public TriggerSkill {
public:
    ThZhengguan(): TriggerSkill("thzhengguan") {
        events << PhaseSkipped;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        Player::Phase phase = (Player::Phase)data.toInt();
        if (phase == Player::Play || phase == Player::Draw)
        {
            ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
            if (!splayer || player == splayer)
                return false;

            QString phase_str = (phase == Player::Play) ? "play" : "draw";
            if (room->askForCard(splayer, ".|.|.|.|red", "@zhengguan:::" + phase_str, data, objectName()))
            {
                LogMessage log;
                log.type = "#ThZhengguan";
                log.from = splayer;
                log.arg = objectName();
                log.arg2 = phase_str;
                room->sendLog(log);

                QList<Player::Phase> phases;
                phases << phase;
                splayer->play(phases);
            }
        }

        return false;
    }
};

ThBingpuCard::ThBingpuCard(){
    target_fixed = true;
}

void ThBingpuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    source->loseMark("@bingpu");
    source->gainMark("@bingpuused");
    foreach(ServerPlayer *target, room->getOtherPlayers(source))
    {
        if (target->isNude())
            continue;

        if (!room->askForCard(target, "jink", "@thbingpu:" + source->objectName(), QVariant(), Card::MethodResponse))
        {
            int card_id = room->askForCardChosen(source, target, "he", objectName());
            room->throwCard(Sanguosha->getCard(card_id), target, source);

            if(!target->isNude()){
                card_id = room->askForCardChosen(source, target, "he", objectName());
                room->throwCard(Sanguosha->getCard(card_id), target, source);
            }
        }
    }
}

class ThBingpu: public ZeroCardViewAsSkill {
public:
    ThBingpu(): ZeroCardViewAsSkill("thbingpu") {
        frequency = Limited;
    }

    virtual const Card *viewAs() const{
        return new ThBingpuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@bingpu") > 0;
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

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() == player && player->getPhase() == Player::Finish
            && player->isWounded() && player->askForSkillInvoke(objectName()))
            room->askForUseCard(player, "@@thdongmo", "@thdongmo");

        return false;
    }
};

class ThLinhan:public TriggerSkill{
public:
    ThLinhan():TriggerSkill("thlinhan"){
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

        const Card *card = room->askForCard(player, "slash", "@thqiebao", QVariant(), Card::MethodResponse, NULL, false, objectName());

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
        if (triggerEvent == CardUsed && TriggerSkill::triggerable(player))
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->hasFlag("qiebaoinvoke"))
                use.card->setFlags("-qiebaoinvoke");
            
            if (use.to.isEmpty())
                return false;

            if (use.to.contains(use.from))
            {
                QList<ServerPlayer *> targets = use.to;
                targets.removeAll(use.from);
                if (targets.isEmpty())
                    return false;
            }

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

class ThFusheng: public TriggerSkill {
public:
    ThFusheng(): TriggerSkill("thfusheng") {
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == HpRecover){
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.who && recover.who != player) {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                recover.who->drawCards(recover.recover);
                room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
            }
        } else if(triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to != player)
                return false;
            ServerPlayer *source = damage.from;
            if (source && source != player) {
                room->broadcastSkillInvoke(objectName(), qrand() % 2 + 3);

                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                const Card *card = room->askForCard(source, ".H", "@thfusheng:" + player->objectName(), QVariant(), Card::MethodNone);
                if (card) player->obtainCard(card); else room->loseHp(source);
            }
        }

        return false;
    }
};

ThHuanfaCard::ThHuanfaCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
    mute = true;
}

void ThHuanfaCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(this);

    Room *room = effect.from->getRoom();
    room->broadcastSkillInvoke(objectName());
    int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thhuanfa");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "thhuanfa");
    if (target != effect.from) {
        CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, effect.from->objectName());
        reason2.m_playerId = target->objectName();
        room->obtainCard(target, Sanguosha->getCard(card_id), reason2, false);
    }
}

class ThHuanfa: public OneCardViewAsSkill {
public:
    ThHuanfa():OneCardViewAsSkill("thhuanfa") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThHuanfaCard");
    }

    virtual bool viewFilter(const Card *to_select) const{
        return !to_select->isEquipped() && to_select->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThHuanfaCard *huanfaCard = new ThHuanfaCard;
        huanfaCard->addSubcard(originalCard);
        return huanfaCard;
    }
};

class ThLingta:public TriggerSkill{
public:
    ThLingta():TriggerSkill("thlingta"){
        events << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.who != player)
            return false;

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

        if (change.who != player)
            return false;
        
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
        attached_lord_skill = true;
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
    
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *splayer, QVariant &data) const{
        ServerPlayer *player = data.value<PlayerStar>();
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

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(player))
            room->setPlayerMark(player, objectName(), 0);
        else if (triggerEvent == PreHpReduced)
        {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
            if (!splayer)
                return false;

            if (damage.damage >= 2 && damage.to != splayer && splayer->getMark(objectName()) < 1 && splayer->askForSkillInvoke(objectName()))
            {
                player->addMark(objectName());
                damage.transfer = true;
                damage.to = splayer;
                room->damage(damage);
                return true;
            }
        }

        return false;
    }
};

class ThMingling: public TriggerSkill {
public:
    ThMingling(): TriggerSkill("thmingling") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Normal)
        {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            if (damage.nature == DamageStruct::Fire)
                return true;
            else if (damage.nature == DamageStruct::Thunder)
                damage.damage++;

            data = QVariant::fromValue(damage);
        }
        
        return false;
    }
};

ThChuanshangCard::ThChuanshangCard(){
}

bool ThChuanshangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && Self->inMyAttackRange(to_select) && to_select != Self;
}

void ThChuanshangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    targets[0]->gainMark("@nishui");
    room->loseHp(source);
}

class ThChuanshangViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThChuanshangViewAsSkill(): ZeroCardViewAsSkill("thchuanshang") {
    }

    virtual const Card *viewAs() const{
        return new ThChuanshangCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThChuanshangCard");
    }
};

class ThChuanshang: public TriggerSkill {
public:
    ThChuanshang(): TriggerSkill("thchuanshang") {
        events << EventPhaseStart;
        view_as_skill = new ThChuanshangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && target->getPhase() == Player::Finish && target->getMark("@nishui") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (data.value<PlayerStar>() != player)
            return false;

        LogMessage log;
        log.type = "#ThChuanshang";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        
        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(heart):(.*)");
        judge.good = true;
        judge.reason = objectName();
        judge.who = player;

        room->judge(judge);
        
        if (judge.isGood())
            player->loseAllMarks("@nishui");
        else if (judge.card->isBlack())
            player->gainMark("@nishui");

        return false;
    }
};

class ThChuanshangMaxCardsSkill: public MaxCardsSkill {
public:
    ThChuanshangMaxCardsSkill(): MaxCardsSkill("#thchuanshang") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->getMark("@nishui") > 0)
            return -target->getMark("@nishui");
        else
            return 0;
    }
};

ThLingdieCard::ThLingdieCard(){
}

bool ThLingdieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty();
}

void ThLingdieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets[0];
    ServerPlayer *victim = room->askForPlayerChosen(target, room->getOtherPlayers(target), "thlingdie");
    room->showAllCards(victim, target);
}

class ThLingdie: public OneCardViewAsSkill {
public:
    ThLingdie(): OneCardViewAsSkill("thlingdie") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThLingdieCard");
    }
    
    virtual bool viewFilter(const Card* to_select) const{
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThLingdieCard *card = new ThLingdieCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThWushou: public TriggerSkill {
public:
    ThWushou(): TriggerSkill("thwushou") {
        events << DamageInflicted;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getHandcardNum() < 4 && player->askForSkillInvoke(objectName()))
            player->drawCards(4 - player->getHandcardNum());
        return false;
    }
};

ThFuyueCard::ThFuyueCard() {
    m_skillName = "thfuyuev";
    mute = true;
    will_throw = false;
    handling_method = MethodPindian;
}

void ThFuyueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    if(target->hasLordSkill("thfuyue")){
        room->setPlayerFlag(target, "ThFuyueInvoked");
        bool win = source->pindian(target, "thfuyuev", this);
        if (!win)
        {
            RecoverStruct recover;
            recover.who = source;
            room->recover(target, recover);
        }
        QList<ServerPlayer *> lords;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach(ServerPlayer *p, players){
            if(p->hasLordSkill("thfuyue") && !p->hasFlag("ThFuyueInvoked")){
                lords << p;
            }
        }
        if(lords.empty())
            room->setPlayerFlag(source, "ForbidThFuyue");
    }
}

bool ThFuyueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("thfuyue") && to_select->isWounded() && !to_select->isKongcheng()
            && to_select != Self && !to_select->hasFlag("ThFuyueInvoked");
}

class ThFuyueViewAsSkill: public OneCardViewAsSkill {
public:
    ThFuyueViewAsSkill(): OneCardViewAsSkill("thfuyuev"){
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getKingdom() == "wu" && !player->hasFlag("ForbidThFuyue");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThFuyueCard *card = new ThFuyueCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThFuyue: public TriggerSkill{
public:
    ThFuyue(): TriggerSkill("thfuyue$"){
        events << EventPhaseStart << EventPhaseEnd << EventPhaseChanging;
    }
    
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        ServerPlayer *player = NULL;
        if (triggerEvent != EventPhaseChanging)
            player = data.value<PlayerStar>();
        else
            player = data.value<PhaseChangeStruct>().who;
        
        if (player == NULL)
            return false;

        if (triggerEvent == EventPhaseEnd && player->hasSkill("thfuyuev"))
            room->detachSkillFromPlayer(player, "thfuyuev", true);
        else if (triggerEvent == EventPhaseStart)
        {
            bool can_invoke = false;
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if (p->hasLordSkill("thfuyue"))
                {
                    can_invoke = true;
                    break;
                }
            if (can_invoke && player->getPhase() == Player::Play && !player->hasSkill("thfuyuev") && player->getKingdom() == "wu")
                room->attachSkillToPlayer(player, "thfuyuev");
        }
        else if (triggerEvent == EventPhaseChanging)
        {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                  return false;
            if (player->hasFlag("ForbidThFuyue")) {
                room->setPlayerFlag(player, "-ForbidThFuyue");
            }
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach(ServerPlayer *p, players){
                if(p->hasFlag("ThFuyueInvoked")){
                    room->setPlayerFlag(p, "-ThFuyueInvoked");
                }
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
    
    General *yuki005 = new General(this, "yuki005", "wu", 3, false);
    yuki005->addSkill(new ThYuanqi);
    yuki005->addSkill(new ThMoji);

    General *yuki006 = new General(this, "yuki006", "wu");
    yuki006->addSkill("jibu");
    yuki006->addSkill(new ThDunjia);

    General *yuki007 = new General(this, "yuki007", "wu", 3);
    yuki007->addSkill(new ThChouce);
    yuki007->addSkill(new ThZhanshi);
    yuki007->addSkill(new ThHuanzang);

    General *yuki008 = new General(this, "yuki008", "wu", 3);
    yuki008->addSkill(new ThZiyun);
    yuki008->addSkill(new ThChuiji);

    General *yuki009 = new General(this, "yuki009", "wu");
    yuki009->addSkill(new ThLingya);
    yuki009->addSkill(new ThHeimu);

    General *yuki010 = new General(this, "yuki010", "wu");
    yuki010->addSkill(new ThHanpo);
    yuki010->addSkill(new ThZhengguan);
    yuki010->addSkill(new ThBingpu);
    yuki010->addSkill(new MarkAssignSkill("@bingpu", 1));
    related_skills.insertMulti("thbingpu", "#@bingpu-1");
    
    General *yuki011 = new General(this, "yuki011", "wu", 3);
    yuki011->addSkill(new ThDongmo);
    yuki011->addSkill(new ThLinhan);

    General *yuki012 = new General(this, "yuki012", "wu", 3);
    yuki012->addSkill(new ThFusheng);
    yuki012->addSkill(new ThHuanfa);

    General *yuki014 = new General(this, "yuki014", "wu");
    yuki014->addSkill(new ThQiebao);

    General *yuki015 = new General(this, "yuki015", "wu");
    yuki015->addSkill(new ThLingta);
    yuki015->addSkill(new ThWeiguang);
    yuki015->addSkill(new ThChuhui);

    General *yuki016 = new General(this, "yuki016", "wu");
    yuki016->addSkill(new ThKujie);
    yuki016->addSkill(new ThYinbi);

    General *yuki017 = new General(this, "yuki017", "wu");
    yuki017->addSkill(new ThMingling);
    yuki017->addSkill(new ThChuanshang);
    yuki017->addSkill(new ThChuanshangMaxCardsSkill);
    related_skills.insertMulti("thchuanshang", "#thchuanshang");

    General *yuki018 = new General(this, "yuki018$", "wu", 3, false);
    yuki018->addSkill(new ThLingdie);
    yuki018->addSkill(new ThWushou);
    yuki018->addSkill(new ThFuyue);

    addMetaObject<ThYuanqiCard>();
    addMetaObject<ThChouceCard>();
    addMetaObject<ThBingpuCard>();
    addMetaObject<ThDongmoCard>();
    addMetaObject<ThHuanfaCard>();
    addMetaObject<ThKujieCard>();
    addMetaObject<ThChuanshangCard>();
    addMetaObject<ThLingdieCard>();
    addMetaObject<ThFuyueCard>();

    skills << new ThKujieViewAsSkill << new ThFuyueViewAsSkill;
}

