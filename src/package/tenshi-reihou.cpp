#include "tenshi-reihou.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "client.h"
#include "maneuvering.h"

RhDuanlongCard::RhDuanlongCard()
{
    target_fixed = true;
}

void RhDuanlongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->removeReihouCard(source);
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        const Card *card = room->askForUseSlashTo(player, damage.from, "@rhpohuang:" + damage.from->objectName(), false);
        if (!card) {
            Card *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_rhpohuang");
            if (!player->canSlash(damage.from, slash, false)) {
                delete slash;
                return false;
            }

            room->removeReihouCard(player);
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

    room->removeReihouCard(rhruyi_general);

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

    room->removeReihouCard(user);

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
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
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
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return NULL;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
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
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
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

        LogMessage log;
        log.type = "#RhHuanjing";
        log.from = player;
        log.arg = obj_n;
        room->sendLog(log);
        room->setPlayerProperty(player, "rhhuanjing", obj_n);

        return false;
    }
};

class RhLvcao : public ProhibitSkill
{
public:
    RhLvcao() : ProhibitSkill("rhlvcao")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return from->getPhase() != Player::NotActive && to->hasSkill(objectName())
                && card->isKindOf("TrickCard") && card->isBlack();
    }
};

class RhLvcaoTargetMod : public TargetModSkill
{
public:
    RhLvcaoTargetMod() : TargetModSkill("#rhlvcao")
    {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        bool find_skill = false;
        if (from->hasSkill("rhlvcao"))
            find_skill = true;
        else {
            QList<const Player *> players = from->getAliveSiblings();
            foreach (const Player *p, players) {
                if (p->hasSkill("rhlvcao")) {
                    find_skill = true;
                    break;
                }
            }
        }
        if (find_skill && from->getPhase() != Player::NotActive)
            return 1000;
        return 0;
    }
};

class RhLiufu : public TriggerSkill
{
public:
    RhLiufu() : TriggerSkill("rhliufu")
    {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.card->isKindOf("BasicCard") || use.card->isNDTrick())
                    && use.to.contains(player))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->removeReihouCard(player);

        QString choice;
        QStringList choices;
        choices << "null" << "give";
        choice = room->askForChoice(player, objectName(), choices.join("+"), data);
        while (choice != "cancel") {
            choices.removeOne(choice);
            CardUseStruct use = data.value<CardUseStruct>();
            if (choice == "null") {
                LogMessage log;
                log.type = "#RhLiufu1";
                log.from = player;
                log.arg = "1";
                log.arg2 = use.card->objectName();
                room->sendLog(log);

                use.nullified_list << player->objectName();
                data = QVariant::fromValue(use);
            } else if (choice == "give") {
                LogMessage log;
                log.type = "#RhLiufu2";
                log.from = player;
                log.arg = "2";
                room->sendLog(log);

                room->setCardFlag(use.card, "rhliufu");
                use.card->tag["rhliufu"] = QVariant::fromValue(player);
            }
            if (!choices.contains("cancel"))
                choices << "cancel";
            choice = room->askForChoice(player, objectName(), choices.join("+"), data);
        }
        return false;
    }
};

class RhLiufuGet : public TriggerSkill
{
public:
    RhLiufuGet() : TriggerSkill("#rhliufu")
    {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *card = move.reason.m_extraData.value<const Card *>();
        if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
                && card->hasFlag("rhliufu")) {
            foreach (int id, move.card_ids) {
                if (card->getSubcards().contains(id))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *card = move.reason.m_extraData.value<const Card *>();
        ServerPlayer *player = card->tag["rhliufu"].value<ServerPlayer *>();
        card->tag.remove("rhliufu");
        room->setCardFlag(card, "-rhliufu");
        room->sendCompulsoryTriggerLog(player, "rhliufu");
        QList<int> ids;
        for (int i = 0; i < move.card_ids.length(); ++i) {
            int id = move.card_ids[i];
            if (card->getSubcards().contains(id)
                    && move.from_places[i] == Player::PlaceTable && move.to_place == Player::DiscardPile)
                ids << id;
        }
        if (!ids.isEmpty()) {
            move.removeCardIds(ids);
            data = QVariant::fromValue(move);
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), "rhliufu", "@rhliufu", false, true);
            DummyCard dummy(ids);
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE,
                                  player->objectName(),
                                  target->objectName(),
                                  "rhliufu",
                                  QString());
            room->obtainCard(target, &dummy, reason);
        }
        return false;
    }
};

RhPujiuCard::RhPujiuCard()
{
    handling_method = MethodUse;
    mute = true;
}

bool RhPujiuCard::targetFixed() const
{
    Card *peach = Sanguosha->cloneCard("peach");
    peach->setSkillName("rhpujiu");
    peach->addSubcard(this);
    peach->deleteLater();
    return peach && peach->targetFixed();
}

bool RhPujiuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *peach = Sanguosha->cloneCard("peach");
    peach->setSkillName("rhpujiu");
    peach->addSubcard(this);
    peach->deleteLater();
    return peach && peach->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, peach, targets);
}

bool RhPujiuCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card *peach = Sanguosha->cloneCard("peach");
    peach->setSkillName("rhpujiu");
    peach->addSubcard(this);
    peach->deleteLater();
    return peach && peach->targetsFeasible(targets, Self);
}

const Card *RhPujiuCard::validate(CardUseStruct &use) const
{
    ServerPlayer *user = use.from;
    Room *room = user->getRoom();
    room->addPlayerMark(user, "rhpujiu");

    Card *peach = Sanguosha->cloneCard("peach");
    peach->setSkillName("rhpujiu");
    peach->addSubcard(this);
    return peach;
}

const Card *RhPujiuCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    room->addPlayerMark(user, "rhpujiu");

    Card *peach = Sanguosha->cloneCard("peach");
    peach->setSkillName("rhpujiu");
    peach->addSubcard(this);
    return peach;
}

class RhPujiu : public OneCardViewAsSkill
{
public:
    RhPujiu() : OneCardViewAsSkill("rhpujiu")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual Card *viewAs(const Card *originalCard) const
    {
        Card *card = new RhPujiuCard();
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        Card *peach = new Peach(Card::NoSuit, 0);
        peach->setSkillName(objectName());
        return peach->isAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("peach") && player->getMark("Global_PreventPeach") == 0;
    }
};

class RhPujiuTrigger : public TriggerSkill
{
public:
    RhPujiuTrigger() : TriggerSkill("#rhpujiu")
    {
        events << CardUsed << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == "rhpujiu")
                room->setPlayerMark(player, "rhpujiu", 0);
        } else if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Peach") && use.card->getSkillName() == objectName()
                    && player->getMark("rhpujiu") == player->getHp())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->removeReihouCard(player);
        player->throwAllHandCards();
        return false;
    }
};

