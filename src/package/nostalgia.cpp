#include "standard.h"
#include "skill.h"
#include "wind.h"
#include "client.h"
#include "engine.h"
#include "nostalgia.h"
#include "yjcm.h"
#include "yjcm2013.h"
#include "settings.h"

class MoonSpearSkill: public WeaponSkill {
public:
    MoonSpearSkill(): WeaponSkill("moon_spear") {
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive)
            return false;

        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            card = card_use.card;
        } else if (triggerEvent == CardResponded) {
            card = data.value<CardResponseStruct>().m_card;
        }

        if (card == NULL || !card->isBlack()
            || (card->getHandlingMethod() != Card::MethodUse && card->getHandlingMethod() != Card::MethodResponse))
            return false;

        player->setFlags("MoonspearUse");
        if (!room->askForUseCard(player, "slash", "@moon-spear-slash", -1, Card::MethodUse, false))
            player->setFlags("-MoonspearUse");

        return false;
    }
};

MoonSpear::MoonSpear(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("moon_spear");
}

NostalgiaPackage::NostalgiaPackage()
    : Package("nostalgia")
{
    type = CardPack;

    Card *moon_spear = new MoonSpear;
    moon_spear->setParent(this);

    skills << new MoonSpearSkill;
}

// old yjcm's generals

class NosWuyan: public TriggerSkill {
public:
    NosWuyan(): TriggerSkill("noswuyan") {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.to == effect.from)
            return false;
        if (effect.card->isNDTrick()) {
            if (effect.from && effect.from->hasSkill(objectName())) {
                LogMessage log;
                log.type = "#WuyanBaD";
                log.from = effect.from;
                log.to << effect.to;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(effect.from, objectName());
                room->broadcastSkillInvoke("noswuyan");
                return true;
            }
            if (effect.to->hasSkill(objectName()) && effect.from) {
                LogMessage log;
                log.type = "#WuyanGooD";
                log.from = effect.to;
                log.to << effect.from;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(effect.to, objectName());
                room->broadcastSkillInvoke("noswuyan");
                return true;
            }
        }
        return false;
    }
};

NosJujianCard::NosJujianCard() {
}

void NosJujianCard::onEffect(const CardEffectStruct &effect) const{
    int n = subcardsLength();
    effect.to->drawCards(n, "nosjujian");
    Room *room = effect.from->getRoom();

    if (effect.from->isAlive() && n == 3) {
        QSet<Card::CardType> types;
        foreach (int card_id, effect.card->getSubcards())
            types << Sanguosha->getCard(card_id)->getTypeId();

        if (types.size() == 1) {
            LogMessage log;
            log.type = "#JujianRecover";
            log.from = effect.from;
            const Card *card = Sanguosha->getCard(subcards.first());
            log.arg = card->getType();
            room->sendLog(log);
            room->recover(effect.from, RecoverStruct(effect.from));
        }
    }
}

class NosJujian: public ViewAsSkill {
public:
    NosJujian(): ViewAsSkill("nosjujian") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.length() < 3 && !Self->isJilei(to_select);
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("NosJujianCard");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        NosJujianCard *card = new NosJujianCard;
        card->addSubcards(cards);
        return card;
    }
};

class NosEnyuan: public TriggerSkill {
public:
    NosEnyuan(): TriggerSkill("nosenyuan") {
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == HpRecover) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.who && recover.who != player) {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                room->broadcastSkillInvoke("nosenyuan", qrand() % 2 + 1);
                room->notifySkillInvoked(player, objectName());
                recover.who->drawCards(recover.recover, objectName());
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (source && source != player) {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                room->broadcastSkillInvoke("nosenyuan", qrand() % 2 + 3);
                room->notifySkillInvoked(player, objectName());

                const Card *card = room->askForCard(source, ".|heart|.|hand", "@nosenyuan-heart", data, Card::MethodNone);
                if (card)
                    player->obtainCard(card);
                else
                    room->loseHp(source);
            }
        }

        return false;
    }
};

NosXuanhuoCard::NosXuanhuoCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosXuanhuoCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(this);

    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "he", "nosxuanhuo");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "nosxuanhuo", "@nosxuanhuo-give:" + effect.to->objectName());
    if (target != effect.from) {
        CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), target->objectName(), "nosxuanhuo", QString());
        room->obtainCard(target, Sanguosha->getCard(card_id), reason2, false);
    }
}

class NosXuanhuo: public OneCardViewAsSkill {
public:
    NosXuanhuo():OneCardViewAsSkill("nosxuanhuo") {
        filter_pattern = ".|heart|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("NosXuanhuoCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        NosXuanhuoCard *xuanhuoCard = new NosXuanhuoCard;
        xuanhuoCard->addSubcard(originalCard);
        return xuanhuoCard;
    }
};

class NosXuanfeng: public TriggerSkill {
public:
    NosXuanfeng(): TriggerSkill("nosxuanfeng") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lingtong, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == lingtong && move.from_places.contains(Player::PlaceEquip)) {
                QStringList choicelist;
                choicelist << "nothing";
                QList<ServerPlayer *> targets1;
                foreach (ServerPlayer *target, room->getAlivePlayers()) {
                    if (lingtong->canSlash(target, NULL, false))
                        targets1 << target;
                }
                Slash *slashx = new Slash(Card::NoSuit, 0);
                if (!targets1.isEmpty() && !lingtong->isCardLimited(slashx, Card::MethodUse))
                    choicelist << "slash";
                slashx->deleteLater();
                QList<ServerPlayer *> targets2;
                foreach (ServerPlayer *p, room->getOtherPlayers(lingtong)) {
                    if (lingtong->distanceTo(p) <= 1)
                        targets2 << p;
                }
                if (!targets2.isEmpty()) choicelist << "damage";

                QString choice = room->askForChoice(lingtong, objectName(), choicelist.join("+"));
                if (choice == "slash") {
                    ServerPlayer *target = room->askForPlayerChosen(lingtong, targets1, "nosxuanfeng_slash", "@dummy-slash");
                    room->broadcastSkillInvoke(objectName(), 1);
                    Slash *slash = new Slash(Card::NoSuit, 0);
                    slash->setSkillName(objectName());
                    room->useCard(CardUseStruct(slash, lingtong, target));
                } else if (choice == "damage") {
                    room->broadcastSkillInvoke(objectName(), 2);

                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = lingtong;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(lingtong, objectName());

                    ServerPlayer *target = room->askForPlayerChosen(lingtong, targets2, "nosxuanfeng_damage", "@nosxuanfeng-damage");
                    room->damage(DamageStruct("nosxuanfeng", lingtong, target));
                }
            }
        }

        return false;
    }
};

class NosShangshi: public Shangshi {
public:
    NosShangshi(): Shangshi() {
        setObjectName("nosshangshi");
    }

    virtual int getMaxLostHp(ServerPlayer *zhangchunhua) const{
        return qMin(zhangchunhua->getLostHp(), zhangchunhua->getMaxHp());
    }
};

