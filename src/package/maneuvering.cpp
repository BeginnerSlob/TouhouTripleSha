#include "maneuvering.h"
#include "achievement.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "room.h"

NatureSlash::NatureSlash(Suit suit, int number, DamageStruct::Nature nature)
    : Slash(suit, number)
{
    this->nature = nature;
}

bool NatureSlash::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("slash"))
        return true;
    else
        return Slash::match(pattern);
}

ThunderSlash::ThunderSlash(Suit suit, int number)
    : NatureSlash(suit, number, DamageStruct::Thunder)
{
    setObjectName("thunder_slash");
}

FireSlash::FireSlash(Suit suit, int number)
    : NatureSlash(suit, number, DamageStruct::Fire)
{
    setObjectName("fire_slash");
    nature = DamageStruct::Fire;
}

Analeptic::Analeptic(Card::Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("analeptic");
    target_fixed = true;
}

QString Analeptic::getSubtype() const
{
    return "buff_card";
}

bool Analeptic::IsAvailable(const Player *player, const Card *analeptic)
{
    Analeptic *newanaleptic = new Analeptic(Card::NoSuit, 0);
    newanaleptic->deleteLater();
#define THIS_ANALEPTIC (analeptic == NULL ? newanaleptic : analeptic)
    if (player->isCardLimited(THIS_ANALEPTIC, Card::MethodUse) || player->isProhibited(player, THIS_ANALEPTIC))
        return false;

    return player->usedTimes("Analeptic") <= Sanguosha->correctCardTarget(TargetModSkill::Residue, player, THIS_ANALEPTIC);
#undef THIS_ANALEPTIC
}

bool Analeptic::isAvailable(const Player *player) const
{
    return IsAvailable(player, this) && BasicCard::isAvailable(player);
}

void Analeptic::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    if (use.from->hasFlag("Global_Dying")
        && Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY)
        room->setCardFlag(use.card, "analeptic_recover");
    BasicCard::onUse(room, use);
}

void Analeptic::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (targets.isEmpty())
        targets << source;
    BasicCard::use(room, source, targets);
}

void Analeptic::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.to, "analeptic");

    if (effect.card->hasFlag("analeptic_recover"))
        room->recover(effect.to, RecoverStruct(effect.from, this));
    else
        room->addPlayerMark(effect.to, "drank");
}

class FanSkill : public OneCardViewAsSkill
{
public:
    FanSkill()
        : OneCardViewAsSkill("fan")
    {
        filter_pattern = "%slash";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && player->getMark("Equips_Nullified_to_Yourself") == 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            && pattern == "slash" && player->getMark("Equips_Nullified_to_Yourself") == 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *acard = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

Fan::Fan(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("fan");
}

class GudingBladeSkill : public WeaponSkill
{
public:
    GudingBladeSkill()
        : WeaponSkill("guding_blade")
    {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.to->getMark("Equips_of_Others_Nullified_to_You") == 0
            && damage.to->isKongcheng() && damage.by_user && !damage.chain && !damage.transfer) {
            room->setEmotion(player, "effects/weapon");

            LogMessage log;
            log.type = "#GudingBladeEffect";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            QStringList list = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
            if (!list.contains("guding_blade")) {
                room->setAchievementData(player, "jhsr", "guding_blade", false);
                room->addAchievementData(player, "jhsr", 1, false);
                if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                    const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                    if (s && s->inherits("AchieveSkill")) {
                        const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                        as->gainAchievement(player, room);
                    }
                }
            }

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

GudingBlade::GudingBlade(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("guding_blade");
}

class MaidSuitSkill : public ArmorSkill
{
public:
    MaidSuitSkill()
        : ArmorSkill("maid_suit")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!ArmorSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.from->hasSkill("ikkongni"))
            return QStringList();
        if (damage.nature == DamageStruct::Normal && damage.card && damage.card->getTypeId() != Card::TypeSkill)
            return QStringList(objectName());
        if (damage.nature == DamageStruct::Fire && damage.from && damage.from->isAlive())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName(), false);

        QStringList list = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
        if (!list.contains("maid_suit")) {
            room->setAchievementData(player, "jhsr", "maid_suit", false);
            room->addAchievementData(player, "jhsr", 1, false);
            if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                if (s && s->inherits("AchieveSkill")) {
                    const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                    as->gainAchievement(player, room);
                }
            }
        }

