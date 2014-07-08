#include "yjcm.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "engine.h"
#include "settings.h"
#include "ai.h"
#include "general.h"

class Xuanfeng: public TriggerSkill {
public:
    Xuanfeng(): TriggerSkill("xuanfeng") {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    void perform(Room *room, ServerPlayer *lingtong) const{
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
            if (lingtong->canDiscard(target, "he"))
                targets << target;
        }
        if (targets.isEmpty())
            return;

        if (lingtong->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());

            ServerPlayer *first = room->askForPlayerChosen(lingtong, targets, "xuanfeng");
            ServerPlayer *second = NULL;
            int first_id = -1;
            int second_id = -1;
            if (first != NULL) {
                first_id = room->askForCardChosen(lingtong, first, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(first_id, first, lingtong);
            }
            if (!lingtong->isAlive())
                return;
            targets.clear();
            foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
                if (lingtong->canDiscard(target, "he"))
                    targets << target;
            }
            if (!targets.isEmpty())
                second = room->askForPlayerChosen(lingtong, targets, "xuanfeng");
            if (second != NULL) {
                second_id = room->askForCardChosen(lingtong, second, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(second_id, second, lingtong);
            }
        }
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lingtong, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            lingtong->setMark("xuanfeng", 0);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != lingtong)
                return false;

            if (lingtong->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                lingtong->addMark("xuanfeng", move.card_ids.length());

            if (move.from_places.contains(Player::PlaceEquip) && TriggerSkill::triggerable(lingtong))
                perform(room, lingtong);
        } else if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(lingtong)
                   && lingtong->getPhase() == Player::Discard && lingtong->getMark("xuanfeng") >= 2) {
            perform(room, lingtong);
        }

        return false;
    }
};

class Pojun: public TriggerSkill {
public:
    Pojun(): TriggerSkill("pojun") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && !damage.chain && !damage.transfer
            && damage.to->isAlive() && !damage.to->hasFlag("Global_DebutFlag")
            && room->askForSkillInvoke(player, objectName(), data)) {
            int x = qMin(5, damage.to->getHp());
            room->broadcastSkillInvoke(objectName(), (x >= 3 || !damage.to->faceUp()) ? 2 : 1);
            damage.to->drawCards(x, objectName());
            damage.to->turnOver();
        }
        return false;
    }
};

XianzhenCard::XianzhenCard() {
}

bool XianzhenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void XianzhenCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (effect.from->pindian(effect.to, "xianzhen", NULL)) {
        ServerPlayer *target = effect.to;
        effect.from->tag["XianzhenTarget"] = QVariant::fromValue(target);
        room->setPlayerFlag(effect.from, "XianzhenSuccess");

        QStringList assignee_list = effect.from->property("extra_slash_specific_assignee").toString().split("+");
        assignee_list << target->objectName();
        room->setPlayerProperty(effect.from, "extra_slash_specific_assignee", assignee_list.join("+"));

        room->setFixedDistance(effect.from, effect.to, 1);
        room->addPlayerMark(effect.to, "Armor_Nullified");
    } else {
        room->setPlayerCardLimitation(effect.from, "use", "Slash", true);
    }
}

class XianzhenViewAsSkill: public ZeroCardViewAsSkill {
public:
    XianzhenViewAsSkill(): ZeroCardViewAsSkill("xianzhen") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("XianzhenCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new XianzhenCard;
    }
};

class Xianzhen: public TriggerSkill {
public:
    Xianzhen(): TriggerSkill("xianzhen") {
        events << EventPhaseChanging << Death;
        view_as_skill = new XianzhenViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->tag["XianzhenTarget"].value<ServerPlayer *>() != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *gaoshun, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        }
        ServerPlayer *target = gaoshun->tag["XianzhenTarget"].value<ServerPlayer *>();
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != gaoshun) {
                if (death.who == target) {
                    room->setFixedDistance(gaoshun, target, -1);
                    gaoshun->tag.remove("XianzhenTarget");
                    room->setPlayerFlag(gaoshun, "-XianzhenSuccess");
                }
                return false;
            }
        }
        if (target) {
            QStringList assignee_list = gaoshun->property("extra_slash_specific_assignee").toString().split("+");
            assignee_list.removeOne(target->objectName());
            room->setPlayerProperty(gaoshun, "extra_slash_specific_assignee", assignee_list.join("+"));

            room->setFixedDistance(gaoshun, target, -1);
            gaoshun->tag.remove("XianzhenTarget");
            room->removePlayerMark(target, "Armor_Nullified");
        }
        return false;
    }
};

