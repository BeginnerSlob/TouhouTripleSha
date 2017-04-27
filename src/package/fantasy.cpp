#include "fantasy.h"
#include "maneuvering.h"
#include "engine.h"
#include "client.h"

class IceSwordSkill : public WeaponSkill
{
public:
    IceSwordSkill() : WeaponSkill("ice_sword")
    {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.card && damage.card->isKindOf("Slash")
            && damage.to->getMark("Equips_of_Others_Nullified_to_You") == 0
            && !damage.to->isNude() && damage.by_user
            && !damage.chain && !damage.transfer && player->askForSkillInvoke("ice_sword", data)) {
                room->setEmotion(player, "effects/weapon");
                if (damage.from->canDiscard(damage.to, "he")) {
                    int card_id = room->askForCardChosen(player, damage.to, "he", "ice_sword", false, Card::MethodDiscard);
                    room->throwCard(Sanguosha->getCard(card_id), damage.to, damage.from);

                    if (damage.from->isAlive() && damage.to->isAlive() && damage.from->canDiscard(damage.to, "he")) {
                        card_id = room->askForCardChosen(player, damage.to, "he", "ice_sword", false, Card::MethodDiscard);
                        room->throwCard(Sanguosha->getCard(card_id), damage.to, damage.from);
                    }
                }
                return true;
        }
        return false;
    }
};

IceSword::IceSword(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("ice_sword");
}

class LureTigerSkill : public TriggerSkill
{
public:
    LureTigerSkill() : TriggerSkill("lure_tiger")
    {
        events << Death << EventPhaseChanging;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player->hasFlag("LureTigerTurn"))
            return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        }

        foreach (ServerPlayer *p, room->getAllPlayers(player)) {
            if (p->isRemoved()) {
                room->setPlayerProperty(p, "removed", false);
                room->removePlayerCardLimitation(p, "use", ".$0");
            }
        }

        return QStringList();
    }
};

class LureTigerProhibit : public ProhibitSkill
{
public:
    LureTigerProhibit() : ProhibitSkill("#lure_tiger")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->isRemoved() && (card->getTypeId() != Card::TypeSkill
                                   || card->isKindOf("IkMiceCard")
                                   || card->isKindOf("IkWudiCard")
                                   || card->isKindOf("IkMingwangCard"));
    }
};

LureTiger::LureTiger(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("lure_tiger");
}

QString LureTiger::getSubtype() const
{
    return "lure_tiger";
}

bool LureTiger::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty();
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num)
        return false;
    if (Self->isCardLimited(this, Card::MethodUse))
        return false;

    return to_select != Self;
}

void LureTiger::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/lure_tiger");
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    foreach (ServerPlayer *target, targets)
        room->addPlayerMark(target, "cardEffect_" + toString());
    foreach (ServerPlayer *target, targets) {
        room->removePlayerMark(target, "cardEffect_" + toString());
        bool null_heg = target->getMark("cardNullifyHeg_" + toString()) > 0;
        if (null_heg)
            room->removePlayerMark(target, "cardNullifyHeg_" + toString());

        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        effect.nullified = null_heg || (all_nullified || nullified_list.contains(target->objectName()));

        room->cardEffect(effect);
    }

    foreach (ServerPlayer *p, room->getAlivePlayers())
        room->setPlayerMark(p, "cardEffect_" + toString(), 0);

    source->drawCards(1, objectName());

    if (room->getCardPlace(getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), getSkillName(), QString());
        if (targets.size() == 1) reason.m_targetId = targets.first()->objectName();
        reason.m_extraData = QVariant::fromValue((const Card *)this);
        room->moveCardTo(this, source, NULL, Player::DiscardPile, reason, true);
    }
}

void LureTiger::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    ServerPlayer *current = room->getCurrent();
    if (!current || current->getPhase() == Player::NotActive || current->isDead())
        return;

    room->setPlayerCardLimitation(effect.to, "use", ".", false);
    room->setPlayerProperty(effect.to, "removed", true);
    current->setFlags("LureTigerTurn");
}

class ThMengxuan: public TriggerSkill
{
public:
    ThMengxuan(): TriggerSkill("thmengxuan")
    {
        events << Damaged;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        QStringList list;
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            for (int i = 0; i < damage.damage; ++i)
                list << objectName();
        }
        return list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1, objectName());
        return false;
    }
};

