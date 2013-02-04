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

class Wushuang: public TriggerSkill {
public:
    Wushuang(): TriggerSkill("wushuang") {
        events << TargetConfirmed << CardFinished;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if (event == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            bool can_invoke = false;
            if (use.card->isKindOf("Slash") && TriggerSkill::triggerable(use.from) && use.from == player){
                can_invoke = true;
                int count = 1;
                int mark_n = player->getMark("double_jink" + use.card->getEffectIdString());
                for (int i = 0; i < use.to.length(); i++) {
                    mark_n += count;
                    room->setPlayerMark(player, "double_jink" + use.card->getEffectIdString(), mark_n);
                    count *= 10;
                }
            }
            if (use.card->isKindOf("Duel")) {
                if (TriggerSkill::triggerable(use.from) && use.from == player)
                    can_invoke = true;
                if (TriggerSkill::triggerable(player) && use.to.contains(player))
                    can_invoke = true;
            }
            if (!can_invoke) return false;

            LogMessage log;
            log.from = player;
            log.arg = objectName();
            log.type = "#TriggerSkill";
            room->sendLog(log);

            room->broadcastSkillInvoke(objectName());
            if (use.card->isKindOf("Duel"))
                room->setPlayerMark(player, "WushuangTarget", 1);
        } else if (event == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                if (player->hasSkill(objectName()))
                    room->setPlayerMark(player, "double_jink" + use.card->getEffectIdString(), 0);
            } else if (use.card->isKindOf("Duel")) {
                foreach(ServerPlayer *lvbu, room->getAllPlayers())
                    if (lvbu->getMark("WushuangTarget") > 0)
                        room->setPlayerMark(lvbu, "WushuangTarget", 0);
            }
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
			const Card *second = cards.last();
            Card::Suit suit = first->isRed() ? Card::NoSuitRed : Card::NoSuitBlack;
            int number = first->getNumber() == second->getNumber() ? first->getNumber() : 0;
			Duel *aa = new Duel(suit, number);
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

    Duel *duel = new Duel(Card::NoSuitNoColor, 0);
    if(targets.isEmpty() && Self->isProhibited(to_select, duel)){
        duel->deleteLater();
        return false;
    }
    if (targets.length() == 1 && to_select->isCardLimited(duel, Card::MethodUse)) {
        duel->deleteLater();
        return false;
    }
    duel->deleteLater();

    return targets.length() < 2 && to_select != Self;
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

    Duel *duel = new Duel(Card::NoSuitNoColor, 0);
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
			const Card *second = cards.last();
            Card::Suit suit = first->isRed() ? Card::NoSuitRed : Card::NoSuitBlack;
            int number = first->getNumber() == second->getNumber() ? first->getNumber() : 0;
			ArcheryAttack *aa = new ArcheryAttack(suit, number);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        }else
            return NULL;
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

class Fusheng: public OneCardViewAsSkill {
public:
    Fusheng(): OneCardViewAsSkill("fusheng") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.contains("analeptic");
    }

    virtual bool viewFilter(const Card *to_select) const{
        return !to_select->isEquipped() && to_select->getSuit() == Card::Spade;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());

        if (!Analeptic::IsAvailable(Self, analeptic)) return NULL;
        return analeptic;
    }
};

class Huanbei: public TriggerSkill {
public:
    Huanbei(): TriggerSkill("huanbei") {
        events << TargetConfirmed << CardFinished;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && (target->hasSkill(objectName()) || target->isFemale());
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        if (event == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && player == use.from) {
                int mark_n = player->getMark("double_jink" + use.card->toString());
                int count = 0;
                bool play_effect = false;
                if (TriggerSkill::triggerable(use.from)) {
                    count = 1;
                    foreach (ServerPlayer *p, use.to) {
                        if (p->isFemale()) {
                            play_effect = true;
                            mark_n += count;
                            room->setPlayerMark(use.from, "double_jink" + use.card->toString(), mark_n);
                        }
                        count *= 10;
                    }
                    if (play_effect) {
                        LogMessage log;
                        log.from = use.from;
                        log.arg = objectName();
                        log.type = "#TriggerSkill";
                        room->sendLog(log);

                        room->broadcastSkillInvoke(objectName(), 1);
                    }
                } else if (use.from->isFemale()) {
                    count = 1;
                    foreach (ServerPlayer *p, use.to) {
                        if (p->hasSkill(objectName())) {
                            play_effect = true;
                            mark_n += count;
                            room->setPlayerMark(use.from, "double_jink" + use.card->toString(), mark_n);
                        }
                        count *= 10;
                    }
                    if (play_effect) {
                        foreach (ServerPlayer *p, use.to) {
                            if (p->hasSkill(objectName())) {
                                LogMessage log;
                                log.from = p;
                                log.arg = objectName();
                                log.type = "#TriggerSkill";
                                room->sendLog(log);
                            }
                        }
                        room->broadcastSkillInvoke(objectName(), 2);
                    }
                }
            }
        } else if(event == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                room->setPlayerMark(use.from, "double_jink" + use.card->toString(), 0);
        }

