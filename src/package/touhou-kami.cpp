#include "touhou-kami.h"

#include "carditem.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"
#include "skill.h"

class ThKexing : public TriggerSkill
{
public:
    ThKexing()
        : TriggerSkill("thkexing")
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<int> card_ids = room->getNCards(3, false);
        QList<int> copy = card_ids;
        CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        CardsMoveStruct move(card_ids, NULL, Player::PlaceTable, reason);
        room->moveCardsAtomic(move, true);
        QList<int> remains;
        foreach (int id, card_ids)
            if (Sanguosha->getCard(id)->isKindOf("TrickCard")) {
                card_ids.removeOne(id);
                remains << id;
            }
        if (!card_ids.isEmpty()) {
            room->fillAG(copy, player, remains);
            while (!card_ids.isEmpty()) {
                int id = room->askForAG(player, card_ids, true, objectName());
                if (id == -1)
                    break;
                QList<ServerPlayer *> _player;
                _player << player;
                room->takeAG(NULL, id, false, _player);
                card_ids.removeOne(id);
                remains << id;
            }
            room->clearAG(player);
        }
        if (!card_ids.isEmpty()) {
            CardMoveReason reason2(CardMoveReason::S_REASON_PUT, QString());
            CardsMoveStruct move2(card_ids, NULL, NULL, Player::PlaceTable, Player::DrawPile, reason2);
            room->moveCardsAtomic(move2, true);
            room->askForGuanxing(player, card_ids, Room::GuanxingDownOnly);
        }
        if (!remains.isEmpty()) {
            DummyCard *dummy = new DummyCard;
            dummy->addSubcards(remains);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
        }
        return false;
    }
};

ThShenfengCard::ThShenfengCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

bool ThShenfengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->getHp() > qMax(0, Self->getHp());
}

void ThShenfengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thshenfeng", QString());
    room->obtainCard(target, this, reason);
    QList<ServerPlayer *> victims;
    foreach (ServerPlayer *p, room->getOtherPlayers(target))
        if (target->distanceTo(p) == 1)
            victims << p;

    if (victims.isEmpty())
        return;

    source->tag["ThShenfengTarget"] = QVariant::fromValue(target);
    ServerPlayer *victim = room->askForPlayerChosen(source, victims, "thshenfeng");
    source->tag.remove("ThShenfengTarget");

    if (!victim)
        victim = victims.first();
    room->damage(DamageStruct("thshenfeng", target, victim));
}

class ThShenfeng : public ViewAsSkill
{
public:
    ThShenfeng()
        : ViewAsSkill("thshenfeng")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 2)
            return false;
        else if (!selected.isEmpty())
            return to_select->getColor() == selected.first()->getColor();
        else
            return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;
        else {
            ThShenfengCard *card = new ThShenfengCard;
            card->addSubcards(cards);
            return card;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThShenfengCard");
    }
};

class ThKaihai : public TriggerSkill
{
public:
    ThKaihai()
        : TriggerSkill("thkaihai")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (TriggerSkill::triggerable(player) && move.from == player && move.from_places.contains(Player::PlaceHand)
            && move.is_last_handcard) {
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<int> card_ids;
        room->getThread()->trigger(FetchDrawPileCard, room, NULL);
        QList<int> &draw = room->getDrawPile();
        if (draw.isEmpty())
            room->swapPile();
        card_ids << draw.takeLast();
        CardsMoveStruct move;
        move.card_ids = card_ids;
        move.from = NULL;
        move.to = player;
        move.to_place = Player::PlaceHand;
        move.reason = CardMoveReason(CardMoveReason::S_REASON_DRAW, player->objectName(), objectName(), QString());
        room->moveCardsAtomic(move, false);
        return false;
    }
};

class ThTianbao : public TriggerSkill
{
public:
    ThTianbao()
        : TriggerSkill("thtianbao")
    {
        events << EventPhaseStart << GameStart;
        frequency = Compulsory;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *, ServerPlayer *p, QVariant &, ServerPlayer *&) const
    {
        if (e == GameStart && TriggerSkill::triggerable(p))
            return QStringList(objectName());
        else if (e == EventPhaseStart && TriggerSkill::triggerable(p)
                 && (p->getPhase() == Player::Start || p->getPhase() == Player::Finish))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        room->removeReihouCard(player);

        QStringList skills;
        const Package *reihoupack = Sanguosha->getPackage("tenshi-reihou");
        if (reihoupack) {
            QList<const General *> reihous = reihoupack->findChildren<const General *>();
            const General *reihou = reihous.at(qrand() % reihous.length());

            JsonArray args;
            args << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            args << player->objectName();
            args << reihou->objectName();
            args << QString();
            args << true;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            player->tag["Reihou"] = reihou->objectName();
            foreach (const Skill *skill, reihou->getVisibleSkillList())
                skills << skill->objectName();
        }
        room->handleAcquireDetachSkills(player, skills, true, true);
        return false;
    }
};

class ThWudao : public TriggerSkill
{
public:
    ThWudao()
        : TriggerSkill("thwudao")
    {
        events << GameStart << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == GameStart || (player->getPhase() == Player::Start && player->getHp() != player->getLostHp()))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        if (triggerEvent == GameStart)
            room->loseHp(player, 4);
        else {
            int delta = player->getHp() - player->getLostHp();
            if (delta > 0)
                room->loseHp(player, delta);
            else
                room->recover(player, RecoverStruct(player, NULL, -delta));
        }
        return false;
    }
};

class ThHuanjunFilterSkill : public FilterSkill
{
public:
    ThHuanjunFilterSkill()
        : FilterSkill("thhuanjun")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        if (to_select->isKindOf("Jink") && to_select->getSuit() == Card::Diamond)
            return true;
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return place == Player::PlaceHand && to_select->isKindOf("Armor");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard->isKindOf("Jink")) {
            Duel *duel = new Duel(originalCard->getSuit(), originalCard->getNumber());
            duel->setSkillName(objectName());
            WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
            card->takeOver(duel);
            return card;
        } else {
            Analeptic *anal = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
            anal->setSkillName(objectName());
            WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
            card->takeOver(anal);
            return card;
        }
    }
};

class ThHuanjun : public TriggerSkill
{
public:
    ThHuanjun()
        : TriggerSkill("thhuanjun")
    {
        events << BeforeCardsMove;
        frequency = Compulsory;
        view_as_skill = new ThHuanjunFilterSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!TriggerSkill::triggerable(player) || move.to != player)
            return QStringList();
        if (move.to_place == Player::PlaceEquip) {
            for (int i = 0; i < move.card_ids.length(); i++) {
                int card_id = move.card_ids[i];
                if (Sanguosha->getEngineCard(card_id)->isKindOf("Armor"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        DummyCard *dummy = new DummyCard;
        dummy->deleteLater();
        for (int i = 0; i < move.card_ids.length(); i++) {
            int card_id = move.card_ids[i];
            if (Sanguosha->getEngineCard(card_id)->isKindOf("Armor"))
                dummy->addSubcard(card_id);
        }
        if (dummy->subcardsLength() > 0) {
            move.removeCardIds(dummy->getSubcards());
            room->obtainCard(player, dummy);
            data = QVariant::fromValue(move);
        }
        return false;
    }
};

ThGugaoCard::ThGugaoCard()
{
}

bool ThGugaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThGugaoCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    source->pindian(targets.first(), "thgugao");
}

class ThGugaoViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThGugaoViewAsSkill()
        : ZeroCardViewAsSkill("thgugao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("ThGugaoCard");
    }

    virtual const Card *viewAs() const
    {
        return new ThGugaoCard;
    }
};

class ThGugao : public TriggerSkill
{
public:
    ThGugao()
        : TriggerSkill("thgugao")
    {
        events << Pindian;
        view_as_skill = new ThGugaoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != objectName())
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        ServerPlayer *win = NULL, *lose = NULL;
        if (pindian->isSuccess()) {
            win = pindian->from;
            lose = pindian->to;
            if (win->getMark("@huangyi") > 0)
                room->addPlayerHistory(win, "ThGugaoCard", -1);
        } else if (pindian->from_number < pindian->to_number) {
            if (player->getMark("@qianyu") == 0) {
                win = pindian->to;
                lose = pindian->from;
            }
        }
        if (win && lose)
            room->damage(DamageStruct(objectName(), win, lose));

