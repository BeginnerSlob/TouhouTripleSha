#include "tenshi-reihou.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "client.h"

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
            args << player->getGeneralName();
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

RhRuyiCard::RhRuyiCard()
{
    mute = true;
}

bool RhRuyiCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first());
            card->deleteLater();
            return card && card->targetFixed();
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("rhruyi").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->setSkillName("rhruyi");
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetFixed();
}

bool RhRuyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first());
            card->deleteLater();
            return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("rhruyi").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->setSkillName("rhruyi");
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool RhRuyiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first());
            card->deleteLater();
            return card && card->targetsFeasible(targets, Self);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("rhruyi").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->setSkillName("rhruyi");
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *RhRuyiCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *rhruyi_general = card_use.from;
    Room *room = rhruyi_general->getRoom();

    QStringList skills;
    QString old = rhruyi_general->tag["Reihou"].toString();
    if (Sanguosha->getGeneral(old)) {
        foreach (const Skill *skill, Sanguosha->getGeneral(old)->getVisibleSkillList())
            skills << "-" + skill->objectName();
        rhruyi_general->tag.remove("Reihou");
    }
    JsonArray args;
    args << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
    args << rhruyi_general->objectName();
    args << rhruyi_general->getGeneralName();
    args << QString();
    args << true;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    room->handleAcquireDetachSkills(rhruyi_general, skills, true);

    QString to_use = user_string;

    if (user_string == "slash"
        && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        QStringList use_list;
        use_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        to_use = room->askForChoice(rhruyi_general, "rhruyi_skill_slash", use_list.join("+"));
    }

    Card *use_card = Sanguosha->cloneCard(to_use);
    use_card->setSkillName("rhruyi");
    use_card->deleteLater();
    return use_card;
}

const Card *RhRuyiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    QStringList skills;
    QString old = user->tag["Reihou"].toString();
    if (Sanguosha->getGeneral(old)) {
        foreach (const Skill *skill, Sanguosha->getGeneral(old)->getVisibleSkillList())
            skills << "-" + skill->objectName();
        user->tag.remove("Reihou");
    }
    JsonArray args;
    args << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
    args << user->objectName();
    args << user->getGeneralName();
    args << QString();
    args << true;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    room->handleAcquireDetachSkills(user, skills, true);

    QString to_use;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        use_list << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            use_list << "analeptic";
        to_use = room->askForChoice(user, "rhruyi_skill_saveself", use_list.join("+"));
    } else if (user_string == "slash") {
        QStringList use_list;
        use_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        to_use = room->askForChoice(user, "rhruyi_skill_slash", use_list.join("+"));
    } else
        to_use = user_string;

    Card *use_card = Sanguosha->cloneCard(to_use);
    use_card->setSkillName("rhruyi");
    use_card->deleteLater();
    return use_card;
}

#include "touhou-hana.h"
class RhRuyi: public ZeroCardViewAsSkill
{
public:
    RhRuyi(): ZeroCardViewAsSkill("rhruyi")
    {
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (pattern == "peach" && player->getMark("Global_PreventPeach") > 0) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false;
        }
        return true;
    }

    virtual const Card *viewAs() const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            RhRuyiCard *card = new RhRuyiCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            return card;
        }

        const Card *c = Self->tag.value("rhruyi").value<const Card *>();
        if (c) {
            RhRuyiCard *card = new RhRuyiCard;
            card->setUserString(c->objectName());
            return card;
        } else
            return NULL;
    }

    virtual QDialog *getDialog() const
    {
        return ThMimengDialog::getInstance("rhruyi");
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *) const
    {
        return true;
    }
};

RhHuanjingCard::RhHuanjingCard()
{
    mute = true;
}

bool RhHuanjingCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(Self->property("rhhuanjing").toString());
        card->setSkillName("rhhuanjing");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *new_card = Sanguosha->cloneCard(Self->property("rhhuanjing").toString());
    new_card->setSkillName("rhhuanjing");
    new_card->addSubcard(this);
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetFixed();
}

bool RhHuanjingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(Self->property("rhhuanjing").toString());
        card->setSkillName("rhhuanjing");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *new_card = Sanguosha->cloneCard(Self->property("rhhuanjing").toString());
    new_card->setSkillName("rhhuanjing");
    new_card->addSubcard(this);
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool RhHuanjingCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(Self->property("rhhuanjing").toString());
        card->setSkillName("rhhuanjing");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *new_card = Sanguosha->cloneCard(Self->property("rhhuanjing").toString());
    new_card->setSkillName("rhhuanjing");
    new_card->addSubcard(this);
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *RhHuanjingCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *user = card_use.from;
    Room *room = user->getRoom();

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, user->objectName(), "rhhuanjing", QString());
    room->throwCard(this, reason, user);

    room->addPlayerMark(user, "rhhuanjing");

    Card *use_card = Sanguosha->cloneCard(user->property("rhhuanjing").toString());
    use_card->setSkillName("rhhuanjing");
    use_card->deleteLater();
    return use_card;
}