class RhXuesha : public TriggerSkill
{
public:
    RhXuesha() : TriggerSkill("rhxuesha")
    {
        events << DamageInflicted << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == DamageInflicted) {
            if (TriggerSkill::triggerable(player) && player->getMark("ikguijing") > 0 && player->isWounded())
                return QStringList(objectName());
        } else if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash") || !use.card->isRed())
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DamageInflicted) {
            room->sendCompulsoryTriggerLog(player, objectName());
            room->broadcastSkillInvoke(objectName());
            return true;
        } else {
            room->sendCompulsoryTriggerLog(player, objectName());

            CardUseStruct use = data.value<CardUseStruct>();
            QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
            int index = 0;
            foreach (ServerPlayer *p, use.to) {
                LogMessage log;
                log.type = "#NoJink";
                log.from = p;
                room->sendLog(log);
                jink_list.replace(index, QVariant(0));
                index++;
            }
            player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
            return false;
        }
    }
};

class RhXueshaTargetMod : public TargetModSkill
{
public:
    RhXueshaTargetMod() : TargetModSkill("#rhxuesha")
    {
        pattern = "Slash|black";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasSkill("rhxuesha"))
            return 1000;
        return 0;
    }
};

class RhZhenyao : public TriggerSkill
{
public:
    RhZhenyao() : TriggerSkill("rhzhenyao")
    {
        events << TargetSpecified << TargetConfirmed << EventPhaseChanging << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        } else if (triggerEvent == TargetSpecified) {
            if (TriggerSkill::triggerable(player) && player->getMark(objectName()) == 0) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->isKindOf("Slash"))
                    return QStringList(objectName());
            }
        } else if (triggerEvent == TargetConfirmed) {
            if (TriggerSkill::triggerable(player) && player->getMark(objectName()) == 0) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.to.contains(player) && use.card->isKindOf("Slash"))
                    return QStringList(objectName());
            }
        } else if (triggerEvent == FinishJudge) {
            if (player->hasFlag("rhzhenyao")) {
                player->setFlags("-rhzhenyao");
                CardUseStruct use = player->tag["RhZhenyaoUse"].value<CardUseStruct>();
                player->tag.remove("RhZhenyaoUse");
                player->addZhenyaoTag(use.card);
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->addPlayerMark(player, objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct judge;
        judge.pattern = ".|black";
        judge.good = true;
        judge.reason = objectName();
        judge.who = player;
        if (triggerEvent == TargetConfirmed) {
            player->setFlags("rhzhenyao");
            player->tag["RhZhenyaoUse"] = data;
        }
        room->judge(judge);
        if (triggerEvent == TargetConfirmed && player->hasFlag("rhzhenyao")) {
            player->setFlags("-rhzhenyao");
            player->tag.remove("RhZhenyaoDamage");
        }

        if (judge.isBad())
            return false;

        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified) {
            QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
            int index = 0;
            foreach (ServerPlayer *p, use.to) {
                LogMessage log;
                log.type = "#NoJink";
                log.from = p;
                room->sendLog(log);
                jink_list.replace(index, QVariant(0));
                index++;
            }
            player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        }
        return false;
    }
};

class RhZhenyaoPrevent : public TriggerSkill
{
public:
    RhZhenyaoPrevent(): TriggerSkill("#rhzhenyao")
    {
        events << DamageForseen << PreHpLost;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return !player->tag["RhZhenyao"].toStringList().isEmpty();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "rhzhenyao");
        room->broadcastSkillInvoke(objectName());
        return true;
    }
};

RhGaimingCard::RhGaimingCard()
{
}

bool RhGaimingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;
    QList<const Player *> players = Self->getAliveSiblings();
    players << Self;
    int max = -1000, min = 1000;
    foreach (const Player *p, players) {
        if (max < p->getHp())
            max = p->getHp();
        if (min > p->getHp())
            min = p->getHp();
    }
    return to_select->getHp() == max || (to_select->getHp() == min && to_select->isWounded());
}

void RhGaimingCard::onEffect(const CardEffectStruct &effect) const
{
    QStringList choices;
    Room *room = effect.from->getRoom();
    QList<ServerPlayer *> players = room->getAlivePlayers();
    int max = -1000, min = 1000;
    foreach (ServerPlayer *p, players) {
        if (max < p->getHp())
            max = p->getHp();
        if (min > p->getHp())
            min = p->getHp();
    }
    if (effect.to->getHp() == max)
        choices << "lose";
    if (effect.to->getHp() == min && effect.to->isWounded())
        choices << "recover";
    QString choice = choices.first();
    if (choices.length() != 1)
        choice = room->askForChoice(effect.from, "rhgaiming", choices.join("+"), QVariant::fromValue(effect));
    if (choice == "lose")
        room->loseHp(effect.to);
    else if (choice == "recover")
        room->recover(effect.to, RecoverStruct(effect.from));
}

class RhGaiming : public ZeroCardViewAsSkill
{
public:
    RhGaiming() : ZeroCardViewAsSkill("rhgaiming")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("RhGaimingCard");
    }

    virtual Card *viewAs() const
    {
        return new RhGaimingCard;
    }
};

class RhXusheng : public TriggerSkill
{
public:
    RhXusheng() : TriggerSkill("rhxusheng")
    {
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who->getHp() > 0 || dying.who->isDead())
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            room->removeReihouCard(player);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        room->recover(dying.who, RecoverStruct(player, NULL, 1 - dying.who->getHp()));
        return false;
    }
};

class RhBaiming : public TriggerSkill
{
public:
    RhBaiming() : TriggerSkill("rhbaiming")
    {
        events << EventAcquireSkill << EventLoseSkill << Damaged;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == Damaged) {
            if (player->isAlive()) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->tag[objectName()].toString() == player->objectName())
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        } else if (data.toString() == objectName()) {
            if (triggerEvent == EventLoseSkill)
                player->tag.remove(objectName());
            else
                skill_list.insert(player, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventAcquireSkill)
            return true;

        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventAcquireSkill) {
            ServerPlayer *target = room->askForPlayerChosen(ask_who, room->getOtherPlayers(ask_who), objectName(), "@rhbaiming", false, true);
            room->broadcastSkillInvoke(objectName());
            ask_who->tag[objectName()] = target->objectName();
            return false;
        }

        if (ask_who->canDiscard(ask_who, "he"))
            room->askForDiscard(ask_who, objectName(), 1, 1);

        if (!player->isNude()) {
            if (ask_who->canDiscard(player, "he")) {
                int card_id = room->askForCardChosen(ask_who, player, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, player, ask_who);
            }
        } else
            room->loseHp(player);

        return false;
    }
};

class RhChengfeng : public OneCardViewAsSkill
{
public:
    RhChengfeng() : OneCardViewAsSkill("rhchengfeng") {
        filter_pattern = "Slash|black";
        response_or_use = true;
        response_pattern = "jink";
    }

    virtual Card *viewAs(const Card *originalCard) const
    {
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->addSubcard(originalCard);
        jink->setSkillName(objectName());
        return jink;
    }
};

class RhYuhuo : public OneCardViewAsSkill
{
public:
    RhYuhuo() : OneCardViewAsSkill("rhyuhuo") {
        filter_pattern = "BasicCard|red";
        response_or_use = true;
        response_pattern = "slash";
    }

    virtual Card *viewAs(const Card *originalCard) const
    {
        FireSlash *f_slash = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        f_slash->addSubcard(originalCard);
        f_slash->setSkillName(objectName());
        return f_slash;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }
};