        return false;
    }
};

class ThQianyu : public TriggerSkill
{
public:
    ThQianyu()
        : TriggerSkill("thqianyu")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                if (p->getHp() == 1 && p->getMark("@qianyu") <= 0)
                    skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        LogMessage log;
        log.type = "#ThQianyu";
        log.from = ask_who;
        log.arg = objectName();
        log.arg2 = QString::number(ask_who->getHp());
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(ask_who, objectName());
        room->addPlayerMark(ask_who, "@qianyu");
        if (ask_who->getMaxHp() > 1)
            room->changeMaxHpForAwakenSkill(ask_who, 1 - ask_who->getMaxHp());
        room->acquireSkill(ask_who, "thkuangmo");
        ask_who->gainAnExtraTurn();
        return false;
    }
};

class ThKuangmo : public TriggerSkill
{
public:
    ThKuangmo()
        : TriggerSkill("thkuangmo")
    {
        events << Damage;
        frequency = Compulsory;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QStringList skill;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash"))
            return skill;
        if (!TriggerSkill::triggerable(player))
            return skill;
        for (int i = 0; i < damage.damage; i++)
            skill << objectName();
        return skill;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);

        LogMessage log2;
        log2.type = "#GainMaxHp";
        log2.from = player;
        log2.arg = QString::number(1);
        room->sendLog(log2);

        room->recover(player, RecoverStruct(player));
        return false;
    }
};

class ThHuangyi : public TriggerSkill
{
public:
    ThHuangyi()
        : TriggerSkill("thhuangyi")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start && target->getMark("@qianyu") > 0
            && target->getMaxHp() > target->getGeneralMaxHp() && target->getMark("@huangyi") <= 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        LogMessage log;
        log.type = "#ThHuangyi";
        log.from = player;
        log.arg = objectName();
        log.arg2 = QString::number(player->getMaxHp());
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        room->addPlayerMark(player, "@huangyi");

        int n = player->getMaxHp() - player->getGeneralMaxHp();
        room->changeMaxHpForAwakenSkill(player, -n);
        player->drawCards(n);
        if (player->isWounded())
            room->recover(player, RecoverStruct(player));
        room->detachSkillFromPlayer(player, "thkuangmo", false, true);
        return false;
    }
};

class ThSuhu : public TriggerSkill
{
public:
    ThSuhu()
        : TriggerSkill("thsuhu")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Draw;
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
        player->addToPile("mask", room->drawCard());

        if (player->isAlive()) {
            room->getRoomState()->setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
            QStringList names;
            foreach (const Card *card, Sanguosha->findChildren<const Card *>()) {
                QString class_name = card->getClassName();
                if (class_name.endsWith("Slash"))
                    class_name = "Slash";
                if (!names.contains(class_name) && card->isAvailable(player))
                    names << class_name;
            }
            if (!names.isEmpty()) {
                QString pattern = QString("%1|.|.|hand").arg(names.join(","));
                room->askForUseCard(player, pattern, "@thsuhu");
            }
        }

        return true;
    }
};

class ThFenlang : public TriggerSkill
{
public:
    ThFenlang()
        : TriggerSkill("thfenlang")
    {
        events << HpChanged;
        frequency = Compulsory;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QStringList skills;
        if (!TriggerSkill::triggerable(player) || !data.canConvert<DamageStruct>())
            return skills;
        DamageStruct damage = data.value<DamageStruct>();
        for (int i = 0; i < damage.damage; i++)
            skills << objectName();
        return skills;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->recover(player, RecoverStruct(player));
        player->drawCards(2);

        return false;
    }
};

ThLeshiCard::ThLeshiCard()
{
    target_fixed = true;
}

void ThLeshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QString choice = room->askForChoice(source, "thleshi", "BasicCard+EquipCard+TrickCard");
    LogMessage log;
    log.type = "#ThLeshi";
    log.from = source;
    log.arg = choice;
    room->sendLog(log);
    if (source->hasUsed(choice))
        room->addPlayerHistory(source, choice, 0);
    if (choice == "BasicCard") {
        room->addPlayerHistory(source, "Slash", 0);
        room->addPlayerHistory(source, "FireSlash", 0);
        room->addPlayerHistory(source, "ThunderSlash", 0);
        room->addPlayerHistory(source, "Analeptic", 0);
    }
    source->drawCards(4, "thleshi");
}

class ThLeshi : public ZeroCardViewAsSkill
{
public:
    ThLeshi()
        : ZeroCardViewAsSkill("thleshi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThLeshiCard");
    }

    virtual const Card *viewAs() const
    {
        return new ThLeshiCard;
    }
};

class ThYouli : public TriggerSkill
{
public:
    ThYouli()
        : TriggerSkill("thyouli")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to != player)
            return QStringList();
        if (move.to_place == Player::PlaceHand && player->getHandcardNum() > 4)
            return QStringList(objectName());
        else if (move.to_place == Player::PlaceSpecial && player->getPile("mask").length() >= 2)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place == Player::PlaceHand) {
            int n = player->getHandcardNum() - 4;
            const Card *dummy = room->askForExchange(player, objectName(), n, n, false, "@thyouli-hand");
            if (!dummy || dummy->subcardsLength() != n) {
                QList<int> card_ids = player->handCards();
                qShuffle(card_ids);
                card_ids = card_ids.mid(0, n);
                dummy = new DummyCard(card_ids);
            }
            player->addToPile("mask", dummy);
            delete dummy;
        } else if (move.to_place == Player::PlaceSpecial) {
            do {
                QList<int> card_ids = player->getPile("mask");
                QList<int> to_discard;
                if (card_ids.size() == 2) {
                    to_discard = card_ids;
                } else {
                    room->fillAG(card_ids);
                    int id1 = room->askForAG(player, card_ids, false, objectName());
                    card_ids.removeOne(id1);
                    to_discard << id1;
                    room->takeAG(player, id1, false);
                    int id2 = room->askForAG(player, card_ids, false, objectName());
                    to_discard << id2;
                    room->clearAG();
                }
                DummyCard *dummy = new DummyCard(to_discard);
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                room->throwCard(dummy, reason, NULL);
                delete dummy;
                room->loseHp(player);
            } while (player->isAlive() && player->getPile("mask").size() > 1);
        }
        return false;
    }
};

class ThYouliMaxCardsSkill : public MaxCardsSkill
{
public:
    ThYouliMaxCardsSkill()
        : MaxCardsSkill("#thyouli")
    {
    }

    virtual int getFixed(const Player *target) const
    {
        if (target->hasSkill("thyouli"))
            return 4;
        else
            return -1;
    }
};

class ThJingyuan : public TriggerSkill
{
public:
    ThJingyuan()
        : TriggerSkill("thjingyuan")
    {
        events << PreCardUsed << PreCardResponded;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::Play)
            return QStringList();
        const Card *card = NULL;
        if (triggerEvent == PreCardUsed)
            card = data.value<CardUseStruct>().card;
        else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
                card = response.m_card;
        }
        if (card && card->getTypeId() != Card::TypeSkill && card->getHandlingMethod() == Card::MethodUse) {
            QString str = card->getType();
            str[0] = str[0].toUpper();
            str += "Card";
            if (!player->hasUsed(str))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        const Card *card = NULL;
        if (triggerEvent == PreCardUsed)
            card = data.value<CardUseStruct>().card;
        else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
                card = response.m_card;
        }
        QString str = card->getType();
        str[0] = str[0].toUpper();
        str += "Card";
        room->addPlayerHistory(player, str);
        return false;
    }
};