        DamageStruct damage = data.value<DamageStruct>();
        room->setEmotion(player, "effects/armor");
        if (damage.nature == DamageStruct::Normal)
            player->drawCards(1, objectName());
        else if (damage.nature == DamageStruct::Fire) {
            QStringList choices;
            if (damage.from->isWounded())
                choices << "recover";
            choices << "draw";
            QString choice = room->askForChoice(damage.from, objectName(), choices.join("+"));
            if (choice == "recover")
                room->recover(damage.from, RecoverStruct(player));
            else
                damage.from->drawCards(1, objectName() + "_draw");
        }

        return false;
    }
};

MaidSuit::MaidSuit(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("maid_suit");
}

class VineSkill : public ArmorSkill
{
public:
    VineSkill()
        : ArmorSkill("vine")
    {
        events << DamageInflicted << SlashEffected << CardEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!ArmorSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.from && effect.from->hasSkill("ikkongni"))
                return QStringList();
            if (effect.nature == DamageStruct::Normal)
                return QStringList(objectName());
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.from && effect.from->hasSkill("ikkongni"))
                return QStringList();
            if (effect.card->isKindOf("SavageAssault") || effect.card->isKindOf("ArcheryAttack")
                || effect.card->isKindOf("Drowning"))
                return QStringList(objectName());
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->hasSkill("ikkongni"))
                return QStringList();
            if (damage.nature == DamageStruct::Fire)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();

            room->setEmotion(player, "effects/armor");
            LogMessage log;
            log.from = player;
            log.type = "#ArmorNullify";
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            room->sendLog(log);

            QStringList list = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
            if (!list.contains("vine")) {
                room->setAchievementData(player, "jhsr", "vine", false);
                room->addAchievementData(player, "jhsr", 1, false);
                if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                    const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                    if (s && s->inherits("AchieveSkill")) {
                        const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                        as->gainAchievement(player, room);
                    }
                }
            }

            effect.to->setFlags("Global_NonSkillNullify");
            return true;
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            room->setEmotion(player, "effects/armor");
            LogMessage log;
            log.from = player;
            log.type = "#ArmorNullify";
            log.arg = objectName();
            log.arg2 = effect.card->objectName();
            room->sendLog(log);

            QStringList list = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
            if (!list.contains("vine")) {
                room->setAchievementData(player, "jhsr", "vine", false);
                room->addAchievementData(player, "jhsr", 1, false);
                if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                    const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                    if (s && s->inherits("AchieveSkill")) {
                        const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                        as->gainAchievement(player, room);
                    }
                }
            }

            room->addAchievementData(player, "tssz");
            if (room->getAchievementData(player, "tssz").toInt() == 3) {
                const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_tssz");
                if (s && s->inherits("AchieveSkill")) {
                    const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                    as->gainAchievement(player, room);
                }
            }

            effect.to->setFlags("Global_NonSkillNullify");
            return true;
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            room->setEmotion(player, "effects/armor_vineburn");
            LogMessage log;
            log.type = "#VineDamage";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            QStringList list = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
            if (!list.contains("vine")) {
                room->setAchievementData(player, "jhsr", "vine", false);
                room->addAchievementData(player, "jhsr", 1, false);
                if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                    const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                    if (s && s->inherits("AchieveSkill")) {
                        const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                        as->gainAchievement(player, room);
                    }
                }
            }

            room->addAchievementData(player, "pmdx");
            if (room->getAchievementData(player, "pmdx").toInt() == 3) {
                const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_pmdx");
                if (s && s->inherits("AchieveSkill")) {
                    const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                    as->gainAchievement(player, room);
                }
            }

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

Vine::Vine(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("vine");
}

class SilverLionSkill : public ArmorSkill
{
public:
    SilverLionSkill()
        : ArmorSkill("silver_lion")
    {
        events << DamageInflicted << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->hasSkill("ikkongni"))
                return QStringList();
            if (ArmorSkill::triggerable(player) && damage.damage > 1)
                return QStringList(objectName());
        } else if (player->hasFlag("SilverLionRecover")) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player || !move.from_places.contains(Player::PlaceEquip))
                return QStringList();
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (move.from_places[i] != Player::PlaceEquip)
                    continue;
                const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
                if (card->objectName() == objectName()) {
                    if (player->isWounded())
                        return QStringList(objectName());
                    else
                        player->setFlags("-SilverLionRecover");
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            room->setEmotion(player, "effects/armor");
            LogMessage log;
            log.type = "#SilverLion";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = objectName();
            room->sendLog(log);

            QStringList list = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
            if (!list.contains("silver_lion")) {
                room->setAchievementData(player, "jhsr", "silver_lion", false);
                room->addAchievementData(player, "jhsr", 1, false);
                if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                    const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                    if (s && s->inherits("AchieveSkill")) {
                        const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                        as->gainAchievement(player, room);
                    }
                }
            }

            damage.damage = 1;
            data = QVariant::fromValue(damage);
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            player->setFlags("-SilverLionRecover");
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (move.from_places[i] != Player::PlaceEquip)
                    continue;
                const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
                if (card->objectName() == objectName()) {
                    room->setEmotion(player, "effects/armor_silver_lion");

                    QStringList list
                        = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
                    if (!list.contains("silver_lion")) {
                        room->setAchievementData(player, "jhsr", "silver_lion", false);
                        room->addAchievementData(player, "jhsr", 1, false);
                        if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                            const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                            if (s && s->inherits("AchieveSkill")) {
                                const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                                as->gainAchievement(player, room);
                            }
                        }
                    }

                    RecoverStruct recover;
                    recover.card = card;
                    room->recover(player, recover);

                    return false;
                }
            }
        }
        return false;
    }
};

