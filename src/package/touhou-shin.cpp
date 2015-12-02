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
        room->askForDiscard(player, objectName(), 998, 1, false, false, "@thzongni-discard");
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
                if (card->isKindOf("Jink") || card->isKindOf("Nullification")) {
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
        if (room->askForChoice(player, objectName(), "show+cancel") == "show") {
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

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
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
    };

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
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
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thwangyu", true, player->hasSkill(objectName()));
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
        if (!player->canDiscard(player, "he") || !room->askForCard(player, "..", "@thguangshi"))
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

    General *shin010 = new General(this, "shin010", "hana", 3);
    shin010->addSkill(new ThWangyu);
    shin010->addSkill(new ThGuangshi);

    addMetaObject<ThLuanshenCard>();
    addMetaObject<ThLianyingCard>();

    skills << new ThBaochuiRecord;
}

ADD_PACKAGE(TouhouShin)