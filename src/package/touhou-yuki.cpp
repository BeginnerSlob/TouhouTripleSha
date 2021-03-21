#include "touhou-yuki.h"

#include "carditem.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"
#include "skill.h"
#include "touhou-hana.h"

class ThJianmo : public TriggerSkill
{
public:
    ThJianmo()
        : TriggerSkill("thjianmo")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            if (player->getMark("@imprison") > 0)
                room->setPlayerMark(player, "@imprison", 0);
            if (player->getMark("jianmouse")) {
                room->removePlayerMark(player, "jianmouse");
                room->removePlayerCardLimitation(player, "use,response", "Slash$0");
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() != Player::Play || player->getHandcardNum() < player->getMaxHp())
                return skill_list;
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (player == p)
                    continue;
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (room->askForChoice(player, objectName(), "jian+mo") == "jian") {
            LogMessage log;
            log.type = "#thjianmochoose1";
            log.from = player;
            log.arg = "1";
            room->sendLog(log);
            player->drawCards(1);
            room->addPlayerMark(player, "jianmouse");
            room->setPlayerCardLimitation(player, "use,response", "Slash", false);
        } else {
            LogMessage log;
            log.type = "#thjianmochoose2";
            log.from = player;
            log.arg = "2";
            room->sendLog(log);
            room->addPlayerMark(player, "@imprison");
        }
        return false;
    }
};

class ThJianmoDiscard : public TriggerSkill
{
public:
    ThJianmoDiscard()
        : TriggerSkill("#thjianmo")
    {
        events << CardUsed;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && player->getMark("@imprison"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        LogMessage log;
        log.type = "#ThJianmo";
        log.from = player;
        log.arg = "thjianmo";
        log.arg2 = use.card->objectName();
        room->sendLog(log);

        if (!room->askForDiscard(player, "thjianmo", 1, 1, true, true, "@thjianmo"))
            use.nullified_list << "_ALL_TARGETS";
        data = QVariant::fromValue(use);

        return false;
    }
};

class ThErchong : public TriggerSkill
{
public:
    ThErchong()
        : TriggerSkill("therchong")
    {
        events << EventPhaseChanging;
        frequency = Wake;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        TriggerList skill_list;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->isWounded() && p->getMark("@layer") >= 2 && p->getMark("@erchong") <= 0)
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        room->notifySkillInvoked(player, objectName());
        LogMessage log;
        log.type = "#ThErchong";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(player, "@layer", 0);
        room->addPlayerMark(player, "@erchong");

        if (room->changeMaxHpForAwakenSkill(player)) {
            room->acquireSkill(player, "thhuanfa");
            room->acquireSkill(player, "ikzhuji");
        }

        return false;
    }
};

class ThErchongRecord : public TriggerSkill
{
public:
    ThErchongRecord()
        : TriggerSkill("#therchong-record")
    {
        events << PreCardUsed << EventPhaseChanging << EventPhaseStart << EventAcquireSkill << EventLoseSkill << EventMarksGot
               << EventMarksLost;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == PreCardUsed && room->isSomeonesTurn(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                player->setFlags("ThErchongSlash");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                if (player->hasFlag("ThErchongSlash")) {
                    foreach (ServerPlayer *p, room->getAlivePlayers()) {
                        room->setPlayerMark(p, "layer", 0);
                        room->setPlayerMark(p, "@layer", 0);
                    }
                } else {
                    return QStringList(objectName());
                }
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            room->setPlayerMark(player, "layer", 0);
            room->setPlayerMark(player, "@layer", 0);
        } else if (triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill) {
            if (data.toString() != "therchong")
                return QStringList();
            int mark_num = player->getMark("layer");
            if (mark_num > 0) {
                int num = triggerEvent == EventAcquireSkill ? mark_num : 0;
                room->setPlayerMark(player, "@layer", num);
            }
        } else if (triggerEvent == EventMarksGot || triggerEvent == EventMarksLost) {
            if (data.toString() != "@erchong")
                return QStringList();
            int mark_num = player->getMark("layer");
            if (mark_num > 0) {
                int num = triggerEvent == EventMarksLost ? mark_num : 0;
                room->setPlayerMark(player, "@layer", num);
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            room->addPlayerMark(p, "layer");
            if (p->hasSkill("therchong") && p->getMark("@erchong") == 0)
                room->setPlayerMark(p, "@layer", p->getMark("layer"));
        }
        return false;
    }
};

ThHuanfaCard::ThHuanfaCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void ThHuanfaCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    CardMoveReason reason3(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "thhuanfa",
                           QString());
    room->obtainCard(effect.to, this, reason3);

    int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thhuanfa");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    targets.removeOne(effect.from);
    if (!targets.isEmpty()) {
        ServerPlayer *target
            = room->askForPlayerChosen(effect.from, targets, "thhuanfa", "@thhuanfa-give:" + effect.to->objectName(), true);
        if (target) {
            CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), target->objectName(), "thhuanfa",
                                   QString());
            room->obtainCard(target, Sanguosha->getCard(card_id), reason2, false);
        }
    }
}

class ThHuanfa : public OneCardViewAsSkill
{
public:
    ThHuanfa()
        : OneCardViewAsSkill("thhuanfa")
    {
        filter_pattern = ".|heart|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("ThHuanfaCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThHuanfaCard *huanfaCard = new ThHuanfaCard;
        huanfaCard->addSubcard(originalCard);
        return huanfaCard;
    }
};

class ThChundu : public TriggerSkill
{
public:
    ThChundu()
        : TriggerSkill("thchundu$")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player && player->isAlive() && player->hasLordSkill(objectName()) && player->canDiscard(player, "h")) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from != player && move.from->getKingdom() == "yuki" && move.to_place == Player::DiscardPile
                && move.from_places.contains(Player::PlaceTable)
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE) {
                const Card *card = move.reason.m_extraData.value<const Card *>();
                if (card->getSuit() == Card::Heart && card->getTypeId() == Card::TypeBasic)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForCard(player, ".", "@thchundu", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        DummyCard *dummy = new DummyCard(move.card_ids);
        player->obtainCard(dummy);
        delete dummy;
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);

        return false;
    }
};

class ThZuishangGivenSkill : public ViewAsSkill
{
public:
    ThZuishangGivenSkill()
        : ViewAsSkill("thzuishangv")
    {
        response_or_use = true;
        attached_lord_skill = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        Analeptic *card = new Analeptic(Card::SuitToBeDecided, -1);
        card->addSubcards(cards);
        card->setSkillName("thzuishang");
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        QString obj_name = player->property("zuishang_source").toString();
        const Player *source = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->objectName() == obj_name) {
                source = p;
                break;
            }
        }
        if (source && source->hasSkill("thzuishang"))
            return IsEnabledAtPlay(player);
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        QString obj_name = player->property("zuishang_source").toString();
        const Player *source = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->objectName() == obj_name) {
                source = p;
                break;
            }
        }
        if (source && source->hasSkill("thzuishang"))
            return IsEnabledAtResponse(player, pattern);
        return false;
    }

    bool IsEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player);
    }

    bool IsEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.contains("analeptic");
    }
};

