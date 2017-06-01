#include "h-momentum.h"
#include "ai.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"
#include "serverplayer.h"
#include "settings.h"
#include "skill.h"
#include "standard-equips.h"
#include "standard.h"

class Hengzheng : public PhaseChangeSkill
{
public:
    Hengzheng()
        : PhaseChangeSkill("hengzheng")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *dongzhuo) const
    {
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
                        room->obtainCard(dongzhuo, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
                    }
                }

                dongzhuo->setFlags("-HengzhengUsing");
                return true;
            }
        }

        return false;
    }
};

class Baoling : public TriggerSkill
{
public:
    Baoling()
        : TriggerSkill("baoling")
    {
        events << EventPhaseEnd;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Play && target->getMark("baoling") == 0 && target->getMark("HengzhengUsed") >= 1;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
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

    General *heg_dongzhuo = new General(this, "heg_dongzhuo$", "qun", 4); // QUN 006 G
    heg_dongzhuo->addSkill(new Hengzheng);
    heg_dongzhuo->addSkill(new Baoling);
    heg_dongzhuo->addSkill("ikwuhua");
}

ADD_PACKAGE(HMomentum)
