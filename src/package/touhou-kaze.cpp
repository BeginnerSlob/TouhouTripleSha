#include "touhou-kaze.h"

#include "carditem.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"
#include "skill.h"

class ThZhiji : public TriggerSkill
{
public:
    ThZhiji()
        : TriggerSkill("thzhiji")
    {
        frequency = Frequent;
        events << EventPhaseChanging << EventPhaseEnd;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == EventPhaseChanging)
            player->tag.remove("ThZhijiList");
        else if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() == Player::Discard) {
                if (player->tag["ThZhijiList"].toList().length() >= 2)
                    return QStringList(objectName());
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
        if (!player->isWounded() || room->askForChoice(player, objectName(), "recover+draw") == "draw")
            player->drawCards(2, objectName());
        else
            room->recover(player, RecoverStruct(player));

        return false;
    }
};

class ThZhijiRecord : public TriggerSkill
{
public:
    ThZhijiRecord()
        : TriggerSkill("#thzhiji")
    {
        events << CardsMoveOneTime;
        frequency = NotCompulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() == Player::Discard && move.from == player
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            QVariantList QijiList = player->tag["ThZhijiList"].toList();
            foreach (int id, move.card_ids) {
                int index = move.card_ids.indexOf(id);
                if (!QijiList.contains(id) && move.from_places[index] == Player::PlaceHand)
                    QijiList << id;
            }
            player->tag["ThZhijiList"] = QijiList;
        }

        return QStringList();
    }
};

ThJiyiCard::ThJiyiCard()
{
}

void ThJiyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    const Card *card = room->askForCard(target, ".Trick", "@thjiyi:" + source->objectName(), QVariant(), MethodNone);
    if (card) {
        room->showCard(target, card->getId());
        target->drawCards(1, "thjiyi");
    } else if (!target->isNude()) {
        const Card *card2 = room->askForCard(target, "..!", "@thjiyigive:" + source->objectName(), QVariant(), MethodNone);
        if (!card2) {
            QList<const Card *> cards = target->getCards("he");
            card2 = cards.at(qrand() % cards.length());
        }

        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), source->objectName(), "thjiyi", QString());
        room->obtainCard(source, card2, reason, false);
    }
}

class ThJiyi : public ZeroCardViewAsSkill
{
public:
    ThJiyi()
        : ZeroCardViewAsSkill("thjiyi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThJiyiCard");
    }

    virtual const Card *viewAs() const
    {
        return new ThJiyiCard;
    }
};

class ThYisi : public PhaseChangeSkill
{
public:
    ThYisi()
        : PhaseChangeSkill("thyisi$")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive() && target->hasLordSkill(objectName()) && target->getPhase() == Player::Start
            && target->getHp() == 1 && target->getMark("@yisi") == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#ThYisiWake";
        log.from = player;
        log.arg = QString::number(player->getHp());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(player, "@yisi", 1);
        if (player->isWounded())
            room->recover(player, RecoverStruct(player));
        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            if (player->isLord())
                room->acquireSkill(player, "thhuazhi");
        }

        return false;
    }
};

class ThHuazhi : public TriggerSkill
{
public:
    ThHuazhi()
        : TriggerSkill("thhuazhi$")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player && player->isAlive() && player->hasLordSkill(objectName()) && player->getPhase() == Player::NotActive) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceHand)) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getKingdom() == "kaze" && !p->isKongcheng())
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getKingdom() == "kaze" && !p->isKongcheng()) {
                const Card *card
                    = room->askForCard(p, ".", "@thhuazhi:" + player->objectName(), QVariant(), Card::MethodNone, player);
                if (card) {
                    room->broadcastSkillInvoke(objectName());
                    room->notifySkillInvoked(player, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = p;
                    log.to << player;
                    log.arg = objectName();
                    room->sendLog(log);

                    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, p->objectName(), player->objectName(), objectName(),
                                          QString());
                    room->obtainCard(player, card, reason, false);
                }
            }
        }
        return false;
    }
};

class ThJilanwen : public TriggerSkill
{
public:
    ThJilanwen()
        : TriggerSkill("thjilanwen")
    {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player && player->getPhase() == Player::Draw && TriggerSkill::triggerable(player))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            player->setFlags("ThJilanwenInvoke");
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        JudgeStruct judge;
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = player;

        room->judge(judge);

        Card::Suit suit = (Card::Suit)judge.pattern.toInt();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            foreach (const Card *card, p->getCards("ej")) {
                if (card->getSuit() != suit) {
                    targets << p;
                    break;
                }
            }
        }

        if (!targets.isEmpty()) {
            player->tag["ThJilanwenJudge"] = QVariant::fromValue(&judge); //for AI
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "thjilanwen-choose", true);
            if (target) {
                QList<int> disabled_ids;
                foreach (const Card *card, target->getCards("ej"))
                    if (card->getSuit() == suit)
                        disabled_ids << card->getEffectiveId();

                int card_id
                    = room->askForCardChosen(player, target, "ej", objectName(), false, Card::MethodNone, disabled_ids);

                if (card_id != -1)
                    room->obtainCard(player, card_id);
            } else
                player->obtainCard(judge.card);
            player->tag.remove("ThJilanwenJudge");
        } else
            player->obtainCard(judge.card);

        return false;
    }
};

class ThJilanwenGet : public TriggerSkill
{
public:
    ThJilanwenGet()
        : TriggerSkill("#thjilanwen")
    {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player != NULL) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "thjilanwen")
                judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return QStringList();
    }
};

class ThJilanwenDraw : public DrawCardsSkill
{
public:
    ThJilanwenDraw()
        : DrawCardsSkill("#thjilanwen-draw")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return player && player->isAlive() && player->hasFlag("ThJilanwenInvoke");
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        player->setFlags("-ThJilanwenInvoke");
        room->sendCompulsoryTriggerLog(player, "thjilanwen");

        return n - 1;
    }
};

ThNiankeCard::ThNiankeCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

void ThNiankeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thnianke", QString());
    room->obtainCard(target, this, reason);
    if (!target->isKongcheng()) {
        int card_id = room->askForCardChosen(source, target, "h", "thnianke");
        room->showCard(target, card_id);
        if (Sanguosha->getCard(card_id)->isRed()) {
            QList<ServerPlayer *> players;
            players << source << target;
            room->sortByActionOrder(players);
            room->drawCards(players, 1, "thnianke");
        }
    }
}

