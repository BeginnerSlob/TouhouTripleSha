#include "tenshi-reihou.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "client.h"
#include "maneuvering.h"
#include "fantasy.h"

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

RhHuanjieCard::RhHuanjieCard()
{
    mute = true;
}

bool RhHuanjieCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(Self->property("rhhuanjie").toString());
        card->setSkillName("rhhuanjie");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *new_card = Sanguosha->cloneCard(Self->property("rhhuanjie").toString());
    new_card->setSkillName("rhhuanjie");
    new_card->addSubcard(this);
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetFixed();
}

bool RhHuanjieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(Self->property("rhhuanjie").toString());
        card->setSkillName("rhhuanjie");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *new_card = Sanguosha->cloneCard(Self->property("rhhuanjie").toString());
    new_card->setSkillName("rhhuanjie");
    new_card->addSubcard(this);
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool RhHuanjieCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = Sanguosha->cloneCard(Self->property("rhhuanjie").toString());
        card->setSkillName("rhhuanjie");
        card->addSubcard(this);
        card->deleteLater();
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *new_card = Sanguosha->cloneCard(Self->property("rhhuanjie").toString());
    new_card->setSkillName("rhhuanjie");
    new_card->addSubcard(this);
    new_card->setCanRecast(false);
    new_card->deleteLater();
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *RhHuanjieCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *user = card_use.from;
    Room *room = user->getRoom();

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, user->objectName(), "rhhuanjie", QString());
    room->throwCard(this, reason, user);

    room->addPlayerMark(user, "rhhuanjie");

    Card *use_card = Sanguosha->cloneCard(user->property("rhhuanjie").toString());
    use_card->setSkillName("rhhuanjie");
    use_card->deleteLater();
    return use_card;
}

const Card *RhHuanjieCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, user->objectName(), "rhhuanjie", QString());
    room->throwCard(this, reason, user);

    room->addPlayerMark(user, "rhhuanjie");

    Card *use_card = Sanguosha->cloneCard(user->property("rhhuanjie").toString());
    use_card->setSkillName("rhhuanjie");
    use_card->deleteLater();
    return use_card;
}

class RhHuanjieVS : public OneCardViewAsSkill
{
public:
    RhHuanjieVS() : OneCardViewAsSkill("rhhuanjie")
    {
        filter_pattern = ".|spade!";
    }

    virtual Card *viewAs(const Card *originalCard) const
    {
        Card *card = new RhHuanjieCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("rhhuanjie") > 0)
            return false;
        if (!player->canDiscard(player, "he"))
            return false;
        QString obj = player->property("rhhuanjie").toString();
        if (obj.isEmpty())
            return false;
        const Card *card = Sanguosha->cloneCard(obj);
        return card->isAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
        if (player->getMark("rhhuanjie") > 0)
            return false;
        if (!player->canDiscard(player, "he"))
            return false;
        QString obj = player->property("rhhuanjie").toString();
        if (obj.isEmpty())
            return false;
        return pattern.contains(obj);
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->getMark("rhhuanjie") > 0)
            return false;
        if (!player->canDiscard(player, "he"))
            return false;
        QString obj = player->property("rhhuanjie").toString();
        return obj == "nullification";
    }
};

class RhHuanjie : public TriggerSkill
{
public:
    RhHuanjie() : TriggerSkill("rhhuanjie")
    {
        events << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
        view_as_skill = new RhHuanjieVS;
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
                room->setPlayerProperty(player, "rhhuanjie", QVariant());
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
        log.type = "#RhHuanjie";
        log.from = player;
        log.arg = obj_n;
        room->sendLog(log);
        room->setPlayerProperty(player, "rhhuanjie", obj_n);

        return false;
    }
};

class RhHonghuang : public ProhibitSkill
{
public:
    RhHonghuang() : ProhibitSkill("rhhonghuang")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return from->getPhase() != Player::NotActive && to->hasSkill(objectName())
                && card->isKindOf("TrickCard") && card->isBlack();
    }
};

