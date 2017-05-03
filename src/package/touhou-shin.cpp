#include "touhou-shin.h"

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
    QString choice = room->askForChoice(target, "thluanshen", choices.join("+"), QVariant::fromValue(IntList2VariantList(subcards)));
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
                ServerPlayer *victim = room->askForPlayerChosen(target, victims, "thfeiman-damage");
                room->damage(DamageStruct(objectName(), target, victim, 1, DamageStruct::Fire));
            } else if (choice == "obtain") {
                ServerPlayer *to_get = room->askForPlayerChosen(target, to_gets, "thfeiman-obtain");
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

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        TriggerList skill_list;
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
            ServerPlayer *splayer = player->getRoom()->findPlayer(player->objectName());
            if (splayer)
                splayer->setFlags("-thzongni");
            return false;
        }
        return player->hasFlag("thzongni");
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->setFlags("-thzongni");
        room->sendCompulsoryTriggerLog(player, "thzongni");
        if (player->canDiscard(player, "h"))
            room->askForDiscard(player, "thzongni", 998, 1, false, false, "@thzongni-discard");
        return false;
    }
};

class ThLanzou: public TriggerSkill {
public:
    ThLanzou(): TriggerSkill("thlanzou") {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                int index = 0;
                foreach (int id, move.card_ids) {
                    if (move.from_places[index] != Player::PlaceHand
                        && move.from_places[index] != Player::PlaceEquip) {
                        ++index;
                        continue;
                    }
                    const Card *card = Sanguosha->getCard(id);
                    if ((card->getTypeId() == Card::TypeBasic && !card->isKindOf("Slash")) || card->isKindOf("Nullification"))
                        return QStringList(objectName());
                    ++index;
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thlanzou", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThLanzouTarget"] = QVariant::fromValue(target);
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThLanzouTarget"].value<ServerPlayer *>();
        player->tag.remove("ThLanzouTarget");
        if (target)
            target->drawCards(1, objectName());

        return false;
    }
};

