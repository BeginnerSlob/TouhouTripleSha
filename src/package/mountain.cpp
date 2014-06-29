#include "mountain.h"
#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "generaloverview.h"
#include "clientplayer.h"
#include "client.h"
#include "ai.h"
#include "jsonutils.h"

#include <QCommandLinkButton>

IkMancaiCard::IkMancaiCard() {
    mute = true;
}

bool IkMancaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    Player::Phase phase = (Player::Phase)Self->getMark("ikmancaiPhase");
    if (phase == Player::Draw)
        return targets.length() <= 2 && !targets.isEmpty();
    else if (phase == Player::Play)
        return targets.length() == 1;
    return false;
}

bool IkMancaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Player::Phase phase = (Player::Phase)Self->getMark("ikmancaiPhase");
    if (phase == Player::Draw)
        return targets.length() < 2 && to_select != Self && !to_select->isKongcheng();
    else if (phase == Player::Play)
        return targets.isEmpty()
               && (!to_select->getJudgingArea().isEmpty() || !to_select->getEquips().isEmpty());
    return false;
}

void IkMancaiCard::use(Room *room, ServerPlayer *zhanghe, QList<ServerPlayer *> &targets) const{
    Player::Phase phase = (Player::Phase)zhanghe->getMark("ikmancaiPhase");
    if (phase == Player::Draw) {
        if (targets.isEmpty())
            return;

        foreach (ServerPlayer *target, targets) {
            if (zhanghe->isAlive() && target->isAlive())
                room->cardEffect(this, zhanghe, target);
        }
    } else if (phase == Player::Play) {
        if (targets.isEmpty())
            return;

        PlayerStar from = targets.first();
        if (!from->hasEquip() && from->getJudgingArea().isEmpty())
            return;

        int card_id = room->askForCardChosen(zhanghe, from , "ej", "ikmancai");
        const Card *card = Sanguosha->getCard(card_id);
        Player::Place place = room->getCardPlace(card_id);

        int equip_index = -1;
        if (place == Player::PlaceEquip) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equip->location());
        }

        QList<ServerPlayer *> tos;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (equip_index != -1) {
                if (p->getEquip(equip_index) == NULL)
                    tos << p;
            } else {
                if (!zhanghe->isProhibited(p, card) && !p->containsTrick(card->objectName()))
                    tos << p;
            }
        }

        room->setTag("IkMancaiTarget", QVariant::fromValue(from));
        ServerPlayer *to = room->askForPlayerChosen(zhanghe, tos, "ikmancai", "@ikmancai-to:::" + card->objectName());
        if (to)
            room->moveCardTo(card, from, to, place,
                             CardMoveReason(CardMoveReason::S_REASON_TRANSFER,
                                            zhanghe->objectName(), "ikmancai", QString()));
        room->removeTag("IkMancaiTarget");
    }
}

void IkMancaiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (!effect.to->isKongcheng()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "ikmancai");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
        room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
    }
}

class IkMancaiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkMancaiViewAsSkill(): ZeroCardViewAsSkill("ikmancai") {
        response_pattern = "@@ikmancai";
    }

    virtual const Card *viewAs() const{
        return new IkMancaiCard;
    }
};

class IkMancai: public TriggerSkill {
public:
    IkMancai(): TriggerSkill("ikmancai") {
        events << EventPhaseChanging;
        view_as_skill = new IkMancaiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(target) && target->canDiscard(target, "h")) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            switch (change.to) {
            case Player::RoundStart:
            case Player::Start:
            case Player::Finish:
            case Player::NotActive: return QStringList();

            case Player::Judge:
            case Player::Draw:
            case Player::Play:
            case Player::Discard: return QStringList(objectName());
            case Player::PhaseNone: Q_ASSERT(false);
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data, ServerPlayer *) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        room->setPlayerMark(zhanghe, "ikmancaiPhase", (int)change.to);
        int index = 0;
        switch (change.to) {
        case Player::RoundStart:
        case Player::Start:
        case Player::Finish:
        case Player::NotActive: return false;

        case Player::Judge: index = 1 ;break;
        case Player::Draw: index = 2; break;
        case Player::Play: index = 3; break;
        case Player::Discard: index = 4; break;
        case Player::PhaseNone: Q_ASSERT(false);
        }