class ThZuishangViewAsSkill : public ThZuishangGivenSkill
{
public:
    ThZuishangViewAsSkill()
        : ThZuishangGivenSkill()
    {
        attached_lord_skill = false;
        setObjectName("thzuishang");
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return IsEnabledAtPlay(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return player->getPhase() != Player::NotActive && IsEnabledAtResponse(player, pattern);
    }
};

class ThZuishang : public TriggerSkill
{
public:
    ThZuishang()
        : TriggerSkill("thzuishang")
    {
        events << EventPhaseStart << EventPhaseChanging << Death;
        view_as_skill = new ThZuishangViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart && player->hasSkill(objectName(), true) && player->isAlive()) {
                player->setFlags("ZuishangTurn");
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->hasSkill("thzuishangv"))
                        room->attachSkillToPlayer(p, "thzuishangv");
                    room->setPlayerProperty(p, "zuishang_source", player->objectName());
                }
            }
            return QStringList();
        }
        if (player != NULL && player->hasFlag("ZuishangTurn")) {
            if (triggerEvent == EventPhaseChanging) {
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.to != Player::NotActive)
                    return QStringList();
            }
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return QStringList();
            }
            player->setFlags("-ZuishangTurn");
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasSkill("thzuishangv"))
                    room->detachSkillFromPlayer(p, "thzuishangv", true);
                room->setPlayerProperty(p, "zuishang_source", "");
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return false;
    }
};

class ThXugu : public TriggerSkill
{
public:
    ThXugu()
        : TriggerSkill("thxugu")
    {
        events << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && room->isSomeonesTurn(player)) {
            if (!player->hasFlag("ThXuguFailed") && use.card->isKindOf("Analeptic"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *p = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thxugu", true, true);
        if (p) {
            player->tag["ThXuguTarget"] = QVariant::fromValue(p);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *victim = player->tag["ThXuguTarget"].value<ServerPlayer *>();
        player->tag.remove("ThXuguTarget");
        if (victim) {
            if (!room->askForUseCard(victim, "analeptic", "@thxugu-use:" + player->objectName(), -1, Card::MethodUse, false)) {
                player->setFlags("ThXuguFailed");
                if (player->canSlash(victim, false)) {
                    Slash *slash = new Slash(Card::NoSuit, 0);
                    slash->setSkillName("_thxugu");
                    room->useCard(CardUseStruct(slash, player, victim));
                }
            }
        }
        return false;
    }
};

class ThShenzhan : public TriggerSkill
{
public:
    ThShenzhan()
        : TriggerSkill("thshenzhan")
    {
        events << Damage;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && !player->hasFlag("Global_DebutFlag") && player->getSlashCount() > 0) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->getTypeId() != Card::TypeSkill && damage.card->getTypeId() != Card::TypeBasic
                && damage.to != player) {
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        room->addPlayerHistory(player, "Slash", 0);
        room->addPlayerHistory(player, "FireSlash", 0);
        room->addPlayerHistory(player, "ThunderSlash", 0);

        return false;
    }
};

class ThHunqieVS : public OneCardViewAsSkill
{
public:
    ThHunqieVS()
        : OneCardViewAsSkill("thhunqie")
    {
        filter_pattern = ".|black";
        response_or_use = true;
        response_pattern = "@@thhunqie";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        slash->addSubcard(originalCard);
        slash->setSkillName(objectName());
        return slash;
    }
};

class ThHunqie : public TriggerSkill
{
public:
    ThHunqie()
        : TriggerSkill("thhunqie")
    {
        events << EventPhaseEnd;
        view_as_skill = new ThHunqieVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Play && target->getSlashCount() == 0;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thhunqie", "@thhunqie");
    }
};

class ThDaojian : public TriggerSkill
{
public:
    ThDaojian()
        : TriggerSkill("thdaojian")
    {
        events << GameStart;
        frequency = Compulsory;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player)
            return QStringList();
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(Sanguosha->getSkill("qinggang_sword"));
        room->getThread()->addTriggerSkill(trigger_skill);
        return QStringList();
    }
};

class ThZhancao : public TriggerSkill
{
public:
    ThZhancao()
        : TriggerSkill("thzhancao")
    {
        events << TargetConfirming << BeforeCardsMove;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (room->isSomeonesTurn(p))
                        continue;
                    if (p->inMyAttackRange(player) || p == player)
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->tag["thzhancao_user"].toBool()) {
                        const Card *zhancao_card = move.reason.m_extraData.value<const Card *>();
                        if (zhancao_card && zhancao_card->hasFlag("thzhancao"))
                            skill_list.insert(p, QStringList(objectName()));
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (triggerEvent == TargetConfirming) {
            ask_who->tag["ThZhancaoData"] = data; // for AI
            if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
                ask_who->tag.remove("ThZhancaoData");
                room->broadcastSkillInvoke(objectName());
                if (!room->askForCard(ask_who, "^BasicCard", "@thzhancao")) {
                    room->loseHp(ask_who);
                    CardUseStruct use = data.value<CardUseStruct>();
                    const Card *card = use.card;
                    QList<int> ids;
                    if (!card->isVirtualCard())
                        ids << card->getEffectiveId();
                    else if (card->subcardsLength() > 0)
                        ids = card->getSubcards();
                    if (!ids.isEmpty()) {
                        room->setCardFlag(card, "thzhancao");
                        ask_who->tag["thzhancao_user"] = true;
                    }
                }
                return true;
            }
            ask_who->tag.remove("ThZhancaoData");
        } else if (triggerEvent == BeforeCardsMove) {
            ask_who->tag["thzhancao_user"] = false;
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                        ServerPlayer *ask_who) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetConfirming) {
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if (card) {
                room->setCardFlag(card, "-thzhancao");
                ask_who->obtainCard(card);
                move.removeCardIds(move.card_ids);
                data = QVariant::fromValue(move);
            }
        }
        return false;
    }
};

ThMojiCard::ThMojiCard()
{
    will_throw = false;
}

bool ThMojiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->setSkillName("_thmoji");
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;

    Card *card = Sanguosha->cloneCard(user_string.split("+").first());
    card->setSkillName("_thmoji");
    card->addSubcards(subcards);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool ThMojiCard::targetFixed() const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->setSkillName("_thmoji");
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetFixed();
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    Card *card = Sanguosha->cloneCard(user_string.split("+").first());
    card->setSkillName("_thmoji");
    card->addSubcards(subcards);
    card->deleteLater();
    return card && card->targetFixed();
}

bool ThMojiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->setSkillName("_thmoji");
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;

    Card *card = Sanguosha->cloneCard(user_string.split("+").first());
    card->setSkillName("_thmoji");
    card->addSubcards(subcards);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *ThMojiCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *user = card_use.from;
    Room *room = user->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, user->objectName(), "thmoji", QString());
    room->moveCardTo(this, NULL, Player::DrawPile, reason, false);

    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("_thmoji");
    use_card->deleteLater();
    return use_card;
}

const Card *ThMojiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, user->objectName(), "thmoji", QString());
    room->moveCardTo(this, NULL, Player::DrawPile, reason, false);

    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("_thmoji");
    use_card->deleteLater();
    return use_card;
}

class ThMoji : public ViewAsSkill
{
public:
    ThMoji()
        : ViewAsSkill("thmoji")
    {
    }