class ThNianke : public OneCardViewAsSkill
{
public:
    ThNianke()
        : OneCardViewAsSkill("thnianke")
    {
        filter_pattern = "Jink";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThNiankeCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThNiankeCard *card = new ThNiankeCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThJilan : public TriggerSkill
{
public:
    ThJilan()
        : TriggerSkill("thjilan")
    {
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QStringList skill_list;
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->canDiscard(p, "he")) {
                    for (int i = 0; i < damage.damage; i++)
                        skill_list << objectName();
                    break;
                }
            }
        }

        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (p->canDiscard(p, "he"))
                targets << p;
        if (!targets.isEmpty()) {
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thjilan", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThJilanTarget"] = QVariant::fromValue(target);
                return true;
            }
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThJilanTarget"].value<ServerPlayer *>();
        player->tag.remove("ThJilanTarget");
        int n = qMax(target->getLostHp(), 1);
        room->askForDiscard(target, objectName(), n, n, false, true);

        return false;
    }
};

class ThWangshou : public TriggerSkill
{
public:
    ThWangshou()
        : TriggerSkill("thwangshou")
    {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isDead())
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        JudgeStruct judge;
        judge.pattern = ".|black";
        judge.good = false;
        judge.negative = true;
        judge.reason = objectName();
        judge.who = damage.to;

        room->judge(judge);
        if (judge.isBad()) {
            if (player->canDiscard(damage.to, "he")
                && player->askForSkillInvoke("thwangshou_discard", "yes:" + damage.to->objectName())) {
                int card_id = room->askForCardChosen(player, damage.to, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, damage.to, player);
            }
        }

        return false;
    }
};

class ThZhanye : public TriggerSkill
{
public:
    ThZhanye()
        : TriggerSkill("thzhanye")
    {
        events << FinishJudge;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (player == NULL)
            return skill_list;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (!judge->card->isRed())
            return skill_list;

        if (player->isDead())
            return skill_list;

        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (owner == player || owner == room->getCurrent())
                continue;
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_thzhanye");
            if (owner->canSlash(player, slash, false))
                skill_list.insert(owner, QStringList(objectName()));
            delete slash;
        }

        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (room->askForCard(ask_who, "..", "@thzhanye:" + player->objectName(), QVariant::fromValue(player), objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_thzhanye");
        if (!ask_who->canSlash(player, slash, false)) {
            delete slash;
            return false;
        }
        room->useCard(CardUseStruct(slash, ask_who, player));
        return false;
    }
};

ThEnanCard::ThEnanCard()
{
}

bool ThEnanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && (Self == to_select || Self->inMyAttackRange(to_select));
}

void ThEnanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->loseMaxHp(source);
    room->loseHp(targets.first());
}

class ThEnan : public ZeroCardViewAsSkill
{
public:
    ThEnan()
        : ZeroCardViewAsSkill("thenan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThEnanCard");
    }

    virtual const Card *viewAs() const
    {
        return new ThEnanCard;
    }
};

class ThBeiyun : public TriggerSkill
{
public:
    ThBeiyun()
        : TriggerSkill("thbeiyun")
    {
        frequency = Frequent;
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DyingStruct dying = data.value<DyingStruct>();
        if (player != dying.who)
            return QStringList();
        if (player->isAlive() && player->getHp() < 1)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!player->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName());
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<int> card_ids = room->getNCards(qMax(4 - player->getMaxHp(), 1), false);
        CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        room->moveCardsAtomic(CardsMoveStruct(card_ids, NULL, Player::PlaceTable, reason), true);
        while (!card_ids.isEmpty()) {
            QStringList choices;
            choices << "cancel";
            foreach (int id, card_ids) {
                const Card *card = Sanguosha->getCard(id);
                if (card->isRed() && card->getTypeId() != Card::TypeTrick) {
                    choices << "red";
                    break;
                }
            }
            foreach (int id, card_ids) {
                const Card *card = Sanguosha->getCard(id);
                if (card->isBlack() && card->getTypeId() != Card::TypeTrick) {
                    choices << "black";
                    break;
                }
            }
            QString choice = room->askForChoice(player, objectName(), choices.join("+"),
                                                QVariant::fromValue(IntList2VariantList(card_ids)));

            DummyCard *dummy = new DummyCard;
            dummy->deleteLater();
            CardMoveReason reason2(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), objectName(), QString());
            if (choice == "red") {
                foreach (int id, card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (card->isRed() && card->getTypeId() != Card::TypeTrick) {
                        dummy->addSubcard(id);
                        card_ids.removeOne(id);
                    }
                }
                if (dummy->subcardsLength() > 0)
                    room->throwCard(dummy, reason2, NULL);
                dummy->clearSubcards();
                RecoverStruct recover(player);
                room->recover(player, recover);
            } else if (choice == "black") {
                foreach (int id, card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (card->isBlack() && card->getTypeId() != Card::TypeTrick) {
                        dummy->addSubcard(id);
                        card_ids.removeOne(id);
                    }
                }

                if (dummy->subcardsLength() > 0)
                    room->throwCard(dummy, reason2, NULL);
                dummy->clearSubcards();

                room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);

                LogMessage log;
                log.type = "#GainMaxHp";
                log.from = player;
                log.arg = QString::number(1);
                room->sendLog(log);

                LogMessage log2;
                log2.type = "#GetHp";
                log2.from = player;
                log2.arg = QString::number(player->getHp());
                log2.arg2 = QString::number(player->getMaxHp());
                room->sendLog(log2);
            } else {
                dummy->addSubcards(card_ids);
                if (room->askForChoice(player, objectName(), "get+discard") == "get")
                    player->obtainCard(dummy);
                else
                    room->throwCard(dummy, reason2, NULL);

                break;
            }
        }

        return false;
    }
};

class ThMicaiGivenSkill : public ViewAsSkill
{
public:
    ThMicaiGivenSkill()
        : ViewAsSkill("thmicaiv")
    {
        attached_lord_skill = true;
    }

    virtual bool shouldBeVisible(const Player *) const
    {
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getWeapon() || player->hasOwnerOnlySkill(true))
            return false;
        if (player->hasWeapon("spear")) {
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("spear");
            return spear_skill->isEnabledAtPlay(player);
        } else if (player->hasWeapon("fan")) {
            const ViewAsSkill *fan_skill = Sanguosha->getViewAsSkill("fan");
            return fan_skill->isEnabledAtPlay(player);
        } else
            return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getWeapon() || player->hasOwnerOnlySkill(true))
            return false;
        if (player->hasWeapon("spear")) {
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("spear");
            return spear_skill->isEnabledAtResponse(player, pattern);
        } else if (player->hasWeapon("fan")) {
            const ViewAsSkill *fan_skill = Sanguosha->getViewAsSkill("fan");
            return fan_skill->isEnabledAtResponse(player, pattern);
        } else
            return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->hasWeapon("spear")) {
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("spear");
            return spear_skill->viewFilter(selected, to_select);
        } else if (Self->hasWeapon("fan")) {
            const ViewAsSkill *fan_skill = Sanguosha->getViewAsSkill("fan");
            return fan_skill->viewFilter(selected, to_select);
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Self->hasWeapon("spear")) {
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("spear");
            return spear_skill->viewAs(cards);
        } else if (Self->hasWeapon("fan")) {
            const ViewAsSkill *fan_skill = Sanguosha->getViewAsSkill("fan");
            return fan_skill->viewAs(cards);
        } else
            return NULL;
    }
};