class RhHonghuangTargetMod : public TargetModSkill
{
public:
    RhHonghuangTargetMod() : TargetModSkill("#rhhonghuang")
    {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        bool find_skill = false;
        if (from->hasSkill("rhhonghuang"))
            find_skill = true;
        else {
            QList<const Player *> players = from->getAliveSiblings();
            foreach (const Player *p, players) {
                if (p->hasSkill("rhhonghuang")) {
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
                room->setTag("rhliufu_" + use.card->toString(), QVariant::fromValue(player));
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
                if (card->getSubcards().contains(id) || card->getEffectiveId() == id)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *card = move.reason.m_extraData.value<const Card *>();
        ServerPlayer *player = room->getTag("rhliufu_" + card->toString()).value<ServerPlayer *>();
        room->removeTag("rhliufu_" + card->toString());
        room->setCardFlag(card, "-rhliufu");
        room->sendCompulsoryTriggerLog(player, "rhliufu");
        QList<int> ids;
        for (int i = 0; i < move.card_ids.length(); ++i) {
            int id = move.card_ids[i];
            if ((card->getSubcards().contains(id) || card->getEffectiveId() == id)
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
                JudgeStruct *judge = data.value<JudgeStruct *>();
                if (judge->card->isBlack()) {
                    CardUseStruct use = player->tag["RhZhenyaoUse"].value<CardUseStruct>();
                    player->tag.remove("RhZhenyaoUse");
                    player->addZhenyaoTag(use.card);
                    room->addPlayerMark(player, "@repression");
                }
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
            player->tag.remove("RhZhenyaoUse");
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
        return player->getMark("@repression") > 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "rhzhenyao");
        room->broadcastSkillInvoke("rhzhenyao");
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
            if (triggerEvent == EventLoseSkill) {
                QString obj = player->tag[objectName()].toString();
                player->tag.remove(objectName());
                if (room->findPlayer(obj))
                    room->setPlayerMark(room->findPlayer(obj), "@filth", 0);
            } else
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
            room->addPlayerMark(target, "@filth");
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
    room->addPlayerMark(effect.from, "rhyinren");

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
            if (use.card->isKindOf("RhYinrenCard") && player->getMark("rhyinren") == 2)
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
        if (ask_who->getHandcardNum() >= n && ask_who != player) {
            const Card *dummy = room->askForExchange(ask_who, objectName(), n, n, false,
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

class RhJinbei : public TriggerSkill
{
public:
    RhJinbei() : TriggerSkill("rhjinbei")
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
        peach->setSkillName("_rhjinbei");
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName("_rhjinbei");
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
                if (p == player)
                    continue;
                if (p->canDiscard(player, "h") && !p->inMyAttackRange(player))
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

        if (!dying.who->faceUp())
            dying.who->turnOver();

        if (dying.who->isChained())
            room->setPlayerProperty(dying.who, "chained", false);

        room->recover(dying.who, RecoverStruct(player, NULL, 1 - dying.who->getHp()));
        return false;
    }
};

class RhShenluo : public TriggerSkill
{
public:
    RhShenluo() : TriggerSkill("rhshenluo")
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
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@rhshenluo", true, true);
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

class RhShenluoRecover : public TriggerSkill
{
public:
    RhShenluoRecover() : TriggerSkill("#rhshenluo")
    {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->isAlive()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->isAlive() && p->getMark("rhshenluo") > 0 && p->isWounded())
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

RhYoushengCard::RhYoushengCard()
{
}

bool RhYoushengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) {
        return false;
    }
    QList<const DelayedTrick *> cards = Sanguosha->findChildren<const DelayedTrick *>();
    foreach (const Card *c, cards) {
        if (c->isKindOf("DelayedTrick")) {
            Card *trick = Sanguosha->cloneCard(c->objectName());
            trick->addSubcard(this);
            trick->setSkillName("rhyousheng");
            if (Self->isCardLimited(trick, MethodUse, true)) {
                delete trick;
                continue;
            }
            if (trick->isKindOf("Lightning")) {
                delete trick;
                return false;
            }
            bool can_use = trick->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, trick);
            delete trick;
            if (can_use)
                return true;
        }
    }
    return false;
}

bool RhYoushengCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (targets.length() == 0) {
        Card *trick = Sanguosha->cloneCard("lightning");
        trick->addSubcard(this);
        trick->setSkillName("rhyousheng");
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
                trick->setSkillName("rhyousheng");
                bool can_use = trick->targetsFeasible(targets, Self);
                delete trick;
                if (can_use)
                    return true;
            }
        }
    }
    return false;
}

const Card *RhYoushengCard::validate(CardUseStruct &card_use) const
{
    Room *room = card_use.from->getRoom();
    if (card_use.to.isEmpty()) {
        Lightning *lightning = new Lightning(SuitToBeDecided, -1);
        lightning->addSubcard(this);
        lightning->setSkillName("rhyousheng");
        room->addPlayerMark(card_use.from, "rhyousheng");
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
            trick->setSkillName("rhyousheng");
            bool is_prohibit = card_use.from->isProhibited(target, trick);
            delete trick;
            if (!is_prohibit)
                choices << c->objectName();
        }
    }
    Q_ASSERT(!choices.isEmpty());
    QString choice = room->askForChoice(card_use.from, "rhyousheng", choices.join("+"));
    Card *trick = Sanguosha->cloneCard(choice);
    trick->addSubcard(this);
    trick->setSkillName("rhyousheng");
    room->addPlayerMark(card_use.from, "rhyousheng");
    return trick;
}

class RhYoushengVS : public OneCardViewAsSkill
{
public:
    RhYoushengVS() : OneCardViewAsSkill("rhyousheng")
    {
        response_pattern = "@@rhyousheng";
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new RhYoushengCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class RhYousheng : public TriggerSkill
{
public:
    RhYousheng() : TriggerSkill("rhyousheng")
    {
        events << TargetSpecified << TargetConfirmed << EventPhaseChanging;
        view_as_skill = new RhYoushengVS;
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
        return room->askForUseCard(player, "@@rhyousheng", "@rhyousheng");
    }
};

class RhJianuo : public TriggerSkill
{
public:
    RhJianuo() : TriggerSkill("rhjianuo")
    {
        events << EventPhaseStart << BeforeCardsMove << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    skill_list.insert(p, QStringList(objectName()));
                }
            }
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && player->tag[objectName()].toString() == move.from->objectName()) {
                if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                        && move.to_place == Player::DiscardPile)
                    skill_list.insert(player, QStringList(objectName()));
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    QString obj = p->tag[objectName()].toString();
                    if (!obj.isEmpty()) {
                        p->tag.remove(objectName());
                        if (room->findPlayer(obj))
                            room->removePlayerMark(room->findPlayer(obj), "@charm");
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
                room->broadcastSkillInvoke(objectName());
                room->removeReihouCard(ask_who);
                return true;
            }
            return false;
        }

        return true;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventPhaseStart) {
            ask_who->tag[objectName()] = player->objectName();
            room->addPlayerMark(player, "@charm");
        }

        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

