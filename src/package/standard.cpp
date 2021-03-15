#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "exppattern.h"
#include "maneuvering.h"
#include "room.h"
#include "roomthread.h"
#include "serverplayer.h"
#include "skill.h"

QString BasicCard::getType() const
{
    return "basic";
}

Card::CardType BasicCard::getTypeId() const
{
    return TypeBasic;
}

bool BasicCard::isAvailable(const Player *player) const
{
    if (player->hasUsed("BasicCard"))
        return false;
    return Card::isAvailable(player);
}

TrickCard::TrickCard(Suit suit, int number)
    : Card(suit, number)
    , cancelable(true)
{
    handling_method = Card::MethodUse;
}

void TrickCard::setCancelable(bool cancelable)
{
    this->cancelable = cancelable;
}

QString TrickCard::getType() const
{
    return "trick";
}

Card::CardType TrickCard::getTypeId() const
{
    return TypeTrick;
}

bool TrickCard::isAvailable(const Player *player) const
{
    if (player->hasUsed("TrickCard"))
        return false;
    return Card::isAvailable(player);
}

bool TrickCard::isCancelable(const CardEffectStruct &effect) const
{
    Q_UNUSED(effect);
    return cancelable;
}

QString EquipCard::getType() const
{
    return "equip";
}

Card::CardType EquipCard::getTypeId() const
{
    return TypeEquip;
}

bool EquipCard::isAvailable(const Player *player) const
{
    if (player->hasUsed("EquipCard"))
        return false;
    if (player->hasFlag("ThChouceUse"))
        return Card::isAvailable(player);
    return !player->isProhibited(player, this) && Card::isAvailable(player);
}

void EquipCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    WrappedCard *wrapped = Sanguosha->getWrappedCard(this->getEffectiveId());
    use.card = wrapped;
    room->setCardFlag(this, "using");

    if (use.from->hasFlag("ThChouceUse")) {
        room->setPlayerFlag(use.from, "-ThChouceUse");
        int n = use.from->getMark("ThChouce");
        if (n > 0)
            room->removePlayerCardLimitation(use.from, "use", QString("^SkillCard|.|~%1$0").arg(n));

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = use.from;
        log.arg = "thchouce";
        room->sendLog(log);

        use.card->setFlags("thchouce_use");
    }

    ServerPlayer *player = use.from;
    if (use.to.isEmpty())
        use.to << player;

    LogMessage log;
    log.from = card_use.from;
    if (!card_use.card->targetFixed() || card_use.to.length() > 1 || !card_use.to.contains(card_use.from))
        log.to = card_use.to;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();
    thread->trigger(PreCardUsed, room, player, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, use.from, data);
}

void EquipCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (targets.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        reason.m_extraData = QVariant::fromValue((const Card *)this);
        room->moveCardTo(this, room->getCardOwner(getEffectiveId()), NULL, Player::DiscardPile, reason, true);
        room->setCardFlag(this, "-using");
    }
    int equipped_id = Card::S_UNKNOWN_CARD_ID;
    ServerPlayer *target = targets.first();
    if (target->getEquip(location()))
        equipped_id = target->getEquip(location())->getEffectiveId();

    QList<CardsMoveStruct> exchangeMove;
    CardMoveReason reason1(CardMoveReason::S_REASON_USE, target->objectName());
    reason1.m_extraData = this;
    CardsMoveStruct move1(getEffectiveId(), target, Player::PlaceEquip, reason1);
    exchangeMove.push_back(move1);
    if (equipped_id != Card::S_UNKNOWN_CARD_ID) {
        CardsMoveStruct move2(equipped_id, NULL, Player::DiscardPile,
                              CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
        exchangeMove.push_back(move2);
    }
    LogMessage log;
    log.from = target;
    log.type = "$Install";
    log.card_str = QString::number(getEffectiveId());
    room->sendLog(log);

    room->moveCardsAtomic(exchangeMove, true);
    setFlags("-using");
}

