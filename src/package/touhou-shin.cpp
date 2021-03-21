#include "touhou-shin.h"

#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "fantasy.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"
#include "standard.h"
#include "touhou-hana.h"

#include <QButtonGroup>
#include <QDialog>

ThLuanshenCard::ThLuanshenCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

void ThLuanshenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->showCard(source, subcards);
    ServerPlayer *target = targets.first();
    QStringList choices;
    if (target->canDiscard(target, "he") && target->getCardCount() >= subcardsLength())
        choices << "discard";
    choices << "turnover";
    QString choice
        = room->askForChoice(target, "thluanshen", choices.join("+"), QVariant::fromValue(IntList2VariantList(subcards)));
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

class ThLuanshen : public ViewAsSkill
{
public:
    ThLuanshen()
        : ViewAsSkill("thluanshen")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;
        ThLuanshenCard *card = new ThLuanshenCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThLuanshenCard") && player->getHandcardNum() >= player->getHp();
    }
};

class ThFeiman : public TriggerSkill
{
public:
    ThFeiman()
        : TriggerSkill("thfeiman")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Play;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target
            = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thfeiman", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThFeimanTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThFeimanTarget"].value<ServerPlayer *>();
        player->tag.remove("ThFeimanTarget");
        if (target) {
            room->damage(DamageStruct(objectName(), player, target));
            if (target->isDead())
                return false;
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

class ThGuaiqi : public TriggerSkill
{
public:
    ThGuaiqi()
        : TriggerSkill("thguaiqi")
    {
        events << EventPhaseChanging << EventPhaseEnd;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                p->setMark("thguaiqi", 0);
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play && player->hasFlag("thguaiqi_invoke")
                   && player->isAlive())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1);
        return false;
    }
};

class ThGuaiqiRecord : public TriggerSkill
{
public:
    ThGuaiqiRecord()
        : TriggerSkill("#thguaiqi-record")
    {
        events << PreDamageDone;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *&) const
    {
        ServerPlayer *owner = room->getCurrent();
        if (owner && owner->getPhase() == Player::Play)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        player->addMark("thguaiqi", damage.damage);
        if (player->getMark("thguaiqi") > 1)
            room->getCurrent()->setFlags("thguaiqi_invoke");

        return false;
    }
};

class ThJingtao : public TriggerSkill
{
public:
    ThJingtao()
        : TriggerSkill("thjingtao")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard)
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
        player->drawCards(2, objectName());
        return false;
    }
};

class ThZongni : public TriggerSkill
{
public:
    ThZongni()
        : TriggerSkill("thzongni")
    {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || !room->isSomeonesTurn(player))
            return QStringList();
        if (player->getPhase() == Player::Draw)
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
        player->setFlags("thzongni");
        player->setPhase(Player::Play);
        room->broadcastProperty(player, "phase");
        return false;
    }
};

class ThZongniDiscard : public TriggerSkill
{
public:
    ThZongniDiscard()
        : TriggerSkill("#thzongni")
    {
        events << EventPhaseEnd << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        if (player->getPhase() == Player::PhaseNone) {
            ServerPlayer *splayer = player->getRoom()->findPlayer(player->objectName());
            if (splayer)
                splayer->setFlags("-thzongni");
            return false;
        }
        return player->hasFlag("thzongni");
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->setFlags("-thzongni");
        room->sendCompulsoryTriggerLog(player, "thzongni");
        if (player->canDiscard(player, "h"))
            room->askForDiscard(player, "thzongni", 998, 1, false, false, "@thzongni-discard");
        return false;
    }
};

class ThLanzou : public TriggerSkill
{
public:
    ThLanzou()
        : TriggerSkill("thlanzou")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                int index = 0;
                foreach (int id, move.card_ids) {
                    if (move.from_places[index] != Player::PlaceHand && move.from_places[index] != Player::PlaceEquip) {
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target
            = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thlanzou", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThLanzouTarget"] = QVariant::fromValue(target);
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThLanzouTarget"].value<ServerPlayer *>();
        player->tag.remove("ThLanzouTarget");
        if (target)
            target->drawCards(1, objectName());

        return false;
    }
};

class ThLanzouSecond : public TriggerSkill
{
public:
    ThLanzouSecond()
        : TriggerSkill("#thlanzou")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            int index = 0;
            foreach (int id, move.card_ids) {
                if (move.from_places[index] != Player::PlaceHand && move.from_places[index] != Player::PlaceEquip) {
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *skill_invoker) const
    {
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *skill_target, QVariant &, ServerPlayer *) const
    {
        skill_target->drawCards(1, "thlanzou");

        return false;
    }
};

class ThXinqi : public TriggerSkill
{
public:
    ThXinqi()
        : TriggerSkill("thxinqi")
    {
        events << TargetSpecifying;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isNDTrick() && use.card->isBlack())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player)
                    continue;
                if (use.to.contains(owner))
                    skill_list.insert(owner, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        room->broadcastSkillInvoke(objectName());

        ask_who->drawCards(1);
        if (player->isKongcheng())
            return false;
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

class ThNengwu : public TriggerSkill
{
public:
    ThNengwu()
        : TriggerSkill("thnengwu")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Start && !player->isKongcheng())
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player || owner->isKongcheng())
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

        if (player->isDead())
            return false;

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

class ThNengwuClear : public TriggerSkill
{
public:
    ThNengwuClear()
        : TriggerSkill("#thnengwu-clear")
    {
        events << EventPhaseChanging << CardsMoveOneTime << Death;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from || !move.from_places.contains(Player::PlaceHand))
                return QStringList();
            QVariantList ids = player->tag["ThNengwuId"].toList();
            if (ids.isEmpty() || move.from != player)
                return QStringList();
            QStringList sources = player->tag["ThNengwuSource"].toStringList();
            int reason = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
            if (reason == CardMoveReason::S_REASON_USE || reason == CardMoveReason::S_REASON_RESPONSE
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

class ThBaochui : public TriggerSkill
{
public:
    ThBaochui()
        : TriggerSkill("thbaochui")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Play && !player->isKongcheng()) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player || owner->isKongcheng() || owner->getHandcardNum() > 3)
                    continue;
                skill_list.insert(owner, QStringList(objectName()));
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        const Card *card = room->askForCard(player, ".!", "@thbaochui:" + ask_who->objectName(), data, Card::MethodNone);
        if (!card)
            card = player->getRandomHandCard();
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), ask_who->objectName(), objectName(),
                              QString());
        room->obtainCard(ask_who, card, reason, false);
        ask_who->addToPile("currency", ask_who->handCards());
        room->setPlayerFlag(player, "thbaochui");
        return false;
    }
};

class ThBaochuiReturn : public TriggerSkill
{
public:
    ThBaochuiReturn()
        : TriggerSkill("#thbaochui-return")
    {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->hasFlag("thbaochui") && player->getPhase() == Player::Play)
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getPile("currency").isEmpty())
                    continue;
                skill_list.insert(p, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        DummyCard *dummy = new DummyCard(ask_who->getPile("currency"));
        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, ask_who->objectName(), "thbaochui", QString());
        room->obtainCard(ask_who, dummy, reason, true);
        delete dummy;
        ask_who->drawCards(1);
        return false;
    }
};

class ThBaochuiRecord : public TriggerSkill
{
public:
    ThBaochuiRecord()
        : TriggerSkill("thbaochui_record")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() != Player::Play)
            return QStringList();
        int reason = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
        if (reason == CardMoveReason::S_REASON_USE || reason == CardMoveReason::S_REASON_RESPONSE) {
            for (int i = 0; i < move.card_ids.size(); i++)
                if (move.from_pile_names[i] == "currency")
                    return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
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

class ThYishi : public TriggerSkill
{
public:
    ThYishi()
        : TriggerSkill("thyishi")
    {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (TriggerSkill::triggerable(player) && effect.card->hasFlag("thbaochui+" + player->objectName()))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        CardEffectStruct effect = data.value<CardEffectStruct>();
        effect.nullified = true;
        data = QVariant::fromValue(effect);
        return false;
    }
};

class ThYishiNullified : public TriggerSkill
{
public:
    ThYishiNullified()
        : TriggerSkill("#thyishi")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        int reason = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
        if (reason == CardMoveReason::S_REASON_USE && move.from_places.contains(Player::PlaceSpecial)
            && move.from_pile_names.contains("currency")) {
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if (!card)
                return QStringList();
            if (!card->isVirtualCard()
                || (card->subcardsLength() == 1 && card->getSubcards() == move.card_ids
                    && card->getClassName() == Sanguosha->getCard(move.card_ids.first())->getClassName())) {
                card->setFlags("thbaochui+" + move.from->objectName());
                move.reason.m_extraData = QVariant::fromValue(card);
                data = QVariant::fromValue(move);
            }
        }
        return QStringList();
    }
};

class ThMoju : public TriggerSkill
{
public:
    ThMoju()
        : TriggerSkill("thmoju")
    {
        events << DrawNCards << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == DrawNCards && player->getWeapon() && TriggerSkill::triggerable(player))
            return QStringList(objectName());
        else if (triggerEvent == TargetSpecified && player->getArmor() && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && player->getPhase() == Player::Play)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
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
                index++;
            }
            player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        }
        return false;
    }
};

class ThMojuMaxCardsSkill : public MaxCardsSkill
{
public:
    ThMojuMaxCardsSkill()
        : MaxCardsSkill("#thmoju")
    {
    }

    virtual int getExtra(const Player *target) const
    {
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

class ThGuzhen : public TriggerSkill
{
public:
    ThGuzhen()
        : TriggerSkill("thguzhen")
    {
        events << CardsMoveOneTime << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (e == CardsMoveOneTime && TriggerSkill::triggerable(p)) {
            CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
            if (move.from == p && move.from_places.contains(Player::PlaceEquip))
                return QStringList(objectName());
            if (move.to == p && move.to_place == Player::PlaceEquip)
                return QStringList(objectName());
        } else if (e == EventPhaseStart && p->getMark(objectName()) > 0 && p->getPhase() == Player::RoundStart)
            r->setPlayerMark(p, objectName(), 0);
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->addPlayerMark(player, objectName());
        return false;
    }
};

ThLianyingCard::ThLianyingCard()
{
    target_fixed = true;
}

void ThLianyingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->setFlags("thlianying");
    room->handleAcquireDetachSkills(source, "ikchilian|thjibu");
}

class ThLianyingViewAsSkill : public ViewAsSkill
{
public:
    ThLianyingViewAsSkill()
        : ViewAsSkill("thlianying")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &cards, const Card *to_select) const
    {
        return cards.size() < 2 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.size() != 2)
            return NULL;
        ThLianyingCard *card = new ThLianyingCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThLianyingCard") && player->getCardCount() > 1;
    }
};

class ThLianying : public TriggerSkill
{
public:
    ThLianying()
        : TriggerSkill("thlianying")
    {
        events << EventPhaseChanging;
        view_as_skill = new ThLianyingViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive && player->hasFlag(objectName())) {
            player->setFlags("-" + objectName());
            room->handleAcquireDetachSkills(player, "-ikchilian|-thjibu", true);
        }
        return QStringList();
    }
};