            QList<int> all_ids = move.card_ids, ids = move.card_ids;
            QList<int> disabled;
            while (!ids.isEmpty()) {
                room->fillAG(all_ids, player, disabled);
                bool only = (all_ids.length() == 1);
                int card_id = -1;
                if (only)
                    card_id = ids.first();
                else
                    card_id = room->askForAG(player, ids, true, objectName());
                room->clearAG(player);
                if (card_id == -1) break;
                const Card *card = Sanguosha->getCard(card_id);
                ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers((ServerPlayer *)move.from), objectName(),
                                                                QString("@ikyanyu-give:::%1:%2\\%3")
                                                                .arg(card->objectName())
                                                                .arg(card->getSuitString() + "_char")
                                                                .arg(card->getNumberString()),
                                                                only, true);
                if (target) {
                    Player::Place place = move.from_places.at(move.card_ids.indexOf(card_id));
                    QList<int> _card_id;
                    _card_id << card_id;
                    move.removeCardIds(_card_id);
                    data = QVariant::fromValue(move);
                    ids.removeOne(card_id);
                    disabled << card_id;
                    if (move.from && move.from->objectName() == target->objectName() && place != Player::PlaceTable) {
                        // just indicate which card she chose...
                        LogMessage log;
                        log.type = "$MoveCard";
                        log.from = target;
                        log.to << target;
                        log.card_str = QString::number(card_id);
                        room->sendLog(log);
                    }
                    room->obtainCard(target, card, move.reason);
                } else
                    break;
            }
        }
        return false;
    }
};

RhYizhiCard::RhYizhiCard()
{
}

bool RhYizhiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (targets.length() >= 2)
        return false;
    if (targets.isEmpty())
        return to_select->hasEquip();
    if (!targets.first()->hasEquip())
        return false;
    foreach (const Card *card, targets.first()->getEquips()) {
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        EquipCard::Location location = equip->location();
        if (!to_select->getEquip(location))
            return true;
    }
    return false;
}

bool RhYizhiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    if (targets.length() == 2) {
        if (!targets.first()->hasEquip())
            return false;
        foreach (const Card *card, targets.first()->getEquips()) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            EquipCard::Location location = equip->location();
            if (!targets.last()->getEquip(location))
                return true;
        }
    }
    return false;
}

void RhYizhiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    QList<ServerPlayer *> targets = card_use.to;
    ServerPlayer *source = card_use.from;

    QList<int> disabled_ids;
    foreach (const Card *card, targets.first()->getEquips()) {
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        EquipCard::Location location = equip->location();
        if (targets.last()->getEquip(location))
            disabled_ids << card->getEffectiveId();
    }

    room->loseHp(source);

    int id = room->askForCardChosen(source, targets.first(), "e", "rhyizhi", false, MethodNone, disabled_ids);
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, source->objectName(), targets.last()->objectName(), "rhyizhi", QString());
    CardsMoveStruct move(id, targets.last(), Player::PlaceEquip, reason);
    room->moveCardsAtomic(move, true);
}

class RhYizhi : public ZeroCardViewAsSkill
{
public:
    RhYizhi() : ZeroCardViewAsSkill("rhyizhi")
    {
        response_pattern = "@@rhyizhi";
    }

    virtual const Card *viewAs() const
    {
        return new RhYizhiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getHp() > 0;
    }
};

class RhGuozhu : public TriggerSkill
{
public:
    RhGuozhu() : TriggerSkill("rhguozhu")
    {
        events << Damaged;
        frequency = Frequent;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->isWounded() && player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->recover(player, RecoverStruct(player));
        }

        if (player->getHp() > 0)
            room->askForUseCard(player, "@@rhyizhi", "@rhyizhi", -1, Card::MethodNone);

        return false;
    }
};

RhShiguangCard::RhShiguangCard()
{
    m_skillName = "rhshiguangv";
    target_fixed = true;
}

void RhShiguangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->loseMark("@time");
    source->drawCards(1, "rhshiguang");
    room->addPlayerMark(source, "rhshiguang");
}