ScrollCard::ScrollCard()
{
    target_fixed = true;
}

void ScrollCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->acquireSkill(source, "thmengxuan");
    room->setPlayerMark(source, "scroll", 1);
    if (source->getTreasure() && source->canDiscard(source, source->getTreasure()->getEffectiveId()))
        room->throwCard(source->getTreasure(), source);
}

class ScrollSkill : public ZeroCardViewAsSkill
{
public:
    ScrollSkill() : ZeroCardViewAsSkill("scroll")
    {
    }

    virtual const Card *viewAs() const
    {
        return new ScrollCard;
    }
};

class ScrollTriggerSkill: public TreasureSkill
{
public:
    ScrollTriggerSkill(): TreasureSkill("scroll_trigger")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
        global = true;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive() && target->getMark("scroll") > 0 && target->getPhase() == Player::RoundStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->setPlayerMark(player, "scroll", 0);
        room->detachSkillFromPlayer(player, "thmengxuan");
        return false;
    }
};

class ScrollProhibit : public ProhibitSkill
{
public:
    ScrollProhibit() : ProhibitSkill("#scroll")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasTreasure("scroll") && card->isKindOf("DelayedTrick");
    }
};

Scroll::Scroll(Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("scroll");
}

KnownBoth::KnownBoth(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("known_both");
}

bool KnownBoth::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return SingleTargetTrick::targetFilter(targets, to_select, Self);
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;

    return !to_select->isKongcheng();
}

void KnownBoth::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/known_both");
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    foreach (ServerPlayer *target, targets) {
        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        effect.nullified = (all_nullified || nullified_list.contains(target->objectName()));

        room->cardEffect(effect);
    }

    source->drawCards(1, objectName());

    if (room->getCardPlace(getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), getSkillName(), QString());
        if (targets.size() == 1) reason.m_targetId = targets.first()->objectName();
        reason.m_extraData = QVariant::fromValue((const Card *)this);
        room->moveCardTo(this, source, NULL, Player::DiscardPile, reason, true);
    }
}

void KnownBoth::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isKongcheng())
        return;
    Room *room = effect.to->getRoom();

    LogMessage log;
    log.type = "$IkLingtongView";
    log.from = effect.from;
    log.to << effect.to;
    log.arg = "iklingtong:handcards";
    room->sendLog(log, room->getOtherPlayers(effect.from));

    room->showAllCards(effect.to, effect.from);
}

JadeCard::JadeCard()
{
}

bool JadeCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const
{
    return to_select->getMark("cardEffect_" + Self->property("jade_trick").toString()) > 0;
}

void JadeCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->addPlayerMark(effect.to, "cardNullifyHeg_" + effect.from->property("jade_trick").toString());
}

class JadeSkill : public ZeroCardViewAsSkill
{
public:
    JadeSkill() : ZeroCardViewAsSkill("jade")
    {
    }

    virtual const Card *viewAs() const
    {
        if (Sanguosha->getCurrentCardUsePattern() != "nullification")
            return new JadeCard;
        else {
            Nullification *null = new Nullification(Card::SuitToBeDecided, -1);
            if (Self->getTreasure() && Self->getTreasure()->objectName() == objectName())
                null->addSubcard(Self->getTreasure());
            null->setSkillName("jade");
            return null;
        }
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "@@jade" || (pattern == "nullification" && player->hasTreasure(objectName()));
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->isAlive() && player->hasTreasure(objectName());
    }
};

class JadeTriggerSkill : public TreasureSkill
{
public:
    JadeTriggerSkill() : TreasureSkill("jade_trigger")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player->hasFlag("JadeDraw")) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player || !move.from_places.contains(Player::PlaceEquip))
                return QStringList();
            for (int i = 0; i < move.card_ids.size(); ++ i) {
                if (move.from_places[i] != Player::PlaceEquip)
                    continue;
                const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
                if (card->objectName() == "jade")
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        player->setFlags("-JadeDraw");
        for (int i = 0; i < move.card_ids.size(); ++ i) {
            if (move.from_places[i] != Player::PlaceEquip) continue;
            const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
            if (card->objectName() == "jade")
                player->drawCards(1, "jade");
        }

        return false;
    }
};

