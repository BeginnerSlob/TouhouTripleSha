#include "chunxue-scenario.h"

#include "engine.h"
#include "room.h"
#include "skill.h"
#include "standard.h"

class CxLinli : public DrawCardsSkill
{
public:
    CxLinli()
        : DrawCardsSkill("cxlinli")
    {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#CxLinli";
        log.from = player;
        log.arg = "1";
        log.arg2 = "ChunXueWu";
        room->sendLog(log);
        return n - 1;
    }
};

class CxLinliLord : public InvaliditySkill
{
public:
    CxLinliLord()
        : InvaliditySkill("cxlinli_lord")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return player->getMark("ChunXueWu") == 0 || !(skill->objectName() == "thyoushang" || skill->objectName() == "thmanxiao");
    }
};

class CxQihuang : public TriggerSkill
{
public:
    CxQihuang()
        : TriggerSkill("cxqihuang")
    {
        events << CardUsed;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&ask_who) const
    {
        if (player->getGeneralName() == "hana002") {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && (use.card->isKindOf("Snatch") || use.card->isKindOf("ThJiewuCard"))) {
                foreach (ServerPlayer *p, use.to) {
                    if (TriggerSkill::triggerable(p)) {
                        ask_who = p;
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        room->doStory("$TouXinDao", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = ask_who;
        log.arg = "TouXinDao";
        room->sendLog(log);

        ask_who->drawCards(1, objectName());
        room->setPlayerProperty(ask_who, "role", "rebel");
        room->setTag("TouXinDao", true);
        return false;
    }
};

class CxXuqu : public TriggerSkill
{
public:
    CxXuqu()
        : TriggerSkill("cxxuqu")
    {
        events << TargetSpecified << Death;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&ask_who) const
    {
        if (triggerEvent == TargetSpecified) {
            if (player->getRole() == "rebel" || player->getGeneralName() == "yuki004") {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card && use.card->getTypeId() != Card::TypeSkill) {
                    foreach (ServerPlayer *to, use.to) {
                        if (to->hasSkill(objectName(), true)) {
                            QStringList froms = to->tag["CxXuquRecord"].toStringList();
                            if (!froms.contains(player->objectName())) {
                                froms << player->objectName();
                                to->tag["CxXuquRecord"] = QVariant::fromValue(froms);
                            }
                            if (froms.length() == 4 && to->getMark("mingyufei") == 0) {
                                ask_who = to;
                                return QStringList(objectName());
                            }
                        }
                    }
                }
            }
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player && player->isDead() && player->hasSkill(objectName()))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        room->addPlayerMark(ask_who, "mingyufei");
        room->doStory("$MingYuFei", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = ask_who;
        log.arg = "MingYuFei";
        room->sendLog(log);

        QList<ServerPlayer *> targets = room->getAllPlayers();
        targets.removeOne(room->getLord());
        targets.removeOne(room->findPlayer("yuki003", true));
        room->drawCards(targets, 1, objectName());

        room->setTag("MingYuFei", true);
        return false;
    }
};

CxQiuwenCard::CxQiuwenCard()
{
    target_fixed = true;
}

void CxQiuwenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->doStory("$YouMingFa", 4000);
    room->setPlayerFlag(source, "CxQiuwenUsed");
    room->detachSkillFromPlayer(source, "cxqiuwen", true);
    room->setTag("YouMingFa", true);
}

class CxQiuwen : public ZeroCardViewAsSkill
{
public:
    CxQiuwen()
        : ZeroCardViewAsSkill("cxqiuwen")
    {
        frequency = Limited;
    }

    virtual const Card *viewAs() const
    {
        return new CxQiuwenCard;
    }
};

class CxQiuwenTrigger : public TriggerSkill
{
public:
    CxQiuwenTrigger()
        : TriggerSkill("#cxqiuwen")
    {
        events << PreCardUsed << TargetSpecified;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified) {
            if (use.card->isKindOf("Slash") && player->hasFlag("CxQiuwenUsed"))
                return QStringList(objectName());
        } else if (triggerEvent == PreCardUsed) {
            if (use.card->isKindOf("Slash") && player->hasFlag("CxQiuwenUsed")) {
                if (use.m_addHistory)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to.toSet())
                p->addQinggangTag(use.card);
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            room->addPlayerHistory(player, use.card->getClassName(), -1);
            use.m_addHistory = false;
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class CxQiuwenTargetMod : public TargetModSkill
{
public:
    CxQiuwenTargetMod()
        : TargetModSkill("#cxqiuwen-tar")
    {
        frequency = Limited;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasFlag("CxQiuwenUsed"))
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasFlag("CxQiuwenUsed"))
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasFlag("CxQiuwenUsed"))
            return 1;
        else
            return 0;
    }
};

class CxLiujing : public TriggerSkill
{
public:
    CxLiujing()
        : TriggerSkill("cxliujing")
    {
        events << DamageInflicted;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->doStory("$KongGuanJian", 4000);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->getHandcardNum() < 4)
            player->drawCards(4 - player->getHandcardNum(), objectName());

        DamageStruct damage = data.value<DamageStruct>();
        damage.to = damage.from;
        damage.transfer = true;
        damage.transfer_reason = objectName();
        player->tag["TransferDamage"] = QVariant::fromValue(damage);

        if (damage.from->isAlive())
            damage.from->turnOver();

        room->setTag("KongGuanJian", true);

        return true;
    }
};

class CxWangwo : public PhaseChangeSkill
{
public:
    CxWangwo()
        : PhaseChangeSkill("cxwangwo")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->doStory("$MoRanYing", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "MoRanYing";
        room->sendLog(log);

        player->loseAllMarks("@bloom");
        room->acquireSkill(player, "thfuyue");
        room->setTag("MoRanYing", true);
        return false;
    }
};

class CxXianlin : public TriggerSkill
{
public:
    CxXianlin()
        : TriggerSkill("cxxianlin")
    {
        events << Dying;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who == player)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doStory("$XianHuYan", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "XianHuYan";
        room->sendLog(log);

        room->setPlayerProperty(player, "general", "yuki007");

        room->handleAcquireDetachSkills(player, "thchouce|thzhanshi");

        if (player->getHandcardNum() < 4)
            player->drawCards(4 - player->getHandcardNum(), objectName());

        room->loseMaxHp(player);
        room->recover(player, RecoverStruct(player, NULL, 3));

        if (player->isChained()) {
            player->setChained(true);
            room->setEmotion(player, "effects/iron_chain");
            room->broadcastProperty(player, "chained");
        }

        room->setTag("XianHuYan", true);

        return false;
    }
};

class CxYongjie : public TriggerSkill
{
public:
    CxYongjie()
        : TriggerSkill("cxyongjie")
    {
        events << Dying;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who == player)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doStory("$MiJinZhan", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "MiJinZhan";
        room->sendLog(log);

        room->recover(player, RecoverStruct(player, NULL, 2));

        room->acquireSkill(player, "ikshensha");

        room->setTag("MiJinZhan", true);

        return false;
    }
};

class CxChunzuiBase : public TriggerSkill
{
public:
    CxChunzuiBase(const QString &general_name, const QString &obtain_skill, const QString &wake)
        : TriggerSkill("cxchunzui_" + general_name)
        , general_name(general_name)
        , obtain_skill(obtain_skill)
        , wake(wake)
    {
        events << BuryVictim;
        frequency = Wake;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return -4;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *&ask_who) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->getRole() == "rebel") {
            ServerPlayer *player = room->findPlayerBySkillName(objectName());
            if (player && player->getRole() == "rebel") {
                ask_who = player;
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        room->doLightbox("$ZaiMiGui_" + general_name, 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "ZaiMiGui";
        room->sendLog(log);

        LogMessage log2;
        log2.type = "#GainMaxHp";
        log2.from = player;
        log2.arg = "1";
        room->sendLog(log2);

        room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
        room->recover(player, RecoverStruct(player, NULL, 1));

        room->acquireSkill(player, obtain_skill);

        if (!wake.isEmpty() && player->getMark("@" + wake) == 0) {
            const TriggerSkill *skill = Sanguosha->getTriggerSkill("th" + wake);
            if (skill)
                skill->effect(NonTrigger, room, player, data, player);
        }

        room->detachSkillFromPlayer(player, objectName(), true);

        return false;
    }

private:
    QString general_name;
    QString obtain_skill;
    QString wake;
};

class CxJiushi : public TriggerSkill
{
public:
    CxJiushi()
        : TriggerSkill("cxjiushi")
    {
        events << BuryVictim;
        frequency = Wake;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return -4;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&ask_who) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->getGeneralName() == "yuki007") {
            if (death.damage && TriggerSkill::triggerable(death.damage->from)) {
                ask_who = death.damage->from;
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        room->doStory("$TianCeYun", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "TianCeYun";
        room->sendLog(log);

        room->setPlayerProperty(player, "general", "sp011");

        room->handleAcquireDetachSkills(player, "thjiuzhang|thshushu|thfengling");

        if (player->getHandcardNum() < 4)
            player->drawCards(4 - player->getHandcardNum(), objectName());

        room->recover(player, RecoverStruct(player, NULL, 3));

        if (player->isChained()) {
            player->setChained(true);
            room->setEmotion(player, "effects/iron_chain");
            room->broadcastProperty(player, "chained");
        }

        room->detachSkillFromPlayer(player, "cxjiushi", true);

        return false;
    }
};

class CxWangdie : public TriggerSkill
{
public:
    CxWangdie()
        : TriggerSkill("cxwangdie")
    {
        events << EventMarksGot;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@bloom") == 1;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doStory("$SanFenXiao", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "SanFenXiao";
        room->sendLog(log);

        room->acquireSkill(player, "thlingdie");

        room->detachSkillFromPlayer(player, "cxwangdie", true);
        room->acquireSkill(player, "cxkongnie");

        return false;
    }
};

class CxKongnie : public TriggerSkill
{
public:
    CxKongnie()
        : TriggerSkill("cxkongnie")
    {
        events << EventMarksGot;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@bloom") == 2;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doStory("$WuFenXiao", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "WuFenXiao";
        room->sendLog(log);

        room->acquireSkill(player, "thwushou");

        room->detachSkillFromPlayer(player, "cxkongnie", true);
        room->acquireSkill(player, "cxhuaxu");

        return false;
    }
};

class CxHuaxu : public TriggerSkill
{
public:
    CxHuaxu()
        : TriggerSkill("cxhuaxu")
    {
        events << EventMarksGot;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@bloom") == 3;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doStory("$BaFenXiao", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "BaFenXiao";
        room->sendLog(log);

        QList<ServerPlayer *> lists;
        lists << player;
        ServerPlayer *yaomeng = NULL;
        yaomeng = room->findPlayer("yuki003");
        if (yaomeng)
            lists << yaomeng;
        room->drawCards(lists, 3, objectName());

        QList<ServerPlayer *> victims = room->getOtherPlayers(player);
        if (yaomeng)
            victims.removeOne(yaomeng);
        foreach (ServerPlayer *p, victims) {
            int card_id = room->getCardFromPile("@cxhuaxu");
            if (card_id == -1)
                break;

            const Card *originalCard = Sanguosha->getCard(card_id);
            Indulgence *indulgence = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
            indulgence->setSkillName("BaFenXiao");
            WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
            card->takeOver(indulgence);
            room->broadcastUpdateCard(room->getPlayers(), card->getId(), card);
            room->moveCardTo(card, p, Player::PlaceDelayedTrick, true);
            indulgence->deleteLater();
        }

        room->detachSkillFromPlayer(player, "cxhuaxu", true);
        room->acquireSkill(player, "cxzhongyan");

        return false;
    }
};

class CxZhongyan : public TriggerSkill
{
public:
    CxZhongyan()
        : TriggerSkill("cxzhongyan")
    {
        events << GameOverJudge;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player->isLord() && player->isDead()) {
            int n = 0;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getRole() == "rebel")
                    ++n;
                else
                    return QStringList();
            }
            if (n >= 3)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doStory("$ShenMengYin", 4000);

        LogMessage log;
        log.type = "#CxWake";
        log.from = player;
        log.arg = "ShenMengYin";
        room->sendLog(log);

        room->revivePlayer(player);

        room->setPlayerProperty(player, "general", "yuki009");
        room->setPlayerProperty(player, "gender", (int)player->getGeneral()->getGender());

        QList<const Skill *> skills = player->getVisibleSkillList();
        QStringList detachList;
        foreach (const Skill *skill, skills) {
            if (!skill->inherits("SPConvertSkill") && !skill->isAttachedLordSkill())
                detachList.append("-" + skill->objectName());
        }
        room->handleAcquireDetachSkills(player, detachList);

        room->handleAcquireDetachSkills(player, "thlingya|thheimu|thxijing|thhongdao");

        if (player->getHandcardNum() < 4)
            player->drawCards(4 - player->getHandcardNum(), objectName());

        room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
        room->recover(player, RecoverStruct(player, NULL, 3));

        if (player->isChained()) {
            player->setChained(true);
            room->setEmotion(player, "effects/iron_chain");
            room->broadcastProperty(player, "chained");
        }

        return true;
    }
};

class ChunxueRule : public ScenarioRule
{
public:
    ChunxueRule(Scenario *scenario)
        : ScenarioRule(scenario)
    {
        events << GameStart << BuryVictim;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == BuryVictim)
            return -5;
        return ScenarioRule::getPriority(triggerEvent);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        switch (triggerEvent) {
        case GameStart: {
            if (player == NULL) {
                room->doStory("$ChunXueWu", 4000);

                ServerPlayer *lord = room->getLord();
                room->installEquip(lord, "moon_spear");
                room->acquireSkill(lord, "cxlinli_lord");
                room->setPlayerMark(lord, "ChunXueWu", 1);

                ServerPlayer *yaomeng = room->findPlayer("yuki003");
                room->installEquip(yaomeng, "qinggang_sword");

                ServerPlayer *chen = room->findPlayer("yuki006");
                room->acquireSkill(chen, "cxxuqu");

                ServerPlayer *lingmeng = room->findPlayer("yuki001");
                room->installEquip(lingmeng, "chanayakongxue");
                room->acquireSkill(lingmeng, "cxlinli");

                ServerPlayer *molisha = room->findPlayer("hana002");
                room->installEquip(molisha, "fan");
                room->acquireSkill(molisha, "cxlinli");

                ServerPlayer *xiaoye = room->findPlayer("tsuki008");
                room->acquireSkill(xiaoye, "cxlinli");

                ServerPlayer *ailisi = room->findPlayer("yuki004");
                room->installEquip(ailisi, "vine");
                room->acquireSkill(ailisi, "cxlinli");
                room->acquireSkill(ailisi, "cxqihuang");
            }

            break;
        }
        case BuryVictim: {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->getGeneralName() == "yuki003") {
                if (!room->getTag("YouMingFa").toBool()) {
                    ServerPlayer *lord = room->getLord();
                    if (lord)
                        room->acquireSkill(lord, "cxwangwo");
                }
            }

            break;
        }
        default:
            break;
        }

