#include "yjcm2013.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"

class Juece: public PhaseChangeSkill {
public:
    Juece(): PhaseChangeSkill("juece") {
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Finish) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> kongcheng_players;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->isKongcheng())
                kongcheng_players << p;
        }
        if (kongcheng_players.isEmpty()) return false;

        ServerPlayer *to_damage = room->askForPlayerChosen(target, kongcheng_players, objectName(),
                                                           "@juece", true, true);
        if (to_damage) {
            room->broadcastSkillInvoke(objectName());
            room->damage(DamageStruct(objectName(), target, to_damage));
        }
        return false;
    }
};

MiejiCard::MiejiCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool MiejiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void MiejiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, effect.from->objectName(), QString(), "mieji", QString());
    room->moveCardTo(this, effect.from, NULL, Player::DrawPile, reason, true);

    int trick_num = 0, nontrick_num = 0;
    foreach (const Card *c, effect.to->getCards("he")) {
        if (effect.to->canDiscard(effect.to, c->getId())) {
            if (c->isKindOf("TrickCard"))
                trick_num++;
            else
                nontrick_num++;
        }
    }
    bool discarded = room->askForDiscard(effect.to, "mieji", 1, qMin(1, trick_num), nontrick_num > 1, true, "@mieji-trick", "TrickCard");
    if (trick_num == 0 || !discarded)
        room->askForDiscard(effect.to, "mieji", 2, 2, false, true, "@mieji-nontrick", "^TrickCard");
}

class Mieji: public OneCardViewAsSkill {
public:
    Mieji(): OneCardViewAsSkill("mieji") {
        filter_pattern = "TrickCard|black";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("MiejiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        MiejiCard *card = new MiejiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Fencheng: public ZeroCardViewAsSkill {
public:
    Fencheng(): ZeroCardViewAsSkill("fencheng") {
        frequency = Limited;
        limit_mark = "@burn";
    }

    virtual const Card *viewAs() const{
        return new FenchengCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@burn") >= 1;
    }
};

class FenchengMark: public TriggerSkill {
public:
    FenchengMark(): TriggerSkill("#fencheng") {
        events << ChoiceMade;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        QStringList data_str = data.toString().split(":");
        if (data_str.length() != 3 || data_str.first() != "cardDiscard" || data_str.at(1) != "fencheng")
            return false;
        room->setTag("FenchengDiscard", data_str.last().split("+").length());
        return false;
    }
};

FenchengCard::FenchengCard() {
    mute = true;
    target_fixed = true;
}

void FenchengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@burn");
    room->broadcastSkillInvoke("fencheng");
    room->doLightbox("$FenchengAnimate", 3000);
    room->setTag("FenchengDiscard", 0);

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    source->setFlags("FenchengUsing");
    try {
        foreach (ServerPlayer *player, players) {
            if (player->isAlive())
                room->cardEffect(this, source, player);
                room->getThread()->delay();
        }
        source->setFlags("-FenchengUsing");
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            source->setFlags("-FenchengUsing");
        throw triggerEvent;
    }
}

void FenchengCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    int length = room->getTag("FenchengDiscard").toInt() + 1;
    if (!effect.to->canDiscard(effect.to, "he") || effect.to->getCardCount(true) < length
        || !room->askForDiscard(effect.to, "fencheng", 1000, length, true, true, "@fencheng:::" + QString::number(length))) {
        room->setTag("FenchengDiscard", 0);
        room->damage(DamageStruct("fencheng", effect.from, effect.to, 2, DamageStruct::Fire));
    }
}

class Zhuikong: public TriggerSkill {
public:
    Zhuikong(): TriggerSkill("zhuikong") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        foreach (ServerPlayer *fuhuanghou, room->getAllPlayers()) {
            if (TriggerSkill::triggerable(fuhuanghou)
                && player != fuhuanghou && fuhuanghou->isWounded() && !fuhuanghou->isKongcheng()
                && room->askForSkillInvoke(fuhuanghou, objectName())) {
                room->broadcastSkillInvoke("zhuikong");
                if (fuhuanghou->pindian(player, objectName(), NULL)) {
                    room->setPlayerFlag(player, "zhuikong");
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

class ZhuikongClear: public TriggerSkill {
public:
    ZhuikongClear(): TriggerSkill("#zhuikong-clear") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;

        QVariantList zhuikonglist = player->tag["zhuikong"].toList();
        if (zhuikonglist.isEmpty()) return false;
        foreach (QVariant p, zhuikonglist) {
            ServerPlayer *fuhuanghou = p.value<ServerPlayer *>();
            room->setFixedDistance(player, fuhuanghou, -1);
        }
        player->tag.remove("zhuikong");
        return false;
    }
};

class ZhuikongProhibit: public ProhibitSkill {
public:
    ZhuikongProhibit(): ProhibitSkill("#zhuikong") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        if (card->getTypeId() != Card::TypeSkill && from->hasFlag("zhuikong"))
            return to != from;
        return false;
    }
};

class Qiuyuan: public TriggerSkill {
public:
    Qiuyuan(): TriggerSkill("qiuyuan") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p != use.from)
                    targets << p;
            }
            if (targets.isEmpty()) return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "qiuyuan-invoke", true, true);
            if (target) {
                if (target->getGeneralName().contains("fuwan") || target->getGeneral2Name().contains("fuwan"))
                    room->broadcastSkillInvoke("qiuyuan", 2);
                else
                    room->broadcastSkillInvoke("qiuyuan", 1);
                const Card *card = NULL;
                if (!target->isKongcheng())
                    card = room->askForCard(target, "Jink", "@qiuyuan-give:" + player->objectName(), data, Card::MethodNone);
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), "nosqiuyuan", QString());
                if (!card) {
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
                } else {
                    room->obtainCard(player, card, reason);
                }
            }
        }
        return false;
    }
};

YJCM2013Package::YJCM2013Package()
    : Package("YJCM2013")
{

    General *fuhuanghou = new General(this, "fuhuanghou", "qun", 3, false); // YJ 202
    fuhuanghou->addSkill(new Zhuikong);
    fuhuanghou->addSkill(new ZhuikongClear);
    fuhuanghou->addSkill(new ZhuikongProhibit);
    fuhuanghou->addSkill(new Qiuyuan);
    related_skills.insertMulti("zhuikong", "#zhuikong");
    related_skills.insertMulti("zhuikong", "#zhuikong-clear");

    General *liru = new General(this, "liru", "qun", 3); // YJ 206
    liru->addSkill(new Juece);
    liru->addSkill(new Mieji);
    liru->addSkill(new Fencheng);
    liru->addSkill(new FenchengMark);
    related_skills.insertMulti("fencheng", "#fencheng");

    addMetaObject<MiejiCard>();
    addMetaObject<FenchengCard>();
}

ADD_PACKAGE(YJCM2013)