    virtual SkillDialog *getDialog() const
    {
        return ThMimengDialog::getInstance("thmoji", true, false);
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        int n = qMin(2, Self->getHp());
        if (selected.length() >= n)
            return false;
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != qMin(2, Self->getHp()))
            return NULL;

        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
            ThMojiCard *tianyan_card = new ThMojiCard;
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic")
                pattern = "analeptic";
            tianyan_card->addSubcards(cards);
            tianyan_card->setUserString(pattern);
            return tianyan_card;
        }

        const Card *c = Self->tag["thmoji"].value<const Card *>();
        if (c) {
            ThMojiCard *card = new ThMojiCard;
            card->addSubcards(cards);
            card->setUserString(c->objectName());
            return card;
        }

        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getHp() < 1 || player->getCardCount() < qMin(player->getHp(), 2))
            return false;

        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        if (slash->isAvailable(player))
            return true;

        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->deleteLater();
        if (analeptic->isAvailable(player))
            return true;

        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getHp() < 1 || player->getCardCount() < qMin(player->getHp(), 2))
            return false;

        return pattern.contains("analeptic") || pattern == "jink" || pattern == "slash";
    }
};

ThYuanqiCard::ThYuanqiCard()
{
    target_fixed = true;
}

void ThYuanqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    Card::Suit suit = room->askForSuit(source, "thyuanqi");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = source;
    log.arg = Suit2String(suit);
    room->sendLog(log);

    QList<int> card_ids = room->getNCards(1, false);
    CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "thyuanqi", QString());
    room->moveCardsAtomic(CardsMoveStruct(card_ids, NULL, Player::PlaceTable, reason), true);
    const Card *card = Sanguosha->getCard(card_ids.first());
    if (card->getSuit() == suit) {
        if (source->askForSkillInvoke("thyuanqi_draw", "yes"))
            source->drawCards(2, objectName());
    } else if (card->getColor() == Suit2Color(suit)) {
        source->tag["ThYuanqiCards"] = QVariant::fromValue(IntList2VariantList(card_ids));
        ServerPlayer *target = room->askForPlayerChosen(source, room->getAlivePlayers(), "thyuanqi", "@thyuanqi", true);
        source->tag.remove("ThYuanqiCards");
        if (target) {
            QList<CardsMoveStruct> moves;
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thyuanqi",
                                  QString());
            moves << CardsMoveStruct(card_ids, target, Player::PlaceHand, reason);
            room->moveCardsAtomic(moves, true);
        }
    }

    if (room->getCardPlace(card_ids.first()) == Player::PlaceTable) {
        QList<CardsMoveStruct> moves;
        CardMoveReason reason(CardMoveReason::S_REASON_PUT, source->objectName(), "thyuanqi", QString());
        moves << CardsMoveStruct(card_ids, NULL, Player::DiscardPile, reason);
        room->moveCardsAtomic(moves, true);
    }
}

class ThYuanqi : public ZeroCardViewAsSkill
{
public:
    ThYuanqi()
        : ZeroCardViewAsSkill("thyuanqi")
    {
    }

    virtual const Card *viewAs() const
    {
        return new ThYuanqiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThYuanqiCard");
    }
};

class ThDunjia : public TriggerSkill
{
public:
    ThDunjia()
        : TriggerSkill("thdunjia")
    {
        events << TargetConfirming;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && use.card->getTypeId() != Card::TypeSkill && use.to.length() == 1 && use.from
            && use.from->isAlive() && use.from->getEquips().length() < player->getEquips().length())
            return QStringList(objectName());
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1, objectName());
        return false;
    }
};

class ThQingming : public TriggerSkill
{
public:
    ThQingming()
        : TriggerSkill("thqingming")
    {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                QStringList targets;
                foreach (ServerPlayer *to, use.to) {
                    if (to->getEquips().length() > player->getEquips().length())
                        targets << to->objectName();
                }
                if (!targets.isEmpty())
                    return QStringList(objectName() + "->" + targets.join("+"));
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &data, ServerPlayer *player) const
    {
        player->tag["ThQingmingUse"] = data; //for AI
        bool invoke = player->askForSkillInvoke(objectName(), QVariant::fromValue(skill_target));
        player->tag.remove("ThQingmingUse");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *p, QVariant &data, ServerPlayer *player) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << p->objectName();
        data = QVariant::fromValue(use);

        QStringList choices;
        QStringList cards;
        cards << "rout"
              << "snatch"
              << "dismantlement"
              << "duel";
        foreach (QString card_name, cards) {
            Card *card = Sanguosha->cloneCard(card_name);
            card->setSkillName("_thqingming");
            if (card->targetFilter(QList<const Player *>(), p, player))
                choices << card_name;
            delete card;
        }

        if (!choices.isEmpty()) {
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant::fromValue(p));
            Card *card = Sanguosha->cloneCard(choice);
            card->setSkillName("_thqingming");
            card->deleteLater();
            room->useCard(CardUseStruct(card, player, p));
        }

        return false;
    }
};

ThChouceCard::ThChouceCard()
{
    target_fixed = true;
}

const Card *ThChouceCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;
    Room *room = player->getRoom();
    const Card *new_card = NULL;
    room->setPlayerFlag(player, "ThChouceUse");
    int n = player->getMark("ThChouce");
    if (n > 0)
        room->setPlayerCardLimitation(player, "use", QString("^SkillCard|.|~%1").arg(n), false);
    CardUseStruct::CardUseReason reason = Sanguosha->getCurrentCardUseReason();
    if (reason == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        new_card = room->askForUseCard(player, user_string, "@thchouce");
    else if (reason == CardUseStruct::CARD_USE_REASON_PLAY) {
        CardUseStruct new_card_use;
        room->activate(player, new_card_use);
        new_card = new_card_use.card;
        if (new_card != NULL)
            room->useCard(new_card_use, true);
    }
    if (!new_card) {
        if (n > 0)
            room->removePlayerCardLimitation(player, "use", QString("^SkillCard|.|~%1$0").arg(n));
        room->setPlayerFlag(player, "Global_ThChouceFailed");
        room->setPlayerFlag(player, "-ThChouceUse");
        return NULL;
    }
    return this;
}

const Card *ThChouceCard::validateInResponse(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    const Card *new_card = NULL;
    room->setPlayerFlag(player, "ThChouceUse");
    int n = player->getMark("ThChouce");
    if (n > 0)
        room->setPlayerCardLimitation(player, "use", QString("^SkillCard|.|~%1").arg(n), false);
    CardUseStruct::CardUseReason reason = Sanguosha->getCurrentCardUseReason();
    if (reason == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        new_card = room->askForUseCard(player, user_string, "@thchouce");
    else if (reason == CardUseStruct::CARD_USE_REASON_PLAY) {
        CardUseStruct new_card_use;
        room->activate(player, new_card_use);
        new_card = new_card_use.card;
        if (new_card != NULL)
            room->useCard(new_card_use, true);
    }
    if (!new_card) {
        if (n > 0)
            room->removePlayerCardLimitation(player, "use", QString("^SkillCard|.|~%1$0").arg(n));
        room->setPlayerFlag(player, "Global_ThChouceFailed");
        room->setPlayerFlag(player, "-ThChouceUse");
        return NULL;
    }
    return this;
}

void ThChouceCard::onUse(Room *, const CardUseStruct &) const
{
    // do nothing
}

class ThChouce : public ZeroCardViewAsSkill
{
public:
    ThChouce()
        : ZeroCardViewAsSkill("thchouce")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("ThChouce") < 13 && !player->hasFlag("Global_ThChouceFailed") && !player->hasFlag("ThChouceUse");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (pattern == "slash" || pattern == "peach" || pattern.contains("analeptic"))
            return player->getPhase() == Player::Play
                && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
                && player->getMark("ThChouce") < 13 && !player->hasFlag("Global_ThChouceFailed")
                && !player->hasFlag("ThChouceUse");
        return false;
    }

    virtual const Card *viewAs() const
    {
        ThChouceCard *card = new ThChouceCard;
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            card->setUserString(Sanguosha->getCurrentCardUsePattern());
        return card;
    }
};

