#include "special3v3.h"
#include "ai.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"
#include "server.h"
#include "skill.h"
#include "standard.h"

class VsGanglie : public MasochismSkill
{
public:
    VsGanglie()
        : MasochismSkill("vsganglie")
    {
    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &) const
    {
        Room *room = xiahou->getRoom();
        QString mode = room->getMode();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(xiahou)) {
            if ((!mode.startsWith("06_") && !mode.startsWith("04_")) || AI::GetRelation3v3(xiahou, p) == AI::Enemy)
                targets << p;
        }

        ServerPlayer *from = room->askForPlayerChosen(xiahou, targets, objectName(), "vsganglie-invoke", true, true);
        if (!from)
            return;

        room->broadcastSkillInvoke("ikaoli");

        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = objectName();
        judge.who = xiahou;

        room->judge(judge);
        if (from->isDead())
            return;
        if (judge.isGood()) {
            if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
                room->damage(DamageStruct(objectName(), xiahou, from));
        }
    }
};

ZhongyiCard::ZhongyiCard()
{
    mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ZhongyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->broadcastSkillInvoke("zhongyi");
    room->doLightbox("$ZhongyiAnimate");
    room->removePlayerMark(source, "@loyal");
    source->addToPile("loyal", this);
}

class Zhongyi : public OneCardViewAsSkill
{
public:
    Zhongyi()
        : OneCardViewAsSkill("zhongyi")
    {
        frequency = Limited;
        limit_mark = "@loyal";
        filter_pattern = ".|red|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && player->getMark("@loyal") > 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ZhongyiCard *card = new ZhongyiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ZhongyiAction : public TriggerSkill
{
public:
    ZhongyiAction()
        : TriggerSkill("#zhongyi-action")
    {
        events << DamageCaused << EventPhaseStart << ActionedReset;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QString mode = room->getMode();
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer || !damage.by_user)
                return false;
            if (damage.card && damage.card->isKindOf("Slash")) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getPile("loyal").isEmpty())
                        continue;
                    bool on_effect = false;
                    if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_"))
                        on_effect = (AI::GetRelation3v3(player, p) == AI::Friend);
                    else
                        on_effect = (room->askForSkillInvoke(p, "zhongyi", data));
                    if (on_effect) {
                        LogMessage log;
                        log.type = "#ZhongyiBuff";
                        log.from = p;
                        log.to << damage.to;
                        log.arg = QString::number(damage.damage);
                        log.arg2 = QString::number(++damage.damage);
                        room->sendLog(log);
                    }
                }
            }
            data = QVariant::fromValue(damage);
        } else if ((mode == "06_3v3" && triggerEvent == ActionedReset) || (mode != "06_3v3" && triggerEvent == EventPhaseStart)) {
            if (triggerEvent == EventPhaseStart && player->getPhase() != Player::RoundStart)
                return false;
            if (player->getPile("loyal").length() > 0)
                player->clearOnePrivatePile("loyal");
        }
        return false;
    }
};

JiuzhuCard::JiuzhuCard()
{
    target_fixed = true;
}

void JiuzhuCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const
{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who)
        return;

    room->loseHp(player);
    room->recover(who, RecoverStruct(player));
}

class Jiuzhu : public OneCardViewAsSkill
{
public:
    Jiuzhu()
        : OneCardViewAsSkill("jiuzhu")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (pattern != "peach" || !player->canDiscard(player, "he") || player->getHp() <= 1)
            return false;
        QString dyingobj = player->property("currentdying").toString();
        const Player *who = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->objectName() == dyingobj) {
                who = p;
                break;
            }
        }
        if (!who)
            return false;
        if (ServerInfo.GameMode.startsWith("06_"))
            return player->getRole().at(0) == who->getRole().at(0);
        else
            return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        JiuzhuCard *card = new JiuzhuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Zhanshen : public TriggerSkill
{
public:
    Zhanshen()
        : TriggerSkill("zhanshen")
    {
        events << Death << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
            foreach (ServerPlayer *lvbu, room->getAllPlayers()) {
                if (!TriggerSkill::triggerable(lvbu))
                    continue;
                if (room->getMode().startsWith("06_") || room->getMode().startsWith("04_")) {
                    if (lvbu->getMark(objectName()) == 0 && lvbu->getMark("zhanshen_fight") == 0 && AI::GetRelation3v3(lvbu, player) == AI::Friend)
                        lvbu->addMark("zhanshen_fight");
                } else {
                    if (lvbu->getMark(objectName()) == 0 && lvbu->getMark("@fight") == 0 && room->askForSkillInvoke(player, objectName(), "mark:" + lvbu->objectName()))
                        room->addPlayerMark(lvbu, "@fight");
                }
            }
        } else if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Start && player->getMark(objectName()) == 0 && player->isWounded()
                   && (player->getMark("zhanshen_fight") > 0 || player->getMark("@fight") > 0)) {
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#ZhanshenWake";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke(objectName());
            room->doLightbox("$ZhanshenAnimate");

            if (player->getMark("@fight") > 0)
                room->setPlayerMark(player, "@fight", 0);
            player->setMark("zhanshen_fight", 0);

            room->setPlayerMark(player, "zhanshen", 1);
            if (room->changeMaxHpForAwakenSkill(player)) {
                if (player->getWeapon())
                    room->throwCard(player->getWeapon(), player);
                if (player->getMark("zhanshen") == 1)
                    room->handleAcquireDetachSkills(player, "thjibu|shenji");
            }
        }
        return false;
    }
};

VSCrossbow::VSCrossbow(Suit suit, int number)
    : Crossbow(suit, number)
{
    setObjectName("vscrossbow");
}

bool VSCrossbow::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("crossbow"))
        return true;
    else
        return Crossbow::match(pattern);
}

New3v3CardPackage::New3v3CardPackage()
    : Package("New3v3Card")
{
    QList<Card *> cards;
    cards << new SupplyShortage(Card::Spade, 1) << new SupplyShortage(Card::Club, 12) << new Nullification(Card::Heart, 12);

    foreach (Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(New3v3Card)

New3v3_2013CardPackage::New3v3_2013CardPackage()
    : Package("New3v3_2013Card")
{
    QList<Card *> cards;
    cards << new VSCrossbow(Card::Club) << new VSCrossbow(Card::Diamond);

    foreach (Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(New3v3_2013Card)

Special3v3Package::Special3v3Package()
    : Package("Special3v3")
{
    General *vs_nos_xiahoudun = new General(this, "vs_nos_xiahoudun", "wei");
    vs_nos_xiahoudun->addSkill(new VsGanglie);

    General *vs_nos_guanyu = new General(this, "vs_nos_guanyu", "shu");
    vs_nos_guanyu->addSkill("ikchilian");
    vs_nos_guanyu->addSkill(new Zhongyi);
    vs_nos_guanyu->addSkill(new ZhongyiAction);
    related_skills.insertMulti("zhongyi", "#zhongyi-action");

    General *vs_nos_zhaoyun = new General(this, "vs_nos_zhaoyun", "shu");
    vs_nos_zhaoyun->addSkill("ikhuahuan");
    vs_nos_zhaoyun->addSkill(new Jiuzhu);

    General *vs_nos_lvbu = new General(this, "vs_nos_lvbu", "qun");
    vs_nos_lvbu->addSkill("ikwushuang");
    vs_nos_lvbu->addSkill(new Zhanshen);

    addMetaObject<ZhongyiCard>();
    addMetaObject<JiuzhuCard>();
}

ADD_PACKAGE(Special3v3)