ThMicaiCard::ThMicaiCard()
{
}

void ThMicaiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "thmicaisource");
    room->addPlayerMark(effect.to, "@technology");
    room->attachSkillToPlayer(effect.to, "thmicaiv");
}

class ThMicai : public ZeroCardViewAsSkill
{
public:
    ThMicai()
        : ZeroCardViewAsSkill("thmicai")
    {
        owner_only_skill = true;
    }

    virtual const Card *viewAs() const
    {
        return new ThMicaiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThMicaiCard");
    }
};

class ThMicaiClear : public TriggerSkill
{
public:
    ThMicaiClear()
        : TriggerSkill("#thmicai")
    {
        events << EventPhaseStart << Death << EventLoseSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart || triggerEvent == Death) {
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return QStringList();
            }
            if (player->getMark("thmicaisource") <= 0)
                return QStringList();
            bool invoke = false;
            if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) || triggerEvent == Death)
                invoke = true;
            if (!invoke)
                return QStringList();
        } else if (triggerEvent == EventLoseSkill) {
            if (data.toString() != "thmicai")
                return QStringList();
        }

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("@technology") > 0) {
                room->setPlayerMark(p, "@technology", 0);
                room->detachSkillFromPlayer(p, "thmicaiv", true);
            }
        }
        room->setPlayerMark(player, "thmicaisource", 0);

        return QStringList();
    }
};

ThQiaogongCard::ThQiaogongCard()
{
}

bool ThQiaogongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (!targets.isEmpty() || !to_select->hasEquip())
        return false;

    Color color = Sanguosha->getCard(getEffectiveId())->getColor();
    foreach (const Card *cd, to_select->getEquips()) {
        if (!getSubcards().contains(cd->getEffectiveId()) && cd->getColor() == color)
            return true;
    }

    return false;
}

void ThQiaogongCard::onEffect(const CardEffectStruct &effect) const
{
    QList<int> disabled_ids;
    Color color = Sanguosha->getCard(getEffectiveId())->getColor();
    foreach (const Card *cd, effect.to->getEquips())
        if (cd->getColor() != color)
            disabled_ids << cd->getEffectiveId();

    Room *room = effect.from->getRoom();
    int id = room->askForCardChosen(effect.from, effect.to, "e", "thqiaogong", false, Card::MethodNone, disabled_ids);
    room->obtainCard(effect.from, id);
}

class ThQiaogong : public ViewAsSkill
{
public:
    ThQiaogong()
        : ViewAsSkill("thqiaogong")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->isJilei(to_select))
            return false;
        if (selected.isEmpty())
            return true;
        else if (selected.length() == 1)
            return to_select->sameColorWith(selected.first());
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ThQiaogongCard *card = new ThQiaogongCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThQiaogongCard");
    }
};

class ThGuiyu : public TriggerSkill
{
public:
    ThGuiyu()
        : TriggerSkill("thguiyu")
    {
        events << Damage << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->hasFlag("ThGuiyuUsed")) {
                room->setCardFlag(use.card, "-ThGuiyuUsed");
                room->addPlayerHistory(player, use.card->getClassName(), -1);
            }
            return QStringList();
        }
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.by_user && !damage.chain && !damage.transfer
                && damage.to && damage.to->isAlive()) {
                if (room->getCardPlace(damage.card->getEffectiveId()) == Player::PlaceTable)
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
        DamageStruct damage = data.value<DamageStruct>();
        damage.to->obtainCard(damage.card);
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (player->canDiscard(p, "he"))
                targets << p;
        }
        ServerPlayer *target = NULL;
        if (!targets.isEmpty())
            target = room->askForPlayerChosen(player, targets, objectName(), "@thguiyu", true);
        if (target) {
            int card_id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, target, player);
        } else
            room->setCardFlag(damage.card, "ThGuiyuUsed");

        return false;
    }
};

class ThZhouhua : public TriggerSkill
{
public:
    ThZhouhua()
        : TriggerSkill("thzhouhua")
    {
        events << EventPhaseEnd;
        frequency = Limited;
        limit_mark = "@zhouhua";
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Draw
            && target->aliveCount(false) < room->getPlayers().length() && target->getMark("@zhouhua") > 0;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->removePlayerMark(player, "@zhouhua");
            room->addPlayerMark(player, "@zhouhuaused");
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QStringList choices;
        if (player->isWounded())
            choices << "recover";
        choices << "draw";
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "recover")
            room->recover(player, RecoverStruct(player));
        else
            player->drawCards(2, objectName());

        room->acquireSkill(player, "thhuaimie");
        return false;
    }
};

class ThHuaimie : public TriggerSkill
{
public:
    ThHuaimie()
        : TriggerSkill("thhuaimie")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(player, objectName(), 0);
                room->detachSkillFromPlayer(player, "#thhuaimie", false, true);
                room->filterCards(player, player->getCards("he"), true);
            }
            return QStringList();
        }
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play)
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
        room->addPlayerMark(player, objectName(), 1);
        if (!player->hasSkill("#thhuaimie", true)) {
            room->acquireSkill(player, "#thhuaimie", false);
            room->filterCards(player, player->getCards("he"), false);
        }
        return false;
    }
};

class ThHuaimieFilter : public FilterSkill
{
public:
    ThHuaimieFilter()
        : FilterSkill("#thhuaimie")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->getTypeId() == Card::TypeTrick;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName("thhuaimie");
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class ThShenzhou : public TriggerSkill
{
public:
    ThShenzhou()
        : TriggerSkill("thshenzhou")
    {
        frequency = Frequent;
        events << TurnedOver << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        QStringList skills;
        if (triggerEvent == TurnedOver && player->faceUp())
            skills << objectName();
        else if (triggerEvent == Damaged)
            skills << objectName();
        return skills;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!player->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName());
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<int> card_ids = room->getNCards(qMin(room->alivePlayerCount(), 5));
        CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        room->moveCardsAtomic(CardsMoveStruct(card_ids, NULL, Player::PlaceTable, reason), true);
        QStringList choices;
        foreach (int id, card_ids) {
            QString str = Sanguosha->getCard(id)->getType();
            if (str == "skill")
                continue;
            if (choices.contains(str))
                continue;
            choices << str;
        }
        QString type
            = room->askForChoice(player, objectName(), choices.join("+"), QVariant::fromValue(IntList2VariantList(card_ids)));
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName(), QString(), false, true);
        CardMoveReason reason2(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), objectName(), QString());
        DummyCard *dummy = new DummyCard;
        dummy->deleteLater();
        foreach (int id, card_ids)
            if (Sanguosha->getCard(id)->getType() == type) {
                card_ids.removeOne(id);
                dummy->addSubcard(id);
            }
        room->obtainCard(target, dummy);
        dummy->clearSubcards();
        dummy->addSubcards(card_ids);
        if (dummy->subcardsLength() > 0)
            room->throwCard(dummy, reason2, NULL);

        return false;
    }
};