        return false;
    }
};

ChunxueScenario::ChunxueScenario()
    : Scenario("chunxue")
{
    lord = "kami007"; // kami007->yuki009
    loyalists << "yuki003"
              << "yuki006"; // yuki006->yuki007
    rebels << "yuki001"
           << "hana002"
           << "tsuki008";
    renegades << "yuki004"
              << "yuki010"; // yuki010->sp011

    rule = new ChunxueRule(this);

    skills << new CxLinli << new CxLinliLord << new CxQihuang << new CxXuqu << new CxQiuwen << new CxQiuwenTrigger << new CxQiuwenTargetMod << new CxLiujing << new CxWangwo
           << new CxXianlin << new CxYongjie << new CxChunzuiBase("hana002", "thhuaji", "genxing") << new CxChunzuiBase("tsuki008", "thshennao", QString())
           << new CxChunzuiBase("yuki004", "thhuilun", QString()) << new CxChunzuiBase("yuki001", "thmengsheng", "erchong") << new CxJiushi << new CxWangdie << new CxKongnie
           << new CxHuaxu << new CxZhongyan;
    related_skills.insertMulti("cxqiuwen", "#cxqiuwen");
    related_skills.insertMulti("cxqiuwen", "#cxqiuwen-tar");

    addMetaObject<CxQiuwenCard>();
}