        return false;
    }
};

class Benghuai: public PhaseChangeSkill {
public:
    Benghuai(): PhaseChangeSkill("benghuai") {
        frequency = Compulsory;
    }

    virtual bool onPhaseChange(ServerPlayer *dongzhuo) const{
        bool trigger_this = false;
        Room *room = dongzhuo->getRoom();

        if (dongzhuo->getPhase() == Player::Finish) {
            QList<ServerPlayer *> players = room->getOtherPlayers(dongzhuo);
            foreach (ServerPlayer *player, players) {
                if (dongzhuo->getHp() > player->getHp()) {
                    trigger_this = true;
                    break;
                }
            }
        }

        if (trigger_this) {
            QString result = room->askForChoice(dongzhuo, "benghuai", "hp+maxhp");

            int index = (result == "hp") ? 1 : 2;
            room->broadcastSkillInvoke(objectName(), index);

            LogMessage log;
            log.from = dongzhuo;
            log.arg = objectName();
            log.type = "#TriggerSkill";
            room->sendLog(log);
            if (result == "hp")
                room->loseHp(dongzhuo);
            else
                room->loseMaxHp(dongzhuo);
        }

        return false;
    }
};

class Wuhua: public TriggerSkill {
public:
    Wuhua(): TriggerSkill("wuhua$") {
        events << Damage << DamageDone;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (event == DamageDone && damage.from)
            damage.from->tag["InvokeWuhua"] = damage.from->getKingdom() == "qun";
        else if (event == Damage && player->tag.value("InvokeWuhua", false).toBool() && player->isAlive()) {
            QList<ServerPlayer *> dongzhuos;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill(objectName()))
                    dongzhuos << p;
            }

            while (!dongzhuos.isEmpty()) {
                if (player->askForSkillInvoke(objectName())) {
                    ServerPlayer *dongzhuo = room->askForPlayerChosen(player, dongzhuos, objectName());
                    dongzhuos.removeOne(dongzhuo);

                    JudgeStruct judge;
                    judge.pattern = QRegExp("(.*):(spade):(.*)");
                    judge.good = true;
                    judge.reason = objectName();
                    judge.who = player;

                    room->judge(judge);

                    if (judge.isGood()) {
                        room->broadcastSkillInvoke(objectName());

                        RecoverStruct recover;
                        recover.who = player;
                        room->recover(dongzhuo, recover);
                    }
                } else
                    break;
            }
        }
        return false;
    }
};

class Sishi: public TriggerSkill{
public:
    Sishi():TriggerSkill("sishi"){
        events << PreAskForPeaches;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        ServerPlayer *jiaxu = target->getRoom()->getCurrent();
        return jiaxu && jiaxu->hasSkill(objectName()) && jiaxu->isAlive() && target->getHp() <= 0;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *jiaxu = room->getCurrent();
        if (jiaxu->hasInnateSkill("sishi") || !jiaxu->hasSkill("jilve"))
            room->broadcastSkillInvoke(objectName());
        else
            room->broadcastSkillInvoke("jilve", 3);

        QList<ServerPlayer *> savers;
        savers << jiaxu;

        LogMessage log;
        log.from = jiaxu;
        log.arg = objectName();
        if (jiaxu != player) {
            savers << player;
            log.type = "#SishiTwo";
            log.to << player;
        } else {
            log.type = "#SishiOne";
        }
        room->sendLog(log);
        dying.savers = savers;

        data = QVariant::fromValue(dying);

        return false;
    }
};

WenleCard::WenleCard(){
    target_fixed = true;
}

void WenleCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->loseMark("@wenle");
    source->gainMark("@wenleused");
    room->broadcastInvoke("animate", "lightbox:$wenle");

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach(ServerPlayer *player, players){
        if(player->isAlive())
            room->cardEffect(this, source, player);
    }
}

void WenleCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    QList<ServerPlayer *> players = room->getOtherPlayers(effect.to);
    QList<int> distance_list;
    int nearest = 1000;
    foreach(ServerPlayer *player, players){
        int distance = effect.to->distanceTo(player);
        distance_list << distance;

        nearest = qMin(nearest, distance);
    }

    QList<ServerPlayer *> wenle_targets;
    for(int i = 0; i < distance_list.length(); i++){
        if(distance_list.at(i) == nearest && effect.to->canSlash(players.at(i))){
            wenle_targets << players.at(i);
        }
    }

    if(wenle_targets.isEmpty() || !room->askForUseSlashTo(effect.to, wenle_targets, "@wenle-slash"))
           room->loseHp(effect.to);
}