Jade::Jade(Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("jade");
}

void Jade::onUninstall(ServerPlayer *player) const
{
    if (player->isAlive() && player->hasTreasure(objectName()))
        player->setFlags("JadeDraw");
    Treasure::onUninstall(player);
}

PurpleSong::PurpleSong(Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("purple_song");

    judge.pattern = ".|diamond";
    judge.good = false;
    judge.negative = false;
    judge.reason = objectName();
}

bool PurpleSong::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty() && !to_select->containsTrick(objectName());
    return targets.isEmpty() && !to_select->containsTrick(objectName()) && to_select != Self;
}

void PurpleSong::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    Q_ASSERT(targets.length() == 1);
    room->setEmotion(targets.first(), "effects/purple_song");
    DelayedTrick::use(room, source, targets);
}

void PurpleSong::takeEffect(ServerPlayer *target) const
{
    target->skip(Player::Discard);
    target->getRoom()->addPlayerMark(target, "zilianshengyong");
}

class PurpleSongSkill: public TriggerSkill
{
public:
    PurpleSongSkill(): TriggerSkill("purple_song")
    {
        events << EventPhaseStart;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (player->getMark("zilianshengyong") == 0)
            return QStringList();
        if (player->getPhase() == Player::NotActive) {
            while (player->getMark("zilianshengyong") > 0) {
                room->removePlayerMark(player, "zilianshengyong");
                player->drawCards(2, objectName());
            }
        }

        return QStringList();
    }
};

class IronArmorSkill: public ProhibitSkill
{
public:
    IronArmorSkill(): ProhibitSkill("iron_armor")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        if (to->hasArmorEffect(objectName()) && !from->hasSkill("ikkongni"))
            return card->isKindOf("FireAttack")
                || card->isKindOf("BurningCamps")
                || card->isKindOf("IronChain")
                || (card->isKindOf("Slash") && card->isRed());
        return false;
    }
};

IronArmor::IronArmor(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("iron_armor");
}