class ThPanghun : public TriggerSkill
{
public:
    ThPanghun()
        : TriggerSkill("thpanghun")
    {
        events << EventPhaseEnd << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() == Player::Draw && TriggerSkill::triggerable(player) && player->canDiscard(player, "he"))
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->getMark("@stitch") > 0) {
                room->setPlayerMark(player, "@stitch", 0);
                foreach (ServerPlayer *p, room->getOtherPlayers(player))
                    room->removeFixedDistance(player, p, 1);
                room->filterCards(player, player->getCards("he"), true);
                foreach (ServerPlayer *p, room->getOtherPlayers(player))
                    room->removeFixedDistance(player, p, 1);
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        const Card *card = room->askForCard(player, "..", "@thpanghun", QVariant(), objectName());
        if (card) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addPlayerMark(player, "@stitch");
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            room->setFixedDistance(player, p, 1);
        room->filterCards(player, player->getCards("he"), false);
        QList<ServerPlayer *> players = room->getOtherPlayers(player);
        foreach (ServerPlayer *p, players)
            room->setFixedDistance(player, p, 1);
        return false;
    }
};

class ThPanghunFilter : public FilterSkill
{
public:
    ThPanghunFilter()
        : FilterSkill("#thpanghun-fil")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        Room *room = Sanguosha->currentRoom();
        ServerPlayer *owner = room->getCardOwner(to_select->getEffectiveId());
        if (owner && owner->getMark("@stitch") > 0)
            return to_select->isKindOf("Slash");
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IronChain *iron_chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        iron_chain->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(iron_chain);
        return card;
    }
};

ThJingwuCard::ThJingwuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool ThJingwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->canDiscard(to_select, "ej");
}

void ThJingwuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->showCard(source, getEffectiveId());
    room->setPlayerFlag(source, "thjingwuInvoke");
    int card_id = room->askForCardChosen(source, target, "ej", "thjingwu", false, Card::MethodDiscard);
    room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : target, source);
}

class ThJingwu : public OneCardViewAsSkill
{
public:
    ThJingwu()
        : OneCardViewAsSkill("thjingwu")
    {
        filter_pattern = "TrickCard|black";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("ThJingwuCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThJingwuCard *card = new ThJingwuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThLunyu : public TriggerSkill
{
public:
    ThLunyu()
        : TriggerSkill("thlunyu")
    {
        events << TargetSpecified << BeforeCardsMove << EventPhaseEnd;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *to, use.to)
                room->setCardFlag(use.card, "DisableLunyuOf_" + to->objectName());
            return skill_list;
        }
        if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)) {
            if (player->getMark("@stitch") == 0 || !player->hasFlag("thjingwuInvoke") || player->hasFlag("thlunyuInvoke"))
                return skill_list;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.card_ids.isEmpty() || move.to_place != Player::DiscardPile)
                return skill_list;
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_USE
                || !move.reason.m_extraData.value<const Card *>()->hasFlag("DisableLunyuOf_" + player->objectName()))
                skill_list.insert(player, QStringList(objectName()));
        }
        if (triggerEvent == EventPhaseEnd) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag("thlunyuDraw"))
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            foreach (int id, move.card_ids) {
                const Card *card = Sanguosha->getEngineCard(id);
                if (player->askForSkillInvoke(objectName(),
                                              QString("prompt:%1::%2:%3\\%4")
                                                  .arg(id)
                                                  .arg(card->objectName())
                                                  .arg(card->getSuitString() + "_char")
                                                  .arg(card->getNumberString()))) {
                    player->tag["ThLunyuCard"] = QVariant::fromValue(card);
                    room->broadcastSkillInvoke(objectName());
                    return true;
                }
            }
            return false;
        } else
            return true;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                        ServerPlayer *ask_who) const
    {
        if (triggerEvent == BeforeCardsMove) {
            const Card *card = player->tag["ThLunyuCard"].value<const Card *>();
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), QString(), "thlunyu", QString());
            LogMessage log;
            log.type = "#ThLunyuPut";
            log.from = player;
            log.card_str = card->toString();
            room->sendLog(log);
            CardsMoveStruct move(card->getId(), NULL, Player::DrawPile, reason);
            room->moveCardsAtomic(move, true);
            room->setPlayerFlag(player, "thlunyuInvoke");
            room->setPlayerFlag(player, "thlunyuDraw");

            CardsMoveOneTimeStruct data_move = data.value<CardsMoveOneTimeStruct>();
            data_move.removeCardIds(QList<int>() << card->getId());
            data = QVariant::fromValue(data_move);
        } else {
            room->setPlayerFlag(ask_who, "-thlunyuDraw");
            QList<int> card_ids;
            room->getThread()->trigger(FetchDrawPileCard, room, NULL);
            QList<int> &draw = room->getDrawPile();
            if (draw.isEmpty())
                room->swapPile();
            card_ids << draw.takeLast();
            CardsMoveStruct move;
            move.card_ids = card_ids;
            move.from = NULL;
            move.to = ask_who;
            move.to_place = Player::PlaceHand;
            move.reason = CardMoveReason(CardMoveReason::S_REASON_DRAW, ask_who->objectName(), objectName(), QString());
            room->moveCardsAtomic(move, false);
        }
        return false;
    }
};

class ThFanhun : public TriggerSkill
{
public:
    ThFanhun()
        : TriggerSkill("thfanhun")
    {
        events << AskForPeaches;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (!TriggerSkill::triggerable(player) || dying_data.who != player)
            return QStringList();
        if (player->isDead() || player->getHp() > 0)
            return QStringList();
        return QStringList(objectName());
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
        player->gainMark("@bloom");

        if (player->isDead())
            return false;

        if (player->getHp() < 1)
            room->recover(player, RecoverStruct(player, NULL, 1 - player->getHp()));
        if (player->isChained())
            room->setPlayerProperty(player, "chained", false);
        if (!player->faceUp())
            player->turnOver();
        return false;
    }
};

class ThYoushang : public TriggerSkill
{
public:
    ThYoushang()
        : TriggerSkill("thyoushang")
    {
        events << DamageCaused;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->getSuit() != Card::Spade && !damage.chain && !damage.transfer)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->tag["ThYoushangData"] = QVariant::fromValue(data);
        bool invoke = player->askForSkillInvoke(objectName(), data);
        player->tag.remove("ThYoushangData");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->loseMaxHp(damage.to);
        player->gainMark("@bloom");
        return true;
    }
};

ThYouyaCard::ThYouyaCard()
{
}

bool ThYouyaCard::targetFilter(const QList<const Player *> &selected, const Player *, const Player *) const
{
    return selected.length() < Self->getMark("@bloom");
}

void ThYouyaCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (!room->askForCard(effect.to, "jink", "@thyouya-jink:" + effect.from->objectName(), QVariant(), Card::MethodResponse,
                          effect.from)) {
        if (!effect.to->isNude()) {
            int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thyouya");
            room->obtainCard(effect.from, card_id, false);
        }
    }
}

class ThYouyaViewAsSkill : public OneCardViewAsSkill
{
public:
    ThYouyaViewAsSkill()
        : OneCardViewAsSkill("thyouya")
    {
        response_pattern = "@@thyouya";
        filter_pattern = ".!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThYouyaCard *card = new ThYouyaCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThYouya : public TriggerSkill
{
public:
    ThYouya()
        : TriggerSkill("thyouya")
    {
        events << DamageInflicted;
        view_as_skill = new ThYouyaViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@bloom") > 0 && target->canDiscard(target, "he");
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(player, "@@thyouya", "@thyouya");
        return false;
    }
};

class ThManxiao : public TriggerSkill
{
public:
    ThManxiao()
        : TriggerSkill("thmanxiao")
    {
        events << EventMarksGot;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@bloom") >= 4;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        room->killPlayer(player);
        return false;
    }
};

class ThManxiaoMaxCards : public MaxCardsSkill
{
public:
    ThManxiaoMaxCards()
        : MaxCardsSkill("#thmanxiao")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill("thmanxiao"))
            return target->getMark("@bloom");
        else
            return 0;
    }
};

ThJinluCard::ThJinluCard()
{
}

void ThJinluCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    effect.to->setFlags("ThJinluTarget");
    DummyCard *dummy_card = new DummyCard(effect.to->handCards());
    dummy_card->addSubcards(effect.to->getEquips());
    if (dummy_card->subcardsLength() > 0) {
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, effect.from->objectName(), effect.to->objectName(), "thjinlu",
                              QString());
        room->moveCardTo(dummy_card, effect.to, effect.from, Player::PlaceHand, reason, false);
    }
    delete dummy_card;
}