class ThYuanxiao : public TriggerSkill
{
public:
    ThYuanxiao()
        : TriggerSkill("thyuanxiao")
    {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
            foreach (ServerPlayer *to, use.to)
                if (to->getHandcardNum() > player->getHandcardNum())
                    return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *to, use.to) {
            if (to->getHandcardNum() > player->getHandcardNum())
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(to))) {
                    room->broadcastSkillInvoke(objectName());

                    int card_id = room->askForCardChosen(player, to, "h", objectName());
                    room->obtainCard(player, card_id);
                    room->showCard(player, card_id);
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
    ThWuyi()
        : TriggerSkill("thwuyi")
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
            if (use.card->isKindOf("Slash") && use.card->getSkillName() == objectName() && p->hasFlag("ThWuyiEquip"))
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
    ThWuyiTargetMod()
        : TargetModSkill("#thwuyi-tar")
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
    ThMumiVS()
        : ViewAsSkill("thmumi")
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
    ThMumi()
        : TriggerSkill("thmumi")
    {
        events << CardAsked;
        view_as_skill = new ThMumiVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || !player->canDiscard(player, "he"))
            return QStringList();
        QString asked = data.toStringList().first();
        if (asked == "jink")
            return QStringList(objectName());

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
    ThWangyu()
        : TriggerSkill("thwangyu")
    {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash")
            && room->getCardPlace(damage.card->getEffectiveId()) == Player::PlaceTable) {
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
            ServerPlayer *target
                = room->askForPlayerChosen(player, targets, objectName(), "@thwangyu", true, player->hasSkill(objectName()));
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
    ThGuangshi()
        : TriggerSkill("thguangshi")
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
    ThSunwu()
        : TriggerSkill("thsunwu")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
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

class ThLiaogan : public TriggerSkill
{
public:
    ThLiaogan()
        : TriggerSkill("thliaogan")
    {
        events << EventPhaseStart << Death;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->getPile("frost").isEmpty())
                    skill_list.insert(p, QStringList(objectName()));
            }
            return skill_list;
        }
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return skill_list;
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

        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        ServerPlayer *target
            = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@thliaogan", true, true);
        if (target) {
            player->tag["ThLiaoganTarget"] = QVariant::fromValue(target);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        ServerPlayer *target = player->tag["ThLiaoganTarget"].value<ServerPlayer *>();
        player->tag.remove("ThLiaoganTarget");
        if (target) {
            DummyCard *dummy = new DummyCard(player->getPile("frost"));
            target->obtainCard(dummy);
            delete dummy;
            player->addMark("thliaogan_" + target->objectName());
            QStringList list;
            if (!target->hasSkill("thxiagong"))
                list << "thxiagong";
            if (!target->hasSkill("ikjingnie"))
                list << "ikjingnie";
            if (!list.isEmpty())
                room->handleAcquireDetachSkills(target, list, true, true);
        }
        return false;
    }
};

class ThJianyueVS : public ZeroCardViewAsSkill
{
public:
    ThJianyueVS()
        : ZeroCardViewAsSkill("thjianyue")
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
    ThJianyue()
        : TriggerSkill("thjianyue")
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

ThHuanjianCard::ThHuanjianCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void ThHuanjianCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &) const
{
    room->moveCardTo(this, NULL, Player::DrawPile, true);
}

class ThHuanjianVS : public OneCardViewAsSkill
{
public:
    ThHuanjianVS()
        : OneCardViewAsSkill("thhuanjian")
    {
        response_pattern = "@@thhuanjian";
        expand_pile = "note";
        filter_pattern = ".|.|.|note";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        ThHuanjianCard *card = new ThHuanjianCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThHuanjian : public TriggerSkill
{
public:
    ThHuanjian()
        : TriggerSkill("thhuanjian")
    {
        events << BeforeCardsMove << EventPhaseChanging << EventPhaseStart;
        view_as_skill = new ThHuanjianVS;
    }

    virtual TriggerList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (event == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("ThHuanjianUsed") && !p->getPile("note").isEmpty())
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        } else if (event == EventPhaseStart) {
            if (player->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setFlags("-ThHuanjianUsed");
            }
        } else if (event == BeforeCardsMove) {
            if (TriggerSkill::triggerable(player) && !player->isKongcheng()) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.to_place == Player::DiscardPile
                    && (move.from_places.contains(Player::PlaceEquip) || move.from_places.contains(Player::PlaceJudge)))
                    skill_list.insert(player, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent event, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        if (event == EventPhaseChanging) {
            room->askForUseCard(player, "@@thhuanjian", "@thhuanjian-put", -1, Card::MethodNone);
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            int n = move.from_places.count(Player::PlaceEquip) + move.from_places.count(Player::PlaceJudge);
            const Card *card = room->askForExchange(player, objectName(), n, 1, false, "@thhuanjian", true);
            if (card) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                player->tag["ThHuanjianIds"] = QVariant::fromValue(IntList2VariantList(card->getSubcards()));
                delete card;
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QList<int> hands = VariantList2IntList(player->tag["ThHuanjianIds"].toList());
        player->tag.remove("ThHuanjianIds");
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> move_ids, disabled_ids;
        for (int i = 0; i < move.card_ids.length(); ++i) {
            if (move.from_places[i] == Player::PlaceEquip || move.from_places[i] == Player::PlaceJudge)
                move_ids << move.card_ids[i];
            else
                disabled_ids << move.card_ids[i];
        }

        if (move_ids.isEmpty() || move_ids.length() < hands.length())
            return false;

        QList<int> to_move;
        if (hands.length() == move_ids.length())
            to_move = move_ids;
        else {
            QList<int> to_move;
            room->fillAG(move.card_ids, NULL, disabled_ids);
            while (to_move.length() < hands.length()) {
                int id = room->askForAG(player, move_ids, false, objectName());
                if (id == -1)
                    id = move_ids.at(qrand() % move_ids.length());
                move_ids.removeOne(id);
                room->takeAG(player, id, false);
                to_move << id;
            }
            room->clearAG();
        }
        if (hands.length() == to_move.length()) {
            move.removeCardIds(to_move);
            data = QVariant::fromValue(move);
            player->addToPile("note", hands + to_move);
            if (room->isSomeonesTurn())
                player->setFlags("ThHuanjianUsed");
        }
        return false;
    }
};

ThShenmiCard::ThShenmiCard()
{
    will_throw = false;
}

bool ThShenmiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetFilter(targets, to_select, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;

    Card *card = Sanguosha->cloneCard(user_string.split("+").first());
    card->addSubcards(subcards);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self);
}

bool ThShenmiCard::targetFixed() const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetFixed();
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    Card *card = Sanguosha->cloneCard(user_string.split("+").first());
    card->addSubcards(subcards);
    card->deleteLater();
    return card && card->targetFixed();
}

bool ThShenmiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;

    Card *card = Sanguosha->cloneCard(user_string.split("+").first());
    card->addSubcards(subcards);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *ThShenmiCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *wenyang = card_use.from;
    Room *room = wenyang->getRoom();

    QString to_iklixin = user_string;
    if (user_string == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList iklixin_list;
        iklixin_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            iklixin_list << "thunder_slash"
                         << "fire_slash";
        to_iklixin = room->askForChoice(wenyang, "thshenmi_slash", iklixin_list.join("+"));
    }

    Card *use_card = Sanguosha->cloneCard(to_iklixin);
    use_card->addSubcards(subcards);
    use_card->setSkillName("thshenmi");
    return use_card;
}

const Card *ThShenmiCard::validateInResponse(ServerPlayer *wenyang) const
{
    Room *room = wenyang->getRoom();

    QString to_iklixin;
    if (user_string == "peach+analeptic") {
        QStringList iklixin_list;
        iklixin_list << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            iklixin_list << "analeptic";
        to_iklixin = room->askForChoice(wenyang, "thshenmi_saveself", iklixin_list.join("+"));
    } else if (user_string == "slash") {
        QStringList iklixin_list;
        iklixin_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            iklixin_list << "thunder_slash"
                         << "fire_slash";
        to_iklixin = room->askForChoice(wenyang, "thshenmi_slash", iklixin_list.join("+"));
    } else
        to_iklixin = user_string;

    Card *use_card = Sanguosha->cloneCard(to_iklixin);
    use_card->addSubcards(subcards);
    use_card->setSkillName("thshenmi");
    return use_card;
}

class ThShenmi : public ViewAsSkill
{
public:
    ThShenmi()
        : ViewAsSkill("thshenmi")
    {
        expand_pile = "note";
    }

    virtual SkillDialog *getDialog() const
    {
        return ThMimengDialog::getInstance("thshenmi", true, false);
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() > 1 || !Self->getPile("note").contains(to_select->getId()))
            return false;
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return selected.isEmpty() || to_select->sameColorWith(selected.first());
        else {
            QString pattern = Sanguosha->getCurrentCardUsePattern();
            if (pattern == "slash" || pattern == "jink" || pattern.contains("peach"))
                return selected.isEmpty() || to_select->sameColorWith(selected.first());
            if (pattern == "nullification")
                return selected.isEmpty() || !to_select->sameColorWith(selected.first());
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        if (cards.first()->sameColorWith(cards.last())) {
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
                || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
                ThShenmiCard *tianyan_card = new ThShenmiCard;
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                    pattern = "analeptic";
                tianyan_card->addSubcards(cards);
                tianyan_card->setUserString(pattern);
                return tianyan_card;
            }

            const Card *c = Self->tag["thshenmi"].value<const Card *>();
            if (c) {
                ThShenmiCard *card = new ThShenmiCard;
                card->addSubcards(cards);
                card->setUserString(c->objectName());
                return card;
            }
        } else {
            Nullification *null = new Nullification(Card::SuitToBeDecided, -1);
            null->addSubcards(cards);
            null->setSkillName(objectName());
            return null;
        }
        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        QList<int> ids = player->getPile("note");
        if (ids.length() <= 1)
            return false;

        const Card *first = Sanguosha->getCard(ids.takeFirst());
        foreach (int id, ids) {
            if (first->sameColorWith(Sanguosha->getCard(id))) {
                Slash *slash = new Slash(Card::SuitToBeDecided, -1);
                slash->addSubcard(first);
                slash->addSubcard(id);
                slash->setSkillName(objectName());
                slash->deleteLater();
                if (slash->isAvailable(player))
                    return true;

                Peach *peach = new Peach(Card::SuitToBeDecided, -1);
                peach->addSubcard(first);
                peach->addSubcard(id);
                peach->setSkillName(objectName());
                peach->deleteLater();
                if (peach->isAvailable(player))
                    return true;

                Analeptic *analeptic = new Analeptic(Card::SuitToBeDecided, -1);
                analeptic->addSubcard(first);
                analeptic->addSubcard(id);
                analeptic->setSkillName(objectName());
                analeptic->deleteLater();
                if (analeptic->isAvailable(player))
                    return true;
            }
        }

        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        QList<int> ids = player->getPile("note");
        if (ids.length() <= 1)
            return false;

        const Card *first = Sanguosha->getCard(ids.takeFirst());
        if (!ids.isEmpty()) {
            foreach (int id, ids) {
                if (first->sameColorWith(Sanguosha->getCard(id))) {
                    if (pattern == "peach")
                        return player->getMark("Global_PreventPeach") == 0;
                    else if (pattern.contains("analeptic") || pattern == "jink" || pattern == "slash")
                        return true;
                } else {
                    if (pattern == "nullification")
                        return true;
                }
            }
        }

        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        QList<int> ids = player->getPile("note");
        if (ids.length() <= 1)
            return false;

        const Card *first = Sanguosha->getCard(ids.takeFirst());
        if (!ids.isEmpty()) {
            foreach (int id, ids) {
                if (!first->sameColorWith(Sanguosha->getCard(id)))
                    return true;
            }
        }

        return false;
    }
};

ThMuyuCard::ThMuyuCard()
{
}

bool ThMuyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isNude() && to_select != Self;
}

void ThMuyuCard::onEffect(const CardEffectStruct &effect) const
{
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

class ThMuyuViewAsSkill : public OneCardViewAsSkill
{
public:
    ThMuyuViewAsSkill()
        : OneCardViewAsSkill("thmuyu")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *card) const
    {
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

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("ThMuyuCard");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPile("prison").isEmpty())
            return false;
        return pattern == "jink" || pattern == "nullification";
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !player->getPile("prison").isEmpty();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
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

class ThMuyu : public PhaseChangeSkill
{
public:
    ThMuyu()
        : PhaseChangeSkill("thmuyu")
    {
        view_as_skill = new ThMuyuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start
            && !target->getPile("prison").isEmpty();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
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
    if (source->getSkillStep("thnihui") == 1) {
        source->drawCards(3, "thnihui");
        room->askForDiscard(source, "thnihui", 1, 1, false, true);
        room->setPlayerProperty(source, "thnihui_step", 2);
    } else if (source->getSkillStep("thnihui") == 2) {
        source->drawCards(1, "thnihui");
        room->setPlayerProperty(source, "thnihui_step", 1);
    }
}

class ThNihuiVS : public ViewAsSkill
{
public:
    ThNihuiVS()
        : ViewAsSkill("thnihui")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->getSkillStep(objectName()) < 2)
            return false;

        return selected.length() < 3 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Self->getSkillStep(objectName()) < 2) {
            if (cards.isEmpty())
                return new ThNihuiCard;
        } else {
            if (cards.length() == 3) {
                ThNihuiCard *card = new ThNihuiCard;
                card->addSubcards(cards);
                return card;
            }
        }
        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThNihuiCard");
    }
};