class MoonSpearSkill: public WeaponSkill
{
public:
    MoonSpearSkill(): WeaponSkill("moon_spear")
    {
        events << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!WeaponSkill::triggerable(player) || player->getPhase() != Player::NotActive)
            return QStringList();

        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            if (card_use.m_isHandcard)
                card = card_use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isHandcard && !resp.m_isRetrial)
                card = resp.m_card;
        }

        if (card == NULL || !card->isBlack()
            || (card->getHandlingMethod() != Card::MethodUse && card->getHandlingMethod() != Card::MethodResponse))
            return QStringList();
        foreach (ServerPlayer *tmp, room->getOtherPlayers(player))
            if (player->inMyAttackRange(tmp))
                return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *tmp, room->getOtherPlayers(player)) {
            if (player->inMyAttackRange(tmp))
                targets << tmp;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@moon_spear", true, true);
        if (target) {
            room->setEmotion(player, "effects/weapon");
            player->tag["MoonSpearTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["MoonSpearTarget"].value<ServerPlayer *>();
        player->tag.remove("MoonSpearTarget");
        if (!target) return false;
        if (!room->askForCard(target, "jink", "@moon-spear-jink", QVariant(), Card::MethodResponse, player))
            room->damage(DamageStruct(objectName(), player, target));
        return false;
    }
};

MoonSpear::MoonSpear(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("moon_spear");
}

BurningCamps::BurningCamps(Card::Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("burning_camps");
    can_recast = true;
}

bool BurningCamps::isAvailable(const Player *player) const
{
    if (player->hasFlag("ThChouceUse"))
        return !player->isCardLimited(this, Card::MethodUse) && TrickCard::isAvailable(player);
    if (player->getGameMode() == "04_1v3" && !player->isLord() && !player->isCardLimited(this, Card::MethodRecast))
        return true;

    bool canUse = false;
    const Player *next = player->getNextAlive();
    if (!next || next == player)
        return false;
    QList<const Player *> players = next->getFormation();
    foreach (const Player *p, players) {
        if (player->isProhibited(p, this))
            continue;

        canUse = true;
        break;
    }

    return canUse && !player->isCardLimited(this, Card::MethodUse) && TrickCard::isAvailable(player);
}

void BurningCamps::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct new_use = card_use;
    if (room->getMode() == "04_1v3"
        && card_use.card->isKindOf("BurningCamps")
        && !card_use.from->isLord()) {
        bool recast = card_use.from->isCardLimited(card_use.card, Card::MethodUse);
        if (!recast) {
            bool all_prohibit = true;
            foreach (const Player *target, card_use.from->getNextAlive()->getFormation()) {
                if (!card_use.from->isProhibited(target, card_use.card)) {
                    all_prohibit = false;
                    break;
                }
            }
            recast = all_prohibit;
        }
        if (recast || (card_use.from->handCards().contains(getEffectiveId())
                       && card_use.from->askForSkillInvoke("alliance_recast", QVariant::fromValue(card_use)))) {
            CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
            reason.m_eventName = "alliance_recast";
            room->moveCardTo(card_use.card, card_use.from, NULL, Player::DiscardPile, reason);
            card_use.from->broadcastSkillInvoke("@recast");
            room->setEmotion(card_use.from, "effects/recast");

            LogMessage log;
            log.type = "#UseCard_Recast";
            log.from = card_use.from;
            log.card_str = card_use.card->toString();
            room->sendLog(log);

            card_use.from->drawCards(1, "alliance_recast");
            return;
        }
    }

    if (new_use.to.isEmpty()) {
        QList<const Player *> targets = card_use.from->getNextAlive()->getFormation();
        foreach (const Player *player, targets) {
            const Skill *skill = room->isProhibited(card_use.from, player, this);
            ServerPlayer *splayer = room->findPlayer(player->objectName());
            if (skill) {
                if (!skill->isVisible())
                    skill = Sanguosha->getMainSkill(skill->objectName());
                if (skill->isVisible()) {
                    LogMessage log;
                    log.type = "#SkillAvoid";
                    log.from = splayer;
                    log.arg = skill->objectName();
                    log.arg2 = objectName();
                    room->sendLog(log);

                    room->broadcastSkillInvoke(skill->objectName());
                }
            } else
                new_use.to << splayer;
        }
    }

    TrickCard::onUse(room, new_use);
}

void BurningCamps::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->doLightbox("anim=effects/burning_camps");
    AOE::use(room, source, targets);
}

void BurningCamps::onEffect(const CardEffectStruct &effect) const
{
    effect.to->getRoom()->damage(DamageStruct(this, effect.from, effect.to, 1, DamageStruct::Fire));
}

class BreastplateSkill : public ArmorSkill
{
public:
    BreastplateSkill() : ArmorSkill("breastplate")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (ArmorSkill::triggerable(player) && damage.damage >= player->getHp()
            && player->getArmor() && player->canDiscard(player, player->getArmor()->getEffectiveId()))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->throwCard(player->getArmor(), player);
        player->drawCards(1, objectName());
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#Breastplate";
        log.from = player;
        if (damage.from)
            log.to << damage.from;
        log.arg = QString::number(damage.damage);
        if (damage.nature == DamageStruct::Normal)
            log.arg2 = "normal_nature";
        else if (damage.nature == DamageStruct::Fire)
            log.arg2 = "fire_nature";
        else if (damage.nature == DamageStruct::Thunder)
            log.arg2 = "thunder_nature";
        room->sendLog(log);
        return true;
    }
};

Breastplate::Breastplate(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("breastplate");
}

class RenwangShieldSkill: public ArmorSkill
{
public:
    RenwangShieldSkill(): ArmorSkill("renwang_shield")
    {
        events << SlashEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!ArmorSkill::triggerable(player)) return QStringList();
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!effect.from
            || effect.from->getMark("Equips_of_Others_Nullified_to_You") > 0
            || effect.from->hasSkill("ikkongni")) return QStringList();
        if (effect.slash->isBlack()) return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        LogMessage log;
        log.type = "#ArmorNullify";
        log.from = player;
        log.arg = objectName();
        log.arg2 = effect.slash->objectName();
        player->getRoom()->sendLog(log);

        room->setEmotion(player, "effects/armor");
        effect.to->setFlags("Global_NonSkillNullify");
        return true;
    }
};