class RhShiguangGivenSkill : public ZeroCardViewAsSkill
{
public:
    RhShiguangGivenSkill() : ZeroCardViewAsSkill("rhshiguangv")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@time") > 0;
    }

    virtual bool shouldBeVisible(const Player *player) const
    {
        return player && player->getMark("@time") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new RhShiguangCard;
    }
};

class RhShiguang : public TriggerSkill
{
public:
    RhShiguang() : TriggerSkill("rhshiguang")
    {
        events << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
        frequency = NotCompulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
            return QStringList();
        }

        if (data.toString() == objectName()) {
            if (triggerEvent == EventAcquireSkill)
                return QStringList(objectName());
            else {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark("@time") > 0)
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        if (triggerEvent == EventAcquireSkill) {
            player->gainMark("@time", 4);
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->attachSkillToPlayer(p, "rhshiguangv");
        } else {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                room->detachSkillFromPlayer(p, "rhshiguangv", true);
                if (p->getMark("@time") > 0)
                    p->loseAllMarks("@time");
            }
        }
        return false;
    }
};

class RhShiguangMaxCards : public MaxCardsSkill
{
public:
    RhShiguangMaxCards() : MaxCardsSkill("#rhshiguang") {
    }

    virtual int getExtra(const Player *target) const
    {
        return -target->getMark("rhshiguang");
    }
};

class RhKuili : public TriggerSkill
{
public:
    RhKuili() : TriggerSkill("rhkuili")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::RoundStart) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->getMark("@time") > 0)
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
        QStringList choices;
        for (int i = 1; i <= ask_who->getMark("@time"); ++i)
            choices << QString::number(i);
        QString choice = room->askForChoice(ask_who, objectName(), choices.join("+"));
        int n = choice.toInt();
        ask_who->loseMark("@time", n);
        player->gainMark("@time", n);
        return false;
    }
};

RhChenshengCard::RhChenshengCard()
{
    target_fixed = true;
}

const Card *RhChenshengCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *target = card_use.from;
    Room *room = target->getRoom();
    room->removeReihouCard(target);
    Card *card = new Drowning(NoSuit, 0);
    card->setSkillName("_rhchensheng");
    return card;
}

class RhChenshengVS : public ZeroCardViewAsSkill
{
public:
    RhChenshengVS() : ZeroCardViewAsSkill("rhchensheng")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        Card *card = new Drowning(Card::NoSuit, 0);
        card->setSkillName("_rhchensheng");
        card->deleteLater();
        return card->isAvailable(player);
    }

    virtual const Card *viewAs() const
    {
        return new RhChenshengCard;
    }
};

class RhChensheng : public TriggerSkill
{
public:
    RhChensheng() : TriggerSkill("rhchensheng")
    {
        events << Damaged;
        view_as_skill = new RhChenshengVS;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->removeReihouCard(player);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        Card *card = new Drowning(Card::NoSuit, 0);
        card->setSkillName("_rhchensheng");
        if (card->isAvailable(player))
            room->useCard(CardUseStruct(card, player, QList<ServerPlayer *>()));
        else
            delete card;
        return false;
    }
};

class RhXinmo : public TriggerSkill
{
public:
    RhXinmo() : TriggerSkill("rhxinmo")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        }

        QStringList skills;
        if (TriggerSkill::triggerable(player) && player->getMark(objectName()) == 0) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && (move.from_places.contains(Player::PlaceHand)
                                        || move.from_places.contains(Player::PlaceEquip))) {
                if (move.to != player || (move.to_place != Player::PlaceHand
                                          && move.to_place != Player::PlaceEquip)) {
                    int index = 0;
                    foreach (int id, move.card_ids) {
                        if (move.from_places[index] == Player::PlaceHand || move.from_places[index] == Player::PlaceEquip) {
                            if (Sanguosha->getCard(id)->isRed())
                                skills << objectName();
                        }
                        ++index;
                    }
                }
            }
        }
        return skills;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@rhxinmo", true, true);
        if (target) {
            player->tag["RhXinmoTarget"] = QVariant::fromValue(target);
            room->broadcastSkillInvoke(objectName());
            room->addPlayerMark(player, objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["RhXinmoTarget"].value<ServerPlayer *>();
        player->tag.remove("RhXinmoTarget");
        if (target) {
            target->drawCards(1, objectName());

            if (!player->faceUp())
                player->turnOver();

            if (player->isChained())
                room->setPlayerProperty(player, "chained", false);
        }

        return false;
    }
};

class RhFuyu : public TriggerSkill
{
public:
    RhFuyu() : TriggerSkill("rhfuyu")
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
        QStringList choices;
        choices << "reduce" << "increase";
        QString choice = room->askForChoice(ask_who, objectName(), choices.join("+"));
        LogMessage log;
        log.type = "#RhFuyu";
        log.from = ask_who;
        log.to << player;
        if (choice == "reduce") {
            room->setPlayerFlag(player, "RhFuyuReduce");
            log.arg = "1";
            log.arg2 = "+1";
        } else if (choice == "increase") {
            room->setPlayerFlag(player, "RhFuyuIncrease");
            log.arg = "2";
            log.arg2 = "-1";
        }
        room->sendLog(log);
        return false;
    }
};