        QString discard_prompt = QString("#ikmancai-%1").arg(index);
        if (index > 0 && room->askForDiscard(zhanghe, objectName(), 1, 1, true, false, discard_prompt)) {
            room->broadcastSkillInvoke("ikmancai", index);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data, ServerPlayer *) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        room->setPlayerMark(zhanghe, "ikmancaiPhase", (int)change.to);
        int index = 0;
        switch (change.to) {
        case Player::RoundStart:
        case Player::Start:
        case Player::Finish:
        case Player::NotActive: return false;

        case Player::Judge: index = 1 ;break;
        case Player::Draw: index = 2; break;
        case Player::Play: index = 3; break;
        case Player::Discard: index = 4; break;
        case Player::PhaseNone: Q_ASSERT(false);
        }

        QString use_prompt = QString("@ikmancai-%1").arg(index);
        if (!zhanghe->isAlive()) return false;
        if (!zhanghe->isSkipped(change.to) && (index == 2 || index == 3))
            room->askForUseCard(zhanghe, "@@ikmancai", use_prompt, index);
        zhanghe->skip(change.to, true);
        return false;
    }
};

class IkHuiyao: public TriggerSkill {
public:
    IkHuiyao(): TriggerSkill("ikhuiyao") {
        events << Damaged << FinishJudge;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL || !damage.card->isKindOf("Slash") || damage.to->isDead())
                return skill_list;

            foreach (ServerPlayer *caiwenji, room->findPlayersBySkillName(objectName())) {
                if (caiwenji->canDiscard(caiwenji, "he"))
                    skill_list.insert(caiwenji, QStringList(objectName()));
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason != objectName()) return skill_list;
            judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *caiwenji) const{
        if (room->askForCard(caiwenji, "..", "@ikhuiyao", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *caiwenji) const{
        DamageStruct damage = data.value<DamageStruct>();
        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.who = player;
        judge.reason = objectName();

        room->judge(judge);

        Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
        switch (suit) {
        case Card::Heart: {
                room->recover(player, RecoverStruct(caiwenji));

                break;
            }
        case Card::Diamond: {
                player->drawCards(2, objectName());
                break;
            }
        case Card::Club: {
                if (damage.from && damage.from->isAlive())
                    room->askForDiscard(damage.from, "ikhuiyao", 2, 2, false, true);

                break;
            }
        case Card::Spade: {
                if (damage.from && damage.from->isAlive())
                    damage.from->turnOver();

                break;
            }
        default:
                break;
        }

        return false;
    }
};

class IkQihuang: public TriggerSkill {
public:
    IkQihuang(): TriggerSkill("ikqihuang") {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;

        if (death.damage && death.damage->from) {
            LogMessage log;
            log.type = "#IkQihuangLoseSkills";
            log.from = player;
            log.to << death.damage->from;
            log.arg = objectName();
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            QList<const Skill *> skills = death.damage->from->getVisibleSkillList();
            QStringList detachList;
            foreach (const Skill *skill, skills) {
                if (!skill->inherits("SPConvertSkill") && !skill->isAttachedLordSkill())
                    detachList.append("-" + skill->objectName());
            }
            room->handleAcquireDetachSkills(death.damage->from, detachList);
            if (death.damage->from->isAlive())
                death.damage->from->gainMark("@qihuang");
        }

        return false;
    }
};

class IkYindie: public TriggerSkill {
public:
    IkYindie(): TriggerSkill("ikyindie") {
        events << CardsMoveOneTime << FinishJudge;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player) || (room->getCurrent() == player && player->getPhase() != Player::NotActive))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        return false;
    }
};

class IkYindieMove: public TriggerSkill {
public:
    IkYindieMove(): TriggerSkill("#ikyindie-move") {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == "ikyindie") {
                if (judge->isGood()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStar judge = data.value<JudgeStar>();
        player->addToPile("ikyindiepile", judge->card->getEffectiveId());

        return false;
    }
};

class IkYindieDistance: public DistanceSkill {
public:
    IkYindieDistance(): DistanceSkill("#ikyindie-dist") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill("ikyindie"))
            return -from->getPile("ikyindiepile").length();
        else
            return 0;
    }
};

class IkGuiyue: public PhaseChangeSkill {
public:
    IkGuiyue(): PhaseChangeSkill("ikguiyue") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("@guiyue") == 0
               && target->getPile("ikyindiepile").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *dengai) const{
        Room *room = dengai->getRoom();
        room->notifySkillInvoked(dengai, objectName());

        LogMessage log;
        log.type = "#IkGuiyueWake";
        log.from = dengai;
        log.arg = QString::number(dengai->getPile("ikyindiepile").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(dengai, "@guiyue", 1);
        if (room->changeMaxHpForAwakenSkill(dengai))
            room->acquireSkill(dengai, "ikhuanwu");

        return false;
    }
};