class ThLanzouSecond: public TriggerSkill {
public:
    ThLanzouSecond(): TriggerSkill("#thlanzou") {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            int index = 0;
            foreach (int id, move.card_ids) {
                if (move.from_places[index] != Player::PlaceHand
                    && move.from_places[index] != Player::PlaceEquip) {
                    ++index;
                    continue;
                }
                const Card *card = Sanguosha->getCard(id);
                if ((card->getTypeId() == Card::TypeBasic && !card->isKindOf("Slash")) || card->isKindOf("Nullification")) {
                    QStringList skill_list;
                    foreach (ServerPlayer *p, room->findPlayersBySkillName("thlanzou")) {
                        if (p == player)
                            continue;
                        skill_list << p->objectName() + "'" + objectName();
                    }
                    return skill_list;
                }
                ++index;
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *skill_invoker) const{
        if (skill_invoker->askForSkillInvoke("thlanzou", QVariant::fromValue(skill_target))) {
            room->broadcastSkillInvoke("thlanzou");
            room->notifySkillInvoked(skill_target, "thlanzou");
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = skill_invoker;
            log.to << skill_target;
            log.arg = "thlanzou";
            room->sendLog(log);

            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *skill_target, QVariant &, ServerPlayer *) const {
        skill_target->drawCards(1, "thlanzou");

        return false;
    }
};

class ThXinqi: public TriggerSkill {
public:
    ThXinqi(): TriggerSkill("thxinqi") {
        events << TargetSpecifying;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
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
        player->tag["ThXinqiTarget"] = QVariant::fromValue(ask_who);
        QString choice = room->askForChoice(player, objectName(), "show+cancel", data);
        player->tag.remove("ThXinqiTarget");
        if (choice == "show") {
            LogMessage log;
            log.type = "$IkLingtongView";
            log.from = ask_who;
            log.to << player;
            log.arg = "iklingtong:handcards";
            room->sendLog(log, room->getOtherPlayers(ask_who));

            room->showAllCards(player, ask_who);
        } else {
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

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
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

        QString choice = room->askForChoice(ask_who, objectName(), "basic+equip+trick", QVariant::fromValue(player));

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
        events << EventPhaseChanging << CardsMoveOneTime << Death;
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
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player) {
                QVariantList ids = player->tag["ThNengwuId"].toList();
                if (!ids.isEmpty()) {
                    foreach (QVariant id, ids)
                        room->removePlayerCardLimitation(player, "use", "^" + QString::number(id.toInt()) + "$0");
                    player->tag.remove("ThNengwuId");
                    player->tag.remove("ThNengwuSource");
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
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
        if (player->getPhase() == Player::Play && !player->isKongcheng()) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player || owner->isKongcheng())
                    continue;
                skill_list.insert(owner, QStringList(objectName()));
            }
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
        ask_who->addToPile("currency", ask_who->handCards());
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

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
        if (player->hasFlag("thbaochui") && player->getPhase() == Player::Play)
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getPile("currency").isEmpty()) continue;
                skill_list.insert(p, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        DummyCard *dummy = new DummyCard(ask_who->getPile("currency"));
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
                if (move.from_pile_names[i] == "currency")
                    return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        int count = 0;
        for (int i = 0; i < move.card_ids.size(); i++)
            if (move.from_pile_names[i] == "currency")
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
                                                   && move.from_pile_names.contains("currency")) {
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

class ThWuyi : public TriggerSkill
{
public:
    ThWuyi() : TriggerSkill("thwuyi")
    {
        events << EventPhaseStart << CardsMoveOneTime << EventPhaseChanging << TargetSpecified;
    }

    virtual TriggerList triggerable(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &d) const
    {
        TriggerList tl;
        if (e == EventPhaseChanging) {
            if (d.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *sp, r->getAlivePlayers()) {
                    r->setPlayerFlag(sp, "-ThWuyiBasic");
                    r->setPlayerFlag(sp, "-ThWuyiTrick");
                    r->setPlayerFlag(sp, "-ThWuyiEquip");
                }
            }
        } else if (e == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from->isAlive() && move.from == p
                    && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                    && move.to_place == Player::DiscardPile) {
                for (int i = 0; i < move.card_ids.length(); ++i) {
                    if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip) {
                        const Card *c = Sanguosha->getCard(move.card_ids[i]);
                        if (c->getTypeId() == Card::TypeBasic)
                            r->setPlayerFlag(p, "ThWuyiBasic");
                        else if (c->getTypeId() == Card::TypeEquip)
                            r->setPlayerFlag(p, "ThWuyiEquip");
                        else if (c->getTypeId() == Card::TypeTrick)
                            r->setPlayerFlag(p, "ThWuyiTrick");
                    }
                }
            }
        } else if (e == TargetSpecified) {
            CardUseStruct use = d.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->getSkillName() == objectName()
                    && p->hasFlag("ThWuyiEquip"))
                tl.insert(p, QStringList(objectName()));
        } else if (e == EventPhaseStart) {
            if (p->getPhase() == Player::Finish) {
                foreach (ServerPlayer *owner, r->findPlayersBySkillName(objectName())) {
                    if (owner->hasFlag("ThWuyiBasic")) {
                        foreach (ServerPlayer *sp, r->getOtherPlayers(owner)) {
                            if (owner->canSlash(sp, false))
                                tl.insert(owner, QStringList(objectName()));
                        }
                    }
                }
            }
        }
        return tl;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        if (triggerEvent == TargetSpecified)
            return true;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(ask_who)) {
            if (ask_who->canSlash(p, false))
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(ask_who, targets, objectName(), "@thwuyi", true);
        if (target) {
            ask_who->tag["ThWuyiTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        if (triggerEvent == TargetSpecified) {
            room->sendCompulsoryTriggerLog(ask_who, objectName());
            room->broadcastSkillInvoke(objectName());
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to.toSet())
                p->addQinggangTag(use.card);
        } else {
            ServerPlayer *target = ask_who->tag["ThWuyiTarget"].value<ServerPlayer *>();
            ask_who->tag.remove("ThWuyiTarget");
            if (target) {
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName(objectName());
                room->useCard(CardUseStruct(slash, ask_who, target));
            }
        }
        return false;
    }
};