class NosFuhun: public TriggerSkill {
public:
    NosFuhun(): TriggerSkill("nosfuhun") {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *shuangying, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && shuangying->getPhase() ==  Player::Draw && TriggerSkill::triggerable(shuangying)) {
            if (shuangying->askForSkillInvoke(objectName())) {
                int card1 = room->drawCard();
                int card2 = room->drawCard();
                QList<int> ids;
                ids << card1 << card2;
                bool diff = (Sanguosha->getCard(card1)->getColor() != Sanguosha->getCard(card2)->getColor());

                CardsMoveStruct move;
                move.card_ids = ids;
                move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, shuangying->objectName(), "fuhun", QString());
                move.to_place = Player::PlaceTable;
                room->moveCardsAtomic(move, true);
                room->getThread()->delay();

                DummyCard *dummy = new DummyCard(move.card_ids);
                room->obtainCard(shuangying, dummy);
                delete dummy;

                if (diff) {
                    room->handleAcquireDetachSkills(shuangying, "ikchilian|iklipao");
                    room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
                    shuangying->setFlags(objectName());
                } else {
                    room->broadcastSkillInvoke(objectName(), 3);
                }

                return true;
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && shuangying->hasFlag(objectName()))
                room->handleAcquireDetachSkills(shuangying, "-ikchilian|-iklipao", true);
        }

        return false;
    }
};

class NosGongqi: public OneCardViewAsSkill {
public:
    NosGongqi(): OneCardViewAsSkill("nosgongqi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const Card *to_select) const{
        if (to_select->getTypeId() != Card::TypeEquip)
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(to_select->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard);
        slash->setSkillName(objectName());
        return slash;
    }
};

class NosGongqiTargetMod: public TargetModSkill {
public:
    NosGongqiTargetMod(): TargetModSkill("#nosgongqi-target") {
        frequency = NotFrequent;
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const{
        if (card->getSkillName() == "nosgongqi")
            return 1000;
        else
            return 0;
    }
};

NosJiefanCard::NosJiefanCard() {
    target_fixed = true;
    mute = true;
}

void NosJiefanCard::use(Room *room, ServerPlayer *handang, QList<ServerPlayer *> &) const{
    ServerPlayer *current = room->getCurrent();
    if (!current || current->isDead() || current->getPhase() == Player::NotActive) return;
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    handang->setFlags("NosJiefanUsed");
    room->setTag("NosJiefanTarget", QVariant::fromValue(who));
    bool use_slash = room->askForUseSlashTo(handang, current, "nosjiefan-slash:" + current->objectName(), false);
    if (!use_slash) {
        handang->setFlags("-NosJiefanUsed");
        room->removeTag("NosJiefanTarget");
        room->setPlayerFlag(handang, "Global_NosJiefanFailed");
    }
}

class NosJiefanViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosJiefanViewAsSkill(): ZeroCardViewAsSkill("nosjiefan") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (!pattern.contains("peach")) return false;
        if (player->hasFlag("Global_NosJiefanFailed")) return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getPhase() != Player::NotActive)
                return true;
        }
        return false;
    }

    virtual const Card *viewAs() const{
        return new NosJiefanCard;
    }
};

class NosJiefan: public TriggerSkill {
public:
    NosJiefan(): TriggerSkill("nosjiefan") {
        events << DamageCaused << CardFinished << PreCardUsed;
        view_as_skill = new NosJiefanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *handang, QVariant &data) const{
        if (triggerEvent == PreCardUsed) {
            if (!handang->hasFlag("NosJiefanUsed"))
                return false;

            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                handang->setFlags("-NosJiefanUsed");
                room->setCardFlag(use.card, "nosjiefan-slash");
            }
        } else if (triggerEvent == DamageCaused) {
            ServerPlayer *current = room->getCurrent();
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->hasFlag("nosjiefan-slash")) {
                LogMessage log2;
                log2.type = "#NosJiefanPrevent";
                log2.from = handang;
                log2.to << damage.to;
                room->sendLog(log2);

                ServerPlayer *target = room->getTag("NosJiefanTarget").value<ServerPlayer *>();
                if (target && target->getHp() > 0) {
                    LogMessage log;
                    log.type = "#NosJiefanNull1";
                    log.from = target;
                    room->sendLog(log);
                } else if (target && target->isDead()) {
                    LogMessage log;
                    log.type = "#NosJiefanNull2";
                    log.from = target;
                    log.to << handang;
                    room->sendLog(log);
                } else if (handang->hasFlag("Global_PreventPeach")) {
                    LogMessage log;
                    log.type = "#NosJiefanNull3";
                    log.from = current;
                    room->sendLog(log);
                } else {
                    Peach *peach = new Peach(Card::NoSuit, 0);
                    peach->setSkillName("_nosjiefan");

                    room->setCardFlag(damage.card, "nosjiefan_success");
                    if ((target->getGeneralName().contains("sunquan")
                         || target->getGeneralName().contains("sunce")
                         || target->getGeneralName().contains("sunjian"))
                        && target->isLord())
                        handang->setFlags("NosJiefanToLord");
                    room->useCard(CardUseStruct(peach, handang, target));
                    handang->setFlags("-NosJiefanToLord");
                }
                return true;
            }
            return false;
        } else if (triggerEvent == CardFinished && !room->getTag("NosJiefanTarget").isNull()) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->hasFlag("nosjiefan-slash")) {
                if (!use.card->hasFlag("nosjiefan_success"))
                    room->setPlayerFlag(handang, "Global_NosJiefanFailed");
                room->removeTag("NosJiefanTarget");
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        if (player->hasFlag("NosJiefanToLord"))
            return 2;
        else
            return 1;
    }
};

class NosQianxi: public TriggerSkill {
public:
    NosQianxi(): TriggerSkill("nosqianxi") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if (player->distanceTo(damage.to) == 1 && damage.card && damage.card->isKindOf("Slash")
            && damage.by_user && !damage.chain && !damage.transfer && player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName(), 1);
            JudgeStruct judge;
            judge.pattern = ".|heart";
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

class IkLundao: public TriggerSkill {
public:
    IkLundao(): TriggerSkill("iklundao") {
        events << AskForRetrial;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        room->retrial(Sanguosha->getCard(room->drawCard()), player, judge, objectName());
        return false;
    }
};

class IkXuanwu: public PhaseChangeSkill {
public:
    IkXuanwu(): PhaseChangeSkill("ikxuanwu") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *wangyi) const{
        return PhaseChangeSkill::triggerable(wangyi)
            && wangyi->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *wangyi) const{
        Room *room = wangyi->getRoom();
        JudgeStruct judge;
        judge.pattern = ".|black";
        judge.good = true;
        judge.reason = objectName();
        judge.who = wangyi;

        room->judge(judge);

        if (judge.isGood() && wangyi->isAlive()) {
            QList<int> pile_ids = room->getNCards(wangyi->getLostHp() + 1, false);
            room->fillAG(pile_ids, wangyi);
            ServerPlayer *target = room->askForPlayerChosen(wangyi, room->getAllPlayers(), objectName());
            room->clearAG(wangyi);

            DummyCard *dummy = new DummyCard(pile_ids);
            wangyi->setFlags("Global_GongxinOperator");
            target->obtainCard(dummy, false);
            wangyi->setFlags("-Global_GongxinOperator");
            delete dummy;
        }

        return false;
    }
};

class NosChengxiang: public Chengxiang {
public:
    NosChengxiang(): Chengxiang() {
        setObjectName("noschengxiang");
        total_point = 12;
    }
};

NosRenxinCard::NosRenxinCard() {
    target_fixed = true;
    mute = true;
}

void NosRenxinCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const{
    if (player->isKongcheng()) return;
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    room->broadcastSkillInvoke("renxin");
    DummyCard *handcards = player->wholeHandCards();
    player->turnOver();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), who->objectName(), "nosrenxin", QString());
    room->obtainCard(who, handcards, reason, false);
    delete handcards;
    room->recover(who, RecoverStruct(player));
}