class IkHuanwu: public OneCardViewAsSkill {
public:
    IkHuanwu(): OneCardViewAsSkill("ikhuanwu") {
        expand_pile = "ikyindiepile";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("ikyindiepile").isEmpty();
    }

    virtual bool viewFilter(const Card *to_select) const{
        return Self->getPile("ikyindiepile").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Snatch *snatch = new Snatch(originalCard->getSuit(), originalCard->getNumber());
        snatch->addSubcard(originalCard);
        snatch->setSkillName(objectName());
        return snatch;
    }
};

class IkHeyi: public TriggerSkill {
public:
    IkHeyi(): TriggerSkill("ikheyi") {
        events << TargetSpecified << TargetConfirmed;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *sunce, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(sunce)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified || (triggerEvent == TargetConfirmed && use.to.contains(sunce)))
            if (use.card->isKindOf("Duel") || (use.card->isKindOf("Slash") && use.card->isRed()))
                return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *sunce, QVariant &, ServerPlayer *) const{
        if (sunce->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *sunce, QVariant &, ServerPlayer *) const{ 
        sunce->drawCards(1, objectName());
        return false;
    }
};

class IkChizhu: public PhaseChangeSkill {
public:
    IkChizhu(): PhaseChangeSkill("ikchizhu") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("@chizhu") == 0
               && target->getPhase() == Player::Start
               && target->getHp() <= 2;
    }

    virtual bool onPhaseChange(ServerPlayer *sunce) const{
        Room *room = sunce->getRoom();
        room->notifySkillInvoked(sunce, objectName());

        LogMessage log;
        log.type = "#IkChizhuWake";
        log.from = sunce;
        log.arg = objectName();
        log.arg2 = QString::number(sunce->getHp());
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(sunce, "@chizhu", 1);
        if (room->changeMaxHpForAwakenSkill(sunce))
            room->handleAcquireDetachSkills(sunce, "ikchenhong|ikliangban");
        return false;
    }
};

IkBianshengCard::IkBianshengCard() {
    mute = true;
    m_skillName = "ikbiansheng_pindian";
}

bool IkBianshengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("ikbiansheng") && to_select != Self
           && !to_select->isKongcheng() && !to_select->hasFlag("IkBianshengInvoked");
}

void IkBianshengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *sunce = targets.first();
    room->setPlayerFlag(sunce, "IkBianshengInvoked");
    room->notifySkillInvoked(sunce, "ikbiansheng");
    if (sunce->getMark("@chizhu") > 0 && room->askForChoice(sunce, "ikbiansheng_pindian", "accept+reject") == "reject") {
        LogMessage log;
        log.type = "#IkBianshengReject";
        log.from = sunce;
        log.to << source;
        log.arg = "ikbiansheng_pindian";
        room->sendLog(log);

        return;
    }

    source->pindian(sunce, "ikbiansheng_pindian", NULL);
    QList<ServerPlayer *> sunces;
    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach (ServerPlayer *p, players) {
        if (p->hasLordSkill("ikbiansheng") && !p->hasFlag("IkBianshengInvoked"))
            sunces << p;
    }
    if (sunces.isEmpty())
        room->setPlayerFlag(source, "ForbidIkBiansheng");
}

class IkBianshengPindian: public ZeroCardViewAsSkill {
public:
    IkBianshengPindian(): ZeroCardViewAsSkill("ikbiansheng_pindian") {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getKingdom() == "yuki" && !player->isKongcheng() && !player->hasFlag("ForbidIkBiansheng");
    }

    virtual const Card *viewAs() const{
        return new IkBianshengCard;
    }
};