class Wenle: public ZeroCardViewAsSkill{
public:
    Wenle():ZeroCardViewAsSkill("wenle"){
        frequency = Limited;
    }

    virtual const Card *viewAs() const{
        return new WenleCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@wenle") >= 1;
    }
};

class Moyu2: public ProhibitSkill{
public:
    Moyu2():ProhibitSkill("moyu2"){

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        return to->hasSkill(objectName()) && card->isKindOf("TrickCard") && card->isBlack();
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

LeijiCard::LeijiCard(){

}

bool LeijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void LeijiCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *player = effect.from;
    ServerPlayer *target = effect.to;

    Room *room = player->getRoom();

    JudgeStruct judge;
    judge.pattern = QRegExp("(.*):(spade):(.*)");
    judge.good = false;
    judge.reason = "leiji";
    judge.who = target;
    judge.play_animation = true;
    judge.negative = true;

    room->judge(judge);

    if(judge.isBad()){
        DamageStruct damage;
        damage.card = NULL;
        damage.damage = 2;
        damage.from = player;
        damage.to = target;
        damage.nature = DamageStruct::Thunder;

        room->damage(damage);
    }else
        room->setEmotion(player, "bad");
}

class LeijiViewAsSkill: public ZeroCardViewAsSkill{
public:
    LeijiViewAsSkill():ZeroCardViewAsSkill("leiji"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@leiji";
    }

    virtual const Card *viewAs() const{
        return new LeijiCard;
    }
};

class Leiji: public TriggerSkill{
public:
    Leiji():TriggerSkill("leiji"){
        events << CardResponded;
        view_as_skill = new LeijiViewAsSkill;
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (player == NULL) return false;
        CardStar card_star = data.value<CardResponseStruct>().m_card;
        if (!card_star->isKindOf("Jink"))
            return false;
        room->askForUseCard(player, "@@leiji", "@leiji");

        return false;
    }
};

TianshiCard::TianshiCard(){
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodResponse;
}

void TianshiCard::use(Room *room, ServerPlayer *zhangjiao, QList<ServerPlayer *> &targets) const{

}

class TianshiViewAsSkill:public OneCardViewAsSkill{
public:
    TianshiViewAsSkill():OneCardViewAsSkill(""){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@tianshi";
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isBlack();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        TianshiCard *tianshiCard = new TianshiCard;
        tianshiCard->setSuit(originalCard->getSuit());
        tianshiCard->addSubcard(originalCard);

        return tianshiCard;
    }
};

class Tianshi: public TriggerSkill{
public:
    Tianshi():TriggerSkill("tianshi"){
        view_as_skill = new TianshiViewAsSkill;

        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        
        if(target == NULL || !TriggerSkill::triggerable(target))
            return false;

        if(target->isKongcheng()){
            bool has_black = false;
            for(int i = 0; i < 4; i++){
                const EquipCard *equip = target->getEquip(i);
                if(equip && equip->isBlack()){
                    has_black = true;
                    break;
                }
            }

            return has_black;
        }else
            return true;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (player == NULL) return false;
        JudgeStar judge = data.value<JudgeStar>();

        QStringList prompt_list;
        prompt_list << "@tianshi-card" << judge->who->objectName()
                << objectName() << judge->reason << judge->card->getEffectIdString();
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, "@tianshi", prompt, data, Card::MethodResponse, judge->who, true);

        if (card != NULL){
            room->broadcastSkillInvoke(objectName());
            room->retrial(card, player, judge, objectName(), true);
        }
        return false;
    }
};

YujiCard::YujiCard(){
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "yujiv";
    mute = true;
}

void YujiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    if(target->hasLordSkill("yuji")){
        room->setPlayerFlag(target, "YujiInvoked");
        room->broadcastSkillInvoke("yuji");
        target->obtainCard(this);
        QList<int> subcards = this->getSubcards();
        foreach(int card_id, subcards)
            room->setCardFlag(card_id,"visible");
        room->setEmotion(target, "good");
        QList<ServerPlayer *> lords;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach(ServerPlayer *p, players){
            if(p->hasLordSkill("yuji") && !p->hasFlag("YujiInvoked")){
                lords << p;
            }
        }
        if(lords.empty())
            room->setPlayerFlag(source, "ForbidYuji");
    }
}

bool YujiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("yuji")
            && to_select != Self && !to_select->hasFlag("YujiInvoked");
}

class YujiViewAsSkill: public OneCardViewAsSkill{
public:
    YujiViewAsSkill():OneCardViewAsSkill("yujiv"){
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getKingdom() == "qun" && !player->hasFlag("ForbidYuji");
    }

