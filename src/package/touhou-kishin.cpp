#include "touhou-kishin.h"

#include "general.h"
#include "skill.h"
#include "room.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "standard.h"

ThLuanshenCard::ThLuanshenCard() {
    will_throw = false;
    handling_method = MethodNone;
}

void ThLuanshenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    foreach (int id, getSubcards())
        room->showCard(source, id);
    ServerPlayer *target = targets.first();
    QStringList choices;
    if (target->canDiscard(target, "he") && target->getCardCount() >= subcardsLength())
        choices << "discard";
    choices << "turnover";
    QString choice = room->askForChoice(target, "thluanshen", choices.join("+"));
    if (choice == "discard") {
        room->throwCard(this, source, target);
        room->askForDiscard(target, "thluanshen", subcardsLength(), subcardsLength(), false, true);
    } else {
        target->obtainCard(this);
        if (target->getHandcardNum() < target->getMaxHp())
            target->drawCards(target->getMaxHp() - target->getHandcardNum());
        target->turnOver();
    }
};

class ThLuanshen: public ViewAsSkill {
public:
    ThLuanshen(): ViewAsSkill("thluanshen") {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const {
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (cards.isEmpty())
            return NULL;
        ThLuanshenCard *card = new ThLuanshenCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThLuanshenCard") && player->getHandcardNum() >= player->getHp();
    }
};

class ThFeiman: public TriggerSkill{
public:
    ThFeiman(): TriggerSkill("thfeiman") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Play;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thfeiman", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThFeimanTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThFeimanTarget"].value<ServerPlayer *>();
        player->tag.remove("ThFeimanTarget");
        if (target) {
            room->damage(DamageStruct(objectName(), player, target));
            if (target->isDead()) return false;
            QList<ServerPlayer *> victims;
            foreach (ServerPlayer *p, room->getOtherPlayers(target))
                if (target->inMyAttackRange(p))
                    victims << p;
            QStringList choices;
            if (!victims.isEmpty())
                choices << "damage";

            QList<ServerPlayer *> to_gets;
            foreach (ServerPlayer *p, room->getAlivePlayers())
                if (!p->getCards("ej").isEmpty())
                    to_gets << p;
            if (!to_gets.isEmpty())
                choices << "obtain";

            if (choices.isEmpty())
                return false;

            QString choice = room->askForChoice(target, objectName(), choices.join("+"));
            if (choice == "damage") {
                ServerPlayer *victim = room->askForPlayerChosen(target, victims, objectName());
                room->damage(DamageStruct(objectName(), target, victim, 1, DamageStruct::Fire));
            } else if (choice == "obtain") {
                ServerPlayer *to_get = room->askForPlayerChosen(target, to_gets, objectName());
                int card_id = room->askForCardChosen(target, to_get, "ej", objectName());
                room->obtainCard(target, card_id);
            }
        }

        return false;
    }
};

class ThGuaiqi: public TriggerSkill{
public:
    ThGuaiqi(): TriggerSkill("thguaiqi") {
        events << EventPhaseChanging << EventPhaseEnd;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                p->setMark("thguaiqi", 0);
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play
                   && player->hasFlag("thguaiqi_invoke") && player->isAlive())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                skill_list.insert(owner, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
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

class ThGuaiqiRecord: public TriggerSkill {
public:
    ThGuaiqiRecord(): TriggerSkill("#thguaiqi-record") {
        events << PreDamageDone;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *owner = damage.from;
        if (owner && owner->getPhase() == Player::Play)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();
        player->addMark("thguaiqi", damage.damage);
        if (player->getMark("thguaiqi") > 1)
            damage.from->setFlags("thguaiqi_invoke");

        return false;
    }
};

class ThJingtao: public TriggerSkill {
public:
    ThJingtao(): TriggerSkill("thjingtao") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->drawCards(2, objectName());
        return false;
    }
};

class ThZongni: public TriggerSkill {
public:
    ThZongni(): TriggerSkill("thzongni") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        ServerPlayer *current = room->getCurrent();
        if (!TriggerSkill::triggerable(player) || current != player)
            return QStringList();
        if (player->getPhase() == Player::Draw)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->setFlags("thzongni");
        player->setPhase(Player::Play);
        room->broadcastProperty(player, "phase");
        return false;
    }
};

