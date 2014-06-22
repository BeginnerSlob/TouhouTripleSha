#include "touhou.h"

#include "general.h"
#include "skill.h"
#include "room.h"
#include "carditem.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"

class ThJianmo: public TriggerSkill {
public:
    ThJianmo(): TriggerSkill("thjianmo") {
        events << EventPhaseStart << EventPhaseChanging << CardUsed;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (player->hasFlag("jianmoinvoke"))
                room->setPlayerFlag(player, "-jianmoinvoke");
            room->removePlayerCardLimitation(player, "use,response", "Slash$0");
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() != Player::Play || player->getHandcardNum() < player->getMaxHp())
                return skill_list;
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (player == p) continue;
                skill_list.insert(p, QStringList(objectName()));
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && player->hasFlag("jianmoinvoke"))
                skill_list.insert(player, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        if (triggerEvent == EventPhaseStart) {
            if (ask_who->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        } else
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (triggerEvent == EventPhaseStart) {
            if (room->askForChoice(player, objectName(), "jian+mo") == "jian") {
                LogMessage log;
                log.type = "#thjianmochoose1";
                log.from = player;
                log.arg = "1";
                room->sendLog(log);
                player->drawCards(1);
                room->setPlayerCardLimitation(player, "use,response", "Slash", false);
            } else {
                LogMessage log;
                log.type = "#thjianmochoose2";
                log.from = player;
                log.arg = "2";
                room->sendLog(log);
                room->setPlayerFlag(player, "jianmoinvoke");
            }
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            LogMessage log;
            log.type = "#ThJianmo";
            log.from = player;
            log.arg = objectName();
            log.arg2 = use.card->objectName();
            room->sendLog(log);

            if (!room->askForDiscard(player, objectName(), 1, 1, true, true, "@thjianmo")) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    use.nullified_list << p->objectName();
            }
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class ThErchong: public TriggerSkill {
public:
    ThErchong(): TriggerSkill("therchong") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start
                                                 && target->getHp() <= 2
                                                 && target->getMark("@erchong") <= 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        LogMessage log;
        log.type = "#ThErchong";
        log.from = player;
        log.arg  = objectName();
        log.arg2 = QString::number(qMax(0, player->getHp()));
        room->sendLog(log);

        room->addPlayerMark(player, "@erchong");

        room->askForDiscard(player, objectName(), 2, 2, false, true);

        room->changeMaxHpForAwakenSkill(player);
        room->acquireSkill(player, "thhuanfa");
        room->acquireSkill(player, "ikzhuji");

        return false;
    }
};

class ThChundu: public TriggerSkill {
public:
    ThChundu(): TriggerSkill("thchundu$") {
        events << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        const Card *trigger_card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->getKingdom() != "yuki")
                return QStringList();
            trigger_card = use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (!resp.m_isUse || player->getKingdom() != "yuki")
                return QStringList();
            trigger_card = resp.m_card;
        }
        if (trigger_card == NULL)
            return QStringList();

        if (trigger_card->isKindOf("BasicCard") && trigger_card->getSuit() == Card::Heart) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill("thchundu"))
                    return QStringList(objectName());
            }
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasLordSkill("thchundu"))
                targets << p;
        }

        while (!targets.isEmpty()) {
            ServerPlayer *target = room->askForPlayerChosen(player, targets, "thchundu", QString(), true);
            if (target) {
                targets.removeOne(target);

                LogMessage log;
                log.type = "#InvokeOthersSkill";
                log.from = player;
                log.to << target;
                log.arg = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(target, objectName());

                QList<ServerPlayer *> _target;
                _target.append(target);
                QList<int> chundu_cards = room->getNCards(1, false);

                CardsMoveStruct move(chundu_cards, NULL, target, Player::PlaceTable, Player::PlaceHand,
                                     CardMoveReason(CardMoveReason::S_REASON_PREVIEW, target->objectName(), objectName(), QString()));
                QList<CardsMoveStruct> moves;
                moves.append(move);
                room->notifyMoveCards(true, moves, false, _target);
                room->notifyMoveCards(false, moves, false, _target);

                QList<int> origin_chundu = chundu_cards;
                while (room->askForYiji(target, chundu_cards, objectName(), true, false, true, -1, room->getOtherPlayers(target))) {
                    CardsMoveStruct move(QList<int>(), target, NULL, Player::PlaceHand, Player::PlaceTable,
                                         CardMoveReason(CardMoveReason::S_REASON_PREVIEW, target->objectName(), objectName(), QString()));
                    foreach (int id, origin_chundu) {
                        if (room->getCardPlace(id) != Player::DrawPile) {
                            move.card_ids << id;
                            chundu_cards.removeOne(id);
                        }
                    }
                    origin_chundu = chundu_cards;
                    QList<CardsMoveStruct> moves;
                    moves.append(move);
                    room->notifyMoveCards(true, moves, false, _target);
                    room->notifyMoveCards(false, moves, false, _target);
                    if (!target->isAlive())
                        return false;
                }

                if (!chundu_cards.isEmpty()) {
                    CardsMoveStruct move(chundu_cards, target, NULL, Player::PlaceHand, Player::PlaceTable,
                                         CardMoveReason(CardMoveReason::S_REASON_PREVIEW, target->objectName(), objectName(), QString()));
                    QList<CardsMoveStruct> moves;
                    moves.append(move);
                    room->notifyMoveCards(true, moves, false, _target);
                    room->notifyMoveCards(false, moves, false, _target);

                    DummyCard *dummy = new DummyCard(chundu_cards);
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, target->objectName(), objectName(), QString());
                    room->throwCard(dummy, reason, NULL);
                    delete dummy;
                }
            } else
                break;
        }

        return false;
    }
};

