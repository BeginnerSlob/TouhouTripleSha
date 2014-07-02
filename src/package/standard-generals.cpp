#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "ai.h"
#include "settings.h"
#include "maneuvering.h"

class YijiViewAsSkill: public ZeroCardViewAsSkill {
public:
    YijiViewAsSkill(): ZeroCardViewAsSkill("yiji") {
        response_pattern = "@@yiji";
    }

    virtual const Card *viewAs() const{
        return new YijiCard;
    }
};

class Yiji: public MasochismSkill {
public:
    Yiji(): MasochismSkill("yiji") {
        view_as_skill = new YijiViewAsSkill;
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        for (int i = 0; i < damage.damage; i++) {
            if (target->isAlive() && room->askForSkillInvoke(target, objectName(), QVariant::fromValue(damage))) {
                room->broadcastSkillInvoke(objectName());
                target->drawCards(2, objectName());
                room->askForUseCard(target, "@@yiji", "@yiji");
            } else {
                break;
            }
        }
    }
};

class YijiObtain: public PhaseChangeSkill {
public:
    YijiObtain(): PhaseChangeSkill("#yiji") {
    }

    virtual int getPriority(TriggerEvent) const{
        return 4;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (target->getPhase() == Player::Draw && !target->getPile("yiji").isEmpty()) {
            DummyCard *dummy = new DummyCard(target->getPile("yiji"));
            CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, target->objectName(), "yiji", QString());
            room->obtainCard(target, dummy, reason, false);
            delete dummy;
        }
        return false;
    }
};

class Yajiao: public TriggerSkill {
public:
    Yajiao(): TriggerSkill("yajiao") {
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive) return false;
        const Card *cardstar = NULL;
        bool isHandcard = false;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            cardstar = use.card;
            isHandcard = use.m_isHandcard;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            cardstar = resp.m_card;
            isHandcard = resp.m_isHandcard;
        }
        if (isHandcard && room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, player, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "yajiao", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            room->fillAG(ids, player);
            bool dealt = false;
            if (card->getTypeId() == cardstar->getTypeId()) {
                player->setMark("yajiao", id); // For AI
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                                                                QString("@yajiao-give:::%1:%2\\%3").arg(card->objectName())
                                                                                                   .arg(card->getSuitString() + "_char")
                                                                                                   .arg(card->getNumberString()),
                                                                true);
                if (target) {
                    room->clearAG(player);
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_DRAW, target->objectName(), "yajiao", QString());
                    room->obtainCard(target, card, reason);
                }
            } else {
                QVariant carddata = QVariant::fromValue(card);
                if (room->askForChoice(player, objectName(), "throw+cancel", carddata) == "throw") {
                    room->clearAG(player);
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "yajiao", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
            if (!dealt) {
                room->clearAG(player);
                room->returnToTopDrawPile(ids);
            }
        }
        return false;
    }
};

