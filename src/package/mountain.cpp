#include "mountain.h"
#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "generaloverview.h"
#include "clientplayer.h"
#include "client.h"
#include "ai.h"
#include "jsonutils.h"

#include <QCommandLinkButton>

class IkHuiyao: public TriggerSkill {
public:
    IkHuiyao(): TriggerSkill("ikhuiyao") {
        events << Damaged << FinishJudge;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL || !damage.card->isKindOf("Slash") || damage.to->isDead())
                return skill_list;

            foreach (ServerPlayer *caiwenji, room->findPlayersBySkillName(objectName())) {
                if (caiwenji->canDiscard(caiwenji, "he"))
                    skill_list.insert(caiwenji, QStringList(objectName()));
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return skill_list;
            judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *caiwenji) const{
        if (room->askForCard(caiwenji, "..", "@ikhuiyao", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *caiwenji) const{
        DamageStruct damage = data.value<DamageStruct>();
        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.who = player;
        judge.reason = objectName();

        room->judge(judge);

        Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
        switch (suit) {
        case Card::Heart: {
                room->recover(player, RecoverStruct(caiwenji));

                break;
            }
        case Card::Diamond: {
                player->drawCards(2, objectName());
                break;
            }
        case Card::Club: {
                if (damage.from && damage.from->isAlive())
                    room->askForDiscard(damage.from, "ikhuiyao", 2, 2, false, true);

                break;
            }
        case Card::Spade: {
                if (damage.from && damage.from->isAlive())
                    damage.from->turnOver();

                break;
            }
        default:
                break;
        }

        return false;
    }
};

class IkQihuang: public TriggerSkill {
public:
    IkQihuang(): TriggerSkill("ikqihuang") {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;

        if (death.damage && death.damage->from) {
            LogMessage log;
            log.type = "#IkQihuangLoseSkills";
            log.from = player;
            log.to << death.damage->from;
            log.arg = objectName();
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            QList<const Skill *> skills = death.damage->from->getVisibleSkillList();
            QStringList detachList;
            foreach (const Skill *skill, skills) {
                if (!skill->inherits("SPConvertSkill") && !skill->isAttachedLordSkill())
                    detachList.append("-" + skill->objectName());
            }
            room->handleAcquireDetachSkills(death.damage->from, detachList);
            if (death.damage->from->isAlive())
                death.damage->from->gainMark("@qihuang");
        }

        return false;
    }
};

class IkHuanshen: public PhaseChangeSkill {
public:
    IkHuanshen(): PhaseChangeSkill("ikhuanshen") {
    }
    
    static void playAudioEffect(ServerPlayer *zuoci, const QString &skill_name) {
        zuoci->getRoom()->broadcastSkillInvoke(skill_name, zuoci->isMale(), -1);
    }

    static void AcquireGenerals(ServerPlayer *zuoci, int n) {
        Room *room = zuoci->getRoom();
        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        QStringList list = GetAvailableGenerals(zuoci);
        qShuffle(list);
        if (list.isEmpty()) return;
        n = qMin(n, list.length());

        QStringList acquired = list.mid(0, n);
        foreach (QString name, acquired) {
            huashens << name;
            const General *general = Sanguosha->getGeneral(name);
            if (general) {
                foreach (const TriggerSkill *skill, general->getTriggerSkills()) {
                    if (skill->isVisible())
                        room->getThread()->addTriggerSkill(skill);
                }
            }
        }
        zuoci->tag["IkHuanshens"] = huashens;

        QStringList hidden;
        for (int i = 0; i < n; i++) hidden << "unknown";
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p == zuoci)
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), acquired.join(":"), QList<ServerPlayer *>() << p);
            else
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), hidden.join(":"), QList<ServerPlayer *>() << p);
        }

        LogMessage log;
        log.type = "#GetIkHuanshen";
        log.from = zuoci;
        log.arg = QString::number(n);
        log.arg2 = QString::number(huashens.length());
        room->sendLog(log);

        LogMessage log2;
        log2.type = "#GetIkHuanshenDetail";
        log2.from = zuoci;
        log2.arg = acquired.join("\\, \\");
        room->sendLog(log2, zuoci);

        room->setPlayerMark(zuoci, "@huanshen", huashens.length());
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci) {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames().toSet();
        Room *room = zuoci->getRoom();
        if (isNormalGameMode(room->getMode())
            || room->getMode().contains("_mini_")
            || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        } else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/1v1", "").toStringList().toSet());
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }
        QSet<QString> huashen_set, room_set;
        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        foreach (QVariant huashen, huashens)
            huashen_set << huashen.toString();
        foreach (ServerPlayer *player, room->getAlivePlayers()) {
            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;

            if (!player->getGeneral2()) continue;

            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;
        }

        static QSet<QString> banned;
        if (banned.isEmpty())
            banned << "zuoci";

        return (all - banned - huashen_set - room_set).toList();
    }

    static void SelectSkill(ServerPlayer *zuoci) {
        Room *room = zuoci->getRoom();
        playAudioEffect(zuoci, "ikhuanshen");
        QStringList ac_dt_list;

        QString huashen_skill = zuoci->tag["IkHuanshenSkill"].toString();
        if (!huashen_skill.isEmpty())
            ac_dt_list.append("-" + huashen_skill);

        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        if (huashens.isEmpty()) return;

        QStringList huashen_generals;
        foreach (QVariant huashen, huashens)
            huashen_generals << huashen.toString();

        QStringList skill_names;
        QString skill_name;
        const General *general = NULL;
        AI* ai = zuoci->getAI();
        if (ai) {
            QHash<QString, const General *> hash;
            foreach (QString general_name, huashen_generals) {
                const General *general = Sanguosha->getGeneral(general_name);
                foreach (const Skill *skill, general->getVisibleSkillList()) {
                    if (skill->isLordSkill()
                        || skill->getFrequency() == Skill::Limited
                        || skill->getFrequency() == Skill::Wake)
                        continue;

                    if (!skill_names.contains(skill->objectName())) {
                        hash[skill->objectName()] = general;
                        skill_names << skill->objectName();
                    }
                }
            }
            if (skill_names.isEmpty()) return;
            skill_name = ai->askForChoice("ikhuanshen", skill_names.join("+"), QVariant());
            general = hash[skill_name];
            Q_ASSERT(general != NULL);
        } else {
            QString general_name = room->askForGeneral(zuoci, huashen_generals);
            general = Sanguosha->getGeneral(general_name);

            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill()
                    || skill->getFrequency() == Skill::Limited
                    || skill->getFrequency() == Skill::Wake)
                    continue;

                skill_names << skill->objectName();
            }

            if (!skill_names.isEmpty())
                skill_name = room->askForChoice(zuoci, "ikhuanshen", skill_names.join("+"));
        }
        //Q_ASSERT(!skill_name.isNull() && !skill_name.isEmpty());

        QString kingdom = general->getKingdom();
        if (zuoci->getKingdom() != kingdom) {
            if (kingdom == "kami")
                kingdom = room->askForKingdom(zuoci);
            room->setPlayerProperty(zuoci, "kingdom", kingdom);
        }

        if (zuoci->getGender() != general->getGender())
            zuoci->setGender(general->getGender());

        Json::Value arg(Json::arrayValue);
        arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        arg[1] = QSanProtocol::Utils::toJsonString(zuoci->objectName());
        arg[2] = QSanProtocol::Utils::toJsonString(general->objectName());
        arg[3] = QSanProtocol::Utils::toJsonString(skill_name);
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        zuoci->tag["IkHuanshenSkill"] = skill_name;
        if (!skill_name.isEmpty())
            ac_dt_list.append(skill_name);
        room->handleAcquireDetachSkills(zuoci, ac_dt_list, true);
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && (target->getPhase() == Player::RoundStart || target->getPhase() == Player::NotActive);
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *zuoci, QVariant &, ServerPlayer *) const{
        return zuoci->askForSkillInvoke(objectName());
    }

    virtual bool onPhaseChange(ServerPlayer *zuoci) const{
        SelectSkill(zuoci);
        return false;
    }

    virtual QDialog *getDialog() const{
        static IkHuanshenDialog *dialog;

        if (dialog == NULL)
            dialog = new IkHuanshenDialog;

        return dialog;
    }
};