class NosRenxin: public ZeroCardViewAsSkill {
public:
    NosRenxin(): ZeroCardViewAsSkill("nosrenxin") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "peach" && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new NosRenxinCard;
    }
};

class NosDanshou: public TriggerSkill {
public:
    NosDanshou(): TriggerSkill("nosdanshou") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (room->askForSkillInvoke(player, objectName(), data)) {
            player->drawCards(1, objectName());
            ServerPlayer *current = room->getCurrent();
            if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
                room->broadcastSkillInvoke("danshou");
                LogMessage log;
                log.type = "#SkipAllPhase";
                log.from = current;
                room->sendLog(log);
            }
            throw TurnBroken;
        }
        return false;
    }
};

class NosJuece: public TriggerSkill {
public:
    NosJuece(): TriggerSkill("nosjuece") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() != Player::NotActive && move.from && move.from_places.contains(Player::PlaceHand)
            && move.is_last_handcard) {
            ServerPlayer *from = (ServerPlayer *)move.from;
            if (from->getHp() > 0 && room->askForSkillInvoke(player, objectName(), data)) {
                room->broadcastSkillInvoke("juece");
                room->damage(DamageStruct(objectName(), player, from));
            }
        }
        return false;
    }
};

class NosMieji: public TargetModSkill {
public:
    NosMieji(): TargetModSkill("#nosmieji") {
        pattern = "SingleTargetTrick|black"; // deal with Ex Nihilo and Collateral later
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasSkill("nosmieji"))
            return 1;
        else
            return 0;
    }
};

class NosMiejiViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosMiejiViewAsSkill(): ZeroCardViewAsSkill("nosmieji") {
        response_pattern = "@@nosmieji";
    }

    virtual const Card *viewAs() const{
        return new ExtraCollateralCard;
    }
};

class NosMiejiForExNihiloAndCollateral: public TriggerSkill {
public:
    NosMiejiForExNihiloAndCollateral(): TriggerSkill("nosmieji") {
        events << PreCardUsed;
        frequency = Compulsory;
        view_as_skill = new NosMiejiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isBlack() && (use.card->isKindOf("ExNihilo") || use.card->isKindOf("Collateral"))) {
            QList<ServerPlayer *> targets;
            ServerPlayer *extra = NULL;
            if (use.card->isKindOf("ExNihilo")) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!use.to.contains(p) && !room->isProhibited(player, p, use.card))
                        targets << p;
                }
                if (targets.isEmpty()) return false;
                extra = room->askForPlayerChosen(player, targets, objectName(), "@qiaoshui-add:::" + use.card->objectName(), true);
            } else if (use.card->isKindOf("Collateral")) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                    if (use.card->targetFilter(QList<const Player *>(), p, player))
                        targets << p;
                }
                if (targets.isEmpty()) return false;

                QStringList tos;
                foreach (ServerPlayer *t, use.to)
                    tos.append(t->objectName());
                room->setPlayerProperty(player, "extra_collateral", use.card->toString());
                room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
                bool used = room->askForUseCard(player, "@@nosmieji", "@qiaoshui-add:::collateral");
                room->setPlayerProperty(player, "extra_collateral", QString());
                room->setPlayerProperty(player, "extra_collateral_current_targets", QString());
                if (!used) return false;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasFlag("ExtraCollateralTarget")) {
                        p->setFlags("-ExtraCollateralTarget");
                        extra = p;
                        break;
                    }
                }
            }
            if (!extra) return false;
            room->broadcastSkillInvoke(objectName());
            use.to.append(extra);
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);

            LogMessage log;
            log.type = "#QiaoshuiAdd";
            log.from = player;
            log.to << extra;
            log.arg = use.card->objectName();
            log.arg2 = objectName();
            room->sendLog(log);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());

            if (use.card->isKindOf("Collateral")) {
                ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                if (victim) {
                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = player;
                    log.to << victim;
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                }
            }
        }
        return false;
    }
};

class NosMiejiEffect: public TriggerSkill {
public:
    NosMiejiEffect(): TriggerSkill("#nosmieji-effect") {
        events << PreCardUsed;
    }

    virtual int getPriority(TriggerEvent) const{
        return 6;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SingleTargetTrick") && !use.card->targetFixed() && use.to.length() > 1
            && use.card->isBlack() && use.from->hasSkill("nosmieji"))
            room->broadcastSkillInvoke("mieji");
        return false;
    }
};

class NosFencheng: public ZeroCardViewAsSkill {
public:
    NosFencheng(): ZeroCardViewAsSkill("nosfencheng") {
        frequency = Limited;
        limit_mark = "@nosburn";
    }

    virtual const Card *viewAs() const{
        return new NosFenchengCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@nosburn") >= 1;
    }
};

NosFenchengCard::NosFenchengCard() {
    mute = true;
    target_fixed = true;
}

void NosFenchengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@nosburn");
    room->broadcastSkillInvoke("fencheng");
    room->doLightbox("$NosFenchengAnimate", 3000);

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    source->setFlags("NosFenchengUsing");
    try {
        foreach (ServerPlayer *player, players) {
            if (player->isAlive())
                room->cardEffect(this, source, player);
                room->getThread()->delay();
        }
        source->setFlags("-NosFenchengUsing");
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            source->setFlags("-NosFenchengUsing");
        throw triggerEvent;
    }
}

void NosFenchengCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    int length = qMax(1, effect.to->getEquips().length());
    if (!effect.to->canDiscard(effect.to, "he") || !room->askForDiscard(effect.to, "nosfencheng", length, length, true, true))
        room->damage(DamageStruct("nosfencheng", effect.from, effect.to, 1, DamageStruct::Fire));
}

class NosZhuikong: public TriggerSkill {
public:
    NosZhuikong(): TriggerSkill("noszhuikong") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        bool skip = false;
        foreach (ServerPlayer *fuhuanghou, room->getAllPlayers()) {
            if (TriggerSkill::triggerable(fuhuanghou)
                && player != fuhuanghou && fuhuanghou->isWounded() && !fuhuanghou->isKongcheng()
                && room->askForSkillInvoke(fuhuanghou, objectName())) {
                room->broadcastSkillInvoke("zhuikong");
                if (fuhuanghou->pindian(player, objectName(), NULL)) {
                    if (!skip) {
                        player->skip(Player::Play);
                        skip = true;
                    }
                } else {
                    room->setFixedDistance(player, fuhuanghou, 1);
                    QVariantList zhuikonglist = player->tag[objectName()].toList();
                    zhuikonglist.append(QVariant::fromValue(fuhuanghou));
                    player->tag[objectName()] = QVariant::fromValue(zhuikonglist);
                }
            }
        }
        return false;
    }
};

class NosZhuikongClear: public TriggerSkill {
public:
    NosZhuikongClear(): TriggerSkill("#noszhuikong-clear") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;

        QVariantList zhuikonglist = player->tag["noszhuikong"].toList();
        if (zhuikonglist.isEmpty()) return false;
        foreach (QVariant p, zhuikonglist) {
            ServerPlayer *fuhuanghou = p.value<ServerPlayer *>();
            room->setFixedDistance(player, fuhuanghou, -1);
        }
        player->tag.remove("noszhuikong");
        return false;
    }
};