SilverLion::SilverLion(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("silver_lion");
}

void SilverLion::onUninstall(ServerPlayer *player) const
{
    if (player->isAlive() && player->hasArmorEffect(objectName()))
        player->setFlags("SilverLionRecover");
    Armor::onUninstall(player);
}

FireAttack::FireAttack(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("fire_attack");
}

bool FireAttack::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty();
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && !to_select->isKongcheng()
        && (to_select != Self || !Self->isLastHandCard(this, true));
}

void FireAttack::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/fire_attack");
    SingleTargetTrick::use(room, source, targets);
}

void FireAttack::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.to->isKongcheng())
        return;

    const Card *card = room->askForCardShow(effect.to, effect.from, objectName());
    room->showCard(effect.to, card->getEffectiveId());

    QString suit_str = card->getSuitString();
    QString pattern = QString(".|%1|.|hand").arg(suit_str);
    QString prompt = QString("@fire-attack:%1::%2").arg(effect.to->objectName()).arg(suit_str);
    if (effect.from->hasSkill("thyanlun")) {
        QString color_str = card->isRed() ? "red" : "black";
        pattern = QString(".|%1|.|hand").arg(color_str);
        prompt = QString("@fire-attackex:%1::%2").arg(effect.to->objectName()).arg(color_str);
        room->notifySkillInvoked(effect.from, "thyanlun");
    }
    if (effect.from->isAlive()) {
        QString name = card->getClassName();
        if (name.endsWith("Slash"))
            name = "Slash";
        name += "|.|.|hand";
        const Card *card_to_throw = room->askForCard(effect.from, pattern + "#" + name, prompt);
        if (card_to_throw)
            room->damage(DamageStruct(this, effect.from, effect.to, 1, DamageStruct::Fire));
        else
            effect.from->setFlags("FireAttackFailed_" + effect.to->objectName()); // For AI
    }

    if (card->isVirtualCard())
        delete card;
}

IronChain::IronChain(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("iron_chain");
    can_recast = true;
}

QString IronChain::getSubtype() const
{
    return "damage_spread";
}

bool IronChain::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return !Self->isCardLimited(this, Card::MethodUse);
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && !Self->isCardLimited(this, Card::MethodUse);
}

bool IronChain::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    bool rec = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) && can_recast
        && !Self->hasFlag("ThChouceUse");
    QList<int> sub;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    foreach (int id, sub) {
        if (Self->getPile("wooden_ox").contains(id)) {
            rec = false;
            break;
        }
        if (Self->getPile("pokemon").contains(id)) {
            rec = false;
            break;
        }
        // Coupling of ThBaochui
        if (Self->hasFlag("thbaochui") && Self->getPhase() == Player::Play) {
            foreach (const Player *p, Self->getAliveSiblings())
                if (p->getPile("currency").contains(id)) {
                    rec = false;
                    break;
                }
            if (!rec)
                break;
        }
    }

    if (rec && Self->isCardLimited(this, Card::MethodUse))
        return targets.length() == 0;
    if (Self->hasFlag("ThChouceUse"))
        return targets.length() == 1;
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (!rec)
        return targets.length() > 0 && targets.length() <= total_num;
    else
        return targets.length() <= total_num;
}

void IronChain::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (!targets.isEmpty()) {
        foreach (ServerPlayer *p, targets)
            room->setEmotion(p, "effects/iron_chain");
    }
    TrickCard::use(room, source, targets);
}

void IronChain::onUse(Room *room, const CardUseStruct &card_use) const
{
    if (card_use.to.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
        reason.m_skillName = this->getSkillName();
        room->moveCardTo(this, card_use.from, NULL, Player::DiscardPile, reason);
        if (card_use.card->getSkillName() == "ikfuhua")
            room->broadcastSkillInvoke("ikfuhua", 3);
        else
            card_use.from->broadcastSkillInvoke("@recast");
        room->setEmotion(card_use.from, "effects/recast");

        LogMessage log;
        log.type = "#UseCard_Recast";
        log.from = card_use.from;
        log.card_str = card_use.card->toString();
        room->sendLog(log);

        card_use.from->drawCards(1, "iron_chain");
    } else
        TrickCard::onUse(room, card_use);
}