class ThZongniDiscard: public TriggerSkill {
public:
    ThZongniDiscard(): TriggerSkill("#thzongni") {
        events << EventPhaseEnd << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        if (player->getPhase() == Player::PhaseNone) {
            player->getRoom()->findPlayer(player->objectName())->setFlags("-thzongni");
            return false;
        }
        return player->hasFlag("thzongni");
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->setFlags("-thzongni");
        room->sendCompulsoryTriggerLog(player, "thzongni");
        room->askForDiscard(player, "thzongni", 998, 1, false, false, "@thzongni-discard");
        return false;
    }
};

ThLanzouCard::ThLanzouCard() {
}

bool ThLanzouCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const {
    Q_ASSERT(false);
    return false;
}

bool ThLanzouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select,
                                  const Player *Self, int &maxVotes) const {
    if (to_select == Self) return false;
    int n = qMin(Self->getEquips().length(), 4);
    int i = 0;
    foreach (const Player *player, targets)
        if (player == to_select) i++;
    maxVotes = qMax(n - targets.size(), 0) + i;
    return maxVotes > 0;
}

bool ThLanzouCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const {
    if (targets.size() == 1)
        return targets.first()->getCardCount() >= qMin(Self->getEquips().length(), 4);
    else {
        if (targets.size() != qMin(Self->getEquips().length(), 4)) return false;
        QMap<const Player *, int> map;
        foreach (const Player *p, targets)
            map[p]++;
        bool can = true;
        foreach (const Player *p, map.keys())
            if (map[p] > p->getCardCount()) {
                can = false;
                break;
            }
        return can;
    }
}

void ThLanzouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    int total = qMin(Self->getEquips().length(), 4);
    QMap<ServerPlayer *, int> map;

    foreach (ServerPlayer *sp, targets)
        map[sp]++;

    if (targets.size() == 1)
        map[targets.first()] = total;

    room->loseHp(source);
    QList<ServerPlayer *> victims = map.keys();
    room->sortByActionOrder(victims);
    foreach (ServerPlayer *sp, victims)
        discard(source, sp, map[sp]);
}

void ThLanzouCard::discard(ServerPlayer *source, ServerPlayer *target, int num) const {
    Room *room = source->getRoom();
    room->setPlayerFlag(target, "thlanzou_InTempMoving");
    DummyCard *dummy = new DummyCard;
    QList<int> card_ids;
    QList<Player::Place> original_places;
    for (int i = 0; i < num; i++) {
        if (!source->canDiscard(target, "he"))
            break;
        card_ids << room->askForCardChosen(source, target, "he", objectName(), false, Card::MethodDiscard);
        original_places << room->getCardPlace(card_ids[i]);
        dummy->addSubcard(card_ids[i]);
        source->addToPile("#thlanzou", card_ids[i], false);
    }
    for (int i = 0; i < dummy->subcardsLength(); i++)
        room->moveCardTo(Sanguosha->getCard(card_ids[i]), target, original_places[i], false);
    room->setPlayerFlag(target, "-thlanzou_InTempMoving");
    if (dummy->subcardsLength() > 0)
        room->throwCard(dummy, target, source);
    delete dummy;
}

class ThLanzouViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThLanzouViewAsSkill(): ZeroCardViewAsSkill("thlanzou") {
        response_pattern = "@@thlanzou";
    }

    virtual const Card *viewAs() const{
        return new ThLanzouCard;
    }
};

class ThLanzou: public TriggerSkill {
public:
    ThLanzou(): TriggerSkill("thlanzou") {
        events << EventPhaseEnd;
        view_as_skill = new ThLanzouViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        QVariantList cards = target->tag["ThLanzou"].toList();
        return !cards.isEmpty() && TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Discard;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        QVariantList cards = player->tag["ThLanzou"].toList();
        int x = cards.length();
        int n = player->getEquips().length();
        if (x >= n) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thlanzou", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThLanzouTarget"] = QVariant::fromValue(target);
                return true;
            } else
                player->tag.remove("ThLanzou");
        } else
            room->askForUseCard(player, "@@thlanzou", "@thlanzou", -1, Card::MethodNone);

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThLanzouTarget"].value<ServerPlayer *>();
        player->tag.remove("ThLanzouTarget");
        if (target) {
            QVariantList cards = player->tag["ThLanzou"].toList();
            DummyCard *dummy = new DummyCard;
            foreach (QVariant id, cards)
                dummy->addSubcard(id.toInt());
            if (dummy->subcardsLength() > 0) {
                target->obtainCard(dummy);
                room->loseHp(target);
            }
            delete dummy;
        }
        return false;
    }
};