class RhFuyuDistance : public DistanceSkill
{
public:
    RhFuyuDistance() : DistanceSkill("#rhfuyu")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        int correct = 0;
        if (from->hasFlag("RhFuyuReduce"))
            --correct;
        if (from->hasFlag("RhFuyuIncrease"))
            ++correct;
        return correct;
    }
};

class RhYaodao : public TriggerSkill
{
public:
    RhYaodao() : TriggerSkill("rhyaodao")
    {
        events << EventAcquireSkill << EventLoseSkill;
        frequency = NotCompulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        if (data.toString() == objectName()) {
            if (triggerEvent == EventAcquireSkill) {
                return QStringList(objectName());
            } else if (triggerEvent == EventLoseSkill) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!p->tag["Reihou2"].toString().isEmpty()) {
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (triggerEvent == EventLoseSkill) {
            room->sendCompulsoryTriggerLog(player, objectName());
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@rhyaodao", true, true);
        if (target) {
            player->tag["RhYaodaoTarget"] = QVariant::fromValue(target);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (triggerEvent == EventLoseSkill) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->tag["Reihou2"].toString().isEmpty())
                    room->removeReihouCard(p, true);
            }
            return false;
        }

        ServerPlayer *target = player->tag["RhYaodaoTarget"].value<ServerPlayer *>();
        player->tag.remove("RhYaodaoTarget");
        if (target) {
            QStringList skills;
            const Package *reihoupack = Sanguosha->getPackage("tenshi-reihou");
            if (reihoupack) {
                QList<const General *> reihous = reihoupack->findChildren<const General *>();
                const General *reihou = reihous.at(qrand() % reihous.length());
                while (reihou->objectName() == "reihou042")
                    reihou = reihous.at(qrand() % reihous.length());

                LogMessage log;
                log.type = "#RhYaodao";
                log.from = target;
                log.arg = reihou->objectName();
                room->sendLog(log);

                target->tag["Reihou2"] = reihou->objectName();
                foreach (const Skill *skill, reihou->getVisibleSkillList())
                    skills << skill->objectName();
            }
            room->handleAcquireDetachSkills(target, skills, true, true);
        }
        return true;
    }
};

class RhChuilu : public OneCardViewAsSkill
{
public:
    RhChuilu() : OneCardViewAsSkill("rhchuilu")
    {
        filter_pattern = ".|red";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new Peach(originalCard->getSuit(), originalCard->getNumber());
        card->setSkillName(objectName());
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.contains("peach");
    }
};

RhXianmingCard::RhXianmingCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void RhXianmingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, source->objectName(), "rhxianming", QString());
    room->throwCard(this, reason, source);
}

