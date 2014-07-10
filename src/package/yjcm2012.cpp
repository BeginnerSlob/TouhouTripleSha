#include "yjcm2012.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"

class Zishou: public DrawCardsSkill {
public:
    Zishou(): DrawCardsSkill("zishou") {
    }

    virtual int getDrawNum(ServerPlayer *liubiao, int n) const{
        Room *room = liubiao->getRoom();
        if (liubiao->isWounded() && room->askForSkillInvoke(liubiao, objectName())) {
            int losthp = liubiao->getLostHp();
            room->broadcastSkillInvoke(objectName(), qMin(3, losthp));
            liubiao->clearHistory();
            liubiao->skip(Player::Play);
            return n + losthp;
        } else
            return n;
    }
};

class Zongshi: public MaxCardsSkill {
public:
    Zongshi(): MaxCardsSkill("zongshi") {
    }

    virtual int getExtra(const Player *target) const{
        int extra = 0;
        QSet<QString> kingdom_set;
        if (target->parent()) {
            foreach (const Player *player, target->parent()->findChildren<const Player *>()) {
                if (player->isAlive())
                    kingdom_set << player->getKingdom();
            }
        }
        extra = kingdom_set.size();
        if (target->hasSkill(objectName()))
            return extra;
        else
            return 0;
    }
};

class Shiyong: public TriggerSkill {
public:
    Shiyong(): TriggerSkill("shiyong") {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash")
            && (damage.card->isRed() || damage.card->hasFlag("drank"))) {
            int index = 1;
            if (damage.from->getGeneralName().contains("guanyu"))
                index = 3;
            else if (damage.card->hasFlag("drank"))
                index = 2;
            room->broadcastSkillInvoke(objectName(), index);

            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            room->loseMaxHp(player);
        }
        return false;
    }
};

YJCM2012Package::YJCM2012Package()
    : Package("YJCM2012")
{

    General *huaxiong = new General(this, "huaxiong", "qun", 6); // YJ 106
    huaxiong->addSkill(new Shiyong);

    General *liubiao = new General(this, "liubiao", "qun", 4); // YJ 108
    liubiao->addSkill(new Zishou);
    liubiao->addSkill(new Zongshi);

}

ADD_PACKAGE(YJCM2012)