class ThNihui : public TriggerSkill
{
public:
    ThNihui()
        : TriggerSkill("thnihui")
    {
        events << GameStart;
        view_as_skill = new ThNihuiVS;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->setPlayerProperty(player, "thnihui_step", 1);
        return false;
    }
};

class ThTanguan : public TriggerSkill
{
public:
    ThTanguan()
        : TriggerSkill("thtanguan")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->isKongcheng())
                return TriggerSkill::triggerable(player) && !player->isKongcheng() && player->getPhase() == Player::Finish;
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
    ThTanguanTrigger()
        : TriggerSkill("#thtanguan")
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
    ThYuancui()
        : PhaseChangeSkill("thyuancui")
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
    ThHuikuang()
        : TriggerSkill("thhuikuang")
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
                r->showCard(p, cards);
                const Card *card1 = Sanguosha->getCard(cards.first());
                const Card *card2 = Sanguosha->getCard(cards.last());
                if (card1->sameColorWith(card2)) {
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
                if (card1->sameColorWith(card2)) {
                    SavageAssault *sa = new SavageAssault(Card::NoSuit, 0);
                    sa->setSkillName("_thhuikuang");
                    if (sa->isAvailable(p) && !p->isCardLimited(sa, Card::MethodUse) && p->getMark(objectName()) == 0
                        && p->askForSkillInvoke("thhuikuang_sa", "use")) {
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

class ThXuanman : public TriggerSkill
{
public:
    ThXuanman()
        : TriggerSkill("thxuanman")
    {
        events << DrawInitialCards << AfterDrawInitialCards << CardsMoveOneTime << EventMarksGot;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(p)) {
            if (e == DrawInitialCards || e == AfterDrawInitialCards)
                return QStringList(objectName());
            else if (e == EventMarksGot && d.toString() == "@flying" && p->getMark("@flying") > 3)
                return QStringList(objectName());
            else if (e == CardsMoveOneTime) {
                CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
                if (move.from == p && move.from_places.contains(Player::PlaceHand)) {
                    int reason = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
                    if (reason == CardMoveReason::S_REASON_USE || reason == CardMoveReason::S_REASON_RESPONSE
                        || reason == CardMoveReason::S_REASON_DISCARD)
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *) const
    {
        if (e == DrawInitialCards) {
            r->sendCompulsoryTriggerLog(p, objectName());
            d = d.toInt() + 2;
        } else if (e == AfterDrawInitialCards) {
            r->broadcastSkillInvoke(objectName());
            const Card *exchange_card = r->askForExchange(p, objectName(), 3, 3);
            p->addToPile("dance", exchange_card->getSubcards(), false);
            delete exchange_card;
        } else if (e == EventMarksGot) {
            r->sendCompulsoryTriggerLog(p, objectName());
            r->removePlayerMark(p, "@flying", 4);
            p->drawCards(1, objectName());
        } else if (e == CardsMoveOneTime) {
            r->sendCompulsoryTriggerLog(p, objectName());
            QList<int> hands = p->handCards();
            QList<int> piles = p->getPile("dance");
            CardsMoveStruct move1(hands, p, Player::PlaceSpecial,
                                  CardMoveReason(CardMoveReason::S_REASON_PUT, p->objectName()));
            move1.to_pile_name = "dance";
            CardsMoveStruct move2(
                piles, p, Player::PlaceHand,
                CardMoveReason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), objectName(), QString()));
            QList<CardsMoveStruct> moves;
            moves << move1 << move2;
            r->moveCardsAtomic(moves, false);

            p->addToPile("dance", hands, false);

            r->addPlayerMark(p, "@flying");
        }
        return false;
    }
};

ThKuangwuCard::ThKuangwuCard()
{
    target_fixed = true;
}

void ThKuangwuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(1, objectName());
    const Card *card1 = NULL, *card2 = NULL;
    if (source->canDiscard(source, "he")) {
        card1 = room->askForCard(source, "..!", "@thkuangwu");
        if (!card1) {
            QList<const Card *> cards = source->getCards("he");
            card1 = cards.at(qrand() % cards.length());
        }
    }
    source->drawCards(1, objectName());
    if (source->canDiscard(source, "he")) {
        card2 = room->askForCard(source, "..!", "@thkuangwu");
        if (!card2) {
            QList<const Card *> cards = source->getCards("he");
            card2 = cards.at(qrand() % cards.length());
        }
    }

    if (card1 && card2 && card1->sameColorWith(card2))
        source->setFlags("ThKuangwuInvoke");
}

class ThKuangwuVS : public ZeroCardViewAsSkill
{
public:
    ThKuangwuVS()
        : ZeroCardViewAsSkill("thkuangwu")
    {
    }

    virtual const Card *viewAs() const
    {
        return new ThKuangwuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThKuangwuCard");
    }
};

class ThKuangwu : public TriggerSkill
{
public:
    ThKuangwu()
        : TriggerSkill("thkuangwu")
    {
        view_as_skill = new ThKuangwuVS;
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player && player->isAlive() && player->hasFlag("ThKuangwuInvoke")
            && data.value<PhaseChangeStruct>().to == Player::NotActive)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName(), "draw")) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1, objectName());
        if (!player->isKongcheng()) {
            const Card *card = room->askForCard(player, ".!", "@thkuangwu-put", QVariant(), Card::MethodNone);
            if (!card)
                card = player->getRandomHandCard();
            player->addToPile("dance", card, false);
        }
        room->askForDiscard(player, objectName(), 1, 1, false, true);
        return false;
    }
};

ThDieyingCard::ThDieyingCard()
{
}

bool ThDieyingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && !to_select->isWounded();
}

void ThDieyingCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->damage(DamageStruct("thdieying", effect.from, effect.to));
}

class ThDieyingVS : public OneCardViewAsSkill
{
public:
    ThDieyingVS()
        : OneCardViewAsSkill("thdieying")
    {
        response_pattern = "@@thdieying";
        filter_pattern = ".!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new ThDieyingCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThDieying : public TriggerSkill
{
public:
    ThDieying()
        : TriggerSkill("thdieying")
    {
        view_as_skill = new ThDieyingVS;
        events << PreCardUsed << EventPhaseChanging << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (e == PreCardUsed && r->isSomeonesTurn(p)) {
            CardUseStruct use = d.value<CardUseStruct>();
            int type = use.card->getTypeId();
            if (type != 0) {
                int n = p->getMark(objectName());
                if (n == 0)
                    p->setMark(objectName(), type);
                else if (n != type)
                    p->setFlags("ThDieyingDisabled");
            }
        } else if (e == EventPhaseChanging && p->getMark(objectName()) > 0) {
            if (d.value<PhaseChangeStruct>().to == Player::NotActive)
                p->setMark(objectName(), 0);
        } else if (e == EventPhaseStart && TriggerSkill::triggerable(p) && p->getPhase() == Player::Discard
                   && p->getMark(objectName()) > 0 && !p->hasFlag("ThDieyingDisabled")) {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thdieying", "@thdieying");
    }
};

ThBiyiCard::ThBiyiCard()
{
}

bool ThBiyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    int sum = 0;
    foreach (const Player *p, targets)
        sum += p->getHp();
    if (sum == 3)
        return false;
    return sum + to_select->getHp() <= 3;
}

void ThBiyiCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->drawCards(1, "thbiyi");
}

class ThBiyi : public ZeroCardViewAsSkill
{
public:
    ThBiyi()
        : ZeroCardViewAsSkill("thbiyi")
    {
    }

    virtual const Card *viewAs() const
    {
        return new ThBiyiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThBiyiCard");
    }
};

ThTunaCard::ThTunaCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = MethodNone;
}

void ThTunaCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->skip(Player::Play);
    source->addToPile("breath", subcards);
}

class ThTunaVS : public ViewAsSkill
{
public:
    ThTunaVS()
        : ViewAsSkill("thtuna")
    {
        response_pattern = "@@thtuna";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;
        int sum = 0;
        foreach (const Card *c, selected)
            sum += c->getNumber();
        if (sum == 13)
            return false;
        return sum + to_select->getNumber() <= 13;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;
        Card *card = new ThTunaCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThTuna : public TriggerSkill
{
public:
    ThTuna()
        : TriggerSkill("thtuna")
    {
        view_as_skill = new ThTunaVS;
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && !player->isNude() && !player->isSkipped(Player::Play)) {
            if (data.value<PhaseChangeStruct>().to == Player::Play)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thtuna", "@thtuna");
    }
};

ThNingguCard::ThNingguCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

bool ThNingguCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->hasFlag("ThNingguUsed") && to_select != Self;
}

void ThNingguCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->setPlayerFlag(target, "ThNingguUsed");

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "thninggu", QString());
    room->throwCard(this, reason, NULL);

    const Card *card = room->askForCard(target, "^BasicCard", "@thninggu-give:" + source->objectName(), QVariant(), MethodNone);
    if (card) {
        CardMoveReason reason_g(CardMoveReason::S_REASON_GIVE, target->objectName(), source->objectName(), "thninggu",
                                QString());
        room->obtainCard(source, card, reason_g, false);
    } else
        room->loseHp(target);
}

class ThNingguVS : public OneCardViewAsSkill
{
public:
    ThNingguVS()
        : OneCardViewAsSkill("thninggu")
    {
        filter_pattern = ".|.|.|breath";
        expand_pile = "breath";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new ThNingguCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("breath").isEmpty();
    }
};

class ThNinggu : public TriggerSkill
{
public:
    ThNinggu()
        : TriggerSkill("thninggu")
    {
        view_as_skill = new ThNingguVS;
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasFlag("ThNingguUsed"))
                    room->setPlayerFlag(p, "-ThNingguUsed");
            }
        }
        return QStringList();
    }
};

class ThQiongfaziyuan : public TriggerSkill
{
public:
    ThQiongfaziyuan()
        : TriggerSkill("thqiongfaziyuan")
    {
        events << EventMarksGot << Death;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (event == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (TriggerSkill::triggerable(player) && death.who->getMark("@poor") > 0)
                list.insert(player, QStringList(objectName()));
        } else if (data.toString() == "@poor" && player->hasFlag("thaimin")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                list.insert(p, QStringList(objectName()));
        }
        return list;
    }

    virtual bool effect(TriggerEvent event, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        if (event == Death) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->gainMark("@poor");
        } else {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->drawCards(1, objectName());
        }
        return false;
    }
};

class ThQiongfaSkill : public TriggerSkill
{
public:
    ThQiongfaSkill()
        : TriggerSkill("#thqiongfa")
    {
        events << EventMarksGot << EventMarksLost;
        frequency = NotCompulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (data.toString() == "@poor") {
            if (e == EventMarksLost && player->getMark("@poor") == 0 && player->hasSkill("thchipin", true)
                && player->hasSkill("thaimin", true)) {
                QStringList skills;
                skills << "-thchipin"
                       << "-thaimin";
                room->handleAcquireDetachSkills(player, skills, true, true);
            } else if (e == EventMarksGot && !(player->hasSkill("thchipin", true) && player->hasSkill("thaimin", true))) {
                QStringList skills;
                skills << "thchipin"
                       << "thaimin";
                room->handleAcquireDetachSkills(player, skills, true, true);
            }
        }
        return QStringList();
    }
};

class ThChipin : public DrawCardsSkill
{
public:
    ThChipin()
        : DrawCardsSkill("thchipin")
    {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        return n - 1;
    }
};

ThAiminCard::ThAiminCard()
{
    target_fixed = true;
}

void ThAiminCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (!p->isKongcheng()) {
            const Card *card = room->askForCard(p, ".", "@thaimin-give:" + source->objectName(), QVariant(), MethodNone);
            if (card) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, p->objectName(), source->objectName(), "thaimin",
                                      QString());
                room->obtainCard(source, card, reason, false);
            }
        }
    }
}

class ThAimin : public ZeroCardViewAsSkill
{
public:
    ThAimin()
        : ZeroCardViewAsSkill("thaimin")
    {
    }