class RhXianmingVS : public OneCardViewAsSkill
{
public:
    RhXianmingVS() : OneCardViewAsSkill("rhxianming")
    {
        expand_pile = "tea";
        filter_pattern = ".|.|.|tea";
        response_pattern = "@@rhxianming";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new RhXianmingCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class RhXianming : public TriggerSkill
{
public:
    RhXianming() : TriggerSkill("rhxianming")
    {
        events << EventAcquireSkill << TargetSpecified;
        view_as_skill = new RhXianmingVS;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventAcquireSkill) {
            if (data.toString() == objectName()) {
                if (!player->isKongcheng())
                    skill_list.insert(player, QStringList(objectName()));
            }
        } else if (triggerEvent == TargetSpecified && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (!p->getPile("tea").isEmpty())
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventAcquireSkill) {
            const Card *dummy = room->askForExchange(ask_who, objectName(), 998, 1, false, "@rhxianming-invoke", true);
            if (dummy) {
                room->sendCompulsoryTriggerLog(ask_who, objectName());
                room->broadcastSkillInvoke(objectName());
                ask_who->addToPile("tea", dummy, true);
                delete dummy;
            }
        } else if (triggerEvent == TargetSpecified)
            return room->askForUseCard(ask_who, "@@rhxianming", "@rhxianming", -1, Card::MethodNone);

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.m_addHistory) {
            room->addPlayerHistory(player, use.card->getClassName(), -1);
            use.m_addHistory = false;
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class RhXianmingClear : public TriggerSkill
{
public:
    RhXianmingClear() : TriggerSkill("#rhxianming")
    {
        events << EventLoseSkill;
        frequency = NotCompulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (data.toString() == "rhxianming") {
            if (!player->getPile("tea").isEmpty())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "rhxianming");
        room->broadcastSkillInvoke(objectName());
        player->clearOnePrivatePile("tea");
        return false;
    }
};

RhXiaozhangCard::RhXiaozhangCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = MethodNone;
}

void RhXiaozhangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    LogMessage log;
    log.type = "$MoveCard";
    log.from = source;
    log.to << source;
    log.card_str = IntList2StringList(getSubcards()).join("+");
    room->sendLog(log);
    room->obtainCard(source, this);
}

class RhXiaozhangVS : public ViewAsSkill
{
public:
    RhXiaozhangVS() : ViewAsSkill("rhxiaozhang")
    {
        response_pattern = "@@rhxiaozhang";
        expand_pile = "soil";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && Self->getPile("soil").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (!cards.isEmpty()) {
            Card *card = new RhXiaozhangCard;
            card->addSubcards(cards);
            return card;
        }
        return NULL;
    }
};

class RhXiaozhang : public TriggerSkill
{
public:
    RhXiaozhang() : TriggerSkill("rhxiaozhang")
    {
        events << EventAcquireSkill << EventLoseSkill;
        frequency = NotCompulsory;
        view_as_skill = new RhXiaozhangVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (data.toString() == objectName()) {
            if (triggerEvent == EventLoseSkill && player->getPile("soil").isEmpty())
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        if (triggerEvent == EventAcquireSkill)
            player->addToPile("soil", room->getNCards(room->alivePlayerCount()));
        else if (triggerEvent == EventLoseSkill) {
            room->askForUseCard(player, "@@rhxiaozhang", "@rhxiaozhang-get", -1, Card::MethodNone);
            player->clearOnePrivatePile("soil");
        }
        return false;
    }
};

class RhXiaozhangTrigger : public TriggerSkill
{
public:
    RhXiaozhangTrigger() : TriggerSkill("#rhxiaozhang")
    {
        events << EventPhaseStart;
        global = true;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Finish && player->isAlive() && player->canDiscard(player, "he")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName("rhxiaozhang")) {
                if (!p->getPile("soil").isEmpty())
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (room->askForCard(player, "..", "@rhxiaozhang:" + ask_who->objectName(), data, "rhxiaozhang")) {
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = player;
            log.to << ask_who;
            log.arg = "rhxiaozhang";
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        QList<int> ids = ask_who->getPile("soil");
        room->fillAG(ids, player);
        int id = room->askForAG(player, ids, false, "rhxiaozhang");
        room->clearAG(player);
        room->obtainCard(player, id);
        return false;
    }
};

class RhFapo : public TriggerSkill
{
public:
    RhFapo() : TriggerSkill("rhfapo")
    {
        events << PreCardUsed << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            room->setPlayerMark(player, objectName(), 0);
            return QStringList();
        }

        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play
                && player->getMark(objectName()) == 0) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("Slash")) {
                QMap<ServerPlayer *, int> hands;
                foreach (int id, use.card->getSubcards()) {
                    ServerPlayer *owner = room->getCardOwner(id);
                    if (owner && room->getCardPlace(id) == Player::PlaceHand) {
                        if (hands[owner])
                            ++ hands[owner];
                        else
                            hands[owner] = 1;
                    }
                }
                int num = player->getHandcardNum();
                if (hands[player])
                    num -= hands[player];
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (use.to.contains(p) || !player->canSlash(p, use.card))
                        continue;
                    int hand = p->getHandcardNum();
                    if (hands[p])
                        hand -= hands[p];
                    if (num < hand)
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QList<ServerPlayer *> victims;
        CardUseStruct use = data.value<CardUseStruct>();
        QMap<ServerPlayer *, int> hands;
        foreach (int id, use.card->getSubcards()) {
            ServerPlayer *owner = room->getCardOwner(id);
            if (owner && room->getCardPlace(id) == Player::PlaceHand) {
                if (hands[owner])
                    ++ hands[owner];
                else
                    hands[owner] = 1;
            }
        }
        int num = player->getHandcardNum();
        if (hands[player])
            num -= hands[player];
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (use.to.contains(p) || !player->canSlash(p, use.card))
                continue;
            int hand = p->getHandcardNum();
            if (hands[p])
                hand -= hands[p];
            if (num < hand)
                victims << p;
        }

        while (!victims.isEmpty()) {
            ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName(), "@rhfapo", true, true);
            if (victim) {
                room->broadcastSkillInvoke(objectName());
                LogMessage log;
                log.type = "#BecomeTarget";
                log.from = victim;
                log.card_str = use.card->toString();
                room->sendLog(log);

                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), victim->objectName());
                use.to << victim;
                room->sortByActionOrder(use.to);
                data = QVariant::fromValue(use);
                victims.removeOne(victim);
                room->addPlayerMark(player, objectName());
            } else
                break;
        }

        return false;
    }
};

class RhHujuan : public TriggerSkill
{
public:
    RhHujuan() : TriggerSkill("rhhujuan")
    {
        events << DamageForseen << PreHpLost;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        return true;
    }
};

class RhDanshen : public TriggerSkill
{
public:
    RhDanshen() : TriggerSkill("rhdanshen")
    {
        events << Damage;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (player->isAlive()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isAlive() && damage.to->getHp() < player->getHp()) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            ask_who->drawCards(1, objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (ask_who != player && !ask_who->isNude()) {
            const Card *card = room->askForCard(ask_who, "..", "@rhdanshen:" + player->objectName(), data, Card::MethodNone);
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, ask_who->objectName(), player->objectName(), objectName(), QString());
            room->obtainCard(player, card, reason, false);
        }
        return false;
    }
};