class ThCuimeng: public TriggerSkill {
public:
    ThCuimeng(): TriggerSkill("thcuimeng") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (player->getPhase() == Player::Start){
            if (TriggerSkill::triggerable(player))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())){
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        JudgeStruct judge;
        forever {
            judge.pattern = ".|red";
            judge.good = true;
            judge.reason = objectName();
            judge.play_animation = false;
            judge.who = player;
            judge.time_consuming = true;

            room->judge(judge);
            if ((judge.isGood() && !player->askForSkillInvoke(objectName())) || judge.isBad())
                break;
        }

        return false;
    }
};

class ThCuimengMove: public TriggerSkill {
public:
    ThCuimengMove(): TriggerSkill("#thcuimeng-move") {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == "thcuimeng") {
                if (judge->isGood()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStar judge = data.value<JudgeStar>();
        room->moveCardTo(judge->card, judge->who, Player::PlaceHand, true);

        return false;
    }
};

class ThZuimengFilterSkill: public FilterSkill {
public:
    ThZuimengFilterSkill(): FilterSkill("thzuimeng") {
    }

    virtual bool viewFilter(const Card* to_select) const{
        return to_select->isKindOf("BasicCard") && to_select->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *jiu = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        jiu->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(jiu);
        return card;
    }
};

class ThZuimeng: public TriggerSkill {
public:
    ThZuimeng(): TriggerSkill("thzuimeng") {
        events << CardUsed;
        frequency = Compulsory;
        view_as_skill = new ThZuimengFilterSkill;
    }
    
    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && use.card->isKindOf("Analeptic"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = "thzuimeng";
        room->sendLog(log);

        player->drawCards(1);

        return false;
    }
};

class ThMengwu: public MaxCardsSkill {
public:
    ThMengwu(): MaxCardsSkill("thmengwu") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return 1;
        else
            return 0;
    }
};

class ThCihang: public TriggerSkill {
public:
    ThCihang(): TriggerSkill("thcihang") {
        events << SlashMissed;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (room->askForChoice(player, objectName(), "discard+draw") == "discard") {
            int x = effect.to->getLostHp();
            if (x != 0)
                room->askForDiscard(player, objectName(), x, x, false, true);
        } else {
            int x = qMin(qMax(effect.to->getHp(), 0), 5);
            if (x != 0)
                effect.to->drawCards(x);
        }

        room->slashResult(effect, NULL);
                    
        return true;
    }
};