class IkBiansheng: public TriggerSkill {
public:
    IkBiansheng(): TriggerSkill("ikbiansheng$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Pindian << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (player == NULL) return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "ikbiansheng")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("ikbiansheng_pindian"))
                    room->attachSkillToPlayer(p, "ikbiansheng_pindian");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "ikbiansheng") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("ikbiansheng_pindian"))
                    room->detachSkillFromPlayer(p, "ikbiansheng_pindian", true);
            }
        } else if (triggerEvent == Pindian) {
            PindianStar pindian = data.value<PindianStar>();
            if (pindian->reason != "ikbiansheng_pindian" || !pindian->to->hasLordSkill(objectName()))
                return QStringList();
            if (!pindian->isSuccess()) {
                ask_who = pindian->to;
                return QStringList(objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return QStringList();
            if (player->hasFlag("ForbidIkBiansheng"))
                room->setPlayerFlag(player, "-ForbidIkBiansheng");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("IkBianshengInvoked"))
                    room->setPlayerFlag(p, "-IkBianshengInvoked");
            }
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const{
        return ask_who->askForSkillInvoke(objectName(), "pindian");
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        PindianStar pindian = data.value<PindianStar>();
        if (room->getCardPlace(pindian->from_card->getEffectiveId()) == Player::PlaceTable)
            ask_who->obtainCard(pindian->from_card);
        if (room->getCardPlace(pindian->to_card->getEffectiveId()) == Player::PlaceTable)
            ask_who->obtainCard(pindian->to_card);
        return false;
    }
};

IkTiaoxinCard::IkTiaoxinCard() {
}

bool IkTiaoxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->inMyAttackRange(Self);
}

void IkTiaoxinCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    bool use_slash = false;
    if (effect.to->canSlash(effect.from, NULL, false))
        use_slash = room->askForUseSlashTo(effect.to, effect.from, "@iktiaoxin-slash:" + effect.from->objectName());
    if (!use_slash && effect.from->canDiscard(effect.to, "he"))
        room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "iktiaoxin", false, Card::MethodDiscard), effect.to, effect.from);
}

class IkTiaoxin: public ZeroCardViewAsSkill {
public:
    IkTiaoxin(): ZeroCardViewAsSkill("iktiaoxin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkTiaoxinCard");
    }

    virtual const Card *viewAs() const{
        return new IkTiaoxinCard;
    }
};

class IkShengtian: public PhaseChangeSkill {
public:
    IkShengtian(): PhaseChangeSkill("ikshengtian") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("@shengtian") == 0
               && target->getPhase() == Player::Start
               && target->isKongcheng();
    }

    virtual bool onPhaseChange(ServerPlayer *jiangwei) const{
        Room *room = jiangwei->getRoom();
        room->notifySkillInvoked(jiangwei, objectName());

        LogMessage log;
        log.type = "#IkShengtianWake";
        log.from = jiangwei;
        log.arg = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(jiangwei, "@shengtian", 1);
        if (room->changeMaxHpForAwakenSkill(jiangwei)) {
            if (jiangwei->isWounded() && room->askForChoice(jiangwei, objectName(), "recover+draw") == "recover")
                room->recover(jiangwei, RecoverStruct(jiangwei));
            else
                room->drawCards(jiangwei, 2, objectName());
            room->handleAcquireDetachSkills(jiangwei, "ikxuanwu|ikmohua");
        }

        return false;
    }
};

class IkMohua: public FilterSkill {
public:
    IkMohua(): FilterSkill("ikmohua") {
    }

    static WrappedCard *changeToClub(int cardId) {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("ikmohua");
        new_card->setSuit(Card::Club);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->getSuit() == Card::Diamond;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        return changeToClub(originalCard->getEffectiveId());
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return -2;
    }
};

IkJibanCard::IkJibanCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkJibanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void IkJibanCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *erzhang = effect.from;
    erzhang->getRoom()->moveCardTo(this, erzhang, effect.to, Player::PlaceEquip,
                                   CardMoveReason(CardMoveReason::S_REASON_PUT,
                                                  erzhang->objectName(), "ikjiban", QString()));

    LogMessage log;
    log.type = "$IkJibanEquip";
    log.from = effect.to;
    log.card_str = QString::number(getEffectiveId());
    erzhang->getRoom()->sendLog(log);

    erzhang->drawCards(1, "ikjiban");
}

class IkJiban: public OneCardViewAsSkill {
public:
    IkJiban():OneCardViewAsSkill("ikjiban") {
        filter_pattern = "EquipCard|.|.|hand";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkJibanCard *ikjiban_card = new IkJibanCard();
        ikjiban_card->addSubcard(originalCard);
        return ikjiban_card;
    }
};