    virtual const Card *viewAs() const
    {
        return new ThAiminCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasUsed("ThAiminCard"))
            return false;
        bool has_card = false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getHandcardNum() < player->getHandcardNum())
                return false;
            if (!has_card && !p->isKongcheng())
                has_card = true;
        }
        return has_card;
    }
};

class ThAiminTrigger : public TriggerSkill
{
public:
    ThAiminTrigger()
        : TriggerSkill("#thaimin")
    {
        events << CardsMoveOneTime;
        frequency = NotCompulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to == player && player->getMark("@poor") > 0) {
            if (move.reason.m_reason == CardMoveReason::S_REASON_GIVE && move.reason.m_skillName == "thaimin") {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getHandcardNum() < player->getHandcardNum())
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "thaimin");
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), "thaimin", "@thaimin");
        player->loseMark("@poor");
        target->setFlags("thaimin");
        target->gainMark("@poor");
        target->setFlags("-thaimin");
        return false;
    }
};

class ThShenhu : public TriggerSkill
{
public:
    ThShenhu()
        : TriggerSkill("thshenhu")
    {
        events << Damage << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && !player->isKongcheng()) {
            foreach (const Player *p, room->getOtherPlayers(player)) {
                if (!p->isKongcheng())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isKongcheng())
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thshenhu", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThShenhuTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThShenhuTarget"].value<ServerPlayer *>();
        player->tag.remove("ThShenhuTarget");
        if (target) {
            bool win = player->pindian(target, objectName());
            if (win) {
                LureTiger *lure = new LureTiger(Card::NoSuit, 0);
                lure->setSkillName("_thshenhu");
                room->useCard(CardUseStruct(lure, player, target));
            } else
                target->drawCards(1, objectName());
        }
        return false;
    }
};

class ThHouhu : public TriggerSkill
{
public:
    ThHouhu()
        : TriggerSkill("thhouhu")
    {
        events << Pindian;
        frequency = Frequent;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const
    {
        TriggerList list;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            list.insert(p, QStringList(objectName()));
        return list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        player->drawCards(1, objectName());
        return false;
    }
};

class ThGuizhou : public TargetModSkill
{
public:
    ThGuizhou()
        : TargetModSkill("thguizhou")
    {
        frequency = NotCompulsory;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(objectName())) {
            int n = 0;
            if (from->getHp() == 1)
                n++;
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->getHp() == 1)
                    n++;
            }
            return n;
        }
        return 0;
    }
};

ThRenmoCard::ThRenmoCard()
{
    will_throw = true;
    can_recast = true;
    handling_method = Card::MethodRecast;
    target_fixed = true;
}

void ThRenmoCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;

    CardMoveReason reason(CardMoveReason::S_REASON_RECAST, player->objectName());
    reason.m_skillName = this->getSkillName();
    room->moveCardTo(this, player, NULL, Player::DiscardPile, reason);
    room->setEmotion(player, "effects/recast");

    LogMessage log;
    log.type = "#UseCard_Recast";
    log.from = player;
    log.card_str = toString();
    room->sendLog(log);

    player->drawCards(1, "recast");

    QString choice = room->askForChoice(player, "threnmo", "attackrange+slashnum");

    if (choice == "attackrange")
        room->addPlayerMark(player, "threnmorange");
    else
        room->addPlayerMark(player, "threnmonum");

    //Player::getAttackRange
}

class ThRenmoVS : public OneCardViewAsSkill
{
public:
    ThRenmoVS()
        : OneCardViewAsSkill("threnmo")
    {
        filter_pattern = "EquipCard|.|.|hand";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *card = new ThRenmoCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThRenmo : public TriggerSkill
{
public:
    ThRenmo()
        : TriggerSkill("threnmo")
    {
        view_as_skill = new ThRenmoVS;
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (d.value<PhaseChangeStruct>().to == Player::NotActive) {
            if (p->getMark("threnmorange") > 0)
                r->setPlayerMark(p, "threnmorange", 0);
            if (p->getMark("threnmonum") > 0)
                r->setPlayerMark(p, "threnmonum", 0);
        }
        return QStringList();
    }
};

class ThRenMoTargetMod : public TargetModSkill
{
public:
    ThRenMoTargetMod()
        : TargetModSkill("#threnmo")
    {
        frequency = NotCompulsory;
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        return from->getMark("threnmonum");
    }
};

class ThMieyi : public TriggerSkill
{
public:
    ThMieyi()
        : TriggerSkill("thmieyi")
    {
        events << TargetSpecifying;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data) const
    {
        TriggerList skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                if (TriggerSkill::triggerable(to))
                    skill_list.insert(to, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *ask_who) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << "_ALL_TARGETS";
        data = QVariant::fromValue(use);

        Duel *duel = new Duel(Card::NoSuit, 0);
        duel->setSkillName("_thmieyi");
        duel->setCancelable(false);
        if (!target->isProhibited(ask_who, duel))
            room->useCard(CardUseStruct(duel, target, ask_who));
        else
            delete duel;

        return false;
    }
};

class ThShili : public TriggerSkill
{
public:
    ThShili()
        : TriggerSkill("thshili")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Play;
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
        QList<int> &drawPile = room->getDrawPile();
        QList<int> card_ids;
        for (int i = 0; i < 2; i++) {
            room->getThread()->trigger(FetchDrawPileCard, room, NULL);
            if (drawPile.isEmpty())
                room->swapPile();
            card_ids << drawPile.takeLast();
        }
        CardsMoveStruct move(card_ids, player, Player::PlaceTable,
                             CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        room->getThread()->delay();
        room->getThread()->delay();

        bool same = Sanguosha->getCard(card_ids.first())->sameColorWith(Sanguosha->getCard(card_ids.last()));
        if (same) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcards(card_ids);
            slash->setSkillName("_thshili");
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (player->canSlash(p, slash))
                    targets << p;
            }

            ServerPlayer *target = NULL;
            if (!targets.isEmpty())
                target = room->askForPlayerChosen(player, targets, objectName(), "@dummy-slash", true);
            if (target)
                room->useCard(CardUseStruct(slash, player, target));
            else {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
                room->throwCard(slash, reason, NULL);
                delete slash;
            }
        } else {
            room->fillAG(card_ids, player);
            int id = room->askForAG(player, card_ids, false, objectName());
            card_ids.removeOne(id);
            room->clearAG(player);
            room->obtainCard(player, id, true);

            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
            room->moveCardTo(Sanguosha->getCard(card_ids.first()), NULL, Player::DiscardPile, reason, true);
        }

        return false;
    }
};

class ThYuchi : public TriggerSkill
{
public:
    ThYuchi()
        : TriggerSkill("thyuchi")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList list;
        if (player->getPhase() == Player::Play && !player->isKongcheng() && Slash::IsAvailable(player)) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != player)
                    list.insert(p, QStringList(objectName()));
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
        QString pattern = ".";
        if (player->getHandcardNum() > ask_who->getHandcardNum())
            pattern += "!";
        const Card *card = room->askForCard(player, pattern, "@thyuchi:" + ask_who->objectName(), QVariant(), Card::MethodNone);
        if (pattern.endsWith("!") && !card)
            card = player->getRandomHandCard();
        if (card) {
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), ask_who->objectName(), objectName(),
                                  QString());
            room->obtainCard(ask_who, card, reason, false);

            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_thyuchi");
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->canSlash(p, slash))
                    targets << p;
            }

            if (targets.isEmpty()) {
                delete slash;
                return false;
            }

            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@dummy-slash");
            room->useCard(CardUseStruct(slash, player, target), true);
        }
        return false;
    }
};

class ThSancai : public TriggerSkill
{
public:
    ThSancai()
        : TriggerSkill("thsancai")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Start
            && player->getHandcardNum() > player->getMaxCards())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        QList<ServerPlayer *> targets;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_thsancai");
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->canSlash(player, slash, false))
                targets << p;
        }
        if (targets.isEmpty())
            delete slash;
        else {
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            room->useCard(CardUseStruct(slash, target, player));
        }

        player->drawCards(1, objectName());
        return false;
    }
};

ThRuizhiCard::ThRuizhiCard()
{
}

bool ThRuizhiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void ThRuizhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    targets.first()->drawCards(3, "thruizhi");
    room->setPlayerProperty(source, "thruizhi_step", 2);
}

class ThRuizhiVS : public OneCardViewAsSkill
{
public:
    ThRuizhiVS()
        : OneCardViewAsSkill("thruizhi")
    {
        filter_pattern = ".!";
        response_pattern = "@@thruizhi";
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        ThRuizhiCard *card = new ThRuizhiCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class ThRuizhi : public TriggerSkill
{
public:
    ThRuizhi()
        : TriggerSkill("thruizhi")
    {
        events << Damaged;
        view_as_skill = new ThRuizhiVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        if (!TriggerSkill::triggerable(target))
            return false;
        if (target->getSkillStep(objectName()) == 2) {
            foreach (const Player *p, target->getAliveSiblings()) {
                if (p->canDiscard(p, "he"))
                    return true;
            }
        }
        return true;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getSkillStep(objectName()) < 2) {
            room->askForUseCard(player, "@@thruizhi", "@thruizhi", -1, Card::MethodDiscard);
            return false;
        }
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getSkillStep(objectName()) < 2)
            return false;

        int n = 0;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->canDiscard(p, "he"))
                continue;
            if (room->askForDiscard(p, objectName(), 1, 1, true, true)) {
                n++;
                if (n == 2 && player->isWounded())
                    room->recover(player, RecoverStruct(player));
            }
        }

        room->setPlayerProperty(player, "thruizhi_step", 1);
        return false;
    }
};

ThLingweiCard::ThLingweiCard()
{
}

bool ThLingweiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *card = NULL;
    if (!user_string.isEmpty()) {
        card = Sanguosha->cloneCard(user_string.split("+").first());
        card->deleteLater();
    }
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool ThLingweiCard::targetFixed() const
{
    Card *card = NULL;
    if (!user_string.isEmpty()) {
        card = Sanguosha->cloneCard(user_string.split("+").first());
        card->deleteLater();
    }
    return card && card->targetFixed();
}

bool ThLingweiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card *card = NULL;
    if (!user_string.isEmpty()) {
        card = Sanguosha->cloneCard(user_string.split("+").first());
        card->deleteLater();
    }
    return card && card->targetsFeasible(targets, Self);
}

const Card *ThLingweiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    QString to_use;
    if (user_string == "peach+analeptic") {
        QStringList to_uses;
        to_uses << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            to_uses << "analeptic";
        to_use = room->askForChoice(user, "thlingwei_saveself", to_uses.join("+"));
    } else
        to_use = user_string;

    ServerPlayer *source = room->findPlayer(user->property("thlingweisource").toString());
    Q_ASSERT(source && source->isAlive());

    if (user == source) {
        if (to_use == "slash") {
            Q_ASSERT(subcardsLength() == 1);
            LogMessage log;
            log.type = "$DiscardCardWithSkill";
            log.from = source;
            log.card_str = QString::number(getEffectiveId());
            log.arg = "thlingwei";
            room->sendLog(log);
            room->notifySkillInvoked(source, "thlingwei");
            CardMoveReason reason(CardMoveReason::S_REASON_THROW, source->objectName());
            room->moveCardTo(this, source, NULL, Player::DiscardPile, reason, true);
        } else {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = source;
            log.arg = "thlingwei";
            room->sendLog(log);

            room->loseMaxHp(source);
        }
    } else {
        if (to_use == "slash") {
            if (!room->askForCard(source, ".", "@thlingwei-slash:" + user->objectName(), QVariant(), "thlingwei")) {
                room->setPlayerFlag(user, "Global_ThLingweiFailed");
                return NULL;
            }
        } else {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = user;
            log.arg = "thlingwei";
            room->sendLog(log);

            if (source->askForSkillInvoke(objectName(), QVariant::fromValue(user)))
                room->loseMaxHp(source);
            else {
                room->setPlayerFlag(user, "Global_ThLingweiFailed");
                return NULL;
            }
        }
    }

    if (!user->isAlive())
        return NULL;
    Card *use_card = Sanguosha->cloneCard(to_use);
    use_card->setSkillName("_thlingwei");
    return use_card;
}