class ThJinlu : public OneCardViewAsSkill
{
public:
    ThJinlu()
        : OneCardViewAsSkill("thjinlu")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "h") && !player->hasUsed("ThJinluCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new ThJinluCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThJinluTriggerSkill : public TriggerSkill
{
public:
    ThJinluTriggerSkill()
        : TriggerSkill("#thjinlu")
    {
        events << EventPhaseEnd << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play && player->hasUsed("ThJinluCard")) {
            player->setFlags("ThJinluUsed");
            foreach (ServerPlayer *other, room->getOtherPlayers(player))
                if (other->hasFlag("ThJinluTarget")) {
                    return QStringList(objectName());
                }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish && player->hasFlag("ThJinluUsed"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "thjinlu");
        if (player->getPhase() == Player::Play) {
            ServerPlayer *target = NULL;
            foreach (ServerPlayer *other, room->getOtherPlayers(player))
                if (other->hasFlag("ThJinluTarget")) {
                    other->setFlags("-ThJinluTarget");
                    target = other;
                    break;
                }

            if (!target || target->getHp() < 1 || player->isNude())
                return false;

            DummyCard *to_goback;
            if (player->getCardCount() <= target->getHp()) {
                to_goback = player->isKongcheng() ? new DummyCard : player->wholeHandCards();
                to_goback->addSubcards(player->getEquips());
            } else
                to_goback = (DummyCard *)room->askForExchange(player, objectName(), target->getHp(), target->getHp(), true,
                                                              "ThJinluGoBack");

            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), target->objectName(), objectName(),
                                  QString());
            room->moveCardTo(to_goback, player, target, Player::PlaceHand, reason);
            delete to_goback;
        } else if (player->getPhase() == Player::Finish)
            player->turnOver();

        return false;
    }
};

class ThKuangli : public TriggerSkill
{
public:
    ThKuangli()
        : TriggerSkill("thkuangli")
    {
        events << TurnedOver;
        frequency = Frequent;
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

class ThYuxin : public TriggerSkill
{
public:
    ThYuxin()
        : TriggerSkill("thyuxin")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw;
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
        int card1 = room->drawCard();
        int card2 = room->drawCard();
        QList<int> ids;
        ids << card1 << card2;
        bool red = Sanguosha->getCard(card1)->isRed() && Sanguosha->getCard(card2)->isRed();

        CardsMoveStruct move;
        move.card_ids = ids;
        move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        move.to_place = Player::PlaceTable;
        room->moveCardsAtomic(move, true);
        room->getThread()->delay();

        DummyCard *dummy = new DummyCard(move.card_ids);
        room->obtainCard(player, dummy);
        delete dummy;

        if (red) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->isWounded())
                    targets << p;

            if (!targets.isEmpty()) {
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "thyuxin-recover", true);
                if (target)
                    room->recover(target, RecoverStruct(player));
            }
        }

        return true;
    }
};

ThChuangxinCard::ThChuangxinCard()
{
    target_fixed = true;
}

void ThChuangxinCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->setFlags("thchuangxinUsed");
    if (subcardsLength() == 1) {
        QString choice = room->askForChoice(source, "thchuangxin", "iklingshi+ikbiyue");
        room->acquireSkill(source, choice);
    } else
        room->handleAcquireDetachSkills(source, "iklingshi|ikbiyue");
}

class ThChuangxinViewAsSkill : public ViewAsSkill
{
public:
    ThChuangxinViewAsSkill()
        : ViewAsSkill("thchuangxin")
    {
        response_pattern = "@@thchuangxin";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return !Self->isJilei(to_select) && selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return NULL;
        ThChuangxinCard *card = new ThChuangxinCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThChuangxin : public TriggerSkill
{
public:
    ThChuangxin()
        : TriggerSkill("thchuangxin")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new ThChuangxinViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play
            && player->canDiscard(player, "he"))
            return QStringList(objectName());
        else if (triggerEvent == EventPhaseChanging && player->hasFlag("thchuangxinUsed")) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                player->setFlags("-thchuangxinUsed");
                room->handleAcquireDetachSkills(player, "-iklingshi|-ikbiyue", true);
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(player, "@@thchuangxin", "@thchuangxin", -1, Card::MethodDiscard);
        return false;
    }
};

ThTianxinCard::ThTianxinCard()
{
    target_fixed = true;
}

void ThTianxinCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->setFlags("thtianxinUsed");
    if (subcardsLength() == 1) {
        QString choice = room->askForChoice(source, "thtianxin", "ikxushi+iktiandu");
        room->acquireSkill(source, choice);
    } else
        room->handleAcquireDetachSkills(source, "ikxushi|iktiandu");
};

class ThTianxinViewAsSkill : public ViewAsSkill
{
public:
    ThTianxinViewAsSkill()
        : ViewAsSkill("thtianxin")
    {
        response_pattern = "@@thtianxin";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return !Self->isJilei(to_select) && selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return NULL;
        ThTianxinCard *card = new ThTianxinCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThTianxin : public TriggerSkill
{
public:
    ThTianxin()
        : TriggerSkill("thtianxin")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new ThTianxinViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::RoundStart
            && player->canDiscard(player, "he"))
            return QStringList(objectName());
        else if (triggerEvent == EventPhaseChanging && player->hasFlag("thtianxinUsed")) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                player->setFlags("-thtianxinUsed");
                room->handleAcquireDetachSkills(player, "-ikxushi|-iktiandu", true);
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(player, "@@thtianxin", "@thtianxin", -1, Card::MethodDiscard);
        return false;
    }
};

class ThRangdeng : public TriggerSkill
{
public:
    ThRangdeng()
        : TriggerSkill("thrangdeng")
    {
        events << EventPhaseStart << CardsMoveOneTime << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play
            && !player->isKongcheng())
            return QStringList(objectName());
        else if (triggerEvent == EventLoseSkill && data.toString() == objectName()) {
            QStringList detach;
            detach << "-ikbiyue"
                   << "-thfeiying"
                   << "-ikchenhong"
                   << "-thjifeng";
            room->handleAcquireDetachSkills(player, detach, true);
        } else if (triggerEvent == EventAcquireSkill && data.toString() == objectName()) {
            if (!player->getPile("lantern").isEmpty()) {
                room->notifySkillInvoked(player, objectName());
                QStringList attach;
                foreach (int id, player->getPile("lantern")) {
                    QString skill_name;
                    switch (Sanguosha->getCard(id)->getSuit()) {
                    case Card::Heart: {
                        skill_name = "ikbiyue";
                        break;
                    }
                    case Card::Spade: {
                        skill_name = "thfeiying";
                        break;
                    }
                    case Card::Diamond: {
                        skill_name = "ikchenhong";
                        break;
                    }
                    case Card::Club: {
                        skill_name = "thjifeng";
                        break;
                    }
                    default: {
                        break;
                    }
                    }
                    if (!skill_name.isEmpty() && !attach.contains(skill_name))
                        attach << skill_name;
                }
                room->handleAcquireDetachSkills(player, attach);
            }
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill(objectName(), true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && move.to_pile_name == "lantern") {
                QStringList skills;
                foreach (int id, move.card_ids) {
                    QString skill_name = "";
                    switch (Sanguosha->getCard(id)->getSuit()) {
                    case Card::Heart: {
                        skill_name = "ikbiyue";
                        break;
                    }
                    case Card::Spade: {
                        skill_name = "thfeiying";
                        break;
                    }
                    case Card::Diamond: {
                        skill_name = "ikchenhong";
                        break;
                    }
                    case Card::Club: {
                        skill_name = "thjifeng";
                        break;
                    }
                    default: {
                        break;
                    }
                    }
                    if (!skill_name.isEmpty() && !player->hasSkill(skill_name))
                        skills << skill_name;
                }
                room->handleAcquireDetachSkills(player, skills);
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        for (int i = 0; i < 3; i++) {
            if (player->isKongcheng())
                break;
            QStringList numbers;
            for (int num = 1; num <= 13; num++)
                numbers << QString::number(num);
            foreach (int id, player->getPile("lantern")) {
                QString num = QString::number(Sanguosha->getCard(id)->getNumber());
                if (numbers.contains(num))
                    numbers.removeAll(num);
            }
            const Card *card = NULL;
            if (!numbers.isEmpty()) {
                QString pattern = ".|.|" + numbers.join(",") + "|hand";
                card = room->askForCard(player, pattern, "@thrangdeng:::" + QString::number(i), data, Card::MethodNone);
            }
            if (!card) {
                if (i == 0)
                    room->askForDiscard(player, objectName(), 1, 1);
                break;
            } else
                player->addToPile("lantern", card);
        }

        return false;
    }
};

ThBaihunCard::ThBaihunCard()
{
}

void ThBaihunCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    effect.from->clearOnePrivatePile("lantern");
    room->handleAcquireDetachSkills(effect.from, "-ikbiyue|-thfeiying|-ikchenhong|-thjifeng", true);
    room->killPlayer(effect.to);
}