class ThTianliu : public TriggerSkill
{
public:
    ThTianliu()
        : TriggerSkill("thtianliu")
    {
        frequency = Compulsory;
        events << EventPhaseStart << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName())
                judge->pattern = QString::number(int(judge->card->getSuit()));
        } else if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());

        JudgeStruct judge;
        judge.good = false;
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        Card::Suit suit = (Card::Suit)judge.pattern.toInt();
        switch (suit) {
        case Card::Heart:
            player->drawCards(3);
            break;
        case Card::Diamond:
            player->drawCards(2);
            break;
        case Card::Club:
            player->drawCards(1);
            break;
        default:
            break;
        }

        return true;
    }
};

ThQianyiCard::ThQianyiCard()
{
}

bool ThQianyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void ThQianyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->removePlayerMark(source, "@qianyi");
    room->addPlayerMark(source, "@qianyiused");

    source->turnOver();
    QStringList choices;
    if (target->isWounded())
        choices << "recover";
    choices << "draw";
    QString choice = room->askForChoice(source, "thqianyi", choices.join("+"), QVariant::fromValue(target));
    if (choice == "recover")
        room->recover(target, RecoverStruct(source));
    else
        target->drawCards(2, "thqianyi");
}

class ThQianyi : public ZeroCardViewAsSkill
{
public:
    ThQianyi()
        : ZeroCardViewAsSkill("thqianyi")
    {
        frequency = Limited;
        limit_mark = "@qianyi";
    }

    virtual const Card *viewAs() const
    {
        return new ThQianyiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@qianyi") > 0;
    }
};

ThHuosuiCard::ThHuosuiCard()
{
}

bool ThHuosuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->inMyAttackRange(to_select);
}

void ThHuosuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    if (!room->askForCard(target, "jink", "@thhuosuijink:" + source->objectName(), QVariant(), Card::MethodResponse, source))
        if (!room->askForUseSlashTo(source, target, "@thhuosui-slash:" + target->objectName()))
            source->drawCards(1);
}

class ThHuosui : public OneCardViewAsSkill
{
public:
    ThHuosui()
        : OneCardViewAsSkill("thhuosui")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThHuosuiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThHuosuiCard *card = new ThHuosuiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThTiandi : public TriggerSkill
{
public:
    ThTiandi()
        : TriggerSkill("thtiandi")
    {
        frequency = Frequent;
        events << TargetConfirming;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
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
        player->drawCards(1);
        return false;
    }
};

ThKunyiCard::ThKunyiCard()
{
}

void ThKunyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->removePlayerMark(source, "@kunyi");
    room->addPlayerMark(source, "@kunyiused");

    source->turnOver();
    room->damage(DamageStruct("thkunyi", source, target));
}

class ThKunyi : public ZeroCardViewAsSkill
{
public:
    ThKunyi()
        : ZeroCardViewAsSkill("thkunyi")
    {
        frequency = Limited;
        limit_mark = "@kunyi";
    }

    virtual const Card *viewAs() const
    {
        return new ThKunyiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@kunyi") > 0;
    }
};

ThCannveCard::ThCannveCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

void ThCannveCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "thcannve",
                          QString());
    room->obtainCard(effect.to, this, reason);
    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.to))
        if (effect.to->canSlash(p, true))
            targets << p;

    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "thcannve");

        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = effect.from;
        log.to << target;
        room->sendLog(log);

        if (room->askForUseSlashTo(effect.to, target, "@thcannve-slash:" + target->objectName()))
            return;
    }

    QStringList choices;
    if (!effect.to->isNude())
        choices << "get";
    choices << "hit";
    QString choice = room->askForChoice(effect.from, "thcannve", choices.join("+"), QVariant::fromValue(effect.to));
    if (choice == "get") {
        int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thcannve");
        room->obtainCard(effect.from, card_id, false);
    } else
        room->damage(DamageStruct("thcannve", effect.from, effect.to));
};

class ThCannve : public OneCardViewAsSkill
{
public:
    ThCannve()
        : OneCardViewAsSkill("thcannve")
    {
        filter_pattern = ".|diamond";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThCannveCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThCannveCard *card = new ThCannveCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThSibao : public OneCardViewAsSkill
{
public:
    ThSibao()
        : OneCardViewAsSkill("thsibao")
    {
        filter_pattern = "EquipCard,DelayedTrick";
        response_pattern = "peach+analeptic";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Analeptic *jiu = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        jiu->addSubcard(originalCard);
        jiu->setSkillName(objectName());
        return jiu;
    }
};

class ThWangqin : public TriggerSkill
{
public:
    ThWangqin()
        : TriggerSkill("thwangqin")
    {
        events << CardUsed << CardResponded;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card = resp.m_card;
        }
        if (!card)
            return skill_list;
        if (player->isChained() && card->getTypeId() != Card::TypeSkill && card->isRed())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                if (owner != player && !owner->faceUp())
                    skill_list.insert(owner, QStringList(objectName()));

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
        ask_who->turnOver();
        player->turnOver();
        room->setPlayerProperty(player, "chained", false);

        return false;
    }
};

class ThFusuo : public TriggerSkill
{
public:
    ThFusuo()
        : TriggerSkill("thfusuo")
    {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player))
                if (!p->isChained())
                    return QStringList(objectName());
        } else if (player->getPhase() == Player::Finish && player->hasFlag("fusuoinvoke"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getPhase() == Player::Start) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (!p->isChained())
                    targets << p;
            Q_ASSERT(!targets.isEmpty());
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thfusuoinvoke", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThFusuoTarget"] = QVariant::fromValue(target);
                return true;
            }
        } else
            return true;

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getPhase() == Player::Start) {
            ServerPlayer *target = player->tag["ThFusuoTarget"].value<ServerPlayer *>();
            player->tag.remove("ThFusuoTarget");
            if (target) {
                player->turnOver();
                room->setPlayerProperty(target, "chained", true);
                room->setPlayerFlag(player, "fusuoinvoke");
            }
        } else if (player->getPhase() == Player::Finish) {
            room->sendCompulsoryTriggerLog(player, objectName());

            if (!room->askForCard(player, "^BasicCard", "@thfusuo"))
                room->loseHp(player);
        }

        return false;
    }
};