class Jizhi: public TriggerSkill {
public:
    Jizhi(): TriggerSkill("jizhi") {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->getTypeId() == Card::TypeTrick
            && room->askForSkillInvoke(yueying, objectName())) {
            room->broadcastSkillInvoke(objectName());

            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, yueying, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, yueying->objectName(), "jizhi", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            if (!card->isKindOf("BasicCard")) {
                CardMoveReason reason(CardMoveReason::S_REASON_DRAW, yueying->objectName(), "jizhi", QString());
                room->obtainCard(yueying, card, reason);
            } else {
                const Card *card_ex = NULL;
                if (!yueying->isKongcheng())
                    card_ex = room->askForCard(yueying, ".", "@jizhi-exchange:::" + card->objectName(),
                                               QVariant::fromValue(card), Card::MethodNone);
                if (card_ex) {
                    CardMoveReason reason1(CardMoveReason::S_REASON_PUT, yueying->objectName(), "jizhi", QString());
                    CardMoveReason reason2(CardMoveReason::S_REASON_DRAW, yueying->objectName(), "jizhi", QString());
                    CardsMoveStruct move1(card_ex->getEffectiveId(), yueying, NULL, Player::PlaceUnknown, Player::DrawPile, reason1);
                    CardsMoveStruct move2(ids, yueying, yueying, Player::PlaceUnknown, Player::PlaceHand, reason2);

                    QList<CardsMoveStruct> moves;
                    moves.append(move1);
                    moves.append(move2);
                    room->moveCardsAtomic(moves, false);
                } else {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, yueying->objectName(), "jizhi", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
        }

        return false;
    }
};

class Qicai: public TargetModSkill {
public:
    Qicai(): TargetModSkill("qicai") {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class Zhuhai: public TriggerSkill {
public:
    Zhuhai(): TriggerSkill("zhuhai") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            ServerPlayer *xushu = room->findPlayerBySkillName(objectName());
            if (xushu && xushu != player && xushu->canSlash(player, false)
                && player->hasFlag("ZhuhaiDamage")) {
                xushu->setFlags("ZhuhaiSlash");
                QString prompt = QString("@zhuhai-slash:%1:%2").arg(xushu->objectName()).arg(player->objectName());
                if (!room->askForUseSlashTo(xushu, player, prompt, false))
                    xushu->setFlags("-ZhuhaiSlash");
            }
        } else if (triggerEvent == ChoiceMade && player->hasFlag("ZhuhaiSlash") && data.canConvert<CardUseStruct>()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            player->setFlags("-ZhuhaiSlash");
        }
        return false;
    }
};

class ZhuhaiRecord: public TriggerSkill {
public:
    ZhuhaiRecord(): TriggerSkill("#zhuhai-record") {
        events << PreDamageDone;
        //global = true;
    }

    virtual int getPriority(TriggerEvent) const{
        return 4;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->getPhase() != Player::NotActive && !damage.from->hasFlag("ZhuhaiDamage"))
            damage.from->setFlags("ZhuhaiDamage");
        return false;
    }
};

class Qianxin: public TriggerSkill {
public:
    Qianxin(): TriggerSkill("qianxin") {
        events << Damage;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target)
               && target->getMark("qianxin") == 0
               && target->isWounded();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        room->doLightbox("$QianxinAnimate");

        LogMessage log;
        log.type = "#QianxinWake";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        room->setPlayerMark(player, "qianxin", 1);
        if (room->changeMaxHpForAwakenSkill(player) && player->getMark("qianxin") == 1)
            room->acquireSkill(player, "jianyan");

        return false;
    }
};

class Jianyan: public ZeroCardViewAsSkill {
public:
    Jianyan(): ZeroCardViewAsSkill("jianyan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("JianyanCard");
    }

    virtual const Card *viewAs() const{
        return new JianyanCard;
    }
};

class Qianxun: public TriggerSkill {
public:
    Qianxun(): TriggerSkill("qianxun") {
        events << TrickEffect << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TrickEffect && TriggerSkill::triggerable(player)) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.multiple && effect.card->getTypeId() == Card::TypeTrick
                && (effect.card->isKindOf("DelayedTrick") || effect.from != player)
                && room->askForSkillInvoke(player, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                player->tag["QianxunEffectData"] = data;

                CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, player->objectName(), objectName(), QString());
                QList<int> handcards = player->handCards();
                QList<ServerPlayer *> open;
                open << player;
                player->addToPile("qianxun", handcards, false, open, reason);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getPile("qianxun").length() > 0) {
                        DummyCard *dummy = new DummyCard(p->getPile("qianxun"));
                        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "qianxun", QString());
                        room->obtainCard(p, dummy, reason, false);
                        delete dummy;
                    }
                }
            }
        }
        return false;
    }
};

class LianyingViewAsSkill: public ZeroCardViewAsSkill {
public:
    LianyingViewAsSkill(): ZeroCardViewAsSkill("lianying") {
        response_pattern = "@@lianying";
    }

