#include "touhou-sp.h"
#include "carditem.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "ikai-kin.h"
#include "maneuvering.h"
#include "skill.h"

class ThFanshi : public OneCardViewAsSkill
{
public:
    ThFanshi()
        : OneCardViewAsSkill("thfanshi")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getHandcardNum() >= player->getHp() && Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getHandcardNum() < player->getHp())
            return false;
        return (pattern == "slash" && player->getPhase() != Player::NotActive)
            || ((pattern == "jink" || pattern.contains("peach")) && player->getPhase() == Player::NotActive);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = NULL;
        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            card = new Slash(originalCard->getSuit(), originalCard->getNumber());
            break;
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "jink")
                card = new Jink(originalCard->getSuit(), originalCard->getNumber());
            else if (pattern == "peach" || pattern == "peach+analeptic")
                card = new Peach(originalCard->getSuit(), originalCard->getNumber());
            else if (pattern == "slash")
                card = new Slash(originalCard->getSuit(), originalCard->getNumber());
            else
                return NULL;
            break;
        }
        default: {
            break;
        }
        }
        if (card != NULL) {
            card->setSkillName(objectName());
            card->addSubcard(originalCard);
            return card;
        }
        return NULL;
    }
};

class ThJifeng : public MaxCardsSkill
{
public:
    ThJifeng()
        : MaxCardsSkill("thjifeng")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill(objectName()))
            return 1;
        else
            return 0;
    }
};

class ThKongsuo : public TriggerSkill
{
public:
    ThKongsuo()
        : TriggerSkill("thkongsuo")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target
            = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thkongsuo", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThKongsuoTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThKongsuoTarget"].value<ServerPlayer *>();
        player->tag.remove("ThKongsuoTarget");
        if (target) {
            target->setChained(!target->isChained());
            room->broadcastProperty(target, "chained");
            room->setEmotion(target, "effects/iron_chain");
            room->getThread()->trigger(ChainStateChanged, room, target);
        }

        return false;
    }
};

class ThLiuren : public TriggerSkill
{
public:
    ThLiuren()
        : TriggerSkill("thliuren")
    {
        events << CardUsed << JinkEffect << NullificationEffect;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&ask_who) const
    {
        if (!player->isChained() || player->getMark(objectName()) > 1)
            return QStringList();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeSkill)
                return QStringList();
        }
        if (!room->isSomeonesTurn())
            return QStringList();
        ServerPlayer *current = room->getCurrent();
        if (current != player && TriggerSkill::triggerable(current)) {
            ask_who = current;
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        if (triggerEvent == JinkEffect || triggerEvent == NullificationEffect) {
            room->sendCompulsoryTriggerLog(ask_who, objectName());
            return true;
        }

        CardUseStruct use = data.value<CardUseStruct>();
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        use.nullified_list << "_ALL_TARGETS";
        data = QVariant::fromValue(use);
        return false;
    }
};

class ThLiurenRecord : public TriggerSkill
{
public:
    ThLiurenRecord()
        : TriggerSkill("#thliuren")
    {
        events << PreCardUsed << CardResponded << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setMark("thliuren", 0);
            }
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse && resp.m_card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->addMark("thliuren");
        return false;
    }
};

class ThShenshi : public TriggerSkill
{
public:
    ThShenshi()
        : TriggerSkill("thshenshi")
    {
        events << EventPhaseSkipped << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("thshenshiused"))
                        p->setFlags("-thshenshiused");
                }
            }
        } else if (player->isWounded())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner->hasFlag("thshenshiused") || !owner->canDiscard(owner, "he"))
                    continue;
                skill_list.insert(owner, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (room->askForCard(ask_who, "..", "@thshenshi:" + player->objectName(), data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        room->recover(player, RecoverStruct(ask_who));
        ask_who->setFlags("thshenshiused");
        return false;
    }
};

class ThJiefan : public TriggerSkill
{
public:
    ThJiefan()
        : TriggerSkill("thjiefan")
    {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Start;
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
        int id = room->drawCard();

        CardsMoveStruct move;
        move.card_ids << id;
        move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        move.to_place = Player::PlaceTable;
        room->moveCardsAtomic(move, true);
        room->getThread()->delay();

        const Card *card = Sanguosha->getCard(id);
        if (card->isKindOf("BasicCard")) {
            player->tag["ThJiefanId"] = id;
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName());
            player->tag.remove("ThJiefanId");
            room->obtainCard(target, card);
        } else {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
            room->throwCard(card, reason, NULL);
            return false;
        }

        if (player->askForSkillInvoke(objectName()))
            effect(NonTrigger, room, player, data, player);
        return false;
    }
};

class ThChuangshi : public TriggerSkill
{
public:
    ThChuangshi()
        : TriggerSkill("thchuangshi")
    {
        events << CardEffected;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (!effect.card->isKindOf("Dismantlement") && !effect.card->isKindOf("Collateral") && !effect.card->isKindOf("Duel"))
            return skill_list;

        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (effect.from == owner)
                continue;
            if (owner == player || owner->inMyAttackRange(player))
                skill_list.insert(owner, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        ask_who->tag["ThChuangshiData"] = data;
        if (ask_who->askForSkillInvoke(objectName())) {
            ask_who->tag.remove("ThChuangshiData");
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        ask_who->tag.remove("ThChuangshiData");
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_" + objectName());
        if (effect.from->canSlash(ask_who, slash, false))
            room->useCard(CardUseStruct(slash, effect.from, ask_who));
        else
            delete slash;
        return true;
    }
};

class ThGaotian : public TriggerSkill
{
public:
    ThGaotian()
        : TriggerSkill("thgaotian")
    {
        events << CardAsked;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QString asked_pattern = data.toStringList().first();
        if (TriggerSkill::triggerable(player) && asked_pattern == "jink")
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        int n = qMin(4, room->alivePlayerCount());
        QList<int> card_ids = room->getNCards(n, false);

        room->fillAG(card_ids, player);

        QList<int> reds, blacks;
        foreach (int id, card_ids)
            if (Sanguosha->getCard(id)->isRed())
                reds << id;
            else if (Sanguosha->getCard(id)->isBlack())
                blacks << id;
        QString pattern = "..";
        if (reds.isEmpty())
            pattern = ".|black";
        else if (blacks.isEmpty())
            pattern = ".|red";
        const Card *card = room->askForCard(player, pattern, "@gaotian-discard",
                                            QVariant::fromValue(IntList2VariantList(card_ids)), objectName());
        if (card) {
            int card_id = -1;
            if (card->isBlack()) {
                room->clearAG(player);
                room->fillAG(card_ids, player, reds);
                card_id = room->askForAG(player, blacks, false, objectName());
            } else if (card->isRed()) {
                room->clearAG(player);
                room->fillAG(card_ids, player, blacks);
                card_id = room->askForAG(player, reds, false, objectName());
            }
            if (card_id != -1) {
                room->clearAG(player);
                room->fillAG(card_ids, player);
                QList<ServerPlayer *> p_list;
                p_list << player;
                room->takeAG(player, card_id, false, p_list);
                card_ids.removeOne(card_id);
                room->obtainCard(player, card_id);
            }
        }

        DummyCard *dummy = new DummyCard;
        QList<ServerPlayer *> p_list;
        p_list << player;
        while (!card_ids.isEmpty()) {
            room->setPlayerFlag(player, "ThGaotianSecond");
            int card_id = room->askForAG(player, card_ids, true, objectName());
            room->setPlayerFlag(player, "-ThGaotianSecond");
            if (card_id != -1) {
                room->takeAG(NULL, card_id, false, p_list);
                card_ids.removeOne(card_id);
                dummy->addSubcard(card_id);
            } else
                break;
        }

        room->clearAG(player);
        if (!card_ids.isEmpty())
            room->askForGuanxing(player, card_ids, Room::GuanxingUpOnly);
        if (dummy->subcardsLength() > 0) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
            room->throwCard(dummy, reason, NULL);
        }
        delete dummy;

        return false;
    }
};

class ThWanling : public TriggerSkill
{
public:
    ThWanling()
        : TriggerSkill("thwanling")
    {
        events << BeforeCardsMove << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setMark(objectName(), 0);
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!TriggerSkill::triggerable(player) || player == move.from || !move.from || player->getMark(objectName()) >= 3)
                return QStringList();
            if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
                && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
                const Card *card = move.reason.m_extraData.value<const Card *>();
                if (card && card->isRed() && (card->isKindOf("Slash") || card->isNDTrick()))
                    return QStringList(objectName());
            }
        }
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->addMark(objectName());
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->canDiscard(player, "he")
            && room->askForCard(player, "..", "@thwanling:" + move.from->objectName(), data, objectName())) {
            player->obtainCard(move.reason.m_extraData.value<const Card *>());
            move.removeCardIds(move.card_ids);
            data = QVariant::fromValue(move);
        } else
            room->drawCards((ServerPlayer *)move.from, 1, objectName());
        return false;
    }
};