class ThBaihun : public ZeroCardViewAsSkill
{
public:
    ThBaihun()
        : ZeroCardViewAsSkill("thbaihun")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getPile("lantern").length() >= 13;
    }

    virtual const Card *viewAs() const
    {
        return new ThBaihunCard;
    }
};

class ThXujing : public TriggerSkill
{
public:
    ThXujing()
        : TriggerSkill("thxujing")
    {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (room->getCurrent() == player && player->getPhase() != Player::NotActive)
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.to.contains(player))
            return QStringList();
        if (use.card->isNDTrick() && use.card->isBlack())
            return QStringList(objectName());
        if ((use.card->isKindOf("Peach") || use.card->isKindOf("Analeptic")) && use.from == player)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->tag["ThXujingTarget"] = QVariant::fromValue(data);
        bool invoke = player->askForSkillInvoke(objectName(), data);
        player->tag.remove("ThXujingTarget");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QStringList choices;
        foreach (const Skill *skill, player->getVisibleSkillList()) {
            if (skill->isAttachedLordSkill())
                continue;
            choices << skill->objectName();
        }
        if (choices.isEmpty())
            return false;
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        room->detachSkillFromPlayer(player, choice);
        player->drawCards(1, objectName());
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from && use.from->isAlive()) {
            QStringList skills;
            if (use.card->isNDTrick()) {
                if (!use.from->hasSkill("thhuanzang"))
                    skills << "thhuanzang";
                if (!use.from->hasSkill("thanyue"))
                    skills << "thanyue";
                room->setPlayerMark(use.from, "@xujing_bad", 1);
            } else {
                if (!use.from->hasSkill("thjifeng"))
                    skills << "thjifeng";
                if (!use.from->hasSkill("thxijing"))
                    skills << "thxijing";
                room->setPlayerMark(use.from, "@xujing_good", 1);
            }
            if (!skills.isEmpty()) {
                room->handleAcquireDetachSkills(use.from, skills);
                QStringList list
                    = use.from->property(QString("thxujing_skills_%1").arg(player->objectName()).toLatin1().data())
                          .toStringList();
                list += skills;
                room->setPlayerProperty(use.from, QString("thxujing_skills_%1").arg(player->objectName()).toLatin1().data(),
                                        QVariant::fromValue(list));
            }
            player->tag["ThXujing"] = true;
        }
        return false;
    }
};

class ThXujingClear : public TriggerSkill
{
public:
    ThXujingClear()
        : TriggerSkill("#thxujing-clear")
    {
        events << EventPhaseChanging << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!player || !player->tag["ThXujing"].toBool())
            return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        }
        if (triggerEvent == Death && data.value<DeathStruct>().who != player)
            return QStringList();
        player->tag["ThXujing"] = false;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (!p->property(QString("thxujing_skills_%1").arg(player->objectName()).toLatin1().data())
                     .toStringList()
                     .isEmpty()) {
                QSet<QString> skills = p->property(QString("thxujing_skills_%1").arg(player->objectName()).toLatin1().data())
                                           .toStringList()
                                           .toSet();
                room->setPlayerProperty(p, QString("thxujing_skills_%1").arg(player->objectName()).toLatin1().data(),
                                        QVariant());
                QStringList new_list;
                foreach (QString name, skills)
                    new_list << "-" + name;
                room->handleAcquireDetachSkills(p, new_list, true);
            }
            room->setPlayerMark(p, "@xujing_bad", 0);
            room->setPlayerMark(p, "@xujing_good", 0);
        }
        return QStringList();
    }
};

ThLingyunDialog *ThLingyunDialog::getInstance()
{
    static ThLingyunDialog *instance;
    if (instance == NULL)
        instance = new ThLingyunDialog();

    return instance;
}

ThLingyunDialog::ThLingyunDialog()
{
    setObjectName("thlingyun");
    setWindowTitle(Sanguosha->translate("thlingyun"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, (void (QButtonGroup::*)(QAbstractButton *))(&QButtonGroup::buttonClicked), this,
            &ThLingyunDialog::selectCard);
}

void ThLingyunDialog::popup()
{
    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }
    QHash<QString, QString> skill_map;
    skill_map.insert("ex_nihilo", "ikbenghuai");
    skill_map.insert("fire_attack", "thsanling");
    skill_map.insert("dismantlement", "ikxinshang");
    skill_map.insert("iron_chain", "ikjinlian");

    foreach (QString card, skill_map.keys()) {
        QCommandLinkButton *button = new QCommandLinkButton;
        button->setText(Sanguosha->translate(card));
        button->setObjectName(card);
        group->addButton(button);

        bool can = true;
        if (Self->hasSkill(skill_map[card]))
            can = false;
        if (can) {
            const Card *c = Sanguosha->cloneCard(card);
            if (Self->isCardLimited(c, Card::MethodUse) || !c->isAvailable(Self))
                can = false;
            delete c;
        }
        button->setEnabled(can);
        button_layout->addWidget(button);

        if (!map.contains(card)) {
            Card *c = Sanguosha->cloneCard(card, Card::NoSuit, 0);
            c->setParent(this);
            map.insert(card, c);
        }
    }
    Self->tag.remove("thlingyun");
    exec();
}

void ThLingyunDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag["thlingyun"] = QVariant::fromValue(card);
    emit onButtonClick();
    accept();
}

ThLingyunCard::ThLingyunCard()
{
    handling_method = Card::MethodNone;
}

bool ThLingyunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!Self->tag.value("thlingyun").value<const Card *>())
        return false;
    QString card_name = Self->tag.value("thlingyun").value<const Card *>()->objectName();
    Card *card = Sanguosha->cloneCard(card_name, NoSuit, 0);
    card->setSkillName("thlingyun");
    card->setCanRecast(false);
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool ThLingyunCard::targetFixed() const
{
    if (!Self->tag.value("thlingyun").value<const Card *>())
        return false;
    QString card_name = Self->tag.value("thlingyun").value<const Card *>()->objectName();
    Card *card = Sanguosha->cloneCard(card_name, NoSuit, 0);
    card->setSkillName("thlingyun");
    card->setCanRecast(false);
    return card && card->targetFixed();
}

bool ThLingyunCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (!Self->tag.value("thlingyun").value<const Card *>())
        return false;
    QString card_name = Self->tag.value("thlingyun").value<const Card *>()->objectName();
    Card *card = Sanguosha->cloneCard(card_name, NoSuit, 0);
    card->setSkillName("thlingyun");
    card->setCanRecast(false);
    return card && card->targetsFeasible(targets, Self);
}