void ChunxueScenario::onTagSet(Room *room, const QString &key) const
{
    if (key == "TouXinDao") {
        ServerPlayer *ailisi = room->findPlayer("yuki004");
        if (ailisi)
            room->detachSkillFromPlayer(ailisi, "cxqihuang", true);
    } else if (key == "MingYuFei") {
        ServerPlayer *yaomeng = room->findPlayer("yuki003");
        if (yaomeng) {
            room->acquireSkill(yaomeng, "cxqiuwen");
            room->acquireSkill(yaomeng, "cxliujing");
        }

        ServerPlayer *chen = room->findPlayer("yuki006");
        if (chen)
            room->detachSkillFromPlayer(chen, "cxxuqu", true);

        ServerPlayer *lingmeng = room->findPlayer("yuki001");
        if (lingmeng)
            room->detachSkillFromPlayer(lingmeng, "cxlinli", true);

        ServerPlayer *molisha = room->findPlayer("hana002");
        if (molisha)
            room->detachSkillFromPlayer(molisha, "cxlinli", true);

        ServerPlayer *xiaoye = room->findPlayer("tsuki008");
        if (xiaoye)
            room->detachSkillFromPlayer(xiaoye, "cxlinli", true);

        ServerPlayer *ailisi = room->findPlayer("yuki004");
        if (ailisi)
            room->detachSkillFromPlayer(ailisi, "cxlinli", true);
    } else if (key == "YouMingFa") {
        ServerPlayer *lord = room->getLord();
        if (lord)
            room->acquireSkill(lord, "cxwangwo");
    } else if (key == "KongGuanJian") {
        ServerPlayer *yaomeng = room->findPlayer("yuki003");
        if (yaomeng)
            room->detachSkillFromPlayer(yaomeng, "cxliujing", true);
    } else if (key == "MoRanYing") {
        ServerPlayer *lord = room->getLord();
        if (lord) {
            room->detachSkillFromPlayer(lord, "cxlinli_lord", true);
            room->setPlayerMark(lord, "ChunXueWu", 0);
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            room->detachSkillFromPlayer(lord, "cxwangwo", true);

            room->acquireSkill(lord, "cxwangdie");
        }

        ServerPlayer *chen = room->findPlayer("yuki006");
        if (chen)
            room->acquireSkill(chen, "cxxianlin");

        ServerPlayer *yaomeng = room->findPlayer("yuki003");
        if (yaomeng)
            room->acquireSkill(yaomeng, "cxyongjie");

        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getRole() == "rebel" || p->getGeneralName() == "yuki004")
                room->acquireSkill(p, "cxchunzui_" + p->getGeneralName());
        }
    } else if (key == "XianHuYan") {
        ServerPlayer *lan = room->findPlayer("yuki007");
        if (lan)
            room->detachSkillFromPlayer(lan, "cxxianlin", true);

        ServerPlayer *baka = room->findPlayer("yuki010");
        if (baka)
            room->acquireSkill(baka, "cxjiushi");
    } else if (key == "MiJinZhan") {
        ServerPlayer *yaomeng = room->findPlayer("yuki003");
        if (yaomeng)
            room->detachSkillFromPlayer(yaomeng, "cxyongjie", true);
    }
}