    virtual bool viewFilter(const Card* to_select) const{
        const Card *card = to_select;
        return card->objectName() == "jink" || card->isBlack();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        YujiCard *card = new YujiCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Yuji:public TriggerSkill{
public:
	Yuji():TriggerSkill("yuji$"){
		events << EventPhaseStart << EventPhaseEnd << EventPhaseChanging;
	}
	
	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == EventPhaseEnd && player->hasSkill("yujiv"))
			room->detachSkillFromPlayer(player, "yujiv", true);
		else if (triggerEvent == EventPhaseStart)
		{
			bool can_invoke = false;
			foreach(ServerPlayer *p, room->getOtherPlayers(player))
				if (p->hasLordSkill("yuji"))
				{
					can_invoke = true;
					break;
				}
			if (can_invoke && player->getPhase() == Player::Play && !player->hasSkill("yujiv") && player->getKingdom() == "qun")
				room->attachSkillToPlayer(player, "yujiv");
		}
		else if (triggerEvent == EventPhaseChanging)
		{
			PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                  return false;
            if(player->hasFlag("ForbidYuji")){
                room->setPlayerFlag(player, "-ForbidYuji");
            }
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach(ServerPlayer *p, players){
                if(p->hasFlag("YujiInvoked")){
                    room->setPlayerFlag(p, "-YujiInvoked");
                }
            }
		}

		return false;
	}
};

class Huiyao: public TriggerSkill{
public:
    Huiyao():TriggerSkill("huiyao"){
        events << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.card == NULL || !damage.card->isKindOf("Slash") || damage.to->isDead())
            return false;

        QList<ServerPlayer *> splayers = room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *splayer, splayers){
            if(!splayer->isNude() && splayer->askForSkillInvoke(objectName(), data)){
                room->askForDiscard(splayer, "huiyao", 1, 1, false, true);

                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(.*):(.*)");
                judge.good = true;
                judge.who = player;
                judge.reason = objectName();

                room->judge(judge);

                switch(judge.card->getSuit()){
                case Card::Heart:{
                        room->broadcastSkillInvoke(objectName(), 4);
                        RecoverStruct recover;
                        recover.who = splayer;
                        room->recover(player, recover);

                        break;
                    }

                case Card::Diamond:{
                        room->broadcastSkillInvoke(objectName(), 3);
                        player->drawCards(2);
                        break;
                    }

                case Card::Club:{
                        room->broadcastSkillInvoke(objectName(), 1);
                        if(damage.from && damage.from->isAlive()){
                            int to_discard = qMin(2, damage.from->getCardCount(true));
                            if(to_discard != 0)
                                room->askForDiscard(damage.from, "huiyao", to_discard, to_discard, false, true);
                        }

                        break;
                    }

                case Card::Spade:{
                        room->broadcastSkillInvoke(objectName(), 2);
                        if(damage.from && damage.from->isAlive())
                            damage.from->turnOver();

                        break;
                    }

                default:
                    break;
                }
            }
        }
        return false;
    }
};

class Qihuang: public TriggerSkill{
public:
    Qihuang():TriggerSkill("qihuang"){
        events << Death;

        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStar damage = data.value<DamageStar>();

        if(damage && damage->from){
            LogMessage log;
            log.type = "#QihuangLoseSkills";
            log.from = player;
            log.to << damage->from;
            log.arg = objectName();
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());

            QList<const Skill *> skills = damage->from->getVisibleSkillList();
            foreach(const Skill *skill, skills){
                if(skill->getLocation() == Skill::Right && !skill->isAttachedLordSkill())
                    room->detachSkillFromPlayer(damage->from, skill->objectName());
            }
            damage->from->gainMark("@qihuang");
            if(damage->from->getKingdom() != damage->from->getGeneral()->getKingdom())
                room->setPlayerProperty(damage->from, "kingdom", damage->from->getGeneral()->getKingdom());
            if(damage->from->getGender() != damage->from->getGeneral()->getGender())
                damage->from->setGender(damage->from->getGeneral()->getGender());
        }

        return false;
    }
};

LvdongCard::LvdongCard(){
    once = true;
    will_throw = false;
    handling_method = Card::MethodPindian;
}

bool LvdongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && ! to_select->isKongcheng();
}

void LvdongCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();

    const Card *card = Sanguosha->getCard(subcards.first());
    if(effect.from->pindian(effect.to, "lvdong", card)){
        PlayerStar target = effect.to;
        effect.from->tag["LvdongTarget"] = QVariant::fromValue(target);
        room->setPlayerFlag(effect.from, "lvdong_success");
        room->setFixedDistance(effect.from, effect.to, 1);
        effect.to->addMark("Armor_Nullified");
    }else{
        room->setPlayerCardLimitation(effect.from, "use", "Slash", true);
    }
}

LvdongSlashCard::LvdongSlashCard(){
    target_fixed = true;
    handling_method = Card::MethodUse;
}

void LvdongSlashCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *target = card_use.from->tag["LvdongTarget"].value<PlayerStar>();
    if(target == NULL || target->isDead())
        return;

    if(!card_use.from->canSlash(target, NULL, false))
        return;

    room->askForUseSlashTo(card_use.from, target, "@lvdong-slash");
}

class LvdongViewAsSkill: public ViewAsSkill{
public:
    LvdongViewAsSkill():ViewAsSkill(""){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (!player->hasUsed("LvdongCard"))
            return true;
        Slash *slashx = new Slash(Card::NoSuitNoColor, 0);
        slashx->deleteLater();
        if (!player->isCardLimited(slashx, Card::MethodUse) && player->hasFlag("lvdong_success"))
            return true;

        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(!selected.isEmpty())
            return false;

        if(Self->hasUsed("LvdongCard"))
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(! Self->hasUsed("LvdongCard")){
            if(cards.length() != 1)
                return NULL;

            LvdongCard *card = new LvdongCard;
            card->addSubcards(cards);
            return card;
        }else if(Self->hasFlag("lvdong_success")){
            if(!cards.isEmpty())
                return NULL;

            return new LvdongSlashCard;
        }else
            return NULL;
    }
};

class Lvdong: public TriggerSkill{
public:
    Lvdong():TriggerSkill("lvdong"){
        view_as_skill = new LvdongViewAsSkill;

        events << EventPhaseStart << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill("lvdong");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *target = player->tag["LvdongTarget"].value<PlayerStar>();

        if((triggerEvent == Death || player->getPhase() == Player::Finish) && target){
            Room *room = player->getRoom();
            room->setFixedDistance(player, target, -1);
            player->tag.remove("LvdongTarget");
			target->removeMark("Armor_Nullified");
        }
        return false;
    }
};

class Guozai: public FilterSkill{
public:
    Guozai():FilterSkill("guozai"){

    }

    virtual bool viewFilter(const Card* to_select) const{
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return place == Player::PlaceHand && to_select->objectName() == "analeptic";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class Tianjing: public MaxCardsSkill{
public:
    Tianjing():MaxCardsSkill("tianjing"){
    }
    virtual int getExtra(const Player *target) const{
        if(target->hasSkill(objectName()) && target->isWounded())
            return 4;
        else
            return 0;
    }
};

class Danbo:public DrawCardsSkill{
public:
    Danbo():DrawCardsSkill("danbo"){
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        if(room->askForSkillInvoke(player, objectName())){
            room->broadcastSkillInvoke(objectName());
            player->clearHistory();
            player->skip(Player::Play);
            return n + player->getLostHp();
        }else
            return n;
    }
};

class Chenyan: public TriggerSkill{
public:
    Chenyan():TriggerSkill("chenyan"){
        events << DrawNCards << EventPhaseStart;
        frequency = Compulsory;
    }

    int getKingdoms(ServerPlayer *player) const{
        QSet<QString> kingdom_set;
        Room *room = player->getRoom();
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            kingdom_set << p->getKingdom();
        }

        return kingdom_set.size();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == DrawNCards){
            int x = getKingdoms(player);
            data = data.toInt() + x;

            Room *room = player->getRoom();
            LogMessage log;
            log.type = "#ChenyanGood";
            log.from = player;
            log.arg = QString::number(x);
            log.arg2 = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke("chenyan", x);

        }else if(triggerEvent == EventPhaseStart && player->getPhase() == Player::Discard){
            int x = getKingdoms(player) + 1;
            int total = 0;
            QSet<const Card *> jilei_cards;
            QList<const Card *> handcards = player->getHandcards();
            foreach(const Card *card, handcards){
                if(player->isJilei(card))
                    jilei_cards << card;
            }
            total = handcards.size() - jilei_cards.size() + player->getEquips().length();

            if(x >= total){
                if (player->hasFlag("jilei")){
                    LogMessage log;
                    log.type = "#ChenyanBad";
                    log.from = player;
                    log.arg = QString::number(total);
                    log.arg2 = objectName();
                    room->sendLog(log);
                    DummyCard *dummy_card = new DummyCard;
                    foreach(const Card *card, handcards.toSet() - jilei_cards){
                        dummy_card->addSubcard(card);
                    }
                    QList<const Card *> equips = player->getEquips();
                    foreach(const Card *equip, equips)
                        dummy_card->addSubcard(equip);
                    if (dummy_card->subcardsLength() > 0)
                        room->throwCard(dummy_card, player);
                    room->showAllCards(player);
                    delete dummy_card;
                }
                else
                {
                    LogMessage log;
                    log.type = "#ChenyanWorst";
                    log.from = player;
                    log.arg = QString::number(total);
                    log.arg2 = objectName();
                    room->sendLog(log);

                    player->throwAllHandCardsAndEquips();
                }
            }
            else
            {
                LogMessage log;
                log.type = "#ChenyanBad";
                log.from = player;
                log.arg = QString::number(x);
                log.arg2 = objectName();
                room->sendLog(log);
                room->askForDiscard(player, "chenyan", x, x, false, true);
            }
        }

        return false;
    }
};