class NosQiuyuan: public TriggerSkill {
public:
    NosQiuyuan(): TriggerSkill("nosqiuyuan") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isKongcheng() && p != use.from)
                    targets << p;
            }
            if (targets.isEmpty()) return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "nosqiuyuan-invoke", true, true);
            if (target) {
                if (target->getGeneralName().contains("fuwan") || target->getGeneral2Name().contains("fuwan"))
                    room->broadcastSkillInvoke("qiuyuan", 2);
                else
                    room->broadcastSkillInvoke("qiuyuan", 1);
                const Card *card = NULL;
                if (target->getHandcardNum() > 1) {
                    card = room->askForCard(target, ".!", "@nosqiuyuan-give:" + player->objectName(), data, Card::MethodNone);
                    if (!card)
                        card = target->getHandcards().at(qrand() % target->getHandcardNum());
                } else {
                    Q_ASSERT(target->getHandcardNum() == 1);
                    card = target->getHandcards().first();
                }
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), "nosqiuyuan", QString());
                room->obtainCard(player, card, reason);
                room->showCard(player, card->getEffectiveId());
                if (!card->isKindOf("Jink")) {
                    if (use.from->canSlash(target, use.card, false)) {
                        LogMessage log;
                        log.type = "#BecomeTarget";
                        log.from = target;
                        log.card_str = use.card->toString();
                        room->sendLog(log);

                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());

                        use.to.append(target);
                        room->sortByActionOrder(use.to);
                        data = QVariant::fromValue(use);
                        room->getThread()->trigger(TargetConfirming, room, target, data);
                    }
                }
            }
        }
        return false;
    }
};

class NosZhenggong: public MasochismSkill {
public:
    NosZhenggong(): MasochismSkill("noszhenggong") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("nosbaijiang") == 0;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const{
        if (damage.from && damage.from->hasEquip()) {
            QVariant data = QVariant::fromValue(damage.from);
            if (!zhonghui->askForSkillInvoke(objectName(), data))
                return;

            Room *room = zhonghui->getRoom();
            room->broadcastSkillInvoke(objectName());
            int equip = room->askForCardChosen(zhonghui, damage.from, "e", objectName());
            const Card *card = Sanguosha->getCard(equip);

            int equip_index = -1;
            const EquipCard *equipcard = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equipcard->location());

            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct move1(equip, zhonghui, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_ROB, zhonghui->objectName()));
            exchangeMove.push_back(move1);
            if (zhonghui->getEquip(equip_index) != NULL) {
                CardsMoveStruct move2(zhonghui->getEquip(equip_index)->getId(), NULL, Player::DiscardPile,
                                      CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, zhonghui->objectName()));
                exchangeMove.push_back(move2);
            }
            room->moveCardsAtomic(exchangeMove, true);
        }
    }
};

class NosQuanji: public TriggerSkill {
public:
    NosQuanji(): TriggerSkill("nosquanji") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        bool skip = false;
        foreach (ServerPlayer *zhonghui, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(zhonghui) || zhonghui == player || zhonghui->isKongcheng()
                || zhonghui->getMark("nosbaijiang") > 0 || player->isKongcheng())
                continue;

            if (room->askForSkillInvoke(zhonghui, "nosquanji")) {
                room->broadcastSkillInvoke(objectName(), 1);
                if (zhonghui->pindian(player, objectName(), NULL)) {
                    if (!skip) {
                        room->broadcastSkillInvoke(objectName(), 2);
                        player->skip(Player::Start);
                        player->skip(Player::Judge);
                        skip = true;
                    } else {
                        room->broadcastSkillInvoke(objectName(), 3);
                    }
                }
            }
        }
        return skip;
    }
};

class NosBaijiang: public PhaseChangeSkill {
public:
    NosBaijiang(): PhaseChangeSkill("nosbaijiang") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("nosbaijiang") == 0
               && target->getPhase() == Player::Start
               && target->getEquips().length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#NosBaijiangWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getEquips().length());
        log.arg2 = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$NosBaijiangAnimate", 5000);

        room->setPlayerMark(zhonghui, "nosbaijiang", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui, 1)) {
            room->recover(zhonghui, RecoverStruct(zhonghui));
            if (zhonghui->getMark("nosbaijiang") == 1)
                room->handleAcquireDetachSkills(zhonghui, "-noszhenggong|-nosquanji|nosyexin");
        }

        return false;
    }
};

NosYexinCard::NosYexinCard() {
    target_fixed = true;
}

void NosYexinCard::onUse(Room *, const CardUseStruct &card_use) const{
    ServerPlayer *zhonghui = card_use.from;

    QList<int> powers = zhonghui->getPile("nospower");
    if (powers.isEmpty())
        return;
    zhonghui->exchangeFreelyFromPrivatePile("nosyexin", "nospower");
}

class NosYexinViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosYexinViewAsSkill(): ZeroCardViewAsSkill("nosyexin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("nospower").isEmpty() && !player->hasUsed("NosYexinCard");
    }

    virtual const Card *viewAs() const{
        return new NosYexinCard;
    }
};

class NosYexin: public TriggerSkill {
public:
    NosYexin(): TriggerSkill("nosyexin") {
        events << Damage << Damaged;
        view_as_skill = new NosYexinViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *zhonghui, QVariant &) const{
        if (!zhonghui->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName(), 1);
        zhonghui->addToPile("nospower", room->drawCard());

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 2;
    }
};

class NosPaiyi: public PhaseChangeSkill {
public:
    NosPaiyi(): PhaseChangeSkill("nospaiyi") {
        _m_place["Judging"] = Player::PlaceDelayedTrick;
        _m_place["Equip"] = Player::PlaceEquip;
        _m_place["Hand"] = Player::PlaceHand;
    }

    QString getPlace(Room *room, ServerPlayer *player, QStringList places) const{
        if (places.length() > 0) {
            QString place = room->askForChoice(player, "nospaiyi", places.join("+"));
            return place;
        }
        return QString();
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        if (zhonghui->getPhase() != Player::Finish || zhonghui->getPile("nospower").isEmpty())
            return false;

        Room *room = zhonghui->getRoom();
        QList<int> powers = zhonghui->getPile("nospower");
        if (powers.isEmpty() || !room->askForSkillInvoke(zhonghui, objectName()))
            return false;
        QStringList places;
        places << "Hand";

        room->fillAG(powers, zhonghui);
        int power = room->askForAG(zhonghui, powers, false, "nospaiyi");
        room->clearAG(zhonghui);

        if (power == -1)
            power = powers.first();

        const Card *card = Sanguosha->getCard(power);

        ServerPlayer *target = room->askForPlayerChosen(zhonghui, room->getAlivePlayers(), "nospaiyi", "@nospaiyi-to:::" + card->objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, zhonghui->objectName(), "nospaiyi", QString());

        if (card->isKindOf("DelayedTrick")) {
            if (!zhonghui->isProhibited(target, card) && !target->containsTrick(card->objectName()))
                places << "Judging";
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        } else if (card->isKindOf("EquipCard")) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            if (!target->getEquip(equip->location()))
                places << "Equip";
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        } else
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);

        int index = 1;
        if (target != zhonghui) {
            index++;
            room->drawCards(zhonghui, 1, objectName());
        }
        room->broadcastSkillInvoke(objectName(), index);

        return false;
    }

private:
    QMap<QString, Player::Place> _m_place;
};

class NosZili: public PhaseChangeSkill {
public:
    NosZili(): PhaseChangeSkill("noszili") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("noszili") == 0
               && target->getPhase() == Player::Start
               && target->getPile("nospower").length() >= 4;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#NosZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("nospower").length());
        log.arg2 = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$NosZiliAnimate", 5000);