class ThChouceRecord : public TriggerSkill
{
public:
    ThChouceRecord()
        : TriggerSkill("#thchouce")
    {
        events << PreCardUsed << PreCardResponded << EventPhaseChanging;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Play) {
                room->setPlayerMark(player, "ThChouce", 0);
                room->setPlayerMark(player, "@augury", 0);
                room->setPlayerFlag(player, "-ThChouce_failed");
            }
        } else if (triggerEvent == PreCardUsed || triggerEvent == PreCardResponded) {
            if (player->getPhase() != Player::Play)
                return QStringList();
            const Card *usecard = NULL;
            if (triggerEvent == PreCardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                usecard = use.card;
            } else if (triggerEvent == PreCardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    usecard = resp.m_card;
            }
            if (usecard && usecard->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        const Card *usecard = NULL;
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            usecard = use.card;
        } else if (triggerEvent == PreCardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse)
                usecard = resp.m_card;
        }
        room->setPlayerMark(player, "ThChouce", usecard->getNumber());
        if (!usecard->hasFlag("thchouce_use"))
            room->setPlayerFlag(player, "ThChouce_failed");
        else
            room->addPlayerMark(player, "@augury");
        return false;
    }
};

class ThZhanshi : public TriggerSkill
{
public:
    ThZhanshi()
        : TriggerSkill("thzhanshi")
    {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Play && !target->hasFlag("ThChouce_failed")
            && target->getMark("@augury") >= 3;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(target, objectName());
        target->addMark("thzhanshi");
        target->gainAnExtraTurn();
        return false;
    }
};

class ThZhanshiClear : public TriggerSkill
{
public:
    ThZhanshiClear()
        : TriggerSkill("#thzhanshi-clear")
    {
        events << EventPhaseChanging << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && target->getPhase() == Player::RoundStart && target->getMark("thzhanshi") > 0) {
            target->removeMark("thzhanshi");
            target->addMark("thhuanzang");
            room->acquireSkill(target, "thhuanzang");
        } else if (triggerEvent == EventPhaseChanging && target->getMark("thhuanzang") > 0) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                target->removeMark("thhuanzang");
                room->detachSkillFromPlayer(target, "thhuanzang", false, true);
            }
        }
        return QStringList();
    }
};

class ThHuanzang : public TriggerSkill
{
public:
    ThHuanzang()
        : TriggerSkill("thhuanzang")
    {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Play;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());

        JudgeStruct judge;
        judge.pattern = ".|black";
        judge.good = false;
        judge.negative = true;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        if (judge.isBad())
            room->loseHp(player);

        return false;
    }
};

class ThZiyun : public ProhibitSkill
{
public:
    ThZiyun()
        : ProhibitSkill("thziyun")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        if (to->hasSkill(objectName()))
            return card->isKindOf("SupplyShortage") || card->isKindOf("Lightning");

        return false;
    }
};

class ThChuiji : public TriggerSkill
{
public:
    ThChuiji()
        : TriggerSkill("thchuiji")
    {
        events << CardsMoveOneTime << EventPhaseChanging << EventPhaseEnd;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            player->setMark(objectName(), 0);
            return QStringList();
        }
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() != Player::Discard || player->getMark(objectName()) < 2)
                return QStringList();
        }
        if (triggerEvent == CardsMoveOneTime) {
            if (room->isSomeonesTurn(player))
                return QStringList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player
                || !(move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                || !(move.to != player || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip)))
                return QStringList();
        }
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        if (judge.card->isRed()) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->isWounded())
                    targets << p;

            if (targets.isEmpty())
                return false;

            player->tag["ThChuijiTarget"] = "recover";
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            RecoverStruct recover;
            recover.who = player;
            room->recover(target, recover);
        } else if (judge.card->isBlack()) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (player->canDiscard(p, "he"))
                    targets << p;

            if (targets.isEmpty())
                return false;

            player->tag["ThChuijiTarget"] = "discard";
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            int card_id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, target, player);
        }

        return false;
    }
};

class ThChuijiRecord : public TriggerSkill
{
public:
    ThChuijiRecord()
        : TriggerSkill("#thchuiji")
    {
        events << CardsMoveOneTime;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player) {
            if (player->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->addMark("thchuiji", data.value<CardsMoveOneTimeStruct>().card_ids.length());
        return false;
    }
};

class ThLingya : public TriggerSkill
{
public:
    ThLingya()
        : TriggerSkill("thlingya")
    {
        events << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&ask_who) const
    {
        ServerPlayer *current = room->getCurrent();
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(current) && current->getPhase() != Player::NotActive && use.card->isRed()
            && use.card->getTypeId() != Card::TypeSkill && player != current) {
            ask_who = current;
            return QStringList(objectName());
        }
        return QStringList();
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
        choices << "letdraw";
        if (ask_who->canDiscard(player, "he"))
            choices << "discard";

        QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant::fromValue(ask_who));
        if (choice == "discard") {
            int card_id = room->askForCardChosen(ask_who, player, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, player, ask_who);
        } else
            ask_who->drawCards(1);

        return false;
    }
};

class ThHeimu : public TriggerSkill
{
public:
    ThHeimu()
        : TriggerSkill("thheimu")
    {
        events << TargetSpecifying;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && !player->hasUsed("ThHeimu")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->getPhase() == Player::Play && use.card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            bool can = true;
            foreach (ServerPlayer *to, use.to) {
                if (p->isProhibited(to, use.card)) {
                    can = false;
                    break;
                }
            }
            if (!can)
                continue;
            targets << p;
        }