const Card *ThLingweiCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();
    QString to_use = user_string;

    ServerPlayer *source = room->findPlayer(user->property("thlingweisource").toString());
    Q_ASSERT(source && source->isAlive());

    if (user == source) {
        if (to_use == "slash") {
            Q_ASSERT(subcardsLength() == 1);
            LogMessage log;
            log.type = "$DiscardCardWithSkill";
            log.from = source;
            log.card_str = QString::number(getEffectiveId());
            log.arg = "thlingwei";
            room->sendLog(log);
            room->notifySkillInvoked(source, "thlingwei");
            CardMoveReason reason(CardMoveReason::S_REASON_THROW, source->objectName());
            room->moveCardTo(this, source, NULL, Player::DiscardPile, reason, true);
        } else {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = source;
            log.arg = "thlingwei";
            room->sendLog(log);

            room->loseMaxHp(source);
        }
    } else {
        if (to_use == "slash") {
            if (!room->askForCard(source, ".", "@thlingwei-slash:" + user->objectName(), QVariant(), "thlingwei")) {
                room->setPlayerFlag(user, "Global_ThLingweiFailed");
                return NULL;
            }
        } else {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = user;
            log.arg = "thlingwei";
            room->sendLog(log);

            if (source->askForSkillInvoke(objectName(), QVariant::fromValue(user)))
                room->loseMaxHp(source);
            else {
                room->setPlayerFlag(user, "Global_ThLingweiFailed");
                return NULL;
            }
        }
    }

    if (!user->isAlive())
        return NULL;

    Card *use_card = Sanguosha->cloneCard(to_use);
    use_card->setSkillName("_thlingwei");
    return use_card;
}

class ThLingweiGivenSkill : public ZeroCardViewAsSkill
{
public:
    ThLingweiGivenSkill()
        : ZeroCardViewAsSkill("thlingweiv")
    {
        attached_lord_skill = true;
    }

    virtual SkillDialog *getDialog() const
    {
        return ThMimengDialog::getInstance("thlingwei", true, false, true, true);
    }

    virtual const Card *viewAs() const
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            ThLingweiCard *tianyan_card = new ThLingweiCard;
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";
            tianyan_card->setUserString(pattern);
            return tianyan_card;
        }

        Q_ASSERT(Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE);

        const Card *c = Self->tag["thlingwei"].value<const Card *>();
        if (c) {
            ThLingweiCard *card = new ThLingweiCard;
            card->setUserString(c->objectName());
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasFlag("Global_ThLingweiFailed"))
            return false;

        if (player->getMark("@assistant") > 0) {
            const Player *source = NULL;
            QString obj = player->property("thlingweisource").toString();
            if (player->objectName() == obj)
                source = player;
            else {
                foreach (const Player *p, player->getSiblings()) {
                    if (p->objectName() == obj) {
                        source = p;
                        break;
                    }
                }
            }

            if (!source || source->isDead())
                return false;

            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->deleteLater();
            if (slash->isAvailable(player) && source->canDiscard(source, "h"))
                return true;

            Peach *peach = new Peach(Card::NoSuit, 0);
            peach->deleteLater();
            if (peach->isAvailable(player))
                return true;

            Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
            analeptic->deleteLater();
            if (analeptic->isAvailable(player))
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->hasFlag("Global_ThLingweiFailed"))
            return false;

        if (player->getMark("@assistant") > 0) {
            const Player *source = NULL;
            QString obj = player->property("thlingweisource").toString();
            if (player->objectName() == obj)
                source = player;
            else {
                foreach (const Player *p, player->getSiblings()) {
                    if (p->objectName() == obj) {
                        source = p;
                        break;
                    }
                }
            }

            if (!source || source->isDead())
                return false;

            if (pattern == "slash") {
                if (source->canDiscard(source, "h")
                    && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
                    return true;
            } else if (pattern == "peach")
                return player->getMark("Global_PreventPeach") == 0;
            else
                return pattern.contains("analeptic");
        }
        return false;
    }
};

class ThLingweiViewAsSkill : public ViewAsSkill
{
public:
    ThLingweiViewAsSkill()
        : ViewAsSkill("thlingwei")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (!selected.isEmpty())
            return false;

        QString obj = Self->property("thlingweisource").toString();
        if (Self->objectName() == obj) {
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
                if (Sanguosha->getCurrentCardUsePattern() == "slash")
                    return !to_select->isEquipped() && !Self->isJilei(to_select);
            } else {
                const Card *card = Self->tag["thlingwei"].value<const Card *>();
                if (card && card->isKindOf("Slash"))
                    return !to_select->isEquipped() && !Self->isJilei(to_select);
            }
        }

        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        QString obj = Self->property("thlingweisource").toString();
        if (Self->objectName() == obj) {
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
                if (Sanguosha->getCurrentCardUsePattern() == "slash") {
                    if (cards.isEmpty())
                        return NULL;
                }
            } else {
                const Card *card = Self->tag["thlingwei"].value<const Card *>();
                if (card && card->isKindOf("Slash")) {
                    if (cards.isEmpty())
                        return NULL;
                }
            }
        }

        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            ThLingweiCard *tianyan_card = new ThLingweiCard;
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";
            tianyan_card->setUserString(pattern);
            tianyan_card->addSubcards(cards);
            return tianyan_card;
        }

        Q_ASSERT(Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE);

        const Card *c = Self->tag["thlingwei"].value<const Card *>();
        if (c) {
            ThLingweiCard *card = new ThLingweiCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasFlag("Global_ThLingweiFailed"))
            return false;

        if (player->getMark("@assistant") > 0) {
            const Player *source = NULL;
            QString obj = player->property("thlingweisource").toString();
            if (player->objectName() == obj)
                source = player;
            else {
                foreach (const Player *p, player->getSiblings()) {
                    if (p->objectName() == obj) {
                        source = p;
                        break;
                    }
                }
            }

            if (!source || source->isDead())
                return false;

            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->deleteLater();
            if (slash->isAvailable(player) && source->canDiscard(source, "h"))
                return true;

            Peach *peach = new Peach(Card::NoSuit, 0);
            peach->deleteLater();
            if (peach->isAvailable(player))
                return true;

            Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
            analeptic->deleteLater();
            if (analeptic->isAvailable(player))
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->hasFlag("Global_ThLingweiFailed"))
            return false;

        if (player->getMark("@assistant") > 0) {
            const Player *source = NULL;
            QString obj = player->property("thlingweisource").toString();
            if (player->objectName() == obj)
                source = player;
            else {
                foreach (const Player *p, player->getSiblings()) {
                    if (p->objectName() == obj) {
                        source = p;
                        break;
                    }
                }
            }

            if (!source || source->isDead())
                return false;

            if (pattern == "slash") {
                if (source->canDiscard(source, "h")
                    && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
                    return true;
            } else if (pattern == "peach")
                return player->getMark("Global_PreventPeach") == 0;
            else
                return pattern.contains("analeptic");
        }
        return false;
    }
};

class ThLingwei : public TriggerSkill
{
public:
    ThLingwei()
        : TriggerSkill("thlingwei")
    {
        events << EventAcquireSkill << EventLoseSkill << GameStart << EventPhaseStart << CardAsked;
        view_as_skill = new ThLingweiViewAsSkill;
    }

    virtual SkillDialog *getDialog() const
    {
        return ThMimengDialog::getInstance("thlingwei", true, false, true, true);
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventAcquireSkill && data.toString() == "thlingwei") {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@assistant") > 0) {
                    if (p->hasSkill("thlingwei"))
                        continue;

                    if (!p->hasSkill("thlingweiv") && p->property("thlingweisource").toString() == player->objectName())
                        room->attachSkillToPlayer(p, "thlingweiv");
                    break;
                }
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "thlingwei") {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@assistant") > 0) {
                    if (p->hasSkill("thlingweiv") && p->property("thlingweisource").toString() == player->objectName())
                        room->detachSkillFromPlayer(p, "thlingweiv", true);
                }
            }
            return QStringList();
        } else if (triggerEvent == GameStart || (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play)) {
            if (TriggerSkill::triggerable(player))
                return QStringList(objectName());
        } else if (triggerEvent == CardAsked && player && player->isAlive() && player->getMark("@assistant") > 0) {
            ServerPlayer *source = room->findPlayer(player->property("thlingweisource").toString());
            if (TriggerSkill::triggerable(source)) {
                QString pattern = data.toStringList().first();
                if (pattern == "slash" && source->canDiscard(source, "h"))
                    return QStringList(objectName());
                else if (pattern == "jink"
                         && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == GameStart || triggerEvent == EventPhaseStart) {
            ServerPlayer *target
                = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@thlingwei", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThLingweiTarget"] = QVariant::fromValue(target);
                return true;
            }
        } else if (triggerEvent == CardAsked) {
            ServerPlayer *source = room->findPlayer(player->property("thlingweisource").toString());
            if (player != source) {
                if (player->askForSkillInvoke(objectName())) {
                    room->broadcastSkillInvoke(objectName());
                    return true;
                }
            } else {
                QString pattern = data.toStringList().first();
                if (pattern == "slash") {
                    if (room->askForCard(source, ".", "@thlingwei-slash-self", QVariant(), objectName()))
                        return true;
                } else {
                    if (player->askForSkillInvoke(objectName())) {
                        room->broadcastSkillInvoke(objectName());
                        room->loseMaxHp(source);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == GameStart || triggerEvent == EventPhaseStart) {
            ServerPlayer *target = player->tag["ThLingweiTarget"].value<ServerPlayer *>();
            player->tag.remove("ThLingweiTarget");
            if (target) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark("@assistant") > 0) {
                        p->loseAllMarks("@assistant");
                        room->setPlayerProperty(p, "thlingweisource", "");
                        if (p->hasSkill("thlingweiv"))
                            room->detachSkillFromPlayer(p, "thlingweiv", true);
                    }
                }

                target->gainMark("@assistant");
                room->setPlayerProperty(target, "thlingweisource", player->objectName());
                if (!target->hasSkill("thlingweiv") && !target->hasSkill("thlingwei"))
                    room->attachSkillToPlayer(target, "thlingweiv");
            }
        } else if (triggerEvent == CardAsked) {
            ServerPlayer *source = room->findPlayer(player->property("thlingweisource").toString());
            QString pattern = data.toStringList().first();
            if (player == source) {
                Card *card = Sanguosha->cloneCard(pattern);
                card->setSkillName("_thlingwei");
                room->provide(card);
                return true;
            } else {
                if (pattern == "slash") {
                    if (room->askForCard(source, ".", "@thlingwei-slash:" + player->objectName(), QVariant(), objectName())) {
                        Slash *slash = new Slash(Card::NoSuit, 0);
                        slash->setSkillName("_thlingwei");
                        room->provide(slash);
                        return true;
                    }
                } else {
                    if (source->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
                        room->loseMaxHp(source);

                        Jink *jink = new Jink(Card::NoSuit, 0);
                        jink->setSkillName("_thlingwei");
                        room->provide(jink);

                        return true;
                    }
                }
            }
        }
        return false;
    }
};

ThMinwangCard::ThMinwangCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = MethodNone;
}

void ThMinwangCard::onUse(Room *, const CardUseStruct &) const
{
    // do nothing
}

class ThMinwangViewAsSkill : public ViewAsSkill
{
public:
    ThMinwangViewAsSkill()
        : ViewAsSkill("thminwang")
    {
        response_pattern = "@@thminwang!";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;

        if (selected.length() > 2)
            return false;

        foreach (const Card *card, selected) {
            if (to_select->getNumber() == card->getNumber())
                return false;
        }

        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (!cards.isEmpty()) {
            ThMinwangCard *card = new ThMinwangCard;
            card->addSubcards(cards);
            return card;
        }
        return NULL;
    }
};