void EquipCard::onInstall(ServerPlayer *player) const
{
    Room *room = player->getRoom();

    const Skill *skill = Sanguosha->getSkill(this);
    if (skill) {
        if (skill->inherits("ViewAsSkill")) {
            room->attachSkillToPlayer(player, this->objectName());
        } else if (skill->inherits("TriggerSkill")) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            room->getThread()->addTriggerSkill(trigger_skill);
        }
    }
}

void EquipCard::onUninstall(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    if (Sanguosha->getSkill(this) && Sanguosha->getSkill(this)->inherits("ViewAsSkill"))
        room->detachSkillFromPlayer(player, this->objectName(), true);
}

QString GlobalEffect::getSubtype() const
{
    return "global_effect";
}

void GlobalEffect::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    QList<ServerPlayer *> targets;
    if (card_use.to.isEmpty()) {
        QList<ServerPlayer *> all_players = room->getAllPlayers();
        foreach (ServerPlayer *player, all_players) {
            const Skill *skill = room->isProhibited(source, player, this);
            if (skill) {
                if (!skill->isVisible())
                    skill = Sanguosha->getMainSkill(skill->objectName());
                if (skill->isVisible() && player->hasSkill(skill->objectName())) {
                    LogMessage log;
                    log.type = "#SkillAvoid";
                    log.from = player;
                    log.arg = skill->objectName();
                    log.arg2 = objectName();
                    room->sendLog(log);

                    room->broadcastSkillInvoke(skill->objectName());
                }
            } else
                targets << player;
        }
    } else
        targets = card_use.to;

    CardUseStruct use = card_use;
    use.to = targets;
    TrickCard::onUse(room, use);
}

bool GlobalEffect::isAvailable(const Player *player) const
{
    bool canUse = player->hasFlag("ThChouceUse");
    QList<const Player *> players = player->getAliveSiblings();
    players << player;
    foreach (const Player *p, players) {
        if (player->isProhibited(p, this))
            continue;

        canUse = true;
        break;
    }

    return canUse && TrickCard::isAvailable(player);
}

QString AOE::getSubtype() const
{
    return "aoe";
}

bool AOE::isAvailable(const Player *player) const
{
    bool canUse = player->hasFlag("ThChouceUse");
    QList<const Player *> players = player->getAliveSiblings();
    foreach (const Player *p, players) {
        if (player->isProhibited(p, this))
            continue;

        canUse = true;
        break;
    }

    return canUse && TrickCard::isAvailable(player);
}

void AOE::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    QList<ServerPlayer *> targets;
    if (card_use.to.isEmpty()) {
        QList<ServerPlayer *> other_players = room->getOtherPlayers(source);
        foreach (ServerPlayer *player, other_players) {
            const Skill *skill = room->isProhibited(source, player, this);
            if (skill) {
                if (!skill->isVisible())
                    skill = Sanguosha->getMainSkill(skill->objectName());
                if (skill->isVisible() && player->hasSkill(skill->objectName())) {
                    LogMessage log;
                    log.type = "#SkillAvoid";
                    log.from = player;
                    log.arg = skill->objectName();
                    log.arg2 = objectName();
                    room->sendLog(log);

                    room->broadcastSkillInvoke(skill->objectName());
                }
            } else
                targets << player;
        }
    } else
        targets = card_use.to;

    CardUseStruct use = card_use;
    use.to = targets;
    TrickCard::onUse(room, use);
}

QString SingleTargetTrick::getSubtype() const
{
    return "single_target_trick";
}

bool SingleTargetTrick::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty();
    return true;
}

DelayedTrick::DelayedTrick(Suit suit, int number, bool movable)
    : TrickCard(suit, number)
    , movable(movable)
{
    judge.negative = true;
}

