#include "yjcm-package.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "carditem.h"
#include "engine.h"
#include "ai.h"
#include "general.h"

class Enyuan: public TriggerSkill{
public:
    Enyuan():TriggerSkill("enyuan"){
        events << CardsMoveOneTime << Damaged;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (player == NULL) return false;

        if(triggerEvent == CardsMoveOneTime){
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            if (move->to == player && move->from != NULL && move->card_ids.size() >= 2
                && room->askForSkillInvoke(player,objectName(),data)){
                    room->drawCards((ServerPlayer*)move->from, 1);
                    room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
            }
        }else if(triggerEvent == Damaged){
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if(!source || source == player) return false;
            int x = damage.damage, i;
            for(i = 0; i < x; i++) {
                if (room->askForSkillInvoke(player, objectName(), data)){
                    room->broadcastSkillInvoke(objectName(), qrand() % 2 + 3);
                    const Card *card = room->askForExchange(source, objectName(), 1, false, "EnyuanGive", true);
                    if(card){
                        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
                                              player->objectName(), objectName(), QString());
                        reason.m_playerId = player->objectName();
                        room->moveCardTo(card, source, player, Player::PlaceHand, reason);
                    }else{
                        room->loseHp(source);
                    }
                }
            }
        }
        return false;
    }
};

/*XuanhuoCard::XuanhuoCard(){
}

bool XuanhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select == Self)
        return false;

    return true;
}

void XuanhuoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->drawCards(effect.to,2);

    bool can_use = false;
    foreach(ServerPlayer *p, room->getOtherPlayers(effect.to)){
        if (effect.to->canSlash(p)){
            can_use = true;
            break;
        }
    }
    ServerPlayer *victim = NULL;
    if (can_use){
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *victim, room->getOtherPlayers(effect.to)){
            if(effect.to->canSlash(victim))
                targets << victim;
        }
        victim = room->askForPlayerChosen(effect.from, targets, "xuanhuo");

        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = effect.from;
        log.to << victim;
        room->sendLog(log);

        QString prompt = QString("xuanhuo-slash:%1:%2")
                .arg(effect.from->objectName()).arg(victim->objectName());
        if (!room->askForUseSlashTo(effect.to, victim, prompt)){
            if (effect.to->isNude())
                return;
            int first_id = room->askForCardChosen(effect.from, effect.to, "he", "xuanhuo");
            DummyCard *dummy = new DummyCard;
            dummy->addSubcard(first_id);
            effect.to->addToPile("#xuanhuo", dummy, false);
            if (!effect.to->isNude()){
                int second_id = room->askForCardChosen(effect.from, effect.to, "he", "xuanhuo");
                dummy->addSubcard(second_id);
            }
            room->moveCardTo(dummy, effect.from, Player::PlaceHand, false);
            delete dummy;
        }
    }
    else{
        if (effect.to->isNude())
            return;
        int first_id = room->askForCardChosen(effect.from, effect.to, "he", "xuanhuo");
        DummyCard *dummy = new DummyCard;
        dummy->addSubcard(first_id);
        effect.to->addToPile("#xuanhuo", dummy, false);
        if (!effect.to->isNude()){
            int second_id = room->askForCardChosen(effect.from, effect.to, "he", "xuanhuo");
            dummy->addSubcard(second_id);
        }
        room->moveCardTo(dummy, effect.from, Player::PlaceHand, false);
        delete dummy;
    }
}

class XuanhuoViewAsSkill: public ZeroCardViewAsSkill{
public:
    XuanhuoViewAsSkill():ZeroCardViewAsSkill("xuanhuo"){
    }

    virtual const Card *viewAs() const{
        return new XuanhuoCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@xuanhuo";
    }
};

class Xuanhuo:public PhaseChangeSkill{
public:
    Xuanhuo():PhaseChangeSkill("xuanhuo"){
        view_as_skill = new XuanhuoViewAsSkill;
    }

    virtual QString getDefaultChoice(ServerPlayer *) const{
        return "give";
    }

    virtual bool onPhaseChange(ServerPlayer *fazheng) const{
        Room *room = fazheng->getRoom();
        if(fazheng->getPhase() == Player::Draw){
            if(room->askForUseCard(fazheng, "@@xuanhuo", "@xuanhuo-card"))
                return true;
        }
        return false;
    }
};*/

XuanfengCard::XuanfengCard(){
}

bool XuanfengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.length() >= 2)
        return false;

    if(to_select == Self)
        return false;

    return !to_select->isNude();
}

