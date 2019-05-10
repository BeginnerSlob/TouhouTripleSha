#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"
#include "standard-equips.h"
#include "standard.h"

Slash::Slash(Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("slash");
    nature = DamageStruct::Normal;
    specific_assignee = QStringList();
}

bool Slash::IsAvailable(const Player *player, const Card *slash, bool considerSpecificAssignee)
{
    Slash *newslash = new Slash(Card::NoSuit, 0);
    newslash->setFlags("Global_SlashAvailabilityChecker");
    newslash->deleteLater();
#define THIS_SLASH (slash == NULL ? newslash : slash)
    if (player->isCardLimited(THIS_SLASH, Card::MethodUse))
        return false;

    if (player->getPhase() == Player::Play && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
        QList<int> ids;
        if (slash) {
            if (slash->isVirtualCard()) {
                if (slash->subcardsLength() > 0)
                    ids = slash->getSubcards();
            } else {
                ids << slash->getEffectiveId();
            }
        }
        bool has_weapon = (player->hasWeapon("crossbow") || player->hasWeapon("vscrossbow"))
            && (player->getWeapon() && ids.contains(player->getWeapon()->getEffectiveId()));
        if ((!has_weapon && player->hasWeapon("crossbow")) || player->canSlashWithoutCrossbow(THIS_SLASH))
            return true;
        int used = player->getSlashCount();
        int valid = 1 + Sanguosha->correctCardTarget(TargetModSkill::Residue, player, newslash);
        if ((!has_weapon && player->hasWeapon("vscrossbow")) && used < valid + 3)
            return true;

        if (considerSpecificAssignee) {
            QStringList assignee_list = player->property("extra_slash_specific_assignee").toString().split("+");
            if (!assignee_list.isEmpty()) {
                foreach (const Player *p, player->getAliveSiblings()) {
                    if (assignee_list.contains(p->objectName()) && player->canSlash(p, THIS_SLASH))
                        return true;
                }
            }
        }
        return false;
    } else {
        return true;
    }
#undef THIS_SLASH
}

bool Slash::IsSpecificAssignee(const Player *player, const Player *from, const Card *slash)
{
    if (from->hasFlag("slashTargetFix") && player->hasFlag("SlashAssignee"))
        return true;
    else if (from->getPhase() == Player::Play && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
             && !Slash::IsAvailable(from, slash, false)) {
        QStringList assignee_list = from->property("extra_slash_specific_assignee").toString().split("+");
        if (assignee_list.contains(player->objectName()))
            return true;
    } else {
        const Slash *s = qobject_cast<const Slash *>(slash);
        if (s && s->hasSpecificAssignee(player))
            return true;
    }
    return false;
}

bool Slash::isAvailable(const Player *player) const
{
    return IsAvailable(player, this) && BasicCard::isAvailable(player);
}

QString Slash::getSubtype() const
{
    return "attack_card";
}

