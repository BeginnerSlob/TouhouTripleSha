#include "god.h"
#include "client.h"
#include "engine.h"
#include "maneuvering.h"
#include "general.h"
#include "settings.h"

class Wushen: public FilterSkill {
public:
    Wushen(): FilterSkill("wushen") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return to_select->getSuit() == Card::Heart && place == Player::PlaceHand;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class WushenTargetMod: public TargetModSkill {
public:
    WushenTargetMod(): TargetModSkill("#wushen-target") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("wushen") && card->getSuit() == Card::Heart)
            return 1000;
        else
            return 0;
    }
};

class Wuhun: public TriggerSkill {
public:
    Wuhun(): TriggerSkill("wuhun") {
        events << PreDamageDone;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.from && damage.from != player) {
            damage.from->gainMark("@nightmare", damage.damage);
            damage.from->getRoom()->broadcastSkillInvoke(objectName(), 1);
            room->notifySkillInvoked(player, objectName());
        }

        return false;
    }
};

class WuhunRevenge: public TriggerSkill {
public:
    WuhunRevenge(): TriggerSkill("#wuhun") {
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill("wuhun");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *shenguanyu, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != shenguanyu)
            return false;

        QList<ServerPlayer *> players = room->getOtherPlayers(shenguanyu);

        int max = 0;
        foreach (ServerPlayer *player, players)
            max = qMax(max, player->getMark("@nightmare"));
        if (max == 0) return false;

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

class IkZhuohuo: public TriggerSkill {
public:
    IkZhuohuo(): TriggerSkill("ikzhuohuo") {
        events << Damage << Damaged;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = triggerEvent == Damage ? "#IkZhuohuoDamage" : "#IkZhuohuoDamaged";
        log.from = player;
        log.arg = QString::number(damage.damage);
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());

        room->addPlayerMark(player, "@mailun", damage.damage);
        room->broadcastSkillInvoke(objectName(), triggerEvent == Damage ? 1 : 2);
        return false;
    }
};

class IkWumou: public TriggerSkill {
public:
    IkWumou(): TriggerSkill("ikwumou") {
        frequency = Compulsory;
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && use.card->isNDTrick())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());

        int num = player->getMark("@mailun");
        if (num >= 1 && room->askForChoice(player, objectName(), "discard+losehp") == "discard") {
            player->loseMark("@mailun");
        } else
            room->loseHp(player);

        return false;
    }
};

IkSuikongCard::IkSuikongCard() {
}

void IkSuikongCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    effect.from->loseMark("@mailun", 2);
    room->acquireSkill(effect.from, "ikwushuang");
    effect.from->setFlags("IkSuikongSource");
    effect.to->setFlags("IkSuikongTarget");
    room->addPlayerMark(effect.to, "Armor_Nullified");
}

class IkSuikongViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkSuikongViewAsSkill(): ZeroCardViewAsSkill("iksuikong") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@mailun") >= 2;
    }

    virtual const Card *viewAs() const{
        return new IkSuikongCard;
    }
};

class IkSuikong: public TriggerSkill {
public:
    IkSuikong(): TriggerSkill("iksuikong") {
        events << EventPhaseChanging << Death;
        view_as_skill = new IkSuikongViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player->hasFlag("IkSukongSource")) return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        }
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        }

        foreach (ServerPlayer *p , room->getAllPlayers()) {
            if (p->hasFlag("IkSukongTarget")) {
                p->setFlags("-IkSukongTarget");
                if (p->getMark("Armor_Nullified") > 0)
                    room->removePlayerMark(p, "Armor_Nullified");
            }
        }
        room->detachSkillFromPlayer(player, "ikwushuang", false, true);

        return QStringList();
    }
};

class IkTianwubaka: public ZeroCardViewAsSkill {
public:
    IkTianwubaka(): ZeroCardViewAsSkill("iktianwubaka") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@mailun") >= 6 && !player->hasUsed("IkTianwubakaCard");
    }

    virtual const Card *viewAs() const{
        return new IkTianwubakaCard;
    }
};

IkTianwubakaCard::IkTianwubakaCard() {
    target_fixed = true;
    mute = true;
}

void IkTianwubakaCard::use(Room *room, ServerPlayer *shenlvbu, QList<ServerPlayer *> &) const{
    room->broadcastSkillInvoke("iktianwubaka");
    shenlvbu->loseMark("@mailun", 6);

    QList<ServerPlayer *> players = room->getOtherPlayers(shenlvbu);
    foreach (ServerPlayer *player, players) {
        room->damage(DamageStruct("iktianwubaka", shenlvbu, player));
        room->getThread()->delay();
    }

    foreach (ServerPlayer *player, players) {
        QList<const Card *> equips = player->getEquips();
        player->throwAllEquips();
        if (!equips.isEmpty())
            room->getThread()->delay();
    }

    foreach (ServerPlayer *player, players) {
        bool delay = !player->isKongcheng();
        room->askForDiscard(player, "iktianwubaka", 4, 4);
        if (delay)
            room->getThread()->delay();
    }

    shenlvbu->turnOver();
}

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

    General *luna029 = new General(this, "luna029", "tsuki", 5);
    luna029->addSkill(new IkZhuohuo);
    luna029->addSkill(new MarkAssignSkill("@mailun", 2));
    luna029->addSkill(new IkWumou);
    luna029->addSkill(new IkSuikong);
    luna029->addSkill(new IkTianwubaka);
    related_skills.insertMulti("ikzhuohuo", "#@mailun-2");

    addMetaObject<IkTianwubakaCard>();
    addMetaObject<IkSuikongCard>();
}

ADD_PACKAGE(God)