class ThZhancao: public TriggerSkill {
public:
    ThZhancao(): TriggerSkill("thzhancao") {
        events << TargetConfirming << BeforeCardsMove;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p == room->getCurrent() && p->getPhase() != Player::NotActive)
                        continue;
                    if (p->inMyAttackRange(player) || p == player)
                        skill_list.insert(p, QStringList(objectName()));
                }
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->tag["thzhancao_user"].toBool()) {
                        CardStar zhancao_card = move.reason.m_extraData.value<CardStar>();
                        if (zhancao_card && zhancao_card->hasFlag("thzhancao"))
                            skill_list.insert(p, QStringList(objectName()));
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const {
        if (triggerEvent == TargetConfirming) {
            if (ask_who->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                if (!room->askForCard(ask_who, "EquipCard|.|.|equipped", "@thzhancao")) {
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
        } else if (triggerEvent == BeforeCardsMove) {
            ask_who->tag["thzhancao_user"] = false;
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetConfirming) {
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            const Card *card = move.reason.m_extraData.value<CardStar>();
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

ThYuanqiGiveCard::ThYuanqiGiveCard() {
    m_skillName = "thyuanqi";
    will_throw = false;
    handling_method = MethodNone;
}

void ThYuanqiGiveCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    ServerPlayer *target = targets.first();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thyuanqi", QString());
    room->obtainCard(target, this, reason);

    QStringList choices;
    choices << "draw";
    if (target->getCards("he").length() != 1)
        choices << "throw";
    QString choice = room->askForChoice(target, "thyuanqi", choices.join("+"));

    if (choice == "draw") {
        target->drawCards(1);
        room->loseHp(target);
    } else {
        room->throwCard(this, target);
        int id = room->askForCardChosen(source, target, "he", "thyuanqi");
        room->obtainCard(source, id, false);
    }
}

ThYuanqiCard::ThYuanqiCard() {
    target_fixed = true;
}

void ThYuanqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const {
    JudgeStruct judge;
    judge.good = true;
    judge.play_animation = false;
    judge.reason = "thyuanqi";
    judge.who = source;
    room->judge(judge);

    QString pattern = ".|";
    if (judge.card->isRed())
        pattern += "red";
    else if (judge.card->isBlack())
        pattern += "black";
    pattern += "|.|hand";
    room->setPlayerProperty(source, "thyuanqi_pattern", pattern);
    room->askForUseCard(source, "@@thyuanqi", "@thyuanqi", -1, MethodNone);
    room->setPlayerProperty(source, "thyuanqi_pattern", "");
}

class ThYuanqi: public ViewAsSkill {
public:
    ThYuanqi(): ViewAsSkill("thyuanqi") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const {
        if (Self->property("thyuanqi_pattern").toString().isEmpty())
            return false;
        else if (selected.isEmpty()) {
            QString str = Self->property("thyuanqi_pattern").toString();
            ExpPattern pattern(str);
            return pattern.match(Self, to_select);
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (Self->property("thyuanqi_pattern").toString().isEmpty())
            return new ThYuanqiCard;
        else if (!cards.isEmpty()) {
            ThYuanqiGiveCard *card = new ThYuanqiGiveCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThYuanqiCard");
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const {
        return pattern == "@@thyuanqi";
    }
};

class ThMoji: public TriggerSkill {
public:
    ThMoji(): TriggerSkill("thmoji") {
        events << EventPhaseChanging << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        } else if (triggerEvent == CardsMoveOneTime) {
            if (player == room->getCurrent() && player->getPhase() != Player::NotActive) return QStringList();
            bool can_invoke = false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceHand))
                can_invoke = true;
            if (move.to == player && move.to_place == Player::PlaceHand)
                can_invoke = true;
            if (!can_invoke)
                return QStringList();
        }
        
        if (player->getHandcardNum() < 2)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        player->drawCards(2 - player->getHandcardNum(), objectName());

        return false;
    }
};

class ThJibu: public DistanceSkill {
public:
    ThJibu(): DistanceSkill("thjibu") {
    }

    virtual int getCorrect(const Player *from, const Player *) const {
        if (from->hasSkill(objectName()))
            return -1;
        else
            return 0;
    }
};

class ThDunjia: public TriggerSkill {
public:
    ThDunjia(): TriggerSkill("thdunjia") {
        events << Damage;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.card || !damage.card->isKindOf("Slash") || damage.to->isDead())
            return QStringList();
        int n = qAbs(player->getEquips().size() - damage.to->getEquips().size());
        if (n == 0)
            return QStringList();

        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();
        int n = qMin(3, qAbs(player->getEquips().size() - damage.to->getEquips().size()));
        QStringList choices;
        if (damage.to->getCardCount(true) >= n)
            choices << "discard";

        choices << "draw";

        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "discard") {
            room->setPlayerFlag(damage.to, "thdunjia_InTempMoving");
            DummyCard *dummy = new DummyCard;
            QList<int> card_ids;
            QList<Player::Place> original_places;
            for (int i = 0; i < n; i++) {
                if (!player->canDiscard(damage.to, "he"))
                    break;
                card_ids << room->askForCardChosen(player, damage.to, "he", objectName(), false, Card::MethodDiscard);
                original_places << room->getCardPlace(card_ids[i]);
                dummy->addSubcard(card_ids[i]);
                player->addToPile("#thdunjia", card_ids[i], false);
            }
            for (int i = 0; i < dummy->subcardsLength(); i++)
                room->moveCardTo(Sanguosha->getCard(card_ids[i]), damage.to, original_places[i], false);
            room->setPlayerFlag(damage.to, "-thdunjia_InTempMoving");
            if (dummy->subcardsLength() > 0)
                room->throwCard(dummy, damage.to, player);
            dummy->deleteLater();
        } else
            player->drawCards(n);

        return false;
    }
};

ThChouceCard::ThChouceCard() {
    will_throw = false;
}

bool ThChouceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    const Card *card = Sanguosha->getCard(getSubcards().first());
    if (!card)
        return false;
    if (card->isKindOf("Collateral"))
        return targets.isEmpty()
            || (targets.length() == 1 && targets.first()->canSlash(to_select));
    else
        return targets.isEmpty();
}

bool ThChouceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const {
    const Card *card = Sanguosha->getCard(getSubcards().first());
    if (!card)
        return false;
    if (card->isKindOf("Collateral")) {
        if (targets.length() == 2)
            return true;
        else if (targets.length() == 1) {
            const Player *target = targets.first();
            foreach (const Player *p, target->getAliveSiblings())
                if (target->canSlash(p))
                    return false;
            return true;
        } else
            return false;
    } else
        return targets.length() == 1;
}

const Card *ThChouceCard::validate(CardUseStruct &card_use) const{
    const Card *card = Sanguosha->getCard(getSubcards().first());
    Card *use_card = Sanguosha->cloneCard(card->objectName(), card->getSuit(), card->getNumber());
    use_card->addSubcard(card);
    use_card->setSkillName("thchouce");
    Room *room = card_use.from->getRoom();
    
    room->setPlayerMark(card_use.from, "ThChouce", use_card->getNumber());
    card_use.from->addMark("choucecount");

    return use_card;
}

class ThChouceViewAsSkill: public OneCardViewAsSkill {
public:
    ThChouceViewAsSkill(): OneCardViewAsSkill("thchouce") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return true;
    }

    virtual bool viewFilter(const Card* to_select) const{
        if (to_select->getNumber() <= Self->getMark("ThChouce"))
            return false;

        if (to_select->isKindOf("Slash") || to_select->isKindOf("Analeptic"))
            return to_select->isAvailable(Self);
        else
            return !Self->isCardLimited(to_select, Card::MethodUse, true)
                && !to_select->isKindOf("Jink")
                && !to_select->isKindOf("Nullification")
                && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThChouceCard *card = new ThChouceCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThChouce: public TriggerSkill {
public:
    ThChouce(): TriggerSkill("thchouce"){
        events << PreCardUsed << CardResponded << EventPhaseChanging;
        view_as_skill = new ThChouceViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        QStringList skills;
        if (!TriggerSkill::triggerable(player)) return skills;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Play) {
                room->setPlayerMark(player, "ThChouce", 0);
                room->setPlayerMark(player, "choucecount", 0);
                room->setPlayerFlag(player, "-ThChouce_failed");
            }
        } else if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            if (player->getPhase() != Player::Play)
                return skills;
            const Card *usecard = NULL;
            if (triggerEvent == PreCardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                usecard = use.card;
            } else if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    usecard = resp.m_card;
            }
            if (!usecard || usecard->getTypeId() == Card::TypeSkill || usecard->getSkillName() == objectName())
                return skills;

            if (usecard->isKindOf("Jink") || usecard->isKindOf("Nullification")) {
                room->setPlayerFlag(player, "ThChouce_failed");
                room->setPlayerMark(player, "ThChouce", usecard->getNumber());
                return skills;
            }

            int precardnum = player->getMark("ThChouce"); //the cardnumber store of thchouce
            if (usecard->getNumber() > precardnum) {
                if (usecard->isKindOf("Collateral")) {
                    foreach (ServerPlayer *p, room->getAlivePlayers())
                        if (!player->isProhibited(p, usecard)) {
                            foreach(ServerPlayer *target, room->getOtherPlayers(p))
                                if (p->inMyAttackRange(target)) {
                                    skills << objectName();
                                    break;
                                }
                            if (!skills.isEmpty())
                                break;
                        }
                } else {
                    foreach (ServerPlayer *p, room->getAlivePlayers())
                        if (!player->isProhibited(p, usecard)) {
                            skills << objectName();
                            break;
                        }
                }
            } else if (usecard->toString() != player->tag["ThChouceCard"].toString()){
                room->setPlayerFlag(player, "ThChouce_failed");
                room->setPlayerMark(player, "ThChouce", usecard->getNumber());
            }
        }

        return skills;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        QList<ServerPlayer *> targets;
        const Card *usecard = NULL;
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            usecard = use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse)
                usecard = resp.m_card;
        }
        if (!usecard) return false;
        if (usecard->isKindOf("Collateral")) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                if (!player->isProhibited(p, usecard))
                    foreach(ServerPlayer *target, room->getOtherPlayers(p))
                        if (p->inMyAttackRange(target))
                            targets << p;
        } else {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                if (!player->isProhibited(p, usecard))
                    targets << p;
        }
        room->setPlayerMark(player, "ThChouce", usecard->getNumber());
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thchouce", true);
        player->tag["ThChouceCard"] = usecard->toString();
        if (target) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            player->tag["ThChouceTarget"] = QVariant::fromValue(target);
            return true;
        } else
            room->setPlayerFlag(player, "ThChouce_failed");

        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThChouceTarget"].value<PlayerStar>();
        if (!target) return false;
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Collateral")) {
                foreach (ServerPlayer *killer, use.to)
                    killer->tag.remove("collateralVictim");
            }
            use.to.clear();
            use.to << target;
            if (use.card->isKindOf("Collateral")) {
                foreach (ServerPlayer *killer, use.to) {
                    QList<ServerPlayer *> victims;
                    foreach (ServerPlayer *p, room->getOtherPlayers(killer))
                        if (killer->inMyAttackRange(p))
                            victims << p;
                    Q_ASSERT(!victims.isEmpty());
                    ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName(), "thchouce-slash:" + killer->objectName());
                    killer->tag["collateralVictim"] = QVariant::fromValue(victim);
                }
            }
            data = QVariant::fromValue(use);

            LogMessage log;
            log.type = "$ThChouce";
            log.from = player;
            log.to = use.to;
            log.arg = objectName();
            log.card_str = QString::number(use.card->getEffectiveId());
            room->sendLog(log);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            resp.m_who = target;
            data = QVariant::fromValue(resp);

            LogMessage log;
            log.type = "$ThChouce";
            log.from = player;
            log.to << resp.m_who;
            log.arg = objectName();
            log.card_str = QString::number(resp.m_card->getEffectiveId());
            room->sendLog(log);
        }
        
        room->addPlayerMark(player, "choucecount"); //the count of thchouce
        return false;
    }
};