class ThMinwang : public TriggerSkill
{
public:
    ThMinwang()
        : TriggerSkill("thminwang")
    {
        events << BuryVictim;
        view_as_skill = new ThMinwangViewAsSkill;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const
    {
        TriggerList skill_list;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            skill_list.insert(p, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *victim, QVariant &, ServerPlayer *player) const
    {
        QList<ServerPlayer *> _player;
        _player.append(player);
        QList<int> card_ids = room->getNCards(3, false);

        CardsMoveStruct move(card_ids, NULL, player, Player::PlaceTable, Player::PlaceHand,
                             CardMoveReason(CardMoveReason::S_REASON_PREVIEW, player->objectName(), objectName(), QString()));
        QList<CardsMoveStruct> moves;
        moves.append(move);
        room->notifyMoveCards(true, moves, false, _player);
        room->notifyMoveCards(false, moves, false, _player);

        //QList<int> origin_ids = card_ids;
        QList<int> put_ids;
        const Card *card = room->askForUseCard(player, "@@thminwang!", "@thminwang", -1, Card::MethodNone);
        if (card)
            put_ids = card->getSubcards();

        if (put_ids.isEmpty())
            put_ids << card_ids.first();

        move = CardsMoveStruct(card_ids, player, NULL, Player::PlaceHand, Player::PlaceTable,
                               CardMoveReason(CardMoveReason::S_REASON_PREVIEW, player->objectName(), objectName(), QString()));

        moves.clear();
        moves.append(move);
        room->notifyMoveCards(true, moves, false, _player);
        room->notifyMoveCards(false, moves, false, _player);

        player->addToPile("feed", put_ids);

        foreach (int id, put_ids)
            card_ids.removeOne(id);

        if (!card_ids.isEmpty()) {
            DummyCard *dummy = new DummyCard(card_ids);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), objectName(), QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
        }

        QString gnr_str = victim->getGeneralName();
        QStringList targets = player->tag["ThMinwangTargets"].toStringList();
        targets << gnr_str;
        player->tag["ThMinwangTargets"] = QVariant::fromValue(targets);
        player->tag["ThMinwang_" + gnr_str] = QVariant::fromValue(IntList2StringList(put_ids));

        return false;
    }
};

class ThLingbo : public TriggerSkill
{
public:
    ThLingbo()
        : TriggerSkill("thlingbo")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (event == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play && !player->getPile("feed").isEmpty())
                return QStringList(objectName());
        } else if (player->hasFlag("ThLingboUsed") && data.value<PhaseChangeStruct>().from == Player::Play) {
            player->setFlags("-ThLingboUsed");

            QStringList remove_list;
            QStringList skill_list = player->tag["ThMinwangSkillList"].toStringList();
            player->tag.remove("ThMinwangSkillList");
            foreach (QString name, skill_list)
                remove_list << "-" + name;
            room->handleAcquireDetachSkills(player, remove_list.join("|"), true, true);

            QList<int> hand = player->handCards();
            player->addToPile("feed", hand);
            QList<int> to_back = StringList2IntList(player->tag["ThMinwangHandCards"].toStringList());
            DummyCard *dummy = new DummyCard(to_back);
            CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, player->objectName());
            room->obtainCard(player, dummy, reason, false);
            delete dummy;
            player->tag.remove("ThMinwangHandCards");
            dummy = new DummyCard(hand);
            CardMoveReason reason2(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
            room->throwCard(dummy, reason2, NULL);
            delete dummy;
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
        player->setFlags("ThLingboUsed");
        QStringList gnr_list = player->tag["ThMinwangTargets"].toStringList();

        QStringList _gnr_list = gnr_list;
        _gnr_list.prepend("mute");
        QString general_name = room->askForGeneral(player, _gnr_list);
        const General *general = Sanguosha->getGeneral(general_name);

        gnr_list.removeOne(general_name);
        player->tag["ThMinwangTargets"] = QVariant::fromValue(gnr_list);
        // for handcards
        QList<int> hand = player->handCards();
        player->tag["ThMinwangHandCards"] = QVariant::fromValue(IntList2StringList(hand));
        player->addToPile("feed", hand, false);
        QString tag_str = "ThMinwang_" + general_name;
        DummyCard *dummy = new DummyCard(StringList2IntList(player->tag[tag_str].toStringList()));
        player->tag.remove(tag_str);
        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, player->objectName());
        room->obtainCard(player, dummy, reason);
        delete dummy;

        QList<const Skill *> skills = general->getVisibleSkillList();
        QStringList list;
        foreach (const Skill *skill, skills) {
            if (!player->hasSkill(skill->objectName()))
                list << skill->objectName();
        }
        if (!list.isEmpty()) {
            room->handleAcquireDetachSkills(player, list.join("|"), true, true);
            player->tag["ThMinwangSkillList"] = QVariant::fromValue(list);
        }

        return false;
    }
};

ThYuguangCard::ThYuguangCard()
{
}

bool ThYuguangCard::CompareBySuit(int card1, int card2)
{
    const Card *c1 = Sanguosha->getCard(card1);
    const Card *c2 = Sanguosha->getCard(card2);

    return SkillCard::CompareBySuit(c1, c2);
}

bool ThYuguangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThYuguangCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->showAllCards(effect.to);

    QMap<Suit, QList<int> > yuguangMap;
    QList<int> card_ids = effect.to->handCards();
    qSort(card_ids.begin(), card_ids.end(), CompareBySuit);

    foreach (int id, card_ids) {
        Suit suit = Sanguosha->getCard(id)->getSuit();
        if (!yuguangMap.contains(suit))
            yuguangMap[suit] = QList<int>();
        yuguangMap[suit] << id;
    }

    QList<int> choose_ids, disabled_ids;
    foreach (Suit suit, yuguangMap.keys()) {
        if (yuguangMap[suit].length() < 2) {
            disabled_ids << yuguangMap[suit];
            continue;
        }

        foreach (int id, yuguangMap[suit]) {
            if (!effect.from->canDiscard(effect.to, id))
                disabled_ids << id;
            else
                choose_ids << id;
        }
    }

    QList<int> to_discard;
    while (!choose_ids.isEmpty()) {
        room->fillAG(card_ids, effect.from, disabled_ids);
        int card_id = room->askForAG(effect.from, choose_ids, false, "thyuguang");
        room->clearAG(effect.from);
        to_discard << card_id;
        Suit suit = Sanguosha->getCard(card_id)->getSuit();
        yuguangMap[suit].removeOne(card_id);
        choose_ids = yuguangMap[suit];
        disabled_ids << card_id;

        if (choose_ids.length() == 1)
            break;

        foreach (Suit m_suit, yuguangMap.keys()) {
            if (m_suit == suit)
                continue;
            if (yuguangMap[m_suit].length() < 2)
                continue;
            disabled_ids << yuguangMap[m_suit];
        }
    }

    if (!to_discard.isEmpty()) {
        DummyCard *dummy = new DummyCard(to_discard);
        room->throwCard(dummy, effect.to, effect.from);
    }
}

class ThYuguang : public ZeroCardViewAsSkill
{
public:
    ThYuguang()
        : ZeroCardViewAsSkill("thyuguang")
    {
    }

    virtual const Card *viewAs() const
    {
        return new ThYuguangCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThYuguangCard");
    }
};

class ThGuidu : public TriggerSkill
{
public:
    ThGuidu()
        : TriggerSkill("thguidu")
    {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getHandcardNum() < p->getHp())
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
        int n = 0;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getHandcardNum() < p->getHp())
                ++n;
        }

        player->drawCards(n, objectName());

        QList<int> card_ids = player->handCards();
        if (player->getHandcardNum() > n) {
            const Card *dummy = room->askForExchange(player, objectName(), n, n, false, "@thguidu", false);
            if (dummy) {
                card_ids = dummy->getSubcards();
                room->showCard(player, card_ids);
                delete dummy;
            }
        }

        room->fillAG(card_ids);
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getHandcardNum() < p->getHp()) {
                int id = room->askForAG(p, card_ids, false, objectName());
                if (id == -1)
                    id = card_ids.first();
                card_ids.removeOne(id);
                room->takeAG(p, id, false);
                room->obtainCard(p, id);
                if (card_ids.isEmpty())
                    break;
            }
        }
        room->clearAG();

        return false;
    }
};

ThCanfeiCard::ThCanfeiCard()
{
    will_throw = true;
    can_recast = true;
    handling_method = MethodRecast;
    target_fixed = true;
}

void ThCanfeiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    CardMoveReason reason(CardMoveReason::S_REASON_RECAST, source->objectName());
    reason.m_skillName = "thcanfei";
    room->moveCardTo(this, source, NULL, Player::DiscardPile, reason);
    room->setEmotion(source, "effects/recast");

    LogMessage log;
    log.type = "#UseCard_Recast";
    log.from = source;
    log.card_str = toString();
    room->sendLog(log);

    source->drawCards(subcardsLength(), "recast");

    room->setPlayerFlag(source, QString("ThCanfei%1").arg(Sanguosha->getCard(subcards.first())->getNumber()));
    if (subcardsLength() > 1)
        room->setPlayerFlag(source, QString("ThCanfei%1").arg(Sanguosha->getCard(subcards.last())->getNumber()));

    room->setPlayerFlag(source, "ThCanfeiUsed");
}

#include "ikai-kin.h"
class ThCanfeiVS : public ViewAsSkill
{
public:
    ThCanfeiVS()
        : ViewAsSkill("thcanfei")
    {
        response_pattern = "@@thcanfei";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        const Card *coll = Card::Parse(Self->property("extra_collateral").toString());
        if (coll)
            return false;

        if (selected.length() > 1 || to_select->isEquipped())
            return false;

        return selected.isEmpty() || to_select->getNumber() != selected.first()->getNumber();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        const Card *coll = Card::Parse(Self->property("extra_collateral").toString());
        if (coll) {
            if (coll->isKindOf("Collateral"))
                return new ExtraCollateralCard;
            else
                return new ExtraFeintAttackCard;
        }
        if (!cards.isEmpty()) {
            ThCanfeiCard *card = new ThCanfeiCard;
            card->addSubcards(cards);
            return card;
        }

        return NULL;
    }
};

class ThCanfei : public TriggerSkill
{
public:
    ThCanfei()
        : TriggerSkill("thcanfei")
    {
        events << EventPhaseStart << PreCardUsed;
        view_as_skill = new ThCanfeiVS;
    }

    virtual QStringList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (event == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play && !player->isKongcheng())
                return QStringList(objectName());
        } else {
            if (!player || player->isDead() || !player->hasFlag("ThCanfeiUsed"))
                return QStringList();
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->hasFlag(QString("ThCanfei%1").arg(use.card->getNumber()))
                && (use.card->isKindOf("Peach") || use.card->isKindOf("Analeptic") || use.card->isKindOf("ExNihilo")
                    || use.card->isKindOf("Collateral") || use.card->isKindOf("FeintAttack"))) {
                if (use.card->isKindOf("Peach") || use.card->isKindOf("Analeptic") || use.card->isKindOf("ExNihilo")) {
                    foreach (ServerPlayer *p, room->getAlivePlayers()) {
                        if (!use.to.contains(p) && !room->isProhibited(player, p, use.card))
                            return QStringList(objectName());
                    }
                } else if (use.card->isKindOf("Collateral")) {
                    foreach (ServerPlayer *p, room->getAlivePlayers()) {
                        if (use.to.contains(p) || room->isProhibited(player, p, use.card))
                            continue;
                        if (use.card->targetFilter(QList<const Player *>(), p, player))
                            return QStringList(objectName());
                    }
                } else if (use.card->isKindOf("FeintAttack")) {
                    foreach (ServerPlayer *p, room->getAlivePlayers()) {
                        if (use.to.contains(p) || room->isProhibited(player, p, use.card))
                            continue;
                        if (use.card->targetFilter(QList<const Player *>(), p, player))
                            return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent event, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        if (event == EventPhaseStart) {
            return room->askForUseCard(player, "@@thcanfei", "@thcanfei", -1, Card::MethodRecast);
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            ServerPlayer *extra = NULL;
            if (use.card->isKindOf("Peach")) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->isWounded() && !use.to.contains(p) && !room->isProhibited(player, p, use.card))
                        targets << p;
                }
                extra = room->askForPlayerChosen(player, targets, objectName(), "@thyongye-add:::" + use.card->objectName(),
                                                 true);
            } else if (use.card->isKindOf("Analeptic") || use.card->isKindOf("ExNihilo")) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!use.to.contains(p) && !room->isProhibited(player, p, use.card))
                        targets << p;
                }
                extra = room->askForPlayerChosen(player, targets, objectName(), "@thyongye-add:::" + use.card->objectName(),
                                                 true);
            } else if (use.card->isKindOf("Collateral") || use.card->isKindOf("FeintAttack")) {
                QStringList tos;
                foreach (ServerPlayer *t, use.to)
                    tos.append(t->objectName());
                room->setPlayerProperty(player, "extra_collateral", use.card->toString());
                room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
                bool used = room->askForUseCard(player, "@@thcanfei", "@thyongye-add:::" + use.card->objectName(),
                                                use.card->isKindOf("Collateral") ? 2 : 3);
                room->setPlayerProperty(player, "extra_collateral", QString());
                room->setPlayerProperty(player, "extra_collateral_current_targets", QString());
                if (!used)
                    return false;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasFlag("ExtraCollateralTarget")) {
                        p->setFlags("-ExtraCollateralTarget");
                        extra = p;
                        break;
                    }
                }
            }
            if (extra) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThCanfeiTarget"] = QVariant::fromValue(extra);
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent event, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        if (event == EventPhaseStart)
            return false;
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *extra = player->tag["ThCanfeiTarget"].value<ServerPlayer *>();
        player->tag.remove("ThCanfeiTarget");
        if (extra) {
            use.to.append(extra);
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);

            LogMessage log;
            log.type = "#ThYongyeAdd";
            log.from = player;
            log.to << extra;
            log.arg = objectName();
            log.card_str = use.card->toString();
            room->sendLog(log);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());

            if (use.card->isKindOf("Collateral")) {
                ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                if (victim) {
                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = player;
                    log.to << victim;
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                }
            } else if (use.card->isKindOf("FeintAttack")) {
                ServerPlayer *victim = extra->tag["feintTarget"].value<ServerPlayer *>();
                if (victim) {
                    LogMessage log;
                    log.type = "#FeintAttackWest";
                    log.from = player;
                    log.to << victim;
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                }
            }
        }
        return false;
    }
};