class RhZhengyang : public TriggerSkill
{
public:
    RhZhengyang() : TriggerSkill("rhzhengyang")
    {
        events << EventAcquireSkill << EventLoseSkill << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == TargetSpecified) {
            if (TriggerSkill::triggerable(player) && !player->getPile("rhzhengyangpile").isEmpty()) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->isKindOf("Slash")
                        && use.card->getSuit() == Sanguosha->getEngineCard(player->getPile("rhzhengyangpile").first())->getSuit()) {
                    return QStringList(objectName());
                }
            }
        } else if (data.toString() == objectName()) {
            if (triggerEvent == EventAcquireSkill && !player->isKongcheng())
                return QStringList(objectName());
            if (triggerEvent == EventLoseSkill && !player->getPile("rhzhengyangpile").isEmpty())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (triggerEvent == EventAcquireSkill) {
            const Card *card = room->askForCard(player, ".", "@rhzhengyang", QVariant(), Card::MethodNone);
            if (card) {
                player->tag["RhZhengyangCard"] = QVariant::fromValue(card);
                room->sendCompulsoryTriggerLog(player, objectName());
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        } else {
            room->sendCompulsoryTriggerLog(player, objectName());
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventAcquireSkill) {
            const Card *card = player->tag["RhZhengyangCard"].value<const Card *>();
            player->tag.remove("RhZhengyangCard");
            player->addToPile("rhzhengyangpile", card);
        } else if (triggerEvent == EventLoseSkill) {
            room->obtainCard(player, player->getPile("rhzhengyangpile").first());
        } else if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
            int index = 0;
            foreach (ServerPlayer *p, use.to) {
                LogMessage log;
                log.type = "#NoJink";
                log.from = p;
                room->sendLog(log);
                jink_list.replace(index, QVariant(0));
                index++;
            }
            player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        }
        return false;
    }
};

class RhZhengyangTrigger : public TriggerSkill
{
public:
    RhZhengyangTrigger() : TriggerSkill("#rhzhengyang")
    {
         events << TrickCardCanceling;
         frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &ask_who) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.from && effect.from->isAlive()
                && effect.from->hasSkill("rhzhengyang")
                && !effect.from->getPile("rhzhengyangpile").isEmpty()) {
            if (effect.card->getSuit() == Sanguosha->getEngineCard(effect.from->getPile("rhzhengyangpile").first())->getSuit()) {
                ask_who = effect.from;
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        room->notifySkillInvoked(ask_who, objectName());
        return true;
    }
};

class RhZhengyangProhibit : public ProhibitSkill
{
public:
    RhZhengyangProhibit() : ProhibitSkill("#rhzhengyang-prohibit")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill("rhzhengyang") && !to->getPile("rhzhengyangpile").isEmpty()
                && card->getSuit() == Sanguosha->getEngineCard(to->getPile("rhzhengyangpile").first())->getSuit();
    }
};

class RhChunyin : public TriggerSkill
{
public:
    RhChunyin() : TriggerSkill("rhchunyin")
    {
        events << EventAcquireSkill << EventLoseSkill << CardUsed;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->getPile("rhchunyinpile").isEmpty())
                    continue;
                Card::Suit suit = Sanguosha->getEngineCard(p->getPile("rhchunyinpile").first())->getSuit();
                if (use.card->getSuit() == suit)
                    skill_list.insert(p, QStringList(objectName()));
            }
        } else if (data.toString() == objectName())
            skill_list.insert(player, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventAcquireSkill) {
            const Card *card = room->askForCard(player, ".", "@rhchunyin", QVariant(), Card::MethodNone);
            if (card) {
                player->tag["RhChunyinCard"] = QVariant::fromValue(card);
                room->sendCompulsoryTriggerLog(player, objectName());
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == ask_who) {
                ServerPlayer *target = room->askForPlayerChosen(ask_who, room->getOtherPlayers(ask_who), objectName(), "@rhchunyin-draw", true, true);
                if (target) {
                    room->broadcastSkillInvoke(objectName());
                    ask_who->tag["RhChunyinTarget"] = QVariant::fromValue(target);
                    return true;
                }
            } else if (ask_who->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        } else {
            room->sendCompulsoryTriggerLog(player, objectName());
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventAcquireSkill) {
            const Card *card = player->tag["RhChunyinCard"].value<const Card *>();
            player->tag.remove("RhChunyinCard");
            player->addToPile("rhchunyinpile", card);
        } else if (triggerEvent == EventLoseSkill) {
            room->obtainCard(player, player->getPile("rhchunyinpile").first());
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == ask_who) {
                ServerPlayer *target = player->tag["RhChunyinTarget"].value<ServerPlayer *>();
                player->tag.remove("RhChunyinTarget");
                if (target)
                    room->drawCards(target, 1, objectName());
            } else
                room->drawCards(ask_who, 1, objectName());
        }
        return false;
    }
};

class RhSanmei : public TriggerSkill
{
public:
    RhSanmei() : TriggerSkill("rhsanmei")
    {
        events << ConfirmDamage << DamageInflicted;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == ConfirmDamage) {
            if (damage.card && damage.card->getTypeId() != Card::TypeSkill
                    && damage.card->isRed() && damage.nature != DamageStruct::Fire) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                    skill_list.insert(p, QStringList(objectName()));
            }
        } else {
            if (TriggerSkill::triggerable(player) && damage.nature == DamageStruct::Fire)
                skill_list.insert(player, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        room->broadcastSkillInvoke(objectName());
        if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.nature = DamageStruct::Fire;
            data = QVariant::fromValue(damage);
        } else
            return true;

        return false;
    }
};

RhXuanrenCard::RhXuanrenCard()
{
}

bool RhXuanrenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;
    QString obj_name = Self->property("rhxuanren").toString();
    const Player *to = NULL;
    if (Self->objectName() == obj_name)
        to = Self;
    else {
        foreach (const Player *p, Self->getAliveSiblings()) {
            if (p->objectName() == obj_name) {
                to = p;
                break;
            }
        }
    }
    if (!to)
        return false;
    return to->distanceTo(to_select) == 1;
}

void RhXuanrenCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->damage(DamageStruct("rhxuanren", effect.from, effect.to));
}

class RhXuanrenVS : public OneCardViewAsSkill
{
public:
    RhXuanrenVS() : OneCardViewAsSkill("rhxuanren")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@rhxuanren";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new RhXuanrenCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class RhXuanren : public TriggerSkill
{
public:
    RhXuanren() : TriggerSkill("rhxuanren")
    {
        events << Damaged;
        view_as_skill = new RhXuanrenVS;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (player->isAlive()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash")) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->canDiscard(p, "h")) {
                        foreach (ServerPlayer *victim, room->getOtherPlayers(p)) {
                            if (player->distanceTo(victim) == 1) {
                                skill_list.insert(p, QStringList(objectName()));
                                break;
                            }
                        }
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        room->setPlayerProperty(ask_who, "rhxuanren", player->objectName());
        room->askForUseCard(ask_who, "@@rhxuanren", "@rhxuanren", -1, Card::MethodDiscard);
        room->setPlayerProperty(ask_who, "rhxuanren", QVariant());
        return false;
    }
};

class RhGuozao : public TriggerSkill
{
public:
    RhGuozao() : TriggerSkill("rhguozao")
    {
        events << EventLoseSkill << EventAcquireSkill;
        frequency = NotCompulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        if (data.toString() == objectName())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        if (triggerEvent == EventAcquireSkill) {
            room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);

            LogMessage log;
            log.type = "#GainMaxHp";
            log.from = player;
            log.arg = "1";
            room->sendLog(log);

            if (player->isWounded()) {
                room->recover(player, RecoverStruct(player));
            } else {
                LogMessage log2;
                log2.type = "#GetHp";
                log2.from = player;
                log2.arg = QString::number(player->getHp());
                log2.arg2 = QString::number(player->getMaxHp());
                room->sendLog(log2);
            }
        } else {
            bool draw = !player->isWounded();
            room->loseMaxHp(player);
            if (draw)
                player->drawCards(1, objectName());
        }
        return false;
    }
};

class RhShenguang : public TriggerSkill
{
public:
    RhShenguang() : TriggerSkill("rhshenguang")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Draw) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        room->removeReihouCard(ask_who);