void Slash::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;

    if (player->hasFlag("slashTargetFix")) {
        room->setPlayerFlag(player, "-slashTargetFix");
        room->setPlayerFlag(player, "-slashTargetFixToOne");
        foreach (ServerPlayer *target, room->getAlivePlayers())
            if (target->hasFlag("SlashAssignee"))
                room->setPlayerFlag(target, "-SlashAssignee");
    }

    if (objectName() == "slash" && use.m_isOwnerUse) {
        bool has_changed = false;
        QString skill_name = getSkillName();
        if (!skill_name.isEmpty()) {
            const ViewAsSkill *skill = Sanguosha->getViewAsSkill(skill_name);
            if (skill && !skill->inherits("FilterSkill") && skill->objectName() != "ikguihuo")
                has_changed = true;
        }
        if (!has_changed || subcardsLength() == 0) {
            QVariant data = QVariant::fromValue(use);
            if (player->hasSkill("iklingpao")) {
                FireSlash *fire_slash = new FireSlash(getSuit(), getNumber());
                fire_slash->setFlags(use.card->getFlags());
                if (!isVirtualCard() || subcardsLength() > 0)
                    fire_slash->addSubcard(this);
                fire_slash->setSkillName("iklingpao");
                bool can_use = true;
                foreach (ServerPlayer *p, use.to) {
                    if (!player->canSlash(p, fire_slash, false)) {
                        can_use = false;
                        break;
                    }
                }
                if (can_use && room->askForSkillInvoke(player, "iklingpao", data)) {
                    if (has_changed)
                        fire_slash->setSkillName(getSkillName(false));
                    use.card = fire_slash;
                } else {
                    delete fire_slash;
                }
            }
            if (use.card->objectName() == "slash" && player->hasSkill("ikhuangzhen")) {
                ThunderSlash *thunder_slash = new ThunderSlash(getSuit(), getNumber());
                thunder_slash->setFlags(use.card->getFlags());
                if (!isVirtualCard() || subcardsLength() > 0)
                    thunder_slash->addSubcard(this);
                thunder_slash->setSkillName("ikhuangzhen");
                bool can_use = true;
                foreach (ServerPlayer *p, use.to) {
                    if (!player->canSlash(p, thunder_slash, false)) {
                        can_use = false;
                        break;
                    }
                }
                if (can_use && room->askForSkillInvoke(player, "ikhuangzhen", data)) {
                    if (has_changed)
                        thunder_slash->setSkillName(getSkillName(false));
                    use.card = thunder_slash;
                } else {
                    delete thunder_slash;
                }
            }
            if (use.card->objectName() == "slash" && player->hasWeapon("fan")) {
                FireSlash *fire_slash = new FireSlash(getSuit(), getNumber());
                fire_slash->setFlags(use.card->getFlags());
                if (!isVirtualCard() || subcardsLength() > 0)
                    fire_slash->addSubcard(this);
                fire_slash->setSkillName("fan");
                bool can_use = true;
                foreach (ServerPlayer *p, use.to) {
                    if (!player->canSlash(p, fire_slash, false)) {
                        can_use = false;
                        break;
                    }
                }
                if (can_use && room->askForSkillInvoke(player, "fan", data)) {
                    if (has_changed)
                        fire_slash->setSkillName(getSkillName(false));
                    use.card = fire_slash;
                } else {
                    delete fire_slash;
                }
            }
        }
    }
    if (((use.card->isVirtualCard() && use.card->subcardsLength() == 0) || (getSkillName() == "ikguihuo" && use.card != this))
        && !player->hasFlag("slashDisableExtraTarget")) {
        QList<ServerPlayer *> targets_ts;
        while (true) {
            QList<const Player *> targets_const;
            foreach (ServerPlayer *p, use.to)
                targets_const << qobject_cast<const Player *>(p);
            foreach (ServerPlayer *p, room->getAlivePlayers())
                if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, use.from))
                    targets_ts << p;
            if (targets_ts.isEmpty())
                break;

            ServerPlayer *extra_target
                = room->askForPlayerChosen(player, targets_ts, "slash_extra_targets", "@slash_extra_targets", true);
            if (extra_target) {
                use.to.append(extra_target);
                room->sortByActionOrder(use.to);
            } else
                break;
            targets_ts.clear();
            targets_const.clear();
        }
    }

    if (player->hasFlag("slashNoDistanceLimit"))
        room->setPlayerFlag(player, "-slashNoDistanceLimit");
    if (player->hasFlag("slashDisableExtraTarget"))
        room->setPlayerFlag(player, "-slashDisableExtraTarget");

    if (use.from->hasFlag("IkXiashanUse")) {
        use.from->setFlags("-IkXiashanUse");
        room->broadcastSkillInvoke("ikxiashan");

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = use.from;
        log.arg = "ikxiashan";
        room->sendLog(log);

        foreach (ServerPlayer *p, use.to) {
            if (!use.from->inMyAttackRange(p)) {
                room->setPlayerFlag(use.from, "ikxiashan_ikyipao");
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                break;
            }
        }
    }

    if (player->getPhase() == Player::Play && player->hasFlag("Global_MoreSlashInOneTurn")) {
        QString name;
        if (player->hasSkill("ikyipao"))
            name = "ikyipao";
        if (!name.isEmpty()) {
            player->setFlags("-Global_MoreSlashInOneTurn");
            int index = qrand() % 2 + 1;
            if (name == "ikyipao") {
                if (player->getGeneral()->objectName() == "wind003")
                    room->broadcastSkillInvoke(name, index);
                /*if (Player::isNostalGeneral(player, "zhangfei"))
                    index += 2;*/
            }
            room->notifySkillInvoked(player, name);
        }
    }
    if (use.to.size() > 1 && player->hasSkill("thshenmie")) {
        room->broadcastSkillInvoke("thshenmie");
        room->notifySkillInvoked(player, "thshenmie");
    } else if (use.to.size() > 1 && !player->getWeapon() && player->hasSkill("ikshenti")) {
        room->broadcastSkillInvoke("ikshenti");
        room->notifySkillInvoked(player, "ikshenti");
    } else if (use.to.size() > 1 && player->hasSkill("iklingpao") && use.card->isKindOf("FireSlash")
               && use.card->getSkillName() != "iklingpao") {
        room->broadcastSkillInvoke("iklingpao");
        room->notifySkillInvoked(player, "iklingpao");
    } else if (use.to.size() > 1 && player->hasSkill("ikxindu")) {
        room->broadcastSkillInvoke("ikxindu");
        room->notifySkillInvoked(player, "ikxindu");
    } else if (player->hasSkill("thxiagong") && !player->getWeapon() && player->getAttackRange() < 2) {
        foreach (ServerPlayer *p, use.to) {
            if (player->distanceTo(p) == 2) {
                room->broadcastSkillInvoke("thxiagong");
                room->notifySkillInvoked(player, "thxiagong");
                break;
            }
        }
    }

    int rangefix = 0;
    if (use.card->isVirtualCard()) {
        if (use.from->getWeapon() && use.card->getSubcards().contains(use.from->getWeapon()->getId())) {
            const Weapon *weapon = qobject_cast<const Weapon *>(use.from->getWeapon()->getRealCard());
            rangefix += weapon->getRange() - use.from->getAttackRange(false);
        }
        if (use.from->getOffensiveHorse() && use.card->getSubcards().contains(use.from->getOffensiveHorse()->getId()))
            rangefix += 1;
    }
    foreach (ServerPlayer *p, use.to) {
        if (p->hasSkill("ikjinlian") && p->getHandcardNum() > p->getHp() && use.from->inMyAttackRange(p, rangefix)) {
            room->broadcastSkillInvoke("ikjinlian");
            room->notifySkillInvoked(p, "ikjinlian");
            break;
        }
    }

    if (use.from->hasFlag("BladeUse")) {
        use.from->setFlags("-BladeUse");
        room->setEmotion(player, "effects/weapon");

        LogMessage log;
        log.type = "#BladeUse";
        log.from = use.from;
        log.to << use.to;
        room->sendLog(log);

        QStringList list = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
        if (!list.contains("blade")) {
            room->setAchievementData(player, "jhsr", "blade", false);
            room->addAchievementData(player, "jhsr", 1, false);
            if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                if (s && s->inherits("AchieveSkill")) {
                    const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                    as->gainAchievement(player, room);
                }
            }
        }
    } else if (use.from->hasFlag("ThYingshiUse")) {
        use.from->setFlags("-ThYingshiUse");
        room->broadcastSkillInvoke("thyingshi");

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = use.from;
        log.arg = "thyingshi";
        room->sendLog(log);
    } else if (use.from->hasFlag("MoonspearUse")) {
        use.from->setFlags("-MoonspearUse");
        room->setEmotion(player, "effects/weapon");

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = use.from;
        log.arg = "moon_spear";
        room->sendLog(log);
    } else if (use.to.size() > 1 && player->hasWeapon("halberd") && player->isLastHandCard(this))
        room->setEmotion(player, "effects/weapon");
    else if (use.card->isVirtualCard() && use.card->getSkillName() == "fan")
        room->setEmotion(player, "effects/weapon");
    if (player->getPhase() == Player::Play && player->hasFlag("Global_MoreSlashInOneTurn") && player->hasWeapon("crossbow")
        && !player->hasSkill("ikyipao") && !(player->hasSkill("ikcanyue") && player->getMark("ikcanyue") > 0)) {
        player->setFlags("-Global_MoreSlashInOneTurn");
        room->setEmotion(player, "effects/weapon");
    }
    if (use.card->isKindOf("ThunderSlash"))
        room->setEmotion(player, "thunder_slash");
    else if (use.card->isKindOf("FireSlash"))
        room->setEmotion(player, "fire_slash");
    else if (use.card->isRed())
        room->setEmotion(player, "slash_red");
    else if (use.card->isBlack())
        room->setEmotion(player, "slash_black");
    else
        room->setEmotion(player, "killer");

    if (use.from->getMark("drank") > 0) {
        room->setCardFlag(use.card, "drank");
        use.card->setTag("drank", use.from->getMark("drank"));
        room->setPlayerMark(use.from, "drank", 0);
    }

    BasicCard::onUse(room, use);
}