ThGelongCard::ThGelongCard()
{
}

bool ThGelongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThGelongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    bool success = source->pindian(target, "thgelong");
    if (success) {
        const Card *card = NULL;
        if (!source->isKongcheng())
            card = room->askForCard(source, ".", "@thgelonggive:" + target->objectName(), QVariant(), Card::MethodNone);
        if (card) {
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thgelong",
                                  QString());
            room->obtainCard(target, card, reason, false);
        } else
            room->loseHp(source);
    } else {
        QStringList choices;
        choices << "damage";
        if (!target->isNude())
            choices << "get";
        QString choice = room->askForChoice(source, "thgelong", choices.join("+"), QVariant::fromValue(target));
        if (choice == "damage")
            room->damage(DamageStruct("thgelong", source, target));
        else {
            int card_id = room->askForCardChosen(source, target, "he", "thgelong");
            room->obtainCard(source, card_id, false);
        }
    }
}

class ThGelong : public ZeroCardViewAsSkill
{
public:
    ThGelong()
        : ZeroCardViewAsSkill("thgelong")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThGelongCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new ThGelongCard;
    }
};

class ThYuanzhou : public TriggerSkill
{
public:
    ThYuanzhou()
        : TriggerSkill("thyuanzhou")
    {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (p->getHandcardNum() < player->getHandcardNum())
                    return QStringList();
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (victims.isEmpty() || p->getHandcardNum() == victims.first()->getHandcardNum())
                victims << p;
            else if (p->getHandcardNum() > victims.first()->getHandcardNum()) {
                victims.clear();
                victims << p;
            }
        ServerPlayer *target = room->askForPlayerChosen(player, victims, objectName(), "@thyuanzhou", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThYuanzhouTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThYuanzhouTarget"].value<ServerPlayer *>();
        player->tag.remove("ThYuanzhouTarget");
        if (target) {
            int card_id = room->askForCardChosen(player, target, "hej", "thyuanzhou", false, Card::MethodDiscard);
            room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : target, player);
        }
        return false;
    }
};

ThDasuiCard::ThDasuiCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void ThDasuiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addToPile("tassel", this);
}

class ThDasuiViewAsSkill : public ViewAsSkill
{
public:
    ThDasuiViewAsSkill()
        : ViewAsSkill("thdasui")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThDasuiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.size() < 3 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.size() != 0) {
            ThDasuiCard *card = new ThDasuiCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class ThDasui : public TriggerSkill
{
public:
    ThDasui()
        : TriggerSkill("thdasui")
    {
        events << EventPhaseStart;
        view_as_skill = new ThDasuiViewAsSkill;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() != Player::Play)
            return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (owner == player || owner->getPile("tassel").isEmpty())
                continue;
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
        QList<int> card_ids = ask_who->getPile("tassel");
        if (card_ids.isEmpty())
            return false;
        room->fillAG(card_ids);
        int card_id = room->askForAG(player, card_ids, true, objectName());
        room->clearAG();
        if (card_id != -1)
            room->obtainCard(player, card_id);
        return false;
    }
};

class ThFengren : public TriggerSkill
{
public:
    ThFengren()
        : TriggerSkill("thfengren")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Start)
            return !player->getPile("tassel").isEmpty();
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());

        QList<int> card_ids = player->getPile("tassel");
        DummyCard *dummy = new DummyCard(card_ids);
        QStringList choices;
        if (card_ids.length() >= 2 && player->isWounded())
            choices << "recover";
        choices << "obtain";
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "recover") {
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
            room->throwCard(dummy, reason, NULL);
            room->recover(player, RecoverStruct(player));
        } else {
            CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, player->objectName(), objectName(), QString());
            room->obtainCard(player, dummy, reason);
        }

        return false;
    }
};

class ThFuli : public TriggerSkill
{
public:
    ThFuli()
        : TriggerSkill("thfuli")
    {
        frequency = Frequent;
        events << TargetConfirmed << BeforeCardsMove << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setMark(objectName(), 0);
            }
        } else if (TriggerSkill::triggerable(player)) {
            if (triggerEvent == TargetConfirmed) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.to.contains(player)) {
                    player->setFlags("fuli_target");
                    use.card->setFlags("thfuli");
                }
            } else if (triggerEvent == BeforeCardsMove && player->hasFlag("fuli_target")
                       && player->getPile("tassel").length() < 3) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
                    && move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
                    && player != move.from && player->getMark(objectName()) == 0) {
                    const Card *card = move.reason.m_extraData.value<const Card *>();
                    if (card->hasFlag("thfuli") && card->getNumber() >= 2 && card->getNumber() <= 9)
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *card = move.reason.m_extraData.value<const Card *>();
        player->setFlags("-fuli_target");
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(card))) {
            room->broadcastSkillInvoke(objectName());
            card->setFlags("-thfuli");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->addMark(objectName());
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        player->addToPile("tassel", move.card_ids);
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);

        return false;
    }
};

class ThKudao : public TriggerSkill
{
public:
    ThKudao()
        : TriggerSkill("thkudao")
    {
        events << TargetSpecified << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        ServerPlayer *target = NULL;
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill && use.card->isRed() && use.to.length() == 1)
                target = use.to.first();
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player && use.from->hasFlag("CollateralUsing") && use.card->isKindOf("Slash")
                && use.card->isRed()) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (p->hasFlag("CollateralSource")) {
                        target = p;
                        break;
                    }
            }
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();

            if (resp.m_card->isRed() && resp.m_card->isKindOf("BasicCard"))
                target = resp.m_who;
        }

        if (target == NULL || target == player)
            return QStringList();

        if (!target->isNude()) {
            player->tag["ThKudaoTarget"] = QVariant::fromValue(target);
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThKudaoTarget"].value<ServerPlayer *>();
        if (target && player->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        player->tag.remove("ThKudaoTarget");
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThKudaoTarget"].value<ServerPlayer *>();
        player->tag.remove("ThKudaoTarget");
        if (target) {
            int card_id = room->askForCardChosen(player, target, "he", objectName());
            player->addToPile("leaf", card_id, true);
        }
        return false;
    }
};

ThSuilunCard::ThSuilunCard()
{
    target_fixed = true;
}

void ThSuilunCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->gainAnExtraTurn();
}