class ThZuibu : public TriggerSkill
{
public:
    ThZuibu()
        : TriggerSkill("thzuibu")
    {
        events << DamageInflicted << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&ask_who) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->setMark(objectName(), 0);
            }
        } else if (triggerEvent == DamageInflicted) {
            if (TriggerSkill::triggerable(player) && player->getMark(objectName()) < 3 && room->isSomeonesTurn()) {
                DamageStruct damage = data.value<DamageStruct>();
                if (!damage.from || damage.from->isDead())
                    return QStringList();
                ask_who = damage.from;
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = ask_who;
            log.to << player;
            log.arg = objectName();
            room->sendLog(log);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        player->addMark(objectName());
        player->drawCards(1, objectName());
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#ThZuibu";
        log.from = ask_who;
        log.to << player;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(--damage.damage);
        room->sendLog(log);

        data = QVariant::fromValue(damage);
        if (damage.damage < 1)
            return true;
        return false;
    }
};

class ThModao : public PhaseChangeSkill
{
public:
    ThModao()
        : PhaseChangeSkill("thmodao")
    {
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        if (!PhaseChangeSkill::triggerable(player) || player->getPhase() != Player::Draw)
            return false;
        QStringList flags;
        flags << "h"
              << "e"
              << "j";
        foreach (ServerPlayer *p, player->getRoom()->getAllPlayers()) {
            if (player->inMyAttackRange(p)) {
                foreach (QString flag, flags) {
                    if (qAbs(p->getCards(flag).length() - player->getCards(flag).length()) <= player->getLostHp() + 1)
                        return true;
                }
            }
        }
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets;
        QStringList flags;
        flags << "h"
              << "e"
              << "j";
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (player->inMyAttackRange(p)) {
                foreach (QString flag, flags) {
                    if (qAbs(p->getCards(flag).length() - player->getCards(flag).length()) <= player->getLostHp() + 1) {
                        targets << p;
                        break;
                    }
                }
            }
        }
        if (targets.isEmpty())
            return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thmodao", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThModaoTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        ServerPlayer *target = player->tag["ThModaoTarget"].value<ServerPlayer *>();
        player->tag.value("ThModaoTarget");
        if (target) {
            QStringList flags;
            flags << "h"
                  << "e"
                  << "j";
            QStringList choices;
            foreach (QString flag, flags) {
                if (qAbs(target->getCards(flag).length() - player->getCards(flag).length()) <= player->getLostHp() + 1)
                    choices << flag;
            }
            player->tag["ThModaoData"] = QVariant::fromValue(target); // for AI
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant::fromValue(target));
            player->tag.remove("ThModaoData");
            if (choice == "h") {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p != player && p != target)
                        room->doNotify(p, QSanProtocol::S_COMMAND_EXCHANGE_KNOWN_CARDS,
                                       JsonArray() << player->objectName() << target->objectName());
                }
                QList<CardsMoveStruct> exchangeMove;
                CardsMoveStruct move1(player->handCards(), target, Player::PlaceHand,
                                      CardMoveReason(CardMoveReason::S_REASON_SWAP, player->objectName(), target->objectName(),
                                                     objectName(), QString()));
                CardsMoveStruct move2(target->handCards(), player, Player::PlaceHand,
                                      CardMoveReason(CardMoveReason::S_REASON_SWAP, target->objectName(), player->objectName(),
                                                     objectName(), QString()));
                exchangeMove.push_back(move1);
                exchangeMove.push_back(move2);
                room->moveCardsAtomic(exchangeMove, false);
            } else if (choice == "e") {
                QList<CardsMoveStruct> exchangeMove;
                QList<int> ids1;
                foreach (const Card *c, player->getEquips())
                    ids1 << c->getEffectiveId();
                CardsMoveStruct move1(ids1, target, Player::PlaceEquip,
                                      CardMoveReason(CardMoveReason::S_REASON_SWAP, player->objectName(), target->objectName(),
                                                     objectName(), QString()));
                QList<int> ids2;
                foreach (const Card *c, target->getEquips())
                    ids2 << c->getEffectiveId();
                CardsMoveStruct move2(ids2, player, Player::PlaceEquip,
                                      CardMoveReason(CardMoveReason::S_REASON_SWAP, target->objectName(), player->objectName(),
                                                     objectName(), QString()));
                exchangeMove.push_back(move1);
                exchangeMove.push_back(move2);
                room->moveCardsAtomic(exchangeMove, false);
            } else if (choice == "j") {
                QList<CardsMoveStruct> exchangeMove;
                CardsMoveStruct move1(player->getJudgingAreaID(), target, Player::PlaceDelayedTrick,
                                      CardMoveReason(CardMoveReason::S_REASON_SWAP, player->objectName(), target->objectName(),
                                                     objectName(), QString()));
                CardsMoveStruct move2(target->getJudgingAreaID(), player, Player::PlaceDelayedTrick,
                                      CardMoveReason(CardMoveReason::S_REASON_SWAP, target->objectName(), player->objectName(),
                                                     objectName(), QString()));
                exchangeMove.push_back(move1);
                exchangeMove.push_back(move2);
                room->moveCardsAtomic(exchangeMove, false);
            }
            QList<ServerPlayer *> targets;
            targets << player << target;
            room->sortByActionOrder(targets);
            room->drawCards(targets, 1, objectName());
            return true;
        }
        return false;
    }
};

class ThMengsheng : public TriggerSkill
{
public:
    ThMengsheng()
        : TriggerSkill("thmengsheng")
    {
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && TriggerSkill::triggerable(player))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(data.value<DamageStruct>().from))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from->isNude())
            room->obtainCard(player, room->askForCardChosen(player, damage.from, "he", objectName()), false);

        QStringList sources = damage.from->tag["ThMengsheng"].toStringList();
        sources << player->objectName();
        damage.from->tag["ThMengsheng"] = QVariant::fromValue(sources);
        player->tag["ThMengshengRecord"] = true;
        room->setPlayerMark(damage.from, "@mengsheng", 1);
        foreach (ServerPlayer *pl, room->getAllPlayers())
            room->filterCards(pl, pl->getHandcards(), true);
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        if (room->isSomeonesTurn(player))
            room->setPlayerFlag(player, "thmengsheng");

        return false;
    }
};

class ThMengshengClear : public TriggerSkill
{
public:
    ThMengshengClear()
        : TriggerSkill("#thmengsheng-clear")
    {
        events << EventPhaseChanging << Death;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!player || !player->tag["ThMengshengRecord"].toBool())
            return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (player->hasFlag("thmengsheng") || change.to != Player::NotActive)
                return QStringList();
        }
        if (triggerEvent == Death && data.value<DeathStruct>().who != player)
            return QStringList();
        player->tag["ThMengshengRecord"] = false;
        foreach (ServerPlayer *p, room->getAllPlayers())
            if (p->tag.value("ThMengsheng").toStringList().contains(player->objectName())) {
                QStringList sources = p->tag.value("ThMengsheng").toStringList();
                sources.removeAll(player->objectName());
                if (sources.isEmpty()) {
                    room->setPlayerMark(p, "@mengsheng", 0);
                    foreach (ServerPlayer *pl, room->getAllPlayers())
                        room->filterCards(pl, pl->getHandcards(), false);
                    JsonArray args;
                    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                }
                p->tag["ThMengsheng"] = QVariant::fromValue(sources);
            }
        return QStringList();
    }
};

class ThMengshengInvalidity : public InvaliditySkill
{
public:
    ThMengshengInvalidity()
        : InvaliditySkill("#thmengsheng-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return skill->isOwnerOnlySkill() || player->getMark("@mengsheng") == 0;
    }
};