class IkJizhou: public TriggerSkill {
public:
    IkJizhou(): TriggerSkill("ikjizhou") {
        events << EventPhaseEnd << EventPhaseChanging;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard)
                foreach (ServerPlayer *erzhang, room->findPlayersBySkillName(objectName())) {
                    erzhang->tag.remove("IkJizhouToGet");
                    erzhang->tag.remove("IkJizhouOther");
                }
        } else if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() != Player::Discard) return skill_list;
            QList<ServerPlayer *> erzhangs = room->findPlayersBySkillName(objectName());

            foreach(ServerPlayer *erzhang, erzhangs) {
                QVariantList ikjizhou_cardsToGet = erzhang->tag["IkJizhouToGet"].toList();
                foreach(QVariant card_data, ikjizhou_cardsToGet) {
                    int card_id = card_data.toInt();
                    if (room->getCardPlace(card_id) == Player::DiscardPile)
                        skill_list.insert(erzhang, QStringList(objectName()));
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *erzhang) const {
        if (erzhang->askForSkillInvoke(objectName())){
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *erzhang) const{
        QVariantList ikjizhou_cardsToGet = erzhang->tag["IkJizhouToGet"].toList();
        QVariantList ikjizhou_cardsOther = erzhang->tag["IkJizhouOther"].toList();
        erzhang->tag.remove("IkJizhouToGet");
        erzhang->tag.remove("IkJizhouOther");

        QList<int> cardsToGet;
        foreach(QVariant card_data, ikjizhou_cardsToGet) {
            int card_id = card_data.toInt();
            if (room->getCardPlace(card_id) == Player::DiscardPile)
                cardsToGet << card_id;
        }
        QList<int> cardsOther;
        foreach(QVariant card_data, ikjizhou_cardsOther) {
            int card_id = card_data.toInt();
            if (room->getCardPlace(card_id) == Player::DiscardPile)
                cardsOther << card_id;
        }

        if (cardsToGet.isEmpty())
            return false;

        QList<int> cards = cardsToGet + cardsOther;

        room->fillAG(cards, erzhang, cardsOther);

        int to_back = room->askForAG(erzhang, cardsToGet, false, objectName());
        player->obtainCard(Sanguosha->getCard(to_back));

        cards.removeOne(to_back);

        room->clearAG(erzhang);

        DummyCard dummy(cards);
        room->obtainCard(erzhang, &dummy);

        return false;
    }
};

class IkJizhouRecord : public TriggerSkill {
public:
    IkJizhouRecord() : TriggerSkill("#ikjizhou-record") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *erzhang, QVariant &data, ServerPlayer * &) const{
        if (!erzhang || !erzhang->isAlive() || !erzhang->hasSkill("ikjizhou")) return QStringList();
        ServerPlayer *current = room->getCurrent();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (!current || erzhang == current || current->getPhase() != Player::Discard)
            return QStringList();

        if (current->getPhase() == Player::Discard) {
            QVariantList ikjizhouToGet = erzhang->tag["IkJizhouToGet"].toList();
            QVariantList ikjizhouOther = erzhang->tag["IkJizhouOther"].toList();

            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                int i = 0;
                foreach (int card_id, move.card_ids) {
                    if (move.from == current && move.from_places[i] == Player::PlaceHand)
                        ikjizhouToGet << card_id;
                    else if (!ikjizhouToGet.contains(card_id))
                        ikjizhouOther << card_id;
                    i++;
                }
            }

            erzhang->tag["IkJizhouToGet"] = ikjizhouToGet;
            erzhang->tag["IkJizhouOther"] = ikjizhouOther;
        }

        return QStringList();
    }
};