class ThWuyiTargetMod : public TargetModSkill
{
public:
    ThWuyiTargetMod() : TargetModSkill("#thwuyi-tar")
    {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const
    {
        if (from->hasFlag("ThWuyiTrick") && card->getSkillName() == "thwuyi")
            return 1;
        return 0;
    }
};

ThMumiCard::ThMumiCard()
{
    target_fixed = true;
}

class ThMumiVS : public ViewAsSkill
{
public:
    ThMumiVS(): ViewAsSkill("thmumi")
    {
        response_pattern = "@@thmumi";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        ThMumiCard *card = new ThMumiCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThMumi : public TriggerSkill
{
public:
    ThMumi(): TriggerSkill("thmumi")
    {
        events << CardAsked;
        view_as_skill = new ThMumiVS;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player) || !player->canDiscard(player, "he")) return QStringList();
        QString asked = data.toStringList().first();
        if (asked == "jink") return QStringList(objectName());

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thmumi", "@thmumi", -1, Card::MethodDiscard);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        Jink *jink = new Jink(Card::NoSuit, 0);
        jink->setSkillName("_thmumi");
        room->provide(jink);
        return true;
    }
};

class ThWangyu : public TriggerSkill
{
public:
    ThWangyu() : TriggerSkill("thwangyu")
    {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && room->getCardPlace(damage.card->getEffectiveId()) == Player::PlaceTable) {
            if (TriggerSkill::triggerable(player)) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getHandcardNum() <= p->getMaxHp())
                        return QStringList(objectName());
                }
            } else {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->getHandcardNum() <= p->getMaxHp())
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets;
        if (TriggerSkill::triggerable(player)) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getHandcardNum() <= p->getMaxHp())
                    targets << p;
            }
        } else {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->getHandcardNum() <= p->getMaxHp())
                    targets << p;
            }
        }
        if (!targets.isEmpty()) {
            player->tag["ThWangyuSlash"] = data;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thwangyu", true, player->hasSkill(objectName()));
            player->tag.remove("ThWangyuSlash");
            if (target) {
                room->broadcastSkillInvoke(objectName());
                if (!player->hasSkill(objectName())) {
                    room->notifySkillInvoked(target, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = player;
                    log.to << target;
                    log.arg = objectName();
                    room->sendLog(log);
                }
                player->tag["ThWangyuTarget"] = QVariant::fromValue(target);
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThWangyuTarget"].value<ServerPlayer *>();
        player->tag.remove("ThWangyuTarget");
        if (target) {
            DamageStruct damage = data.value<DamageStruct>();
            target->obtainCard(damage.card);
        }
        return false;
    }
};

class ThGuangshi : public TriggerSkill
{
public:
    ThGuangshi() : TriggerSkill("thguangshi")
    {
        events << Damaged;
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
        if (!player->canDiscard(player, "he") || !room->askForCard(player, "..", "@thguangshi", data))
            player->drawCards(1, objectName());
        if (!player->isKongcheng()) {
            room->showAllCards(player);
            int red = 0, black = 0;
            foreach (const Card *c, player->getHandcards()) {
                if (c->isRed())
                    ++red;
                else if (c->isBlack())
                    ++black;
            }
            if (red > black)
                player->drawCards(1, objectName());
            else if (black > red) {
                DamageStruct damage = data.value<DamageStruct>();
                if (damage.from && player->canDiscard(damage.from, "he")) {
                    int id = room->askForCardChosen(player, damage.from, "he", objectName(), false, Card::MethodDiscard);
                    room->throwCard(id, damage.from, player);
                }
            }
        }
        return false;
    }
};

class ThSunwu : public TriggerSkill
{
public:
    ThSunwu() : TriggerSkill("thsunwu")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer* &) const
    {
        if (TriggerSkill::triggerable(p) && p->getPile("frost").isEmpty()) {
            CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                foreach (int id, move.card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    if (r->getCardPlace(id) == Player::DiscardPile && (c->isKindOf("Weapon") || c->isKindOf("Armor")))
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        if (p->askForSkillInvoke(objectName())) {
            r->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
        QList<int> equips, not_equips;
        foreach (int id, move.card_ids) {
            const Card *c = Sanguosha->getCard(id);
            if (r->getCardPlace(id) == Player::DiscardPile && (c->isKindOf("Weapon") || c->isKindOf("Armor")))
                equips << id;
            else
                not_equips << id;
        }
        if (!equips.isEmpty()) {
            if (equips.length() == 1) {
                p->addToPile("frost", equips);
            } else {
                r->fillAG(move.card_ids, NULL, not_equips);
                int id = r->askForAG(p, equips, false, objectName());
                r->clearAG();
                p->addToPile("frost", id);
            }
        }
        return false;
    }
};

ThLiaoganCard::ThLiaoganCard()
{
}

bool ThLiaoganCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void ThLiaoganCard::onEffect(const CardEffectStruct &effect) const
{
    DummyCard *dummy = new DummyCard(effect.from->getPile("frost"));
    effect.to->obtainCard(dummy);
    delete dummy;
    effect.from->addMark("thliaogan_" + effect.to->objectName());
    QStringList list;
    if (!effect.to->hasSkill("thxiagong"))
        list << "thxiagong";
    if (!effect.to->hasSkill("ikjingnie"))
        list << "ikjingnie";
    if (!list.isEmpty())
        effect.to->getRoom()->handleAcquireDetachSkills(effect.to, list, true, true);
}

class ThLiaoganVS : public ZeroCardViewAsSkill
{
public:
    ThLiaoganVS() : ZeroCardViewAsSkill("thliaogan")
    {
    }

    virtual const Card *viewAs() const
    {
        return new ThLiaoganCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("frost").isEmpty();
    }
};

class ThLiaogan : public TriggerSkill
{
public:
    ThLiaogan() : TriggerSkill("thliaogan")
    {
        events << EventPhaseStart << Death;
        view_as_skill = new ThLiaoganVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        }
        if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) || triggerEvent == Death) {
            QList<ServerPlayer *> players = room->getAllPlayers();
            foreach (ServerPlayer *p, players) {
                if (player->getMark("thliaogan_" + p->objectName())) {
                    player->setMark("thliaogan_" + p->objectName(), 0);
                    room->handleAcquireDetachSkills(p, "-thxiagong|-ikjingnie", true, true);
                }
            }
        }

        return QStringList();
    }
};