class ThQixiang : public TriggerSkill
{
public:
    ThQixiang()
        : TriggerSkill("thqixiang")
    {
        events << EventPhaseEnd;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Discard) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner->canDiscard(owner, "he") && owner->inMyAttackRange(player))
                    skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (room->askForCard(ask_who, "..", "@thqixiang", QVariant::fromValue(player), objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        QStringList choices;
        choices << "draw";
        if (!player->isKongcheng())
            choices << "discard";
        QString choice = room->askForChoice(ask_who, objectName(), choices.join("+"), QVariant::fromValue(player));
        if (choice == "discard")
            room->askForDiscard(player, objectName(), 1, 1);
        else
            player->drawCards(1);
        return false;
    }
};

class ThHuanlongViewAsSkill : public ViewAsSkill
{
public:
    ThHuanlongViewAsSkill()
        : ViewAsSkill("thhuanlong")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 2)
            return false;
        else
            return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;
        else {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcards(cards);
            slash->setSkillName(objectName());
            return slash;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->hasFlag("thhuanlong") && Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return player->hasFlag("thhuanlong") && pattern == "slash";
    }
};

class ThHuanlong : public TriggerSkill
{
public:
    ThHuanlong()
        : TriggerSkill("thhuanlong")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new ThHuanlongViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play)
            return QStringList(objectName());
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                if (player->hasFlag("thhuanlong"))
                    room->setPlayerFlag(player, "-thhuanlong");
                if (player->getMark("thhuanlong1") > 0)
                    room->setPlayerMark(player, "thhuanlong1", 0);
                if (player->getMark("thhuanlong2") > 0)
                    room->setPlayerMark(player, "thhuanlong2", 0);
            }
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        bool first = true;
        forever {
            QStringList choices;
            choices << "thhuanlong1"
                    << "thhuanlong2";
            if (!player->hasFlag("thhuanlong"))
                choices << "thhuanlong3";
            if (choices.isEmpty())
                break;
            if (!first)
                choices << "cancel";
            QString choice = room->askForChoice(player, "thhuanlong", choices.join("+"));
            if (choice == "cancel")
                break;
            LogMessage log;
            log.type = "#ThHuanlong";
            log.from = player;
            log.arg = "thhuanlong";
            log.arg2 = choice.right(1);
            room->sendLog(log);
            if (choice == "thhuanlong3")
                room->setPlayerFlag(player, "thhuanlong");
            else
                room->addPlayerMark(player, choice);
            if (player->getMaxCards() <= 0)
                break;
            first = false;
        }
        return false;
    }
};

class ThHuanlongTargetMod : public TargetModSkill
{
public:
    ThHuanlongTargetMod()
        : TargetModSkill("#thhuanlong-target")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        return from->getMark("thhuanlong2");
    }
};

class ThHuanlongMaxCards : public MaxCardsSkill
{
public:
    ThHuanlongMaxCards()
        : MaxCardsSkill("#thhuanlong-max-cards")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        int n = target->getMark("thhuanlong1") + target->getMark("thhuanlong2");
        if (target->hasFlag("thhuanlong"))
            ++n;
        return -n;
    }
};

ThYuduCard::ThYuduCard()
{
}

bool ThYuduCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool ThYuduCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFixed();
}

bool ThYuduCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetsFeasible(targets, Self);
}

const Card *ThYuduCard::validate(CardUseStruct &cardUse) const
{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();
    QStringList names = user_string.split("+");
    if (names.contains("slash"))
        names << "fire_slash"
              << "thunder_slash";

    ServerPlayer *current = room->getCurrent();
    if (!room->isSomeonesTurn() || current->isKongcheng()) {
        room->setPlayerFlag(user, "Global_ThYuduFailed");
        return NULL;
    }

    room->setPlayerFlag(user, "thyudu");

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "thyudu";
    room->sendLog(log);

    int card_id = room->askForCardChosen(user, current, "h", "thyudu");
    room->showCard(current, card_id);

    const Card *card = Sanguosha->getCard(card_id);
    if (names.contains(card->objectName()))
        if (user->askForSkillInvoke("thyudu_use", "use"))
            return card;
    if (card->getSuit() == Club)
        if (user->askForSkillInvoke("thyudu_use", "change")) {
            QString name = room->askForChoice(user, "thyudu", names.join("+"));
            Card *c = Sanguosha->cloneCard(name);
            c->addSubcard(card);
            c->setSkillName("thyudu");
            return c;
        }
    return NULL;
}

const Card *ThYuduCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    QStringList names = user_string.split("+");
    if (names.contains("slash"))
        names << "fire_slash"
              << "thunder_slash";

    ServerPlayer *current = room->getCurrent();
    if (!room->isSomeonesTurn() || current->isKongcheng()) {
        room->setPlayerFlag(user, "Global_ThYuduFailed");
        return NULL;
    }

    room->setPlayerFlag(user, "thyudu");

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "thyudu";
    room->sendLog(log);

    int card_id = room->askForCardChosen(user, current, "h", "thyudu");
    room->showCard(current, card_id);

    const Card *card = Sanguosha->getCard(card_id);
    if (names.contains(card->objectName()))
        if (user->askForSkillInvoke("thyudu_use", "use"))
            return card;
    if (card->getSuit() == Club)
        if (user->askForSkillInvoke("thyudu_use", "change")) {
            QString name = room->askForChoice(user, "thyudu", names.join("+"));
            Card *c = Sanguosha->cloneCard(name);
            c->addSubcard(card);
            c->setSkillName("thyudu");
            return c;
        }
    return NULL;
}

class ThYuduViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThYuduViewAsSkill()
        : ZeroCardViewAsSkill("thyudu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPhase() != Player::NotActive || player->hasFlag("Global_ThYuduFailed") || player->hasFlag("thyudu"))
            return false;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
        if (pattern == "slash")
            return true;
        else if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic"))
            return true;
        return false;
    }

    virtual const Card *viewAs() const
    {
        ThYuduCard *card = new ThYuduCard;
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
            pattern = "analeptic";
        card->setUserString(pattern);
        return card;
    }
};

class ThYudu : public TriggerSkill
{
public:
    ThYudu()
        : TriggerSkill("thyudu")
    {
        events << CardAsked << EventPhaseChanging;
        view_as_skill = new ThYuduViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("thyudu"))
                        room->setPlayerFlag(p, "-thyudu");
                }
            }
            return QStringList();
        }
        ServerPlayer *current = room->getCurrent();
        if (!TriggerSkill::triggerable(player) || player->hasFlag("thyudu"))
            return QStringList();
        if (!room->isSomeonesTurn() || current == player || current->isKongcheng())
            return QStringList();
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return QStringList();
        QString pattern = data.toStringList().first();
        if (pattern == "jink")
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->setPlayerFlag(player, "thyudu");
        ServerPlayer *current = room->getCurrent();
        int card_id = room->askForCardChosen(player, current, "h", "thyudu");
        room->showCard(current, card_id);

        const Card *card = Sanguosha->getCard(card_id);
        if (card->objectName() == "jink")
            if (player->askForSkillInvoke("thyudu_use", "use")) {
                room->provide(card);
                return true;
            }
        if (card->getSuit() == Card::Club)
            if (player->askForSkillInvoke("thyudu_use", "change")) {
                Card *c = Sanguosha->cloneCard("jink");
                c->addSubcard(card);
                c->setSkillName("thyudu");
                room->provide(c);
                return true;
            }
        return false;
    }
};

ThZhaoguoCard::ThZhaoguoCard()
{
    target_fixed = true;
}

void ThZhaoguoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<int> card_ids = room->getNCards(subcardsLength(), false);
    CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "thzhaoguo", QString());
    CardsMoveStruct move(card_ids, NULL, Player::PlaceTable, reason);
    room->moveCardsAtomic(move, true);
    room->getThread()->delay();
    QList<int> spade, other;
    foreach (int id, card_ids) {
        if (Sanguosha->getCard(id)->getSuit() == Spade)
            spade << id;
        else
            other << id;
    }
    DummyCard *dummy = new DummyCard;
    if (!other.isEmpty()) {
        dummy->addSubcards(other);
        source->obtainCard(dummy);
    }
    dummy->clearSubcards();
    if (!spade.isEmpty()) {
        dummy->addSubcards(spade);
        source->tag["ThZhaoguoData"] = QVariant::fromValue(dummy);
        ServerPlayer *target = room->askForPlayerChosen(source, room->getOtherPlayers(source), "thzhaoguo");
        source->tag.remove("ThZhaoguoData");
        target->obtainCard(dummy);
        if (target->getHandcardNum() > source->getHandcardNum())
            target->turnOver();
    }
    delete dummy;
}