class IkHuanshenStart: public GameStartSkill {
public:
    IkHuanshenStart(): GameStartSkill("#ikhuanshen-start") {
        frequency = Compulsory;
    }

    virtual void onGameStart(ServerPlayer *zuoci) const{
        zuoci->getRoom()->notifySkillInvoked(zuoci, "ikhuanshen");
        IkHuanshen::AcquireGenerals(zuoci, 2);
        IkHuanshen::SelectSkill(zuoci);
    }
};

IkHuanshenDialog::IkHuanshenDialog() {
    setWindowTitle(Sanguosha->translate("ikhuanshen"));
}

void IkHuanshenDialog::popup() {
    QVariantList huashen_list = Self->tag["IkHuanshens"].toList();
    QList<const General *> huashens;
    foreach (QVariant huashen, huashen_list)
        huashens << Sanguosha->getGeneral(huashen.toString());

    fillGenerals(huashens);
    show();
}

class IkHuanshenClear: public DetachEffectSkill {
public:
    IkHuanshenClear(): DetachEffectSkill("ikhuanshen") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        if (player->getKingdom() != player->getGeneral()->getKingdom() && player->getGeneral()->getKingdom() != "kami")
            room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
        if (player->getGender() != player->getGeneral()->getGender())
            player->setGender(player->getGeneral()->getGender());
        QString huashen_skill = player->tag["IkHuanshenSkill"].toString();
        if (!huashen_skill.isEmpty())
            room->detachSkillFromPlayer(player, huashen_skill, false, true);
        player->tag.remove("IkHuanshens");
        room->setPlayerMark(player, "@huanshen", 0);
    }
};

class IkLingqi: public MasochismSkill {
public:
    IkLingqi(): MasochismSkill("iklingqi") {
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *zuoci, const DamageStruct &damage) const{
        if (zuoci->askForSkillInvoke(objectName())) {
            IkHuanshen::playAudioEffect(zuoci, objectName());
            IkHuanshen::AcquireGenerals(zuoci, damage.damage);
        }
    }
};

MountainPackage::MountainPackage()
    : Package("mountain")
{

    General *luna009 = new General(this, "luna009", "tsuki", 3);
    luna009->addSkill(new IkHuanshen);
    luna009->addSkill(new IkHuanshenStart);
    luna009->addSkill(new IkHuanshenClear);
    luna009->addSkill(new IkLingqi);
    related_skills.insertMulti("ikhuanshen", "#ikhuanshen-start");
    related_skills.insertMulti("ikhuanshen", "#ikhuanshen-clear");

    General *luna012 = new General(this, "luna012", "tsuki", 3, false);
    luna012->addSkill(new IkHuiyao);
    luna012->addSkill(new IkQihuang);

}

ADD_PACKAGE(Mountain)

