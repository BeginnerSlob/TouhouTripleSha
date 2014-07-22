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

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
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

class Chuanxin: public TriggerSkill {
public:
    Chuanxin(): TriggerSkill("chuanxin") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))
            && !damage.chain && !damage.transfer && damage.by_user
            && room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            QStringList choices;
            if (!damage.to->getEquips().isEmpty()) choices << "throw";
            QStringList skills_list;
            if (damage.to->getMark("chuanxin_" + player->objectName()) == 0) {
                foreach (const Skill *skill, damage.to->getVisibleSkillList()) {
                    if (!skill->isAttachedLordSkill())
                        skills_list << skill->objectName();
                }
                if (skills_list.length() > 1) choices << "detach";
            }
            if (choices.isEmpty()) return true;
            QString choice = room->askForChoice(damage.to, objectName(), choices.join("+"), data);
            if (choice == "throw") {
                damage.to->throwAllEquips();
                if (damage.to->isAlive())
                    room->loseHp(damage.to);
            } else {
                room->addPlayerMark(damage.to, "chuanxin_" + player->objectName());
                room->addPlayerMark(damage.to, "@chuanxin");
                QString lost_skill = room->askForChoice(damage.to, "chuanxin_lose", skills_list.join("+"), data);
                room->detachSkillFromPlayer(damage.to, lost_skill);
            }
            return true;
        }
        return false;
    }
};

class Fengshi: public TriggerSkill {
public:
    Fengshi(): TriggerSkill("fengshi") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from->isAlive()) {
            for (int i = 0; i < use.to.length(); i++) {
                ServerPlayer *to = use.to.at(i);
                if (to->isAlive() && to->isAdjacentTo(player) && to->isAdjacentTo(use.from)
                    && !to->getEquips().isEmpty()) {
                    to->setFlags("FengshiTarget"); // For AI
                    bool invoke = room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to));
                    to->setFlags("-FengshiTarget");
                    if (!invoke) continue;
                    room->broadcastSkillInvoke(objectName());
                    int id = -1;
                    if (to->getEquips().length() == 1)
                        id = to->getEquips().first()->getEffectiveId();
                    else
                        id = room->askForCardChosen(to, to, "e", objectName(), false, Card::MethodDiscard);
                    room->throwCard(id, to);
                }
            }
        }

        return false;
    }
};

HMomentumPackage::HMomentumPackage()
    : Package("h_momentum")
{

    General *heg_madai = new General(this, "heg_madai", "shu", 4, true, true); // SHU 019
    heg_madai->addSkill("thjibu");
    heg_madai->addSkill("ikqiansha");

    General *heg_sunce = new General(this, "heg_sunce$", "wu", 4); // WU 010 G
    heg_sunce->addSkill("ikheyi");
    heg_sunce->addSkill(new Yingyang);
    heg_sunce->addSkill("ikbiansheng");

    General *heg_dongzhuo = new General(this, "heg_dongzhuo$", "qun", 4); // QUN 006 G
    heg_dongzhuo->addSkill(new Hengzheng);
    heg_dongzhuo->addSkill(new Baoling);
    heg_dongzhuo->addSkill("ikwuhua");

    General *zhangren = new General(this, "zhangren", "qun", 4); // QUN 024
    zhangren->addSkill(new Chuanxin);
    zhangren->addSkill(new Fengshi);
}

ADD_PACKAGE(HMomentum)