class ThJianyueVS : public ZeroCardViewAsSkill
{
public:
    ThJianyueVS() : ZeroCardViewAsSkill("thjianyue")
    {
        response_pattern = "@@thjianyue";
    }

    virtual const Card *viewAs() const
    {
        Dismantlement *d = new Dismantlement(Card::NoSuit, 0);
        d->setSkillName(objectName());
        return d;
    }
};

class ThJianyue : public TriggerSkill
{
public:
    ThJianyue() : TriggerSkill("thjianyue")
    {
        events << EventPhaseStart << EventPhaseChanging;
        // Dismantlement::onEffect
        view_as_skill = new ThJianyueVS;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (e == EventPhaseChanging) {
            if (d.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *sp, r->getAlivePlayers()) {
                    QVariantList list = sp->tag["ThJianyue"].toList();
                    if (!list.isEmpty()) {
                        foreach (QVariant data, list) {
                            bool ok = false;
                            int id = data.toInt(&ok);
                            if (ok)
                                r->removePlayerCardLimitation(sp, "use,response", QString("%1$0").arg(id));
                        }
                        sp->tag.remove("ThJianyue");
                    }
                }
            }
        } else {
            if (TriggerSkill::triggerable(p) && (p->getPhase() == Player::Draw || p->getPhase() == Player::Play))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thjianyue", "@thjianyue");
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->hasFlag("ThJianyueSkip")) {
            room->setPlayerFlag(player, "-ThJianyueSkip");
            return true;
        }
        return false;
    }
};

class ThHuanjian : public TriggerSkill
{
public:
    ThHuanjian() : TriggerSkill("thhuanjian")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList list;
        if (player->getPhase() == Player::RoundStart && !player->isKongcheng()) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner != player && !owner->isKongcheng())
                    list.insert(owner, QStringList(objectName()));
            }
        }
        return list;
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
        const Card *card1 = room->askForCard(player, ".!", "@thhuanjian-ask:" + ask_who->objectName(), QVariant(), Card::MethodNone);
        if (!card1)
            card1 = player->getRandomHandCard();
        const Card *card2 = room->askForCard(ask_who, ".!", "@thhuanjian-self", QVariant(), Card::MethodNone);
        if (!card2)
            card2 = ask_who->getRandomHandCard();
        ask_who->addToPile("note", card1);
        ask_who->addToPile("note", card2);
        return false;
    }
};

