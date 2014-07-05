#include "assassins.h"
#include "skill.h"
#include "standard.h"
#include "clientplayer.h"
#include "engine.h"

class Jieyuan: public TriggerSkill {
public:
    Jieyuan(): TriggerSkill("jieyuan") {
        events << DamageCaused << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused) {
            if (damage.to && damage.to->isAlive()
                && damage.to->getHp() >= player->getHp() && damage.to != player && player->canDiscard(player, "h")
                && room->askForCard(player, ".black", "@jieyuan-increase:" + damage.to->objectName(), data, objectName())) {
                room->broadcastSkillInvoke(objectName(), 1);

                LogMessage log;
                log.type = "#JieyuanIncrease";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(++damage.damage);
                room->sendLog(log);

                data = QVariant::fromValue(damage);
            }
        } else if (triggerEvent == DamageInflicted) {
            if (damage.from && damage.from->isAlive()
                && damage.from->getHp() >= player->getHp() && damage.from != player && player->canDiscard(player, "h")
                && room->askForCard(player, ".red", "@jieyuan-decrease:" + damage.from->objectName(), data, objectName())) {
                room->broadcastSkillInvoke(objectName(), 2);

                LogMessage log;
                log.type = "#JieyuanDecrease";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(--damage.damage);
                room->sendLog(log);

                data = QVariant::fromValue(damage);
                if (damage.damage < 1)
                    return true;
            }
        }

        return false;
    }
};

class Fenxin: public TriggerSkill {
public:
    Fenxin(): TriggerSkill("fenxin") {
        events << BeforeGameOverJudge;
        frequency = Limited;
        limit_mark = "@burnheart";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (!isNormalGameMode(room->getMode()))
            return false;
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage == NULL)
            return false;
        ServerPlayer *killer = death.damage->from;
        if (killer == NULL || killer->isLord() || player->isLord() || player->getHp() > 0)
            return false;
        if (!TriggerSkill::triggerable(killer) || killer->getMark("@burnheart") == 0)
            return false;
        player->setFlags("FenxinTarget");
        bool invoke = room->askForSkillInvoke(killer, objectName(), QVariant::fromValue(player));
        player->setFlags("-FenxinTarget");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            room->doLightbox("$FenxinAnimate");
            room->removePlayerMark(killer, "@burnheart");
            QString role1 = killer->getRole();
            killer->setRole(player->getRole());
            room->notifyProperty(killer, killer, "role", player->getRole());
            room->setPlayerProperty(player, "role", role1);
        }
        return false;
    }
};

AssassinsPackage::AssassinsPackage(): Package("assassins") {

    General *lingju = new General(this, "lingju", "qun", 3, false); // SP 017
    lingju->addSkill(new Jieyuan);
    lingju->addSkill(new Fenxin);

}

ADD_PACKAGE(Assassins)