class ShengzunViewAsSkill: public ZeroCardViewAsSkill {
public:
    ShengzunViewAsSkill(): ZeroCardViewAsSkill("shengzun") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        LiqiViewAsSkill *liqi = new LiqiViewAsSkill;
        liqi->deleteLater();
        return liqi->isEnabledAtPlay(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        LiqiViewAsSkill *liqi = new LiqiViewAsSkill;
        liqi->deleteLater();
        return liqi->isEnabledAtResponse(player, pattern);
    }

    virtual const Card *viewAs() const{
        return new LiqiCard;
    }
};

class Shengzun:public GameStartSkill{
public:
    Shengzun():GameStartSkill("shengzun"){
        view_as_skill = new ShengzunViewAsSkill;
    }

    virtual void onGameStart(ServerPlayer *) const{
        // do nothing
        return;
    }
};

class Zhuji: public DistanceSkill{
public:
    Zhuji():DistanceSkill("zhuji"){

    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        int correct = 0;
        if(from->hasSkill(objectName()) && from->getHp() > 2)
            correct --;
        if(to->hasSkill(objectName()) && to->getHp() <= 2)
            correct ++;

        return correct;
    }
};

class Benyin: public TriggerSkill{
public:
    Benyin():TriggerSkill("benyin"){
        events << PostHpReduced << HpRecover;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        int hp = player->getHp();
        bool invoke = false;
        if (triggerEvent == HpRecover){
            RecoverStruct recover = data.value<RecoverStruct>();
            if (hp <= 2 && hp + recover.recover > 2)
                invoke = true;
        }
        else if (triggerEvent == PostHpReduced){
            int reduce = 0;
            if (data.canConvert<DamageStruct>()) {
                DamageStruct damage = data.value<DamageStruct>();
                reduce = damage.damage;
            } else
                reduce = data.toInt();
            if (hp <= 2 && hp + reduce > 2)
                invoke = true;
        }

        if(invoke && player->askForSkillInvoke(objectName())){
            room->broadcastSkillInvoke(objectName());
            QStringList choices;
            choices << "draw";

            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if(!p->isAllNude()) {
                    choices << "discard";
                    break;
                }

            QString choice;

            if(choices.length() > 1)
                choice = room->askForChoice(player, objectName(), choices.join("+"));
            else
                choice = "draw";

            if(choice == "draw")
                player->drawCards(1);
            else {
                QList<ServerPlayer *> victims;
                foreach(ServerPlayer *p, room->getOtherPlayers(player))
                    if(!p->isAllNude())
                        victims << p;

                ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
                int card_id = room->askForCardChosen(player, victim, "hej", objectName());
                room->throwCard(card_id, victim, player);
            }
        }
        return false;
    }
};

class Wenxin: public TriggerSkill{
public:
    Wenxin():TriggerSkill("wenxin"){
        events << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct &damage = data.value<DamageStruct>();
        if(damage.from == NULL)
            return false;

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        QStringList choices;
        choices << "show" << "reduce";
        QString choice = room->askForChoice(damage.from, objectName(), choices.join("+"));
        if(choice == "reduce") {
            LogMessage log;
            log.type = "#WenxinDecrease";
            log.from = damage.from;
            log.to << player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(--damage.damage);
            room->sendLog(log);

            if(damage.damage <= 0) {
                LogMessage log;
                log.type = "#ZeroDamage";
                log.from = damage.from;
                log.to << player;
                room->sendLog(log);

                return true;
            } else
                data = QVariant::fromValue(damage);

            return false;
        } else
            room->showAllCards(damage.from, NULL);

        return false;
    }
};

class Shuhui: public TriggerSkill{
public:
    Shuhui():TriggerSkill("shuhui"){
        events << CardsMoveOneTime;
    }

    virtual int getPriority() const{
        return 4;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
        if (move->from != player)
            return false;
        if (move->to_place == Player::DiscardPile
           && ((move->reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)){

            int i = 0;
            QList<int> shuhui_card;
            foreach(int card_id, move->card_ids){
                if(move->from_places[i] == Player::PlaceHand || move->from_places[i] == Player::PlaceEquip){
                        shuhui_card << card_id;
                }
                i++;
            }
            if (shuhui_card.isEmpty())
                return false;

            if (!player->askForSkillInvoke(objectName(), data))
                return false;
            room->setPlayerFlag(player, "Shuhui_InTempMoving");

            CardsMoveStruct move3;
            move3.card_ids = shuhui_card;
            move3.to_place = Player::PlaceHand;
            move3.to = player;
            room->moveCardsAtomic(move3, true);

            while(room->askForYumeng(player, shuhui_card, false, true));

            CardsMoveStruct move4;
            move4.card_ids = shuhui_card;
            move4.to_place = Player::DiscardPile;
            move4.reason = move->reason;
            room->moveCardsAtomic(move4, true);

            room->setPlayerFlag(player, "-Shuhui_InTempMoving");
        }
        return false;
    }
};

class ShuhuiAvoidTriggeringCardsMove: public TriggerSkill{
public:
    ShuhuiAvoidTriggeringCardsMove():TriggerSkill("#shuhui"){
        events << CardsMoveOneTime;
    }