        QList<int> card_ids = room->getNCards(room->getAllPlayers().length());
        room->fillAG(card_ids);

        try {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->canSlash(player, false) && room->askForUseSlashTo(p, player, "@rhshenguang:" + player->objectName(), false)) {
                    int card_id = room->askForAG(p, card_ids, false, objectName());
                    card_ids.removeOne(card_id);
                    room->takeAG(p, card_id);
                }
            }
            clearRestCards(room, ask_who, card_ids);
            return false;
        }
        catch (TriggerEvent triggerEvent) {
            if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                clearRestCards(room, ask_who, card_ids);
            throw triggerEvent;
        }
    }

    void clearRestCards(Room *room, ServerPlayer *player, const QList<int> &card_ids) const
    {
        room->clearAG();
        if (card_ids.isEmpty())
            return;
        DummyCard *dummy = new DummyCard(card_ids);
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
        room->throwCard(dummy, reason, NULL);
        delete dummy;
    }
};

class RhMangti : public OneCardViewAsSkill
{
public:
    RhMangti() : OneCardViewAsSkill("rhmangti")
    {
        response_or_use = true;
        filter_pattern = ".|club";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Dismantlement *dis = new Dismantlement(Card::SuitToBeDecided, -1);
        dis->addSubcard(originalCard);
        dis->setSkillName(objectName());
        return dis;
    }
};

class RhLingwei : public ProhibitSkill
{
public:
    RhLingwei() : ProhibitSkill("rhlingwei")
    {
        frequency = NotCompulsory;
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(objectName()) && (card->isKindOf("Dismantlement") || card->isKindOf("Snatch"));
    }
};

class RhWangzhong : public TriggerSkill
{
public:
    RhWangzhong() : TriggerSkill("rhwangzhong")
    {
        events << EventPhaseEnd;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->getHandcardNum() < p->getMaxHp())
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        ask_who->drawCards(1, objectName());
        if (!ask_who->canSlash(player, false)
                || !room->askForUseSlashTo(ask_who, player, "@rhwangzhong:" + player->objectName(), false))
            room->loseHp(ask_who);
        return false;
    }
};

class RhCuigu : public TriggerSkill
{
public:
    RhCuigu() : TriggerSkill("rhcuigu")
    {
        events << TargetSpecified << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark(objectName()) > 0) {
                        room->setPlayerMark(p, objectName(), 0);
                        room->removePlayerCardLimitation(p, "use", "Peach$0");
                    }
                }
            }
        } else if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->getPhase() == Player::Play && use.card->isKindOf("Slash") && !use.to.isEmpty())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to) {
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());

                room->removeReihouCard(player);
                room->setPlayerCardLimitation(p, "use", "Peach", false);
                room->addPlayerMark(p, objectName());
                LogMessage log;
                log.type = "#RhCuigu";
                log.from = p;
                log.arg = "peach";
                room->sendLog(log);
                break;
            }
        }
        return false;
    }
};

class RhCuiguProhibit : public ProhibitSkill
{
public:
    RhCuiguProhibit() : ProhibitSkill("#rhcuigu") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->getMark("rhcuigu") > 0 && card->isKindOf("Peach");
    }
};

class RhLinghai : public TriggerSkill
{
public:
    RhLinghai() : TriggerSkill("rhlinghai")
    {
        events << DamageInflicted;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage >= player->getHp()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            room->removeReihouCard(ask_who);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return true;
    }
};

RhHaoqiangDialog *RhHaoqiangDialog::getInstance()
{
    static RhHaoqiangDialog *instance;
    if (instance == NULL)
        instance = new RhHaoqiangDialog();

    return instance;
}

RhHaoqiangDialog::RhHaoqiangDialog()
{
    setObjectName("rhhaoqiang");
    setWindowTitle(Sanguosha->translate("rhhaoqiang"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void RhHaoqiangDialog::popup()
{
    if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        Self->tag.remove("rhhaoqiang");
        emit onButtonClick();
        return;
    }

    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QStringList card_names;
    card_names << "slash" << "duel";

    foreach (QString card_name, card_names) {
        QCommandLinkButton *button = new QCommandLinkButton;
        button->setText(Sanguosha->translate(card_name));
        button->setObjectName(card_name);
        group->addButton(button);

        bool can = true;
        const Card *c = Sanguosha->cloneCard(card_name);
        if (Self->isCardLimited(c, Card::MethodUse) || !c->isAvailable(Self))
            can = false;
        delete c;
        button->setEnabled(can);
        button_layout->addWidget(button);

        if (!map.contains(card_name)) {
            Card *c = Sanguosha->cloneCard(card_name);
            c->setParent(this);
            map.insert(card_name, c);
        }
    }

    exec();
}

void RhHaoqiangDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag["rhhaoqiang"] = QVariant::fromValue(card);
    emit onButtonClick();
    accept();
}

RhHaoqiangCard::RhHaoqiangCard()
{
    mute = true;
}

bool RhHaoqiangCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(user_string);
        card->setSkillName("rhhaoqiang");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *new_card = Sanguosha->cloneCard(user_string);
    new_card->setSkillName("rhhaoqiang");
    new_card->addSubcard(this);
    new_card->deleteLater();
    return new_card && new_card->targetFixed();
}

bool RhHaoqiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(user_string);
        card->setSkillName("rhhaoqiang");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *new_card = Sanguosha->cloneCard(user_string);
    new_card->setSkillName("rhhaoqiang");
    new_card->addSubcard(this);
    new_card->deleteLater();
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool RhHaoqiangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(user_string);
        card->setSkillName("rhhaoqiang");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *new_card = Sanguosha->cloneCard(user_string);
    new_card->setSkillName("rhhaoqiang");
    new_card->addSubcard(this);
    new_card->deleteLater();
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *RhHaoqiangCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *user = card_use.from;
    Room *room = user->getRoom();

    room->addPlayerMark(user, "rhhaoqiang");

    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("rhhaoqiang");
    use_card->addSubcard(this);
    use_card->deleteLater();
    return use_card;
}

const Card *RhHaoqiangCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    room->addPlayerMark(user, "rhhaoqiang");

    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("rhhaoqiang");
    use_card->addSubcard(this);
    use_card->deleteLater();
    return use_card;
}