class ThZhaoguo : public ViewAsSkill
{
public:
    ThZhaoguo()
        : ViewAsSkill("thzhaoguo")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !Self->isJilei(to_select) && to_select->getSuit() == Card::Club;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;
        ThZhaoguoCard *card = new ThZhaoguoCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("ThZhaoguoCard");
    }
};

ThLunminCard::ThLunminCard()
{
    target_fixed = true;
}

void ThLunminCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(1);
}

class ThLunmin : public OneCardViewAsSkill
{
public:
    ThLunmin()
        : OneCardViewAsSkill("thlunmin")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->usedTimes("ThLunminCard") < 3;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThLunminCard *card = new ThLunminCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThYupan : public TriggerSkill
{
public:
    ThYupan()
        : TriggerSkill("thyupan")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        foreach (ServerPlayer *p, player->getRoom()->getAlivePlayers())
            if (p->isWounded())
                return TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish
                    && player->getMark("thyupan") == 0xF;
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (p->isWounded())
                targets << p;
        if (targets.isEmpty())
            return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thyupan", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThYupanTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThYupanTarget"].value<ServerPlayer *>();
        player->tag.value("ThYupanTarget");
        if (target)
            room->recover(target, RecoverStruct(player));
        return false;
    }
};

class ThYupanRecord : public TriggerSkill
{
public:
    ThYupanRecord()
        : TriggerSkill("#thyupan-record")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded << BeforeCardsMove;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                if (player->getMark("thyupan") > 0)
                    room->setPlayerMark(player, "thyupan", 0);
        } else {
            if (!player->isAlive() || player->getPhase() == Player::NotActive)
                return QStringList();
            if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
                const Card *card = NULL;
                if (triggerEvent == PreCardUsed) {
                    card = data.value<CardUseStruct>().card;
                } else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    if (resp.m_isUse)
                        card = resp.m_card;
                }
                if (!card)
                    return QStringList();
                recordThYupanCardSuit(room, player, card);
            } else if (triggerEvent == BeforeCardsMove) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (player != move.from
                    || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_DISCARD))
                    return QStringList();
                foreach (int id, move.card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    recordThYupanCardSuit(room, player, c);
                }
            }
        }
        return QStringList();
    }

private:
    void recordThYupanCardSuit(Room *room, ServerPlayer *player, const Card *card) const
    {
        if (player->getMark("thyupan") == 0xF)
            return;
        int suitID = (1 << int(card->getSuit()));
        int mark = player->getMark("thyupan");
        if ((mark & suitID) == 0)
            room->setPlayerMark(player, "thyupan", mark | suitID);
    }
};

class ThJiuzhang : public FilterSkill
{
public:
    ThJiuzhang()
        : FilterSkill("thjiuzhang")
    {
    }

    static WrappedCard *changeToNine(int cardId)
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("thjiuzhang");
        new_card->setNumber(9);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->getNumber() > 9;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        return changeToNine(originalCard->getEffectiveId());
    }
};

ThShushuCard::ThShushuCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void ThShushuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    LogMessage log;
    log.type = "$ThShushu";
    log.from = card_use.from;
    log.arg = "thshushu";
    log.card_str = IntList2StringList(subcards).join("+");
    room->sendLog(log);

    CardMoveReason reason(CardMoveReason::S_REASON_OVERRIDE, card_use.from->objectName(), "thshushu", QString());
    QList<CardsMoveStruct> moves;
    foreach (int id, subcards) {
        CardsMoveStruct move(id, NULL, Player::PlaceTable, reason);
        moves.append(move);
    }
    room->moveCardsAtomic(moves, true);
}