class ThSulunViewAsSkill : public ViewAsSkill
{
public:
    ThSulunViewAsSkill()
        : ViewAsSkill("thsuilun")
    {
        response_pattern = "@@thsuilun";
        expand_pile = "leaf";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.isEmpty())
            return Self->getHandcards().contains(to_select) && !Self->isJilei(to_select);
        else if (selected.length() == 1)
            return Self->getPile("leaf").contains(to_select->getId());
        else if (selected.length() == 2)
            return Self->getPile("leaf").contains(to_select->getId()) && to_select->getSuit() != selected.at(1)->getSuit();
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.size() == 3) {
            int hand = 0;
            foreach (const Card *card, cards) {
                if (Self->getHandcards().contains(card))
                    ++hand;
            }
            if (hand != 1)
                return NULL;
            ThSuilunCard *card = new ThSuilunCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class ThSuilun : public TriggerSkill
{
public:
    ThSuilun()
        : TriggerSkill("thsuilun")
    {
        events << EventPhaseStart;
        view_as_skill = new ThSulunViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (player->getPhase() == Player::NotActive) {
            if (player->getPile("leaf").length() > 1)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(player, "@@thsuilun", "@thsuilun");
        return false;
    }
};

ThRansangCard::ThRansangCard()
{
}

bool ThRansangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThRansangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    bool success = source->pindian(target, "thransang");
    if (success) {
        room->setPlayerFlag(source, "thransang");
        room->acquireSkill(source, "thyanlun");
    } else
        room->setPlayerCardLimitation(source, "use", "TrickCard|black", true);
}

class ThRansangViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThRansangViewAsSkill()
        : ZeroCardViewAsSkill("thransang")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThRansangCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new ThRansangCard;
    }
};

class ThRansang : public TriggerSkill
{
public:
    ThRansang()
        : TriggerSkill("thransang")
    {
        events << EventPhaseChanging;
        view_as_skill = new ThRansangViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!player->hasFlag("thransang"))
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive)
            room->detachSkillFromPlayer(player, "thyanlun", false, true);
        return QStringList();
    }
};

class ThYanlunViewAsSkill : public OneCardViewAsSkill
{
public:
    ThYanlunViewAsSkill()
        : OneCardViewAsSkill("thyanlun")
    {
        response_or_use = true;
        filter_pattern = ".|red|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        FireAttack *fa = new FireAttack(Card::SuitToBeDecided, -1);
        fa->deleteLater();
        return fa->isAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FireAttack *fa = new FireAttack(Card::SuitToBeDecided, -1);
        fa->addSubcard(originalCard);
        fa->setSkillName(objectName());
        return fa;
    }
};

class ThYanlun : public TriggerSkill
{
public:
    ThYanlun()
        : TriggerSkill("thyanlun")
    {
        events << Damage;
        view_as_skill = new ThYanlunViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (TriggerSkill::triggerable(player) && damage.card->isKindOf("FireAttack"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName()))
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1, objectName());
        return false;
    }
};

class ThBazhiFilterSkill : public FilterSkill
{
public:
    ThBazhiFilterSkill()
        : FilterSkill("thbazhi")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        if (!to_select->isKindOf("Lightning") && !(to_select->isKindOf("Jink") && to_select->getSuit() == Card::Diamond))
            return false;

        Room *room = Sanguosha->currentRoom();
        ServerPlayer *splayer = NULL;
        foreach (ServerPlayer *p, room->getAllPlayers())
            if (room->getCardOwner(to_select->getEffectiveId()) == p) {
                splayer = p;
                break;
            }

        if (splayer == NULL)
            return false;

        foreach (ServerPlayer *p, room->getOtherPlayers(splayer))
            if (splayer->getHp() > qMax(0, p->getHp()))
                return true;

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FireSlash *fs = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        fs->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(fs);
        return card;
    }
};

class ThBazhi : public TriggerSkill
{
public:
    ThBazhi()
        : TriggerSkill("thbazhi")
    {
        frequency = Compulsory;
        events << HpChanged;
        view_as_skill = new ThBazhiFilterSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        foreach (ServerPlayer *p, room->findPlayersBySkillName("thbazhi"))
            room->filterCards(p, p->getCards("he"), true);

        return false;
    }
};

ThYanxingCard::ThYanxingCard()
{
    target_fixed = true;
}

void ThYanxingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QStringList choices;
    if (source->getHp() > 0)
        choices << "hp";
    choices << "maxhp";
    if (room->askForChoice(source, "thyanxing", choices.join("+")) == "hp")
        room->loseHp(source);
    else
        room->loseMaxHp(source);

    if (source->isAlive()) {
        room->setPlayerFlag(source, "thyanxing");
        room->acquireSkill(source, "thheyu");
    }
}

class ThYanxingViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThYanxingViewAsSkill()
        : ZeroCardViewAsSkill("thyanxing")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThYanxingCard");
    }

    virtual const Card *viewAs() const
    {
        return new ThYanxingCard;
    }
};

class ThYanxing : public TriggerSkill
{
public:
    ThYanxing()
        : TriggerSkill("thyanxing")
    {
        events << EventPhaseChanging;
        view_as_skill = new ThYanxingViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!player->hasFlag("thyanxing"))
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive)
            room->detachSkillFromPlayer(player, "thheyu", false, true);
        return QStringList();
    }
};

class ThHeyu : public TriggerSkill
{
public:
    ThHeyu()
        : TriggerSkill("thheyu")
    {
        events << DamageCaused;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire)
            return QStringList(objectName());

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        DamageStruct damage = data.value<DamageStruct>();
        ++damage.damage;
        data = QVariant::fromValue(damage);
        return false;
    }
};

class ThMaihuo : public TriggerSkill
{
public:
    ThMaihuo()
        : TriggerSkill("thmaihuo")
    {
        events << TargetSpecifying;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill && use.card->getSuit() == Card::Heart) {
                QStringList objs;
                foreach (ServerPlayer *p, use.to)
                    objs << p->objectName();
                return QStringList(objectName() + "->" + objs.join("+"));
            }
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
        Card::Suit suit = room->askForSuit(ask_who, objectName());

        LogMessage log;
        log.type = "#ChooseSuit";
        log.from = ask_who;
        log.arg = Card::Suit2String(suit);
        room->sendLog(log);

        if (!player->isKongcheng()) {
            room->showAllCards(player);
            int n = 0;
            foreach (const Card *c, player->getHandcards()) {
                if (c->getSuit() == suit)
                    ++n;
            }
            if (n > 0)
                player->drawCards(qMin(n, 3), objectName());
        }
        return false;
    }
};

