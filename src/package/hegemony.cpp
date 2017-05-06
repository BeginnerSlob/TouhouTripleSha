#include "hegemony.h"
#include "skill.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "room.h"
#include "standard-skillcards.h"

class Huoshui: public TriggerSkill {
public:
    Huoshui(): TriggerSkill("huoshui") {
        events << EventPhaseStart << Death
               << EventLoseSkill << EventAcquireSkill
               << HpChanged << MaxHpChanged;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const{
        return 5;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (!TriggerSkill::triggerable(player)
                || (player->getPhase() != Player::RoundStart && player->getPhase() != Player::NotActive)) return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(objectName())) return false;
        } else if (triggerEvent == EventLoseSkill) {
            if (data.toString() != objectName() || player->getPhase() == Player::NotActive) return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName() || !player->hasSkill(objectName()) || player->getPhase() == Player::NotActive)
                return false;
        } else if (triggerEvent == MaxHpChanged || triggerEvent == HpChanged) {
            if (!room->getCurrent() || !room->getCurrent()->hasSkill(objectName())) return false;
        }

        foreach (ServerPlayer *p, room->getAllPlayers())
            room->filterCards(p, p->getCards("he"), true);
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        return false;
    }
};

class HuoshuiInvalidity: public InvaliditySkill {
public:
    HuoshuiInvalidity(): InvaliditySkill("#huoshui-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        if (player->getPhase() == Player::NotActive) {
            const Player *current = NULL;
            foreach (const Player *p, player->getAliveSiblings()) {
                if (p->getPhase() != Player::NotActive) {
                    current = p;
                    break;
                }
            }
            if (current && current->hasSkill("huoshui")
                && player->getHp() >= (player->getMaxHp() + 1) / 2 && !skill->isAttachedLordSkill())
                return false;
        }
        return true;
    }
};

QingchengCard::QingchengCard() {
    handling_method = Card::MethodDiscard;
}

void QingchengCard::onUse(Room *room, const CardUseStruct &use) const{
    CardUseStruct card_use = use;
    ServerPlayer *player = card_use.from, *to = card_use.to.first();

    LogMessage log;
    log.from = player;
    log.to = card_use.to;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString();
    room->sendLog(log);

    QStringList skill_list;
    foreach (const Skill *skill, to->getVisibleSkillList(false, true)) {
        if (!skill_list.contains(skill->objectName()) && !skill->isAttachedLordSkill()) {
            skill_list << skill->objectName();
        }
    }
    QString skill_qc;
    if (!skill_list.isEmpty()) {
        QVariant data_for_ai = QVariant::fromValue(to);
        skill_qc = room->askForChoice(player, "qingcheng", skill_list.join("+"), data_for_ai);
    }

    if (!skill_qc.isEmpty()) {
        LogMessage log;
        log.type = "$QingchengNullify";
        log.from = player;
        log.to << to;
        log.arg = skill_qc;
        room->sendLog(log);

        QStringList Qingchenglist = to->tag["Qingcheng"].toStringList();
        Qingchenglist << skill_qc;
        to->tag["Qingcheng"] = QVariant::fromValue(Qingchenglist);
        room->addPlayerMark(to, "Qingcheng" + skill_qc);

        foreach (ServerPlayer *p, room->getAllPlayers())
            room->filterCards(p, p->getCards("he"), true);

        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    }

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();
    thread->trigger(PreCardUsed, room, player, data);
    card_use = data.value<CardUseStruct>();

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), card_use.card->getSkillName(), QString());
    room->moveCardTo(this, player, NULL, Player::DiscardPile, reason, true);

    thread->trigger(CardUsed, room, card_use.from, data);
    card_use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, card_use.from, data);
}

bool QingchengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

class QingchengViewAsSkill: public OneCardViewAsSkill {
public:
    QingchengViewAsSkill(): OneCardViewAsSkill("qingcheng") {
        filter_pattern = "EquipCard!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        QingchengCard *first = new QingchengCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Qingcheng: public TriggerSkill {
public:
    Qingcheng(): TriggerSkill("qingcheng") {
        events << EventPhaseStart;
        view_as_skill = new QingchengViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const{
        return 6;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() == Player::RoundStart) {
            QStringList Qingchenglist = player->tag["Qingcheng"].toStringList();
            if (Qingchenglist.isEmpty()) return false;
            foreach (QString skill_name, Qingchenglist) {
                room->setPlayerMark(player, "Qingcheng" + skill_name, 0);
                if (player->hasSkill(skill_name)) {
                    LogMessage log;
                    log.type = "$QingchengReset";
                    log.from = player;
                    log.arg = skill_name;
                    room->sendLog(log);
                }
            }
            player->tag.remove("Qingcheng");
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), true);

            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return false;
    }
};

class QingchengInvalidity: public InvaliditySkill {
public:
    QingchengInvalidity(): InvaliditySkill("#qingcheng-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return player->getMark("Qingcheng" + skill->objectName()) == 0;
    }
};

HegemonyPackage::HegemonyPackage()
    : Package("hegemony")
{

    General *heg_luxun = new General(this, "heg_luxun", "wu", 3); // WU 007 G
    heg_luxun->addSkill("ikyuanhe");

    General *zoushi = new General(this, "zoushi", "qun", 3, false); // QUN 018
    zoushi->addSkill(new Huoshui);
    zoushi->addSkill(new HuoshuiInvalidity);
    zoushi->addSkill(new Qingcheng);
    zoushi->addSkill(new QingchengInvalidity);
    related_skills.insertMulti("huoshui", "#huoshui-inv");
    related_skills.insertMulti("qingcheng", "#qingcheng-inv");

    General *heg_caopi = new General(this, "heg_caopi$", "wei", 3, true, true); // WEI 014 G
    heg_caopi->addSkill("ikbisuo");
    heg_caopi->addSkill("iktanwan");
    heg_caopi->addSkill("iksongwei");

    General *heg_zhenji = new General(this, "heg_zhenji", "wei", 3, false, true); // WEI 007 G
    heg_zhenji->addSkill("ikzhongyan");
    heg_zhenji->addSkill("ikmengyang");

    General *heg_zhugeliang = new General(this, "heg_zhugeliang", "shu", 3, true, true); // SHU 004 G
    heg_zhugeliang->addSkill("ikxushi");
    heg_zhugeliang->addSkill("ikjingyou");

    General *heg_huangyueying = new General(this, "heg_huangyueying", "shu", 3, false, true); // SHU 007 G
    heg_huangyueying->addSkill("ikhuiquan");
    heg_huangyueying->addSkill("thjizhi");

    General *heg_zhouyu = new General(this, "heg_zhouyu", "wu", 3, true, true); // WU 005 G
    heg_zhouyu->addSkill("nosyingzi");
    heg_zhouyu->addSkill("nosfanjian");

    General *heg_xiaoqiao = new General(this, "heg_xiaoqiao", "wu", 3, false, true); // WU 011 G
    heg_xiaoqiao->addSkill("ikzhihui");
    heg_xiaoqiao->addSkill("ikchiqiu");

    General *heg_lvbu = new General(this, "heg_lvbu", "qun", 4, true, true); // QUN 002 G
    heg_lvbu->addSkill("ikwushuang");

    General *heg_diaochan = new General(this, "heg_diaochan", "qun", 3, false, true); // QUN 003 G
    heg_diaochan->addSkill("ikqingguo");
    heg_diaochan->addSkill("ikbiyue");

    addMetaObject<QingchengCard>();
}

ADD_PACKAGE(Hegemony)