class ThZhanshi: public TriggerSkill{
public:
    ThZhanshi(): TriggerSkill("thzhanshi") {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return TriggerSkill::triggerable(target)
               && target->getPhase() == Player::Play
               && !target->hasFlag("ThChouce_failed")
               && target->getMark("choucecount") >= 3;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *) const {
        room->setPlayerMark(target, "choucecount", 0);
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = target;
        log.arg = objectName();
        room->sendLog(log);
        target->gainMark("@tianji");
        return false;
    }
};

class ThZhanshiDo: public TriggerSkill {
public:
    ThZhanshiDo(): TriggerSkill("#thzhanshi") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return TriggerSkill::triggerable(target)
               && target->getPhase() == Player::NotActive
               && target->getMark("@tianji") > 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *) const {
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = target;
        log.arg = "thzhanshi";
        room->sendLog(log);
        target->gainAnExtraTurn();
        return false;
    }
};

class ThHuanzang: public TriggerSkill{
public:
    ThHuanzang(): TriggerSkill("thhuanzang"){
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Judge
            && target->getMark("@tianji") > 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        
        player->loseMark("@tianji");

        JudgeStruct judge;
        judge.pattern = ".|spade";
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

class ThZiyun: public ProhibitSkill {
public:
    ThZiyun(): ProhibitSkill("thziyun") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        if (to->hasSkill(objectName()))
            return card->isKindOf("SupplyShortage") || card->isKindOf("Lightning");
        
        return false;
    }
};

class ThChuiji: public TriggerSkill {
public:
    ThChuiji(): TriggerSkill("thchuiji") {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && (move.to != player || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip))) {
            bool invoke = false;
            if (player->isWounded())
                invoke = true;
            else
                foreach (ServerPlayer *p, room->getOtherPlayers(player))
                    if (!p->isNude() || p->isWounded()) {
                        invoke = true;
                        break;
                    }

            if (invoke && player != room->getCurrent())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
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

            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            RecoverStruct recover;
            recover.who = player;
            room->recover(target, recover);
        } else if (judge.card->isBlack()) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (p->canDiscard(p, "he"))
                    targets << p;