void Slash::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (getNature() == DamageStruct::Thunder) {
        foreach (ServerPlayer *p, targets)
            room->setEmotion(p, "effects/thunder_slash");
    } else if (getNature() == DamageStruct::Fire) {
        foreach (ServerPlayer *p, targets)
            room->setEmotion(p, "effects/fire_slash");
    } else {
        foreach (ServerPlayer *p, targets)
            room->setEmotion(p, "effects/slash");
    }

    BasicCard::use(room, source, targets);
}

void Slash::onEffect(const CardEffectStruct &card_effect) const
{
    SlashEffectStruct effect;
    effect.from = card_effect.from;
    effect.nature = nature;
    effect.slash = this;

    effect.to = card_effect.to;
    effect.drank = this->tag["drank"].toInt();
    effect.nullified = card_effect.nullified;

    QVariantList jink_list = effect.from->tag["Jink_" + toString()].toList();
    effect.jink_num = jink_list.takeFirst().toInt();
    if (jink_list.isEmpty())
        effect.from->tag.remove("Jink_" + toString());
    else
        effect.from->tag["Jink_" + toString()] = QVariant::fromValue(jink_list);

    effect.from->getRoom()->slashEffect(effect);
}

bool Slash::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return !targets.isEmpty();
}

bool Slash::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty();
    int slash_targets = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    bool distance_limit = ((1 + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, this)) < 500);
    if (Self->hasFlag("slashNoDistanceLimit") || Self->hasFlag("IkXiashanUse") || Self->hasFlag("ThXuyouUse"))
        distance_limit = false;

    int rangefix = 0;
    if (Self->getWeapon() && subcards.contains(Self->getWeapon()->getId())) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += weapon->getRange() - Self->getAttackRange(false);
    }

    if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
        rangefix += 1;

    bool has_specific_assignee = false;
    foreach (const Player *p, Self->getAliveSiblings()) {
        if (Slash::IsSpecificAssignee(p, Self, this)) {
            has_specific_assignee = true;
            break;
        }
    }

    if (has_specific_assignee) {
        if (targets.isEmpty())
            return Slash::IsSpecificAssignee(to_select, Self, this)
                && Self->canSlash(to_select, this, distance_limit, rangefix);
        else {
            if (Self->hasFlag("slashDisableExtraTarget"))
                return false;
            bool canSelect = false;
            foreach (const Player *p, targets) {
                if (Slash::IsSpecificAssignee(p, Self, this)) {
                    canSelect = true;
                    break;
                }
            }
            if (!canSelect)
                return false;
        }
    }

    if (!Self->canSlash(to_select, this, distance_limit, rangefix, targets))
        return false;
    if (targets.length() >= slash_targets) {
        if (Self->hasSkill("ikxindu") && targets.length() == slash_targets) {
            QList<const Player *> ikxindu_targets;
            bool no_other_assignee = true;
            foreach (const Player *p, targets) {
                if (Self->distanceTo(p, rangefix) == 1)
                    ikxindu_targets << p;
                else if (no_other_assignee && Slash::IsSpecificAssignee(p, Self, this))
                    no_other_assignee = false;
            }
            if (no_other_assignee && ikxindu_targets.length() == 1
                && Slash::IsSpecificAssignee(ikxindu_targets.first(), Self, this))
                return Self->distanceTo(to_select, rangefix) == 1;
            return !ikxindu_targets.isEmpty() || Self->distanceTo(to_select, rangefix) == 1;
        } else
            return false;
    }

    return true;
}

Jink::Jink(Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("jink");
    target_fixed = true;
}

QString Jink::getSubtype() const
{
    return "defense_card";
}

bool Jink::isAvailable(const Player *) const
{
    return false;
}

Peach::Peach(Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("peach");
    target_fixed = true;
}

QString Peach::getSubtype() const
{
    return "recover_card";
}

bool Peach::targetFixed() const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        return BasicCard::targetFixed();
    if (Self && Self->hasSkill("rhchuilu"))
        return false;
    return BasicCard::targetFixed();
}

bool Peach::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self && Self->hasSkill("rhchuilu")) {
        int num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
        if (targets.length() >= num)
            return false;
        if (!to_select->isWounded())
            return false;
        return true;
    }
    return BasicCard::targetFilter(targets, to_select, Self);
}

bool Peach::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        return BasicCard::targetsFeasible(targets, Self);
    if (Self && Self->hasSkill("rhchuilu")) {
        return !targets.isEmpty() || Self->isWounded();
    }
    return BasicCard::targetsFeasible(targets, Self);
}

void Peach::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    BasicCard::onUse(room, use);
}

void Peach::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "peach");
    room->recover(effect.to, RecoverStruct(effect.from, this));
}

bool Peach::isAvailable(const Player *player) const
{
    if (player->hasFlag("ThChouceUse") || player->hasSkill("rhchuilu"))
        return BasicCard::isAvailable(player);
    return player->isWounded() && !player->isProhibited(player, this) && BasicCard::isAvailable(player);
}

Crossbow::Crossbow(Suit suit, int number)
    : Weapon(suit, number, 1)
{
    setObjectName("crossbow");
}

class DoubleSwordSkill : public WeaponSkill
{
public:
    DoubleSwordSkill()
        : WeaponSkill("double_sword")
    {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *to, use.to) {
            if (((use.from->isMale() && to->isFemale()) || (use.from->isFemale() && to->isMale()))
                && use.card->isKindOf("Slash")) {
                if (use.from->askForSkillInvoke(objectName())) {
                    to->getRoom()->setEmotion(use.from, "effects/weapon");

                    bool draw_card = false;
                    if (!to->canDiscard(to, "h"))
                        draw_card = true;
                    else {
                        QString prompt = "double-sword-card:" + use.from->objectName();
                        const Card *card = room->askForCard(to, ".", prompt, data);
                        if (!card)
                            draw_card = true;
                    }
                    if (draw_card)
                        use.from->drawCards(1, objectName());
                }
            }
        }