void XuanfengCard::use(Room *room, ServerPlayer *lingtong, QList<ServerPlayer *> &targets) const{
    QMap<ServerPlayer*,int> map;
    int totaltarget = 0;
    foreach(ServerPlayer* sp, targets)
        map[sp]++;
    for (int i = 0; i < map.keys().size(); i++) {
        totaltarget++;
    }
    // only chose one and throw only one card of him is forbiden
    if(totaltarget == 1){
        foreach(ServerPlayer* sp,map.keys()){
            map[sp]++;
        }
    }
    foreach(ServerPlayer* sp,map.keys()){
        while(map[sp] > 0){
            if(!sp->isNude()){
                int card_id = room->askForCardChosen(lingtong, sp, "he", "xuanfeng");
                room->throwCard(card_id, sp, lingtong);
            }
            map[sp]--;
        }
    }
}

class XuanfengViewAsSkill: public ZeroCardViewAsSkill{
public:
    XuanfengViewAsSkill():ZeroCardViewAsSkill("xuanfeng"){
    }

    virtual const Card *viewAs() const{
        return new XuanfengCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@xuanfeng";
    }
};

class Xuanfeng: public TriggerSkill{
public:
    Xuanfeng():TriggerSkill("xuanfeng"){
        events << CardsMoveOneTime << EventPhaseStart;
        view_as_skill = new XuanfengViewAsSkill;
    }

    virtual QString getDefaultChoice(ServerPlayer *) const{
        return "nothing";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *lingtong, QVariant &data) const{
        if(triggerEvent == EventPhaseStart){
            lingtong->setMark("xuanfeng", 0);
        }
        else if(triggerEvent == CardsMoveOneTime)
        {
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            if(move->from != lingtong)
                return false;

            if(move->to_place == Player::DiscardPile && lingtong->getPhase() == Player::Discard)
                lingtong->setMark("xuanfeng", lingtong->getMark("xuanfeng") + move->card_ids.length());

            if((lingtong->getMark("xuanfeng") >= 2 && !lingtong->hasFlag("xuanfeng_used"))
                    || move->from_places.contains(Player::PlaceEquip))
            {
                if(lingtong->getMark("xuanfeng") >= 2)
                    room->setPlayerFlag(lingtong, "xuanfeng_used");
                bool can_invoke = false;
                QList<ServerPlayer *> other_players = room->getOtherPlayers(lingtong);
                foreach(ServerPlayer *player, other_players){
                    if(!player->isNude()){
                        can_invoke = true;
                        break;
                    }
                }
                if(can_invoke){
                    QString choice = room->askForChoice(lingtong, objectName(), "throw+nothing");
                    if(choice == "throw"){
                        room->askForUseCard(lingtong, "@@xuanfeng", "@xuanfeng-card");
                    }
                }
            }
        }

        return false;
    }
};

MingceCard::MingceCard(){
    once = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void MingceCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(this);

    Room *room = effect.to->getRoom();
    QString choice;
    bool can_use = false;
    foreach(ServerPlayer *p, room->getOtherPlayers(effect.to)){
        if (effect.to->canSlash(p)){
            can_use = true;
            break;
        }
    }
    if (can_use){
        choice = room->askForChoice(effect.to, "mingce", "use+draw");
    }
    else
        choice = "draw";

    if(choice == "use"){
        QList<ServerPlayer *> players = room->getOtherPlayers(effect.to), targets;
        foreach(ServerPlayer *player, players){
            if(effect.to->canSlash(player))
                targets << player;
        }

        if(!targets.isEmpty()){
            ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "mingce");

            Slash *slash = new Slash(Card::NoSuitNoColor, 0);
            slash->setSkillName("mingce");
            CardUseStruct card_use;
            card_use.from = effect.to;
            card_use.to << target;
            card_use.card = slash;
            room->useCard(card_use, false);
        }
    }else if(choice == "draw"){
        room->broadcastSkillInvoke("mingce", 1);
        effect.to->drawCards(1, true);
    }
}

class Mingce: public OneCardViewAsSkill{
public:
    Mingce():OneCardViewAsSkill("mingce"){
        default_choice = "draw";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("MingceCard");
    }

    virtual bool viewFilter(const Card* to_select) const{
        const Card *c = to_select;
        return c->getTypeId() == Card::TypeEquip || c->isKindOf("Slash");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        MingceCard *mingceCard = new MingceCard;
        mingceCard->addSubcard(originalCard);

        return mingceCard;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (card->isKindOf("Slash"))
            return 2;
        else
            return 0;
    }
};

class ZhichiClear: public TriggerSkill{
public:
    ZhichiClear():TriggerSkill("#zhichi-clear"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
        if(player->getPhase() == Player::NotActive)
            player->getRoom()->setTag("Zhichi", QVariant());

        return false;
    }
};