void DelayedTrick::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    WrappedCard *wrapped = Sanguosha->getWrappedCard(this->getEffectiveId());
    use.card = wrapped;

    if (use.from->hasFlag("ThChouceUse")) {
        room->setPlayerFlag(use.from, "-ThChouceUse");
        int n = use.from->getMark("ThChouce");
        if (n > 0)
            room->removePlayerCardLimitation(use.from, "use", QString("^SkillCard|.|~%1$0").arg(n));

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = use.from;
        log.arg = "thchouce";
        room->sendLog(log);

        use.card->setFlags("thchouce_use");
    }

    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();
    thread->trigger(PreCardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();

    LogMessage log;
    log.from = use.from;
    log.to = use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    CardMoveReason reason(CardMoveReason::S_REASON_USE, use.from->objectName(), use.to.first()->objectName(),
                          this->getSkillName(), QString());
    reason.m_extraData = QVariant::fromValue((const Card *)this);
    room->moveCardTo(this, use.to.first(), Player::PlaceDelayedTrick, reason, true);

    thread->trigger(CardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, use.from, data);
}

void DelayedTrick::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    if (all_nullified || targets.isEmpty()) {
        if (movable) {
            onNullified(source);
            if (room->getCardOwner(getEffectiveId()) != source)
                return;
        }
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        reason.m_extraData = QVariant::fromValue((const Card *)this);
        room->moveCardTo(this, room->getCardOwner(getEffectiveId()), NULL, Player::DiscardPile, reason, true);
    }
}

QString DelayedTrick::getSubtype() const
{
    return "delayed_trick";
}

void DelayedTrick::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    CardMoveReason reason(CardMoveReason::S_REASON_USE, effect.to->objectName(), getSkillName(), QString());
    reason.m_extraData = QVariant::fromValue((const Card *)this);
    room->moveCardTo(this, NULL, Player::PlaceTable, reason, true);

    LogMessage log;
    log.from = effect.to;
    log.type = "#DelayedTrick";
    log.arg = effect.card->objectName();
    room->sendLog(log);

    JudgeStruct judge_struct = judge;
    judge_struct.who = effect.to;
    room->judge(judge_struct);

    if (judge_struct.negative == judge_struct.isBad()) {
        takeEffect(effect.to);
        if (room->getCardOwner(getEffectiveId()) == NULL) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString());
            room->throwCard(this, reason, NULL);
        }
    } else if (movable) {
        onNullified(effect.to);
    } else {
        if (room->getCardOwner(getEffectiveId()) == NULL) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString());
            room->throwCard(this, reason, NULL);
        }
    }
}

void DelayedTrick::onNullified(ServerPlayer *target) const
{
    Room *room = target->getRoom();
    RoomThread *thread = room->getThread();
    if (movable) {
        ServerPlayer *player = room->findPlayer(target->getNextAlive()->objectName());
        ServerPlayer *p = NULL;

        do {
            if (player->containsTrick(objectName())) {
                player = room->findPlayer(player->getNextAlive()->objectName());
                continue;
            }

            const Skill *skill = room->isProhibited(NULL, player, this);
            if (skill) {
                if (!skill->isVisible())
                    skill = Sanguosha->getMainSkill(skill->objectName());
                if (skill->isVisible() && player->hasSkill(skill->objectName())) {
                    LogMessage log;
                    log.type = "#SkillAvoid";
                    log.from = player;
                    log.arg = skill->objectName();
                    log.arg2 = objectName();
                    room->sendLog(log);

                    room->broadcastSkillInvoke(skill->objectName());
                }
                player = room->findPlayer(player->getNextAlive()->objectName());
                continue;
            }

            CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, target->objectName(), QString(), this->getSkillName(),
                                  QString());
            room->moveCardTo(this, target, player, Player::PlaceDelayedTrick, reason, true);

            if (player != target) {
                CardUseStruct use;
                use.from = NULL;
                use.to << player;
                use.card = this;
                QVariant data = QVariant::fromValue(use);
                thread->trigger(TargetConfirming, room, player, data);
                CardUseStruct new_use = data.value<CardUseStruct>();
                if (new_use.to.isEmpty()) {
                    p = player;
                    break;
                }

                foreach (ServerPlayer *p, room->getAllPlayers())
                    thread->trigger(TargetConfirmed, room, p, data);
            }
            break;
        } while (player != target);

        if (p)
            onNullified(p);
    } else {
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, target->objectName());
        room->throwCard(this, reason, NULL);
    }
}