    virtual const Card *viewAs() const{
        return new LianyingCard;
    }
};

class Lianying: public TriggerSkill {
public:
    Lianying(): TriggerSkill("lianying") {
        events << CardsMoveOneTime;
        view_as_skill = new LianyingViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *luxun, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == luxun && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            luxun->tag["LianyingMoveData"] = data;
            int count = 0;
            for (int i = 0; i < move.from_places.length(); i++) {
                if (move.from_places[i] == Player::PlaceHand) count++;
            }
            room->setPlayerMark(luxun, "lianying", count);
            room->askForUseCard(luxun, "@@lianying", "@lianying-card:::" + QString::number(count));
        }
        return false;
    }
};

class Chuli: public OneCardViewAsSkill {
public:
    Chuli(): OneCardViewAsSkill("chuli") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("ChuliCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ChuliCard *chuli_card = new ChuliCard;
        chuli_card->addSubcard(originalCard->getId());
        return chuli_card;
    }
};

class Xunxun: public PhaseChangeSkill {
public:
    Xunxun(): PhaseChangeSkill("xunxun") {
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *lidian) const{
        if (lidian->getPhase() == Player::Draw) {
            Room *room = lidian->getRoom();
            if (room->askForSkillInvoke(lidian, objectName())) {
                room->broadcastSkillInvoke(objectName());
                QList<ServerPlayer *> p_list;
                p_list << lidian;
                QList<int> card_ids = room->getNCards(4);
                QList<int> obtained;
                room->fillAG(card_ids, lidian);
                int id1 = room->askForAG(lidian, card_ids, false, objectName());
                card_ids.removeOne(id1);
                obtained << id1;
                room->takeAG(lidian, id1, false, p_list);
                int id2 = room->askForAG(lidian, card_ids, false, objectName());
                card_ids.removeOne(id2);
                obtained << id2;
                room->clearAG(lidian);

                room->askForGuanxing(lidian, card_ids, Room::GuanxingDownOnly);
                DummyCard *dummy = new DummyCard(obtained);
                lidian->obtainCard(dummy, false);
                delete dummy;

                return true;
            }
        }

        return false;
    }
};

class Wangxi: public TriggerSkill {
public:
    Wangxi(): TriggerSkill("wangxi") {
        events << Damage << Damaged;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = NULL;
        if (triggerEvent == Damage && !damage.to->hasFlag("Global_DebutFlag"))
            target = damage.to;
        else if (triggerEvent == Damaged)
            target = damage.from;
        if (!target || target == player) return false;
        QList<ServerPlayer *> players;
        players << player << target;
        room->sortByActionOrder(players);

        for (int i = 1; i <= damage.damage; i++) {
            if (!target->isAlive() || !player->isAlive())
                return false;
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target))) {
                room->broadcastSkillInvoke(objectName(), (triggerEvent == Damaged) ? 1 : 2);
                room->drawCards(players, 1, objectName());
            } else {
                break;
            }
        }
        return false;
    }
};

class Yaowu: public TriggerSkill {
public:
    Yaowu(): TriggerSkill("yaowu") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.card->isRed()
            && damage.from && damage.from->isAlive()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(damage.to, objectName());

            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = damage.to;
            log.arg = objectName();
            room->sendLog(log);

            if (damage.from->isWounded() && room->askForChoice(damage.from, objectName(), "recover+draw", data) == "recover")
                room->recover(damage.from, RecoverStruct(damage.to));
            else
                damage.from->drawCards(1, objectName());
        }
        return false;
    }
};

class Xiaoxi: public TriggerSkill {
public:
    Xiaoxi(): TriggerSkill("xiaoxi") {
        events << Debut;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        ServerPlayer *opponent = player->getNext();
        if (!opponent->isAlive())
            return false;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_xiaoxi");
        if (player->isLocked(slash) || !player->canSlash(opponent, slash, false)) {
            delete slash;
            return false;
        }
        if (room->askForSkillInvoke(player, objectName()))
            room->useCard(CardUseStruct(slash, player, opponent));
        return false;
    }
};