const Card *ThLingyunCard::validate(CardUseStruct &card_use) const
{
    Card *use_card = Sanguosha->cloneCard(user_string, NoSuit, 0);
    use_card->setSkillName("_thlingyun");
    bool available = true;
    foreach (ServerPlayer *to, card_use.to)
        if (card_use.from->isProhibited(to, use_card)) {
            available = false;
            break;
        }
    available = available && use_card->isAvailable(card_use.from);
    use_card->deleteLater();
    if (!available)
        return NULL;

    Room *room = card_use.from->getRoom();

    LogMessage log;
    log.from = card_use.from;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString();
    room->sendLog(log);

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), "thlingyun", QString());
    room->moveCardTo(this, card_use.from, NULL, Player::DiscardPile, reason, true);

    QMap<QString, QString> map;
    map.insert("ex_nihilo", "ikbenghuai");
    map.insert("fire_attack", "thsanling");
    map.insert("dismantlement", "ikxinshang");
    map.insert("iron_chain", "ikjinlian");
    if (!card_use.from->hasSkill(map[user_string]))
        room->acquireSkill(card_use.from, map[user_string]);
    return use_card;
}

class ThLingyun : public OneCardViewAsSkill
{
public:
    ThLingyun()
        : OneCardViewAsSkill("thlingyun")
    {
        filter_pattern = ".!";
    }

    virtual SkillDialog *getDialog() const
    {
        return ThLingyunDialog::getInstance();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        const Card *c = Self->tag.value("thlingyun").value<const Card *>();
        if (c) {
            ThLingyunCard *card = new ThLingyunCard;
            card->setUserString(c->objectName());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        QMap<QString, QString> map;
        map.insert("ex_nihilo", "ikbenghuai");
        map.insert("fire_attack", "thsanling");
        map.insert("dismantlement", "ikxinshang");
        map.insert("iron_chain", "ikjinlian");
        bool can = false;
        foreach (QString card_name, map.keys()) {
            Card *card = Sanguosha->cloneCard(card_name, Card::NoSuit, 0);
            card->setSkillName(objectName());
            if (!Self->isCardLimited(card, Card::MethodUse) && card->isAvailable(Self) && !Self->hasSkill(map[card_name]))
                can = true;
            delete card;
            if (can)
                break;
        }
        return can;
    }
};

class ThZhaoai : public TriggerSkill
{
public:
    ThZhaoai()
        : TriggerSkill("thzhaoai")
    {
        events << Death;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player && !player->isAlive() && player->hasSkill(objectName())) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player && death.damage && death.damage->from && death.damage->from != player)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
        QStringList skill;
        if (!target->hasSkill("thshenbao"))
            skill << "thshenbao";
        if (!target->hasSkill("thyuhuo"))
            skill << "thyuhuo";
        room->handleAcquireDetachSkills(target, skill);

        int n = 0;
        foreach (const Skill *skill, player->getVisibleSkillList()) {
            if (skill->isAttachedLordSkill())
                continue;
            n++;
        }
        if (n > 0)
            target->drawCards(n, objectName());
        return false;
    }
};

class ThWunan : public TriggerSkill
{
public:
    ThWunan()
        : TriggerSkill("thwunan")
    {
        events << CardUsed << HpRecover << Damaged << CardResponded << DamageCaused;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (player->isDead())
            return skill_list;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("GodSalvation") || use.card->isKindOf("AmazingGrace"))
                foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                    if (owner != player)
                        skill_list.insert(owner, QStringList(objectName()));
        } else if (triggerEvent == HpRecover && player->hasFlag("Global_Dying") && player->getHp() >= 1) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                if (owner != player)
                    skill_list.insert(owner, QStringList(objectName()));
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature == DamageStruct::Fire)
                foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                    if (owner != player)
                        skill_list.insert(owner, QStringList(objectName()));
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Jink") && resp.m_card->getSkillName() == "eight_diagram")
                foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                    if (owner != player)
                        skill_list.insert(owner, QStringList(objectName()));
        } else if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.to->isKongcheng())
                foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                    if (owner != player)
                        skill_list.insert(owner, QStringList(objectName()));
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
        choices << "draw";
        if (ask_who->canDiscard(player, "he"))
            choices << "throw";
        QString choice = room->askForChoice(ask_who, objectName(), choices.join("+"), QVariant::fromValue(player));
        if (choice == "throw") {
            int card_id = room->askForCardChosen(ask_who, player, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, player, ask_who);
        } else
            ask_who->drawCards(1, objectName());

        bool can_recover = true;
        foreach (ServerPlayer *p, room->getOtherPlayers(ask_who)) {
            if (p->getHp() < ask_who->getHp()) {
                can_recover = false;
                break;
            }
        }
        if (ask_who->isWounded() && can_recover && ask_who->askForSkillInvoke("thwunan_recover", "yes"))
            room->recover(ask_who, RecoverStruct(ask_who));

        return false;
    }
};

class ThSanling : public TriggerSkill
{
public:
    ThSanling()
        : TriggerSkill("thsanling")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner->isKongcheng())
                    skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->killPlayer(player, NULL);
        return false;
    }
};

ThBingzhangCard::ThBingzhangCard()
{
    target_fixed = true;
}

class ThBingzhangViewAsSkill : public ViewAsSkill
{
public:
    ThBingzhangViewAsSkill()
        : ViewAsSkill("thbingzhang")
    {
        response_pattern = "@@thbingzhang";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;
        ThBingzhangCard *card = new ThBingzhangCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThBingzhang : public TriggerSkill
{
public:
    ThBingzhang()
        : TriggerSkill("thbingzhang")
    {
        events << DamageForseen << PreHpLost;
        view_as_skill = new ThBingzhangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getCardCount() > 1;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thbingzhang", "@thbingzhang", -1, Card::MethodDiscard);
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return true;
    }
};

class ThJiwuFilterSkill : public FilterSkill
{
public:
    ThJiwuFilterSkill()
        : FilterSkill("thjiwu")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->isKindOf("Peach") || to_select->isKindOf("Analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(jink);
        return card;
    }
};

class ThJiwu : public TriggerSkill
{
public:
    ThJiwu()
        : TriggerSkill("thjiwu")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        view_as_skill = new ThJiwuFilterSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QStringList skills;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!TriggerSkill::triggerable(player) || move.from != player)
            return skills;
        ServerPlayer *current = room->getCurrent();
        if (current == player && player->getPhase() != Player::NotActive)
            return skills;

        for (int i = 0; i < move.card_ids.length(); i++)
            if ((move.from_places[i] == Player::PlaceHand && move.to != player && move.to_place == Player::PlaceHand)
                || (move.to_place == Player::DiscardPile && move.from_places[i] != Player::PlaceJudge
                    && move.from_places[i] != Player::PlaceSpecial
                    && (Sanguosha->getCard(move.card_ids[i])->isKindOf("Jink")
                        || Sanguosha->getCard(move.card_ids[i])->isKindOf("Peach")
                        || Sanguosha->getCard(move.card_ids[i])->isKindOf("Analeptic"))))
                skills << objectName();

        return skills;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        player->drawCards(1);
        return false;
    }
};

class ThSisui : public TriggerSkill
{
public:
    ThSisui()
        : TriggerSkill("thsisui")
    {
        events << EventPhaseStart;
        owner_only_skill = true;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Start)
            foreach (ServerPlayer *p, player->getRoom()->getAllPlayers())
                if (p != player && !p->isKongcheng())
                    return true;
        return false;
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
        QList<int> card_ids;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (!p->isKongcheng()) {
                const Card *card = room->askForCard(p, ".!", "@thsisui");
                if (!card) {
                    card = p->getRandomHandCard();
                    room->throwCard(card, p);
                }
                card_ids << card->getEffectiveId();
            }

        foreach (int card_id, card_ids)
            if (room->getCardPlace(card_id) != Player::DiscardPile)
                card_ids.removeOne(card_id);

        if (card_ids.isEmpty())
            return false;

        DummyCard *dummy = new DummyCard;
        room->fillAG(card_ids, NULL);
        for (int i = 0; i < 2; i++) {
            int card_id = room->askForAG(player, card_ids, true, "thsisui");
            if (card_id == -1)
                break;
            room->takeAG(player, card_id, false);
            card_ids.removeOne(card_id);
            dummy->addSubcard(card_id);
            if (card_ids.isEmpty())
                break;
        }
        room->clearAG();
        room->obtainCard(player, dummy);
        if (dummy->subcardsLength() > 0) {
            QList<int> card_ids = dummy->getSubcards();
            while (room->askForYiji(player, card_ids, objectName(), false, true, true, -1))
                if (player->isDead())
                    break;
        }