            if (targets.isEmpty())
                return false;

            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            room->askForDiscard(target, objectName(), 1, 1, false, true);
        }

        return false;
    }
};

class ThLingya: public TriggerSkill {
public:
    ThLingya(): TriggerSkill("thlingya") {
        events << CardFinished;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        ServerPlayer *current = room->getCurrent();
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(current) && current->getPhase() != Player::NotActive 
            && use.card->isRed() && player != current) {
            ask_who = current;
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        QStringList choices;
        choices << "letdraw";
        if (ask_who->canDiscard(player, "he"))
            choices << "discard";

        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "discard") {
            int card_id = room->askForCardChosen(ask_who, player, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, player, ask_who);
        } else
            ask_who->drawCards(1);

        return false;
    }
};

class ThHeimu: public TriggerSkill {
public:
    ThHeimu(): TriggerSkill("thheimu") {
        events << PreCardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && !player->hasUsed("ThHeimu")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->getPhase() == Player::Play && use.card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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
            if (!can) continue;
            targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thheimu", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThHeimuTarget"] = QVariant::fromValue(target);
            room->addPlayerHistory(player, "ThHeimu");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = player->tag["ThHeimuTarget"].value<PlayerStar>();
        player->tag.remove("ThHeimuTarget");
        if (target) {
            CardUseStruct use = data.value<CardUseStruct>();
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
            room->moveCardTo(use.card, NULL, Player::DiscardPile, reason, true);
            QString key = use.card->getClassName();
            room->addPlayerHistory(player, key, -1);
            if (use.to.isEmpty())
                use.to << use.from;

            use.from = target;
            use.m_isOwnerUse = false;
            room->useCard(use, false);
            return true;
        }

        return false;
    }
};

class ThHanpo: public TriggerSkill {
public:
    ThHanpo(): TriggerSkill("thhanpo") {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused && (damage.to == player || damage.nature != DamageStruct::Fire))
            return QStringList();
        else if (triggerEvent == DamageInflicted && damage.nature != DamageStruct::Fire)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        return true;
    }
};

class ThZhengguan: public TriggerSkill {
public:
    ThZhengguan(): TriggerSkill("thzhengguan") {
        events << EventPhaseSkipped;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        Player::Phase phase = (Player::Phase)data.toInt();
        if (phase == Player::Play || phase == Player::Draw) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player) continue;
                skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        QString str = ask_who->getPhaseString((Player::Phase)data.toInt());
        if (room->askForCard(ask_who, ".|red", "@thzhengguan:::" + str, data, objectName())) {
            LogMessage log;
            log.type = "#ThZhengguan";
            log.from = ask_who;
            log.arg = objectName();
            log.arg2 = str;
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        Player::Phase phase = (Player::Phase)data.toInt();
        QList<Player::Phase> phases;
        phases << phase;
        ask_who->play(phases);

        return false;
    }
};

ThBingpuCard::ThBingpuCard(){
    target_fixed = true;
}

void ThBingpuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    room->removePlayerMark(source, "@bingpu");
    room->addPlayerMark(source, "@bingpuused");
    foreach(ServerPlayer *target, room->getOtherPlayers(source)) {
        if (target->isNude())
            continue;

        if (!room->askForCard(target, "jink", "@thbingpu:" + source->objectName(), QVariant(), MethodResponse)) {
            int card_id = room->askForCardChosen(source, target, "he", objectName(), false, MethodDiscard);
            room->throwCard(card_id, target, source);

            if (!target->isNude()) {
                card_id = room->askForCardChosen(source, target, "he", objectName(), false, MethodDiscard);
                room->throwCard(card_id, target, source);
            }
        }
    }
}

class ThBingpu: public ZeroCardViewAsSkill {
public:
    ThBingpu(): ZeroCardViewAsSkill("thbingpu") {
        frequency = Limited;
        limit_mark = "@bingpu";
    }

    virtual const Card *viewAs() const{
        return new ThBingpuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@bingpu") > 0;
    }
};

ThDongmoCard::ThDongmoCard() {
}

bool ThDongmoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.size() < Self->getLostHp() && to_select != Self;
}

void ThDongmoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    targets << source;
    room->sortByActionOrder(targets);
    foreach(ServerPlayer *p, targets)
        p->turnOver();
    foreach(ServerPlayer *p, targets)
        p->drawCards(1);
}

class ThDongmoViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThDongmoViewAsSkill(): ZeroCardViewAsSkill("thdongmo") {
        response_pattern == "@@thdongmo";
    }

    virtual const Card *viewAs() const{
        return new ThDongmoCard;
    }
};

class ThDongmo:public TriggerSkill{
public:
    ThDongmo():TriggerSkill("thdongmo"){
        events << EventPhaseStart;
        view_as_skill = new ThDongmoViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && target->isWounded();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        room->askForUseCard(player, "@@thdongmo", "@thdongmo");
        return false;
    }
};

class ThLinhan:public TriggerSkill{
public:
    ThLinhan():TriggerSkill("thlinhan"){
        events << CardResponded;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardResponseStruct resp = data.value<CardResponseStruct>();
        if (!resp.m_card->isKindOf("Jink"))
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->drawCards(1);
        return false;
    }
};