        return false;
    }
};

DoubleSword::DoubleSword(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("double_sword");
}

class QinggangSwordSkill : public WeaponSkill
{
public:
    QinggangSwordSkill()
        : WeaponSkill("qinggang_sword")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (WeaponSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                QStringList targets;
                foreach (ServerPlayer *to, use.to) {
                    if (to->getMark("Equips_of_Others_Nullified_to_You") > 0)
                        continue;
                    targets << to->objectName();
                }
                if (!targets.isEmpty())
                    return QStringList(objectName() + "->" + targets.join("+"));
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *p, QVariant &data, ServerPlayer *player) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!player->hasSkill("ikkongni")
            && ((p->getArmor() && p->hasArmorEffect(p->getArmor()->objectName())) || p->hasSkill("ikjingnie")
                || p->hasSkill("iklinglong")))
            room->setEmotion(player, "effects/weapon");
        p->addQinggangTag(use.card);

        QStringList list = room->getAchievementData(player, "jhsr", false).toString().split("\n", QString::SkipEmptyParts);
        if (!list.contains("qinggang_sword")) {
            room->setAchievementData(player, "jhsr", "qinggang_sword", false);
            room->addAchievementData(player, "jhsr", 1, false);
            if (room->getAchievementData(player, "jhsr", false, false).toInt() == 21) {
                const TriggerSkill *s = Sanguosha->getTriggerSkill("#achievement_jhsr");
                if (s && s->inherits("AchieveSkill")) {
                    const AchieveSkill *as = qobject_cast<const AchieveSkill *>(s);
                    as->gainAchievement(player, room);
                }
            }
        }
        return false;
    }
};

QinggangSword::QinggangSword(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("qinggang_sword");
}

class BladeSkill : public WeaponSkill
{
public:
    BladeSkill()
        : WeaponSkill("blade")
    {
        events << SlashMissed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!effect.to->isAlive() || effect.to->getMark("Equips_of_Others_Nullified_to_You") > 0)
            return false;
        if (!effect.from->canSlash(effect.to, NULL, false))
            return false;

        WrappedCard *weapon = player->getWeapon();
        int weapon_id = weapon ? weapon->getId() : -1;
        if (weapon_id != -1)
            room->setCardFlag(weapon_id, "using");
        effect.from->setFlags("BladeUse");
        bool use = room->askForUseSlashTo(effect.from, effect.to, QString("blade-slash:%1").arg(effect.to->objectName()), false,
                                          true);
        if (!use)
            effect.from->setFlags("-BladeUse");
        if (weapon_id != -1)
            room->setCardFlag(weapon_id, "-using");

        return use;
    }
};

Blade::Blade(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("blade");
}

class SpearSkill : public ViewAsSkill
{
public:
    SpearSkill()
        : ViewAsSkill("spear")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && player->getMark("Equips_Nullified_to_Yourself") == 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "slash" && player->getMark("Equips_Nullified_to_Yourself") == 0;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        Slash *slash = new Slash(Card::SuitToBeDecided, 0);
        slash->setSkillName(objectName());
        slash->addSubcards(cards);

        return slash;
    }
};

class SpearEmotion : public TriggerSkill
{
public:
    SpearEmotion()
        : TriggerSkill("#spear-emotion")
    {
        events << PreCardUsed << CardResponded;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        const Card *card = NULL;
        if (triggerEvent == PreCardUsed)
            card = data.value<CardUseStruct>().card;
        else
            card = data.value<CardResponseStruct>().m_card;
        if (card->isKindOf("Slash") && card->getSkillName() == "spear")
            room->setEmotion(player, "effects/weapon");
        return QStringList();
    }
};

Spear::Spear(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("spear");
}

class AxeViewAsSkill : public ViewAsSkill
{
public:
    AxeViewAsSkill()
        : ViewAsSkill("axe")
    {
        response_pattern = "@axe";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && to_select != Self->getWeapon() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        DummyCard *card = new DummyCard;
        card->setSkillName(objectName());
        card->addSubcards(cards);
        return card;
    }
};

class AxeSkill : public WeaponSkill
{
public:
    AxeSkill()
        : WeaponSkill("axe")
    {
        events << SlashMissed;
        view_as_skill = new AxeViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if (!effect.to->isAlive() || effect.to->getMark("Equips_of_Others_Nullified_to_You") > 0)
            return false;

        const Card *card = NULL;
        if (player->getCardCount() >= 3) // Need 2 more cards except from the weapon itself
            card = room->askForCard(player, "@axe", "@axe:" + effect.to->objectName(), data, objectName());
        if (card) {
            room->setEmotion(player, "effects/weapon");
            room->slashResult(effect, NULL);
        }

        return false;
    }
};

Axe::Axe(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("axe");
}

