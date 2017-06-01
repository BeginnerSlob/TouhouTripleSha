#include "god.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "settings.h"

class Wushen : public FilterSkill
{
public:
    Wushen()
        : FilterSkill("wushen")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return to_select->getSuit() == Card::Heart && place == Player::PlaceHand;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class WushenTargetMod : public TargetModSkill
{
public:
    WushenTargetMod()
        : TargetModSkill("#wushen-target")
    {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (from->hasSkill("wushen") && card->getSuit() == Card::Heart)
            return 1000;
        else
            return 0;
    }
};

class Wuhun : public TriggerSkill
{
public:
    Wuhun()
        : TriggerSkill("wuhun")
    {
        events << PreDamageDone;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.from && damage.from != player) {
            damage.from->gainMark("@nightmare", damage.damage);
            damage.from->getRoom()->broadcastSkillInvoke(objectName(), 1);
            room->notifySkillInvoked(player, objectName());
        }

        return false;
    }
};

class WuhunRevenge : public TriggerSkill
{
public:
    WuhunRevenge()
        : TriggerSkill("#wuhun")
    {
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill("wuhun");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *shenguanyu, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != shenguanyu)
            return false;

        QList<ServerPlayer *> players = room->getOtherPlayers(shenguanyu);

        int max = 0;
        foreach (ServerPlayer *player, players)
            max = qMax(max, player->getMark("@nightmare"));
        if (max == 0)
            return false;

        QList<ServerPlayer *> foes;
        foreach (ServerPlayer *player, players) {
            if (player->getMark("@nightmare") == max)
                foes << player;
        }

        if (foes.isEmpty())
            return false;

        ServerPlayer *foe;
        if (foes.length() == 1)
            foe = foes.first();
        else
            foe = room->askForPlayerChosen(shenguanyu, foes, "wuhun", "@wuhun-revenge");

        room->notifySkillInvoked(shenguanyu, "wuhun");

        JudgeStruct judge;
        judge.pattern = "Peach,GodSalvation";
        judge.good = true;
        judge.negative = true;
        judge.reason = "wuhun";
        judge.who = foe;

        room->judge(judge);

        if (judge.isBad()) {
            room->broadcastSkillInvoke("wuhun", 2);
            room->doLightbox("$WuhunAnimate", 3000);

            LogMessage log;
            log.type = "#WuhunRevenge";
            log.from = shenguanyu;
            log.to << foe;
            log.arg = QString::number(max);
            log.arg2 = "wuhun";
            room->sendLog(log);

            room->killPlayer(foe);
        } else
            room->broadcastSkillInvoke("wuhun", 3);
        QList<ServerPlayer *> killers = room->getAllPlayers();
        foreach (ServerPlayer *player, killers)
            player->loseAllMarks("@nightmare");

        return false;
    }
};

GodPackage::GodPackage()
    : Package("god")
{
    General *shenguanyu = new General(this, "shenguanyu", "god", 5); // LE 001
    shenguanyu->addSkill(new Wushen);
    shenguanyu->addSkill(new WushenTargetMod);
    shenguanyu->addSkill(new Wuhun);
    shenguanyu->addSkill(new WuhunRevenge);
    related_skills.insertMulti("wushen", "#wushen-target");
    related_skills.insertMulti("wuhun", "#wuhun");
}

ADD_PACKAGE(God)