class ThFusheng: public TriggerSkill {
public:
    ThFusheng(): TriggerSkill("thfusheng") {
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        QStringList skills;
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == HpRecover) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.who != player)
                for (int i = 0; i < recover.recover; i++)
                    skills << objectName();
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (source != player) {
                skills << objectName();
            }
        }
        return skills;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (triggerEvent == HpRecover) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            RecoverStruct recover = data.value<RecoverStruct>();
            recover.who->drawCards(1, objectName());
        } else if (triggerEvent == Damaged) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            
            DamageStruct damage = data.value<DamageStruct>();
            const Card *card = room->askForCard(damage.from, ".|heart|.|hand", "@thfusheng-heart", data, Card::MethodNone);
            if (card) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, damage.from->objectName(), player->objectName(), objectName(), QString());
                room->obtainCard(player, card, reason);
            } else
                room->loseHp(damage.from);
        }

        return false;
    }
};

ThHuanfaCard::ThHuanfaCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void ThHuanfaCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    CardMoveReason reason3(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "thhuanfa", QString());
    room->obtainCard(effect.to, this, reason3);

    int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thhuanfa");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    targets.removeOne(effect.from);
    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "thhuanfa", "@thhuanfa-give:" + effect.to->objectName(), true);
        if (target) {
            CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), target->objectName(), "thhuanfa", QString());
            room->obtainCard(target, Sanguosha->getCard(card_id), reason2, false);
        }
    }
}

class ThHuanfa: public OneCardViewAsSkill {
public:
    ThHuanfa():OneCardViewAsSkill("thhuanfa") {
        filter_pattern = ".|heart|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("ThHuanfaCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThHuanfaCard *huanfaCard = new ThHuanfaCard;
        huanfaCard->addSubcard(originalCard);
        return huanfaCard;
    }
};

class ThSaozang: public TriggerSkill {
public:
    ThSaozang(): TriggerSkill("thsaozang") {
        events << CardsMoveOneTime << EventPhaseStart << EventPhaseEnd;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThSaozangTarget"].value<PlayerStar>();
        player->tag.remove("ThSaozangTarget");
        if (target && player->canDiscard(target, "h")) {
            int card_id = room->askForCardChosen(player, target, "h", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, target, player);
        }

        return false;
    }
};

class ThXuqu: public TriggerSkill {
public:
    ThXuqu(): TriggerSkill("thxuqu") {
        events << CardsMoveOneTime;
    }
    
    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceHand)
            && (move.to != player || move.to_place != Player::PlaceEquip)) {
            if (player != room->getCurrent())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        ServerPlayer *victim = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thxuqu", true, true);
        if (victim) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThXuquTarget"] = QVariant::fromValue(victim);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThXuquTarget"].value<PlayerStar>();
        player->tag.remove("ThXuquTarget");
        if (target)
            target->drawCards(1);

        return false;
    }
};

class ThQiebao:public TriggerSkill{
public:
    ThQiebao():TriggerSkill("thqiebao"){
        events << CardUsed << BeforeCardsMove;
    }

    bool doQiebao(Room *room, ServerPlayer *player, QList<int> card_ids, bool discard) const {
        const Card *card = room->askForCard(player, "slash", "@thqiebao", QVariant(), Card::MethodResponse, NULL, false, objectName());
        if (!card)
            return false;
        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        
        if (!card_ids.isEmpty()) {
            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct qiebaoMove;

            if (card->isRed()) {
                qiebaoMove.to = player;
                qiebaoMove.to_place = Player::PlaceHand;
                qiebaoMove.card_ids = card_ids;
                exchangeMove.push_back(qiebaoMove);
                room->moveCardsAtomic(exchangeMove, false);
            } else if (discard) {
                qiebaoMove.to = NULL;
                qiebaoMove.to_place = Player::DiscardPile;
                CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
                qiebaoMove.reason = reason;
                qiebaoMove.card_ids = card_ids;
                exchangeMove.push_back(qiebaoMove);
                room->moveCardsAtomic(exchangeMove, false);
            }
        }

        return true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.isEmpty())
                return QStringList();

            bool invoke = false;
            foreach (ServerPlayer *to, use.to) {
                if (to == use.from) continue;
                invoke = true;
                break;
            }

            if (invoke && use.card->isKindOf("Peach")) {
                return QStringList(objectName());
            }
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from || !move.to || move.from == move.to)
                return QStringList();

            if (move.to_place == Player::PlaceHand) {
                QList<int> qiebaolist;
                for (int i = 0; i < move.card_ids.size(); i++){
                    if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<int> card_ids;
            if (!use.card->isVirtualCard())
                card_ids << use.card->getEffectiveId();
            else
                card_ids = use.card->getSubcards();
            if (doQiebao(room, player, card_ids, false)) {
                foreach (ServerPlayer *p, use.to)
                    if (p != use.from)
                        use.nullified_list << p->objectName();
                data = QVariant::fromValue(objectName());
            }
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> qiebaolist;
            for (int i = 0; i < move.card_ids.size(); i++){
                int card_id = move.card_ids[i];
                if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)
                    qiebaolist << card_id;
            }
            if (!qiebaolist.isEmpty() && doQiebao(room, player, qiebaolist, true)) {
                move.removeCardIds(qiebaolist);
                data = QVariant::fromValue(move);
            }
        }

        return false;
    }
};