        player->tag["ThHeimuCardUse"] = data;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thheimu", true, true);
        player->tag.remove("ThHeimuCardUse");
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThHeimuTarget"] = QVariant::fromValue(target);
            room->addPlayerHistory(player, "ThHeimu");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThHeimuTarget"].value<ServerPlayer *>();
        player->tag.remove("ThHeimuTarget");
        if (target) {
            CardUseStruct use = data.value<CardUseStruct>();
            LogMessage log;
            log.type = "#BecomeUser";
            log.from = target;
            log.card_str = use.card->toString();
            room->sendLog(log);

            use.from = target;
            use.m_isOwnerUse = false;
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class ThHanpo : public TriggerSkill
{
public:
    ThHanpo()
        : TriggerSkill("thhanpo")
    {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(p)) {
            DamageStruct damage = d.value<DamageStruct>();
            if (damage.nature == DamageStruct::Fire)
                return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        r->sendCompulsoryTriggerLog(p, objectName());
        return true;
    }
};

class ThJidong : public TriggerSkill
{
public:
    ThJidong()
        : TriggerSkill("thjidong")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *r, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, r->getAlivePlayers()) {
                if (p->getMark("thjidongtarget") > 0)
                    p->setMark("thjidongtarget", 0);
                if (p->getMark("thjidong") > 0)
                    p->setMark("thjidong", 0);
            }
            return QStringList();
        }

        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play)
            return QStringList(objectName());

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target
            = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thjidong", true, true);
        if (target) {
            player->tag["ThJidongTarget"] = QVariant::fromValue(target);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThJidongTarget"].value<ServerPlayer *>();
        player->tag.remove("ThJidongTarget");
        if (target)
            target->addMark("thjidongtarget");
        int n = 0;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            foreach (const Card *c, p->getCards("ej")) {
                if (c->getSuit() == Card::Diamond)
                    ++n;
            }
        }
        if (n > 0) {
            target->drawCards(n, objectName());
            room->askForDiscard(target, objectName(), n, n, false, true);
        }
        return false;
    }
};

class ThJidongRecord : public TriggerSkill
{
public:
    ThJidongRecord()
        : TriggerSkill("#thjidong-record")
    {
        events << CardsMoveOneTime;
        global = true;
        frequency = NotCompulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player && player->isAlive() && player->getMark("thjidongtarget") > 0) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player
                && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && (move.to != player || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip)))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach (Player::Place place, move.from_places) {
            if (place == Player::PlaceHand || place == Player::PlaceEquip)
                player->addMark("thjidong");
        }
        return false;
    }
};

class ThJidongTrigger : public TriggerSkill
{
public:
    ThJidongTrigger()
        : TriggerSkill("#thjidong")
    {
        events << EventPhaseEnd;
        frequency = NotCompulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player && player->isAlive() && player->getPhase() == Player::Play) {
            QStringList objs;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("thjidong") > 1)
                    objs << p->objectName();
            }
            if (!objs.isEmpty())
                return QStringList(objectName() + "->" + objs.join("+"));
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *owner) const
    {
        room->sendCompulsoryTriggerLog(owner, "thjidong");

        player->turnOver();
        player->drawCards(2, "thjidong");
        return false;
    }
};

ThBingpuCard::ThBingpuCard()
{
    target_fixed = true;
}

void ThBingpuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->removePlayerMark(source, "@bingpu");
    room->addPlayerMark(source, "@bingpuused");
    foreach (ServerPlayer *target, room->getOtherPlayers(source)) {
        if (target->isNude())
            continue;

        if (!room->askForCard(target, "jink", "@thbingpu:" + source->objectName(), QVariant(), MethodResponse)) {
            int card_id = room->askForCardChosen(source, target, "he", "thbingpu", false, MethodDiscard);
            room->throwCard(card_id, target, source);

            if (!target->isNude()) {
                card_id = room->askForCardChosen(source, target, "he", "thbingpu", false, MethodDiscard);
                room->throwCard(card_id, target, source);
            }
        }
    }
}

class ThBingpu : public ZeroCardViewAsSkill
{
public:
    ThBingpu()
        : ZeroCardViewAsSkill("thbingpu")
    {
        frequency = Limited;
        limit_mark = "@bingpu";
    }

    virtual const Card *viewAs() const
    {
        return new ThBingpuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@bingpu") > 0;
    }
};

ThDongmoCard::ThDongmoCard()
{
}

bool ThDongmoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.size() < Self->getLostHp() && to_select != Self;
}

void ThDongmoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    targets << source;
    room->sortByActionOrder(targets);
    foreach (ServerPlayer *p, targets)
        p->turnOver();
    foreach (ServerPlayer *p, targets)
        p->drawCards(1);
}

class ThDongmoViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThDongmoViewAsSkill()
        : ZeroCardViewAsSkill("thdongmo")
    {
        response_pattern = "@@thdongmo";
    }

    virtual const Card *viewAs() const
    {
        return new ThDongmoCard;
    }
};

class ThDongmo : public TriggerSkill
{
public:
    ThDongmo()
        : TriggerSkill("thdongmo")
    {
        events << EventPhaseStart;
        view_as_skill = new ThDongmoViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Finish && target->isWounded();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(player, "@@thdongmo", "@thdongmo");
        return false;
    }
};

class ThLinhan : public TriggerSkill
{
public:
    ThLinhan()
        : TriggerSkill("thlinhan")
    {
        events << CardResponded;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardResponseStruct resp = data.value<CardResponseStruct>();
        if (!resp.m_card->isKindOf("Jink"))
            return QStringList();
        return QStringList(objectName());
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
        player->drawCards(1);
        return false;
    }
};

class ThFusheng : public TriggerSkill
{
public:
    ThFusheng()
        : TriggerSkill("thfusheng")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->isSkipped(Player::Draw))
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Draw)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            player->skip(Player::Draw, true);
            player->setFlags("ThFushengUsed");
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->tag.remove("ThFushengDraws");
        player->tag.remove("ThFushengDiscards");
        QStringList draws, discards;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            QStringList choices;
            choices << "draw";
            if (player->canDiscard(player, "he"))
                choices << "discard";
            QString choice = room->askForChoice(p, objectName(), choices.join("+"), QVariant::fromValue(player));
            LogMessage log;
            log.type = "#ThFusheng";
            log.from = p;
            log.arg = objectName() + ":" + choice;
            if (choice == "discard") {
                discards << p->objectName();
                room->askForDiscard(player, objectName(), 1, 1, false, true);
            } else {
                draws << p->objectName();
                player->drawCards(1, objectName());
            }
        }

        if (!draws.isEmpty())
            player->tag["ThFushengDraws"] = QVariant::fromValue(draws);
        if (!discards.isEmpty())
            player->tag["ThFushengDiscards"] = QVariant::fromValue(discards);

        return false;
    }
};

class ThFushengEffect : public PhaseChangeSkill
{
public:
    ThFushengEffect()
        : PhaseChangeSkill("#thfusheng")
    {
        frequency = NotCompulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive() && target->hasFlag("ThFushengUsed") && target->getPhase() == Player::Discard
            && target->getHandcardNum() != qMax(target->getHp(), 0);
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        room->sendCompulsoryTriggerLog(target, "thfusheng");
        target->setFlags("-ThFushengUsed");
        QStringList targets;
        bool give = true;
        if (target->getHandcardNum() > qMax(target->getHp(), 0))
            targets = target->tag["ThFushengDraws"].value<QStringList>();
        else {
            give = false;
            targets = target->tag["ThFushengDiscards"].value<QStringList>();
        }
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (!targets.contains(p->objectName()))
                continue;
            if (give) {
                if (target->isNude())
                    break;
                const Card *card
                    = room->askForCard(target, "..!", "@thfusheng-give:" + p->objectName(), QVariant(), Card::MethodNone);
                if (!card) {
                    QList<const Card *> cards = target->getCards("he");
                    card = cards.at(qrand() % cards.length());
                }
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), p->objectName(), "thfusheng",
                                      QString());
                room->obtainCard(p, card, reason, false);
            } else {
                if (p->isNude())
                    continue;
                const Card *card
                    = room->askForCard(p, "..!", "@thfusheng-give:" + target->objectName(), QVariant(), Card::MethodNone);
                if (!card) {
                    QList<const Card *> cards = p->getCards("he");
                    card = cards.at(qrand() % cards.length());
                }
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, p->objectName(), target->objectName(), "thfusheng",
                                      QString());
                room->obtainCard(target, card, reason, false);
            }
        }
        return false;
    }
};