class RhHaoqiangVS : public OneCardViewAsSkill
{
public:
    RhHaoqiangVS() : OneCardViewAsSkill("rhhaoqiang")
    {
        filter_pattern = "TrickCard";
        response_pattern = "slash";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark(objectName()) > 0)
            return false;
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        slash->setSkillName(objectName());
        slash->deleteLater();
        Duel *duel = new Duel(Card::SuitToBeDecided, -1);
        duel->setSkillName(objectName());
        duel->deleteLater();
        return slash->isAvailable(player) || duel->isAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
            RhHaoqiangCard *card = new RhHaoqiangCard();
            card->addSubcard(originalCard);
            card->setUserString("slash");
            return card;
        }

        const Card *c = Self->tag.value("rhhaoqiang").value<const Card *>();
        if (c) {
            RhHaoqiangCard *card = new RhHaoqiangCard();
            card->addSubcard(originalCard);
            card->setUserString(c->objectName());
            return card;
        }

        return NULL;
    }
};

class RhHaoqiang : public TriggerSkill
{
public:
    RhHaoqiang() : TriggerSkill("rhhaoqiang")
    {
        events << EventPhaseChanging;
        view_as_skill = new RhHaoqiangVS;
    }

    virtual QDialog *getDialog() const
    {
        return RhHaoqiangDialog::getInstance();
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->setPlayerMark(p, objectName(), 0);
        }
        return QStringList();
    }
};

class RhLiedan : public TriggerSkill
{
public:
    RhLiedan() : TriggerSkill("rhliedan")
    {
        events << Damaged << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark(objectName()) > 0) {
                        room->setPlayerMark(p, objectName(), 0);
                        room->detachSkillFromPlayer(p, "ikwushuang", false, true);
                    }
                }
            }
        } else {
            if (TriggerSkill::triggerable(player) && player->canDiscard(player, "h"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForCard(player, ".", "@rhliedan", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!player->hasSkill("ikwushuang")) {
            room->addPlayerMark(player, objectName());
            room->acquireSkill(player, "ikwushuang");
        }

        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from) {
            Duel *duel = new Duel(Card::NoSuit, 0);
            duel->setSkillName(objectName());
            if (!player->isCardLimited(duel, Card::MethodUse) && !player->isProhibited(damage.from, duel))
                room->useCard(CardUseStruct(duel, player, damage.from));
            else
                delete duel;
        }
        return false;
    }
};

RhYarenCard::RhYarenCard()
{
}

bool RhYarenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void RhYarenCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "rhyaren");
    if (effect.from->pindian(effect.to, "rhyaren", NULL)) {
        effect.from->tag["RhYarenTarget"] = QVariant::fromValue(effect.to);
        room->setFixedDistance(effect.from, effect.to, 1);
    } else {
        if (effect.to->isWounded() && effect.from->askForSkillInvoke("rhyaren_recover", "yes"))
            room->recover(effect.to, RecoverStruct(effect.from));
    }
}

class RhYarenViewAsSkill : public ZeroCardViewAsSkill
{
public:
    RhYarenViewAsSkill() : ZeroCardViewAsSkill("rhyaren")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("RhYarenCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new RhYarenCard;
    }
};

class RhYaren : public TriggerSkill
{
public:
    RhYaren() : TriggerSkill("rhyaren")
    {
        events << EventPhaseChanging << Death << EventLoseSkill;
        view_as_skill = new RhYarenViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *gaoshun, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName())
                room->setPlayerMark(gaoshun, "rhyaren", 0);
            return QStringList();
        }
        if (!gaoshun || !gaoshun->tag["RhYarenTarget"].value<ServerPlayer *>())
            return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        }
        ServerPlayer *target = gaoshun->tag["RhYarenTarget"].value<ServerPlayer *>();
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != gaoshun) {
                if (death.who == target) {
                    room->removeFixedDistance(gaoshun, target, 1);
                    gaoshun->tag.remove("RhYarenTarget");
                }
                return QStringList();
            }
        }
        if (target) {
            room->removeFixedDistance(gaoshun, target, 1);
            gaoshun->tag.remove("RhYarenTarget");
        }
        return QStringList();
    }
};

class RhShixiang : public TriggerSkill
{
public:
    RhShixiang() : TriggerSkill("rhshixiang")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->getMark("rhshixiang") > 0) {
                room->setPlayerMark(player, "rhshixiang", 0);
                room->detachSkillFromPlayer(player, "rhyaren", false, true);
            }
        } else if (player->getPhase() == Player::RoundStart && !player->hasSkill("rhyaren")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p == player)
                    continue;
                if (p->getMark("rhyaren") > 0)
                    continue;
                skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke("rhshixiang", QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            room->removeReihouCard(ask_who);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->acquireSkill(player, "rhyaren");
        room->addPlayerMark(player, "rhshixiang");
        return false;
    }
};

RhYinrenCard::RhYinrenCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

void RhYinrenCard::onEffect(const CardEffectStruct &effect) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE,
                          effect.from->objectName(),
                          effect.to->objectName(),
                          "rhyinren",
                          QString());
    Room *room = effect.from->getRoom();
    room->obtainCard(effect.to, this, reason, false);

    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_rhyinren");
    if (effect.from->canSlash(effect.to, slash, false))
        room->useCard(CardUseStruct(slash, effect.from, effect.to));
    else
        delete slash;
}

class RhYinren : public ViewAsSkill
{
public:
    RhYinren() : ViewAsSkill("rhyinren")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;
        Card *card = new RhYinrenCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getHandcardNum() > 1;
    }
};

class RhYinrenTrigger : public TriggerSkill
{
public:
    RhYinrenTrigger() : TriggerSkill("#rhyinren")
    {
        events << EventLoseSkill << CardUsed;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == "rhyinren")
                room->setPlayerMark(player, "rhyinren", 0);
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("RhYinrenCard") && player->getMark("rhyinren") == 1)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "rhyinren");
        room->removeReihouCard(player);
        room->loseHp(player);
        return false;
    }
};

class RhYeming : public TriggerSkill
{
public:
    RhYeming() : TriggerSkill("rhyeming")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->getMark("rhyeming") > 0) {
                room->setPlayerMark(player, "rhyeming", 0);
                room->detachSkillFromPlayer(player, "rhyinren", false, true);
            }
        } else if (player->getPhase() == Player::RoundStart && !player->hasSkill("rhyinren")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p == player)
                    continue;
                skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke("rhyeming", QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->acquireSkill(player, "rhyinren");
        room->addPlayerMark(player, "rhyeming");
        return false;
    }
};

class RhYiqie : public TriggerSkill
{
public:
    RhYiqie() : TriggerSkill("rhyiqie")
    {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (player != use.from || !use.card->isKindOf("Slash"))
            return QStringList();
        foreach (ServerPlayer *p, use.to) {
            if (p->canDiscard(p, "e"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to) {
            if (p->canDiscard(p, "e") && player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());
                int card_id = room->askForCardChosen(p, p, "e", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, p);
            }
        }
        return false;
    }
};

class RhYaozhang : public TriggerSkill
{
public:
    RhYaozhang() : TriggerSkill("rhyaozhang")
    {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.to.contains(player))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.from || use.from->isDead()
                || !room->askForCard(use.from, ".Equip", "@rhyaozhang:" + player->objectName())) {
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(player);
        }
        return false;
    }
};