class HalberdSkill : public TargetModSkill
{
public:
    HalberdSkill()
        : TargetModSkill("halberd")
    {
        frequency = NotCompulsory;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const
    {
        if (from->hasWeapon("halberd") && from->isLastHandCard(card))
            return 2;
        else
            return 0;
    }
};

Halberd::Halberd(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("halberd");
}

class KylinBowSkill : public WeaponSkill
{
public:
    KylinBowSkill()
        : WeaponSkill("kylin_bow")
    {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        QStringList horses;
        if (damage.card && damage.card->isKindOf("Slash") && damage.by_user && !damage.chain && !damage.transfer
            && damage.to->getMark("Equips_of_Others_Nullified_to_You") == 0) {
            if (damage.to->getDefensiveHorse()
                && damage.from->canDiscard(damage.to, damage.to->getDefensiveHorse()->getEffectiveId()))
                horses << "dhorse";
            if (damage.to->getOffensiveHorse()
                && damage.from->canDiscard(damage.to, damage.to->getOffensiveHorse()->getEffectiveId()))
                horses << "ohorse";

            if (horses.isEmpty())
                return false;

            if (player == NULL)
                return false;
            if (!player->askForSkillInvoke(objectName(), data))
                return false;

            room->setEmotion(player, "effects/weapon");

            QString horse_type = room->askForChoice(player, objectName(), horses.join("+"));

            if (horse_type == "dhorse")
                room->throwCard(damage.to->getDefensiveHorse(), damage.to, damage.from);
            else if (horse_type == "ohorse")
                room->throwCard(damage.to->getOffensiveHorse(), damage.to, damage.from);
        }

        return false;
    }
};

KylinBow::KylinBow(Suit suit, int number)
    : Weapon(suit, number, 5)
{
    setObjectName("kylin_bow");
}

class EightDiagramSkill : public ArmorSkill
{
public:
    EightDiagramSkill()
        : ArmorSkill("eight_diagram")
    {
        events << CardAsked;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!ArmorSkill::triggerable(player))
            return QStringList();
        QString asked = data.toStringList().first();
        if (asked == "jink")
            return QStringList(objectName());

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (room->askForSkillInvoke(player, "eight_diagram")) {
            if (!player->getArmor() && player->hasSkill("ikjingnie")) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = "eight_diagram";
                room->sendLog(log);
                room->notifySkillInvoked(player, "ikjingnie");
            } else if (!player->getArmor() && player->hasSkill("iklinglong")) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = "eight_diagram";
                room->sendLog(log);
                room->notifySkillInvoked(player, "iklinglong");
            }
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        int armor_id = -1;
        if (player->getArmor()) {
            armor_id = player->getArmor()->getId();
            room->setCardFlag(armor_id, "using");
        }
        room->setEmotion(player, "effects/armor");
        JudgeStruct judge;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = objectName();
        judge.who = player;

        room->judge(judge);
        if (armor_id != -1)
            room->setCardFlag(armor_id, "-using");

        bool good = judge.isGood();

        if (player->hasSkill("jnjicha") && player->getHp() > 1 && player->getArmor() && player->getArmor()->isRed()) {
            room->sendCompulsoryTriggerLog(player, "jnjicha");
            room->broadcastSkillInvoke("jnjicha", 1);
            good = true;
        }

        if (good) {
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->setSkillName(objectName());
            room->provide(jink);

            return true;
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return -2;
    }
};

EightDiagram::EightDiagram(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("eight_diagram");
}

AmazingGrace::AmazingGrace(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("amazing_grace");
    has_preact = true;
}

void AmazingGrace::clearRestCards(Room *room) const
{
    room->clearAG();
    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    if (ag_list.isEmpty())
        return;
    DummyCard *dummy = new DummyCard(VariantList2IntList(ag_list));
    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "amazing_grace", QString());
    room->throwCard(dummy, reason, NULL);
    delete dummy;
}

void AmazingGrace::doPreAction(Room *room, const CardUseStruct &) const
{
    QList<int> card_ids = room->getNCards(room->getAllPlayers().length());
    room->fillAG(card_ids);
    room->setTag("AmazingGrace", IntList2VariantList(card_ids));
}

void AmazingGrace::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    try {
        room->doLightbox("anim=effects/amazing_grace");
        GlobalEffect::use(room, source, targets);
        clearRestCards(room);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            clearRestCards(room);
        throw triggerEvent;
    }
}

void AmazingGrace::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    QList<int> card_ids;
    foreach (QVariant card_id, ag_list)
        card_ids << card_id.toInt();

    int card_id = room->askForAG(effect.to, card_ids, false, objectName());
    card_ids.removeOne(card_id);

    room->takeAG(effect.to, card_id);
    ag_list.removeOne(card_id);

    room->setTag("AmazingGrace", ag_list);
}

GodSalvation::GodSalvation(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("god_salvation");
}

bool GodSalvation::isCancelable(const CardEffectStruct &effect) const
{
    return (effect.to->isWounded() || effect.to->isChained()) && TrickCard::isCancelable(effect);
}

void GodSalvation::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets) {
        if (p->isWounded() || p->isChained())
            room->setEmotion(p, "effects/god_salvation");
    }
    GlobalEffect::use(room, source, targets);
}

void GodSalvation::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (effect.to->isWounded())
        room->recover(effect.to, RecoverStruct(effect.from, this));
    if (effect.to->isChained())
        room->setPlayerProperty(effect.to, "chained", QVariant::fromValue(false));
}

SavageAssault::SavageAssault(Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("savage_assault");
}

void SavageAssault::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/savage_assault");
    AOE::use(room, source, targets);
}

void SavageAssault::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    const Card *slash
        = room->askForCard(effect.to, "slash", "savage-assault-slash:" + effect.from->objectName(), QVariant::fromValue(effect),
                           Card::MethodResponse, effect.from->isAlive() ? effect.from : NULL);
    if (slash) {
        room->setEmotion(effect.to, "killer");
    } else {
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to));
        room->getThread()->delay();
    }
}

ArcheryAttack::ArcheryAttack(Card::Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("archery_attack");
}

void ArcheryAttack::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->doLightbox("anim=effects/archery_attack");
    AOE::use(room, source, targets);
}

void ArcheryAttack::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    const Card *jink
        = room->askForCard(effect.to, "jink", "archery-attack-jink:" + effect.from->objectName(), QVariant::fromValue(effect),
                           Card::MethodResponse, effect.from->isAlive() ? effect.from : NULL);
    if (jink && jink->getSkillName() != "eight_diagram") {
        room->setEmotion(effect.to, "jink");
    } else if (!jink) {
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to));
        room->getThread()->delay();
    }
}

Collateral::Collateral(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("collateral");
}

bool Collateral::isAvailable(const Player *player) const
{
    bool canUse = player->hasFlag("ThChouceUse");
    foreach (const Player *p, player->getAliveSiblings()) {
        if (p->getWeapon()) {
            canUse = true;
            break;
        }
    }
    return canUse && SingleTargetTrick::isAvailable(player);
}