const Card *RhHuanjingCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, user->objectName(), "rhhuanjing", QString());
    room->throwCard(this, reason, user);

    room->addPlayerMark(user, "rhhuanjing");

    Card *use_card = Sanguosha->cloneCard(user->property("rhhuanjing").toString());
    use_card->setSkillName("rhhuanjing");
    use_card->deleteLater();
    return use_card;
}

class RhHuanjingVS : public OneCardViewAsSkill
{
public:
    RhHuanjingVS() : OneCardViewAsSkill("rhhuanjing")
    {
        filter_pattern = ".|spade!";
    }

    virtual Card *viewAs(const Card *originalCard) const
    {
        Card *card = new RhHuanjingCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("rhhuanjing") > 0)
            return false;
        if (!player->canDiscard(player, "he"))
            return false;
        QString obj = player->property("rhhuanjing").toString();
        if (obj.isEmpty())
            return false;
        const Card *card = Sanguosha->cloneCard(obj);
        return card->isAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getMark("rhhuanjing") > 0)
            return false;
        if (!player->canDiscard(player, "he"))
            return false;
        QString obj = player->property("rhhuanjing").toString();
        if (obj.isEmpty())
            return false;
        return pattern.contains(obj);
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->getMark("rhhuanjing") > 0)
            return false;
        if (!player->canDiscard(player, "he"))
            return false;
        QString obj = player->property("rhhuanjing").toString();
        return obj == "nullification";
    }
};

class RhHuanjing : public TriggerSkill
{
public:
    RhHuanjing() : TriggerSkill("rhhuanjing")
    {
        events << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
        view_as_skill = new RhHuanjingVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        } else if (data.toString() == objectName()) {
            if (triggerEvent == EventLoseSkill)
                room->setPlayerProperty(player, "rhhuanjing", QVariant());
            else
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QMap<QString, QStringList> name_map;
        name_map["basic"] = QStringList();
        name_map["single_target_trick"] = QStringList();
        name_map["other_trick"] = QStringList();

        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach (const Card *card, cards) {
            QString obj_name = card->objectName();
            if (obj_name.endsWith("slash"))
                obj_name = "slash";
            if (card->getType() == "basic") {
                if (!name_map["basic"].contains(obj_name))
                    name_map["basic"] << obj_name;
            } else if (card->isKindOf("SingleTargetTrick")) {
                if (!name_map["single_target_trick"].contains(obj_name))
                    name_map["single_target_trick"] << obj_name;
            } else if (card->isNDTrick()) {
                if (!name_map["other_trick"].contains(obj_name))
                    name_map["other_trick"] << obj_name;
            }
        }
        if (name_map["basic"].isEmpty())
            name_map.remove("basic");
        else
            name_map["basic"] << "cancel";
        if (name_map["single_target_trick"].isEmpty())
            name_map.remove("single_target_trick");
        else
            name_map["single_target_trick"] << "cancel";
        if (name_map["other_trick"].isEmpty())
            name_map.remove("other_trick");
        else
            name_map["other_trick"] << "cancel";

        room->sendCompulsoryTriggerLog(player, objectName());

        QString obj_n = "cancel";
        while (true) {
            QString type = room->askForChoice(player, objectName(), name_map.keys().join("+"));
            obj_n = room->askForChoice(player, objectName(), name_map[type].join("+"));
            if (obj_n == "cancel")
                continue;
            break;
        }

        room->setPlayerProperty(player, "rhhuanjing", obj_n);

        return false;
    }
};

/*class RhFangcun : public OneCardViewAsSkill
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
};*/

TenshiReihouPackage::TenshiReihouPackage()
    :Package("tenshi-reihou")
{
    General *reihou001 = new General(this, "reihou001", "rei", 4, true, true);
    reihou001->addSkill(new RhDuanlong);
    reihou001->addSkill(new FakeMoveSkill("rhduanlong"));
    related_skills.insertMulti("rhduanlong", "#rhduanlong-fake-move");
    reihou001->addSkill(new RhPohuang);

    General *reihou002 = new General(this, "reihou002", "rei", 4, true, true);
    reihou002->addSkill(new RhRuyi);

    General *reihou003 = new General(this, "reihou003", "rei", 4, true, true);
    reihou003->addSkill(new RhHuanjing);

    /*General *reihou005 = new General(this, "reihou005", "rei", 4, true, true);
    reihou005->addSkill(new RhFangcun);
    reihou005->addSkill(new RhHuifu);*/

    addMetaObject<RhDuanlongCard>();
    addMetaObject<RhRuyiCard>();
    addMetaObject<RhHuanjingCard>();
}

ADD_PACKAGE(TenshiReihou)