class IkManbo: public TriggerSkill {
public:
    IkManbo(): TriggerSkill("ikmanbo") {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *liushan, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(liushan) && use.card->isKindOf("Slash"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(liushan, objectName());

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = liushan;
        log.arg = objectName();
        room->sendLog(log);

        if (!room->askForCard(use.from, ".Basic", "@ikmanbo-discard")) {
            use.nullified_list << liushan->objectName();
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

IkBaishenCard::IkBaishenCard() {
}

bool IkBaishenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void IkBaishenCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    ServerPlayer *liushan = effect.from, *player = effect.to;

    LogMessage log;
    log.type = "#IkBaishen";
    log.from = liushan;
    log.to << player;
    room->sendLog(log);

    room->setTag("IkBaishenTarget", QVariant::fromValue((PlayerStar)player));
}

class IkBaishenViewAsSkill: public OneCardViewAsSkill {
public:
    IkBaishenViewAsSkill(): OneCardViewAsSkill("ikbaishen") {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@ikbaishen";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkBaishenCard *ikbaishen = new IkBaishenCard;
        ikbaishen->addSubcard(originalCard);
        return ikbaishen;
    }
};

class IkBaishen: public TriggerSkill {
public:
    IkBaishen(): TriggerSkill("ikbaishen") {
        events << EventPhaseChanging << EventPhaseStart;
        view_as_skill = new IkBaishenViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            switch (change.to) {
            case Player::Play: {
                    if (!TriggerSkill::triggerable(liushan) || liushan->isSkipped(Player::Play))
                        return QStringList();
                    return QStringList(objectName());
                }
            case Player::NotActive: {
                    if (liushan->hasFlag(objectName())) {
                        if (!liushan->canDiscard(liushan, "h"))
                            return QStringList();
                        return QStringList(objectName());
                    }
                    break;
                }
            default:
                    break;
            }
        } else if (triggerEvent == EventPhaseStart && liushan->getPhase() == Player::NotActive) {
            Room *room = liushan->getRoom();
            if (!room->getTag("IkBaishenTarget").isNull()) {
                PlayerStar target = room->getTag("IkBaishenTarget").value<PlayerStar>();
                if (target->isAlive())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            switch (change.to) {
            case Player::Play: {
                if (liushan->askForSkillInvoke(objectName()))
                    return true;
                }
            case Player::NotActive: {
                room->askForUseCard(liushan, "@@ikbaishen", "@ikbaishen-give", -1, Card::MethodDiscard);
                }
            default:
                    break;
            }
        } else if (triggerEvent == EventPhaseStart)
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *liushan, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            liushan->setFlags(objectName());
            liushan->skip(Player::Play, true);
        } else if (triggerEvent == EventPhaseStart) {
            PlayerStar target = room->getTag("IkBaishenTarget").value<PlayerStar>();
            room->removeTag("IkBaishenTarget");
            target->gainAnExtraTurn();
        }
        return false;
    }
};

class IkHunshou: public PhaseChangeSkill {
public:
    IkHunshou(): PhaseChangeSkill("ikhunshou$") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        bool can_invoke = true;
        foreach (ServerPlayer *p, target->getRoom()->getAllPlayers()) {
            if (target->getHp() > p->getHp()) {
                can_invoke = false;
                break;
            }
        }
        return can_invoke && target != NULL && target->getPhase() == Player::Start
               && target->hasLordSkill("ikhunshou")
               && target->isAlive()
               && target->getMark("@hunshou") == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *liushan) const{
        Room *room = liushan->getRoom();
        room->notifySkillInvoked(liushan, objectName());

        LogMessage log;
        log.type = "#IkHunshouWake";
        log.from = liushan;
        log.arg = QString::number(liushan->getHp());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(liushan, "@hunshou", 1);
        if (room->changeMaxHpForAwakenSkill(liushan, 1)) {
            room->recover(liushan, RecoverStruct(liushan));
            if (liushan->isLord())
                room->acquireSkill(liushan, "ikxinqi");
        }

        return false;
    }
};

class IkHuanshen: public PhaseChangeSkill {
public:
    IkHuanshen(): PhaseChangeSkill("ikhuanshen") {
    }
    
    static void playAudioEffect(ServerPlayer *zuoci, const QString &skill_name) {
        zuoci->getRoom()->broadcastSkillInvoke(skill_name, zuoci->isMale(), -1);
    }