class ThCanfeiDistance : public DistanceSkill
{
public:
    ThCanfeiDistance()
        : DistanceSkill("#thcanfei-distance")
    {
        frequency = NotCompulsory;
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasFlag("ThCanfeiUsed"))
            return -1;
        return 0;
    }
};

class ThCanfeiTargetMod : public TargetModSkill
{
public:
    ThCanfeiTargetMod()
        : TargetModSkill("#thcanfei-tar")
    {
        frequency = NotCompulsory;
        pattern = "BasicCard,SingleTargetTrick,IronChain,LureTiger"; // deal with Ex Nihilo and Collateral later
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const
    {
        if (from->hasFlag(QString("ThCanfei%1").arg(card->getNumber())))
            return 1;
        return 0;
    }
};

class ThChuanyu : public TriggerSkill
{
public:
    ThChuanyu()
        : TriggerSkill("thchuanyu")
    {
        events << Damaged;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player && player->isAlive() && player->hasSkill("thzuoyong") && !ThZuoyongDialog::hasAllNamed(player)) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        QStringList name_list = ThZuoyongDialog::chuanyuNames.split("+");
        QStringList choices;
        foreach (QString name, name_list) {
            if (!ThZuoyongDialog::hasNamed(name, player))
                choices << name;
        }

        Q_ASSERT(!choices.isEmpty());

        QString choice = room->askForChoice(player, objectName(), choices.join("+"));

        LogMessage log;
        log.type = "#ThChuanyu";
        log.from = player;
        log.arg = choice;
        room->sendLog(log);

        int name = ThZuoyongDialog::chuanyuMap[choice];

        int mark_index = 1;
        while (name != 1) {
            name = name >> 1;
            ++mark_index;
        }
        room->addPlayerMark(player, QString("@chuanyu%1").arg(mark_index));

        int n = player->getMark("thchuanyu");
        n |= ThZuoyongDialog::chuanyuMap[choice];
        foreach (ServerPlayer *p, room->getAlivePlayers())
            room->setPlayerMark(p, "thchuanyu", n);

        return false;
    }
};

const QString &ThZuoyongDialog::chuanyuNames = "jink+analeptic+nullification+fire_attack";
QMap<QString, ThZuoyongDialog::ThChuanyuName> ThZuoyongDialog::chuanyuMap;

bool ThZuoyongDialog::hasNamed(QString name, const Player *player)
{
    if (name == "slash")
        return true;

    if (chuanyuMap.isEmpty()) {
        chuanyuMap.insert("jink", Jink);
        chuanyuMap.insert("analeptic", Analeptic);
        chuanyuMap.insert("nullification", Nullification);
        chuanyuMap.insert("fire_attack", FireAttack);
    }

    return (player->getMark("thchuanyu") & chuanyuMap[name]) != 0;
}

bool ThZuoyongDialog::hasAllNamed(const Player *player)
{
    QStringList name_list = chuanyuNames.split("+");
    foreach (QString name, name_list) {
        if (!hasNamed(name, player))
            return false;
    }
    return true;
}

ThZuoyongDialog *ThZuoyongDialog::getInstance()
{
    static ThZuoyongDialog *instance;
    if (instance == NULL)
        instance = new ThZuoyongDialog();

    return instance;
}

ThZuoyongDialog::ThZuoyongDialog()
{
    setObjectName("thzuoyong");
    setWindowTitle(Sanguosha->translate("thzuoyong"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, (void (QButtonGroup::*)(QAbstractButton *))(&QButtonGroup::buttonClicked), this,
            &ThZuoyongDialog::selectCard);
}

void ThZuoyongDialog::popup()
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        Self->tag.remove("thzuoyong");
        emit onButtonClick();
        return;
    }

    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QStringList card_names;
    card_names << "slash" << chuanyuNames.split("+");

    foreach (QString card_name, card_names) {
        QCommandLinkButton *button = new QCommandLinkButton;
        button->setText(Sanguosha->translate(card_name));
        button->setObjectName(card_name);
        group->addButton(button);

        bool can = hasNamed(card_name, Self);
        if (can) {
            const Card *c = Sanguosha->cloneCard(card_name);
            if (Self->isCardLimited(c, Card::MethodUse) || !c->isAvailable(Self))
                can = false;
            delete c;
        }
        button->setEnabled(can);
        button_layout->addWidget(button);

        if (!map.contains(card_name)) {
            Card *c = Sanguosha->cloneCard(card_name);
            c->setParent(this);
            map.insert(card_name, c);
        }
    }

    exec();
}

void ThZuoyongDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag["thzuoyong"] = QVariant::fromValue(card);
    emit onButtonClick();
    accept();
}

class ThZuoyongViewAsSkill : public OneCardViewAsSkill
{
public:
    ThZuoyongViewAsSkill()
        : OneCardViewAsSkill("thzuoyong")
    {
        filter_pattern = "EquipCard";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        QString pattern;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            const Card *c = Self->tag["thzuoyong"].value<const Card *>();
            if (c == NULL)
                return NULL;
            pattern = c->objectName();
        } else {
            pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern.contains("peach+analeptic"))
                pattern = "analeptic";
        }

        Card *card = Sanguosha->cloneCard(pattern, originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName("thzuoyong");
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        QStringList card_names;
        card_names << "slash" << ThZuoyongDialog::chuanyuNames.split("+");
        foreach (QString card_name, card_names) {
            if (ThZuoyongDialog::hasNamed(card_name, player)) {
                Card *card = Sanguosha->cloneCard(card_name);
                bool ava = card->isAvailable(player);
                delete card;
                if (ava)
                    return true;
            }
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
        if (pattern.contains("analeptic"))
            return ThZuoyongDialog::hasNamed("analeptic", player);
        if (pattern == "slash" || pattern == "jink" || pattern == "nullification")
            return ThZuoyongDialog::hasNamed(pattern, player);
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return ThZuoyongDialog::hasNamed("nullification", player);
    }
};

class ThZuoyong : public TriggerSkill
{
public:
    ThZuoyong()
        : TriggerSkill("thzuoyong")
    {
        events << CardsMoveOneTime;
        view_as_skill = new ThZuoyongViewAsSkill;
    }

    virtual SkillDialog *getDialog() const
    {
        return ThZuoyongDialog::getInstance();
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceEquip) && move.is_last_equip)
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

class ThZaoxing : public TriggerSkill
{
public:
    ThZaoxing()
        : TriggerSkill("thzaoxing")
    {
        events << EventPhaseEnd << Death << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseEnd) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw) {
                foreach (const Card *c, player->getHandcards())
                    if (c->getTypeId() == Card::TypeBasic)
                        return QStringList(objectName());
            }
        }
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        }
        if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) || triggerEvent == Death) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (player->getMark("thzaoxing_" + p->objectName())) {
                    room->removePlayerMark(p, "@zaoxing", player->getMark("thzaoxing_" + p->objectName()));
                    player->setMark("thzaoxing_" + p->objectName(), 0);
                    room->handleAcquireDetachSkills(p, "-thzuoyong", true, true);
                }
            }
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target
            = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thzaoxing", true, true);
        if (target) {
            player->tag["ThZaoxingTarget"] = QVariant::fromValue(target);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["ThZaoxingTarget"].value<ServerPlayer *>();
        player->tag.remove("ThZaoxingTarget");
        if (target) {
            room->showAllCards(player);
            QList<int> card_ids;
            foreach (const Card *c, player->getHandcards()) {
                if (c->getTypeId() == Card::TypeBasic)
                    card_ids << c->getEffectiveId();
            }

            DummyCard *dummy = new DummyCard(card_ids);
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), objectName(), "");
            room->obtainCard(target, dummy, reason);
            delete dummy;
            player->addMark("thzaoxing_" + target->objectName());
            room->addPlayerMark(target, "@zaoxing");
            QStringList list;
            if (!target->hasSkill("thzuoyong"))
                list << "thzuoyong";
            if (!list.isEmpty())
                room->handleAcquireDetachSkills(target, list, true, true);
        }
        return false;
    }
};

ThGuiyuniuCard::ThGuiyuniuCard()
{
}

bool ThGuiyuniuCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void ThGuiyuniuCard::onEffect(const CardEffectStruct &effect) const
{
    QStringList targets = effect.from->tag["ThGuiyuniuTargets"].toString().split("+");
    targets << effect.to->objectName();
    effect.from->tag["ThGuiyuniuTargets"] = targets.join("+");
    effect.to->gainMark("@stable");
}

class ThGuiyuniuViewAsSkill : public OneCardViewAsSkill
{
public:
    ThGuiyuniuViewAsSkill()
        : OneCardViewAsSkill("thguiyuniu")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThGuiyuniuCard *card = new ThGuiyuniuCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "h") && !player->hasUsed("ThGuiyuniuCard");
    }
};

class ThGuiyuniu : public TriggerSkill
{
public:
    ThGuiyuniu()
        : TriggerSkill("thguiyuniu")
    {
        events << EventPhaseStart << Death;
        view_as_skill = new ThGuiyuniuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        }

        if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) || triggerEvent == Death) {
            if (!player->tag["ThGuiyuniuTargets"].toString().isEmpty()) {
                QStringList targets = player->tag["ThGuiyuniuTargets"].toString().split("+");
                player->tag.remove("ThGuiyuniuTargets");
                foreach (QString tar, targets) {
                    ServerPlayer *p = room->findPlayer(tar);
                    if (p)
                        p->loseMark("@stable");
                }
            }
        }

        return QStringList();
    }
};

class ThGuiyuniuDistance : public DistanceSkill
{
public:
    ThGuiyuniuDistance()
        : DistanceSkill("#thguiyuniu-distance")
    {
        frequency = NotCompulsory;
    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        return from->getMark("@stable") - to->getMark("@stable");
    }
};

ThHaixingCard::ThHaixingCard()
{
    will_throw = false;
    handling_method = MethodNone;
}

bool ThHaixingCard::targetFixed() const
{
    return subcards.isEmpty();
}

bool ThHaixingCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    if (targetFixed())
        return false;

    return targets.isEmpty();
}

bool ThHaixingCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targetFixed() == targets.isEmpty();
}

void ThHaixingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (subcards.isEmpty()) {
        room->removePlayerMark(source, "@haixing");
        room->addPlayerMark(source, "@haixingused");
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (!p->isNude()) {
                const Card *card = room->askForCard(p, "..!", "@thhaixing-put:" + source->objectName());
                if (!card) {
                    QList<const Card *> cards = p->getCards("he");
                    card = cards.at(qrand() % cards.length());
                }
                source->addToPile("seafood", card);
            }
        }
    } else
        SkillCard::use(room, source, targets);
}