bool Collateral::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool Collateral::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) {
        // @todo: fix this. We should probably keep the codes here, but change the code in
        // roomscene such that if it is collateral, then targetFilter's result is overrode
        Q_ASSERT(targets.length() <= 2);
        if (targets.length() == 2)
            return false;
        const Player *slashFrom = targets[0];
        /* @todo: develop a new mechanism of filtering targets
                    to remove the coupling here and to fix the similar bugs caused by IkJinlian */
        if (to_select == Self && to_select->hasSkill("ikjingyou") && Self->isLastHandCard(this, true))
            return false;
        return slashFrom->canSlash(to_select);
    } else {
        if (!Self->hasFlag("ThChouceUse")) {
            if (!to_select->getWeapon() || to_select == Self)
                return false;
        }
        foreach (const Player *p, to_select->getAliveSiblings()) {
            if (to_select->canSlash(p) && (!(p == Self && p->hasSkill("ikjingyou") && Self->isLastHandCard(this, true))))
                return true;
        }
    }
    return false;
}

void Collateral::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct new_use = card_use;
    if (card_use.to.length() == 2) {
        ServerPlayer *killer = card_use.to.at(0);
        ServerPlayer *victim = card_use.to.at(1);

        new_use.to.removeAt(1);
        killer->tag["collateralVictim"] = QVariant::fromValue(victim);
    }

    SingleTargetTrick::onUse(room, new_use);
}

void Collateral::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    Q_ASSERT(targets.length() == 1);
    if (!targets.isEmpty()) {
        ServerPlayer *killer = targets.first();
        ServerPlayer *victim = killer->tag["collateralVictim"].value<ServerPlayer *>();
        room->setEmotion(killer, "effects/collateral");
        if (victim) {
            room->getThread()->delay(800);
            room->setEmotion(victim, "effects/collateral_slash");
        }
    }

    SingleTargetTrick::use(room, source, targets);
}

bool Collateral::doCollateral(Room *room, ServerPlayer *killer, ServerPlayer *victim, const QString &prompt) const
{
    bool useSlash = false;
    if (killer->canSlash(victim, NULL, false))
        useSlash = room->askForUseSlashTo(killer, victim, prompt);
    return useSlash;
}

void Collateral::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *killer = effect.to;
    ServerPlayer *victim = effect.to->tag["collateralVictim"].value<ServerPlayer *>();
    effect.to->tag.remove("collateralVictim");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, killer->objectName(), source->objectName(), objectName(), QString());
    if (!victim || victim->isDead()) {
        if (source->isAlive() && killer->isAlive() && killer->getWeapon())
            room->obtainCard(source, killer->getWeapon(), reason);
        return;
    }

    QString prompt;
    if (victim)
        prompt = QString("collateral-slash:%1:%2").arg(victim->objectName()).arg(source->objectName());

    room->setPlayerFlag(source, "CollateralSource");
    room->setPlayerFlag(killer, "CollateralUsing");
    if (source->isDead()) {
        if (killer->isAlive())
            doCollateral(room, killer, victim, prompt);
    } else {
        if (killer->isDead()) {
            ; // do nothing
        } else if (!killer->getWeapon()) {
            doCollateral(room, killer, victim, prompt);
        } else {
            if (!doCollateral(room, killer, victim, prompt)) {
                if (killer->getWeapon())
                    room->obtainCard(source, killer->getWeapon(), reason);
            }
        }
    }
    room->setPlayerFlag(source, "-CollateralSource");
    room->setPlayerFlag(killer, "-CollateralUsing");
}

Nullification::Nullification(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    target_fixed = true;
    setObjectName("nullification");
}

void Nullification::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    // does nothing, just throw it
    CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName());
    reason.m_extraData = QVariant::fromValue((const Card *)this);
    room->moveCardTo(this, source, NULL, Player::DiscardPile, reason);
}

bool Nullification::isAvailable(const Player *) const
{
    return false;
}

ExNihilo::ExNihilo(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("ex_nihilo");
    target_fixed = true;
}

void ExNihilo::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    SingleTargetTrick::onUse(room, use);
}

void ExNihilo::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/ex_nihilo");
    SingleTargetTrick::use(room, source, targets);
}

bool ExNihilo::isAvailable(const Player *player) const
{
    if (player->hasFlag("ThChouceUse"))
        return SingleTargetTrick::isAvailable(player);
    return !player->isProhibited(player, this) && SingleTargetTrick::isAvailable(player);
}

#include "ai.h"
#include "settings.h"
void ExNihilo::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    int extra = 0;
    if (room->getMode() == "06_3v3" && Config.value("3v3/OfficialRule", "2013").toString() == "2013") {
        int friend_num = 0, enemy_num = 0;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (AI::GetRelation3v3(effect.to, p) == AI::Friend)
                friend_num++;
            else
                enemy_num++;
        }
        if (friend_num < enemy_num)
            extra = 1;
    }
    effect.to->drawCards(2 + extra, "ex_nihilo");
}

Duel::Duel(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("duel");
}

void Duel::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->setEmotion(source, "effects/duel");
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/duel");
    SingleTargetTrick::use(room, source, targets);
}

bool Duel::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty();
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && to_select != Self;
}

void Duel::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *first = effect.to;
    ServerPlayer *second = effect.from;
    Room *room = first->getRoom();

    forever {
        if (!first->isAlive())
            break;
        if (second->tag["IkWushuang_" + toString()].toStringList().contains(first->objectName())) {
            const Card *slash = room->askForCard(first, "slash", "@ikwushuang-slash-1:" + second->objectName(),
                                                 QVariant::fromValue(effect), Card::MethodResponse, second);
            if (slash == NULL)
                break;

            slash = room->askForCard(first, "slash", "@ikwushuang-slash-2:" + second->objectName(), QVariant::fromValue(effect),
                                     Card::MethodResponse, second);
            if (slash == NULL)
                break;
        } else {
            const Card *slash = room->askForCard(first, "slash", "duel-slash:" + second->objectName(),
                                                 QVariant::fromValue(effect), Card::MethodResponse, second);
            if (slash == NULL)
                break;
        }

        qSwap(first, second);
    }

    DamageStruct damage(this, second->isAlive() ? second : NULL, first);
    if (second != effect.from)
        damage.by_user = false;
    room->damage(damage);
}

Snatch::Snatch(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("snatch");
}

