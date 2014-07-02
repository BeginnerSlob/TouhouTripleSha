#include "ikai-kin.h"

#include "general.h"
#include "skill.h"
#include "engine.h"

class IkLundao: public TriggerSkill {
public:
    IkLundao(): TriggerSkill("iklundao") {
        events << AskForRetrial;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        room->retrial(Sanguosha->getCard(room->drawCard()), player, judge, objectName());
        return false;
    }
};

class IkXuanwu: public PhaseChangeSkill {
public:
    IkXuanwu(): PhaseChangeSkill("ikxuanwu") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *wangyi) const{
        return PhaseChangeSkill::triggerable(wangyi)
            && wangyi->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *wangyi) const{
        Room *room = wangyi->getRoom();
        JudgeStruct judge;
        judge.pattern = ".|black";
        judge.good = true;
        judge.reason = objectName();
        judge.who = wangyi;

        room->judge(judge);

        if (judge.isGood() && wangyi->isAlive()) {
            QList<int> pile_ids = room->getNCards(wangyi->getLostHp() + 1, false);
            room->fillAG(pile_ids, wangyi);
            ServerPlayer *target = room->askForPlayerChosen(wangyi, room->getAllPlayers(), objectName());
            room->clearAG(wangyi);

            DummyCard *dummy = new DummyCard(pile_ids);
            wangyi->setFlags("Global_GongxinOperator");
            target->obtainCard(dummy, false);
            wangyi->setFlags("-Global_GongxinOperator");
            delete dummy;
        }

        return false;
    }
};

IkaiKinPackage::IkaiKinPackage()
    :Package("ikai-kin")
{
    General *bloom022 = new General(this, "bloom022", "hana", 3, false);
    bloom022->addSkill(new IkLundao);
    bloom022->addSkill(new IkXuanwu);
}

ADD_PACKAGE(IkaiKin)