class ThHuanjianReturn : public TriggerSkill
{
public:
    ThHuanjianReturn() : TriggerSkill("#thhuanjian")
    {
        events << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p != player && !p->getPile("note").isEmpty())
                    list.insert(p, QStringList(objectName()));
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, "thhuanjian");
        room->broadcastSkillInvoke("thhuanjian");
        QList<int> card_ids = ask_who->getPile("note");
        room->fillAG(card_ids);
        int card_id = room->askForAG(ask_who, card_ids, false, "thhuanjian");
        card_ids.removeOne(card_id);
        room->takeAG(ask_who, card_id, false);
        room->obtainCard(ask_who, card_id);
        if (!card_ids.isEmpty()) {
            card_id = room->askForAG(player, card_ids, false, "thhuanjian");
            room->takeAG(player, card_id, false);
            room->obtainCard(player, card_id);
        }
        room->clearAG();
        return false;
    }
};

class ThShenmi : public ViewAsSkill
{
public:
    ThShenmi() : ViewAsSkill("thshenmi")
    {
        expand_pile = "note";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (!Self->getPile("note").contains(to_select->getId()))
            return false;
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "jink")
            return selected.isEmpty() || to_select->sameColorWith(selected.first());
        if (pattern == "nullification")
            return selected.isEmpty() || !to_select->sameColorWith(selected.first());
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            if (cards.first()->sameColorWith(cards.last())) {
                Jink *jink = new Jink(Card::SuitToBeDecided, -1);
                jink->addSubcards(cards);
                jink->setSkillName(objectName());
                return jink;
            } else {
                Nullification *null = new Nullification(Card::SuitToBeDecided, -1);
                null->addSubcards(cards);
                null->setSkillName(objectName());
                return null;
            }
        }
        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        QList<int> ids = player->getPile("note");
        if (!ids.isEmpty()) {
            const Card *first = Sanguosha->getCard(ids.takeFirst());
            if (!ids.isEmpty()) {
                foreach (int id, ids) {
                    if (first->sameColorWith(Sanguosha->getCard(id))) {
                        if (pattern == "jink")
                            return true;
                        continue;
                    } else {
                        if (pattern == "nullification")
                            return true;
                        continue;
                    }
                }
            }
        }
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        QList<int> ids = player->getPile("note");
        if (!ids.isEmpty()) {
            const Card *first = Sanguosha->getCard(ids.takeFirst());
            if (!ids.isEmpty()) {
                foreach (int id, ids) {
                    if (!first->sameColorWith(Sanguosha->getCard(id)))
                        return true;
                }
            }
        }
        return false;
    }
};

ThMuyuCard::ThMuyuCard() {
}

bool ThMuyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isNude() && to_select != Self;
}

void ThMuyuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    bool can_discard = effect.to->canDiscard(effect.to, "he") && effect.to->getCardCount() > 1;
    QString pattern = "..";
    if (!can_discard)
        pattern += "!";
    const Card *card = room->askForCard(effect.to, pattern, "@thmuyu-put:" + effect.from->objectName(), QVariant(), MethodNone);
    if (!card && !can_discard) {
        QList<const Card *> cards = effect.to->getCards("he");
        card = cards.at(qrand() % cards.length());
    }
    if (card)
        effect.from->addToPile("prison", card);
    else if (can_discard)
        room->askForDiscard(effect.to, "thmuyu", 2, 2, false, true);
}

class ThMuyuViewAsSkill: public OneCardViewAsSkill {
public:
    ThMuyuViewAsSkill(): OneCardViewAsSkill("thmuyu") {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *card) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            if (Self->isJilei(card))
                return false;
            if (Self->getPile("wooden_ox").contains(card->getEffectiveId()))
                return false;
            if (Self->getPile("pokemon").contains(card->getEffectiveId()))
                return false;
            if (Self->hasFlag("thbaochui") && Self->getPhase() == Player::Play) {
                foreach (const Player *p, Self->getAliveSiblings()) {
                    if (p->getPile("currency").contains(card->getEffectiveId()))
                        return false;
                }
            }
            return true;
        }
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "jink")
            return card->isRed();
        else if (pattern == "nullification")
            return card->isBlack();
        return false;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("ThMuyuCard");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getPile("prison").isEmpty())
            return false;
        return  pattern == "jink" || pattern == "nullification";
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return !player->getPile("prison").isEmpty();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            ThMuyuCard *first = new ThMuyuCard;
            first->addSubcard(originalCard);
            return first;
        }
        if (originalCard->isRed()) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        }
        if (originalCard->isBlack()) {
            Nullification *nullification = new Nullification(originalCard->getSuit(), originalCard->getNumber());
            nullification->addSubcard(originalCard);
            nullification->setSkillName(objectName());
            return nullification;
        }
        return NULL;
    }
};