class ThShushuViewAsSkill : public ViewAsSkill
{
public:
    ThShushuViewAsSkill()
        : ViewAsSkill("thshushu")
    {
        response_pattern = "@@thshushu";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        int num = Self->getMark("thshushu");
        int n = 0;
        foreach (const Card *c, selected)
            n += c->getNumber();
        if (n >= num)
            return false;
        return !Self->isJilei(to_select) && to_select->getNumber() <= num - n;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        int num = Self->getMark("thshushu");
        int n = 0;
        foreach (const Card *c, cards)
            n += c->getNumber();
        if (num != n)
            return NULL;
        ThShushuCard *card = new ThShushuCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThShushu : public TriggerSkill
{
public:
    ThShushu()
        : TriggerSkill("thshushu")
    {
        events << TargetSpecifying << TargetConfirming;
        view_as_skill = new ThShushuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (triggerEvent == TargetSpecifying || (triggerEvent == TargetConfirming && use.to.contains(player))) {
                if ((use.card->isKindOf("BasicCard") || (use.card->isNDTrick() && use.card->isBlack()))
                    && use.card->getNumber() > 0) {
                    QList<int> ids;
                    if (use.card->isVirtualCard())
                        ids = use.card->getSubcards();
                    else
                        ids << use.card->getEffectiveId();
                    if (ids.length() > 0) {
                        bool all_place_table = true;
                        foreach (int id, ids) {
                            if (room->getCardPlace(id) != Player::PlaceTable) {
                                all_place_table = false;
                                break;
                            }
                        }
                        if (all_place_table)
                            return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->setPlayerMark(ask_who, "thshushu", use.card->getNumber());
        ask_who->tag["ThShushuData"] = data;
        const Card *card = room->askForUseCard(ask_who, "@@thshushu", "@thshushu", -1, Card::MethodNone);
        ask_who->tag.remove("ThShushuData");
        room->setPlayerMark(ask_who, "thshushu", 0);
        if (card)
            ask_who->tag["ThShushuCard"] = QVariant::fromValue(card);
        return card;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        const Card *card = ask_who->tag["ThShushuCard"].value<const Card *>();
        ask_who->tag.remove("ThShushuCard");
        if (card) {
            CardUseStruct use = data.value<CardUseStruct>();
            bool is_int = false;
            int drank = use.card->tag["drank"].toInt(&is_int);
            room->obtainCard(ask_who, use.card);
            Card *new_card = Sanguosha->cloneCard(use.card->getClassName(), use.card->getSuit(), use.card->getNumber(),
                                                  use.card->getFlags());
            new_card->addSubcards(card->getSubcards());
            new_card->setSkillName(objectName());
            delete use.card;
            if (is_int && drank > 0)
                new_card->setTag("drank", drank);
            use.card = new_card;
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

ThFenglingCard::ThFenglingCard()
{
    target_fixed = true;
}

class ThFenglingViewAsSkill : public ViewAsSkill
{
public:
    ThFenglingViewAsSkill()
        : ViewAsSkill("thfengling")
    {
        response_pattern = "@@thfengling";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        int n = 0;
        foreach (const Card *cd, selected)
            n += cd->getNumber();
        if (n >= 9)
            return false;
        return n + to_select->getNumber() <= 9;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        int n = 0;
        foreach (const Card *cd, cards)
            n += cd->getNumber();
        if (n != 9)
            return NULL;

        ThFenglingCard *card = new ThFenglingCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThFengling : public TriggerSkill
{
public:
    ThFengling()
        : TriggerSkill("thfengling")
    {
        events << Dying;
        frequency = Limited;
        limit_mark = "@fengling";
        view_as_skill = new ThFenglingViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who != player || player->getMark("@fengling") == 0)
            return QStringList();
        if (player->getHp() > 0)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->removePlayerMark(player, "@fengling");
            room->addPlayerMark(player, "@fenglingused");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(5, objectName());

        while (room->askForUseCard(player, "@@thfengling", "@thfengling", -1, Card::MethodDiscard)) {
            room->recover(player, RecoverStruct(player));
            if (!player->isWounded())
                break;
            if (!player->canDiscard(player, "he"))
                break;
        }
        return false;
    }
};

ThYingshiCard::ThYingshiCard()
{
    mute = true;
    target_fixed = true;
}

const Card *ThYingshiCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;
    Room *room = player->getRoom();
    int n = player->usedTimes("ThYingshiCard") + 1;
    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getAllPlayers())
        if (player->distanceTo(p) == n)
            targets << p;
    if (targets.isEmpty()) {
        room->setPlayerFlag(player, "Global_ThYingshiFailed");
        return NULL;
    }

    player->setFlags("ThYingshiUse");
    bool use = room->askForUseSlashTo(player, targets, "@thyingshi:::" + QString::number(n), false, true);
    if (!use) {
        player->setFlags("-ThYingshiUse");
        room->setPlayerFlag(player, "Global_ThYingshiFailed");
    } else {
        if (n != 1)
            room->setPlayerMark(player, "@yingshi" + QString::number(n - 1), 0);
        room->addPlayerHistory(player, "ThYingshiCard");
        room->setPlayerMark(player, "@yingshi" + QString::number(n), 1);
    }
    return NULL;
}

class ThYingshiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThYingshiViewAsSkill()
        : ZeroCardViewAsSkill("thyingshi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        int n = player->usedTimes("ThYingshiCard");
        if (n >= 3 || player->hasFlag("Global_ThYingshiFailed"))
            return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (player->distanceTo(p) == n + 1 && player->canSlash(p, false))
                return true;
        }
        return false;
    }

    virtual const Card *viewAs() const
    {
        return new ThYingshiCard;
    }
};

class ThYingshi : public TriggerSkill
{
public:
    ThYingshi()
        : TriggerSkill("thyingshi")
    {
        events << EventPhaseChanging;
        view_as_skill = new ThYingshiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return QStringList();
        room->setPlayerMark(player, "@yingshi1", 0);
        room->setPlayerMark(player, "@yingshi2", 0);
        room->setPlayerMark(player, "@yingshi3", 0);
        return QStringList();
    }
};

class ThZanghun : public TriggerSkill
{
public:
    ThZanghun()
        : TriggerSkill("thzanghun")
    {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card->isKindOf("Slash"))
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1);
        room->addPlayerMark(player, "@gallop");
        return false;
    }
};

class ThZanghunClear : public TriggerSkill
{
public:
    ThZanghunClear()
        : TriggerSkill("#thzanghun-clear")
    {
        events << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!player || player->getMark("@gallop") == 0)
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return QStringList();
        room->setPlayerMark(player, "@gallop", 0);
        return QStringList();
    }
};

class ThZanghunDistance : public DistanceSkill
{
public:
    ThZanghunDistance()
        : DistanceSkill("#thzanghun-distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        return from->getMark("@gallop");
    }
};

class ThYimeng : public TriggerSkill
{
public:
    ThYimeng()
        : TriggerSkill("thyimeng")
    {
        events << Damage;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        if (!player->isKongcheng()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (player == p)
                    continue;
                if (damage.to == p)
                    skill_list.insert(p, QStringList(objectName()));
                else if (!damage.to->isRemoved() && !p->isRemoved() && p->isAdjacentTo(damage.to))
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
        int card_id = room->askForCardChosen(ask_who, player, "h", objectName());
        room->obtainCard(ask_who, card_id, false);
        player->drawCards(1, objectName());
        return false;
    }
};

ThXuyouCard::ThXuyouCard()
{
    target_fixed = true;
    mute = true;
}

const Card *ThXuyouCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;
    Room *room = player->getRoom();
    room->setPlayerFlag(player, "ThXuyouUse");
    bool use = room->askForUseCard(player, "slash", "@thxuyou");
    if (!use) {
        room->setPlayerFlag(player, "Global_ThXuyouFailed");
        room->setPlayerFlag(player, "-ThXuyouUse");
        return NULL;
    }
    return this;
}

const Card *ThXuyouCard::validateInResponse(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    room->setPlayerFlag(player, "ThXuyouUse");
    bool use = room->askForUseCard(player, "slash", "@thxuyou");
    if (!use) {
        room->setPlayerFlag(player, "Global_ThXuyouFailed");
        room->setPlayerFlag(player, "-ThXuyouUse");
        return NULL;
    }
    return this;
}

void ThXuyouCard::onUse(Room *, const CardUseStruct &) const
{
    // do nothing
}

class ThXuyou : public ZeroCardViewAsSkill
{
public:
    ThXuyou()
        : ZeroCardViewAsSkill("thxuyou")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("ThXuyou") && !player->hasFlag("Global_ThXuyouFailed") && Slash::IsAvailable(player);
    }

    virtual const Card *viewAs() const
    {
        return new ThXuyouCard;
    }
};

class ThXuyouTrigger : public TriggerSkill
{
public:
    ThXuyouTrigger()
        : TriggerSkill("#thxuyou")
    {
        events << ChoiceMade << CardFinished << PreDamageDone << Damage;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == ChoiceMade && player->hasFlag("ThXuyouUse") && data.canConvert<CardUseStruct>()) {
            room->broadcastSkillInvoke("thxuyou");
            room->notifySkillInvoked(player, "thxuyou");

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = "thxuyou";
            room->sendLog(log);

            room->setPlayerFlag(player, "-ThXuyouUse");
            room->setCardFlag(data.value<CardUseStruct>().card, "thxuyou_slash");
            room->setPlayerFlag(player, "ThXuyou");
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->hasFlag("thxuyou_slash") && !use.card->hasFlag("ThXuyouDamage")) {
                if (!player->isRemoved()) {
                    foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                        if (p->isRemoved() || !player->isAdjacentTo(p))
                            continue;
                        if (player->canDiscard(p, "he"))
                            return QStringList(objectName());
                    }
                }
            }
        } else if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag("thxuyou_slash"))
                room->setCardFlag(damage.card, "ThXuyouDamage");
        } else if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag("thxuyou_slash"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "thxuyou");
        room->broadcastSkillInvoke("thxuyou");
        if (triggerEvent == CardFinished) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->isRemoved() || !player->isAdjacentTo(p))
                    continue;
                if (player->canDiscard(p, "he")) {
                    int card_id = room->askForCardChosen(player, p, "he", "thxuyou", false, Card::MethodDiscard);
                    room->throwCard(card_id, p, player);
                }
            }
        } else
            player->drawCards(1, objectName());
        return false;
    }
};

class ThHuanghu : public TriggerSkill
{
public:
    ThHuanghu()
        : TriggerSkill("thhuanghu")
    {
        events << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Jink") && resp.m_who && resp.m_who->isAlive()) {
                if (player->canDiscard(player, "h"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *target = data.value<CardResponseStruct>().m_who;
        player->tag["ThHuanghuTarget"] = QVariant::fromValue(target);
        const Card *card
            = room->askForExchange(player, objectName(), 998, 1, false, "@thhuanghu:" + target->objectName(), true);
        player->tag.remove("ThHuanghuTarget");
        if (card) {
            LogMessage log;
            log.type = "#ChoosePlayerWithSkill";
            log.from = player;
            log.to << target;
            log.arg = objectName();
            room->sendLog(log);
            room->throwCard(card, player);
            player->tag["ThHuanghuNum"] = card->subcardsLength();
            room->broadcastSkillInvoke(objectName());
            delete card;
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        int n = player->tag["ThHuanghuNum"].toInt();
        player->tag.remove("ThHuanghuNum");
        if (n > 0) {
            ServerPlayer *target = data.value<CardResponseStruct>().m_who;
            if (target->canDiscard(target, "he"))
                room->askForDiscard(target, objectName(), n, n, false, true);
        }
        return false;
    }
};

class ThLinyao : public TriggerSkill
{
public:
    ThLinyao()
        : TriggerSkill("thlinyao")
    {
        events << CardAsked;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        QString asked = data.toStringList().first();
        if (asked == "jink") {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p == player)
                    continue;
                if (!p->faceUp())
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            ask_who->turnOver();
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        Jink *jink = new Jink(Card::NoSuit, 0);
        jink->setSkillName("_thlinyao");
        room->provide(jink);
        return true;
    }
};

class ThFeijing : public TriggerSkill
{
public:
    ThFeijing()
        : TriggerSkill("thfeijing")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.is_last_handcard)
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getHandcardNum() < player->getMaxHp())
            player->drawCards(player->getMaxHp() - player->getHandcardNum(), objectName());
        player->turnOver();
        return false;
    }
};

