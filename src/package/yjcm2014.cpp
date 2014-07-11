#include "yjcm2014.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"

class Youdi: public PhaseChangeSkill {
public:
    Youdi(): PhaseChangeSkill("youdi") {
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Finish || target->isNude()) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> players;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->canDiscard(target, "he")) players << p;
        }
        if (players.isEmpty()) return false;
        ServerPlayer *player = room->askForPlayerChosen(target, players, objectName(), "youdi-invoke", true, true);
        if (player) {
            int id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, target, player);
            if (!Sanguosha->getCard(id)->isKindOf("Slash") && player->isAlive() && !player->isNude()) {
                int id2= room->askForCardChosen(target, player, "he", "youdi_obtain");
                room->obtainCard(target, id2);
            }
        }
        return false;
    }
};class Qieting: public TriggerSkill {
public:
    Qieting(): TriggerSkill("qieting") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive || player->getMark("qieting") > 0) return false;
        foreach (ServerPlayer *caifuren, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(caifuren) || caifuren == player) continue;
            QStringList choices;
            for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
                if (player->getEquip(i) && !caifuren->getEquip(i))
                    choices << QString::number(i);
            }
            choices << "draw" << "cancel";
            QString choice = room->askForChoice(caifuren, objectName(), choices.join("+"), QVariant::fromValue(player));
            if (choice == "cancel") {
                continue;
            } else {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = caifuren;
                room->sendLog(log);
                room->notifySkillInvoked(caifuren, objectName());
                if (choice == "draw") {
                    room->broadcastSkillInvoke(objectName(), 2);
                    caifuren->drawCards(1, objectName());
                } else {
                    room->broadcastSkillInvoke(objectName(), 1);
                    int index = choice.toInt();
                    const Card *card = player->getEquip(index);
                    room->moveCardTo(card, caifuren, Player::PlaceEquip);
                }
            }
        }
        return false;
    }
};

class QietingRecord: public TriggerSkill {
public:
    QietingRecord(): TriggerSkill("#qieting-record") {
        events << PreCardUsed << TurnStart;
        //global = true;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const{
        return 6;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreCardUsed && player->isAlive() && player->getPhase() != Player::NotActive
            && player->getMark("qieting") == 0) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill) {
                foreach (ServerPlayer *p, use.to) {
                    if (p != player) {
                        player->addMark("qieting");
                        return false;
                    }
                }
            }
        } else if (triggerEvent == TurnStart) {
            player->setMark("qieting", 0);
        }
        return false;
    }
};

XianzhouDamageCard::XianzhouDamageCard() {
    mute = true;
}

void XianzhouDamageCard::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, use.from, data);
}

bool XianzhouDamageCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == Self->getMark("xianzhou");
}

bool XianzhouDamageCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < Self->getMark("xianzhou") && Self->inMyAttackRange(to_select);
}

void XianzhouDamageCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->damage(DamageStruct("xianzhou", effect.from, effect.to));
}

XianzhouCard::XianzhouCard() {
}

bool XianzhouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void XianzhouCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->removePlayerMark(effect.from, "@handover");
    room->doLightbox("$XianzhouAnimate");

    int len = 0;
    DummyCard *dummy = new DummyCard;
    foreach (const Card *c, effect.from->getEquips()) {
        dummy->addSubcard(c);
        len++;
    }
    room->setPlayerMark(effect.to, "xianzhou", len);
    effect.to->obtainCard(dummy);
    delete dummy;

    bool rec = true;
    int count = 0;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
        if (effect.to->inMyAttackRange(p)) {
            count++;
            if (count >= len) {
                rec = false;
                break;
            }
        }
    }

    if ((rec || !room->askForUseCard(effect.to, "@xianzhou", "@xianzhou-damage:::" + QString::number(len)))
        && effect.from->isWounded())
        room->recover(effect.from, RecoverStruct(effect.to, NULL, len));
}

class Xianzhou: public ZeroCardViewAsSkill {
public:
    Xianzhou(): ZeroCardViewAsSkill("xianzhou") {
        frequency = Skill::Limited;
        limit_mark = "@handover";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@handover") > 0 && player->getEquips().length() > 0;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@xianzhou";
    }

    virtual const Card *viewAs() const{
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@xianzhou") {
            return new XianzhouDamageCard;
        } else {
            return new XianzhouCard;
        }
    }
};

YJCM2014Package::YJCM2014Package()
    : Package("YJCM2014")
{
    General *caifuren = new General(this, "caifuren", "qun", 3, false); // YJ 301
    caifuren->addSkill(new Qieting);
    caifuren->addSkill(new QietingRecord);
    caifuren->addSkill(new Xianzhou);
    related_skills.insertMulti("qieting", "#qieting-record");

    General *zhuhuan = new General(this, "zhuhuan", "wu"); // YJ 311
    zhuhuan->addSkill(new Youdi);

    addMetaObject<XianzhouCard>();
    addMetaObject<XianzhouDamageCard>();
}

ADD_PACKAGE(YJCM2014)