        room->setPlayerMark(zhonghui, "noszili", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui) && zhonghui->getMark("noszili") == 1)
            room->acquireSkill(zhonghui, "nospaiyi");

        return false;
    }
};

class NosGuixin: public PhaseChangeSkill {
public:
    NosGuixin(): PhaseChangeSkill("nosguixin") {
    }

    virtual bool onPhaseChange(ServerPlayer *weiwudi) const{
        if (weiwudi->getPhase() != Player::Finish)
            return false;

        Room *room = weiwudi->getRoom();
        if (!room->askForSkillInvoke(weiwudi, objectName()))
            return false;

        QString choice = room->askForChoice(weiwudi, objectName(), "modify+obtain");

        int index = qrand() % 2 + 1;

        if (choice == "modify") {
            ServerPlayer *to_modify = room->askForPlayerChosen(weiwudi, room->getOtherPlayers(weiwudi), objectName());
            QStringList kingdomList = Sanguosha->getKingdoms();
            kingdomList.removeOne("kami");
            QString old_kingdom = to_modify->getKingdom();
            kingdomList.removeOne(old_kingdom);
            if (kingdomList.isEmpty()) return false;
            QString kingdom = room->askForChoice(weiwudi, "nosguixin_kingdom", kingdomList.join("+"), QVariant::fromValue(to_modify));
            room->setPlayerProperty(to_modify, "kingdom", kingdom);

            room->broadcastSkillInvoke(objectName(), index);

            LogMessage log;
            log.type = "#ChangeKingdom";
            log.from = weiwudi;
            log.to << to_modify;
            log.arg = old_kingdom;
            log.arg2 = kingdom;
            room->sendLog(log);
        } else if (choice == "obtain") {
            room->broadcastSkillInvoke(objectName(), index + 2);
            QStringList lords = Sanguosha->getLords();
            foreach (ServerPlayer *player, room->getAlivePlayers()) {
                QString name = player->getGeneralName();
                if (Sanguosha->isGeneralHidden(name)) {
                    QString fname = Sanguosha->findConvertFrom(name);
                    if (!fname.isEmpty()) name = fname;
                }
                lords.removeOne(name);

                if (!player->getGeneral2()) continue;

                name = player->getGeneral2Name();
                if (Sanguosha->isGeneralHidden(name)) {
                    QString fname = Sanguosha->findConvertFrom(name);
                    if (!fname.isEmpty()) name = fname;
                }
                lords.removeOne(name);
            }

            QStringList lord_skills;
            foreach (QString lord, lords) {
                const General *general = Sanguosha->getGeneral(lord);
                QList<const Skill *> skills = general->findChildren<const Skill *>();
                foreach (const Skill *skill, skills) {
                    if (skill->isLordSkill() && !weiwudi->hasSkill(skill->objectName()))
                        lord_skills << skill->objectName();
                }
            }

            if (!lord_skills.isEmpty()) {
                QString skill_name = room->askForChoice(weiwudi, "nosguixin_lordskills", lord_skills.join("+"));

                const Skill *skill = Sanguosha->getSkill(skill_name);
                room->acquireSkill(weiwudi, skill);
            }
        }
        return false;
    }
};

// old stantard generals

class NosJianxiong: public MasochismSkill {
public:
    NosJianxiong(): MasochismSkill("nosjianxiong") {
    }

    virtual void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const{
        Room *room = caocao->getRoom();
        const Card *card = damage.card;
        if (!card) return;

        QList<int> ids;
        if (card->isVirtualCard())
            ids = card->getSubcards();
        else
            ids << card->getEffectiveId();

        if (ids.isEmpty()) return;
        foreach (int id, ids) {
            if (room->getCardPlace(id) != Player::PlaceTable) return;
        }
        QVariant data = QVariant::fromValue(damage);
        if (room->askForSkillInvoke(caocao, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            caocao->obtainCard(card);
        }
    }
};

class NosFankui: public MasochismSkill {
public:
    NosFankui(): MasochismSkill("nosfankui") {
    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = simayi->getRoom();
        QVariant data = QVariant::fromValue(from);
        if (from && !from->isNude() && room->askForSkillInvoke(simayi, "nosfankui", data)) {
            room->broadcastSkillInvoke(objectName());
            int card_id = room->askForCardChosen(simayi, from, "he", "nosfankui");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, simayi->objectName());
            room->obtainCard(simayi, Sanguosha->getCard(card_id),
                             reason, room->getCardPlace(card_id) != Player::PlaceHand);
        }
    }
};

class NosGuicai: public TriggerSkill {
public:
    NosGuicai(): TriggerSkill("nosguicai") {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->isKongcheng())
            return false;

        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@nosguicai-card" << judge->who->objectName()
                    << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, "." , prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
            room->broadcastSkillInvoke(objectName());
            room->retrial(card, player, judge, objectName());
        }

        return false;
    }
};

class NosGanglie: public MasochismSkill {
public:
    NosGanglie(): MasochismSkill("nosganglie") {
    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();
        QVariant data = QVariant::fromValue(damage);

        if (room->askForSkillInvoke(xiahou, "nosganglie", data)) {
            room->broadcastSkillInvoke("ikaoli");

            JudgeStruct judge;
            judge.pattern = ".|heart";
            judge.good = false;
            judge.reason = objectName();
            judge.who = xiahou;

            room->judge(judge);
            if (!from || from->isDead()) return;
            if (judge.isGood()) {
                if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
                    room->damage(DamageStruct(objectName(), xiahou, from));
            }
        }
    }
};

NosTuxiCard::NosTuxiCard() {
}

bool NosTuxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() >= 2 || to_select == Self)
        return false;

    return !to_select->isKongcheng();
}

void NosTuxiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (effect.from->isAlive() && !effect.to->isKongcheng()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "iklianbao");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
        room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
    }
}

class NosTuxiViewAsSkill: public ZeroCardViewAsSkill {
public:
    NosTuxiViewAsSkill(): ZeroCardViewAsSkill("nostuxi") {
        response_pattern = "@@nostuxi";
    }

    virtual const Card *viewAs() const{
        return new NosTuxiCard;
    }
};

class NosTuxi: public PhaseChangeSkill {
public:
    NosTuxi(): PhaseChangeSkill("nostuxi") {
        view_as_skill = new NosTuxiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *zhangliao) const{
        if (zhangliao->getPhase() == Player::Draw) {
            Room *room = zhangliao->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(zhangliao);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }
            zhangliao->setFlags("-NosTuxiAudioBroadcast");

            if (can_invoke && room->askForUseCard(zhangliao, "@@nostuxi", "@nostuxi-card"))
                return true;
        }

        return false;
    }
};

class NosLuoyiBuff: public TriggerSkill {
public:
    NosLuoyiBuff(): TriggerSkill("#nosluoyi") {
        events << DamageCaused;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag("nosluoyi") && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user) return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class NosLuoyi: public DrawCardsSkill {
public:
    NosLuoyi(): DrawCardsSkill("nosluoyi") {
    }

    virtual int getDrawNum(ServerPlayer *xuchu, int n) const{
        Room *room = xuchu->getRoom();
        if (room->askForSkillInvoke(xuchu, objectName())) {
            room->broadcastSkillInvoke(objectName());
            xuchu->setFlags(objectName());
            return n - 1;
        } else
            return n;
    }
};

NosRendeCard::NosRendeCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosRendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();