class Jinjiu: public FilterSkill {
public:
    Jinjiu(): FilterSkill("jinjiu") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->objectName() == "analeptic";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

MingceCard::MingceCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void MingceCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    QList<ServerPlayer *> targets;
    if (Slash::IsAvailable(effect.to)) {
        foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
            if (effect.to->canSlash(p))
                targets << p;
        }
    }

    ServerPlayer *target = NULL;
    QStringList choicelist;
    choicelist << "draw";
    if (!targets.isEmpty() && effect.from->isAlive()) {
        target = room->askForPlayerChosen(effect.from, targets, "mingce", "@dummy-slash2:" + effect.to->objectName());
        target->setFlags("MingceTarget"); // For AI

        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = effect.from;
        log.to << target;
        room->sendLog(log);

        choicelist << "use";
    }

    try {
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "mingce", QString());
        room->obtainCard(effect.to, this, reason);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            if (target && target->hasFlag("MingceTarget")) target->setFlags("-MingceTarget");
        throw triggerEvent;
    }

    QString choice = room->askForChoice(effect.to, "mingce", choicelist.join("+"));
    if (target && target->hasFlag("MingceTarget")) target->setFlags("-MingceTarget");

    if (choice == "use") {
        if (effect.to->canSlash(target, NULL, false)) {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_mingce");
            room->useCard(CardUseStruct(slash, effect.to, target));
        }
    } else if (choice == "draw") {
        effect.to->drawCards(1, "mingce");
    }
}

class Mingce: public OneCardViewAsSkill {
public:
    Mingce(): OneCardViewAsSkill("mingce") {
        filter_pattern = "EquipCard,Slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("MingceCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        MingceCard *mingceCard = new MingceCard;
        mingceCard->addSubcard(originalCard);

        return mingceCard;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (card->isKindOf("Slash"))
            return -2;
        else
            return -1;
    }
};

class Zhichi: public TriggerSkill {
public:
    Zhichi(): TriggerSkill("zhichi") {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::NotActive)
            return false;

        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
            room->broadcastSkillInvoke(objectName(), 1);
            room->notifySkillInvoked(player, objectName());
            if (player->getMark("@late") == 0)
                room->addPlayerMark(player, "@late");

            LogMessage log;
            log.type = "#ZhichiDamaged";
            log.from = player;
            room->sendLog(log);
        }

        return false;
    }
};

class ZhichiProtect: public TriggerSkill {
public:
    ZhichiProtect(): TriggerSkill("#zhichi-protect") {
        events << CardEffected;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.card->isKindOf("Slash") || effect.card->isNDTrick()) && effect.to->getMark("@late") > 0) {
            room->broadcastSkillInvoke("zhichi", 2);
            room->notifySkillInvoked(effect.to, "zhichi");
            LogMessage log;
            log.type = "#ZhichiAvoid";
            log.from = effect.to;
            log.arg = "zhichi";
            room->sendLog(log);

            return true;
        }
        return false;
    }
};

class ZhichiClear: public TriggerSkill {
public:
    ZhichiClear(): TriggerSkill("#zhichi-clear") {
        events << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || player != room->getCurrent())
                return false;
        }

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("@late") > 0)
                room->setPlayerMark(p, "@late", 0);
        }

        return false;
    }
};

GanluCard::GanluCard() {
}

void GanluCard::swapEquip(ServerPlayer *first, ServerPlayer *second) const{
    Room *room = first->getRoom();

    QList<int> equips1, equips2;
    foreach (const Card *equip, first->getEquips())
        equips1.append(equip->getId());
    foreach (const Card *equip, second->getEquips())
        equips2.append(equip->getId());

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(equips1, second, Player::PlaceEquip,
                          CardMoveReason(CardMoveReason::S_REASON_SWAP, first->objectName(), second->objectName(), "ganlu", QString()));
    CardsMoveStruct move2(equips2, first, Player::PlaceEquip,
                          CardMoveReason(CardMoveReason::S_REASON_SWAP, second->objectName(), first->objectName(), "ganlu", QString()));
    exchangeMove.push_back(move2);
    exchangeMove.push_back(move1);
    room->moveCardsAtomic(exchangeMove, false);
}