class ThLanzouRecord: public TriggerSkill {
public:
    ThLanzouRecord(): TriggerSkill("#thlanzou-record") {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging)
            player->tag.remove("ThLanzou");
        else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player)
                return QStringList();

            if (player->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                QVariantList cards = player->tag["ThLanzou"].toList();
                foreach (int id, move.card_ids)
                    if (!cards.contains(id))
                        cards << id;
                player->tag["ThLanzou"] = cards;
            }
        }

        return QStringList();
    }
};

class ThXinqi: public TriggerSkill {
public:
    ThXinqi(): TriggerSkill("thxinqi") {
        events << TargetSpecifying;
        frequency = Compulsory;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isNDTrick() && use.card->isBlack())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player) continue;
                if (use.to.contains(owner))
                    skill_list.insert(owner, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        room->broadcastSkillInvoke(objectName());

        ask_who->drawCards(1);
        if (player->isKongcheng()) return false;
        if (room->askForChoice(player, objectName(), "show+cancel") == "show")
            room->showAllCards(player, ask_who);
        else {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << ask_who->objectName();
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class ThNengwu: public TriggerSkill {
public:
    ThNengwu(): TriggerSkill("thnengwu") {
        events << EventPhaseStart;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() == Player::Start && !player->isKongcheng())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player || owner->isKongcheng()) continue;
                skill_list.insert(owner, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        const Card *card = room->askForCardShow(player, ask_who, objectName());
        if (!card || card->getEffectiveId() == -1)
            card = player->getRandomHandCard();

        QString choice = room->askForChoice(ask_who, objectName(), "basic+equip+trick");

        LogMessage log;
        log.type = "#ThNengwu";
        log.from = ask_who;
        log.arg = choice;
        room->sendLog(log);

        int id = card->getId();
        room->showCard(player, id);
        if (card->getType() == choice)
            room->loseHp(player);
        else if (ask_who->canDiscard(ask_who, "he"))
            room->askForDiscard(ask_who, objectName(), 1, 1, false, true);

        if (player->isDead()) return false;

        QVariantList ids = player->tag["ThNengwuId"].toList();
        ids << id;
        player->tag["ThNengwuId"] = QVariant::fromValue(ids);
        QStringList sources = player->tag["ThNengwuSource"].toStringList();
        sources << ask_who->objectName();
        player->tag["ThNengwuSource"] = QVariant::fromValue(sources);
        room->setPlayerCardLimitation(player, "use", "^" + QString::number(id), false);

        return false;
    }
};

class ThNengwuClear: public TriggerSkill {
public:
    ThNengwuClear(): TriggerSkill("#thnengwu-clear") {
        events << EventPhaseChanging << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from || !move.from_places.contains(Player::PlaceHand)) return QStringList();
            QVariantList ids = player->tag["ThNengwuId"].toList();
            if (ids.isEmpty() || move.from != player) return QStringList();
            QStringList sources = player->tag["ThNengwuSource"].toStringList();
            int reason = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
            if (reason == CardMoveReason::S_REASON_USE
                || reason == CardMoveReason::S_REASON_RESPONSE
                || reason == CardMoveReason::S_REASON_DISCARD) {
                    foreach (int id, move.card_ids) {
                        int index = move.card_ids.indexOf(id);
                        if (ids.contains(id) && move.from_places.at(index) == Player::PlaceHand) {
                            while (ids.contains(id)) {
                                int i = ids.indexOf(id);
                                if (i != -1) {
                                    ids.removeAt(i);
                                    sources.removeAt(i);
                                    room->removePlayerCardLimitation(player, "use", "^" + QString::number(id) + "$0");
                                }
                            }
                            player->tag["ThNengwuId"] = QVariant::fromValue(ids);
                            player->tag["ThNengwuSource"] = QVariant::fromValue(sources);
                        }
                    }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && !player->tag["ThNengwuId"].toList().isEmpty()) {
                QVariantList ids = player->tag["ThNengwuId"].toList();
                player->tag.remove("ThNengwuId");
                player->tag.remove("ThNengwuSource");
                while (!ids.isEmpty()) {
                    int id = ids.takeFirst().toInt();
                    room->removePlayerCardLimitation(player, "use", "^" + QString::number(id) + "$0");
                }
            }
        }
        return QStringList();
    }
};