void StandardPackage::addGenerals() {
    // Wei

    General *guojia = new General(this, "guojia", "wei", 3); // WEI 006
    guojia->addSkill("iktiandu");
    guojia->addSkill(new Yiji);
    guojia->addSkill(new YijiObtain);
    related_skills.insertMulti("yiji", "#yiji");

    General *lidian = new General(this, "lidian", "wei", 3); // WEI 017
    lidian->addSkill(new Xunxun);
    lidian->addSkill(new Wangxi);

    // Shu
    General *zhaoyun = new General(this, "zhaoyun", "shu"); // SHU 005
    zhaoyun->addSkill("ikhuahuan");
    zhaoyun->addSkill(new Yajiao);

    General *huangyueying = new General(this, "huangyueying", "shu", 3, false); // SHU 007
    huangyueying->addSkill(new Jizhi);
    huangyueying->addSkill(new Qicai);

    General *st_xushu = new General(this, "st_xushu", "shu"); // SHU 017
    st_xushu->addSkill(new Zhuhai);
    st_xushu->addSkill(new ZhuhaiRecord);
    st_xushu->addSkill(new Qianxin);
    st_xushu->addRelateSkill("jianyan");
    related_skills.insertMulti("zhuhai", "#zhuhai-record");

    // Wu

    General *luxun = new General(this, "luxun", "wu", 3); // WU 007
    luxun->addSkill(new Qianxun);
    luxun->addSkill(new Lianying);

    // Qun
    General *huatuo = new General(this, "huatuo", "qun", 3); // QUN 001
    huatuo->addSkill(new Chuli);
    huatuo->addSkill("ikhuichun");

    General *st_huaxiong = new General(this, "st_huaxiong", "qun", 6); // QUN 019
    st_huaxiong->addSkill(new Yaowu);

    // for skill cards
    addMetaObject<ChuliCard>();
    addMetaObject<LianyingCard>();
    addMetaObject<YijiCard>();
    addMetaObject<JianyanCard>();

    skills << new Xiaoxi << new Jianyan;
}

class SuperMaxCards: public MaxCardsSkill {
public:
    SuperMaxCards(): MaxCardsSkill("super_max_cards") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return target->getMark("@max_cards_test");
        return 0;
    }
};

class SuperOffensiveDistance: public DistanceSkill {
public:
    SuperOffensiveDistance(): DistanceSkill("super_offensive_distance") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName()))
            return -from->getMark("@offensive_distance_test");
        else
            return 0;
    }
};

class SuperDefensiveDistance: public DistanceSkill {
public:
    SuperDefensiveDistance(): DistanceSkill("super_defensive_distance") {
    }

    virtual int getCorrect(const Player *, const Player *to) const{
        if (to->hasSkill(objectName()))
            return to->getMark("@defensive_distance_test");
        else
            return 0;
    }
};

#include "sp.h"
class SuperYongsi: public Yongsi {
public:
    SuperYongsi(): Yongsi() {
        setObjectName("super_yongsi");
    }

    virtual int getKingdoms(ServerPlayer *yuanshu) const{
        return yuanshu->getMark("@yongsi_test");
    }
};

#include "wind.h"
class SuperJushou: public Jushou {
public:
    SuperJushou(): Jushou() {
        setObjectName("super_jushou");
    }

    virtual int getJushouDrawNum(ServerPlayer *caoren) const{
        return caoren->getMark("@jushou_test");
    }
};