bool GanluCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

bool GanluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    switch (targets.length()) {
    case 0: return true;
    case 1: {
            int n1 = targets.first()->getEquips().length();
            int n2 = to_select->getEquips().length();
            return qAbs(n1 - n2) <= Self->getLostHp();
        }
    default:
        return false;
    }
}

void GanluCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    LogMessage log;
    log.type = "#GanluSwap";
    log.from = source;
    log.to = targets;
    room->sendLog(log);

    swapEquip(targets.first(), targets[1]);
}

class Ganlu: public ZeroCardViewAsSkill {
public:
    Ganlu(): ZeroCardViewAsSkill("ganlu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("GanluCard");
    }

    virtual const Card *viewAs() const{
        return new GanluCard;
    }
};

class Buyi: public TriggerSkill {
public:
    Buyi(): TriggerSkill("buyi") {
        events << Dying;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *wuguotai, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *player = dying.who;
        if (player->isKongcheng()) return false;
        if (player->getHp() < 1 && wuguotai->askForSkillInvoke(objectName(), data)) {
            const Card *card = NULL;
            if (player == wuguotai)
                card = room->askForCardShow(player, wuguotai, objectName());
            else {
                int card_id = room->askForCardChosen(wuguotai, player, "h", "buyi");
                card = Sanguosha->getCard(card_id);
            }

            room->showCard(player, card->getEffectiveId());

            if (card->getTypeId() != Card::TypeBasic) {
                if (!player->isJilei(card))
                    room->throwCard(card, player);

                room->broadcastSkillInvoke(objectName());
                room->recover(player, RecoverStruct(wuguotai));
            }
        }
        return false;
    }
};

class Quanji: public MasochismSkill {
public:
    Quanji(): MasochismSkill("#quanji") {
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const{
        Room *room = zhonghui->getRoom();

        int x = damage.damage;
        for (int i = 0; i < x; i++) {
            if (zhonghui->askForSkillInvoke("quanji")) {
                room->broadcastSkillInvoke("quanji");
                room->drawCards(zhonghui, 1, objectName());
                if (!zhonghui->isKongcheng()) {
                    int card_id;
                    if (zhonghui->getHandcardNum() == 1) {
                        room->getThread()->delay();
                        card_id = zhonghui->handCards().first();
                    } else {
                        const Card *card = room->askForExchange(zhonghui, "quanji", 1, 1, false, "QuanjiPush");
                        card_id = card->getEffectiveId();
                        delete card;
                    }
                    zhonghui->addToPile("power", card_id);
                }
            }
        }

    }
};

class QuanjiKeep: public MaxCardsSkill {
public:
    QuanjiKeep(): MaxCardsSkill("quanji") {
        frequency = Frequent;
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return target->getPile("power").length();
        else
            return 0;
    }
};

class Zili: public PhaseChangeSkill {
public:
    Zili(): PhaseChangeSkill("zili") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("zili") == 0
               && target->getPile("power").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#ZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("power").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$ZiliAnimate", 4000);

        room->setPlayerMark(zhonghui, "zili", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui)) {
            if (zhonghui->isWounded() && room->askForChoice(zhonghui, objectName(), "recover+draw") == "recover")
                room->recover(zhonghui, RecoverStruct(zhonghui));
            else
                room->drawCards(zhonghui, 2, objectName());
            if (zhonghui->getMark("zili") == 1)
                room->acquireSkill(zhonghui, "paiyi");
        }

        return false;
    }
};

PaiyiCard::PaiyiCard() {
    mute = true;
}

bool PaiyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void PaiyiCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhonghui = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhonghui->getRoom();
    QList<int> powers = zhonghui->getPile("power");
    if (powers.isEmpty()) return;

    room->broadcastSkillInvoke("paiyi", target == zhonghui ? 1 : 2);

    int card_id;
    if (powers.length() == 1)
        card_id = powers.first();
    else {
        room->fillAG(powers, zhonghui);
        card_id = room->askForAG(zhonghui, powers, false, "paiyi");
        room->clearAG(zhonghui);
    }

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(),
                          target->objectName(), "paiyi", QString());
    room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
    room->drawCards(target, 2, "paiyi");
    if (target->getHandcardNum() > zhonghui->getHandcardNum())
        room->damage(DamageStruct("paiyi", zhonghui, target));
}