    room->broadcastSkillInvoke("ikshenai");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nosrende", QString());
    room->obtainCard(target, this, reason, false);

    int old_value = source->getMark("nosrende");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "nosrende", new_value);

    if (old_value < 2 && new_value >= 2)
        room->recover(source, RecoverStruct(source));
}

class NosRendeViewAsSkill: public ViewAsSkill {
public:
    NosRendeViewAsSkill(): ViewAsSkill("nosrende") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("nosrende") >= 2)
            return false;
        else
            return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("nosrende") >= 2)
           return false;
        return !player->isKongcheng();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        NosRendeCard *rende_card = new NosRendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class NosRende: public TriggerSkill {
public:
    NosRende(): TriggerSkill("nosrende") {
        events << EventPhaseChanging;
        view_as_skill = new NosRendeViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getMark("nosrende") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "nosrende", 0);
        return false;
    }
};

class NosTieji: public TriggerSkill {
public:
    NosTieji(): TriggerSkill("nostieji") {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive()) break;
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());

                p->setFlags("NosTiejiTarget"); // For AI

                JudgeStruct judge;
                judge.pattern = ".|red";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                try {
                    room->judge(judge);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        p->setFlags("-NosTiejiTarget");
                    throw triggerEvent;
                }

                if (judge.isGood()) {
                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);
                    jink_list.replace(index, QVariant(0));
                }

                p->setFlags("-NosTiejiTarget");
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class NosYingzi: public DrawCardsSkill {
public:
    NosYingzi(): DrawCardsSkill("nosyingzi") {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *zhouyu, int n) const{
        Room *room = zhouyu->getRoom();
        if (room->askForSkillInvoke(zhouyu, objectName())) {
            room->broadcastSkillInvoke("nosyingzi");
            return n + 1;
        } else
            return n;
    }
};

NosFanjianCard::NosFanjianCard() {
}

void NosFanjianCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    int card_id = zhouyu->getRandomHandCardId();
    const Card *card = Sanguosha->getCard(card_id);
    Card::Suit suit = room->askForSuit(target, "nosfanjian");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = target;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->getThread()->delay();
    target->obtainCard(card);
    room->showCard(target, card_id);

    if (card->getSuit() != suit)
        room->damage(DamageStruct("nosfanjian", zhouyu, target));
}

class NosFanjian: public ZeroCardViewAsSkill {
public:
    NosFanjian(): ZeroCardViewAsSkill("nosfanjian") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("NosFanjianCard");
    }

    virtual const Card *viewAs() const{
        return new NosFanjianCard;
    }
};

class NosGuose: public OneCardViewAsSkill {
public:
    NosGuose(): OneCardViewAsSkill("nosguose") {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Indulgence *indulgence = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
        indulgence->addSubcard(originalCard->getId());
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

// old wind generals

class IkLeiji: public TriggerSkill {
public:
    IkLeiji(): TriggerSkill("ikleiji") {
        events << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &data, ServerPlayer* &) const{
        const Card *card_star = data.value<CardResponseStruct>().m_card;
        if (TriggerSkill::triggerable(zhangjiao) && card_star->isKindOf("Jink"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = room->askForPlayerChosen(zhangjiao, room->getAlivePlayers(), objectName(), "ikleiji-invoke", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            zhangjiao->tag["IkLeijiTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = zhangjiao->tag["IkLeijiTarget"].value<ServerPlayer *>();
        zhangjiao->tag.remove("IkLeijiTarget");
        if (target) {
            JudgeStruct judge;
            judge.pattern = ".|spade";
            judge.good = false;
            judge.negative = true;
            judge.reason = objectName();
            judge.who = target;

            room->judge(judge);

            if (judge.isBad())
                room->damage(DamageStruct(objectName(), zhangjiao, target, 2, DamageStruct::Thunder));
        }

        return false;
    }
};

#include "wind.h"
class NosJushou: public Jushou {
public:
    NosJushou(): Jushou() {
        setObjectName("nosjushou");
    }

    virtual int getJushouDrawNum(ServerPlayer *) const{
        return 3;
    }
};

class IkSusheng: public TriggerSkill {
public:
    IkSusheng(): TriggerSkill("iksusheng") {
        events << HpChanged;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(zhoutai)
            && !data.isNull() && !data.canConvert<RecoverStruct>() && zhoutai->getHp() < 1)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data, ServerPlayer *) const{
        if (zhoutai->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data, ServerPlayer *) const{
        QList<int> iksusheng = zhoutai->getPile("iksushengpile");

        int need = 1 - zhoutai->getHp(); // the buqu cards that should be turned over
        int n = need - iksusheng.length();
        if (n > 0) {
            QList<int> card_ids = room->getNCards(n, false);
            zhoutai->addToPile("iksushengpile", card_ids);
        }
        iksusheng = zhoutai->getPile("iksushengpile");
        QList<int> duplicate_numbers;

        QSet<int> numbers;
        foreach (int card_id, iksusheng) {
            const Card *card = Sanguosha->getCard(card_id);
            int number = card->getNumber();

            if (numbers.contains(number))
                duplicate_numbers << number;
            else
                numbers << number;
        }

        if (duplicate_numbers.isEmpty())
            return true;
        else
            zhoutai->clearOnePrivatePile("iksushengpile");
        
        return false;
    }
};

class IkSushengRemove: public TriggerSkill {
public:
    IkSushengRemove(): TriggerSkill("#iksusheng-remove") {
        events << HpRecover;
        frequency = Compulsory;
    }

    static void Remove(ServerPlayer *zhoutai) {
        Room *room = zhoutai->getRoom();
        QList<int> iksusheng(zhoutai->getPile("iksushengpile"));

        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "iksusheng", QString());
        int need = 1 - zhoutai->getHp();
        if (need <= 0) {
            // clear all the buqu cards
            foreach (int card_id, iksusheng) {
                LogMessage log;
                log.type = "$IkSushengRemove";
                log.from = zhoutai;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
            }
        } else {
            int to_remove = iksusheng.length() - need;
            for (int i = 0; i < to_remove; i++) {
                room->fillAG(iksusheng);
                int card_id = room->askForAG(zhoutai, iksusheng, false, "iksusheng");

                LogMessage log;
                log.type = "$IkSushengRemove";
                log.from = zhoutai;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                iksusheng.removeOne(card_id);
                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
                room->clearAG();
            }
        }
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && !target->getPile("iksushengpile").isEmpty();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *zhoutai, QVariant &, ServerPlayer *) const{
        Remove(zhoutai);
        return false;
    }
};

class IkSushengClear: public DetachEffectSkill {
public:
    IkSushengClear(): DetachEffectSkill("iksusheng") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        if (player->getHp() <= 0)
            room->enterDying(player, NULL);
    }
};

class IkEli: public TriggerSkill {
public:
    IkEli(): TriggerSkill("ikeli") {
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        QVariantList cards = target->tag["IkEli"].toList();
        return cards.length() >= 2 && TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Discard;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        SavageAssault *savage_assault = new SavageAssault(Card::NoSuit, 0);
        savage_assault->setSkillName("_ikeli");
        if (!savage_assault->isAvailable(player)) {
            delete savage_assault;
            return false;
        }
        room->useCard(CardUseStruct(savage_assault, player, QList<ServerPlayer *>()));
        return false;
    }
};

class IkEliRecord: public TriggerSkill {
public:
    IkEliRecord(): TriggerSkill("#theli-record") {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging)
            player->tag.remove("IkEli");
        else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player)
                return QStringList();

            if (player->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                QVariantList cards = player->tag["IkEli"].toList();
                foreach (int id, move.card_ids)
                    if (!cards.contains(id))
                        cards << id;
                player->tag["IkEli"] = cards;
            }
        }

        return QStringList();
    }
};

#include "touhou-hana.h"

IkGuihuoCard::IkGuihuoCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkGuihuoCard::ikguihuo(ServerPlayer *yuji) const{
    Room *room = yuji->getRoom();
    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);
    QSet<ServerPlayer *> questioned;

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    foreach (int card_id, getSubcards())
        used_cards << card_id;
    room->setTag("IkGuihuoType", user_string);