RenwangShield::RenwangShield(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("renwang_shield");
}

Drowning::Drowning(Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("drowning");
}

void Drowning::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/drowning");
    AOE::use(room, source, targets);
}

void Drowning::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    QStringList choices;
    if (effect.from->canDiscard(effect.to, "e"))
        choices << "throw";
    if (effect.from->canDiscard(effect.to, "h") && effect.to->getHandcardNum() >= 2)
        choices << "discard";
    choices << "damage";
    QString choice = room->askForChoice(effect.to, objectName(), choices.join("+"), QVariant::fromValue(effect));
    if (choice == "throw") {
        int card_id = room->askForCardChosen(effect.from, effect.to, "e", objectName(), false, MethodDiscard);
        room->throwCard(card_id, effect.to, effect.from);
    } else if (choice == "discard") {
        room->setPlayerFlag(effect.to, "drowning_InTempMoving");
        DummyCard *dummy = new DummyCard;
        QList<int> card_ids;
        for (int i = 0; i < 2; i++) {
            if (!effect.from->canDiscard(effect.to, "h"))
                break;
            card_ids << room->askForCardChosen(effect.from, effect.to, "h", objectName(), false, Card::MethodDiscard);
            dummy->addSubcard(card_ids[i]);
            effect.to->addToPile("#drowning", card_ids[i], false);
        }
        for (int i = 0; i < dummy->subcardsLength(); i++)
            room->moveCardTo(Sanguosha->getCard(card_ids[i]), effect.to, Player::PlaceHand, false);
        room->setPlayerFlag(effect.to, "-drowning_InTempMoving");
        if (dummy->subcardsLength() > 0)
            room->throwCard(dummy, effect.to, effect.from);
        dummy->deleteLater();
    } else
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to));
}

class DrowningFakeMoveSkill: public FakeMoveSkill
{
public:
    DrowningFakeMoveSkill(): FakeMoveSkill("drowning")
    {
        global = true;
    }
};

WoodenOxCard::WoodenOxCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "wooden_ox";
}

void WoodenOxCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setEmotion(source, "effects/wooden_ox");
    source->addToPile("wooden_ox", subcards, false);

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (!p->getTreasure())
            targets << p;
    }
    if (targets.isEmpty())
        return;
    ServerPlayer *target = room->askForPlayerChosen(source, targets, "wooden_ox", "@wooden_ox-move", true);
    if (target) {
        const Card *treasure = source->getTreasure();
        if (treasure) {
            room->setEmotion(target, "effects/wooden_ox_move");
            room->moveCardTo(treasure, source, target, Player::PlaceEquip,
                             CardMoveReason(CardMoveReason::S_REASON_TRANSFER,
                                            source->objectName(), "wooden_ox", QString()));
        }
    }
}

class WoodenOxSkill: public OneCardViewAsSkill
{
public:
    WoodenOxSkill(): OneCardViewAsSkill("wooden_ox")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("WoodenOxCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        WoodenOxCard *card = new WoodenOxCard;
        card->addSubcard(originalCard);
        card->setSkillName("wooden_ox");
        return card;
    }
};

class WoodenOxTriggerSkill: public TreasureSkill
{
public:
    WoodenOxTriggerSkill(): TreasureSkill("wooden_ox_trigger")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        global = true;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!move.from || move.from != player)
            return false;
        if (player->hasTreasure("wooden_ox")) {
            int count = 0;
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (move.from_pile_names[i] == "wooden_ox") count++;
            }
            if (count > 0) {
                LogMessage log;
                log.type = "#WoodenOx";
                log.from = player;
                log.arg = QString::number(count);
                log.arg2 = "wooden_ox";
                room->sendLog(log);
            }
        }
        if (player->getPile("wooden_ox").isEmpty())
            return false;
        for (int i = 0; i < move.card_ids.size(); i++) {
            if (move.from_places[i] != Player::PlaceEquip && move.from_places[i] != Player::PlaceTable) continue;
            const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
            if (card->objectName() == "wooden_ox") {
                ServerPlayer *to = (ServerPlayer *)move.to;
                if (to && to->getTreasure() && to->getTreasure()->objectName() == "wooden_ox"
                    && move.to_place == Player::PlaceEquip) {
                    QList<ServerPlayer *> p_list;
                    p_list << to;
                    to->addToPile("wooden_ox", player->getPile("wooden_ox"), false, p_list);
                } else {
                    player->clearOnePrivatePile("wooden_ox");
                }
                return false;
            }
        }
        return false;
    }
};