class ThOuji : public TriggerSkill
{
public:
    ThOuji()
        : TriggerSkill("thouji")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QStringList skills;
        if (TriggerSkill::triggerable(player) && room->isSomeonesTurn(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from->isAlive() && move.from_places.contains(Player::PlaceEquip))
                skills << objectName();
        }
        return skills;
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
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (player->canDiscard(p, "h"))
                targets << p;
        }
        ServerPlayer *target = NULL;
        if (!targets.isEmpty())
            target = room->askForPlayerChosen(player, targets, objectName(), "@thouji", true);
        if (target) {
            int card_id = room->askForCardChosen(player, target, "h", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, target, player);
        } else
            player->drawCards(1, objectName());
        return false;
    }
};

ThJingyuanspCard::ThJingyuanspCard()
{
}

bool ThJingyuanspCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.length() < 2 && to_select->hasEquip();
}

void ThJingyuanspCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    QString pattern = ".|.|.|equipped";
    QString prompt = "@thjingyuansp";
    if (effect.from == effect.to || effect.to->isKongcheng()) {
        pattern += "!";
        prompt += "-give";
    }
    prompt += QString(":%1").arg(effect.from->objectName());
    const Card *card = room->askForCard(effect.to, pattern, prompt, QVariant(), Card::MethodNone);
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.to->objectName(), "thjingyuansp", QString());
    if (card) {
        effect.to->tag["ThJingyuanspCard"] = QVariant::fromValue(card); // for AI
        ServerPlayer *target = room->askForPlayerChosen(effect.to, room->getOtherPlayers(effect.to), "thjingyuansp");
        effect.to->tag.remove("ThJingyuanspCard");
        reason.m_targetId = target->objectName();
        room->obtainCard(target, card, reason);
    } else if (pattern.endsWith("!")) {
        QList<const Card *> equips = effect.to->getEquips();
        if (!equips.isEmpty()) {
            card = equips.at(qrand() % equips.length());
            QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
            ServerPlayer *target = targets.at(qrand() % targets.length());
            reason.m_targetId = target->objectName();
            room->obtainCard(target, card, reason);
        }
    } else if (!effect.to->isKongcheng()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "thjingyuansp");
        room->obtainCard(effect.from, card_id, false);
    }
}

class ThJingyuansp : public OneCardViewAsSkill
{
public:
    ThJingyuansp()
        : OneCardViewAsSkill("thjingyuansp")
    {
        filter_pattern = "BasicCard|red!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThJingyuanspCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new ThJingyuanspCard;
        card->addSubcard(originalCard);
        return card;
    }
};

ThFeihuCard::ThFeihuCard()
{
}

bool ThFeihuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->getHp() <= Self->getHp() && (Self->getLostHp() > 2 || to_select != Self);
}

void ThFeihuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    QStringList choices;
    if (effect.from->canDiscard(effect.to, "he"))
        choices << "recover";
    choices << "damage";
    QString choice = room->askForChoice(effect.from, "thfeihu", choices.join("+"), QVariant::fromValue(effect.to));
    if (choice == "recover") {
        if (effect.from == effect.to)
            room->askForDiscard(effect.to, "thfeihu", 1, 1, false, true);
        else {
            int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thfeihu", false, Card::MethodDiscard);
            room->throwCard(card_id, effect.to, effect.from);
        }
        room->recover(effect.to, RecoverStruct(effect.from));
    } else {
        room->damage(DamageStruct("thfeihu", effect.from, effect.to));
        if (effect.to->isAlive()) {
            room->recover(effect.to, RecoverStruct(effect.from));
            effect.to->drawCards(1, "thfeihu");
        }
    }
}

class ThFeihu : public ZeroCardViewAsSkill
{
public:
    ThFeihu()
        : ZeroCardViewAsSkill("thfeihu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThFeihuCard");
    }

    virtual const Card *viewAs() const
    {
        return new ThFeihuCard;
    }
};

class ThHuanling : public TriggerSkill
{
public:
    ThHuanling()
        : TriggerSkill("thhuanling")
    {
        events << Death;
        frequency = Frequent;
    }

    virtual bool cost(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        if (p->askForSkillInvoke(objectName())) {
            r->broadcastSkillInvoke(objectName());
            p->drawCards(2, objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        QStringList skills;
        skills << "ikchilian"
               << "ikmeiying"
               << "ikjingmu";
        foreach (QString skill, skills) {
            if (p->hasSkill(skill))
                skills.removeAll(skill);
        }
        if (skills.length() == 0)
            return false;
        QString choice = r->askForChoice(p, objectName(), skills.join("+"));
        r->acquireSkill(p, choice);
        return false;
    }
};

class ThYoukong : public DistanceSkill
{
public:
    ThYoukong()
        : DistanceSkill("thyoukong")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (!from->hasSkill(objectName()))
            return 0;
        int n = 0;
        if (from->isFemale())
            --n;
        foreach (const Player *p, from->getAliveSiblings()) {
            if (p->isFemale())
                --n;
        }
        return n;
    }
};

ThGuanzhiCard::ThGuanzhiCard()
{
}

bool ThGuanzhiCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const
{
    const Player *lord = NULL;
    if (Self->isLord())
        lord = Self;
    else {
        foreach (const Player *p, Self->getAliveSiblings()) {
            if (p->isLord()) {
                lord = p;
                break;
            }
        }
    }
    return lord && to_select->inMyAttackRange(lord) && Self->canDiscard(to_select, "he");
}

void ThGuanzhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets) {
        if (source->canDiscard(p, "he")) {
            int card_id = room->askForCardChosen(source, p, "he", "thguanzhi", false, MethodDiscard);
            room->throwCard(card_id, p, source);
        }
    }
    room->drawCards(targets, 1, "thguanzhi");
    foreach (ServerPlayer *p, targets) {
        if (p->getHandcardNum() > room->getLord()->getHandcardNum()) {
            source->drawCards(1, "thguanzhi");
            break;
        }
    }
}

class ThGuanzhiVS : public ZeroCardViewAsSkill
{
public:
    ThGuanzhiVS()
        : ZeroCardViewAsSkill("thguanzhi")
    {
        response_pattern = "@@thguanzhi";
    }

    virtual const Card *viewAs() const
    {
        return new ThGuanzhiCard;
    }
};

class ThGuanzhi : public TriggerSkill
{
public:
    ThGuanzhi()
        : TriggerSkill("thguanzhi")
    {
        events << EventPhaseStart;
        view_as_skill = new ThGuanzhiVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->inMyAttackRange(room->getLord()) && player->canDiscard(p, "he"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thguanzhi", "@thguanzhi");
    }
};

ThFuhuaCard::ThFuhuaCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void ThFuhuaCard::onUse(Room *, const CardUseStruct &card_use) const
{
    card_use.from->tag["ThFuhuaIds"] = QVariant::fromValue(IntList2VariantList(subcards));
}

class ThFuhuaVS : public ViewAsSkill
{
public:
    ThFuhuaVS()
        : ViewAsSkill("thfuhua")
    {
        response_pattern = "@@thfuhua";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *) const
    {
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return NULL;
        else {
            ThFuhuaCard *card = new ThFuhuaCard;
            card->addSubcards(cards);
            return card;
        }
    }
};

class ThFuhua : public TriggerSkill
{
public:
    ThFuhua()
        : TriggerSkill("thfuhua")
    {
        events << DamageInflicted;
        view_as_skill = new ThFuhuaVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.from != player
            && damage.from->getMark("thfuhua_" + player->objectName()) == 0) {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thfuhua", "@thfuhua", -1, Card::MethodNone);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->addPlayerMark(damage.from, "@comfort");
        room->setPlayerMark(damage.from, "thfuhua_" + player->objectName(), 1);
        QList<int> card_ids = VariantList2IntList(player->tag["ThFuhuaIds"].toList());

        room->fillAG(card_ids, damage.from);

        LogMessage log2;
        log2.type = "$ShowCard";
        log2.from = player;
        log2.card_str = IntList2StringList(card_ids).join("+");
        room->sendLog(log2, damage.from);

        LogMessage log;
        log.type = "#ThFuhua";
        log.from = player;
        log.to << damage.from;
        log.arg = QString::number(card_ids.length());
        room->sendLog(log, room->getOtherPlayers(damage.from));

        int card_id = room->askForAG(damage.from, card_ids, damage.from->getCardCount() >= card_ids.length(), objectName());
        room->clearAG(damage.from);
        if (card_id != -1) {
            room->obtainCard(damage.from, card_id);
            LogMessage log;
            log.type = "#ThFuhua2";
            log.from = player;
            log.to << damage.from;
            log.arg = QString::number(damage.damage);
            room->sendLog(log);
            return true;
        } else
            room->askForDiscard(damage.from, objectName(), card_ids.length(), card_ids.length(), false, true);
        return false;
    }
};

class ThYongjieViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThYongjieViewAsSkill()
        : ZeroCardViewAsSkill("thyongjie")
    {
        response_pattern = "@@thyongjie";
    }