    foreach (ServerPlayer *player, players) {
        if (player->getHp() <= 0) {
            LogMessage log;
            log.type = "#IkGuihuoCannotQuestion";
            log.from = player;
            log.arg = QString::number(player->getHp());
            room->sendLog(log);

            room->setEmotion(player, "no-question");
            continue;
        }

        QString choice = room->askForChoice(player, "ikguihuo", "noquestion+question");
        if (choice == "question") {
            room->setEmotion(player, "question");
            questioned << player;
        } else
            room->setEmotion(player, "no-question");

        LogMessage log;
        log.type = "#IkGuihuoQuery";
        log.from = player;
        log.arg = choice;

        room->sendLog(log);
    }

    LogMessage log;
    log.type = "$IkGuihuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    bool success = false;
    if (questioned.isEmpty()) {
        success = true;
        foreach (ServerPlayer *player, players)
            room->setEmotion(player, ".");

        CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "ikguihuo");
        CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
        moves.append(move);
        room->moveCardsAtomic(moves, true);
    } else {
        const Card *card = Sanguosha->getCard(subcards.first());
        bool real;
        if (user_string == "peach+analeptic")
            real = card->objectName() == yuji->tag["IkGuihuoSaveSelf"].toString();
        else if (user_string == "slash")
            real = card->objectName().contains("slash");
        else if (user_string == "normal_slash")
            real = card->objectName() == "slash";
        else
            real = card->match(user_string);

        success = real && card->getSuit() == Card::Heart;
        if (success) {
            CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "ikguihuo");
            CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
            moves.append(move);
            room->moveCardsAtomic(moves, true);
        } else {
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                             CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), "ikguihuo"), true);
        }
        foreach (ServerPlayer *player, players) {
            room->setEmotion(player, ".");
            if (questioned.contains(player)) {
                if (real)
                    room->loseHp(player);
                else
                    player->drawCards(1, "ikguihuo");
            }
        }
    }
    yuji->tag.remove("IkGuihuoSaveSelf");
    yuji->tag.remove("IkGuihuoSlash");
    return success;
}

bool IkGuihuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("ikguihuo").value<const Card *>();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool IkGuihuoCard::targetFixed() const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("ikguihuo").value<const Card *>();
    return card && card->targetFixed();
}

bool IkGuihuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("ikguihuo").value<const Card *>();
    return card && card->targetsFeasible(targets, Self);
}