#include "ikai-ki.h"
#include "maneuvering.h"
class GdJuejing: public TriggerSkill {
public:
    GdJuejing(): TriggerSkill("gdjuejing") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *gaodayihao, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != gaodayihao && move.to != gaodayihao)
                return false;
            if (move.to_place != Player::PlaceHand && !move.from_places.contains(Player::PlaceHand))
                return false;
        }
        if (gaodayihao->getHandcardNum() == 4)
            return false;
        int diff = abs(gaodayihao->getHandcardNum() - 4);
        if (gaodayihao->getHandcardNum() < 4) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = gaodayihao;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(gaodayihao, objectName());
            gaodayihao->drawCards(diff, objectName());
        } else if (gaodayihao->getHandcardNum() > 4) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = gaodayihao;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(gaodayihao, objectName());
            room->askForDiscard(gaodayihao, objectName(), diff, diff);
        }

        return false;
    }
};

class GdJuejingSkipDraw: public DrawCardsSkill {
public:
    GdJuejingSkipDraw(): DrawCardsSkill("#gdjuejing") {
    }

    virtual int getPriority(TriggerEvent) const{
        return 1;
    }

    virtual int getDrawNum(ServerPlayer *gaodayihao, int n) const{
        LogMessage log;
        log.type = "#GdJuejing";
        log.from = gaodayihao;
        log.arg = "gdjuejing";
        gaodayihao->getRoom()->sendLog(log);

        return 0;
    }
};

class GdLonghun: public IkZhihun {
public:
    GdLonghun(): IkZhihun() {
        setObjectName("gdlonghun");
    }

    virtual int getEffHp(const Player *) const{
        return 1;
    }
};

class GdLonghunDuojian: public TriggerSkill {
public:
    GdLonghunDuojian(): TriggerSkill("#gdlonghun-duojian") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *gaodayihao, QVariant &) const{
        if (gaodayihao->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, room->getOtherPlayers(gaodayihao)) {
               if (p->getWeapon() && p->getWeapon()->isKindOf("QinggangSword")) {
                   if (room->askForSkillInvoke(gaodayihao, "gdlonghun")) {
                       room->broadcastSkillInvoke("gdlonghun", 5);
                       gaodayihao->obtainCard(p->getWeapon());
                    }
                    break;
                }
            }
        }

        return false;
    }
};

TestPackage::TestPackage()
    : Package("test")
{
    // for test only
    General *gaodayihao = new General(this, "gaodayihao", "god", 1, true, true);
    gaodayihao->addSkill(new GdJuejing);
    gaodayihao->addSkill(new GdJuejingSkipDraw);
    related_skills.insertMulti("gdjuejing", "#gdjuejing");
    gaodayihao->addSkill(new GdLonghun);
    gaodayihao->addSkill(new GdLonghunDuojian);
    related_skills.insertMulti("gdlonghun", "#gdlonghun-duojian");

    General *super_yuanshu = new General(this, "super_yuanshu", "qun", 4, true, true);
    super_yuanshu->addSkill(new SuperYongsi);
    super_yuanshu->addSkill(new MarkAssignSkill("@yongsi_test", 4));
    related_skills.insertMulti("super_yongsi", "#@yongsi_test-4");
    super_yuanshu->addSkill("weidi");

    General *super_caoren = new General(this, "super_caoren", "wei", 4, true, true);
    super_caoren->addSkill(new SuperJushou);
    super_caoren->addSkill(new MarkAssignSkill("@jushou_test", 5));
    related_skills.insertMulti("super_jushou", "#@jushou_test-5");

    General *nobenghuai_dongzhuo = new General(this, "nobenghuai_dongzhuo$", "qun", 4, true, true);
    nobenghuai_dongzhuo->addSkill("ikfusheng");
    nobenghuai_dongzhuo->addSkill("ikhuanbei");
    nobenghuai_dongzhuo->addSkill("ikwuhua");

    new General(this, "sujiang", "god", 5, true, true);
    new General(this, "sujiangf", "god", 5, false, true);

    new General(this, "anjiang", "god", 4, true, true, true);

    skills << new SuperMaxCards << new SuperOffensiveDistance << new SuperDefensiveDistance;
}

ADD_PACKAGE(Test)