    virtual const Card *viewAs() const
    {
        const Card *coll = Card::Parse(Self->property("extra_collateral").toString());
        if (!coll)
            return NULL;
        if (coll->isKindOf("Collateral"))
            return new ExtraCollateralCard;
        else
            return new ExtraFeintAttackCard;
    }
};

class ThYongjie : public TriggerSkill
{
public:
    ThYongjie()
        : TriggerSkill("thyongjie")
    {
        events << TargetSpecified << PreCardUsed;
        view_as_skill = new ThYongjieViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (e == PreCardUsed && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isBlack() && use.from->getMark("@doom") > 0
                && (use.card->isKindOf("Collateral") || use.card->isKindOf("FeintAttack") || use.card->isKindOf("ExNihilo")))
                return QStringList(objectName());
        } else if (e == TargetSpecified && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") || (use.card->isNDTrick() && use.card->isBlack())) {
                if (use.to.length() < player->getMark("@doom"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent e, Room *r, ServerPlayer *player, QVariant &d, ServerPlayer *) const
    {
        if (e == PreCardUsed) {
            CardUseStruct use = d.value<CardUseStruct>();
            for (int i = 0; i < player->getMark("@doom"); ++i) {
                ServerPlayer *extra = NULL;
                if (use.card->isKindOf("ExNihilo")) {
                    QList<ServerPlayer *> targets;
                    foreach (ServerPlayer *p, r->getAlivePlayers()) {
                        if (!use.to.contains(p) && !r->isProhibited(player, p, use.card))
                            targets << p;
                    }
                    extra = r->askForPlayerChosen(player, targets, objectName(), "@thyongye-add:::" + use.card->objectName(),
                                                  true);
                } else if (use.card->isKindOf("Collateral") || use.card->isKindOf("FeintAttack")) {
                    QStringList tos;
                    foreach (ServerPlayer *t, use.to)
                        tos.append(t->objectName());
                    r->setPlayerProperty(player, "extra_collateral", use.card->toString());
                    r->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
                    bool used = r->askForUseCard(player, "@@thyongjie", "@thyongye-add:::" + use.card->objectName());
                    r->setPlayerProperty(player, "extra_collateral", QString());
                    r->setPlayerProperty(player, "extra_collateral_current_targets", QString());
                    if (!used)
                        return false;
                    foreach (ServerPlayer *p, r->getOtherPlayers(player)) {
                        if (p->hasFlag("ExtraCollateralTarget")) {
                            p->setFlags("-ExtraCollateralTarget");
                            extra = p;
                            break;
                        }
                    }
                }
                if (extra) {
                    use.to.append(extra);
                    r->sortByActionOrder(use.to);

                    LogMessage log;
                    log.type = "#ThYongyeAdd";
                    log.from = player;
                    log.to << extra;
                    log.arg = objectName();
                    log.card_str = use.card->toString();
                    r->sendLog(log);
                    r->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());

                    if (use.card->isKindOf("Collateral")) {
                        ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                        if (victim) {
                            LogMessage log;
                            log.type = "#CollateralSlash";
                            log.from = player;
                            log.to << victim;
                            r->sendLog(log);
                            r->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                        }
                    } else if (use.card->isKindOf("FeintAttack")) {
                        ServerPlayer *victim = extra->tag["feintTarget"].value<ServerPlayer *>();
                        if (victim) {
                            LogMessage log;
                            log.type = "#FeintAttackWest";
                            log.from = player;
                            log.to << victim;
                            r->sendLog(log);
                            r->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                        }
                    }
                } else
                    break;
            }
            d = QVariant::fromValue(use);
            return false;
        } else
            return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        room->setPlayerMark(player, "doom", 0);
        player->loseAllMarks("@doom");
        return false;
    }
};

class ThYongjieTargetMod : public TargetModSkill
{
public:
    ThYongjieTargetMod()
        : TargetModSkill("#thyongjie-tar")
    {
        frequency = NotCompulsory;
        pattern = "Slash#TrickCard+^DelayedTrick+^Collateral+^FeintAttack+^ExNihilo|black";
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasSkill("thyongjie"))
            return from->getMark("@doom");
        return 0;
    }
};

class ThYongjieRecord : public TriggerSkill
{
public:
    ThYongjieRecord()
        : TriggerSkill("#thyongjie-record")
    {
        events << PreDamageDone << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (e == EventAcquireSkill || e == EventLoseSkill) {
            if (data.toString() != "thyongjie")
                return QStringList();
            int num = player->getMark("doom");
            if (num > 0) {
                num = e == EventAcquireSkill ? num : 0;
                room->setPlayerMark(player, "@doom", num);
            }
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->addPlayerMark(damage.from, "doom");
        if (damage.from->hasSkill("thyongjie"))
            room->setPlayerMark(damage.from, "@doom", damage.from->getMark("doom"));
        return false;
    }
};

ThHuanyaoCard::ThHuanyaoCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

bool ThHuanyaoCard::targetFixed() const
{
    if (Self && Self->getSkillStep("thhuanyao") == 2)
        return true;
    else
        return false;
}

bool ThHuanyaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int dis = 998;
    foreach (const Player *p, Self->getAliveSiblings()) {
        int n = Self->distanceTo(p);
        if (n != -1 && n < dis)
            dis = n;
    }
    return targets.isEmpty() && Self->distanceTo(to_select) == dis && to_select != Self;
}

void ThHuanyaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->showCard(source, getEffectiveId());
    ServerPlayer *target = targets.isEmpty() ? source : targets.first();

    QMap<QString, QStringList> name_map;
    name_map["basic"] = QStringList();
    name_map["single_target_trick"] = QStringList();
    name_map["other_trick"] = QStringList();

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach (const Card *card, cards) {
        QString obj_name = card->objectName();
        if (obj_name.endsWith("slash"))
            obj_name = "slash";
        if (card->getTypeId() == TypeBasic) {
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
    QStringList choices;
    if (!name_map["basic"].isEmpty())
        choices << name_map["basic"].join("+");
    if (source->getSkillStep("thhuanyao") > 0) {
        if (!name_map["single_target_trick"].isEmpty())
            choices << name_map["single_target_trick"].join("+");
        if (!name_map["other_trick"].isEmpty())
            choices << name_map["other_trick"].join("+");
    }

    QString obj_n = room->askForChoice(target, "thhuanyao", choices.join("|"));

    LogMessage log;
    log.type = "#RhHuanjie";
    log.from = target;
    log.arg = obj_n;
    room->sendLog(log);

    room->setPlayerProperty(source, "thhuanyao", QString("%1->%2").arg(getEffectiveId()).arg(obj_n));
}

class ThHuanyaoVS : public OneCardViewAsSkill
{
public:
    ThHuanyaoVS()
        : OneCardViewAsSkill("thhuanyao")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        if (Self->property("thhuanyao").toString().isEmpty())
            return !to_select->isEquipped();
        else {
            QStringList map = Self->property("thhuanyao").toString().split("->");
            return !to_select->isEquipped() && map.first().toInt() == to_select->getId();
        }
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (Self->property("thhuanyao").toString().isEmpty()) {
            Card *card = new ThHuanyaoCard;
            card->addSubcard(originalCard);
            return card;
        } else {
            QStringList map = Self->property("thhuanyao").toString().split("->");
            Card *card = Sanguosha->cloneCard(map.last(), originalCard->getSuit(), originalCard->getNumber());
            card->addSubcard(originalCard);
            card->setSkillName("thhuanyao");
            return card;
        }
        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->property("thhuanyao").toString().isEmpty()) {
            return !player->hasUsed("ThHuanyaoCard");
        } else {
            QStringList map = player->property("thhuanyao").toString().split("->");
            Card *card = Sanguosha->cloneCard(map.last());
            card->setSkillName("thhuanyao");
            card->deleteLater();
            return card->isAvailable(player);
        }
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (!player->property("thhuanyao").toString().isEmpty()) {
            QStringList map = player->property("thhuanyao").toString().split("->");
            return pattern.contains(map.last());
        }
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (!player->property("thhuanyao").toString().isEmpty()) {
            QStringList map = player->property("thhuanyao").toString().split("->");
            return map.last() == "nullification";
        }
        return false;
    }
};