    static void AcquireGenerals(ServerPlayer *zuoci, int n) {
        Room *room = zuoci->getRoom();
        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        QStringList list = GetAvailableGenerals(zuoci);
        qShuffle(list);
        if (list.isEmpty()) return;
        n = qMin(n, list.length());

        QStringList acquired = list.mid(0, n);
        foreach (QString name, acquired) {
            huashens << name;
            const General *general = Sanguosha->getGeneral(name);
            if (general) {
                foreach (const TriggerSkill *skill, general->getTriggerSkills()) {
                    if (skill->isVisible())
                        room->getThread()->addTriggerSkill(skill);
                }
            }
        }
        zuoci->tag["IkHuanshens"] = huashens;

        QStringList hidden;
        for (int i = 0; i < n; i++) hidden << "unknown";
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p == zuoci)
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), acquired.join(":"), QList<ServerPlayer *>() << p);
            else
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), hidden.join(":"), QList<ServerPlayer *>() << p);
        }

        LogMessage log;
        log.type = "#GetIkHuanshen";
        log.from = zuoci;
        log.arg = QString::number(n);
        log.arg2 = QString::number(huashens.length());
        room->sendLog(log);

        LogMessage log2;
        log2.type = "#GetIkHuanshenDetail";
        log2.from = zuoci;
        log2.arg = acquired.join("\\, \\");
        room->sendLog(log2, zuoci);

        room->setPlayerMark(zuoci, "@huanshen", huashens.length());
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci) {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames().toSet();
        Room *room = zuoci->getRoom();
        if (isNormalGameMode(room->getMode())
            || room->getMode().contains("_mini_")
            || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        } else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/1v1", "").toStringList().toSet());
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }
        QSet<QString> huashen_set, room_set;
        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        foreach (QVariant huashen, huashens)
            huashen_set << huashen.toString();
        foreach (ServerPlayer *player, room->getAlivePlayers()) {
            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;

            if (!player->getGeneral2()) continue;

            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;
        }

        static QSet<QString> banned;
        if (banned.isEmpty())
            banned << "zuoci";

        return (all - banned - huashen_set - room_set).toList();
    }

    static void SelectSkill(ServerPlayer *zuoci) {
        Room *room = zuoci->getRoom();
        playAudioEffect(zuoci, "ikhuanshen");
        QStringList ac_dt_list;

        QString huashen_skill = zuoci->tag["IkHuanshenSkill"].toString();
        if (!huashen_skill.isEmpty())
            ac_dt_list.append("-" + huashen_skill);

        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        if (huashens.isEmpty()) return;

        QStringList huashen_generals;
        foreach (QVariant huashen, huashens)
            huashen_generals << huashen.toString();

        QStringList skill_names;
        QString skill_name;
        const General *general = NULL;
        AI* ai = zuoci->getAI();
        if (ai) {
            QHash<QString, const General *> hash;
            foreach (QString general_name, huashen_generals) {
                const General *general = Sanguosha->getGeneral(general_name);
                foreach (const Skill *skill, general->getVisibleSkillList()) {
                    if (skill->isLordSkill()
                        || skill->getFrequency() == Skill::Limited
                        || skill->getFrequency() == Skill::Wake)
                        continue;

                    if (!skill_names.contains(skill->objectName())) {
                        hash[skill->objectName()] = general;
                        skill_names << skill->objectName();
                    }
                }
            }
            if (skill_names.isEmpty()) return;
            skill_name = ai->askForChoice("ikhuanshen", skill_names.join("+"), QVariant());
            general = hash[skill_name];
            Q_ASSERT(general != NULL);
        } else {
            QString general_name = room->askForGeneral(zuoci, huashen_generals);
            general = Sanguosha->getGeneral(general_name);

            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill()
                    || skill->getFrequency() == Skill::Limited
                    || skill->getFrequency() == Skill::Wake)
                    continue;

                skill_names << skill->objectName();
            }

            if (!skill_names.isEmpty())
                skill_name = room->askForChoice(zuoci, "ikhuanshen", skill_names.join("+"));
        }
        //Q_ASSERT(!skill_name.isNull() && !skill_name.isEmpty());

        QString kingdom = general->getKingdom();
        if (zuoci->getKingdom() != kingdom) {
            if (kingdom == "kami")
                kingdom = room->askForKingdom(zuoci);
            room->setPlayerProperty(zuoci, "kingdom", kingdom);
        }

        if (zuoci->getGender() != general->getGender())
            zuoci->setGender(general->getGender());

        Json::Value arg(Json::arrayValue);
        arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        arg[1] = QSanProtocol::Utils::toJsonString(zuoci->objectName());
        arg[2] = QSanProtocol::Utils::toJsonString(general->objectName());
        arg[3] = QSanProtocol::Utils::toJsonString(skill_name);
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        zuoci->tag["IkHuanshenSkill"] = skill_name;
        if (!skill_name.isEmpty())
            ac_dt_list.append(skill_name);
        room->handleAcquireDetachSkills(zuoci, ac_dt_list, true);
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && (target->getPhase() == Player::RoundStart || target->getPhase() == Player::NotActive);
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *zuoci, QVariant &, ServerPlayer *) const{
        return zuoci->askForSkillInvoke(objectName());
    }

    virtual bool onPhaseChange(ServerPlayer *zuoci) const{
        SelectSkill(zuoci);
        return false;
    }

    virtual QDialog *getDialog() const{
        static IkHuanshenDialog *dialog;

        if (dialog == NULL)
            dialog = new IkHuanshenDialog;

        return dialog;
    }
};

class IkHuanshenStart: public GameStartSkill {
public:
    IkHuanshenStart(): GameStartSkill("#ikhuanshen-start") {
        frequency = Compulsory;
    }