const Card *IkGuihuoCard::validate(CardUseStruct &card_use) const{
    ServerPlayer *yuji = card_use.from;
    Room *room = yuji->getRoom();

    QString to_ikguihuo = user_string;
    if (user_string == "slash"
        && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList ikguihuo_list;
        ikguihuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            ikguihuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_ikguihuo = room->askForChoice(yuji, "ikguihuo_slash", ikguihuo_list.join("+"));
        yuji->tag["IkGuihuoSlash"] = QVariant(to_ikguihuo);
    }
    room->broadcastSkillInvoke("ikguihuo");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#IkGuihuoNoTarget" : "#IkGuihuo";
    log.from = yuji;
    log.to = card_use.to;
    log.arg = to_ikguihuo;
    log.arg2 = "ikguihuo";

    room->sendLog(log);

    if (ikguihuo(card_use.from)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_ikguihuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_ikguihuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_ikguihuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("ikguihuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

const Card *IkGuihuoCard::validateInResponse(ServerPlayer *yuji) const{
    Room *room = yuji->getRoom();
    room->broadcastSkillInvoke("ikguihuo");

    QString to_ikguihuo;
    if (user_string == "peach+analeptic") {
        QStringList ikguihuo_list;
        ikguihuo_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            ikguihuo_list << "analeptic";
        to_ikguihuo = room->askForChoice(yuji, "ikguihuo_saveself", ikguihuo_list.join("+"));
        yuji->tag["IkGuihuoSaveSelf"] = QVariant(to_ikguihuo);
    } else if (user_string == "slash") {
        QStringList ikguihuo_list;
        ikguihuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            ikguihuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_ikguihuo = room->askForChoice(yuji, "ikguihuo_slash", ikguihuo_list.join("+"));
        yuji->tag["IkGuihuoSlash"] = QVariant(to_ikguihuo);
    }
    else
        to_ikguihuo = user_string;

    LogMessage log;
    log.type = "#IkGuihuoNoTarget";
    log.from = yuji;
    log.arg = to_ikguihuo;
    log.arg2 = "ikguihuo";
    room->sendLog(log);

    if (ikguihuo(yuji)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_ikguihuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_ikguihuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_ikguihuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("ikguihuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

class IkGuihuo: public OneCardViewAsSkill {
public:
    IkGuihuo(): OneCardViewAsSkill("ikguihuo") {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->isKongcheng() || pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (pattern == "peach" && player->hasFlag("Global_PreventPeach")) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            IkGuihuoCard *card = new IkGuihuoCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        const Card *c = Self->tag.value("ikguihuo").value<const Card *>();
        if (c) {
            IkGuihuoCard *card = new IkGuihuoCard;
            if (!c->objectName().contains("slash"))
                card->setUserString(c->objectName());
            else
                card->setUserString(Self->tag["IkGuihuoSlash"].toString());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    virtual QDialog *getDialog() const{
        return ThMimengDialog::getInstance("ikguihuo");
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (!card->isKindOf("IkGuihuoCard"))
            return -2;
        else
            return -1;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return !player->isKongcheng();
    }
};

NostalGeneralPackage::NostalGeneralPackage()
    : Package("nostal_general")
{
    General *nos_zhonghui = new General(this, "nos_zhonghui", "wei", 3);
    nos_zhonghui->addSkill(new NosZhenggong);
    nos_zhonghui->addSkill(new NosQuanji);
    nos_zhonghui->addSkill(new NosBaijiang);
    nos_zhonghui->addSkill(new NosZili);
    nos_zhonghui->addRelateSkill("nosyexin");
    nos_zhonghui->addRelateSkill("#nosyexin-fake-move");
    related_skills.insertMulti("nosyexin", "#nosyexin-fake-move");
    nos_zhonghui->addRelateSkill("nospaiyi");

    General *nos_shencaocao = new General(this, "nos_shencaocao", "god", 3);
    nos_shencaocao->addSkill(new NosGuixin);
    nos_shencaocao->addSkill("thfeiying");

    addMetaObject<NosYexinCard>();

    skills << new NosYexin << new FakeMoveSkill("nosyexin") << new NosPaiyi;
}

NostalStandardPackage::NostalStandardPackage()
    : Package("nostal_standard")
{
    General *nos_caocao = new General(this, "nos_caocao$", "wei");
    nos_caocao->addSkill(new NosJianxiong);
    nos_caocao->addSkill("ikhuanwei");

    General *nos_simayi = new General(this, "nos_simayi", "wei", 3);
    nos_simayi->addSkill(new NosFankui);
    nos_simayi->addSkill(new NosGuicai);

    General *nos_xiahoudun = new General(this, "nos_xiahoudun", "wei");
    nos_xiahoudun->addSkill(new NosGanglie);

    General *nos_zhangliao = new General(this, "nos_zhangliao", "wei");
    nos_zhangliao->addSkill(new NosTuxi);

    General *nos_xuchu = new General(this, "nos_xuchu", "wei");
    nos_xuchu->addSkill(new NosLuoyi);
    nos_xuchu->addSkill(new NosLuoyiBuff);
    related_skills.insertMulti("nosluoyi", "#nosluoyi");

    General *nos_liubei = new General(this, "nos_liubei$", "shu");
    nos_liubei->addSkill(new NosRende);
    nos_liubei->addSkill("ikxinqi");

    General *nos_guanyu = new General(this, "nos_guanyu", "shu");
    nos_guanyu->addSkill("ikchilian");

    General *nos_zhangfei = new General(this, "nos_zhangfei", "shu");
    nos_zhangfei->addSkill("iklipao");

    General *nos_zhaoyun = new General(this, "nos_zhaoyun", "shu");
    nos_zhaoyun->addSkill("ikhuahuan");

    General *nos_machao = new General(this, "nos_machao", "shu");
    nos_machao->addSkill("thjibu");
    nos_machao->addSkill(new NosTieji);

    General *nos_ganning = new General(this, "nos_ganning", "wu");
    nos_ganning->addSkill("ikkuipo");

    General *nos_lvmeng = new General(this, "nos_lvmeng", "wu");
    nos_lvmeng->addSkill("ikbiju");

    General *nos_zhouyu = new General(this, "nos_zhouyu", "wu", 3);
    nos_zhouyu->addSkill(new NosYingzi);
    nos_zhouyu->addSkill(new NosFanjian);

    General *nos_daqiao = new General(this, "nos_daqiao", "wu", 3, false);
    nos_daqiao->addSkill(new NosGuose);
    nos_daqiao->addSkill("ikxuanhuo");

    General *nos_lvbu = new General(this, "nos_lvbu", "qun");
    nos_lvbu->addSkill("ikwushuang");

    addMetaObject<NosTuxiCard>();
    addMetaObject<NosRendeCard>();
    addMetaObject<NosFanjianCard>();
}

NostalWindPackage::NostalWindPackage()
    : Package("nostal_wind")
{
    General *nos_caoren = new General(this, "nos_caoren", "wei");
    nos_caoren->addSkill(new NosJushou);

    General *snow013 = new General(this, "snow013", "yuki", 3);
    snow013->addSkill(new IkSusheng);
    snow013->addSkill(new IkSushengRemove);
    snow013->addSkill(new IkSushengClear);
    related_skills.insertMulti("iksusheng", "#iksusheng-remove");
    related_skills.insertMulti("iksusheng", "#iksusheng-clear");
    snow013->addSkill(new IkEli);
    snow013->addSkill(new IkEliRecord);
    related_skills.insertMulti("ikeli", "#ikeli-record");

    General *luna014 = new General(this, "luna014$", "tsuki", 3);
    luna014->addSkill(new IkLeiji);
    luna014->addSkill("iktianshi");
    luna014->addSkill("ikyuji");

    General *luna011 = new General(this, "luna011", "tsuki");
    luna011->addSkill(new IkGuihuo);

    addMetaObject<IkGuihuoCard>();
}

NostalYJCMPackage::NostalYJCMPackage()
    : Package("nostal_yjcm")
{
    General *nos_fazheng = new General(this, "nos_fazheng", "shu", 3);
    nos_fazheng->addSkill(new NosEnyuan);
    nos_fazheng->addSkill(new NosXuanhuo);

    General *nos_lingtong = new General(this, "nos_lingtong", "wu");
    nos_lingtong->addSkill(new NosXuanfeng);
    nos_lingtong->addSkill(new SlashNoDistanceLimitSkill("nosxuanfeng"));
    related_skills.insertMulti("nosxuanfeng", "#nosxuanfeng-slash-ndl");

    General *nos_xushu = new General(this, "nos_xushu", "shu", 3);
    nos_xushu->addSkill(new NosWuyan);
    nos_xushu->addSkill(new NosJujian);

    General *nos_zhangchunhua = new General(this, "nos_zhangchunhua", "wei", 3, false);
    nos_zhangchunhua->addSkill("jueqing");
    nos_zhangchunhua->addSkill(new NosShangshi);

    addMetaObject<NosXuanhuoCard>();
    addMetaObject<NosJujianCard>();
}

NostalYJCM2012Package::NostalYJCM2012Package()
    : Package("nostal_yjcm2012")
{
    General *nos_guanxingzhangbao = new General(this, "nos_guanxingzhangbao", "shu");
    nos_guanxingzhangbao->addSkill(new NosFuhun);

    General *nos_handang = new General(this, "nos_handang", "wu");
    nos_handang->addSkill(new NosGongqi);
    nos_handang->addSkill(new NosGongqiTargetMod);
    nos_handang->addSkill(new NosJiefan);
    related_skills.insertMulti("nosgongqi", "#nosgongqi-target");

    General *nos_madai = new General(this, "nos_madai", "shu");
    nos_madai->addSkill("thjibu");
    nos_madai->addSkill(new NosQianxi);

    General *bloom022 = new General(this, "bloom022", "hana", 3, false);
    bloom022->addSkill(new IkLundao);
    bloom022->addSkill(new IkXuanwu);

    addMetaObject<NosJiefanCard>();
}

NostalYJCM2013Package::NostalYJCM2013Package()
    : Package("nostal_yjcm2013")
{
    General *nos_caochong = new General(this, "nos_caochong", "wei", 3);
    nos_caochong->addSkill(new NosChengxiang);
    nos_caochong->addSkill(new NosRenxin);

    General *nos_fuhuanghou = new General(this, "nos_fuhuanghou", "qun", 3, false);
    nos_fuhuanghou->addSkill(new NosZhuikong);
    nos_fuhuanghou->addSkill(new NosZhuikongClear);
    nos_fuhuanghou->addSkill(new NosQiuyuan);
    related_skills.insertMulti("noszhuikong", "#noszhuikong-clear");

    General *nos_liru = new General(this, "nos_liru", "qun", 3);
    nos_liru->addSkill(new NosJuece);
    nos_liru->addSkill(new NosMieji);
    nos_liru->addSkill(new NosMiejiForExNihiloAndCollateral);
    nos_liru->addSkill(new NosMiejiEffect);
    nos_liru->addSkill(new NosFencheng);
    related_skills.insertMulti("nosmieji", "#nosmieji");
    related_skills.insertMulti("nosmieji", "#nosmieji-effect");

    General *nos_zhuran = new General(this, "nos_zhuran", "wu");
    nos_zhuran->addSkill(new NosDanshou);

    addMetaObject<NosRenxinCard>();
    addMetaObject<NosFenchengCard>();
}

ADD_PACKAGE(Nostalgia)
ADD_PACKAGE(NostalGeneral)
ADD_PACKAGE(NostalWind)
ADD_PACKAGE(NostalStandard)
ADD_PACKAGE(NostalYJCM)
ADD_PACKAGE(NostalYJCM2012)
ADD_PACKAGE(NostalYJCM2013)