class ThLingta: public TriggerSkill {
public:
    ThLingta(): TriggerSkill("thlingta") {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->isSkipped(change.to))
            return QStringList();

        if (change.to == Player::Draw || change.to == Player::Play)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(QString::number((int)change.to)))) {
            room->broadcastSkillInvoke(objectName());
            player->skip(change.to, true);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        player->gainMark("@fadeng");
        return false;
    }
};

class ThWeiguang: public TriggerSkill {
public:
    ThWeiguang(): TriggerSkill("thweiguang") {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player) || player->getMark("@fadeng") <= 0)
            return QStringList();

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        
        if (change.from == Player::Play || change.from == Player::Draw) {
            if (!player->hasFlag(objectName() + QString::number((int)change.from)))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(QString::number((int)change.from)))) {
            room->setPlayerFlag(player, objectName() + QString::number((int)change.from));
            player->loseMark("@fadeng");
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        player->insertPhase(change.from);
        change.to = change.from;
        data = QVariant::fromValue(change);
        return false;
    }
};

class ThWeiguangSkip: public TriggerSkill {
public:
    ThWeiguangSkip(): TriggerSkill("#thweiguang-skip") {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player) || player->getMark("@fadeng") <= 0)
            return QStringList();

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        
        if (change.to == Player::Judge || change.to == Player::Discard) {
            if (!player->hasFlag("thweiguang" + QString::number((int)change.to)))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->askForSkillInvoke("thweiguang", QVariant::fromValue(QString::number((int)change.to)))) {
            room->setPlayerFlag(player, "thweiguang" + QString::number((int)change.to));
            player->loseMark("@fadeng");
            room->broadcastSkillInvoke("thweiguang");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        player->skip(change.to);
        return false;
    }
};

class ThChuhui: public TriggerSkill {
public:
    ThChuhui(): TriggerSkill("thchuhui") {
        events << GameStart;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        player->gainMark("@fadeng");
        return false;
    }
};

ThKujieCard::ThKujieCard() {
    m_skillName = "thkujiev";
    mute = true;
}

bool ThKujieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasSkill("thkujie")
           && Self->inMyAttackRange(to_select) && !to_select->hasFlag("ThKujieInvoked");
}

void ThKujieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
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

class ThKujieViewAsSkill: public OneCardViewAsSkill{
public:
    ThKujieViewAsSkill(): OneCardViewAsSkill("thkujiev") {
        attached_lord_skill = true;
        filter_pattern = "BasicCard|red";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThKujieCard *card = new ThKujieCard();
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasFlag("ForbidThKujie");
    }
};

class ThKujie: public TriggerSkill {
public:
    ThKujie(): TriggerSkill("thkujie") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if ((triggerEvent == GameStart)
            || (triggerEvent == EventAcquireSkill && data.toString() == "thkujie")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

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
            if (lords.length() > 2) return QStringList();

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
            if (phase_change.from != Player::Play)
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

class ThKujieRecover: public TriggerSkill {
public:
    ThKujieRecover(): TriggerSkill("#thkujie-recover") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() == Player::NotActive)
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->getMark("kujie-invoke") > 0 && p->isWounded())
                    skill_list.insert(p, QStringList(objectName()));
                else if (!p->isWounded())
                    room->setPlayerMark(p, "kujie-invoke", 0);
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const {
        room->setPlayerMark(ask_who, "kujie-invoke", 0);
        room->recover(ask_who, RecoverStruct(ask_who, NULL, 2));
        return false;
    }
};

class ThYinbi: public TriggerSkill {
public:
    ThYinbi(): TriggerSkill("thyinbi") {
        events << HpChanged;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (!data.canConvert<DamageStruct>()) return skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage >= 2) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                if (p != player && p->getMark(objectName()) < 1)
                    skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        DamageStruct damage = data.value<DamageStruct>();
        //room->addPlayerMark(ask_who, objectName());
        room->recover(player, RecoverStruct(ask_who, NULL, damage.damage));
        damage.to = ask_who;
        room->damage(damage);
        return false;
    }
};

class ThMingling: public TriggerSkill {
public:
    ThMingling(): TriggerSkill("thmingling") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Normal) {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        if (damage.nature == DamageStruct::Fire)
            return true;
        else if (damage.nature == DamageStruct::Thunder)
            damage.damage++;
        data = QVariant::fromValue(damage);
        return false;
    }
};

ThChuanshangCard::ThChuanshangCard(){
}

bool ThChuanshangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && Self->inMyAttackRange(to_select);
}

void ThChuanshangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    targets.first()->gainMark("@nishui");
    room->loseHp(source);
}

class ThChuanshangViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThChuanshangViewAsSkill(): ZeroCardViewAsSkill("thchuanshang") {
    }

    virtual const Card *viewAs() const{
        return new ThChuanshangCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThChuanshangCard");
    }
};

class ThChuanshang: public TriggerSkill {
public:
    ThChuanshang(): TriggerSkill("thchuanshang") {
        events << EventPhaseStart;
        view_as_skill = new ThChuanshangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && target->getPhase() == Player::Finish && target->getMark("@nishui") > 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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
            player->loseAllMarks("@nishui");
        else if (judge.card->isBlack())
            player->gainMark("@nishui");

        return false;
    }
};

class ThChuanshangMaxCardsSkill: public MaxCardsSkill {
public:
    ThChuanshangMaxCardsSkill(): MaxCardsSkill("#thchuanshang") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->getMark("@nishui") > 0)
            return -target->getMark("@nishui");
        else
            return 0;
    }
};

