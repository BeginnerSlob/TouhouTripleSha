#include "thicket.h"
#include "general.h"
#include "skill.h"
#include "room.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"

class IkSishideng: public TriggerSkill {
public:
    IkSishideng(): TriggerSkill("iksishideng") {
        // just to broadcast audio effects and to send log messages
        // main part in the AskForPeaches trigger of Game Rule
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player && player == room->getAllPlayers().first()) {
            DyingStruct dying = data.value<DyingStruct>();
            ServerPlayer *jiaxu = room->getCurrent();
            if (jiaxu && TriggerSkill::triggerable(jiaxu) && jiaxu->getPhase() != Player::NotActive)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual int getPriority(TriggerEvent) const{
        return 7;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *jiaxu = room->getCurrent();
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(jiaxu, objectName());

        LogMessage log;
        log.from = jiaxu;
        log.arg = objectName();
        if (jiaxu != dying.who) {
            log.type = "#IkSishidengTwo";
            log.to << dying.who;
        } else {
            log.type = "#IkSishidengOne";
        }
        room->sendLog(log);
        
        return false;
    }
};

class IkWenle: public ZeroCardViewAsSkill {
public:
    IkWenle(): ZeroCardViewAsSkill("ikwenle") {
        frequency = Limited;
        limit_mark = "@wenle";
    }

    virtual const Card *viewAs() const{
        return new IkWenleCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@wenle") >= 1;
    }
};

IkWenleCard::IkWenleCard() {
    target_fixed = true;
}

void IkWenleCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@wenle");
    room->addPlayerMark(source, "@wenleused");

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach (ServerPlayer *player, players) {
        if (player->isAlive()) {
            room->cardEffect(this, source, player);
            room->getThread()->delay();
        }
    }
}

void IkWenleCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    QList<ServerPlayer *> players = room->getOtherPlayers(effect.to);
    QList<int> distance_list;
    int nearest = 1000;
    foreach (ServerPlayer *player, players) {
        int distance = effect.to->distanceTo(player);
        distance_list << distance;
        nearest = qMin(nearest, distance);
    }

    QList<ServerPlayer *> ikwenle_targets;
    for (int i = 0; i < distance_list.length(); i++) {
        if (distance_list[i] == nearest && effect.to->canSlash(players[i], NULL, false))
            ikwenle_targets << players[i];
    }

    if (ikwenle_targets.isEmpty() || !room->askForUseSlashTo(effect.to, ikwenle_targets, "@ikwenle-slash"))
        room->loseHp(effect.to);
}

class IkMoyudeng: public ProhibitSkill {
public:
    IkMoyudeng(): ProhibitSkill("ikmoyudeng") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("TrickCard") || card->isKindOf("QiceCard"))
               && card->isBlack() && card->getSkillName() != "ikguihuo"; // Be care!!!!!!
    }
};

class IkFusheng: public OneCardViewAsSkill {
public:
    IkFusheng(): OneCardViewAsSkill("ikfusheng") {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return  pattern.contains("analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());
        return analeptic;
    }
};

class IkHuanbei: public TriggerSkill {
public:
    IkHuanbei(): TriggerSkill("ikhuanbei") {
        events << TargetConfirmed << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            if (triggerEvent == TargetSpecified) {
                foreach (ServerPlayer *p, use.to)
                    if (p->isFemale())
                        return QStringList(objectName());
            } else if (triggerEvent == TargetConfirmed && use.from->isFemale()) {
                if (use.to.contains(player))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        LogMessage log;
        log.from = player;
        log.arg = objectName();
        log.type = "#TriggerSkill";
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());

        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        if (triggerEvent == TargetSpecified) {
            foreach (ServerPlayer *p, use.to) {
                if (p->isFemale()) {
                    if (jink_list.at(index).toInt() == 1)
                        jink_list.replace(index, QVariant(2));
                }
                index++;
            }
            use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        } else if (triggerEvent == TargetConfirmed && use.from->isFemale()) {
            foreach (ServerPlayer *p, use.to) {
                if (p == player) {
                    if (jink_list.at(index).toInt() == 1)
                        jink_list.replace(index, QVariant(2));
                }
                index++;
            }
            use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        }

        return false;
    }
};

class IkBenghuai: public PhaseChangeSkill {
public:
    IkBenghuai(): PhaseChangeSkill("ikbenghuai") {
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish)
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (player->getHp() > p->getHp())
                    return QStringList(objectName());
        return QStringList();
    }

    virtual bool onPhaseChange(ServerPlayer *dongzhuo) const {
        Room *room = dongzhuo->getRoom();

        LogMessage log;
        log.from = dongzhuo;
        log.arg = objectName();
        log.type = "#TriggerSkill";
        room->sendLog(log);
        room->notifySkillInvoked(dongzhuo, objectName());

        QString result = room->askForChoice(dongzhuo, "ikbenghuai", "hp+maxhp");
        room->broadcastSkillInvoke(objectName());
        if (result == "hp")
            room->loseHp(dongzhuo);
        else
            room->loseMaxHp(dongzhuo);

        return false;
    }
};

class IkWuhua: public TriggerSkill {
public:
    IkWuhua(): TriggerSkill("ikwuhua$") {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player->tag.value("InvokeIkWuhua", false).toBool()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill(objectName()))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        QList<ServerPlayer *> dongzhuos;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasLordSkill(objectName()))
                dongzhuos << p;
        }

        while (!dongzhuos.isEmpty()) {
            ServerPlayer *dongzhuo = room->askForPlayerChosen(player, dongzhuos, objectName(), "@ikwuhua-to", true);
            if (dongzhuo) {
                dongzhuos.removeOne(dongzhuo);

                LogMessage log;
                log.type = "#InvokeOthersSkill";
                log.from = player;
                log.to << dongzhuo;
                log.arg = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(dongzhuo, objectName());

                JudgeStruct judge;
                judge.pattern = ".|spade";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                room->judge(judge);

                if (judge.isGood()) {
                    room->broadcastSkillInvoke(objectName());
                    room->recover(dongzhuo, RecoverStruct(player));
                }
            } else
                break;
        }

        return false;
    }
};

class IkWuhuaRecord: public TriggerSkill {
public:
    IkWuhuaRecord(): TriggerSkill("#ikwuhua-record") {
        events << PreDamageDone;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *qun = damage.from;
        if (qun)
            qun->tag["InvokeIkWuhua"] = qun->getKingdom() == "tsuki";
        return QStringList();
    }
};

ThicketPackage::ThicketPackage()
    : Package("thicket")
{

    General *luna001 = new General(this, "luna001$", "tsuki", 8);
    luna001->addSkill(new IkFusheng);
    luna001->addSkill(new IkHuanbei);
    luna001->addSkill(new IkBenghuai);
    luna001->addSkill(new IkWuhua);
    luna001->addSkill(new IkWuhuaRecord);
    related_skills.insertMulti("ikwuhua", "#ikwuhua-record");

    General *luna007 = new General(this, "luna007", "tsuki", 3);
    luna007->addSkill(new IkSishideng);
    luna007->addSkill(new IkWenle);
    luna007->addSkill(new IkMoyudeng);

    addMetaObject<IkWenleCard>();
}

ADD_PACKAGE(Thicket)