class RhSanglv : public TriggerSkill
{
public:
    RhSanglv() : TriggerSkill("rhsanglv")
    {
        events << PreDamageDone << CardsMoveOneTime << EventPhaseEnd;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->getPhase() == Player::Play && !damage.from->hasFlag("RhSanglvDamageInPlayPhase"))
                damage.from->setFlags("RhSanglvDamageInPlayPhase");
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && player->getPhase() == Player::Discard) {
                if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    foreach (int id, move.card_ids) {
                        if (Sanguosha->getCard(id)->isKindOf("Slash")) {
                            player->setFlags("RhSanglvDiscardSlash");
                        }
                    }
                }
            }
        } else if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() == Player::Discard
                    && !player->hasFlag("RhSanglvDamageInPlayPhase")
                    && player->hasFlag("RhSanglvDiscardSlash")) {
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("_rhsanglv");
                slash->deleteLater();
                if (!player->isCardLimited(slash, Card::MethodUse)) {
                    foreach (ServerPlayer *target, room->getAlivePlayers()) {
                        if (player->canSlash(target, slash)) {
                            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                                skill_list.insert(p, QStringList(objectName()));
                            break;
                        }
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_rhsanglv");
        slash->deleteLater();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *target, room->getAlivePlayers()) {
            if (player->canSlash(target, slash))
                targets << target;
        }
        ServerPlayer *victim = room->askForPlayerChosen(player, targets, objectName(), "@dummy-slash");
        room->useCard(CardUseStruct(slash, player, victim));
        return false;
    }
};

class RhWuyin : public TriggerSkill
{
public:
    RhWuyin() : TriggerSkill("rhwuyin")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Play) {
            LureTiger *lt = new LureTiger(Card::NoSuit, 0);
            lt->setSkillName("_rhwuyin");
            lt->deleteLater();
            if (!player->isCardLimited(lt, Card::MethodUse)) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!player->isProhibited(p, lt)) {
                        foreach (ServerPlayer *o, room->findPlayersBySkillName(objectName()))
                            skill_list.insert(o, QStringList(objectName()));
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        LureTiger *lt = new LureTiger(Card::NoSuit, 0);
        lt->setSkillName("_rhwuyin");
        lt->deleteLater();
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!player->isProhibited(p, lt)) {
                victims << p;
            }
        }
        if (!victims.isEmpty()) {
            ServerPlayer *victim = room->askForPlayerChosen(ask_who, victims, objectName(), "@rhwuyin", true, true);
            if (victim) {
                room->broadcastSkillInvoke(objectName());
                ask_who->tag["RhWuyinTarget"] = QVariant::fromValue(victim);
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        ServerPlayer *target = ask_who->tag["RhWuyinTarget"].value<ServerPlayer *>();
        ask_who->tag.remove("RhWuyinTarget");
        if (target) {
            LureTiger *lt = new LureTiger(Card::NoSuit, 0);
            lt->setSkillName("_rhwuyin");
            room->useCard(CardUseStruct(lt, player, target));
        }
        return false;
    }
};

class RhChanling : public TriggerSkill
{
public:
    RhChanling() : TriggerSkill("rhchanling")
    {
        events << Damaged;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player && player->isAlive()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->distanceTo(player) != -1 && p->distanceTo(player) <= 1) {
                    skill_list.insert(p, QStringList(objectName()));
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        int n = data.value<DamageStruct>().damage;
        ask_who->drawCards(n, objectName());
        if (ask_who->getHandcardNum() >= n) {
            const Card *dummy = room->askForExchange(player, objectName(), n, n, false,
                                                    QString("@rhchanling:%1::%2").arg(player->objectName()).arg(n), true);
            if (dummy) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, ask_who->objectName(),
                                      player->objectName(), objectName(), QString());
                room->obtainCard(player, dummy, reason, false);
                delete dummy;
            }
        }
        return false;
    }
};

class RhShendai : public TriggerSkill
{
public:
    RhShendai(): TriggerSkill("rhshendai")
    {
        events << BeforeCardsMove;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from && move.from->isAlive() && move.from_places.contains(Player::PlaceHand)
                && ((move.reason.m_reason == CardMoveReason::S_REASON_DISMANTLE
                     && move.reason.m_playerId != move.reason.m_targetId)
                    || (move.to && move.to != move.from && move.to_place == Player::PlaceHand
                        && move.reason.m_reason != CardMoveReason::S_REASON_GIVE
                        && move.reason.m_reason != CardMoveReason::S_REASON_SWAP)))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        move.from->setFlags("RhShendaiMoveFrom"); // For AI
        bool invoke = player->askForSkillInvoke(objectName(), data);
        move.from->setFlags("-RhShendaiMoveFrom");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->removeReihouCard(player);
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);
        return false;
    }
};

class RhJiyu : public TriggerSkill
{
public:
    RhJiyu() : TriggerSkill("rhjiyu") {
        events << EventPhaseChanging << CardFinished << PreDamageDone;
        frequency = Frequent;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == CardFinished) {
            ServerPlayer *current = room->getCurrent();
            if (current == player && player->isAlive() && player->getPhase() != Player::NotActive) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->getTypeId() != Card::TypeSkill) {
                    foreach (ServerPlayer *p, use.to)
                        p->setMark("rhjiyu_use", 1);
                }
            }
        } else if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive()) {
                ServerPlayer *current = room->getCurrent();
                if (damage.from == current) {
                    player->setMark("rhjiyu_damage", 1);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::RoundStart) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    p->setMark("rhjiyu_use", 0);
                    p->setMark("rhjiyu_damage", 0);
                }
            } else if (change.to == Player::NotActive) {
                foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                    if (owner->getMark("rhjiyu_use") > 0 && owner->getMark("rhjiyu_damage") == 0)
                        skill_list.insert(owner, QStringList(objectName()));
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        ask_who->drawCards(1, objectName());
        return false;
    }
};

class RhFuyin : public TriggerSkill
{
public:
    RhFuyin() : TriggerSkill("rhfuyin")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->removeReihouCard(ask_who);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        Peach *peach = new Peach(Card::NoSuit, 0);
        peach->setSkillName("_rhfuyin");
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName("_rhfuyin");
        QStringList choices;
        if (peach->isAvailable(player))
            choices << "peach";
        if (analeptic->isAvailable(player))
            choices << "analeptic";
        if (choices.isEmpty()) {
            delete peach;
            delete analeptic;
            return false;
        }
        QString choice = choices.first();
        if (choices.length() != 1) {
            choice = room->askForChoice(player, objectName(), choices.join("+"));
        }
        if (choice == "peach") {
            delete analeptic;
            room->useCard(CardUseStruct(peach, player, QList<ServerPlayer *>()));
        } else if (choice == "analeptic") {
            delete peach;
            room->useCard(CardUseStruct(analeptic, player, QList<ServerPlayer *>()), true);
        }
        return false;
    }
};