class RhZhangchi : public TriggerSkill
{
public:
    RhZhangchi() : TriggerSkill("rhzhangchi")
    {
        events << EventAcquireSkill << EventLoseSkill << ChoiceMade;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == ChoiceMade) {
            if (player->hasFlag("RhZhangchiUsed") && data.canConvert<CardUseStruct>()) {
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(player, objectName());

                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                room->setPlayerFlag(player, "-RhZhangchiUsed");
            }
            return QStringList();
        }

        if (data.toString() == objectName()) {
            if (triggerEvent == EventLoseSkill) {
                ServerPlayer *p = player->tag[objectName()].value<ServerPlayer *>();
                player->tag.remove(objectName());
                if (p) {
                    room->setPlayerMark(p, "@knot", 0);
                    room->detachSkillFromPlayer(p, "rhzhangchiv", true);
                }
            } else
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@rhzhangchi", true, true);
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
        if (p) {
            room->setPlayerMark(p, "@knot", 1);
            room->attachSkillToPlayer(p, "rhzhangchiv");
        }
        return false;
    }
};

class RhZhangchiProhibit : public ProhibitSkill
{
public:
    RhZhangchiProhibit() : ProhibitSkill("#rhzhangchi")
    {
        frequency = NotCompulsory;
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->getMark("@knot") && to->isAdjacentTo(from) && card->getTypeId() != Card::TypeSkill;
    }
};

RhZhangchiCard::RhZhangchiCard()
{
    target_fixed = true;
    mute = true;
    m_skillName = "rhzhangchiv";
}

const Card *RhZhangchiCard::validate(CardUseStruct &card_use) const{
    ServerPlayer *player = card_use.from;
    Room *room = player->getRoom();
    Sanguosha->currentRoomState()->setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_PLAY); // for slash
    QString pattern = "^Jink+^Nullification";
    if (!Slash::IsAvailable(player))
        pattern.append("+^Slash");
    if (!Analeptic::IsAvailable(player))
        pattern.append("+^Analeptic");
    room->setPlayerFlag(player, "RhZhangchiUsed");
    bool use = room->askForUseCard(player, pattern, "@rhzhangchi-use");
    if (!use) {
        room->setPlayerFlag(player, "Global_RhZhangchiFailed");
        room->setPlayerFlag(player, "-RhZhangchiUsed");
        return NULL;
    }
    return this;
}

const Card *RhZhangchiCard::validateInResponse(ServerPlayer *player) const{
    Room *room = player->getRoom();
    Sanguosha->currentRoomState()->setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_PLAY); // for slash
    QString pattern = "^Jink+^Nullification";
    if (!Slash::IsAvailable(player))
        pattern.append("+^Slash");
    if (!Analeptic::IsAvailable(player))
        pattern.append("+^Analeptic");
    room->setPlayerFlag(player, "RhZhangchiUsed");
    bool use = room->askForUseCard(player, pattern, "@rhzhangchi-use");
    if (!use) {
        room->setPlayerFlag(player, "Global_RhZhangchiFailed");
        room->setPlayerFlag(player, "-RhZhangchiUsed");
        return NULL;
    }
    return this;
}

void RhZhangchiCard::onUse(Room *, const CardUseStruct &) const{
    // do nothing
}

class RhZhangchiVS : public ZeroCardViewAsSkill
{
public:
    RhZhangchiVS() : ZeroCardViewAsSkill("rhzhangchiv")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@knot") > 0 && !player->hasFlag("Global_RhZhangchiFailed");
    }

    virtual const Card *viewAs() const{
        return new RhZhangchiCard;
    }
};

class RhQiaozouVS : public OneCardViewAsSkill
{
public:
    RhQiaozouVS() : OneCardViewAsSkill("rhqiaozou")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
        response_pattern = "@@rhqiaozou";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        QString obj_name = Self->property("rhqiaozou").toString();
        Card *card = Sanguosha->cloneCard(obj_name);
        if (card) {
            card->addSubcard(originalCard);
            card->setSkillName(objectName());
            return card;
        }
        return NULL;
    }
};