class ThWunian : public TriggerSkill
{
public:
    ThWunian()
        : TriggerSkill("thwunian")
    {
        frequency = Compulsory;
        events << Predamage << CardEffected;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == Predamage) {
            return QStringList(objectName());
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if ((effect.card->isNDTrick() || effect.card->isKindOf("Slash")) && !effect.from->isWounded()
                && effect.from->getMaxHp() != 1)
                return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        if (triggerEvent == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.from = NULL;
            damage.by_user = false;
            data = QVariant::fromValue(damage);
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            effect.nullified = true;
            data = QVariant::fromValue(effect);
        }

        return false;
    }
};

class ThDongxi : public TriggerSkill
{
public:
    ThDongxi()
        : TriggerSkill("thdongxi")
    {
        events << EventPhaseStart;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player->getPhase() == Player::Start && TriggerSkill::triggerable(player)) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                foreach (const Skill *skill, p->getGeneral()->getVisibleSkillList()) {
                    if (skill->isLordSkill() || skill->isAttachedLordSkill() || skill->isOwnerOnlySkill()
                        || skill->getFrequency(player) == Skill::Limited || skill->getFrequency(player) == Skill::Wake)
                        continue;
                    if (player->tag.value("ThDongxiLast").toStringList().contains(skill->objectName()))
                        continue;
                    return QStringList(objectName());
                }
            }
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        TriggerList skill_list;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            QStringList skills;
            foreach (const Skill *skill, p->getGeneral()->getVisibleSkillList()) {
                if (skill->isLordSkill() || skill->isAttachedLordSkill() || skill->isOwnerOnlySkill()
                    || skill->getFrequency(player) == Skill::Limited || skill->getFrequency(player) == Skill::Wake)
                    continue;
                if (player->tag.value("ThDongxiLast").toStringList().contains(skill->objectName()))
                    continue;
                if (!player->hasSkill(skill->objectName()))
                    skills << skill->objectName();
            }
            if (!skills.isEmpty())
                skill_list.insert(p, skills);
        }

        if (!skill_list.keys().isEmpty()) {
            ServerPlayer *target = room->askForPlayerChosen(player, skill_list.keys(), objectName(), "@thdongxi", true, true);
            if (target) {
                QStringList choices = skill_list.value(target, QStringList());
                if (!choices.isEmpty()) {
                    QString choice = room->askForChoice(player, objectName(), choices.join("+"));
                    QStringList list = player->tag.value("ThDongxi").toStringList();
                    list << choice;
                    player->tag["ThDongxi"] = QVariant::fromValue(list);

                    JsonArray args;
                    args << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                    args << player->objectName();
                    args << target->getGeneral()->objectName();
                    args << choice;
                    args << false;
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

                    return true;
                }
            }
        }

        player->tag.remove("ThDongxi");
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->acquireSkill(player, player->tag["ThDongxi"].toStringList().last());
        return false;
    }
};

class ThDongxiClear : public TriggerSkill
{
public:
    ThDongxiClear()
        : TriggerSkill("#thdongxi-clear")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player->getPhase() == Player::Start)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QStringList names = player->tag.value("ThDongxi").toStringList();
        player->tag.remove("ThDongxi");
        QStringList _names;
        foreach (QString name, names)
            _names << "-" + name;
        room->handleAcquireDetachSkills(player, _names, true);

        JsonArray args;
        args << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        args << player->objectName();
        args << player->getGeneral()->objectName();
        args << QString();
        args << false;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        player->tag["ThDongxiLast"] = QVariant::fromValue(names);
        return false;
    }
};

ThSangzhiCard::ThSangzhiCard()
{
}

void ThSangzhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->setPlayerMark(targets.first(), "@sangzhi", 1);
    foreach (ServerPlayer *pl, room->getAllPlayers())
        room->filterCards(pl, pl->getCards("he"), true);
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    source->tag["ThSangzhiUsed"] = true;
}

class ThSangzhiViewAsSkill : public OneCardViewAsSkill
{
public:
    ThSangzhiViewAsSkill()
        : OneCardViewAsSkill("thsangzhi")
    {
        filter_pattern = "Peach,EquipCard!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThSangzhiCard *card = new ThSangzhiCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThSangzhiCard");
    }
};

class ThSangzhi : public TriggerSkill
{
public:
    ThSangzhi()
        : TriggerSkill("thsangzhi")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new ThSangzhiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!player || !player->tag["ThSangzhiUsed"].toBool())
            return QStringList();

        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        }
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player) {
                return QStringList();
            }
        }
        player->tag.remove("ThSangzhiUsed");
        foreach (ServerPlayer *p, room->getAllPlayers())
            if (p->getMark("@sangzhi") > 0) {
                room->setPlayerMark(p, "@sangzhi", 0);

                foreach (ServerPlayer *pl, room->getAllPlayers())
                    room->filterCards(pl, pl->getCards("he"), false);
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }

        return QStringList();
    }
};

class ThSangzhiInvalidity : public InvaliditySkill
{
public:
    ThSangzhiInvalidity()
        : InvaliditySkill("#thsangzhi-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *) const
    {
        return player->getMark("@sangzhi") == 0;
    }
};

ThXinhuaCard::ThXinhuaCard()
{
    will_throw = false;
    m_skillName = "thxinhuav";
    handling_method = MethodNone;
    mute = true;
}

bool ThXinhuaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("thxinhua") && to_select != Self
        && !to_select->hasFlag("ThXinhuaInvoked");
}

void ThXinhuaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    if (target->hasLordSkill("thxinhua")) {
        room->setPlayerFlag(target, "ThXinhuaInvoked");
        room->broadcastSkillInvoke("thxinhua");
        room->notifySkillInvoked(target, "thxinhua");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thxinhua",
                              QString());
        room->obtainCard(target, this, reason);
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->isKongcheng())
                continue;
            victims << p;
        }
        if (!victims.isEmpty()) {
            source->tag["ThXinhuaLord"] = QVariant::fromValue(target); // for AI
            ServerPlayer *victim = room->askForPlayerChosen(source, victims, "thxinhua");
            source->tag.remove("ThXinhuaLord");

            LogMessage log;
            log.type = "$IkLingtongView";
            log.from = target;
            log.to << victim;
            log.arg = "iklingtong:handcards";
            room->sendLog(log, room->getOtherPlayers(target));

            room->showAllCards(victim, target);
        }
        QList<ServerPlayer *> lords;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasLordSkill("thxinhua") && !p->hasFlag("ThXinhuaInvoked")) {
                lords << p;
            }
        }
        if (lords.empty())
            room->setPlayerFlag(source, "ForbidThXinhua");
    }
}

class ThXinhuaViewAsSkill : public OneCardViewAsSkill
{
public:
    ThXinhuaViewAsSkill()
        : OneCardViewAsSkill("thxinhuav")
    {
        attached_lord_skill = true;
        filter_pattern = "Weapon";
    }