class Paiyi: public ZeroCardViewAsSkill {
public:
    Paiyi(): ZeroCardViewAsSkill("paiyi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("power").isEmpty() && !player->hasUsed("PaiyiCard");
    }

    virtual const Card *viewAs() const{
        return new PaiyiCard;
    }
};

class Jueqing: public TriggerSkill {
public:
    Jueqing(): TriggerSkill("jueqing") {
        frequency = Compulsory;
        events << Predamage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == zhangchunhua) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = zhangchunhua;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(zhangchunhua, objectName());
            room->broadcastSkillInvoke(objectName());
            room->loseHp(damage.to, damage.damage);

            return true;
        }
        return false;
    }
};

Shangshi::Shangshi(): TriggerSkill("shangshi") {
    events << HpChanged << MaxHpChanged << CardsMoveOneTime;
    frequency = Frequent;
}

int Shangshi::getMaxLostHp(ServerPlayer *zhangchunhua) const{
    int losthp = zhangchunhua->getLostHp();
    if (losthp > 2)
        losthp = 2;
    return qMin(losthp, zhangchunhua->getMaxHp());
}

bool Shangshi::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data) const{
    int losthp = getMaxLostHp(zhangchunhua);
    if (triggerEvent == CardsMoveOneTime) {
        bool can_invoke = false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == zhangchunhua && move.from_places.contains(Player::PlaceHand))
            can_invoke = true;
        if (move.to == zhangchunhua && move.to_place == Player::PlaceHand)
            can_invoke = true;
        if (!can_invoke)
            return false;
    } else if (triggerEvent == HpChanged || triggerEvent == MaxHpChanged) {
        if (zhangchunhua->getPhase() == Player::Discard) {
            zhangchunhua->addMark("shangshi");
            return false;
        }
    }

    if (zhangchunhua->getHandcardNum() < losthp && zhangchunhua->askForSkillInvoke(objectName())) {
        room->broadcastSkillInvoke("shangshi");
        zhangchunhua->drawCards(losthp - zhangchunhua->getHandcardNum(), objectName());
    }

    return false;
}

YJCMPackage::YJCMPackage()
    : Package("YJCM")
{

    General *chengong = new General(this, "chengong", "qun", 3); // YJ 002
    chengong->addSkill(new Zhichi);
    chengong->addSkill(new ZhichiProtect);
    chengong->addSkill(new ZhichiClear);
    chengong->addSkill(new Mingce);
    related_skills.insertMulti("zhichi", "#zhichi-protect");
    related_skills.insertMulti("zhichi", "#zhichi-clear");

    General *gaoshun = new General(this, "gaoshun", "qun"); // YJ 004
    gaoshun->addSkill(new Xianzhen);
    gaoshun->addSkill(new Jinjiu);

    General *lingtong = new General(this, "lingtong", "wu"); // YJ 005
    lingtong->addSkill(new Xuanfeng);

    General *wuguotai = new General(this, "wuguotai", "wu", 3, false); // YJ 007
    wuguotai->addSkill(new Ganlu);
    wuguotai->addSkill(new Buyi);

    General *xusheng = new General(this, "xusheng", "wu"); // YJ 008
    xusheng->addSkill(new Pojun);

    General *zhangchunhua = new General(this, "zhangchunhua", "wei", 3, false); // YJ 011
    zhangchunhua->addSkill(new Jueqing);
    zhangchunhua->addSkill(new Shangshi);

    General *zhonghui = new General(this, "zhonghui", "wei"); // YJ 012
    zhonghui->addSkill(new QuanjiKeep);
    zhonghui->addSkill(new Quanji);
    zhonghui->addSkill(new Zili);
    zhonghui->addRelateSkill("paiyi");
    related_skills.insertMulti("quanji", "#quanji");

    addMetaObject<MingceCard>();
    addMetaObject<GanluCard>();
    addMetaObject<XianzhenCard>();
    addMetaObject<PaiyiCard>();

    skills << new Paiyi;
}

ADD_PACKAGE(YJCM)