class RhQiaozou : public TriggerSkill
{
public:
    RhQiaozou() : TriggerSkill("rhqiaozou")
    {
        events << CardFinished << EventPhaseChanging;
        view_as_skill = new RhQiaozouVS;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        } else {
            if (player->isAlive() && room->getCurrent() && room->getCurrent() == player && player->getPhase() != Player::NotActive) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->getTypeId() != Card::TypeSkill && use.card->getTypeId() != Card::TypeEquip) {
                    Card *card = Sanguosha->cloneCard(use.card);
                    card->deleteLater();
                    if (card->isAvailable(player)) {
                        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                            if (p->getMark(objectName()) > 0)
                                continue;
                            skill_list.insert(p, QStringList(objectName()));
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
            room->addPlayerMark(ask_who, objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->setPlayerProperty(player, "rhqiaozou", use.card->objectName());
        room->askForUseCard(player, "@@rhqiaozou",
                            QString("@rhqiaozou:%1::%2")
                            .arg(ask_who->objectName())
                            .arg(use.card->objectName()));
        room->setPlayerProperty(player, "rhqiaozou", QVariant());
        return false;
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
    reihou003->addSkill(new RhHuanjie);

    General *reihou004 = new General(this, "reihou004", "rei", 4, true, true);
    reihou004->addSkill(new RhHonghuang);
    reihou004->addSkill(new RhHonghuangTargetMod);
    related_skills.insertMulti("rhhonghuang", "#rhhonghuang");

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
    reihou030->addSkill(new RhJinbei);

    General *reihou031 = new General(this, "reihou031", "rei", 4, true, true);
    reihou031->addSkill(new RhPihuai);

    General *reihou032 = new General(this, "reihou032", "rei", 4, true, true);
    reihou032->addSkill(new RhDangmo);
    reihou032->addSkill(new RhBumo);

    General *reihou033 = new General(this, "reihou033", "rei", 4, true, true);
    reihou033->addSkill(new RhNieji);

    General *reihou034 = new General(this, "reihou034", "rei", 4, true, true);
    reihou034->addSkill(new RhShenluo);
    reihou034->addSkill(new RhShenluoRecover);
    related_skills.insertMulti("rhshenluo", "#rhshenluo");

    General *reihou035 = new General(this, "reihou035", "rei", 4, true, true);
    reihou035->addSkill(new RhYousheng);

    General *reihou036 = new General(this, "reihou036", "rei", 4, true, true);
    reihou036->addSkill(new RhJianuo);

    General *reihou037 = new General(this, "reihou037", "rei", 4, true, true);
    reihou037->addSkill(new RhYizhi);
    reihou037->addSkill(new RhGuozhu);

    General *reihou038 = new General(this, "reihou038", "rei", 4, true, true);
    reihou038->addSkill(new RhShiguang);
    reihou038->addSkill(new RhShiguangMaxCards);
    related_skills.insertMulti("rhshiguang", "#rhshiguang");
    reihou038->addSkill(new RhKuili);

    General *reihou039 = new General(this, "reihou039", "rei", 4, true, true);
    reihou039->addSkill(new RhChensheng);

    General *reihou040 = new General(this, "reihou040", "rei", 4, true, true);
    reihou040->addSkill(new RhXinmo);

    General *reihou041 = new General(this, "reihou041", "rei", 4, true, true);
    reihou041->addSkill(new RhFuyu);
    reihou041->addSkill(new RhFuyuDistance);
    related_skills.insertMulti("rhfuyu", "#rhfuyu");

    General *reihou042 = new General(this, "reihou042", "rei", 4, true, true);
    reihou042->addSkill(new RhYaodao);

    General *reihou043 = new General(this, "reihou043", "rei", 4, true, true);
    reihou043->addSkill(new RhChuilu);

    General *reihou044 = new General(this, "reihou044", "rei", 4, true, true);
    reihou044->addSkill(new RhXianming);
    reihou044->addSkill(new RhXianmingClear);
    related_skills.insertMulti("rhxianming", "#rhxianming");

    General *reihou045 = new General(this, "reihou045", "rei", 4, true, true);
    reihou045->addSkill(new RhXiaozhang);
    reihou045->addSkill(new RhXiaozhangTrigger);
    related_skills.insertMulti("rhxiaozhang", "#rhxiaozhang");

    General *reihou046 = new General(this, "reihou046", "rei", 4, true, true);
    reihou046->addSkill(new RhFapo);
    reihou046->addSkill(new RhHujuan);

    General *reihou047 = new General(this, "reihou047", "rei", 4, true, true);
    reihou047->addSkill(new RhDanshen);

    General *reihou048 = new General(this, "reihou048", "rei", 4, true, true);
    reihou048->addSkill(new RhZhangchi);
    reihou048->addSkill(new RhZhangchiProhibit);
    related_skills.insertMulti("rhzhangchi", "#rhzhangchi");

    General *reihou049 = new General(this, "reihou049", "rei", 4, true, true);
    reihou049->addSkill(new RhQiaozou);

    addMetaObject<RhDuanlongCard>();
    addMetaObject<RhRuyiCard>();
    addMetaObject<RhHuanjieCard>();
    addMetaObject<RhPujiuCard>();
    addMetaObject<RhGaimingCard>();
    addMetaObject<RhXuanrenCard>();
    addMetaObject<RhHaoqiangCard>();
    addMetaObject<RhYarenCard>();
    addMetaObject<RhYinrenCard>();
    addMetaObject<RhYoushengCard>();
    addMetaObject<RhYizhiCard>();
    addMetaObject<RhShiguangCard>();
    addMetaObject<RhChenshengCard>();
    addMetaObject<RhXianmingCard>();
    addMetaObject<RhXiaozhangCard>();
    addMetaObject<RhZhangchiCard>();

    skills << new RhShiguangGivenSkill << new RhZhangchiVS;
}

ADD_PACKAGE(TenshiReihou)