void IronChain::onEffect(const CardEffectStruct &effect) const
{
    effect.to->setChained(!effect.to->isChained());

    Room *room = effect.to->getRoom();

    room->broadcastProperty(effect.to, "chained");
    room->getThread()->trigger(ChainStateChanged, room, effect.to);
}

SupplyShortage::SupplyShortage(Card::Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("supply_shortage");

    judge.pattern = ".|club";
    judge.good = true;
    judge.reason = objectName();
}

void SupplyShortage::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    Q_ASSERT(targets.length() == 1);
    room->setEmotion(targets.first(), "effects/supply_shortage");
    DelayedTrick::use(room, source, targets);
}

bool SupplyShortage::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty() && !to_select->containsTrick(objectName());
    if (!targets.isEmpty() || to_select->containsTrick(objectName()) || to_select == Self)
        return false;

    int distance_limit = 1 + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, this);
    if (Self->hasSkill("ikjieying") && to_select->getHandcardNum() >= Self->getHandcardNum())
        distance_limit += 10086;
    int rangefix = 0;
    if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
        rangefix += 1;

    int dis = Self->distanceTo(to_select, rangefix);
    if (dis == -1 || dis > distance_limit)
        return false;

    return true;
}

void SupplyShortage::takeEffect(ServerPlayer *target) const
{
    target->skip(Player::Draw);
}

ManeuveringPackage::ManeuveringPackage()
    : Package("maneuvering", Package::CardPack)
{
    QList<Card *> cards;

    // spade
    cards << new GudingBlade(Card::Spade, 1) << new MaidSuit() << new Analeptic(Card::Spade, 3)
          << new ThunderSlash(Card::Spade, 4) << new ThunderSlash(Card::Spade, 5) << new ThunderSlash(Card::Spade, 6)
          << new ThunderSlash(Card::Spade, 7) << new ThunderSlash(Card::Spade, 8) << new Analeptic(Card::Spade, 9)
          << new SupplyShortage(Card::Spade, 10) << new IronChain(Card::Spade, 11) << new IronChain(Card::Spade, 12)
          << new Nullification(Card::Spade, 13);

    // heart
    cards << new Nullification(Card::Heart, 1) << new FireAttack(Card::Heart, 2) << new FireAttack(Card::Heart, 3)
          << new FireSlash(Card::Heart, 4) << new Peach(Card::Heart, 5) << new Peach(Card::Heart, 6)
          << new FireSlash(Card::Heart, 7) << new Jink(Card::Heart, 8) << new Jink(Card::Heart, 9)
          << new FireSlash(Card::Heart, 10) << new Jink(Card::Heart, 11) << new Jink(Card::Heart, 12)
          << new Nullification(Card::Heart, 13);

    // club
    cards << new SilverLion(Card::Club, 1) << new Vine() << new Analeptic(Card::Club, 3) << new SupplyShortage(Card::Club, 4)
          << new ThunderSlash(Card::Club, 5) << new ThunderSlash(Card::Club, 6) << new ThunderSlash(Card::Club, 7)
          << new ThunderSlash(Card::Club, 8) << new Analeptic(Card::Club, 9) << new IronChain(Card::Club, 10)
          << new IronChain(Card::Club, 11) << new IronChain(Card::Club, 12) << new IronChain(Card::Club, 13);

    // diamond
    cards << new Fan(Card::Diamond, 1) << new Peach(Card::Diamond, 2) << new Peach(Card::Diamond, 3)
          << new FireSlash(Card::Diamond, 4) << new FireSlash(Card::Diamond, 5) << new Jink(Card::Diamond, 6)
          << new Jink(Card::Diamond, 7) << new Jink(Card::Diamond, 8) << new Analeptic(Card::Diamond, 9)
          << new Jink(Card::Diamond, 10) << new Jink(Card::Diamond, 11) << new FireAttack(Card::Diamond, 12);

    DefensiveHorse *tianpanzhou = new DefensiveHorse(Card::Diamond, 13);
    tianpanzhou->setObjectName("tianpanzhou");
    cards << tianpanzhou;

    foreach (Card *card, cards)
        card->setParent(this);

    skills << new GudingBladeSkill << new FanSkill << new MaidSuitSkill << new VineSkill << new SilverLionSkill;
}

ADD_PACKAGE(Maneuvering)