class Zhichi: public TriggerSkill{
public:
    Zhichi():TriggerSkill("zhichi"){
        events << Damaged << CardEffected;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (player == NULL) return false;

        if(player->getPhase() != Player::NotActive)
            return false;

        if(triggerEvent == Damaged){
            if(room->getCurrent()->isAlive()){
                room->setTag("Zhichi", player->objectName());
                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.type = "#ZhichiDamaged";
                log.from = player;
                room->sendLog(log);
            }

        }else if(triggerEvent == CardEffected){
            if(room->getTag("Zhichi").toString() != player->objectName())
                return false;

            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(effect.card->isKindOf("Slash") || effect.card->getTypeId() == Card::TypeTrick){
                LogMessage log;
                log.type = "#ZhichiAvoid";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                return true;
            }
        }

        return false;
    }
};

GanluCard::GanluCard(){
    once = true;
}

void GanluCard::swapEquip(ServerPlayer *first, ServerPlayer *second) const{
    Room *room = first->getRoom();

    QList<int> equips1, equips2;
    foreach(const Card *equip, first->getEquips())
        equips1.append(equip->getId());
    foreach(const Card *equip, second->getEquips())
        equips2.append(equip->getId());

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1;
    move1.card_ids = equips1;
    move1.to = second;
    move1.to_place = Player::PlaceEquip;
    CardsMoveStruct move2;
    move2.card_ids = equips2;
    move2.to = first;
    move2.to_place = Player::PlaceEquip;
    exchangeMove.push_back(move2);
    exchangeMove.push_back(move1);
    room->moveCards(exchangeMove, false);
}

bool GanluCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

bool GanluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    switch(targets.length()){
    case 0: return true;
    case 1: {
            int n1 = targets.first()->getEquips().length();
            int n2 = to_select->getEquips().length();
            return qAbs(n1-n2) <= Self->getLostHp();
        }

    default:
        return false;
    }
}

void GanluCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *first = targets.first();
    ServerPlayer *second = targets.at(1);

    swapEquip(first, second);

    LogMessage log;
    log.type = "#GanluSwap";
    log.from = source;
    log.to = targets;
    room->sendLog(log);
}

class Ganlu: public ZeroCardViewAsSkill{
public:
    Ganlu():ZeroCardViewAsSkill("ganlu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("GanluCard");
    }

    virtual const Card *viewAs() const{
        return new GanluCard;
    }
};

class Buyi: public TriggerSkill{
public:
    Buyi():TriggerSkill("buyi"){
        events << Dying;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return player != NULL && !player->isKongcheng();
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if (player == NULL) return false;
        QList<ServerPlayer *> wuguots = room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *wuguotai, wuguots){
            if(player->getHp() < 1 && wuguotai->askForSkillInvoke(objectName(), data)){
                const Card *card = NULL;
                if(player == wuguotai)
                    card = room->askForCardShow(player, wuguotai, objectName());
                else{
                    int card_id = room->askForCardChosen(wuguotai, player, "h", "buyi");
                    card = Sanguosha->getCard(card_id);
                }

                room->showCard(player, card->getEffectiveId());

                if(card->getTypeId() != Card::TypeBasic){
                    room->throwCard(card, player);

                    room->broadcastSkillInvoke(objectName());

                    RecoverStruct recover;
                    recover.who = wuguotai;
                    room->recover(player, recover);
                }
            }
        }
        return false;
    }
};

class Quanji:public MasochismSkill{
public:
    Quanji():MasochismSkill("#quanji"){
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const{
        Room *room = zhonghui->getRoom();

        if(!room->askForSkillInvoke(zhonghui, "quanji"))
            return;

        room->broadcastSkillInvoke("quanji");

        int x = damage.damage, i;
        for(i=0; i<x; i++){
            room->drawCards(zhonghui,1);
            if(!zhonghui->isKongcheng())
            {
                int card_id;
                if(zhonghui->handCards().length() == 1){
                    room->getThread()->delay(500);
                    card_id = zhonghui->handCards().first();
                }
                else
                    card_id = room->askForExchange(zhonghui, "quanji", 1, false, "QuanjiPush")->getSubcards().first();
                zhonghui->addToPile("power", card_id);
            }
        }

    }
};

class QuanjiKeep: public MaxCardsSkill{
public:
    QuanjiKeep():MaxCardsSkill("quanji"){
        frequency = Frequent;
    }