        return false;
    }
};

class ThZhanying : public TriggerSkill
{
public:
    ThZhanying()
        : TriggerSkill("thzhanying")
    {
        events << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (TriggerSkill::triggerable(player) && change.to == Player::Draw)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        player->skip(Player::Draw);
        return true;
    }
};

class ThZhanyingMaxCardsSkill : public MaxCardsSkill
{
public:
    ThZhanyingMaxCardsSkill()
        : MaxCardsSkill("#thzhanying")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill("thzhanying"))
            return 4;
        else
            return 0;
    }
};

class ThLuli : public TriggerSkill
{
public:
    ThLuli()
        : TriggerSkill("thluli")
    {
        events << DamageCaused << DamageInflicted;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused) {
            if (damage.to && damage.to->isAlive() && qMax(damage.to->getHp(), 0) >= player->getHp() && damage.to != player
                && player->canDiscard(player, "h"))
                return QStringList(objectName());
        } else if (triggerEvent == DamageInflicted) {
            if (damage.from && damage.from->isAlive() && qMax(damage.from->getHp(), 0) >= player->getHp()
                && damage.from != player && player->canDiscard(player, "h"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        bool invoke = false;
        if (triggerEvent == DamageCaused) {
            invoke = room->askForCard(player, ".black", "@thluli-increase:" + damage.to->objectName(), data, objectName());
        } else if (triggerEvent == DamageInflicted) {
            invoke = room->askForCard(player, ".red", "@thluli-decrease:" + damage.from->objectName(), data, objectName());
        }
        return invoke;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused) {
            LogMessage log;
            log.type = "#ThLuliIncrease";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        } else if (triggerEvent == DamageInflicted) {
            LogMessage log;
            log.type = "#ThLuliDecrease";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(--damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
            if (damage.damage < 1)
                return true;
        }

        return false;
    }
};

class ThGuihuan : public TriggerSkill
{
public:
    ThGuihuan()
        : TriggerSkill("thguihuan")
    {
        events << BeforeGameOverJudge;
        frequency = Limited;
        limit_mark = "@guihuan";
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&ask_who) const
    {
        if (!isNormalGameMode(room->getMode()))
            return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage == NULL)
            return QStringList();
        ServerPlayer *killer = death.damage->from;
        if (killer == NULL || killer->isLord() || player->isLord() || player->getHp() > 0)
            return QStringList();
        if (!TriggerSkill::triggerable(killer) || killer->getMark("@guihuan") == 0)
            return QStringList();
        ask_who = killer;
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        room->setPlayerFlag(player, "ThGuihuanTarget");
        bool invoke = ask_who->askForSkillInvoke(objectName());
        room->setPlayerFlag(player, "-ThGuihuanTarget");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        room->removePlayerMark(ask_who, "@guihuan");
        room->addPlayerMark(ask_who, "@guihuanused");
        QString role1 = ask_who->getRole();
        ask_who->setRole(player->getRole());
        room->notifyProperty(ask_who, ask_who, "role", player->getRole());
        room->setPlayerProperty(player, "role", role1);
        return false;
    }
};

class ThZhizun : public TriggerSkill
{
public:
    ThZhizun()
        : TriggerSkill("thzhizun")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player)
            && (player->getPhase() == Player::Finish || player->getPhase() == Player::Start);
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
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@thzhizun", true);
        if (target) {
            QStringList kingdomList = Sanguosha->getKingdoms();
            kingdomList.removeOne("kami");
            QString old_kingdom = target->getKingdom();
            kingdomList.removeOne(old_kingdom);
            if (kingdomList.isEmpty())
                return false;
            QString kingdom
                = room->askForChoice(player, "thzhizun_kingdom", kingdomList.join("+"), QVariant::fromValue(target));
            room->setPlayerProperty(target, "kingdom", kingdom);

            LogMessage log;
            log.type = "#ChangeKingdom";
            log.from = player;
            log.to << target;
            log.arg = old_kingdom;
            log.arg2 = kingdom;
            room->sendLog(log);
        }

        QStringList lord_skills;
        lord_skills << "ikxinqi"
                    << "ikhuanwei"
                    << "ikjiyuan"
                    << "ikyuji"
                    << "thhuazhi"
                    << "iksongwei"
                    << "thchundu"
                    << "ikwuhua";

        foreach (QString lord_skill, lord_skills) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(lord_skill)) {
                if (owner->hasLordSkill(lord_skill)) {
                    lord_skills.removeOne(lord_skill);
                    break;
                }
            }
        }

        if (!lord_skills.isEmpty()) {
            lord_skills << "cancel";
            QString skill_name = room->askForChoice(player, "thzhizun_lordskills", lord_skills.join("+"));

            if (skill_name != "cancel") {
                const Skill *skill = Sanguosha->getSkill(skill_name);
                room->acquireSkill(player, skill);
            }
        }
        return false;
    }
};

class ThFeiying : public DistanceSkill
{
public:
    ThFeiying()
        : DistanceSkill("thfeiying")
    {
    }

    virtual int getCorrect(const Player *, const Player *to) const
    {
        if (to->hasSkill(objectName()))
            return 1;
        else
            return 0;
    }
};

class ThLijian : public TriggerSkill
{
public:
    ThLijian()
        : TriggerSkill("thlijian")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@lijian";
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::Start)
            return QStringList();
        if (!player->isKongcheng() && player->getMark("@lijian") > 0)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->removePlayerMark(player, "@lijian");
            room->addPlayerMark(player, "@lijianused");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->isKongcheng())
            return false;
        const Card *card = room->askForCardShow(player, player, "@thlijian-show");
        if (card->getEffectiveId() == -1)
            card = player->getRandomHandCard();

        room->showCard(player, card->getEffectiveId());
        QString pattern = card->getType();
        pattern[0] = pattern[0].toUpper();
        pattern.prepend(".");
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            const Card *card
                = room->askForCard(p, pattern, "@thlijian-give:" + player->objectName(), QVariant(), Card::MethodNone);
            if (card) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, p->objectName(), player->objectName(), objectName(),
                                      QString());
                room->obtainCard(player, card, reason);
            } else
                room->damage(DamageStruct("thlijian", player, p));
        }

        return false;
    }
};

ThSiqiangCard::ThSiqiangCard()
{
}

void ThSiqiangCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->removePlayerMark(effect.from, "@siqiang");
    room->addPlayerMark(effect.from, "@siqiangused");
    effect.from->tag["ThSiqiangTarget"] = QVariant::fromValue(effect.to);
    room->setPlayerCardLimitation(effect.to, "use,response", ".", false);
}

class ThSiqiangViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThSiqiangViewAsSkill()
        : ZeroCardViewAsSkill("thsiqiang")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@siqiang") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new ThSiqiangCard;
    }
};

class ThSiqiang : public TriggerSkill
{
public:
    ThSiqiang()
        : TriggerSkill("thsiqiang")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new ThSiqiangViewAsSkill;
        frequency = Limited;
        limit_mark = "@siqiang";
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (player->tag["ThSiqiangTarget"].value<ServerPlayer *>() == NULL)
            return QStringList();
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
        ServerPlayer *target = player->tag["ThSiqiangTarget"].value<ServerPlayer *>();
        player->tag.remove("ThSiqiangTarget");
        if (target)
            room->removePlayerCardLimitation(target, "use,response", ".$0");
        return QStringList();
    }
};

ThJiefuCard::ThJiefuCard()
{
}

void ThJiefuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.to->hasEquip()) {
        QList<const Card *> equips = effect.to->getEquips();
        DummyCard *dummy = new DummyCard;
        dummy->addSubcards(equips);
        room->throwCard(dummy, effect.to, effect.from);
        delete dummy;
    }
    room->removePlayerMark(effect.from, "@jiefu");
    room->addPlayerMark(effect.from, "@jiefuused");
    room->setPlayerFlag(effect.to, "thjiefu_null");

    foreach (ServerPlayer *pl, room->getAllPlayers())
        room->filterCards(pl, pl->getCards("he"), true);
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

class ThJiefuViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThJiefuViewAsSkill()
        : ZeroCardViewAsSkill("thjiefu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@jiefu") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new ThJiefuCard;
    }
};

class ThJiefu : public TriggerSkill
{
public:
    ThJiefu()
        : TriggerSkill("thjiefu")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new ThJiefuViewAsSkill;
        frequency = Limited;
        limit_mark = "@jiefu";
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        }
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->hasFlag("thjiefu_null")) {
                room->setPlayerFlag(death.who, "-thjiefu_null");
                foreach (ServerPlayer *pl, room->getAllPlayers())
                    room->filterCards(pl, pl->getCards("he"), false);
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }
            if (death.who != player || player != room->getCurrent())
                return QStringList();
        }
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasFlag("thjiefu_null")) {
                room->setPlayerFlag(p, "-thjiefu_null");
                foreach (ServerPlayer *pl, room->getAllPlayers())
                    room->filterCards(pl, pl->getCards("he"), false);
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }
        }
        return QStringList();
    }
};

class ThJiefuInvalidity : public InvaliditySkill
{
public:
    ThJiefuInvalidity()
        : InvaliditySkill("#thjiefu-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *) const
    {
        return !player->hasFlag("thjiefu_null");
    }
};

class ThHuanxiang : public TriggerSkill
{
public:
    ThHuanxiang()
        : TriggerSkill("thhuanxiang")
    {
        events << EventPhaseStart << DamageForseen;
        frequency = Limited;
        limit_mark = "@huanxiang";
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart && player->getMark("@quiet") > 0)
            room->removePlayerMark(player, "@quiet");
        else if (triggerEvent == DamageForseen) {
            if (TriggerSkill::triggerable(player) && player->getMark("@huanxiang") > 0)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->removePlayerMark(player, "@huanxiang");
            room->addPlayerMark(player, "@huanxiangused");
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->isWounded() && player->getHp() < 3)
            room->recover(player, RecoverStruct(player, NULL, 3 - player->getHp()));
        room->addPlayerMark(player, "@quiet");
        return true;
    }
};

class ThHuanxiangPrevent : public TriggerSkill
{
public:
    ThHuanxiangPrevent()
        : TriggerSkill("#thhuanxiang-prevent")
    {
        events << DamageForseen;
        frequency = NotCompulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->getMark("@quiet") > 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke("thhuanxiang");
        room->sendCompulsoryTriggerLog(player, "thhuanxiang");
        return true;
    }
};

class ThHuanxiangProhibit : public ProhibitSkill
{
public:
    ThHuanxiangProhibit()
        : ProhibitSkill("#thhuanxiang-prohibit")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->getMark("@quiet") > 0 && card->getTypeId() != Card::TypeSkill;
    }
};

TouhouKamiPackage::TouhouKamiPackage()
    : Package("touhou-kami")
{
    General *kami001 = new General(this, "kami001", "kami", 3);
    kami001->addSkill(new ThKexing);
    kami001->addSkill(new ThShenfeng);
    kami001->addSkill(new ThKaihai);

    General *kami002 = new General(this, "kami002", "kami", 4);
    kami002->addSkill(new ThTianbao);
    kami002->addSkill("thyanmeng");

    General *kami003 = new General(this, "kami003", "kami", 8);
    kami003->addSkill(new ThWudao);
    kami003->addSkill(new ThHuanjun);
    kami003->addSkill("thyanmeng");

    General *kami004 = new General(this, "kami004", "kami");
    kami004->addSkill(new ThGugao);
    kami004->addSkill(new ThQianyu);
    kami004->addRelateSkill("thkuangmo");
    kami004->addSkill(new ThHuangyi);

    General *kami005 = new General(this, "kami005", "kami", 3, false);
    kami005->addSkill(new ThSuhu);
    kami005->addSkill(new ThFenlang);
    kami005->addSkill(new ThLeshi);
    kami005->addSkill(new ThYouli);
    kami005->addSkill(new ThYouliMaxCardsSkill);
    related_skills.insertMulti("thyouli", "#thyouli");
    kami005->addSkill(new ThJingyuan);

    General *kami006 = new General(this, "kami006", "kami", 3);
    kami006->addSkill(new ThPanghun);
    kami006->addSkill(new ThPanghunFilter);
    kami006->addSkill(new ThJingwu);
    kami006->addSkill(new ThLunyu);

    General *kami007 = new General(this, "kami007", "kami", 2, false);
    kami007->addSkill(new ThFanhun);
    kami007->addSkill(new ThYoushang);
    kami007->addSkill(new ThYouya);
    kami007->addSkill(new ThManxiao);
    kami007->addSkill(new ThManxiaoMaxCards);
    related_skills.insertMulti("thmanxiao", "#thmanxiao");

    General *kami008 = new General(this, "kami008", "kami", 3, false);
    kami008->addSkill(new ThJinlu);
    kami008->addSkill(new ThJinluTriggerSkill);
    related_skills.insertMulti("thjinlu", "#thjinlu");
    kami008->addSkill(new ThKuangli);

    General *kami009 = new General(this, "kami009", "kami");
    kami009->addSkill(new ThYuxin);
    kami009->addSkill(new ThChuangxin);
    kami009->addSkill(new ThTianxin);

    General *kami010 = new General(this, "kami010", "kami", 3, false);
    kami010->addSkill(new ThRangdeng);
    kami010->addSkill(new ThBaihun);

    General *kami011 = new General(this, "kami011", "kami");
    kami011->addSkill(new ThXujing);
    kami011->addSkill(new ThXujingClear);
    related_skills.insertMulti("thxujing", "#thxujing-clear");
    kami011->addSkill(new ThLingyun);
    kami011->addSkill(new ThZhaoai);

    General *kami012 = new General(this, "kami012", "kami");
    kami012->addSkill(new ThWunan);

    General *kami013 = new General(this, "kami013", "kami", 1, false);
    kami013->addSkill(new ThSanling);
    kami013->addSkill(new ThBingzhang);
    kami013->addSkill(new ThJiwu);
    kami013->addSkill(new ThSisui);
    kami013->addSkill(new ThZhanying);
    kami013->addSkill(new ThZhanyingMaxCardsSkill);
    related_skills.insertMulti("thzhanying", "#thzhanying");

    General *kami014 = new General(this, "kami014", "kami", 3, false);
    kami014->addSkill(new ThLuli);
    kami014->addSkill(new ThGuihuan);

    General *kami015 = new General(this, "kami015", "kami");
    kami015->addSkill(new ThZhizun);
    kami015->addSkill(new ThFeiying);

    General *kami016 = new General(this, "kami016", "kami", 3, false);
    kami016->addSkill(new ThLijian);
    kami016->addSkill(new ThSiqiang);
    kami016->addSkill(new ThJiefu);
    kami016->addSkill(new ThJiefuInvalidity);
    related_skills.insertMulti("thjiefu", "#thjiefu-inv");
    kami016->addSkill(new ThHuanxiang);
    kami016->addSkill(new ThHuanxiangPrevent);
    kami016->addSkill(new ThHuanxiangProhibit);
    related_skills.insertMulti("thhuanxiang", "#thhuanxiang-prevent");
    related_skills.insertMulti("thhuanxiang", "#thhuanxiang-prohibit");

    addMetaObject<ThShenfengCard>();
    addMetaObject<ThGugaoCard>();
    addMetaObject<ThLeshiCard>();
    addMetaObject<ThJingwuCard>();
    addMetaObject<ThYouyaCard>();
    addMetaObject<ThJinluCard>();
    addMetaObject<ThChuangxinCard>();
    addMetaObject<ThTianxinCard>();
    addMetaObject<ThBaihunCard>();
    addMetaObject<ThLingyunCard>();
    addMetaObject<ThBingzhangCard>();
    addMetaObject<ThSiqiangCard>();
    addMetaObject<ThJiefuCard>();

    skills << new ThKuangmo;
}

ADD_PACKAGE(TouhouKami)