    virtual void onGameStart(ServerPlayer *zuoci) const{
        zuoci->getRoom()->notifySkillInvoked(zuoci, "ikhuanshen");
        IkHuanshen::AcquireGenerals(zuoci, 2);
        IkHuanshen::SelectSkill(zuoci);
    }
};

IkHuanshenDialog::IkHuanshenDialog() {
    setWindowTitle(Sanguosha->translate("ikhuanshen"));
}

void IkHuanshenDialog::popup() {
    QVariantList huashen_list = Self->tag["IkHuanshens"].toList();
    QList<const General *> huashens;
    foreach (QVariant huashen, huashen_list)
        huashens << Sanguosha->getGeneral(huashen.toString());

    fillGenerals(huashens);
    show();
}

class IkHuanshenClear: public DetachEffectSkill {
public:
    IkHuanshenClear(): DetachEffectSkill("ikhuanshen") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        if (player->getKingdom() != player->getGeneral()->getKingdom() && player->getGeneral()->getKingdom() != "kami")
            room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
        if (player->getGender() != player->getGeneral()->getGender())
            player->setGender(player->getGeneral()->getGender());
        QString huashen_skill = player->tag["IkHuanshenSkill"].toString();
        if (!huashen_skill.isEmpty())
            room->detachSkillFromPlayer(player, huashen_skill, false, true);
        player->tag.remove("IkHuanshens");
        room->setPlayerMark(player, "@huanshen", 0);
    }
};

class IkLingqi: public MasochismSkill {
public:
    IkLingqi(): MasochismSkill("iklingqi") {
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *zuoci, const DamageStruct &damage) const{
        if (zuoci->askForSkillInvoke(objectName())) {
            IkHuanshen::playAudioEffect(zuoci, objectName());
            IkHuanshen::AcquireGenerals(zuoci, damage.damage);
        }
    }
};

MountainPackage::MountainPackage()
    : Package("mountain")
{
    General *bloom009 = new General(this, "bloom009", "hana");
    bloom009->addSkill(new IkMancai);

    General *bloom015 = new General(this, "bloom015", "hana");
    bloom015->addSkill(new IkYindie);
    bloom015->addSkill(new IkYindieMove);
    bloom015->addSkill(new IkYindieDistance);
    bloom015->addSkill(new IkGuiyue);
    bloom015->addRelateSkill("ikhuanwu");
    related_skills.insertMulti("ikyindie", "#ikyindie-move");
    related_skills.insertMulti("ikyindie", "#ikyindie-dist");

    General *wind012 = new General(this, "wind012", "kaze");
    wind012->addSkill(new IkTiaoxin);
    wind012->addSkill(new IkShengtian);
    wind012->addRelateSkill("ikxuanwu");
    wind012->addRelateSkill("ikmohua");

    General *wind014 = new General(this, "wind014$", "kaze", 3);
    wind014->addSkill(new IkManbo);
    wind014->addSkill(new IkBaishen);
    wind014->addSkill(new IkHunshou);

    General *snow014 = new General(this, "snow014$", "yuki");
    snow014->addSkill(new IkHeyi);
    snow014->addSkill(new IkChizhu);
    snow014->addSkill(new IkBiansheng);

    General *snow015 = new General(this, "snow015", "yuki", 3);
    snow015->addSkill(new IkJiban);
    snow015->addSkill(new IkJizhou);
    snow015->addSkill(new IkJizhouRecord);
    related_skills.insertMulti("ikjizhou", "#ikjizhou-record");

    General *luna009 = new General(this, "luna009", "tsuki", 3);
    luna009->addSkill(new IkHuanshen);
    luna009->addSkill(new IkHuanshenStart);
    luna009->addSkill(new IkHuanshenClear);
    luna009->addSkill(new IkLingqi);
    related_skills.insertMulti("ikhuanshen", "#ikhuanshen-start");
    related_skills.insertMulti("ikhuanshen", "#ikhuanshen-clear");

    General *luna012 = new General(this, "luna012", "tsuki", 3, false);
    luna012->addSkill(new IkHuiyao);
    luna012->addSkill(new IkQihuang);

    addMetaObject<IkMancaiCard>();
    addMetaObject<IkTiaoxinCard>();
    addMetaObject<IkJibanCard>();
    addMetaObject<IkBianshengCard>();
    addMetaObject<IkBaishenCard>();

    skills << new IkBianshengPindian << new IkHuanwu << new IkMohua;
}

ADD_PACKAGE(Mountain)