    virtual int getPriority() const{
        return 10;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const{
        if (player->hasFlag("Shuhui_InTempMoving"))
            return true;
        return false;
    }
};

class Shuangren: public TriggerSkill{
public:
    Shuangren():TriggerSkill("shuangren"){
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Play)
            return false;

        if(player->askForSkillInvoke(objectName())){
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if(!p->isKongcheng())
                    targets << p;

            if(targets.isEmpty())
                return false;

            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            bool success = player->pindian(target, objectName(), NULL);
            if(success) {
                Slash *slash = new Slash(Card::NoSuitNoColor, 0);
                slash->setSkillName(objectName());
                QList<ServerPlayer *> victims;
                foreach(ServerPlayer *p, room->getOtherPlayers(player))
                    if(!player->isProhibited(p, slash))
                        victims << p;
                if(!victims.isEmpty()) {
                    ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
                    CardUseStruct use;
                    use.card = slash;
                    use.from = player;
                    use.to << victim;
                    room->useCard(use, false);
                }
            } else
                return true;
        }
        return false;
    }
};
class Longya: public TriggerSkill {
public:
    Longya(): TriggerSkill("longya") {
        events << TargetConfirmed << SlashMissed << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player != use.from || !TriggerSkill::triggerable(player) || !use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, use.to) {
                if (player->askForSkillInvoke(objectName())) {
                    QString choice;
                    if (p->isNude())
                        choice = "draw";
                    else
                        choice = room->askForChoice(player, objectName(), "draw+discard");
                    if (choice == "draw")
                        player->drawCards(1);
                    else {
                        int disc = room->askForCardChosen(player, p, "he", objectName());
                        room->throwCard(disc, p, player);
                    }
                    room->setPlayerMark(p, objectName() + use.card->toString(),
                                        p->getMark(objectName() + use.card->toString()) + 1);
                }
            }
        } else if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.to->getMark(objectName() + effect.slash->toString()) <= 0)
                return false;
            if (!effect.from->isAlive() || !effect.to->isAlive() || effect.from->isNude())
                return false;
            int disc = room->askForCardChosen(effect.to, effect.from, "he", objectName());
            room->throwCard(disc, effect.from, effect.to);
            room->setPlayerMark(effect.to, objectName() + effect.slash->toString(),
                                effect.to->getMark(objectName() + effect.slash->toString()) - 1);
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, objectName() + use.card->toString(), 0);
        }

        return false;
    }
};

class Zhuohuo: public TriggerSkill {
public:
    Zhuohuo(): TriggerSkill("zhuohuo") {
        events << Damage << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = triggerEvent == Damage ? "#ZhuohuoDamage" : "#ZhuohuoDamaged";
        log.from = player;
        log.arg = QString::number(damage.damage);
        log.arg2 = objectName();
        room->sendLog(log);

        player->gainMark("@mailun", damage.damage);
        room->broadcastSkillInvoke(objectName(), triggerEvent == Damage ? 1 : 2);
        return false;
    }
};

class Wumou: public TriggerSkill{
public:
    Wumou(): TriggerSkill("wumou"){
        frequency = Compulsory;
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardStar card = NULL;
        if(triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }else if(triggerEvent == CardResponded)
            card = data.value<CardResponseStruct>().m_card;

        if(card->isNDTrick()){
            room->broadcastSkillInvoke(objectName());

            int num = player->getMark("@mailun");
            if(num >= 1 && room->askForChoice(player, objectName(), "discard+losehp") == "discard"){
                player->loseMark("@mailun");
            }else
                room->loseHp(player);
        }

        return false;
    }
};

SuikongCard::SuikongCard() {
}

bool SuikongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self; 
}

void SuikongCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    effect.from->loseMark("@mailun", 2);
    room->acquireSkill(effect.from, "wushuang", false);
    room->setPlayerFlag(effect.to, "SuikongTarget");
    effect.to->addMark("Armor_Nullified");
}

class SuikongViewAsSkill: public ZeroCardViewAsSkill {
public:
    SuikongViewAsSkill(): ZeroCardViewAsSkill("suikong") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@mailun") >= 2;
    }

    virtual const Card *viewAs() const{
        return new SuikongCard;
    }
};