    virtual int getExtra(const Player *target) const{
        if(target->hasSkill(objectName()))
            return target->getPile("power").length();
        else
            return 0;
    }
};

class QuanjiRemove: public TriggerSkill{
public:
    QuanjiRemove():TriggerSkill("#quanji-remove"){
        events << EventLoseSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent , Room *, ServerPlayer *player, QVariant &data) const{
        if(data.toString() == "quanji")
            player->removePileByName("power");
        return false;
    }
};

class Zili: public PhaseChangeSkill{
public:
    Zili():PhaseChangeSkill("zili"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && PhaseChangeSkill::triggerable(target)
                && target->getPhase() == Player::Start
                && target->getMark("zili") == 0
                && target->getPile("power").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();

        room->setPlayerMark(zhonghui, "zili", 1);
        zhonghui->gainMark("@waked");
        room->loseMaxHp(zhonghui);

        LogMessage log;
        log.type = "#ZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("power").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        room->broadcastInvoke("animate", "lightbox:$zili:4000");
        room->getThread()->delay(4000);
        QStringList choicelist;
        choicelist << "draw";
        if (zhonghui->getLostHp() != 0)
            choicelist << "recover";
        QString choice;
        if (choicelist.length() >=2)
            choice = room->askForChoice(zhonghui, objectName(), choicelist.join("+"));
        else
            choice = "draw";
        if(choice == "recover"){
            RecoverStruct recover;
            recover.who = zhonghui;
            room->recover(zhonghui, recover);
        }else
            room->drawCards(zhonghui, 2);
        room->acquireSkill(zhonghui, "paiyi");

        return false;
    }
};

PaiyiCard::PaiyiCard(){
    once = true;
}

bool PaiyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    return true;
}

void PaiyiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *zhonghui = card_use.from;
    ServerPlayer *target = card_use.to.first();
    QList<int> powers = zhonghui->getPile("power");
    if(powers.isEmpty())
        return ;

    int card_id;
    if(powers.length() == 1)
        card_id = powers.first();
    else{
        room->fillAG(powers, zhonghui);
        card_id = room->askForAG(zhonghui, powers, true, "paiyi");
        zhonghui->invoke("clearAG");

        if(card_id == -1)
            return;
    }

    if(target==zhonghui)
        room->broadcastSkillInvoke(objectName(),1);
    else
        room->broadcastSkillInvoke(objectName(),2);

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(),
        target->objectName(), "paiyi", QString());
    room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
    room->drawCards(target, 2,"paiyi");
    if(target->getHandcardNum() > zhonghui->getHandcardNum()){
        DamageStruct damage;
        damage.card = NULL;
        damage.from = zhonghui;
        damage.to = target;

        room->damage(damage);
    }
}

class Paiyi:public ZeroCardViewAsSkill{
public:
    Paiyi():ZeroCardViewAsSkill("paiyi"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("power").isEmpty()&&!player->hasUsed("PaiyiCard");
    }

    virtual const Card *viewAs() const{
        return new PaiyiCard;
    }

    virtual Location getLocation() const{
        return Right;
    }
};

YJCMPackage::YJCMPackage():Package("YJCM"){
    /*General *chengong = new General(this, "chengong", "qun", 3);
    chengong->addSkill(new Zhichi);
    chengong->addSkill(new ZhichiClear);
    chengong->addSkill(new Mingce);
    related_skills.insertMulti("zhichi", "#zhichi-clear");

    General *fazheng = new General(this, "fazheng", "shu", 3);
    fazheng->addSkill(new Enyuan);
    fazheng->addSkill(new Xuanhuo);

    General *lingtong = new General(this, "lingtong", "wu");
    lingtong->addSkill(new Xuanfeng);

    General *wuguotai = new General(this, "wuguotai", "wu", 3, false);
    wuguotai->addSkill(new Ganlu);
    wuguotai->addSkill(new Buyi);

    General *yujin = new General(this, "yujin", "wei");
    yujin->addSkill(new Yizhong);
    yujin->addSkill(new Piaonian);

    General *zhangchunhua = new General(this, "zhangchunhua", "wei", 3, false);
    zhangchunhua->addSkill(new Jueqing);
    zhangchunhua->addSkill(new ShangshiStateChanged);
    zhangchunhua->addSkill(new ShangshiCardMove);
    related_skills.insertMulti("shangshi", "#shangshi");

    General *zhonghui = new General(this, "zhonghui", "wei");
    zhonghui->addSkill(new QuanjiKeep);
    zhonghui->addSkill(new QuanjiRemove);
    zhonghui->addSkill(new Quanji);
    zhonghui->addSkill(new Zili);
    zhonghui->addRelateSkill("paiyi");
    related_skills.insertMulti("quanji", "#quanji");
    related_skills.insertMulti("quanji", "#quanji-remove");

    addMetaObject<MingceCard>();
    addMetaObject<GanluCard>();
    addMetaObject<XuanfengCard>();
    addMetaObject<XuanhuoCard>();
    addMetaObject<JujianCard>();
    addMetaObject<PaiyiCard>();
    skills << new Paiyi;*/
}

ADD_PACKAGE(YJCM)
