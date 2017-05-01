#include "h-formation.h"
#include "general.h"
#include "standard.h"
#include "standard-equips.h"
#include "maneuvering.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "ai.h"
#include "settings.h"

class Ziliang: public TriggerSkill {
public:
    Ziliang(): TriggerSkill("ziliang") {
        events << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        foreach (ServerPlayer *dengai, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(dengai) || !player->isAlive()) break;
            if (dengai->getPile("assassinate").isEmpty()) continue;
            if (!room->askForSkillInvoke(dengai, objectName(), data)) continue;
            room->fillAG(dengai->getPile("assassinate"), dengai);
            int id = room->askForAG(dengai, dengai->getPile("assassinate"), false, objectName());
            room->clearAG(dengai);
            if (player == dengai) {
                LogMessage log;
                log.type = "$MoveCard";
                log.from = player;
                log.to << player;
                log.card_str = QString::number(id);
                room->sendLog(log);
            }
            room->obtainCard(player, id);
        }
        return false;
    }
};

class Tianfu: public TriggerSkill {
public:
    Tianfu(): TriggerSkill("tianfu") {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual int getPriority(TriggerEvent) const{
        return 4;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            QList<ServerPlayer *> jiangweis = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *jiangwei, jiangweis) {
                if (jiangwei->isAlive() && (player == jiangwei || player->isAdjacentTo(jiangwei))
                    && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(jiangwei))) {
                    if (player != jiangwei) {
                        room->notifySkillInvoked(jiangwei, objectName());
                        LogMessage log;
                        log.type = "#InvokeOthersSkill";
                        log.from = player;
                        log.to << jiangwei;
                        log.arg = objectName();
                        room->sendLog(log);
                    }
                    jiangwei->addMark(objectName());
                    room->acquireSkill(jiangwei, "ikxuanying");
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark(objectName()) > 0) {
                    p->setMark(objectName(), 0);
                    room->detachSkillFromPlayer(p, "ikxuanying", false, true);
                }
            }
        }
        return false;
    }
};

class Yicheng: public TriggerSkill {
public:
    Yicheng(): TriggerSkill("yicheng") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash")) return false;
        foreach (ServerPlayer *p, use.to) {
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(p))) {
                p->drawCards(1, objectName());
                if (p->isAlive() && p->canDiscard(p, "he"))
                    room->askForDiscard(p, objectName(), 1, 1, false, true);
            }
            if (!player->isAlive())
                break;
        }
        return false;
    }
};

class Qianhuan: public TriggerSkill {
public:
    Qianhuan(): TriggerSkill("qianhuan") {
        events << Damaged << TargetConfirming;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damaged && player->isAlive()) {
            ServerPlayer *yuji = room->findPlayerBySkillName(objectName());
            if (yuji && room->askForSkillInvoke(player, objectName(), "choice:" + yuji->objectName())) {
                room->broadcastSkillInvoke(objectName());
                if (yuji != player) {
                    room->notifySkillInvoked(yuji, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = player;
                    log.to << yuji;
                    log.arg = objectName();
                    room->sendLog(log);
                }

                int id = room->drawCard();
                Card::Suit suit = Sanguosha->getCard(id)->getSuit();
                bool duplicate = false;
                foreach (int card_id, yuji->getPile("sorcery")) {
                    if (Sanguosha->getCard(card_id)->getSuit() == suit) {
                        duplicate = true;
                        break;
                    }
                }
                yuji->addToPile("sorcery", id);
                if (duplicate) {
                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);
                }
            }
        } else if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeEquip || use.card->getTypeId() == Card::TypeSkill)
                return false;
            if (use.to.length() != 1) return false;
            ServerPlayer *yuji = room->findPlayerBySkillName(objectName());
            if (!yuji || yuji->getPile("sorcery").isEmpty()) return false;
            if (room->askForSkillInvoke(yuji, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(yuji, objectName());
                if (yuji == player || room->askForChoice(player, objectName(), "accept+reject", data) == "accept") {
                    QList<int> ids = yuji->getPile("sorcery");
                    int id = -1;
                    if (ids.length() > 1) {
                        room->fillAG(ids, yuji);
                        id = room->askForAG(yuji, ids, false, objectName());
                        room->clearAG(yuji);
                    } else {
                        id = ids.first();
                    }
                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);

                    LogMessage log;
                    if (use.from) {
                        log.type = "$CancelTarget";
                        log.from = use.from;
                    } else {
                        log.type = "$CancelTargetNoUser";
                    }
                    log.to = use.to;
                    log.arg = use.card->objectName();
                    room->sendLog(log);

                    use.to.clear();
                    data = QVariant::fromValue(use);
                } else {
                    LogMessage log;
                    log.type = "#IkBianshengReject";
                    log.from = player;
                    log.to << yuji;
                    log.arg = objectName();
                    room->sendLog(log);
                }
            }
        }
        return false;
    }
};

HFormationPackage::HFormationPackage()
    : Package("h_formation")
{
    General *heg_dengai = new General(this, "heg_dengai", "wei"); // WEI 015 G
    heg_dengai->addSkill("ikyindie");
    heg_dengai->addSkill(new Ziliang);

    General *heg_jiangwei = new General(this, "heg_jiangwei", "shu"); // SHU 012 G
    heg_jiangwei->addSkill("iktiaoxin");
    heg_jiangwei->addSkill(new Tianfu);

    General *heg_xusheng = new General(this, "heg_xusheng", "wu"); // WU 020
    heg_xusheng->addSkill(new Yicheng);

    General *heg_yuji = new General(this, "heg_yuji", "qun", 3); // QUN 011 G
    heg_yuji->addSkill(new Qianhuan);
}

ADD_PACKAGE(HFormation)