bool Snatch::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty();
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    bool include_judging = !(ServerInfo.GameMode == "02_1v1" && ServerInfo.GameRuleMode != "Classical");
    if (targets.length() >= total_num || to_select->getCardCount(true, include_judging) == 0 || to_select == Self)
        return false;

    int distance_limit = 1 + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, this);
    int rangefix = 0;
    if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
        rangefix += 1;
    if (getSkillName() == "ikhuanwu")
        rangefix += 1;

    int distance = Self->distanceTo(to_select, rangefix);
    if (distance == -1 || distance > distance_limit)
        return false;

    return true;
}

void Snatch::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/snatch");
    SingleTargetTrick::use(room, source, targets);
}

void Snatch::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isDead())
        return;
    if (effect.to->isAllNude())
        return;

    Room *room = effect.to->getRoom();
    bool using_2013 = (room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical");
    QString flag = using_2013 ? "he" : "hej";
    int card_id = room->askForCardChosen(effect.from, effect.to, flag, objectName());
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
}

Dismantlement::Dismantlement(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("dismantlement");
}

bool Dismantlement::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty();
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    bool include_judging = !(ServerInfo.GameMode == "02_1v1" && ServerInfo.GameRuleMode != "Classical");
    return targets.length() < total_num && to_select->getCardCount(true, include_judging) > 0 && to_select != Self;
}

void Dismantlement::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets)
        room->setEmotion(p, "effects/dismantlement");
    SingleTargetTrick::use(room, source, targets);
}

void Dismantlement::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isDead())
        return;

    Room *room = effect.to->getRoom();
    bool using_2013 = (room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical");
    QString flag = using_2013 ? "he" : "hej";
    if (!effect.from->canDiscard(effect.to, flag))
        return;

    int card_id = -1;
    AI *ai = effect.from->getAI();
    if (!using_2013 || ai)
        card_id = room->askForCardChosen(effect.from, effect.to, flag, objectName(), false, Card::MethodDiscard);
    else {
        if (!effect.to->getEquips().isEmpty())
            card_id = room->askForCardChosen(effect.from, effect.to, flag, objectName(), false, Card::MethodDiscard);
        if (card_id == -1 || (!effect.to->isKongcheng() && effect.to->handCards().contains(card_id))) {
            LogMessage log;
            log.type = "$ViewAllCards";
            log.from = effect.from;
            log.to << effect.to;
            log.card_str = IntList2StringList(effect.to->handCards()).join("+");
            room->sendLog(log, effect.from);

            card_id = room->askForCardChosen(effect.from, effect.to, "h", objectName(), true, Card::MethodDiscard);
        }
    }
    room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : effect.to, effect.from);

    // for ThJianyue -----------
    if (getSkillName() == "thjianyue") {
        QStringList choices;
        if (effect.to->isAlive() && room->getCardPlace(card_id) == Player::DiscardPile)
            choices << "obtain";
        choices << "skip";
        QString choice = room->askForChoice(effect.from, "thjianyue", choices.join("+"), QVariant::fromValue(effect.to));
        if (choice == "obtain") {
            room->obtainCard(effect.to, card_id);
            QVariantList list = effect.to->tag["ThJianyue"].toList();
            list << card_id;
            effect.to->tag["ThJianyue"] = QVariant::fromValue(list);
            room->setPlayerCardLimitation(effect.to, "use,response", QString::number(card_id), false);
        } else {
            if (!effect.to->isChained())
                room->setPlayerProperty(effect.to, "chained", true);
            room->setPlayerFlag(effect.from, "ThJianyueSkip");
        }
    }
    // -------------------------
}

Indulgence::Indulgence(Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("indulgence");

    judge.pattern = ".|heart";
    judge.good = true;
    judge.reason = objectName();
}

bool Indulgence::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty() && !to_select->containsTrick(objectName());
    return targets.isEmpty() && !to_select->containsTrick(objectName()) && to_select != Self;
}

void Indulgence::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    Q_ASSERT(targets.length() == 1);
    if (!targets.isEmpty())
        room->setEmotion(targets.first(), "effects/indulgence");
    DelayedTrick::use(room, source, targets);
}

void Indulgence::takeEffect(ServerPlayer *target) const
{
    target->clearHistory();
    target->skip(Player::Play);
}

Disaster::Disaster(Card::Suit suit, int number)
    : DelayedTrick(suit, number, true)
{
    target_fixed = true;
}

bool Disaster::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse"))
        return targets.isEmpty() && !to_select->containsTrick(objectName());
    return DelayedTrick::targetFilter(targets, to_select, Self);
}

void Disaster::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    DelayedTrick::onUse(room, use);
}

bool Disaster::isAvailable(const Player *player) const
{
    if (player->hasFlag("ThChouceUse"))
        return DelayedTrick::isAvailable(player);
    if (player->containsTrick(objectName()))
        return false;

    return !player->isProhibited(player, this) && DelayedTrick::isAvailable(player);
}

Lightning::Lightning(Suit suit, int number)
    : Disaster(suit, number)
{
    setObjectName("lightning");
    can_recast = true;

    judge.pattern = ".|spade|2~9";
    judge.good = false;
    judge.reason = objectName();
}

bool Lightning::isAvailable(const Player *player) const
{
    if (player->hasFlag("ThChouceUse"))
        return !player->isCardLimited(this, Card::MethodUse) && Disaster::isAvailable(player);
    if (player->getGameMode() == "04_1v3" && !player->isLord() && !player->isCardLimited(this, Card::MethodRecast))
        return true;
    return !player->isCardLimited(this, Card::MethodUse) && Disaster::isAvailable(player);
}

void Lightning::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = card_use.from;
    if (room->getMode() == "04_1v3" && use.card->isKindOf("Lightning") && !player->isLord()
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
    Disaster::onUse(room, use);
}

void Lightning::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->doLightbox("anim=effects/lightning");
    Disaster::use(room, source, targets);
}

void Lightning::takeEffect(ServerPlayer *target) const
{
    target->getRoom()->damage(DamageStruct(this, NULL, target, 3, DamageStruct::Thunder));
}

