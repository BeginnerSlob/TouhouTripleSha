#include "h-momentum.h"
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
#include "jsonutils.h"

class Yingyang: public TriggerSkill {
public:
    Yingyang(): TriggerSkill("yingyang") {
        events << PindianVerifying;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (TriggerSkill::triggerable(pindian->from)) {
            QString choice = room->askForChoice(pindian->from, objectName(), "up+down+cancel", data);
            if (choice == "up") {
                pindian->from_number = qMin(pindian->from_number + 3, 13);
                doYingyangLog(room, pindian->from, choice, pindian->from_number);
                data = QVariant::fromValue(pindian);
            } else if (choice == "down") {
                pindian->from_number = qMax(pindian->from_number - 3, 1);
                doYingyangLog(room, pindian->from, choice, pindian->from_number);
                data = QVariant::fromValue(pindian);
            }
        }
        if (TriggerSkill::triggerable(pindian->to)) {
            QString choice = room->askForChoice(pindian->to, objectName(), "up+down+cancel", data);
            if (choice == "up") {
                pindian->to_number = qMin(pindian->to_number + 3, 13);
                doYingyangLog(room, pindian->to, choice, pindian->to_number);
                data = QVariant::fromValue(pindian);
            } else if (choice == "down") {
                pindian->to_number = qMax(pindian->to_number - 3, 1);
                doYingyangLog(room, pindian->to, choice, pindian->to_number);
                data = QVariant::fromValue(pindian);
            }
        }
        return false;
    }

private:
    QString getNumberString(int number) const{
        if (number == 10)
            return "10";
        else {
            static const char *number_string = "-A23456789-JQK";
            return QString(number_string[number]);
        }
    }

    void doYingyangLog(Room *room, ServerPlayer *player, const QString &choice, int number) const{
        room->notifySkillInvoked(player, objectName());
        if (choice == "up") {
            room->broadcastSkillInvoke(objectName(), 1);

            LogMessage log;
            log.type = "#YingyangUp";
            log.from = player;
            log.arg = getNumberString(number);
            room->sendLog(log);
        } else if (choice == "down") {
            room->broadcastSkillInvoke(objectName(), 2);

            LogMessage log;
            log.type = "#YingyangDown";
            log.from = player;
            log.arg = getNumberString(number);
            room->sendLog(log);
        }
    }
};

class Hengzheng: public PhaseChangeSkill {
public:
    Hengzheng(): PhaseChangeSkill("hengzheng") {
    }

    virtual bool onPhaseChange(ServerPlayer *dongzhuo) const{
        if (dongzhuo->getPhase() == Player::Draw && (dongzhuo->isKongcheng() || dongzhuo->getHp() <= 1)) {
            Room *room = dongzhuo->getRoom();
            if (room->askForSkillInvoke(dongzhuo, objectName())) {
                room->broadcastSkillInvoke(objectName());

                dongzhuo->setFlags("HengzhengUsing");
                if (dongzhuo->getMark("HengzhengUsed") == 0)
                    dongzhuo->setMark("HengzhengUsed", 1);
                QList<ServerPlayer *> players = room->getOtherPlayers(dongzhuo);
                if (players.length() >= 4)
                    room->doLightbox("$HengzhengAnimate");

                foreach (ServerPlayer *player, players) {
                    if (player->isAlive() && !player->isAllNude()) {
                        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, dongzhuo->objectName());
                        int card_id = room->askForCardChosen(dongzhuo, player, "hej", objectName());
                        room->obtainCard(dongzhuo, Sanguosha->getCard(card_id),
                                         reason, room->getCardPlace(card_id) != Player::PlaceHand);
                    }
                }

                dongzhuo->setFlags("-HengzhengUsing");
                return true;
            }
        }

        return false;
    }
};

class Baoling: public TriggerSkill {
public:
    Baoling(): TriggerSkill("baoling") {
        events << EventPhaseEnd;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
               && target->getPhase() == Player::Play
               && target->getMark("baoling") == 0
               && target->getMark("HengzhengUsed") >= 1;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#BaolingWake";
        log.from = player;
        log.arg = objectName();
        log.arg2 = "hengzheng";
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$BaolingAnimate");

        room->addPlayerMark(player, "baoling");

        room->setPlayerMark(player, "baoling", 1);
        if (room->changeMaxHpForAwakenSkill(player, 3)) {
            room->recover(player, RecoverStruct(player, NULL, 3));
            if (player->getMark("baoling") == 1)
                room->acquireSkill(player, "ikbenghuai");
        }

        return false;
    }
};

HMomentumPackage::HMomentumPackage()
    : Package("h_momentum")
{

    General *heg_madai = new General(this, "heg_madai", "shu", 4, true, true); // SHU 019
    heg_madai->addSkill("thjibu");
    heg_madai->addSkill("ikmoguang");

    General *heg_sunce = new General(this, "heg_sunce$", "wu", 4); // WU 010 G
    heg_sunce->addSkill("ikheyi");
    heg_sunce->addSkill(new Yingyang);
    heg_sunce->addSkill("ikbiansheng");

    General *heg_dongzhuo = new General(this, "heg_dongzhuo$", "qun", 4); // QUN 006 G
    heg_dongzhuo->addSkill(new Hengzheng);
    heg_dongzhuo->addSkill(new Baoling);
    heg_dongzhuo->addSkill("ikwuhua");
}

ADD_PACKAGE(HMomentum)