class RhPihuai : public TriggerSkill
{
public:
    RhPihuai() : TriggerSkill("rhpihuai")
    {
        events << Damaged;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->isAlive()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->canDiscard(player, "h"))
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        int card_id = room->askForCardChosen(ask_who, player, "h", objectName(), false, Card::MethodDiscard);
        room->throwCard(card_id, player, ask_who);
        return false;
    }
};

class RhDangmo: public TriggerSkill
{
public:
    RhDangmo(): TriggerSkill("rhdangmo")
    {
        events << TargetSpecified << EventPhaseChanging << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == TargetSpecified) {
            if (TriggerSkill::triggerable(player)) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->isKindOf("Slash"))
                    return QStringList(objectName());
            }
            return QStringList();
        }
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || player != room->getCurrent())
                return QStringList();
        }
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("rhdangmo") == 0)
                continue;
            room->removePlayerMark(p, "@skill_invalidity", p->getMark("rhdangmo"));
            p->setMark("rhdangmo", 0);

            foreach (ServerPlayer *pl, room->getAllPlayers())
                room->filterCards(pl, pl->getCards("he"), false);
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> tos;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive())
                break;
            if (!p->isAlive())
                continue;
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());
                if (!tos.contains(p)) {
                    p->addMark("rhdangmo");
                    room->addPlayerMark(p, "@skill_invalidity");
                    tos << p;

                    foreach (ServerPlayer *pl, room->getAllPlayers())
                        room->filterCards(pl, pl->getCards("he"), true);
                    JsonArray args;
                    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                }
            }
        }
        return false;
    }
};

class RhBumo : public TriggerSkill
{
public:
    RhBumo() : TriggerSkill("rhbumo")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)
                || (room->getCurrent() == player && player->getPhase() != Player::NotActive))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        int n = 0;
        for (int i = 0; i < move.card_ids.length(); ++i) {
            if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)
                ++n;
        }
        player->drawCards(n, objectName());
        return false;
    }
};

class RhNieji : public TriggerSkill
{
public:
    RhNieji() : TriggerSkill("rhnieji")
    {
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who->getHp() > 0 || dying.who->isDead())
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            room->removeReihouCard(player);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DyingStruct dying = data.value<DyingStruct>();

        if (!player->faceUp())
            player->turnOver();

        if (player->isChained())
            room->setPlayerProperty(player, "chained", false);

        room->recover(dying.who, RecoverStruct(player, NULL, 1 - dying.who->getHp()));
        return false;
    }
};

class RhYuning : public TriggerSkill
{
public:
    RhYuning() : TriggerSkill("rhyuning")
    {
        events << EventAcquireSkill << EventLoseSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (data.toString() == objectName()) {
            if (triggerEvent == EventLoseSkill) {
                ServerPlayer *p = player->tag[objectName()].value<ServerPlayer *>();
                player->tag.remove(objectName());
                if (p)
                    room->setPlayerMark(p, objectName(), 0);
            } else
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@rhyuning", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag[objectName()] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *p = player->tag[objectName()].value<ServerPlayer *>();
        if (p)
            room->setPlayerMark(p, objectName(), 1);
        return false;
    }
};

class RhYuningRecover : public TriggerSkill
{
public:
    RhYuningRecover() : TriggerSkill("#rhyuning")
    {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->isAlive()) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->isAlive() && p->getMark("rhyuning") > 0 && p->isWounded())
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        room->recover(ask_who, RecoverStruct(ask_who));
        return false;
    }
};

RhHuayuCard::RhHuayuCard()
{
}

bool RhHuayuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) {
        return false;
    }
    QList<const DelayedTrick *> cards = Sanguosha->findChildren<const DelayedTrick *>();
    foreach (const Card *c, cards) {
        if (c->isKindOf("DelayedTrick")) {
            Card *trick = Sanguosha->cloneCard(c->objectName());
            trick->addSubcard(this);
            trick->setSkillName("rhhuayu");
            if (Self->isCardLimited(trick, MethodUse, true)) {
                delete trick;
                continue;
            }
            if (trick->isKindOf("Lightning")) {
                delete trick;
                return false;
            }
            bool can_use = trick->targetFilter(targets, to_select, Self);
            delete trick;
            if (can_use)
                return true;
        }
    }
    return false;
}

bool RhHuayuCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (targets.length() == 0) {
        Card *trick = Sanguosha->cloneCard("lightning");
        trick->addSubcard(this);
        trick->setSkillName("rhhuayu");
        bool can_use = !Self->containsTrick("lightning") && !Self->isProhibited(Self, trick);
        delete trick;
        return can_use;
    }
    if (targets.length() == 1) {
        QList<const DelayedTrick *> cards = Sanguosha->findChildren<const DelayedTrick *>();
        foreach (const Card *c, cards) {
            if (c->isKindOf("DelayedTrick") && !c->isKindOf("Lightning")) {
                Card *trick = Sanguosha->cloneCard(c->objectName());
                trick->addSubcard(this);
                trick->setSkillName("rhhuayu");
                bool can_use = trick->targetsFeasible(targets, Self);
                delete trick;
                if (can_use)
                    return true;
            }
        }
    }
    return false;
}

const Card *RhHuayuCard::validate(CardUseStruct &card_use) const
{
    Room *room = card_use.from->getRoom();
    if (card_use.to.isEmpty()) {
        Lightning *lightning = new Lightning(SuitToBeDecided, -1);
        lightning->addSubcard(this);
        lightning->setSkillName("rhhuayu");
        room->addPlayerMark(card_use.from, "rhhuayu");
        return lightning;
    }
    QStringList choices;
    ServerPlayer *target = card_use.to.first();
    QList<const DelayedTrick *> cards = Sanguosha->findChildren<const DelayedTrick *>();
    foreach (const Card *c, cards) {
        if (!c->isKindOf("Lightning")
                && !choices.contains(c->objectName())
                && !target->containsTrick(c->objectName())) {
            Card *trick = Sanguosha->cloneCard(c->objectName());
            trick->addSubcard(this);
            trick->setSkillName("rhhuayu");
            bool is_prohibit = card_use.from->isProhibited(target, trick);
            delete trick;
            if (!is_prohibit)
                choices << c->objectName();
        }
    }
    Q_ASSERT(!choices.isEmpty());
    QString choice = room->askForChoice(card_use.from, "rhhuayu", choices.join("+"));
    Card *trick = Sanguosha->cloneCard(choice);
    trick->addSubcard(this);
    trick->setSkillName("rhhuayu");
    room->addPlayerMark(card_use.from, "rhhuayu");
    return trick;
}