class ThMuyu: public PhaseChangeSkill {
public:
    ThMuyu(): PhaseChangeSkill("thmuyu") {
        view_as_skill = new ThMuyuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && !target->getPile("prison").isEmpty();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->sendCompulsoryTriggerLog(player, objectName());
        DummyCard *dummy = new DummyCard(player->getPile("prison"));
        player->obtainCard(dummy);
        delete dummy;
        return false;
    }
};

ThNihuiCard::ThNihuiCard()
{
    target_fixed = true;
}

void ThNihuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(3, "thnihui");
    room->askForDiscard(source, "thnihui", 1, 1, false, true);
    room->setPlayerMark(source, "thnihui", 1);
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

class ThNihui : public ZeroCardViewAsSkill
{
public:
    ThNihui() : ZeroCardViewAsSkill("thnihui")
    {
    }

    virtual const Card *viewAs() const {
        return new ThNihuiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThNihuiCard") && !player->hasUsed("ThNihuiEditCard");
    }
};

ThNihuiEditCard::ThNihuiEditCard()
{
    m_skillName = "thnihui-edit";
    target_fixed = true;
}

void ThNihuiEditCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(1, "thnihui");
    room->setPlayerMark(source, "thnihui", 0);
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

class ThNihuiEdit : public ViewAsSkill
{
public:
    ThNihuiEdit() : ViewAsSkill("thnihui-edit")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 3 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 3) {
            Card *card = new ThNihuiEditCard;
            card->addSubcards(cards);
            return card;
        }
        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThNihuiCard") && !player->hasUsed("ThNihuiEditCard");
    }
};

class ThNihuiInvalidity: public InvaliditySkill {
public:
    ThNihuiInvalidity(): InvaliditySkill("#thnihui-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        if (player->getMark("thnihui") != 0)
            return skill->objectName() != "thnihui";
        else
            return skill->objectName() != "thnihui-edit";
    }
};

class ThTanguan : public TriggerSkill
{
public:
    ThTanguan() : TriggerSkill("thtanguan")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->isKongcheng())
                return TriggerSkill::triggerable(player) && !player->isKongcheng()
                        && player->getPhase() == Player::Finish;
        }
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isKongcheng())
                targets << p;
        }
        if (!targets.isEmpty()) {
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thtanguan", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThTanguan"] = QVariant::fromValue(target);
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThTanguan"].value<ServerPlayer *>();
        player->tag.remove("ThTanguan");
        if (target)
            player->pindian(target, "thtanguan");
        return false;
    }
};

class ThTanguanTrigger : public TriggerSkill
{
public:
    ThTanguanTrigger() : TriggerSkill("#thtanguan")
    {
        events << Pindian;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason == "thtanguan") {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        ServerPlayer *target = pindian->to;
        const Card *card1 = pindian->from_card;
        const Card *card2 = pindian->to_card;
        QList<CardsMoveStruct> moves;
        CardMoveReason reason1(CardMoveReason::S_REASON_GOTBACK, target->objectName(), "thtanguan", QString());
        CardMoveReason reason2(CardMoveReason::S_REASON_GOTBACK, player->objectName(), "thtanguan", QString());
        CardsMoveStruct move1(card1->getEffectiveId(), target, Player::PlaceHand, reason1);
        CardsMoveStruct move2(card2->getEffectiveId(), player, Player::PlaceHand, reason2);
        moves << move1 << move2;
        room->moveCardsAtomic(moves, true);
        if (card1->isRed() && card1->sameColorWith(card2) && player->isWounded()) {
            if (player->askForSkillInvoke("thtanguan", "recover"))
                room->recover(player, RecoverStruct(player));
        } else if (card1->isBlack() && card1->sameColorWith(card2)) {
            if (player->askForSkillInvoke("thtanguan", "draw:" + target->objectName()))
                target->drawCards(2, "thtanguan");
        }
        return false;
    }
};