    virtual bool shouldBeVisible(const Player *player) const
    {
        return player->getKingdom() == "kaze";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getKingdom() == "kaze" && !player->hasFlag("ForbidThXinhua");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThXinhuaCard *card = new ThXinhuaCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class ThXinhua : public TriggerSkill
{
public:
    ThXinhua()
        : TriggerSkill("thxinhua$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!player)
            return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "thxinhua")) {
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
                if (!p->hasSkill("thxinhuav"))
                    room->attachSkillToPlayer(p, "thxinhuav");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "thxinhua") {
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
                if (p->hasSkill("thxinhuav"))
                    room->detachSkillFromPlayer(p, "thxinhuav", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play && phase_change.to != Player::Play)
                return QStringList();
            if (player->hasFlag("ForbidThXinhua"))
                room->setPlayerFlag(player, "-ForbidThXinhua");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("ThXinhuaInvoked"))
                    room->setPlayerFlag(p, "-ThXinhuaInvoked");
            }
        }

        return QStringList();
    }
};

TouhouKazePackage::TouhouKazePackage()
    : Package("touhou-kaze")
{
    General *kaze001 = new General(this, "kaze001$", "kaze", 3);
    kaze001->addSkill(new ThZhiji);
    kaze001->addSkill(new ThZhijiRecord);
    related_skills.insertMulti("thzhiji", "#thzhiji");
    kaze001->addSkill(new ThJiyi);
    kaze001->addSkill(new ThYisi);
    kaze001->addRelateSkill("thhuazhi");

    General *kaze002 = new General(this, "kaze002", "kaze");
    kaze002->addSkill(new ThJilanwen);
    kaze002->addSkill(new ThJilanwenGet);
    kaze002->addSkill(new ThJilanwenDraw);
    related_skills.insertMulti("thjilanwen", "#thjilanwen");
    related_skills.insertMulti("thjilanwen", "#thjilanwen-draw");

    General *kaze003 = new General(this, "kaze003", "kaze", 3);
    kaze003->addSkill(new ThNianke);
    kaze003->addSkill(new ThJilan);

    General *kaze004 = new General(this, "kaze004", "kaze");
    kaze004->addSkill(new ThWangshou);
    kaze004->addSkill(new ThZhanye);
    kaze004->addSkill(new SlashNoDistanceLimitSkill("thzhanye"));
    related_skills.insertMulti("thzhanye", "#thzhanye-slash-ndl");

    General *kaze005 = new General(this, "kaze005", "kaze", 3, false);
    kaze005->addSkill(new ThEnan);
    kaze005->addSkill(new ThBeiyun);

    General *kaze006 = new General(this, "kaze006", "kaze");
    kaze006->addSkill(new ThMicai);
    kaze006->addSkill(new ThMicaiClear);
    related_skills.insertMulti("thmicai", "#thmicai");
    kaze006->addSkill(new ThQiaogong);

    General *kaze007 = new General(this, "kaze007", "kaze");
    kaze007->addSkill(new ThGuiyu);
    kaze007->addSkill(new ThZhouhua);
    kaze007->addRelateSkill("thhuaimie");

    General *kaze008 = new General(this, "kaze008", "kaze", 3);
    kaze008->addSkill(new ThShenzhou);
    kaze008->addSkill(new ThTianliu);
    kaze008->addSkill(new ThQianyi);

    General *kaze009 = new General(this, "kaze009", "kaze", 3, false);
    kaze009->addSkill(new ThHuosui);
    kaze009->addSkill(new ThTiandi);
    kaze009->addSkill(new ThKunyi);

    General *kaze010 = new General(this, "kaze010", "kaze", 3);
    kaze010->addSkill(new ThCannve);
    kaze010->addSkill(new ThSibao);

    General *kaze011 = new General(this, "kaze011", "kaze");
    kaze011->addSkill(new ThWangqin);
    kaze011->addSkill(new ThFusuo);

    General *kaze012 = new General(this, "kaze012", "kaze");
    kaze012->addSkill(new ThGelong);
    kaze012->addSkill(new ThYuanzhou);

    General *kaze013 = new General(this, "kaze013", "kaze", 3);
    kaze013->addSkill(new ThDasui);
    kaze013->addSkill(new ThFengren);
    kaze013->addSkill(new ThFuli);

    General *kaze014 = new General(this, "kaze014", "kaze", 3);
    kaze014->addSkill(new ThKudao);
    kaze014->addSkill(new ThSuilun);

    General *kaze015 = new General(this, "kaze015", "kaze");
    kaze015->addSkill(new ThRansang);
    kaze015->addRelateSkill("thyanlun");

    General *kaze016 = new General(this, "kaze016", "kaze", 5);
    kaze016->addSkill(new ThBazhi);
    kaze016->addSkill(new ThYanxing);
    kaze016->addRelateSkill("thheyu");

    General *kaze017 = new General(this, "kaze017", "kaze", 3, false);
    kaze017->addSkill(new ThMaihuo);
    kaze017->addSkill(new ThWunian);

    General *kaze018 = new General(this, "kaze018$", "kaze", 3);
    kaze018->addSkill(new ThDongxi);
    kaze018->addSkill(new ThDongxiClear);
    related_skills.insertMulti("thdongxi", "#thdongxi-clear");
    kaze018->addSkill(new ThSangzhi);
    kaze018->addSkill(new ThSangzhiInvalidity);
    related_skills.insertMulti("thsangzhi", "#thsangzhi-inv");
    kaze018->addSkill(new ThXinhua);

    addMetaObject<ThJiyiCard>();
    addMetaObject<ThNiankeCard>();
    addMetaObject<ThEnanCard>();
    addMetaObject<ThMicaiCard>();
    addMetaObject<ThQiaogongCard>();
    addMetaObject<ThQianyiCard>();
    addMetaObject<ThHuosuiCard>();
    addMetaObject<ThKunyiCard>();
    addMetaObject<ThCannveCard>();
    addMetaObject<ThGelongCard>();
    addMetaObject<ThDasuiCard>();
    addMetaObject<ThSuilunCard>();
    addMetaObject<ThRansangCard>();
    addMetaObject<ThYanxingCard>();
    addMetaObject<ThSangzhiCard>();
    addMetaObject<ThXinhuaCard>();

    skills << new ThHuazhi << new ThMicaiGivenSkill << new ThHuaimie << new ThHuaimieFilter << new ThYanlun << new ThHeyu
           << new ThXinhuaViewAsSkill;
    related_skills.insertMulti("thhuaimie", "#thhuaimie");
}

ADD_PACKAGE(TouhouKaze)