class ThBaochui: public TriggerSkill {
public:
    ThBaochui(): TriggerSkill("thbaochui") {
        events << EventPhaseEnd;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() == Player::Draw && !player->isKongcheng())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player) continue;
                skill_list.insert(owner, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        const Card *card = room->askForCard(player, ".!", "@thbaochui:" + ask_who->objectName(), data, Card::MethodNone);
        if (!card)
            card = player->getRandomHandCard();
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(),
                              ask_who->objectName(), objectName(), QString());
        room->obtainCard(ask_who, card, reason, false);
        ask_who->addToPile("thbaochuipile", ask_who->handCards());
        room->setPlayerFlag(player, "thbaochui");
        return false;
    }
};

class ThBaochuiReturn: public TriggerSkill {
public:
    ThBaochuiReturn(): TriggerSkill("#thbaochui-return") {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->hasFlag("thbaochui") && player->getPhase() == Player::Play)
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getPile("thbaochuipile").isEmpty()) continue;
                skill_list.insert(p, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        DummyCard *dummy = new DummyCard(ask_who->getPile("thbaochuipile"));
        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, ask_who->objectName(), "thbaochui", QString());
        room->obtainCard(ask_who, dummy, reason, true);
        delete dummy;
        ask_who->drawCards(1);
        return false;
    }
};

class ThBaochuiRecord: public TriggerSkill {
public:
    ThBaochuiRecord(): TriggerSkill("thbaochui_record") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() != Player::Play)
            return QStringList();
        int reason = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
        if (reason == CardMoveReason::S_REASON_USE
            || reason == CardMoveReason::S_REASON_RESPONSE) {
            for (int i = 0; i < move.card_ids.size(); i++)
                if (move.from_pile_names[i] == "thbaochuipile")
                    return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        int count = 0;
        for (int i = 0; i < move.card_ids.size(); i++)
            if (move.from_pile_names[i] == "thbaochuipile")
                count++;
        if (count > 0) {
            LogMessage log;
            log.type = "#WoodenOx";
            log.from = player;
            log.arg = QString::number(count);
            log.arg2 = "thbaochui";
            room->sendLog(log);
        }
        return false;
    }
};

class ThYishi: public TriggerSkill {
public:
    ThYishi(): TriggerSkill("thyishi") {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (TriggerSkill::triggerable(player) && effect.card->hasFlag("thbaochui+" + player->objectName()))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        CardEffectStruct effect = data.value<CardEffectStruct>();
        effect.nullified = true;
        data = QVariant::fromValue(effect);
        return false;
    }
};

class ThYishiNullified: public TriggerSkill {
public:
    ThYishiNullified(): TriggerSkill("#thyishi") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        int reason = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
        if (reason == CardMoveReason::S_REASON_USE && move.from_places.contains(Player::PlaceSpecial)
                                                   && move.from_pile_names.contains("thbaochuipile")) {
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if (!card) return QStringList();
            if (!card->isVirtualCard() || (card->subcardsLength() == 1
                                           && card->getSubcards() == move.card_ids
                                           && card->getClassName() == Sanguosha->getCard(move.card_ids.first())->getClassName())) {
                card->setFlags("thbaochui+" + move.from->objectName());
                move.reason.m_extraData = QVariant::fromValue(card);
                data = QVariant::fromValue(move);
            }
        }
        return QStringList();
    }
};

class ThMoju: public TriggerSkill {
public:
    ThMoju(): TriggerSkill("thmoju") {
        events << DrawNCards << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == DrawNCards && player->getWeapon() && TriggerSkill::triggerable(player))
            return QStringList(objectName());
        else if (triggerEvent == TargetSpecified && player->getArmor() && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && player->getPhase() == Player::Play)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        if (triggerEvent == DrawNCards) {
            int n = qMax((qobject_cast<const Weapon *>(player->getWeapon()->getRealCard()))->getRange(), 2);
            data = n;
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
                index ++;
            }
            player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        }
        return false;
    }
};

class ThMojuMaxCardsSkill: public MaxCardsSkill {
public:
    ThMojuMaxCardsSkill(): MaxCardsSkill("#thmoju") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill("thmoju")) {
            int horses = 0;
            if (target->getOffensiveHorse())
                horses++;
            if (target->getDefensiveHorse())
                horses++;
            return horses;
        } else
            return 0;
    }
};