class HorseSkill : public DistanceSkill
{
public:
    HorseSkill()
        : DistanceSkill("horse")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        const Horse *horse = NULL;
        if (from->getOffensiveHorse() && from->getMark("Equips_Nullified_to_Yourself") == 0
            && !(from->hasSkill("ikkongni") || to->hasSkill("ikkongni"))) {
            horse = qobject_cast<const Horse *>(from->getOffensiveHorse()->getRealCard());
            correct += horse->getCorrect();
        }
        if (to->getDefensiveHorse() && to->getMark("Equips_Nullified_to_Yourself") == 0
            && !(to->hasSkill("ikkongni") || from->hasSkill("ikkongni"))) {
            horse = qobject_cast<const Horse *>(to->getDefensiveHorse()->getRealCard());
            correct += horse->getCorrect();
        }

        return correct;
    }
};

StandardCardPackage::StandardCardPackage()
    : Package("standard_cards", Package::CardPack)
{
    QList<Card *> cards;

    // spade
    cards << new Duel(Card::Spade, 1) << new Lightning(Card::Spade, 1) << new DoubleSword << new EightDiagram(Card::Spade)
          << new Dismantlement(Card::Spade, 3) << new Snatch(Card::Spade, 3) << new Dismantlement(Card::Spade, 4)
          << new Snatch(Card::Spade, 4) << new Blade;

    DefensiveHorse *zijing = new DefensiveHorse(Card::Spade, 5);
    zijing->setObjectName("zijing");
    cards << zijing << new Indulgence(Card::Spade, 6) << new QinggangSword << new Slash(Card::Spade, 7)
          << new SavageAssault(Card::Spade, 7) << new Slash(Card::Spade, 8) << new Slash(Card::Spade, 8)
          << new Slash(Card::Spade, 9) << new Slash(Card::Spade, 9) << new Slash(Card::Spade, 10) << new Slash(Card::Spade, 10)
          << new Snatch(Card::Spade, 11) << new Nullification(Card::Spade, 11) << new Dismantlement(Card::Spade, 12)
          << new Spear << new SavageAssault(Card::Spade, 13);

    OffensiveHorse *wuyuzhou = new OffensiveHorse(Card::Spade, 13);
    wuyuzhou->setObjectName("wuyuzhou");
    cards << wuyuzhou;

    // heart
    cards << new ArcheryAttack << new GodSalvation << new Jink(Card::Heart, 2) << new Jink(Card::Heart, 2)
          << new Peach(Card::Heart, 3) << new AmazingGrace(Card::Heart, 3) << new Peach(Card::Heart, 4)
          << new AmazingGrace(Card::Heart, 4) << new KylinBow;

    OffensiveHorse *chanayakongxue = new OffensiveHorse(Card::Heart, 5);
    chanayakongxue->setObjectName("chanayakongxue");
    cards << chanayakongxue << new Peach(Card::Heart, 6) << new Indulgence(Card::Heart, 6) << new Peach(Card::Heart, 7)
          << new ExNihilo(Card::Heart, 7) << new Peach(Card::Heart, 8) << new ExNihilo(Card::Heart, 8)
          << new Peach(Card::Heart, 9) << new ExNihilo(Card::Heart, 9) << new Slash(Card::Heart, 10)
          << new Slash(Card::Heart, 10) << new Slash(Card::Heart, 11) << new ExNihilo(Card::Heart, 11)
          << new Peach(Card::Heart, 12) << new Dismantlement(Card::Heart, 12) << new Jink(Card::Heart, 13);

    DefensiveHorse *kunshentianye = new DefensiveHorse(Card::Heart, 13);
    kunshentianye->setObjectName("kunshentianye");
    cards << kunshentianye;

    // club
    cards << new Duel(Card::Club, 1) << new Crossbow(Card::Club) << new Slash(Card::Club, 2) << new EightDiagram(Card::Club)
          << new Slash(Card::Club, 3) << new Dismantlement(Card::Club, 3) << new Slash(Card::Club, 4)
          << new Dismantlement(Card::Club, 4) << new Slash(Card::Club, 5);

    DefensiveHorse *yaoshi = new DefensiveHorse(Card::Club, 5);
    yaoshi->setObjectName("yaoshi");
    cards << yaoshi << new Slash(Card::Club, 6) << new Indulgence(Card::Club, 6) << new Slash(Card::Club, 7)
          << new SavageAssault(Card::Club, 7) << new Slash(Card::Club, 8) << new Slash(Card::Club, 8)
          << new Slash(Card::Club, 9) << new Slash(Card::Club, 9) << new Slash(Card::Club, 10) << new Slash(Card::Club, 10)
          << new Slash(Card::Club, 11) << new Slash(Card::Club, 11) << new Collateral(Card::Club, 12)
          << new Nullification(Card::Club, 12) << new Collateral(Card::Club, 13) << new Nullification(Card::Club, 13);

    // diamond
    cards << new Duel(Card::Diamond, 1) << new Crossbow(Card::Diamond) << new Jink(Card::Diamond, 2)
          << new Jink(Card::Diamond, 2) << new Jink(Card::Diamond, 3) << new Snatch(Card::Diamond, 3)
          << new Slash(Card::Diamond, 4) << new Jink(Card::Diamond, 4) << new Jink(Card::Diamond, 5) << new Axe
          << new Slash(Card::Diamond, 6) << new Jink(Card::Diamond, 6) << new Slash(Card::Diamond, 7)
          << new Jink(Card::Diamond, 7) << new Slash(Card::Diamond, 8) << new Jink(Card::Diamond, 8)
          << new Slash(Card::Diamond, 9) << new Jink(Card::Diamond, 9) << new Slash(Card::Diamond, 10)
          << new Jink(Card::Diamond, 10) << new Jink(Card::Diamond, 11) << new Jink(Card::Diamond, 11)
          << new Peach(Card::Diamond, 12) << new Halberd << new Slash(Card::Diamond, 13);

    OffensiveHorse *chaoyaoguaifengjiang = new OffensiveHorse(Card::Diamond, 13);
    chaoyaoguaifengjiang->setObjectName("chaoyaoguaifengjiang");
    cards << chaoyaoguaifengjiang;

    skills << new DoubleSwordSkill << new QinggangSwordSkill << new BladeSkill << new SpearSkill << new AxeSkill
           << new KylinBowSkill << new EightDiagramSkill << new HalberdSkill;
    skills << new SpearEmotion;
    related_skills.insertMulti("spear", "#spear-emotion");
    skills << new HorseSkill;

    foreach (Card *card, cards)
        card->setParent(this);
}

ADD_PACKAGE(StandardCard)