class ThHuanyao : public TriggerSkill
{
public:
    ThHuanyao()
        : TriggerSkill("thhuanyao")
    {
        events << EventPhaseChanging;
        view_as_skill = new ThHuanyaoVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (data.value<PhaseChangeStruct>().to == Player::NotActive)
            room->setPlayerProperty(player, "thhuanyao", QString());
        return QStringList();
    }
};

class ThHuanyaoProhibit : public ProhibitSkill
{
public:
    ThHuanyaoProhibit()
        : ProhibitSkill("#thhuanyao")
    {
        frequency = NotCompulsory;
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return from == to && card->getSkillName() == "thhuanyao";
    }
};

class ThZhouzhu : public TriggerSkill
{
public:
    ThZhouzhu()
        : TriggerSkill("thzhouzhu")
    {
        events << Damaged << GameStart;
        frequency = Frequent;
    }

    virtual bool cost(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (e == GameStart) {
            room->setPlayerProperty(player, "thzhouzhu_step", 1);
        } else {
            if (player->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QStringList choices;
        if (player->getSkillStep(objectName()) == 1)
            choices << "change1";
        if (player->getSkillStep(objectName()) == 2)
            choices << "change2";
        choices << "draw";
        QString choice = "draw";
        if (choices.length() > 1)
            choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "change1") {
            room->setPlayerProperty(player, "thzhouzhu_step", 2);
            room->setPlayerProperty(player, "thhuanyao_step", 1);
        } else if (choice == "change2") {
            room->setPlayerProperty(player, "thzhouzhu_step", 3);
            room->setPlayerProperty(player, "thhuanyao_step", 2);
        } else
            player->drawCards(1, objectName());

        return false;
    }
};

TouhouSPPackage::TouhouSPPackage()
    : Package("touhou-sp")
{
    General *sp001 = new General(this, "sp001", "kaze", 4, true, true);
    sp001->addSkill(new ThFanshi);
    sp001->addSkill(new ThJifeng);

    General *sp002 = new General(this, "sp002", "hana", 4, false);
    sp002->addSkill(new ThKongsuo);
    sp002->addSkill(new Skill("thguimen", Skill::Compulsory));
    sp002->addSkill(new ThLiuren);
    sp002->addSkill(new ThLiurenRecord);
    related_skills.insertMulti("thliuren", "#thliuren");

    General *sp003 = new General(this, "sp003", "yuki", 3);
    sp003->addSkill(new ThShenshi);
    sp003->addSkill(new ThJiefan);

    General *sp004 = new General(this, "sp004", "tsuki", 3);
    sp004->addSkill(new ThChuangshi);
    sp004->addSkill(new SlashNoDistanceLimitSkill("thchuangshi"));
    related_skills.insertMulti("thchuangshi", "#thchuangshi-slash-ndl");
    sp004->addSkill(new ThGaotian);

    General *sp005 = new General(this, "sp005", "kaze");
    sp005->addSkill(new ThWanling);
    sp005->addSkill(new ThZuibu);

    General *sp006 = new General(this, "sp006", "hana");
    sp006->addSkill(new ThModao);

    General *sp007 = new General(this, "sp007", "yuki", 3);
    sp007->addSkill(new ThMengsheng);
    sp007->addSkill(new ThMengshengClear);
    sp007->addSkill(new ThMengshengInvalidity);
    related_skills.insertMulti("thmengsheng", "#thmengsheng-clear");
    related_skills.insertMulti("thmengsheng", "#thmengsheng-inv");
    sp007->addSkill(new ThQixiang);

    General *sp008 = new General(this, "sp008", "tsuki");
    sp008->addSkill(new ThHuanlong);
    sp008->addSkill(new ThHuanlongTargetMod);
    sp008->addSkill(new ThHuanlongMaxCards);
    related_skills.insertMulti("thhuanlong", "#thhuanlong-target");
    related_skills.insertMulti("thhuanlong", "#thhuanlong-max-cards");

    General *sp009 = new General(this, "sp009", "kaze", 3);
    sp009->addSkill(new ThYudu);
    sp009->addSkill(new ThZhaoguo);

    General *sp010 = new General(this, "sp010", "hana", 3);
    sp010->addSkill(new ThLunmin);
    sp010->addSkill(new ThYupan);
    sp010->addSkill(new ThYupanRecord);
    related_skills.insertMulti("thyupan", "#thyupan-record");

    General *sp011 = new General(this, "sp011", "yuki");
    sp011->addSkill(new ThJiuzhang);
    sp011->addSkill(new ThShushu);
    sp011->addSkill(new ThFengling);

    General *sp012 = new General(this, "sp012", "tsuki");
    sp012->addSkill(new ThYingshi);
    sp012->addSkill(new ThZanghun);
    sp012->addSkill(new ThZanghunClear);
    sp012->addSkill(new ThZanghunDistance);
    related_skills.insertMulti("thzanghun", "#thzanghun-clear");
    related_skills.insertMulti("thzanghun", "#thzanghun-distance");

    General *sp013 = new General(this, "sp013", "kaze", 3, false);
    sp013->addSkill(new ThYimeng);
    sp013->addSkill(new ThXuyou);
    sp013->addSkill(new ThXuyouTrigger);
    related_skills.insertMulti("thxuyou", "#thxuyou");

    General *sp014 = new General(this, "sp014", "hana", 3);
    sp014->addSkill(new ThHuanghu);
    sp014->addSkill(new ThLinyao);
    sp014->addSkill(new ThFeijing);

    General *sp015 = new General(this, "sp015", "yuki", 3);
    sp015->addSkill(new ThOuji);
    sp015->addSkill(new ThJingyuansp);

    General *sp016 = new General(this, "sp016", "tsuki");
    sp016->addSkill(new ThFeihu);

    General *sp017 = new General(this, "sp017", "kaze");
    sp017->addSkill(new ThHuanling);
    sp017->addSkill(new ThYoukong);

    General *sp018 = new General(this, "sp018", "hana", 3);
    sp018->addSkill(new ThGuanzhi);
    sp018->addSkill(new ThFuhua);

    General *sp019 = new General(this, "sp019", "yuki");
    sp019->addSkill(new ThYongjie);
    sp019->addSkill(new ThYongjieTargetMod);
    sp019->addSkill(new ThYongjieRecord);
    related_skills.insertMulti("thyongye", "#thyongye-tar");
    related_skills.insertMulti("thyongye", "#thyongye-record");

    General *sp020 = new General(this, "sp020", "tsuki", 3, false);
    sp020->addSkill(new ThHuanyao);
    sp020->addSkill(new ThHuanyaoProhibit);
    related_skills.insertMulti("thhuanyao", "#thhuanyao");
    sp020->addSkill(new ThZhouzhu);

    /*General *sp999 = new General(this, "sp999", "te", 5, true, true);
    sp999->addSkill("thjibu");
    sp999->addSkill(new Skill("thfeiniang", Skill::Compulsory));*/

    addMetaObject<ThYuduCard>();
    addMetaObject<ThZhaoguoCard>();
    addMetaObject<ThLunminCard>();
    addMetaObject<ThShushuCard>();
    addMetaObject<ThFenglingCard>();
    addMetaObject<ThYingshiCard>();
    addMetaObject<ThXuyouCard>();
    addMetaObject<ThJingyuanspCard>();
    addMetaObject<ThFeihuCard>();
    addMetaObject<ThGuanzhiCard>();
    addMetaObject<ThFuhuaCard>();
    addMetaObject<ThHuanyaoCard>();
}

ADD_PACKAGE(TouhouSP)