void ThHaixingCard::onEffect(const CardEffectStruct &effect) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "thhaixing",
                          QString());
    Room *room = effect.from->getRoom();
    if (effect.from == effect.to) {
        LogMessage log;
        log.type = "$MoveCard";
        log.from = effect.from;
        log.to << effect.to;
        log.card_str = IntList2StringList(subcards).join("+");
        room->sendLog(log);
    }
    room->obtainCard(effect.to, this);
}

class ThHaixingViewAsSkill : public ViewAsSkill
{
public:
    ThHaixingViewAsSkill()
        : ViewAsSkill("thhaixing")
    {
        response_pattern = "@@thhaixing";
        expand_pile = "seafood";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return false;

        return selected.length() < 2 && Self->getPile("seafood").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return new ThHaixingCard;

        if (!cards.isEmpty()) {
            Card *card = new ThHaixingCard;
            card->addSubcards(cards);
            return card;
        }

        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("@haixing") == 0)
            return false;
        if (!player->isNude())
            return true;
        foreach (const Player *p, player->getSiblings()) {
            if (!p->isNude())
                return true;
        }
        return false;
    }
};

class ThHaixing : public TriggerSkill
{
public:
    ThHaixing()
        : TriggerSkill("thhaixing")
    {
        events << EventPhaseStart;
        view_as_skill = new ThHaixingViewAsSkill;
        frequency = Limited;
        limit_mark = "@haixing";
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish && !player->getPile("seafood").isEmpty())
            return QStringList(objectName());

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thhaixing", "@thhaixing", -1, Card::MethodNone);
    }
};

class ThZuisheng : public TriggerSkill
{
public:
    ThZuisheng()
        : TriggerSkill("thzuisheng")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (event == EventPhaseStart && player->getPhase() == Player::Play && player->canDiscard(player, "h")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                skill_list.insert(p, QStringList(objectName()));
        } else if (event == EventPhaseChanging && player->getMark("@zuisheng") > 0
                   && data.value<PhaseChangeStruct>().to == Player::NotActive) {
            room->removePlayerMark(player, "@zuisheng");
            room->detachSkillFromPlayer(player, "thzuishengv", true);
            room->setPlayerFlag(player, "-thzuishengv");
            room->removePlayerMark(player, "@skill_invalidity");

            foreach (ServerPlayer *pl, room->getAllPlayers())
                room->filterCards(pl, pl->getHandcards(), false);
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (player == ask_who) {
            return room->askForCard(ask_who, ".", "@thzuisheng", data, objectName());
        } else {
            if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (player == ask_who || room->askForDiscard(player, objectName(), 1, 1, true, false, "@thzuisheng")) {
            if (player->getMark("@zuisheng") == 0) {
                room->addPlayerMark(player, "@zuisheng");
                room->attachSkillToPlayer(player, "thzuishengv");
                room->setPlayerFlag(player, "thzuishengv");
                room->addPlayerMark(player, "@skill_invalidity");

                foreach (ServerPlayer *pl, room->getAllPlayers())
                    room->filterCards(pl, pl->getHandcards(), true);
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }
        }
        return false;
    }
};

class ThZuishengVS : public OneCardViewAsSkill
{
public:
    ThZuishengVS()
        : OneCardViewAsSkill("thzuishengv")
    {
        attached_lord_skill = true;
        response_pattern = "peach+analeptic";
        response_or_use = true;
        filter_pattern = "BasicCard|red";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Analeptic *anal = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        anal->addSubcard(originalCard);
        anal->setSkillName(objectName());
        return anal;
    }
};

class ThZuishengProhibit : public ProhibitSkill
{
public:
    ThZuishengProhibit()
        : ProhibitSkill("#thzuisheng")
    {
        frequency = NotCompulsory;
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        if (from && from->getMark("@zuisheng") > 0 && card->isKindOf("Slash")) {
            foreach (const Player *p, from->getAliveSiblings()) {
                if (from->distanceTo(p) < from->distanceTo(to))
                    return true;
            }
        }
        return false;
    }
};

class ThMengsi : public TriggerSkill
{
public:
    ThMengsi()
        : TriggerSkill("thmengsi")
    {
        events << EventPhaseChanging << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (event == EventPhaseChanging && data.value<PhaseChangeStruct>().to == Player::NotActive && player->isKongcheng()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p == player)
                    continue;
                if (p->getCardCount() > 1)
                    skill_list.insert(p, QStringList(objectName()));
            }
        } else if (event == EventPhaseStart && player->getPhase() == Player::RoundStart && player->getMark("@mengsi") > 0) {
            room->removePlayerMark(player, "@mengsi");
            room->handleAcquireDetachSkills(player, "-thmengxuan|-ikfusheng", true);
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        const Card *card = room->askForExchange(ask_who, objectName(), 2, 2, true, "@thmengsi:" + player->objectName(), true);
        if (card) {
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, ask_who->objectName(), player->objectName(), objectName(),
                                  QString());
            room->obtainCard(player, card, reason, false);
            delete card;
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getMark("@mengsi") == 0) {
            room->addPlayerMark(player, "@mengsi");
            room->handleAcquireDetachSkills(player, "thmengxuan|ikfusheng");
        }
        return false;
    }
};

class ThLinglu : public TriggerSkill
{
public:
    ThLinglu()
        : TriggerSkill("thlinglu")
    {
        events << EventPhaseChanging;
        frequency = NotCompulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (player->isLord() && data.value<PhaseChangeStruct>().to == Player::RoundStart) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->getMark("@spirits") > 0)
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        room->broadcastSkillInvoke(objectName());
        int n = (qMin(4, ask_who->getMark("@spirits")) + 2) / 3;
        ask_who->drawCards(n, objectName());
        ask_who->loseAllMarks("@spirits");
        return false;
    }
};

class ThLingluGain : public TriggerSkill
{
public:
    ThLingluGain()
        : TriggerSkill("#thlinglu")
    {
        events << CardFinished;
        frequency = NotCompulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        TriggerList skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Analeptic")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName("thlinglu"))
                skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, "thlinglu");
        room->broadcastSkillInvoke("thlinglu");
        ask_who->gainMark("@spirits");
        return false;
    }
};

TouhouShinPackage::TouhouShinPackage()
    : Package("touhou-shin")
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

    General *shin006 = new General(this, "shin006", "hana", 3, false);
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
    shin007->addSkill(new ThGuzhen);

    General *shin008 = new General(this, "shin008", "tsuki");
    shin008->addSkill(new ThLianying);
    shin008->addSkill(new ThYuanxiao);

    General *shin009 = new General(this, "shin009", "kaze", 3);
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
    shin013->addSkill(new ThShenmi);

    General *shin014 = new General(this, "shin014", "hana");
    shin014->addSkill(new ThMuyu);

    General *shin015 = new General(this, "shin015", "yuki", 3);
    shin015->addSkill(new ThNihui);
    shin015->addSkill(new ThTanguan);
    shin015->addSkill(new ThTanguanTrigger);
    related_skills.insertMulti("thtanguan", "#thtanguan");

    General *shin016 = new General(this, "shin016", "tsuki", 3, false);
    shin016->addSkill(new ThYuancui);
    shin016->addSkill(new ThHuikuang);

    General *shin017 = new General(this, "shin017", "kaze", 3);
    shin017->addSkill(new ThXuanman);
    shin017->addSkill(new ThKuangwu);

    General *shin018 = new General(this, "shin018", "hana", 3);
    shin018->addSkill(new ThDieying);
    shin018->addSkill(new ThBiyi);

    General *shin019 = new General(this, "shin019", "yuki");
    shin019->addSkill(new ThTuna);
    shin019->addSkill(new ThNinggu);

    General *shin020 = new General(this, "shin020", "tsuki");
    shin020->addSkill(new ThQiongfaziyuan);
    shin020->addSkill(new MarkAssignSkill("@poor", 1));
    shin020->addSkill(new ThQiongfaSkill);
    related_skills.insertMulti("thqiongfaziyuan", "#@poor-1");
    related_skills.insertMulti("thqiongfaziyuan", "#thqiongfa");
    shin020->addRelateSkill("thchipin");
    shin020->addRelateSkill("thaimin");

    General *shin021 = new General(this, "shin021", "kaze", 3, false);
    shin021->addSkill(new ThShenhu);
    shin021->addSkill(new ThHouhu);

    General *shin022 = new General(this, "shin022", "hana");
    shin022->addSkill(new ThGuizhou);
    shin022->addSkill(new ThRenmo);
    shin022->addSkill(new ThRenMoTargetMod);
    related_skills.insertMulti("threnmo", "#threnmo");

    General *shin023 = new General(this, "shin023", "yuki", 3);
    shin023->addSkill(new ThMieyi);
    shin023->addSkill(new ThShili);
    shin023->addSkill(new SlashNoDistanceLimitSkill("thshili"));
    related_skills.insertMulti("thshili", "#thshili-slash-ndl");

    General *shin024 = new General(this, "shin024", "tsuki", 3);
    shin024->addSkill(new ThYuchi);
    shin024->addSkill(new ThSancai);

    General *shin025 = new General(this, "shin025", "kaze");
    shin025->addSkill(new ThRuizhi);

    General *shin026 = new General(this, "shin026", "hana", 5, 4);
    shin026->addSkill(new ThLingwei);

    General *shin027 = new General(this, "shin027", "yuki", 4, false);
    shin027->addSkill(new ThMinwang);
    shin027->addSkill(new ThLingbo);

    General *shin028 = new General(this, "shin028", "tsuki", 3);
    shin028->addSkill(new ThYuguang);
    shin028->addSkill(new ThGuidu);

    General *shin029 = new General(this, "shin029", "kaze");
    shin029->addSkill(new ThCanfei);
    shin029->addSkill(new ThCanfeiDistance);
    shin029->addSkill(new ThCanfeiTargetMod);
    related_skills.insertMulti("thcanfei", "#thcanfei-distance");
    related_skills.insertMulti("thcanfei", "#thcanfei-tar");

    General *shin030 = new General(this, "shin030", "hana", 3);
    shin030->addSkill(new ThChuanyu);
    shin030->addSkill(new ThZuoyong);
    shin030->addSkill(new ThZaoxing);

    General *shin031 = new General(this, "shin031", "yuki", 3);
    shin031->addSkill(new ThGuiyuniu);
    shin031->addSkill(new ThGuiyuniuDistance);
    related_skills.insertMulti("thguiyuniu", "#thguiyuniu-distance");
    shin031->addSkill(new ThHaixing);

    General *shin032 = new General(this, "shin032", "tsuki", 3);
    shin032->addSkill(new ThZuisheng);
    shin031->addSkill(new ThZuishengProhibit);
    related_skills.insertMulti("thzuisheng", "#thzuisheng");
    shin032->addSkill(new ThMengsi);
    shin032->addSkill(new ThLinglu);
    shin032->addSkill(new ThLingluGain);
    related_skills.insertMulti("thlinglu", "#thlinglu");

    addMetaObject<ThLuanshenCard>();
    addMetaObject<ThLianyingCard>();
    addMetaObject<ThMumiCard>();
    addMetaObject<ThHuanjianCard>();
    addMetaObject<ThShenmiCard>();
    addMetaObject<ThMuyuCard>();
    addMetaObject<ThNihuiCard>();
    addMetaObject<ThKuangwuCard>();
    addMetaObject<ThDieyingCard>();
    addMetaObject<ThBiyiCard>();
    addMetaObject<ThTunaCard>();
    addMetaObject<ThNingguCard>();
    addMetaObject<ThAiminCard>();
    addMetaObject<ThRenmoCard>();
    addMetaObject<ThRuizhiCard>();
    addMetaObject<ThLingweiCard>();
    addMetaObject<ThMinwangCard>();
    addMetaObject<ThYuguangCard>();
    addMetaObject<ThCanfeiCard>();
    addMetaObject<ThGuiyuniuCard>();
    addMetaObject<ThHaixingCard>();

    skills << new ThBaochuiRecord << new ThChipin << new ThAimin << new ThAiminTrigger << new ThLingweiGivenSkill
           << new ThZuishengVS;
    related_skills.insertMulti("thaimin", "#thaimin");
}

ADD_PACKAGE(TouhouShin)