class ThSaozang : public TriggerSkill
{
public:
    ThSaozang()
        : TriggerSkill("thsaozang")
    {
        events << CardsMoveOneTime << EventPhaseStart << EventPhaseEnd;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::Discard)
            return QStringList();

        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.to_place == Player::DiscardPile
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                QStringList list = player->tag.value("ThSaozang").toStringList();
                foreach (int id, move.card_ids) {
                    const Card *card = Sanguosha->getEngineCard(id);
                    if (!list.contains(card->getType()))
                        list << card->getType();
                }
                player->tag["ThSaozang"] = QVariant::fromValue(list);
            }
        } else if (triggerEvent == EventPhaseStart)
            player->tag["ThSaozang"] = QVariant::fromValue(QStringList());
        else if (triggerEvent == EventPhaseEnd) {
            QStringList list = player->tag.value("ThSaozang").toStringList();
            int n = list.length();
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (player->canDiscard(p, "h")) {
                    QStringList skills;
                    for (int i = 0; i < n; i++)
                        skills << objectName();
                    return skills;
                }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (player->canDiscard(p, "h"))
                victims << p;

        ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName(), "@thsaozang", true, true);
        if (victim) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThSaozangTarget"] = QVariant::fromValue(victim);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThSaozangTarget"].value<ServerPlayer *>();
        player->tag.remove("ThSaozangTarget");
        if (target && player->canDiscard(target, "h")) {
            int card_id = room->askForCardChosen(player, target, "h", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, target, player);
        }

        return false;
    }
};

class ThXuqu : public TriggerSkill
{
public:
    ThXuqu()
        : TriggerSkill("thxuqu")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceHand)
            && (move.to != player || move.to_place != Player::PlaceEquip)) {
            if (!room->isSomeonesTurn(player))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *victim
            = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thxuqu", true, true);
        if (victim) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThXuquTarget"] = QVariant::fromValue(victim);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThXuquTarget"].value<ServerPlayer *>();
        player->tag.remove("ThXuquTarget");
        if (target)
            target->drawCards(1);

        return false;
    }
};

class ThQiebao : public TriggerSkill
{
public:
    ThQiebao()
        : TriggerSkill("thqiebao")
    {
        events << CardUsed << BeforeCardsMove;
    }

    bool doQiebao(Room *room, ServerPlayer *player, QList<int> card_ids, bool move, QVariant &data) const
    {
        QString prompt = "@thqiebao";
        if (move) {
            prompt += "move:%1:%2:%3";
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            prompt = prompt.arg(move.from->objectName()).arg(move.to->objectName()).arg(move.card_ids.length());
        }
        const Card *card = room->askForCard(player, "slash", prompt, data, objectName());
        if (!card)
            return false;

        if (card_ids.contains(card->getEffectiveId()))
            card_ids.removeOne(card->getEffectiveId());

        if (!card_ids.isEmpty()) {
            CardsMoveStruct qiebaoMove;
            if (move || card->isRed()) {
                qiebaoMove.to = player;
                qiebaoMove.to_place = Player::PlaceHand;
                qiebaoMove.card_ids = card_ids;
                room->moveCardsAtomic(qiebaoMove, false);
            }
        }

        return true;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.isEmpty() || !use.card->isKindOf("Peach"))
                return skill_list;

            foreach (ServerPlayer *to, use.to) {
                if (to == use.from)
                    continue;
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                    skill_list.insert(p, QStringList(objectName()));
                break;
            }
        } else if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from || !move.to || move.from == move.to)
                return skill_list;

            if (move.to_place == Player::PlaceHand) {
                for (int i = 0; i < move.card_ids.size(); i++) {
                    if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip) {
                        skill_list.insert(player, QStringList(objectName()));
                        break;
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<int> card_ids;
            if (!use.card->isVirtualCard())
                card_ids << use.card->getEffectiveId();
            else
                card_ids = use.card->getSubcards();
            if (doQiebao(room, ask_who, card_ids, false, data)) {
                foreach (ServerPlayer *p, use.to) {
                    if (p != use.from)
                        use.nullified_list << p->objectName();
                }
                data = QVariant::fromValue(use);
            }
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> qiebaolist;
            for (int i = 0; i < move.card_ids.size(); i++) {
                int card_id = move.card_ids[i];
                if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)
                    qiebaolist << card_id;
            }
            if (!qiebaolist.isEmpty() && doQiebao(room, ask_who, qiebaolist, true, data)) {
                move.removeCardIds(qiebaolist);
                data = QVariant::fromValue(move);
            }
        }

        return false;
    }
};

class ThLingta : public TriggerSkill
{
public:
    ThLingta()
        : TriggerSkill("thlingta")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->isSkipped(change.to))
            return QStringList();

        if (change.to == Player::Draw || change.to == Player::Play)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->askForSkillInvoke(objectName(), QString::number((int)change.to))) {
            room->broadcastSkillInvoke(objectName());
            player->skip(change.to, true);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->gainMark("@bright");
        return false;
    }
};

class ThWeiguang : public TriggerSkill
{
public:
    ThWeiguang()
        : TriggerSkill("thweiguang")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getMark("@bright") <= 0)
            return QStringList();

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();

        if (change.from == Player::Play || change.from == Player::Draw) {
            if (!player->hasFlag(objectName() + QString::number((int)change.from)))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->askForSkillInvoke(objectName(), QString::number((int)change.from))) {
            room->setPlayerFlag(player, objectName() + QString::number((int)change.from));
            player->loseMark("@bright");
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        player->insertPhase(change.from);
        change.to = change.from;
        data = QVariant::fromValue(change);
        return false;
    }
};

class ThWeiguangSkip : public TriggerSkill
{
public:
    ThWeiguangSkip()
        : TriggerSkill("#thweiguang-skip")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getMark("@bright") <= 0)
            return QStringList();

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();

        if (change.to == Player::Judge || change.to == Player::Discard) {
            if (!player->hasFlag("thweiguang" + QString::number((int)change.to)))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->askForSkillInvoke("thweiguang", QVariant::fromValue(QString::number((int)change.to)))) {
            room->setPlayerFlag(player, "thweiguang" + QString::number((int)change.to));
            player->loseMark("@bright");
            room->broadcastSkillInvoke("thweiguang");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        player->skip(change.to);
        return false;
    }
};

class ThChuhui : public TriggerSkill
{
public:
    ThChuhui()
        : TriggerSkill("thchuhui")
    {
        events << GameStart;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        player->gainMark("@bright");
        return false;
    }
};

ThKujieCard::ThKujieCard()
{
    m_skillName = "thkujiev";
    mute = true;
}

bool ThKujieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasSkill("thkujie") && !to_select->hasFlag("ThKujieInvoked");
}

void ThKujieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    if (target->hasSkill("thkujie")) {
        room->setPlayerFlag(target, "ThKujieInvoked");

        room->broadcastSkillInvoke("thkujie");
        room->notifySkillInvoked(target, "thkujie");

        room->loseHp(target);
        room->setPlayerMark(target, "kujie-invoke", 1);

        QList<ServerPlayer *> targets;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasSkill("thkujie") && !p->hasFlag("ThKujieInvoked"))
                targets << p;
        }
        if (targets.isEmpty())
            room->setPlayerFlag(source, "ForbidThKujie");
    }
}

class ThKujieViewAsSkill : public OneCardViewAsSkill
{
public:
    ThKujieViewAsSkill()
        : OneCardViewAsSkill("thkujiev")
    {
        attached_lord_skill = true;
        filter_pattern = "BasicCard|red!";
    }

    virtual bool shouldBeVisible(const Player *) const
    {
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThKujieCard *card = new ThKujieCard();
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("ForbidThKujie");
    }
};

class ThKujie : public TriggerSkill
{
public:
    ThKujie()
        : TriggerSkill("thkujie")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if ((triggerEvent == GameStart) || (triggerEvent == EventAcquireSkill && data.toString() == "thkujie")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty())
                return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("thkujiev"))
                    room->attachSkillToPlayer(p, "thkujiev");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "thkujie") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2)
                return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("thkujiev"))
                    room->detachSkillFromPlayer(p, "thkujiev", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play && phase_change.to != Player::Play)
                return QStringList();
            if (player->hasFlag("ForbidThKujie"))
                room->setPlayerFlag(player, "-ForbidThKujie");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("ThKujieInvoked"))
                    room->setPlayerFlag(p, "-ThKujieInvoked");
            }
        }
        return QStringList();
    }
};

class ThKujieRecover : public TriggerSkill
{
public:
    ThKujieRecover()
        : TriggerSkill("#thkujie-recover")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark("kujie-invoke") > 0 && p->isWounded()) {
                    QStringList skills;
                    for (int i = 0; i < p->getMark("kujie-invoke"); ++i)
                        skills << objectName();
                    skill_list.insert(p, skills);
                }
            }
        } else if (player->getPhase() == Player::RoundStart) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark("kujie-invoke") > 0)
                    room->setPlayerMark(p, "kujie-invoke", 0);
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, "thkujie");
        room->recover(ask_who, RecoverStruct(ask_who, NULL, 2));
        return false;
    }
};

class ThYinbi : public TriggerSkill
{
public:
    ThYinbi()
        : TriggerSkill("thyinbi")
    {
        events << HpChanged;
        owner_only_skill = true;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (!data.canConvert<DamageStruct>())
            return skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            if (p != player && player->getHp() + damage.damage <= p->getHp())
                skill_list.insert(p, QStringList(objectName()));
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
        DamageStruct damage = data.value<DamageStruct>();
        room->recover(player, RecoverStruct(ask_who, NULL, damage.damage));
        damage.invoke_skills.clear();
        damage.to = ask_who;
        room->damage(damage);
        return false;
    }
};

class ThMingling : public TriggerSkill
{
public:
    ThMingling()
        : TriggerSkill("thmingling")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Normal)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire)
            return true;
        else if (damage.nature == DamageStruct::Thunder)
            damage.damage++;
        data = QVariant::fromValue(damage);
        return false;
    }
};

ThChuanshangCard::ThChuanshangCard()
{
}

bool ThChuanshangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->inMyAttackRange(to_select);
}

void ThChuanshangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    target->gainMark("@drowning");
    if (target->getMark("@drowning") > 1) // trick
        room->loseHp(source);
}

class ThChuanshangViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThChuanshangViewAsSkill()
        : ZeroCardViewAsSkill("thchuanshang")
    {
    }

    virtual const Card *viewAs() const
    {
        return new ThChuanshangCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThChuanshangCard");
    }
};

class ThChuanshang : public TriggerSkill
{
public:
    ThChuanshang()
        : TriggerSkill("thchuanshang")
    {
        events << EventPhaseStart;
        view_as_skill = new ThChuanshangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive() && target->getPhase() == Player::Finish && target->getMark("@drowning") > 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        LogMessage log;
        log.type = "#ThChuanshang";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = true;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        if (judge.isGood())
            player->loseMark("@drowning", qMin(2, player->getMark("@drowning")));
        else if (judge.card->isBlack())
            player->gainMark("@drowning");

        return false;
    }
};

class ThChuanshangMaxCardsSkill : public MaxCardsSkill
{
public:
    ThChuanshangMaxCardsSkill()
        : MaxCardsSkill("#thchuanshang")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->getMark("@drowning") > 0)
            return -target->getMark("@drowning");
        else
            return 0;
    }
};

ThLingdieCard::ThLingdieCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

void ThLingdieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    ServerPlayer *target = targets.first();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thlingdie", QString());
    room->obtainCard(target, this, reason);
    QList<ServerPlayer *> victims;
    foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
        if (!p->isKongcheng())
            victims << p;
    }
    ServerPlayer *victim = NULL;
    if (!victims.isEmpty())
        victim = room->askForPlayerChosen(target, victims, "thlingdie", "@thlingdie");
    if (victim) {
        LogMessage log;
        log.type = "$IkLingtongView";
        log.from = target;
        log.to << victim;
        log.arg = "iklingtong:handcards";
        room->sendLog(log, room->getOtherPlayers(target));

        room->showAllCards(victim, target);
    }

    if (suit == Heart)
        target->drawCards(1, "thlingdie");
    else
        room->setPlayerFlag(source, "ThLingdieDisabled");
}

class ThLingdieVS : public OneCardViewAsSkill
{
public:
    ThLingdieVS()
        : OneCardViewAsSkill("thlingdie")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("ThLingdieDisabled") && player->usedTimes("ThLingdieCard") < 2;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThLingdieCard *card = new ThLingdieCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThLingdie : public TriggerSkill
{
public:
    ThLingdie()
        : TriggerSkill("thlingdie")
    {
        events << EventPhaseChanging;
        view_as_skill = new ThLingdieVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player->hasFlag("ThLingdieDisabled"))
            room->setPlayerFlag(player, "-ThLingdieDisabled");
        return QStringList();
    }
};

class ThWushou : public TriggerSkill
{
public:
    ThWushou()
        : TriggerSkill("thwushou")
    {
        events << Damaged;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getHandcardNum() < 4;
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
        player->drawCards(4 - player->getHandcardNum());
        return false;
    }
};

ThFuyueCard::ThFuyueCard()
{
    m_skillName = "thfuyuev";
    mute = true;
}

bool ThFuyueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("thfuyue") && !to_select->isKongcheng() && to_select->getHp() == 1
        && to_select != Self && !to_select->hasFlag("ThFuyueInvoked");
}

void ThFuyueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    if (target->hasLordSkill("thfuyue")) {
        room->setPlayerFlag(target, "ThFuyueInvoked");
        room->broadcastSkillInvoke("thfuyue");
        room->notifySkillInvoked(target, "thfuyue");
        if (room->askForChoice(target, "thfuyue", "accept+reject") == "accept") {
            bool win = source->pindian(target, "thfuyue");
            if (!win) {
                RecoverStruct recover;
                recover.who = source;
                room->recover(target, recover);
            }
        }
        QList<ServerPlayer *> lords;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasLordSkill("thfuyue") && !p->hasFlag("ThFuyueInvoked")) {
                lords << p;
            }
        }
        if (lords.empty())
            room->setPlayerFlag(source, "ForbidThFuyue");
    }
}

class ThFuyueViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThFuyueViewAsSkill()
        : ZeroCardViewAsSkill("thfuyuev")
    {
        attached_lord_skill = true;
    }

    virtual bool shouldBeVisible(const Player *player) const
    {
        return player->getKingdom() == "yuki";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && player->getKingdom() == "yuki" && !player->hasFlag("ForbidThFuyue");
    }

    virtual const Card *viewAs() const
    {
        return new ThFuyueCard;
    }
};

class ThFuyue : public TriggerSkill
{
public:
    ThFuyue()
        : TriggerSkill("thfuyue$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!player)
            return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "thfuyue")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty())
                return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("thfuyuev"))
                    room->attachSkillToPlayer(p, "thfuyuev");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "thfuyue") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2)
                return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("thfuyuev"))
                    room->detachSkillFromPlayer(p, "thfuyuev", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play && phase_change.to != Player::Play)
                return QStringList();
            if (player->hasFlag("ForbidThFuyue"))
                room->setPlayerFlag(player, "-ForbidThFuyue");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("ThFuyueInvoked"))
                    room->setPlayerFlag(p, "-ThFuyueInvoked");
            }
        }
        return QStringList();
    }
};

TouhouYukiPackage::TouhouYukiPackage()
    : Package("touhou-yuki")
{
    General *yuki001 = new General(this, "yuki001$", "yuki");
    yuki001->addSkill(new ThJianmo);
    yuki001->addSkill(new ThJianmoDiscard);
    related_skills.insertMulti("thjianmo", "#thjianmo");
    yuki001->addSkill(new ThErchong);
    yuki001->addSkill(new ThErchongRecord);
    related_skills.insertMulti("therchong", "#therchong-record");
    yuki001->addSkill(new ThChundu);
    yuki001->addRelateSkill("thhuanfa");
    yuki001->addRelateSkill("ikzhuji");

    General *yuki002 = new General(this, "yuki002", "yuki");
    yuki002->addSkill(new ThZuishang);
    yuki002->addSkill(new ThXugu);
    yuki002->addSkill(new SlashNoDistanceLimitSkill("thxugu"));
    related_skills.insertMulti("thxugu", "#thxugu-slash-ndl");

    General *yuki003 = new General(this, "yuki003", "yuki");
    yuki003->addSkill(new ThShenzhan);
    yuki003->addSkill(new ThHunqie);
    yuki003->addSkill(new ThDaojian);

    General *yuki004 = new General(this, "yuki004", "yuki");
    yuki004->addSkill(new ThZhancao);

    General *yuki005 = new General(this, "yuki005", "yuki", 3, false);
    yuki005->addSkill(new ThMoji);
    yuki005->addSkill(new ThYuanqi);

    General *yuki006 = new General(this, "yuki006", "yuki", 3);
    yuki006->addSkill(new ThDunjia);
    yuki006->addSkill(new ThQingming);

    General *yuki007 = new General(this, "yuki007", "yuki", 3);
    yuki007->addSkill(new ThChouce);
    yuki007->addSkill(new ThChouceRecord);
    related_skills.insertMulti("thchouce", "#thchouce");
    yuki007->addSkill(new ThZhanshi);
    yuki007->addSkill(new ThZhanshiClear);
    related_skills.insertMulti("thzhanshi", "#thzhanshi-clear");
    yuki007->addRelateSkill("thhuanzang");

    General *yuki008 = new General(this, "yuki008", "yuki", 3);
    yuki008->addSkill(new ThZiyun);
    yuki008->addSkill(new ThChuiji);
    yuki008->addSkill(new ThChuijiRecord);
    related_skills.insertMulti("thchuiji", "#thchuiji");

    General *yuki009 = new General(this, "yuki009", "yuki");
    yuki009->addSkill(new ThLingya);
    yuki009->addSkill(new ThHeimu);

    General *yuki010 = new General(this, "yuki010", "yuki");
    yuki010->addSkill(new ThHanpo);
    yuki010->addSkill(new ThJidong);
    yuki010->addSkill(new ThJidongRecord);
    yuki010->addSkill(new ThJidongTrigger);
    related_skills.insertMulti("thjidong", "#thjidong-record");
    related_skills.insertMulti("thjidong", "#thjidong");
    yuki010->addSkill(new ThBingpu);

    General *yuki011 = new General(this, "yuki011", "yuki", 3);
    yuki011->addSkill(new ThDongmo);
    yuki011->addSkill(new ThLinhan);

    General *yuki012 = new General(this, "yuki012", "yuki");
    yuki012->addSkill(new ThFusheng);
    yuki012->addSkill(new ThFushengEffect);
    related_skills.insertMulti("thfusheng", "#thfusheng");

    General *yuki013 = new General(this, "yuki013", "yuki", 3);
    yuki013->addSkill(new ThSaozang);
    yuki013->addSkill(new ThXuqu);

    General *yuki014 = new General(this, "yuki014", "yuki");
    yuki014->addSkill(new ThQiebao);

    General *yuki015 = new General(this, "yuki015", "yuki");
    yuki015->addSkill(new ThLingta);
    yuki015->addSkill(new ThWeiguang);
    yuki015->addSkill(new ThWeiguangSkip);
    related_skills.insertMulti("thweiguang", "#thweiguang-skip");
    yuki015->addSkill(new ThChuhui);

    General *yuki016 = new General(this, "yuki016", "yuki");
    yuki016->addSkill(new ThKujie);
    yuki016->addSkill(new ThKujieRecover);
    related_skills.insertMulti("thkujie", "#thkujie-recover");
    yuki016->addSkill(new ThYinbi);

    General *yuki017 = new General(this, "yuki017", "yuki");
    yuki017->addSkill(new ThMingling);
    yuki017->addSkill(new ThChuanshang);
    yuki017->addSkill(new ThChuanshangMaxCardsSkill);
    related_skills.insertMulti("thchuanshang", "#thchuanshang");

    General *yuki018 = new General(this, "yuki018$", "yuki", 3, false);
    yuki018->addSkill(new ThLingdie);
    yuki018->addSkill(new ThWushou);
    yuki018->addSkill(new ThFuyue);

    addMetaObject<ThHuanfaCard>();
    addMetaObject<ThMojiCard>();
    addMetaObject<ThYuanqiCard>();
    addMetaObject<ThChouceCard>();
    addMetaObject<ThBingpuCard>();
    addMetaObject<ThDongmoCard>();
    addMetaObject<ThKujieCard>();
    addMetaObject<ThChuanshangCard>();
    addMetaObject<ThLingdieCard>();
    addMetaObject<ThFuyueCard>();

    skills << new ThHuanfa << new ThZuishangGivenSkill << new ThHuanzang << new ThKujieViewAsSkill << new ThFuyueViewAsSkill;
}

ADD_PACKAGE(TouhouYuki)