Weapon::Weapon(Suit suit, int number, int upper_limit_range, int lower_limit_range)
    : EquipCard(suit, number)
    , upper_limit_range(upper_limit_range)
    , lower_limit_range(lower_limit_range)
{
    can_recast = true;
}

bool Weapon::isAvailable(const Player *player) const
{
    if (player->hasFlag("ThChouceUse"))
        return !player->isCardLimited(this, Card::MethodUse) && EquipCard::isAvailable(player);
    if (player->getGameMode() == "04_1v3" && !player->isLord() && !player->isCardLimited(this, Card::MethodRecast))
        return true;
    return !player->isCardLimited(this, Card::MethodUse) && EquipCard::isAvailable(player);
}

int Weapon::getRange() const
{
    return getRanges().last();
}

QList<int> Weapon::getRanges() const
{
    return QList<int>() << lower_limit_range << upper_limit_range;
}

QString Weapon::getSubtype() const
{
    return "weapon";
}

void Weapon::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = card_use.from;
    if (room->getMode() == "04_1v3" && use.card->isKindOf("Weapon") && !player->isLord()
        && (player->isCardLimited(use.card, Card::MethodUse) || player->isProhibited(player, use.card)
            || (player->handCards().contains(getEffectiveId())
                && player->askForSkillInvoke("alliance_recast", QVariant::fromValue(use))))) {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, player->objectName());
        reason.m_eventName = "alliance_recast";
        room->moveCardTo(use.card, player, NULL, Player::DiscardPile, reason);
        player->broadcastSkillInvoke("@recast");
        room->setEmotion(player, "effects/recast");

        LogMessage log;
        log.type = "#UseCard_Recast";
        log.from = player;
        log.card_str = use.card->toString();
        room->sendLog(log);

        player->drawCards(1, "alliance_recast");
        return;
    }
    EquipCard::onUse(room, use);
}

EquipCard::Location Weapon::location() const
{
    return WeaponLocation;
}

QString Weapon::getCommonEffectName() const
{
    return "weapon";
}

QString Armor::getSubtype() const
{
    return "armor";
}

EquipCard::Location Armor::location() const
{
    return ArmorLocation;
}

QString Armor::getCommonEffectName() const
{
    return "armor";
}

Horse::Horse(Suit suit, int number, int correct)
    : EquipCard(suit, number)
    , correct(correct)
{
}

int Horse::getCorrect() const
{
    return correct;
}

void Horse::onInstall(ServerPlayer *) const
{
}

void Horse::onUninstall(ServerPlayer *) const
{
}

QString Horse::getCommonEffectName() const
{
    return "horse";
}

OffensiveHorse::OffensiveHorse(Card::Suit suit, int number, int correct)
    : Horse(suit, number, correct)
{
}

QString OffensiveHorse::getSubtype() const
{
    return "offensive_horse";
}

DefensiveHorse::DefensiveHorse(Card::Suit suit, int number, int correct)
    : Horse(suit, number, correct)
{
}

QString DefensiveHorse::getSubtype() const
{
    return "defensive_horse";
}

EquipCard::Location Horse::location() const
{
    if (correct > 0)
        return DefensiveHorseLocation;
    else
        return OffensiveHorseLocation;
}

QString Treasure::getSubtype() const
{
    return "treasure";
}

EquipCard::Location Treasure::location() const
{
    return TreasureLocation;
}

QString Treasure::getCommonEffectName() const
{
    return "treasure";
}

StandardPackage::StandardPackage()
    : Package("standard")
{
    addGenerals();
}

ADD_PACKAGE(Standard)