ThLingdieCard::ThLingdieCard(){
}

bool ThLingdieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty();
}

void ThLingdieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    ServerPlayer *victim = room->askForPlayerChosen(target, room->getOtherPlayers(target), "thlingdie", QString(), true);
    if (victim)
        room->showAllCards(victim, target);
}

class ThLingdie: public OneCardViewAsSkill {
public:
    ThLingdie(): OneCardViewAsSkill("thlingdie") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThLingdieCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThLingdieCard *card = new ThLingdieCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThWushou: public TriggerSkill {
public:
    ThWushou(): TriggerSkill("thwushou") {
        events << Damaged;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && player->getHandcardNum() < 4;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(4 - player->getHandcardNum());
        return false;
    }
};

ThFuyueCard::ThFuyueCard() {
    m_skillName = "thfuyuev";
    mute = true;
}

bool ThFuyueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("thfuyue") && !to_select->isKongcheng() && to_select->isWounded()
           && to_select != Self && !to_select->hasFlag("ThFuyueInvoked");
}

void ThFuyueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    if (target->hasLordSkill("thfuyue")) {
        room->setPlayerFlag(target, "ThFuyueInvoked");
        room->broadcastSkillInvoke("thfuyue");
        room->notifySkillInvoked(target, "thfuyue");
        bool win = source->pindian(target, "thfuyue");
        if (!win) {
            RecoverStruct recover;
            recover.who = source;
            room->recover(target, recover);
        }
        QList<ServerPlayer *> lords;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasLordSkill("thfuyue") && !p->hasFlag("ThFuyueInvoked")){
                lords << p;
            }
        }
        if (lords.empty())
            room->setPlayerFlag(source, "ForbidThFuyue");
    }
}

class ThFuyueViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThFuyueViewAsSkill(): ZeroCardViewAsSkill("thfuyuev"){
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && player->getKingdom() == "yuki" && !player->hasFlag("ForbidThFuyue");
    }

    virtual const Card *viewAs() const{
        return new ThFuyueCard;
    }
};

class ThFuyue: public TriggerSkill{
public:
    ThFuyue(): TriggerSkill("thfuyue$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!player) return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "thfuyue")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

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
            if (lords.length() > 2) return QStringList();

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
            if (phase_change.from != Player::Play)
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

void TouhouPackage::addYukiGenerals(){
    General *yuki001 = new General(this, "yuki001$", "yuki");
    yuki001->addSkill(new ThJianmo);
    yuki001->addSkill(new ThErchong);
    yuki001->addSkill(new ThChundu);

    General *yuki002 = new General(this, "yuki002", "yuki", 3);
    yuki002->addSkill(new ThCuimeng);
    yuki002->addSkill(new ThCuimengMove);
    related_skills.insertMulti("thcuimeng", "#thcuimeng-move");
    yuki002->addSkill(new ThZuimeng);
    yuki002->addSkill(new ThMengwu);
    
    General *yuki003 = new General(this, "yuki003", "yuki");
    yuki003->addSkill(new ThCihang);
    
    General *yuki004 = new General(this, "yuki004", "yuki");
    yuki004->addSkill(new ThZhancao);
    
    General *yuki005 = new General(this, "yuki005", "yuki", 3, false);
    yuki005->addSkill(new ThYuanqi);
    yuki005->addSkill(new ThMoji);

    General *yuki006 = new General(this, "yuki006", "yuki");
    yuki006->addSkill(new ThJibu);
    yuki006->addSkill(new ThDunjia);
    yuki006->addSkill(new FakeMoveSkill("thdunjia"));
    related_skills.insertMulti("thdunjia", "#thdunjia-fake-move");

    General *yuki007 = new General(this, "yuki007", "yuki", 3);
    yuki007->addSkill(new ThChouce);
    yuki007->addSkill(new ThZhanshi);
    yuki007->addSkill(new ThZhanshiDo);
    related_skills.insertMulti("thzhanshi", "#thzhanshi");
    yuki007->addSkill(new ThHuanzang);

    General *yuki008 = new General(this, "yuki008", "yuki", 3);
    yuki008->addSkill(new ThZiyun);
    yuki008->addSkill(new ThChuiji);

    General *yuki009 = new General(this, "yuki009", "yuki");
    yuki009->addSkill(new ThLingya);
    yuki009->addSkill(new ThHeimu);

    General *yuki010 = new General(this, "yuki010", "yuki");
    yuki010->addSkill(new ThHanpo);
    yuki010->addSkill(new ThZhengguan);
    yuki010->addSkill(new ThBingpu);
    
    General *yuki011 = new General(this, "yuki011", "yuki", 3);
    yuki011->addSkill(new ThDongmo);
    yuki011->addSkill(new ThLinhan);

    General *yuki012 = new General(this, "yuki012", "yuki", 3);
    yuki012->addSkill(new ThFusheng);
    yuki012->addSkill(new ThHuanfa);

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

    addMetaObject<ThYuanqiCard>();
    addMetaObject<ThYuanqiGiveCard>();
    addMetaObject<ThChouceCard>();
    addMetaObject<ThBingpuCard>();
    addMetaObject<ThDongmoCard>();
    addMetaObject<ThHuanfaCard>();
    addMetaObject<ThKujieCard>();
    addMetaObject<ThChuanshangCard>();
    addMetaObject<ThLingdieCard>();
    addMetaObject<ThFuyueCard>();

    skills << new ThKujieViewAsSkill << new ThFuyueViewAsSkill;
}

