#include "tenshi-reihou.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"

RhDuanlongCard::RhDuanlongCard()
{
    target_fixed = true;
}

void RhDuanlongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QStringList skills;
    QString old = source->tag["Reihou"].toString();
    if (Sanguosha->getGeneral(old)) {
        foreach (const Skill *skill, Sanguosha->getGeneral(old)->getVisibleSkillList())
            skills << "-" + skill->objectName();
        source->tag.remove("Reihou");
    }
    JsonArray args;
    args << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
    args << source->objectName();
    args << source->getGeneralName();
    args << QString();
    args << true;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    room->handleAcquireDetachSkills(source, skills, true);
}

class RhDuanlongVS : public ZeroCardViewAsSkill
{
public:
    RhDuanlongVS() : ZeroCardViewAsSkill("rhduanlong")
    {
    }

    virtual const Card *viewAs() const
    {
        return new RhDuanlongCard;
    }
};

class RhDuanlong : public TriggerSkill
{
public:
    RhDuanlong() : TriggerSkill("rhduanlong")
    {
        events << EventLoseSkill;
        view_as_skill = new RhDuanlongVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        if (data.toString() == "rhduanlong") {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (player->canDiscard(p, "e"))
                victims << p;
        }
        if (!victims.isEmpty()) {
            ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName(), "@rhduanlong", true, true);
            if (victim) {
                player->tag["RhDuanlongTarget"] = QVariant::fromValue(victim);
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *victim = player->tag["RhDuanlongTarget"].value<ServerPlayer *>();
        player->tag.remove("RhDuanlongTarget");
        if (victim) {
            room->setPlayerFlag(victim, "rhduanlong_InTempMoving");
            int first_id = room->askForCardChosen(player, victim, "e", "rhduanlong", false, Card::MethodDiscard);
            Player::Place original_place = room->getCardPlace(first_id);
            DummyCard *dummy = new DummyCard;
            dummy->addSubcard(first_id);
            victim->addToPile("#rhduanlong", dummy, false);
            if (player->canDiscard(victim, "e")) {
                int second_id = room->askForCardChosen(player, victim, "e", "rhduanlong", false, Card::MethodDiscard);
                dummy->addSubcard(second_id);
            }

            //move the first card back temporarily
            room->moveCardTo(Sanguosha->getCard(first_id), victim, original_place, false);
            room->setPlayerFlag(victim, "-rhduanlong_InTempMoving");
            room->throwCard(dummy, victim, player);
            delete dummy;
        }
        return false;
    }
};

class RhPohuang : public TriggerSkill
{
public:
    RhPohuang() : TriggerSkill("rhpohuang")
    {
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && player->canSlash(damage.from, false))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        DamageStruct damage = data.value<DamageStruct>();
        const Card *card = room->askForUseSlashTo(player, damage.from, "@rhpohuang:" + damage.from->objectName(), false);
        if (!card) {
            QStringList skills;
            QString old = player->tag["Reihou"].toString();
            if (Sanguosha->getGeneral(old)) {
                foreach (const Skill *skill, Sanguosha->getGeneral(old)->getVisibleSkillList())
                    skills << "-" + skill->objectName();
                player->tag.remove("Reihou");
            }
            JsonArray args;
            args << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            args << player->objectName();
            args << source->getGeneralName();
            args << QString();
            args << true;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            room->handleAcquireDetachSkills(player, skills, true);

            Card *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_rhpohuang");
            if (!player->canSlash(damage.from, slash, false)) {
                delete slash;
                return false;
            }
            room->useCard(CardUseStruct(slash, player, damage.from));
        }
        return false;
    }
};

class RhFangcun : public OneCardViewAsSkill
{
public:
    RhFangcun() : OneCardViewAsSkill("rhfangcun")
    {
        filter_pattern = "^TrickCard|black";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Dismantlement *dis = new Dismantlement(originalCard->getSuit(), originalCard->getNumber());
        dis->addSubcard(originalCard);
        dis->setSkillName(objectName());
        return dis;
    }
};

class RhHuifu : public ProhibitSkill
{
public:
    RhHuifu() : ProhibitSkill("rhhuifu")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(objectName()) && (card->isKindOf("Dismantlement") || card->isKindOf("Snatch"));
    }
};

TenshiReihouPackage::TenshiReihouPackage()
    :Package("tenshi-reihou")
{
    General *reihou001 = new General(this, "reihou001", "rei", 4, true, true);
    reihou001->addSkill(new RhDuanlong);
    reihou001->addSkill(new FakeMoveSkill("rhduanlong"));
    related_skills.insertMulti("rhduanlong", "#rhduanlong-fake-move");
    reihou001->addSkill(new RhPohuang);

    /*General *reihou005 = new General(this, "reihou005", "rei", 4, true, true);
    reihou005->addSkill(new RhFangcun);
    reihou005->addSkill(new RhHuifu);*/

    addMetaObject<RhDuanlongCard>();
}

ADD_PACKAGE(TenshiReihou)