class RhHuayuVS : public OneCardViewAsSkill
{
public:
    RhHuayuVS() : OneCardViewAsSkill("rhhuayu")
    {
        response_pattern = "@@rhhuayu";
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new RhHuayuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class RhHuayu : public TriggerSkill
{
public:
    RhHuayu() : TriggerSkill("rhhuayu")
    {
        events << TargetSpecified << TargetConfirmed << EventPhaseChanging;
        view_as_skill = new RhHuayuVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
            return QStringList();
        }
        if (!TriggerSkill::triggerable(player) || player->getMark(objectName()) > 0)
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified || (triggerEvent == TargetConfirmed && use.to.contains(player))) {
            if (use.card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@rhhuayu", "@rhhuayu");
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
    reihou001->addSkill(new SlashNoDistanceLimitSkill("rhpohuang"));
    related_skills.insertMulti("rhpohuang", "#rhpohuang-slash-ndl");

    General *reihou002 = new General(this, "reihou002", "rei", 4, true, true);
    reihou002->addSkill(new RhRuyi);

    General *reihou003 = new General(this, "reihou003", "rei", 4, true, true);
    reihou003->addSkill(new RhHuanjing);

    General *reihou004 = new General(this, "reihou004", "rei", 4, true, true);
    reihou004->addSkill(new RhLvcao);
    reihou004->addSkill(new RhLvcaoTargetMod);
    related_skills.insertMulti("rhlvcao", "#rhlvcao");

    General *reihou005 = new General(this, "reihou005", "rei", 4, true, true);
    reihou005->addSkill(new RhLiufu);
    reihou005->addSkill(new RhLiufuGet);
    related_skills.insertMulti("rhliufu", "#rhliufu");

    General *reihou006 = new General(this, "reihou006", "rei", 4, true, true);
    reihou006->addSkill(new RhPujiu);
    reihou006->addSkill(new RhPujiuTrigger);
    related_skills.insertMulti("rhpujiu", "#rhpujiu");

    General *reihou007 = new General(this, "reihou007", "rei", 4, true, true);
    reihou007->addSkill(new RhXuesha);
    reihou007->addSkill(new RhXueshaTargetMod);
    related_skills.insertMulti("rhxuesha", "#rhxuesha");

    General *reihou008 = new General(this, "reihou008", "rei", 4, true, true);
    reihou008->addSkill(new RhZhenyao);
    reihou008->addSkill(new RhZhenyaoPrevent);
    related_skills.insertMulti("rhzhenyao", "#rhzhenyao");

    General *reihou009 = new General(this, "reihou009", "rei", 4, true, true);
    reihou009->addSkill(new RhGaiming);
    reihou009->addSkill(new RhXusheng);

    General *reihou010 = new General(this, "reihou010", "rei", 4, true, true);
    reihou010->addSkill(new RhBaiming);

    General *reihou011 = new General(this, "reihou011", "rei", 4, true, true);
    reihou011->addSkill(new RhChengfeng);
    reihou011->addSkill(new RhYuhuo);

    General *reihou012 = new General(this, "reihou012", "rei", 4, true, true);
    reihou012->addSkill(new RhZhengyang);
    reihou012->addSkill(new RhZhengyangTrigger);
    reihou012->addSkill(new RhZhengyangProhibit);
    related_skills.insertMulti("rhzhengyang", "#rhzhengyang");
    related_skills.insertMulti("rhzhengyang", "#rhzhengyang-prohibit");

    General *reihou013 = new General(this, "reihou013", "rei", 4, true, true);
    reihou013->addSkill(new RhChunyin);

    General *reihou014 = new General(this, "reihou014", "rei", 4, true, true);
    reihou014->addSkill(new RhSanmei);

    General *reihou015 = new General(this, "reihou015", "rei", 4, true, true);
    reihou015->addSkill(new RhXuanren);

    General *reihou016 = new General(this, "reihou016", "rei", 4, true, true);
    reihou016->addSkill(new RhGuozao);

    General *reihou017 = new General(this, "reihou017", "rei", 4, true, true);
    reihou017->addSkill(new RhShenguang);

    General *reihou018 = new General(this, "reihou018", "rei", 4, true, true);
    reihou018->addSkill(new RhMangti);
    reihou018->addSkill(new RhLingwei);

    General *reihou019 = new General(this, "reihou019", "rei", 4, true, true);
    reihou019->addSkill(new RhWangzhong);

    General *reihou020 = new General(this, "reihou020", "rei", 4, true, true);
    reihou020->addSkill(new RhCuigu);
    reihou020->addSkill(new RhCuiguProhibit);
    related_skills.insertMulti("rhcuigu", "#rhcuigu");
    reihou020->addSkill(new RhLinghai);

    General *reihou021 = new General(this, "reihou021", "rei", 4, true, true);
    reihou021->addSkill(new RhHaoqiang);
    reihou021->addSkill(new RhLiedan);

    General *reihou022 = new General(this, "reihou022", "rei", 4, true, true);
    reihou022->addSkill(new RhYaren);
    reihou022->addSkill(new RhShixiang);

    General *reihou023 = new General(this, "reihou023", "rei", 4, true, true);
    reihou023->addSkill(new RhYinren);
    reihou023->addSkill(new RhYinrenTrigger);
    reihou023->addSkill(new SlashNoDistanceLimitSkill("rhyinren"));
    related_skills.insertMulti("rhyinren", "#rhyinren");
    related_skills.insertMulti("rhyinren", "#rhyinren-slash-ndl");
    reihou023->addSkill(new RhYeming);

    General *reihou024 = new General(this, "reihou024", "rei", 4, true, true);
    reihou024->addSkill(new RhYiqie);
    reihou024->addSkill(new RhYaozhang);

    General *reihou025 = new General(this, "reihou025", "rei", 4, true, true);
    reihou025->addSkill(new RhSanglv);

    General *reihou026 = new General(this, "reihou026", "rei", 4, true, true);
    reihou026->addSkill(new RhWuyin);

    General *reihou027 = new General(this, "reihou027", "rei", 4, true, true);
    reihou027->addSkill(new RhChanling);

    General *reihou028 = new General(this, "reihou028", "rei", 4, true, true);
    reihou028->addSkill(new RhShendai);

    General *reihou029 = new General(this, "reihou029", "rei", 4, true, true);
    reihou029->addSkill(new RhJiyu);

    General *reihou030 = new General(this, "reihou030", "rei", 4, true, true);
    reihou030->addSkill(new RhFuyin);

    General *reihou031 = new General(this, "reihou031", "rei", 4, true, true);
    reihou031->addSkill(new RhPihuai);

    General *reihou032 = new General(this, "reihou032", "rei", 4, true, true);
    reihou032->addSkill(new RhDangmo);
    reihou032->addSkill(new RhBumo);

    General *reihou033 = new General(this, "reihou033", "rei", 4, true, true);
    reihou033->addSkill(new RhNieji);

    General *reihou034 = new General(this, "reihou034", "rei", 4, true, true);
    reihou034->addSkill(new RhYuning);
    reihou034->addSkill(new RhYuningRecover);
    related_skills.insertMulti("rhyuning", "#rhyuning");

    General *reihou035 = new General(this, "reihou035", "rei", 4, true, true);
    reihou035->addSkill(new RhHuayu);

    addMetaObject<RhDuanlongCard>();
    addMetaObject<RhRuyiCard>();
    addMetaObject<RhHuanjingCard>();
    addMetaObject<RhPujiuCard>();
    addMetaObject<RhGaimingCard>();
    addMetaObject<RhXuanrenCard>();
    addMetaObject<RhHaoqiangCard>();
    addMetaObject<RhYarenCard>();
    addMetaObject<RhYinrenCard>();
    addMetaObject<RhHuayuCard>();
}

ADD_PACKAGE(TenshiReihou)