class ThYuancui : public PhaseChangeSkill
{
public:
    ThYuancui() : PhaseChangeSkill("thyuancui")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->sendCompulsoryTriggerLog(player, objectName());
        if (player->isWounded())
            room->recover(player, RecoverStruct(player));
        else
            room->loseHp(player);
        return false;
    }
};

class ThHuikuang : public TriggerSkill
{
public:
    ThHuikuang() : TriggerSkill("thhuikuang")
    {
        events << HpLost << HpRecover << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *&) const
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *pl, r->getAlivePlayers())
                r->setPlayerMark(pl, objectName(), 0);
        } else if (TriggerSkill::triggerable(p)) {
            if (e == HpLost)
                return QStringList(objectName());
            else if (e == HpRecover) {
                if (p->canDiscard(p, "he"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        if (e == HpLost) {
            if (p->askForSkillInvoke(objectName())) {
                r->broadcastSkillInvoke(objectName());
                return true;
            }
        } else if (e == HpRecover) {
            const Card *card = r->askForExchange(p, objectName(), 2, 2, true, "@thhuikuang", true);
            if (card) {
                r->broadcastSkillInvoke(objectName());
                p->tag["ThHuikuangCard"] = QVariant::fromValue(card);
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        if (e == HpLost) {
            QList<int> cards = r->getNCards(2, false);
            CardMoveReason reason(CardMoveReason::S_REASON_DRAW, p->objectName(), objectName(), QString());
            CardsMoveStruct move(cards, p, Player::PlaceHand, reason);
            r->moveCardsAtomic(move, false);
            if (p->getMark(objectName()) == 0 && p->askForSkillInvoke("thhuikuang_sa", "show")) {
                r->addPlayerMark(p, objectName());
                r->showCard(p, cards.first());
                r->showCard(p, cards.last());
                const Card *card1 = Sanguosha->getCard(cards.first());
                const Card *card2 = Sanguosha->getCard(cards.last());
                if (card1->isBlack() && card2->isBlack()) {
                    SavageAssault *sa = new SavageAssault(Card::NoSuit, 0);
                    sa->setSkillName("_thhuikuang");
                    if (sa->isAvailable(p) && !p->isCardLimited(sa, Card::MethodUse))
                        r->useCard(CardUseStruct(sa, p, QList<ServerPlayer *>()));
                    else
                        delete sa;
                }
            }
        } else if (e == HpRecover) {
            const Card *card = p->tag["ThHuikuangCard"].value<const Card *>();
            p->tag.remove("ThHuikuangCard");
            if (card) {
                QList<int> cards = card->getSubcards();
                LogMessage log;
                log.type = "$DiscardCardWithSkill";
                log.from = p;
                log.arg = objectName();
                log.card_str = IntList2StringList(cards).join("+");
                r->sendLog(log);
                r->throwCard(card, p);
                delete card;
                const Card *card1 = Sanguosha->getCard(cards.first());
                const Card *card2 = Sanguosha->getCard(cards.last());
                if (card1->isBlack() && card2->isBlack()) {
                    SavageAssault *sa = new SavageAssault(Card::NoSuit, 0);
                    sa->setSkillName("_thhuikuang");
                    if (sa->isAvailable(p) && !p->isCardLimited(sa, Card::MethodUse)
                            && p->getMark(objectName()) == 0 && p->askForSkillInvoke("thhuikuang_sa", "use")) {
                        r->addPlayerMark(p, objectName());
                        r->useCard(CardUseStruct(sa, p, QList<ServerPlayer *>()));
                    } else
                        delete sa;
                }
            }
        }
        return false;
    }
};

TouhouShinPackage::TouhouShinPackage()
    :Package("touhou-shin")
{
    General *shin001 = new General(this, "shin001", "kaze");
    shin001->addSkill(new ThLuanshen);

    General *shin002 = new General(this, "shin002", "hana");
    shin002->addSkill(new ThFeiman);
    shin002->addSkill(new ThGuaiqi);
    shin002->addSkill(new ThGuaiqiRecord);
    related_skills.insertMulti("thguaiqi", "#thguaiqi-record");

    General *shin003 = new General(this, "shin003", "yuki", 2);
    shin003->addSkill(new ThJingtao);
    shin003->addSkill(new ThZongni);
    shin003->addSkill(new ThZongniDiscard);
    related_skills.insertMulti("thzongni", "#thzongni");

    General *shin004 = new General(this, "shin004", "tsuki");
    shin004->addSkill(new ThLanzou);
    shin004->addSkill(new ThLanzouSecond);
    related_skills.insertMulti("thlanzou", "#thlanzou");

    General *shin005 = new General(this, "shin005", "kaze", 3, false);
    shin005->addSkill(new ThXinqi);
    shin005->addSkill(new ThNengwu);
    shin005->addSkill(new ThNengwuClear);
    related_skills.insertMulti("thnengwu", "#thnengwu-clear");

    General *shin006 = new General(this, "shin006", "hana", 3);
    shin006->addSkill(new ThBaochui);
    shin006->addSkill(new ThBaochuiReturn);
    related_skills.insertMulti("thbaochui", "#thbaochui-return");
    shin006->addSkill(new ThYishi);
    shin006->addSkill(new ThYishiNullified);
    related_skills.insertMulti("thyishi", "#thyishi");

    General *shin007 = new General(this, "shin007", "yuki");
    shin007->addSkill(new ThMoju);
    shin007->addSkill(new ThMojuMaxCardsSkill);
    related_skills.insertMulti("thmoju", "#thmoju");

    General *shin008 = new General(this, "shin008", "tsuki");
    shin008->addSkill(new ThLianying);
    shin008->addSkill(new ThYuanxiao);

    General *shin009 = new General(this, "shin009", "kaze", 3, false);
    shin009->addSkill(new ThWuyi);
    shin009->addSkill(new ThWuyiTargetMod);
    shin009->addSkill(new SlashNoDistanceLimitSkill("thwuyi"));
    related_skills.insertMulti("thwuyi", "#thwuyi-tar");
    related_skills.insertMulti("thwuyi", "#thwuyi-slash-ndl");
    shin009->addSkill(new ThMumi);

    General *shin010 = new General(this, "shin010", "hana", 3);
    shin010->addSkill(new ThWangyu);
    shin010->addSkill(new ThGuangshi);

    General *shin011 = new General(this, "shin011", "yuki", 4, false);
    shin011->addSkill(new ThSunwu);
    shin011->addSkill(new ThLiaogan);

    General *shin012 = new General(this, "shin012", "tsuki");
    shin012->addSkill(new ThJianyue);

    General *shin013 = new General(this, "shin013", "kaze", 3);
    shin013->addSkill(new ThHuanjian);
    shin013->addSkill(new ThHuanjianReturn);
    related_skills.insertMulti("thhuanjian", "#thhuanjian");
    shin013->addSkill(new ThShenmi);

    General *shin014 = new General(this, "shin014", "hana");
    shin014->addSkill(new ThMuyu);

    General *shin015 = new General(this, "shin015", "yuki", 3);
    shin015->addSkill(new ThNihui);
    shin015->addSkill(new ThNihuiEdit);
    shin015->addSkill(new ThNihuiInvalidity);
    related_skills.insertMulti("thnihui", "#thnihui-inv");
    shin015->addSkill(new ThTanguan);
    shin015->addSkill(new ThTanguanTrigger);
    related_skills.insertMulti("thtanguan", "#thtanguan");

    General *shin016 = new General(this, "shin016", "tsuki", 4, false);
    shin016->addSkill(new ThYuancui);
    shin016->addSkill(new ThHuikuang);

    addMetaObject<ThLuanshenCard>();
    addMetaObject<ThLianyingCard>();
    addMetaObject<ThMumiCard>();
    addMetaObject<ThLiaoganCard>();
    addMetaObject<ThMuyuCard>();
    addMetaObject<ThNihuiCard>();
    addMetaObject<ThNihuiEditCard>();

    skills << new ThBaochuiRecord;
}

ADD_PACKAGE(TouhouShin)
