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
            room->removeReihouCard(player);

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
                log.arg = use.card->objectName();
                room->sendLog(log);

                use.nullified_list << player->objectName();
                data = QVariant::fromValue(use);
            } else if (choice == "give") {
                LogMessage log;
                log.type = "#RhLiufu2";
                log.from = player;
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

class RhPujiuVS : public OneCardViewAsSkill
{
public:
    RhPujiuVS() : OneCardViewAsSkill("rhpujiu")
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

class RhPujiu : public TriggerSkill
{
public:
    RhPujiu() : TriggerSkill("rhpujiu")
    {
        events << CardUsed << EventLoseSkill;
        frequency = NotCompulsory;
        view_as_skill = new RhPujiuVS;
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
            if (TriggerSkill::triggerable(player) && player->getMark("ikguijing") > 1 && player->isWounded())
                return QStringList(objectName());
        } else if (triggerEvent == TargetSpecified) {
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

    General *reihou007 = new General(this, "reihou007", "rei", 4, true, true);
    reihou007->addSkill(new RhXuesha);
    reihou007->addSkill(new RhXueshaTargetMod);
    related_skills.insertMulti("rhxuesha", "#rhxuesha");

    addMetaObject<RhDuanlongCard>();
    addMetaObject<RhRuyiCard>();
    addMetaObject<RhHuanjingCard>();
    addMetaObject<RhPujiuCard>();
}

ADD_PACKAGE(TenshiReihou)