ThLianyingCard::ThLianyingCard() {
    target_fixed = true;
}

void ThLianyingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const {
    source->setFlags("thlianying");
    room->handleAcquireDetachSkills(source, "ikchilian|thjibu");
}

class ThLianyingViewAsSkill: public ViewAsSkill {
public:
    ThLianyingViewAsSkill(): ViewAsSkill("thlianying") {
    }

    virtual bool viewFilter(const QList<const Card *> &cards, const Card *to_select) const {
        return cards.size() < 2 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (cards.size() != 2)
            return NULL;
        ThLianyingCard *card = new ThLianyingCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThLianyingCard") && player->getCardCount() > 1;
    }
};

class ThLianying: public TriggerSkill {
public:
    ThLianying(): TriggerSkill("thlianying") {
        events << EventPhaseChanging;
        view_as_skill = new ThLianyingViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive && player->hasFlag(objectName())) {
            player->setFlags("-" + objectName());
            room->handleAcquireDetachSkills(player, "-ikchilian|-thjibu", true);
        }
        return QStringList();
    }
};

class ThYuanxiao: public TriggerSkill {
public:
    ThYuanxiao(): TriggerSkill("thyuanxiao") {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
            foreach (ServerPlayer *to, use.to)
                if (to->getHandcardNum() > player->getHandcardNum())
                    return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *to, use.to) {
            if (to->getHandcardNum() > player->getHandcardNum())
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(to))) {
                    room->broadcastSkillInvoke(objectName());

                    int card_id = room->askForCardChosen(player, to, "h", objectName());
                    room->obtainCard(player, card_id, true);
                    if (Sanguosha->getCard(card_id)->isRed()) {
                        LogMessage log;
                        log.type = "#NoJink";
                        log.from = to;
                        room->sendLog(log);
                        jink_list.replace(index, QVariant(0));
                    }
                }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

TouhouKishinPackage::TouhouKishinPackage()
    :Package("touhou-kishin")
{
    General *kishin001 = new General(this, "kishin001", "kaze");
    kishin001->addSkill(new ThLuanshen);

    General *kishin002 = new General(this, "kishin002", "hana");
    kishin002->addSkill(new ThFeiman);
    kishin002->addSkill(new ThGuaiqi);
    kishin002->addSkill(new ThGuaiqiRecord);
    related_skills.insertMulti("thguaiqi", "#thguaiqi-record");

    General *kishin003 = new General(this, "kishin003", "yuki");
    kishin003->addSkill(new ThJingtao);
    kishin003->addSkill(new ThZongni);
    kishin003->addSkill(new ThZongniDiscard);
    related_skills.insertMulti("thzongni", "#thzongni");

    General *kishin004 = new General(this, "kishin004", "tsuki");
    kishin004->addSkill(new ThLanzou);
    kishin004->addSkill(new ThLanzouRecord);
    kishin004->addSkill(new FakeMoveSkill("thlanzou"));
    related_skills.insertMulti("thlanzou", "#thlanzou-fake-move");

    General *kishin005 = new General(this, "kishin005", "kaze", 3, false);
    kishin005->addSkill(new ThXinqi);
    kishin005->addSkill(new ThNengwu);
    kishin005->addSkill(new ThNengwuClear);
    related_skills.insertMulti("thnengwu", "#thnengwu-clear");

    General *kishin006 = new General(this, "kishin006", "hana", 3);
    kishin006->addSkill(new ThBaochui);
    kishin006->addSkill(new ThBaochuiReturn);
    related_skills.insertMulti("thbaochui", "#thbaochui-return");
    kishin006->addSkill(new ThYishi);
    kishin006->addSkill(new ThYishiNullified);
    related_skills.insertMulti("thyishi", "#thyishi");

    General *kishin007 = new General(this, "kishin007", "yuki");
    kishin007->addSkill(new ThMoju);
    kishin007->addSkill(new ThMojuMaxCardsSkill);
    related_skills.insertMulti("thmoju", "#thmoju");

    General *kishin008 = new General(this, "kishin008", "tsuki");
    kishin008->addSkill(new ThLianying);
    kishin008->addSkill(new ThYuanxiao);

    addMetaObject<ThLuanshenCard>();
    addMetaObject<ThLanzouCard>();
    addMetaObject<ThLianyingCard>();

    skills << new ThBaochuiRecord;
}

ADD_PACKAGE(TouhouKishin)