class Suikong: public TriggerSkill {
public:
    Suikong(): TriggerSkill("suikong") {
        events << EventPhaseChanging << Death;
        view_as_skill = new SuikongViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill("suikong");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        }

        foreach (ServerPlayer *p , room->getAllPlayers()) {
            if (p->hasFlag("SuikongTarget")) {
                room->setPlayerFlag(p, "-SuikongTarget");
                if (p->getMark("Armor_Nullified") > 0)
                    p->removeMark("Armor_Nullified");
            }
        }
        room->detachSkillFromPlayer(player, "wushuang");

        return false;
    }
};

TianwuCard::TianwuCard() {
    target_fixed = true;
    mute = true;
}

void TianwuCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const{
    room->broadcastSkillInvoke("tianwu");
    player->loseMark("@mailun", 6);

    QList<ServerPlayer *> players = room->getOtherPlayers(player);
    foreach (ServerPlayer *player, players) {
        DamageStruct damage;
        damage.from = player;
        damage.to = player;

        room->damage(damage);
	}

    foreach (ServerPlayer *player, players) {
        QList<const Card *> equips = player->getEquips();
        player->throwAllEquips();
        if (!equips.isEmpty())
            room->getThread()->delay();
    }

    foreach (ServerPlayer *player, players) {
        room->askForDiscard(player, "tianwu", 4, 4);
    }

    player->turnOver();
}

class Tianwu: public ZeroCardViewAsSkill {
public:
    Tianwu(): ZeroCardViewAsSkill("tianwu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@mailun") >= 6 && !player->hasUsed("TianwuCard");
    }

    virtual const Card *viewAs() const{
        return new TianwuCard;
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

    General *luna004 = new General(this, "luna004", "qun");
    luna004->addSkill(new Xinghuang);

    General *luna005 = new General(this, "luna005", "qun");
    luna005->addSkill(new Shuangniang);

    General *luna006 = new General(this, "luna006$", "qun", 8);
    luna006->addSkill(new Fusheng);
    luna006->addSkill(new Huanbei);
    luna006->addSkill(new Benghuai);
    luna006->addSkill(new Wuhua);

    General *luna007 = new General(this, "luna007", "qun", 3);
    luna007->addSkill(new Sishi);
    luna007->addSkill(new MarkAssignSkill("@wenle", 1));
    luna007->addSkill(new Wenle);
    luna007->addSkill(new Moyu2);
    related_skills.insertMulti("wenle", "#@wenle-1");

    General *luna008 = new General(this, "luna008", "qun");
    luna008->addSkill("jibu");
    luna008->addSkill(new Mengjin);

    General *luna010 = new General(this, "luna010$", "qun", 3);
    luna010->addSkill(new Leiji);
    luna010->addSkill(new Tianshi);
    luna010->addSkill(new Yuji);

    General *luna012 = new General(this, "luna012", "qun", 3, false);
    luna012->addSkill(new Huiyao);
    luna012->addSkill(new Qihuang);

    General *luna014 = new General(this, "luna014", "qun");
    luna014->addSkill(new Lvdong);
    luna014->addSkill(new Guozai);

    General *luna015 = new General(this, "luna015", "qun");
    luna015->addSkill(new Tianjing);
    luna015->addSkill(new Danbo);

    General *luna017 = new General(this, "luna017", "qun");
    luna017->addSkill(new Chenyan);
    luna017->addSkill(new Shengzun);

    General *luna018 = new General(this, "luna018", "qun");
    luna018->addSkill(new Zhuji);
    luna018->addSkill(new Benyin);

    General *luna020 = new General(this, "luna020", "qun", 3);
    luna020->addSkill(new Wenxin);
    luna020->addSkill(new Shuhui);
    luna020->addSkill(new ShuhuiAvoidTriggeringCardsMove);
    related_skills.insertMulti("shuhui", "#shuhui");

    General *luna021 = new General(this, "luna021", "qun");
    luna021->addSkill(new Shuangren);

    General *luna024 = new General(this, "luna024", "qun");
    luna024->addSkill(new Longya);
    
	General *luna029 = new General(this, "luna029", "god", 5);
    luna029->addSkill(new Zhuohuo);
    luna029->addSkill(new MarkAssignSkill("@mailun", 2));
    luna029->addSkill(new Wumou);
    luna029->addSkill(new Suikong);
    luna029->addSkill(new Tianwu);
    related_skills.insertMulti("zhuohuo", "#@mailun-2");

    addMetaObject<QingnangCard>();
    addMetaObject<MoyuCard>();
    addMetaObject<WenleCard>();
    addMetaObject<LeijiCard>();
    addMetaObject<TianshiCard>();
    addMetaObject<YujiCard>();
    addMetaObject<LvdongCard>();
    addMetaObject<LvdongSlashCard>();
    addMetaObject<SuikongCard>();
    addMetaObject<TianwuCard>();

    skills << new YujiViewAsSkill;
}