WoodenOx::WoodenOx(Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("wooden_ox");
}

void WoodenOx::onUninstall(ServerPlayer *player) const
{
    player->getRoom()->addPlayerHistory(player, "WoodenOxCard", 0);
    Treasure::onUninstall(player);
}

FantasyPackage::FantasyPackage()
    : Package("fantasy", Package::CardPack)
{
    QList<Card *> cards;
    cards //<< new NewArmor(Card::Spade, 1)
          << new IceSword()
          //<< new NewTrick1(Card::Spade, 3)
          << new LureTiger(Card::Spade, 4)
          << new Slash(Card::Spade, 5)
          << new Slash(Card::Spade, 6)
          << new Scroll()
          << new KnownBoth(Card::Spade, 8)
          << new Analeptic(Card::Spade, 9)
          << new Nullification(Card::Spade, 10)
          //<< new NewTrick2(Card::Spade, 11)
          << new Analeptic(Card::Spade, 12)
          << new Jade()
          << new PurpleSong(Card::Heart, 1)
          << new IronArmor()
          //<< new NewTrick2(Card::Heart, 3)
          << new MoonSpear()
          << new Jink(Card::Heart, 5)
          << new Jink(Card::Heart, 6)
          << new Peach(Card::Heart, 7)
          << new Peach(Card::Heart, 8)
          << new FireSlash(Card::Heart, 9)
          << new LureTiger(Card::Heart, 10)
          //<< new NewTrick3(Card::Heart, 11)
          << new Lightning(Card::Heart, 12)
          << new BurningCamps()
          << new Breastplate()
          << new RenwangShield()
          //<< new NewWeapon(Card::Club, 3)
          << new KnownBoth(Card::Club, 4);

    OffensiveHorse *yicunshenmiaowan = new OffensiveHorse(Card::Club, 5);
    yicunshenmiaowan->setObjectName("yicunshenmiaowan");
    cards << yicunshenmiaowan
          << new Snatch(Card::Club, 6)
          << new Slash(Card::Club, 7)
          << new Slash(Card::Club, 8)
          << new ThunderSlash(Card::Club, 9)
          << new Slash(Card::Club, 10)
          << new Slash(Card::Club, 11)
          << new ThunderSlash(Card::Club, 12)
          << new Drowning()
          << new Nullification(Card::Diamond, 1)
          << new Slash(Card::Diamond, 2)
          << new FireAttack(Card::Diamond, 3)
          << new WoodenOx()
          << new Jink(Card::Diamond, 5)
          << new FireSlash(Card::Diamond, 6)
          << new Jink(Card::Diamond, 7)
          << new Jink(Card::Diamond, 8)
          << new Peach(Card::Diamond, 9)
          << new Jink(Card::Diamond, 10)
          //<< new NewTrick1(Card::Diamond, 11)
          << new Jink(Card::Diamond, 12)
          << new PurpleSong(Card::Diamond, 13);

    skills << new IceSwordSkill
           << new LureTigerSkill << new LureTigerProhibit;
    related_skills.insertMulti("lure_tiger", "#lure_tiger");
    skills << new ThMengxuan
           << new ScrollSkill
           << new ScrollTriggerSkill << new ScrollProhibit;
    related_skills.insertMulti("scroll_trigger", "#scroll");
    skills << new JadeSkill << new JadeTriggerSkill
           << new PurpleSongSkill
           << new IronArmorSkill
           << new MoonSpearSkill
           << new BreastplateSkill
           << new RenwangShieldSkill
           << new DrowningFakeMoveSkill
           << new WoodenOxSkill << new WoodenOxTriggerSkill;

    foreach (Card *card, cards)
        card->setParent(this);

    addMetaObject<ScrollCard>();
    addMetaObject<JadeCard>();
    addMetaObject<WoodenOxCard>();
}

ADD_PACKAGE(Fantasy)
