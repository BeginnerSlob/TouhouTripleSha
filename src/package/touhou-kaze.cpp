#include "touhou-kaze.h"

#include "general.h"
#include "skill.h"
#include "room.h"
#include "carditem.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"

class ThZhiji: public TriggerSkill {
public:
    ThZhiji(): TriggerSkill("thzhiji") {
        frequency = Frequent;
        events << EventPhaseChanging << EventPhaseEnd;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (!player->isWounded() || room->askForChoice(player, objectName(), "recover+draw") == "draw")
            player->drawCards(1, objectName());
        else
            room->recover(player, RecoverStruct(player));

        return false;
    }
};

class ThZhijiRecord: public TriggerSkill {
public:
    ThZhijiRecord(): TriggerSkill("#thzhiji") {
        frequency = Compulsory;
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() == Player::Discard && move.from == player
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            QVariantList QijiList = player->tag["ThZhijiList"].toList();
            foreach (int id, move.card_ids)
                if (!QijiList.contains(id))
                    QijiList << id;
            player->tag["ThZhijiList"] = QijiList;
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const{
        return false;
    }
};

ThJiyiCard::ThJiyiCard(){
}

bool ThJiyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThJiyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    const Card *card = room->askForCardShow(target, source, "thjiyi");
    room->showCard(target, card->getId());
    if(card->getTypeId() == TypeTrick) {
        const Card *card2 = room->askForCard(source, ".Trick", "@thjiyi:" + target->objectName(), QVariant(), MethodNone);
        if(!card2)
            target->drawCards(1);
        else {
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thjiyi", QString());
            room->obtainCard(target, card2, reason);
        }
    } else
        source->obtainCard(card);
}

class ThJiyi: public ZeroCardViewAsSkill{
public:
    ThJiyi(): ZeroCardViewAsSkill("thjiyi"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThJiyiCard");
    }

    virtual const Card *viewAs() const{
        return new ThJiyiCard;
    }
};

class ThHuadi: public TriggerSkill{
public:
    ThHuadi(): TriggerSkill("thhuadi$") {
    }
};

class ThJilanwen:public TriggerSkill{
public:
    ThJilanwen():TriggerSkill("thjilanwen") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (player && player->getPhase() == Player::Draw && TriggerSkill::triggerable(player))
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = player;

        room->judge(judge);

        Card::Suit suit = (Card::Suit)judge.pattern.toInt();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getAllPlayers())
            foreach (const Card *card, p->getCards("ej"))
                if (card->getSuit() != suit) {
                    targets << p;
                    break;
                }

        if (!targets.isEmpty()) {
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "thjilanwen-choose", true);
            if (target) {
                QList<int> disabled_ids;
                foreach (const Card *card, target->getCards("ej"))
                    if (card->getSuit() == suit)
                        disabled_ids << card->getEffectiveId();

                int card_id = room->askForCardChosen(player, target, "ej", objectName(), false, Card::MethodNone, disabled_ids);

                if (card_id != -1)
                    room->obtainCard(player, card_id);
            } else
                player->drawCards(1, objectName());
        } else
            player->drawCards(1, objectName());

        return true;
    }
};

class ThJilanwenGet: public TriggerSkill {
public:
    ThJilanwenGet(): TriggerSkill("#thjilanwen") {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL){
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "thjilanwen") {
                judge->pattern = QString::number(int(judge->card->getSuit()));
                if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        judge->who->obtainCard(judge->card);

        return false;
    }
};

ThNiankeCard::ThNiankeCard(){
    will_throw = false;
    handling_method = MethodNone;
}

void ThNiankeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thnianke", QString());
    room->obtainCard(target, this, reason);
    int card_id = room->askForCardChosen(source, target, "h", "thnianke");
    room->showCard(target, card_id);
    if (Sanguosha->getCard(card_id)->isRed()) {
        QList<ServerPlayer *> players;
        players << source << target;
        room->sortByActionOrder(players);
        room->drawCards(players, 1, "thnianke");
    }
}

class ThNianke: public OneCardViewAsSkill {
public:
    ThNianke(): OneCardViewAsSkill("thnianke") {
        filter_pattern = "Jink";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThNiankeCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThNiankeCard *card = new ThNiankeCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThJilan: public TriggerSkill{
public:
    ThJilan(): TriggerSkill("thjilan"){
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        QStringList skill_list;
        if (!TriggerSkill::triggerable(player))
            return skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (!p->isNude()) {
                for (int i = 0; i < damage.damage; i++)
                    skill_list << objectName();
                return skill_list;
            }

        return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (!p->isNude())
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

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThJilanTarget"].value<ServerPlayer *>();
        player->tag.remove("ThJilanTarget");
        int n = qMax(target->getLostHp(), 1);
        room->askForDiscard(target, objectName(), n, n, false, true);

        return false;
    }
};

class ThWangshou: public TriggerSkill{
public:
    ThWangshou(): TriggerSkill("thwangshou"){
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.to->isDead())
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();

        JudgeStruct judge;
        judge.pattern = ".|black";
        judge.good = false;
        judge.reason = objectName();
        judge.who = damage.to;

        room->judge(judge);
        if(judge.isBad())
            if(player->canDiscard(damage.to, "he")) {
                int card_id = room->askForCardChosen(player, damage.to, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, damage.to, player);
            }

        return false;
    }
};

class ThZhanye: public TriggerSkill {
public:
    ThZhanye(): TriggerSkill("thzhanye") {
        events << FinishJudge;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player == NULL)
            return skill_list;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if(!judge->card->isRed())
            return skill_list;

        if (player->isDead())
            return skill_list;

        QList<ServerPlayer *> owners = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *owner, owners) {
            if (owner == player || owner == room->getCurrent())
                continue;
            skill_list.insert(owner, QStringList(objectName()));
        }

        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        room->setPlayerFlag(ask_who, "ThZhanyeUse");
        if (!room->askForUseSlashTo(ask_who, player, "@thzhanye:" + player->objectName(), false))
            room->setPlayerFlag(ask_who, "-ThZhanyeUse");

        return false;
    }
};

ThEnanCard::ThEnanCard() {
}

bool ThEnanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && (Self == to_select || Self->inMyAttackRange(to_select));
}

void ThEnanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    room->loseMaxHp(source);
    room->loseHp(targets.first());
}

class ThEnan: public ZeroCardViewAsSkill {
public:
    ThEnan(): ZeroCardViewAsSkill("thenan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThEnanCard");
    }

    virtual const Card *viewAs() const {
        return new ThEnanCard;
    }
};

class ThBeiyun: public TriggerSkill {
public:
    ThBeiyun(): TriggerSkill("thbeiyun") {
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DyingStruct dying = data.value<DyingStruct>();
        if (player != dying.who) return QStringList();
        if (player->isAlive() && player->getHp() < 1)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if(!player->askForSkillInvoke(objectName()))
            return false;

        room->broadcastSkillInvoke(objectName());
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        QList<int> card_ids = room->getNCards(qMin(4, player->getMaxHp()));
        CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString());
        room->moveCardsAtomic(CardsMoveStruct(card_ids, NULL, Player::PlaceTable, reason), true);
        while (!card_ids.isEmpty()) {
            QStringList choices;
            choices << "cancel";
            foreach (int id, card_ids) {
                const Card *card = Sanguosha->getCard(id);
                if(card->getSuit() == Card::Diamond) {
                    choices << "red";
                    break;
                }
            }
            foreach (int id, card_ids) {
                const Card *card = Sanguosha->getCard(id);
                if(card->isBlack()) {
                    choices << "black";
                    break;
                }
            }
            QString choice = room->askForChoice(player, objectName(), choices.join("+"));

            DummyCard *dummy = new DummyCard;
            dummy->deleteLater();
            CardMoveReason reason2(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), objectName(), QString());
            if (choice == "red") {
                foreach (int id, card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (card->isRed()) {
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
                    if (card->isBlack()) {
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

class ThMicaiGivenSkill: public ViewAsSkill {
public:
    ThMicaiGivenSkill(): ViewAsSkill("thmicaiv") {
        attached_lord_skill = true;
    }

    virtual bool shouldBeVisible(const Player *) const{
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        if (player->getWeapon() || player->hasSkill("thsilian"))
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

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (player->getWeapon() || player->hasSkill("thsilian"))
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

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Self->hasWeapon("spear")) {
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("spear");
            return spear_skill->viewFilter(selected, to_select);
        } else if (Self->hasWeapon("fan")) {
            const ViewAsSkill *fan_skill = Sanguosha->getViewAsSkill("fan");
            return fan_skill->viewFilter(selected, to_select);
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
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

ThMicaiCard::ThMicaiCard() {
}

void ThMicaiCard::onEffect(const CardEffectStruct &effect) const {
    Room *room = effect.from->getRoom();
    room->setPlayerMark(effect.from, "thmicaisource", 1);
    room->setPlayerMark(effect.to, "@micai", 1);
    room->attachSkillToPlayer(effect.to, "thmicaiv");
}

class ThMicai: public ZeroCardViewAsSkill {
public:
    ThMicai(): ZeroCardViewAsSkill("thmicai") {
    }

    virtual const Card *viewAs() const{
        return new ThMicaiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThMicaiCard") && player->hasSkill("thqiaogong");
    }
};

class ThMicaiClear: public TriggerSkill {
public:
    ThMicaiClear(): TriggerSkill("#thmicai") {
        events << EventPhaseStart << Death << EventLoseSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
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
            if (p->getMark("@micai") > 0) {
                room->setPlayerMark(p, "@micai", 0);
                room->detachSkillFromPlayer(p, "thmicaiv", true);
            }
        }
        room->setPlayerMark(player, "thmicaisource", 0);

        return QStringList();
    }
};

ThQiaogongCard::ThQiaogongCard() {
}

bool ThQiaogongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const {
    if (!targets.isEmpty() || !to_select->hasEquip())
        return false;

    Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    foreach (const Card *cd, to_select->getEquips())
        if (!getSubcards().contains(cd->getEffectiveId()) && cd->getSuit() == suit)
            return true;

    return false;
}

void ThQiaogongCard::onEffect(const CardEffectStruct &effect) const {
    QList<int> disabled_ids;
    Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    foreach (const Card *cd, effect.to->getEquips())
        if (cd->getSuit() != suit)
            disabled_ids << cd->getEffectiveId();

    Room *room = effect.from->getRoom();
    int id = room->askForCardChosen(effect.from, effect.to, "e", "thqiaogong", false, Card::MethodNone, disabled_ids);
    room->obtainCard(effect.from, id);
}

class ThQiaogong: public ViewAsSkill {
public:
    ThQiaogong(): ViewAsSkill("thqiaogong") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const {
        if (Self->isJilei(to_select)) return false;
        if (selected.isEmpty())
            return true;
        else if (selected.length() == 1) {
            const Card *card = selected.first();
            return to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (cards.length() == 2) {
            ThQiaogongCard *card = new ThQiaogongCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThQiaogongCard");
    }
};

class ThZhouhuaGivenSkill: public ViewAsSkill {
public:
    ThZhouhuaGivenSkill(): ViewAsSkill("thzhouhuav") {
        response_or_use = true;
        attached_lord_skill = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const{
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        Analeptic *card = new Analeptic(Card::SuitToBeDecided, -1);
        card->addSubcards(cards);
        card->setSkillName("thzhouhua");
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        QString obj_name = player->property("zhouhua_source").toString();
        const Player *source = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->objectName() == obj_name) {
                source = p;
                break;
            }
        }
        if (source && source->hasSkill("thzhouhua"))
            return IsEnabledAtPlay(player);
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        QString obj_name = player->property("zhouhua_source").toString();
        const Player *source = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->objectName() == obj_name) {
                source = p;
                break;
            }
        }
        if (source && source->hasSkill("thzhouhua"))
            return IsEnabledAtResponse(player, pattern);
        return false;
    }

    bool IsEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    bool IsEnabledAtResponse(const Player *, const QString &pattern) const {
        return pattern.contains("analeptic");
    }
};

class ThZhouhuaViewAsSkill: public ThZhouhuaGivenSkill{
public:
    ThZhouhuaViewAsSkill(): ThZhouhuaGivenSkill() {
        attached_lord_skill = false;
        setObjectName("thzhouhua");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return IsEnabledAtPlay(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        return player->getPhase() != Player::NotActive && IsEnabledAtResponse(player, pattern);
    }
};

class ThZhouhua: public TriggerSkill {
public:
    ThZhouhua(): TriggerSkill("thzhouhua") {
        events << EventPhaseStart << EventPhaseChanging << Death;
        view_as_skill = new ThZhouhuaViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart && player->hasSkill(objectName(), true) && player->isAlive()) {
                player->setFlags("ZhouhuaTurn");
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->hasSkill("thzhouhuav"))
                        room->attachSkillToPlayer(p, "thzhouhuav");
                    room->setPlayerProperty(p, "zhouhua_source", player->objectName());
                }
            }
            return QStringList();
        }
        if (player != NULL && player->hasFlag("ZhouhuaTurn")) {
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
            player->setFlags("-ZhouhuaTurn");
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasSkill("thzhouhuav"))
                    room->detachSkillFromPlayer(p, "thzhouhuav", true);
                room->setPlayerProperty(p, "zhouhua_source", "");
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const {
        return false;
    }
};

class ThXugu: public TriggerSkill {
public:
    ThXugu(): TriggerSkill("thxugu") {
        events << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && player == room->getCurrent() && player->getPhase() != Player::NotActive) {
            if (!player->hasFlag("ThXuguFailed") && use.card->isKindOf("Analeptic"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *p = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thxugu", true, true);
        if (p) {
            player->tag["ThXuguTarget"] = QVariant::fromValue(p);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *victim = player->tag["ThXuguTarget"].value<ServerPlayer *>();
        player->tag.remove("ThXuguTarget");
        if (victim) {
            if (!room->askForUseCard(victim, "analeptic", "@thxugu-use", -1, Card::MethodUse, false)) {
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

class ThShenzhou: public TriggerSkill {
public:
    ThShenzhou(): TriggerSkill("thshenzhou") {
        frequency = Frequent;
        events << TurnedOver << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        QStringList skills;
        if (triggerEvent == TurnedOver && player->faceUp()) {
            skills << objectName();
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            for (int i = 0; i < damage.damage; ++i)
                skills << objectName();
        }
        return skills;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (!player->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName());
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
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
        QString type = room->askForChoice(player, objectName(), choices.join("+"));
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName(), QString(), false, true);
        CardMoveReason reason2(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), objectName(), QString());
        DummyCard *dummy = new DummyCard;
        dummy->deleteLater();
        foreach(int id, card_ids)
            if(Sanguosha->getCard(id)->getType() == type) {
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

class ThTianliu: public TriggerSkill {
public:
    ThTianliu(): TriggerSkill("thtianliu") {
        frequency = Compulsory;
        events << EventPhaseStart << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName())
                judge->pattern = QString::number(int(judge->card->getSuit()));
        } else if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->sendCompulsoryTriggerLog(player, objectName());

        JudgeStruct judge;
        judge.good = false;
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        Card::Suit suit = (Card::Suit)judge.pattern.toInt();
        switch (suit) {
        case Card::Heart: player->drawCards(3); break;
        case Card::Diamond: player->drawCards(2); break;
        case Card::Club: player->drawCards(1); break;
        }

        return true;
    }
};

ThQianyiCard::ThQianyiCard(){
}

bool ThQianyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void ThQianyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    room->removePlayerMark(source, "@qianyi");
    room->addPlayerMark(source, "@qianyiused");
    //room->broadcastInvoke("animate", "lightbox:$kunyi");

    source->turnOver();
    QStringList choices;
    if (target->isWounded())
        choices << "recover";
    choices << "draw";
    QString choice = room->askForChoice(source, "thqianyi", choices.join("+"));
    if (choice == "recover")
        room->recover(target, RecoverStruct(source));
    else
        target->drawCards(2, "thqianyi");
}

class ThQianyi: public ZeroCardViewAsSkill {
public:
    ThQianyi(): ZeroCardViewAsSkill("thqianyi") {
        frequency = Limited;
        limit_mark = "@qianyi";
    }

    virtual const Card *viewAs() const {
        return new ThQianyiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return player->getMark("@qianyi") > 0;
    }
};

ThHuosuiCard::ThHuosuiCard() {
}

bool ThHuosuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && Self->inMyAttackRange(to_select);
}

void ThHuosuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    ServerPlayer *target = targets.first();
    if (!room->askForCard(target, "jink", "@thhuosuijink", QVariant(), Card::MethodResponse, source))
        if(!room->askForUseSlashTo(source, target, "@thhuosui-slash"))
            source->drawCards(1);
}

class ThHuosui: public OneCardViewAsSkill {
public:
    ThHuosui(): OneCardViewAsSkill("thhuosui") {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThHuosuiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        ThHuosuiCard *card = new ThHuosuiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThTiandi:public TriggerSkill{
public:
    ThTiandi(): TriggerSkill("thtiandi"){
        frequency = Frequent;
        events << TargetConfirming;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
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
        player->drawCards(1);
        return false;
    }
};

ThKunyiCard::ThKunyiCard(){
}

bool ThKunyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->inMyAttackRange(to_select);
}

void ThKunyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    room->removePlayerMark(source, "@kunyi");
    room->addPlayerMark(source, "@kunyiused");
    //room->broadcastInvoke("animate", "lightbox:$kunyi");

    source->turnOver();
    room->damage(DamageStruct("thkunyi", source, target));
}

class ThKunyi: public ZeroCardViewAsSkill {
public:
    ThKunyi(): ZeroCardViewAsSkill("thkunyi") {
        frequency = Limited;
        limit_mark = "@kunyi";
    }

    virtual const Card *viewAs() const {
        return new ThKunyiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return player->getMark("@kunyi") > 0;
    }
};

ThCannueCard::ThCannueCard() {
    will_throw = false;
    handling_method = MethodNone;
}

void ThCannueCard::onEffect(const CardEffectStruct &effect) const {
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "thcannue", QString());
    room->obtainCard(effect.to, this, reason);
    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.to))
        if (effect.to->inMyAttackRange(p) && effect.to->canSlash(p, false))
            targets << p;

    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "thcannue");

        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = effect.from;
        log.to << target;
        room->sendLog(log);

        if (room->askForUseSlashTo(effect.to, target, "@thcannue-slash"))
            return ;
    }

    QStringList choices;
    if (!effect.to->isNude())
        choices << "get";
    choices << "hit";
    QString choice = room->askForChoice(effect.from, "thcannue", choices.join("+"));
    if (choice == "get") {
        int card_id = room->askForCardChosen(effect.from, effect.to, "he", "thcannue");
        room->obtainCard(effect.from, card_id, false);
    } else
        room->damage(DamageStruct("thcannue", effect.from, effect.to));
};

class ThCannue: public OneCardViewAsSkill {
public:
    ThCannue(): OneCardViewAsSkill("thcannue") {
        filter_pattern = ".|diamond";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThCannueCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThCannueCard *card = new ThCannueCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThSibao: public OneCardViewAsSkill {
public:
    ThSibao(): OneCardViewAsSkill("thsibao") {
        filter_pattern = "EquipCard";
        response_pattern = "peach+analeptic";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return Analeptic::IsAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        Analeptic *jiu = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        jiu->addSubcard(originalCard);
        jiu->setSkillName(objectName());
        return jiu;
    }
};

class ThWangqin: public TriggerSkill {
public:
    ThWangqin(): TriggerSkill("thwangqin") {
        events << CardUsed << CardResponded;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        QMap<ServerPlayer *, QStringList> skill_list;
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
                if (!owner->faceUp())
                    skill_list.insert(owner, QStringList(objectName()));

        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        ask_who->turnOver();
        player->turnOver();
        room->setPlayerProperty(player, "chained", false);

        return false;
    }
};

class ThFusuo: public TriggerSkill {
public:
    ThFusuo(): TriggerSkill("thfusuo") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player))
                if (!p->isChained())
                    return QStringList(objectName());
        } else if (player->getPhase() == Player::Finish && player->hasFlag("fusuoinvoke"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
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

ThGelongCard::ThGelongCard() {
}

bool ThGelongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThGelongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    ServerPlayer *target = targets.first();
    bool success = source->pindian(target, "thgelong");
    if (success) {
        const Card *card = NULL;
        if (!source->isKongcheng())
            card = room->askForCard(source, ".", "@thgelonggive:" + source->objectName(), QVariant(), Card::MethodNone);
        if (card) {
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thgelong", QString());
            room->obtainCard(target, card, reason, false);
        } else
            room->loseHp(source);
    } else {
        QStringList choices;
        choices << "damage";
        if (!target->isNude())
            choices << "get";
        QString choice = room->askForChoice(source, "thgelong", choices.join("+"));
        if(choice == "damage")
            room->damage(DamageStruct("thgelong", source, target));
        else {
            int card_id = room->askForCardChosen(source, target, "he", "thgelong");
            room->obtainCard(source, card_id, false);
        }
    }
}

class ThGelong: public ZeroCardViewAsSkill {
public:
    ThGelong(): ZeroCardViewAsSkill("thgelong") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThGelongCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new ThGelongCard;
    }
};

class ThYuanzhou: public TriggerSkill {
public:
    ThYuanzhou(): TriggerSkill("thyuanzhou") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (p->getHandcardNum() < player->getHandcardNum())
                    return QStringList();
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThYuanzhouTarget"].value<ServerPlayer *>();
        player->tag.remove("ThYuanzhouTarget");
        if (target) {
            int card_id = room->askForCardChosen(player, target, "hej", "thyuanzhou", false, Card::MethodDiscard);
            room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : target, player);
        }
        return false;
    }
};

ThDasuiCard::ThDasuiCard() {
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void ThDasuiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const {
    source->addToPile("dasuipile", this);
}

class ThDasuiViewAsSkill: public ViewAsSkill {
public:
    ThDasuiViewAsSkill(): ViewAsSkill("thdasui") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThDasuiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const {
        return selected.size() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (cards.size() != 0) {
            ThDasuiCard *card = new ThDasuiCard;
            card->addSubcards(cards);
            return card;
        }
        else
            return NULL;
    }
};

class ThDasui: public TriggerSkill {
public:
    ThDasui(): TriggerSkill("thdasui") {
        events << EventPhaseStart;
        view_as_skill = new ThDasuiViewAsSkill;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() != Player::Play) return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (owner == player || owner->getPile("dasuipile").isEmpty())
                continue;
            skill_list.insert(owner, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        QList<int> card_ids = ask_who->getPile("dasuipile");
        if (card_ids.isEmpty()) return false;
        room->fillAG(card_ids);
        int card_id = room->askForAG(player, card_ids, true, objectName());
        room->clearAG();
        if (card_id != -1)
            room->obtainCard(player, card_id);
        return false;
    }
};

class ThFengren: public TriggerSkill {
public:
    ThFengren(): TriggerSkill("thfengren") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Start)
            return !player->getPile("dasuipile").isEmpty();
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->sendCompulsoryTriggerLog(player, objectName());
        
        QList<int> card_ids = player->getPile("dasuipile");
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

class ThFuli: public TriggerSkill {
public:
    ThFuli(): TriggerSkill("thfuli") {
        frequency = Frequent;
        events << TargetConfirmed << BeforeCardsMove;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.to.contains(player)) {
                player->setFlags("fuli_target");
                use.card->setFlags("thfuli");
            }
        } else if (triggerEvent == BeforeCardsMove && player->hasFlag("fuli_target")) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
                && move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
                && player != move.from) {
                    const Card *card = move.reason.m_extraData.value<const Card *>();
                    if (card->isKindOf("Slash") && card->hasFlag("thfuli"))
                        return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        move.reason.m_extraData.value<const Card *>()->setFlags("-thfuli");
        player->setFlags("-fuli_target");
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        player->addToPile("dasuipile", move.card_ids);
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);

        return false;
    }
};

class ThKudao: public TriggerSkill {
public:
    ThKudao(): TriggerSkill("thkudao") {
        events << TargetConfirmed << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        ServerPlayer *target = NULL;
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player && use.card->isRed() && use.to.length() == 1 && !use.to.contains(player))
                target = use.to.first();
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player && use.from->hasFlag("CollateralUsing") && use.card->isKindOf("Slash") && use.card->isRed()) {
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThKudaoTarget"].value<ServerPlayer *>();
        if (target && player->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        player->tag.remove("ThKudaoTarget");
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThKudaoTarget"].value<ServerPlayer *>();
        player->tag.remove("ThKudaoTarget");
        if (target) {
            int card_id = room->askForCardChosen(player, target, "he", objectName());
            player->addToPile("kudaopile", card_id, true);
        }
        return false;
    }
};

class ThSuilun: public TriggerSkill {
public:
    ThSuilun(): TriggerSkill("thsuilun") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::NotActive) {
            QList<int> lists[4];
            foreach (int card_id, player->getPile("kudaopile"))
                lists[(int)Sanguosha->getEngineCard(card_id)->getSuit()] << card_id;

            int empty = 0;
            for (int i = 0; i < 4; ++i)
                if (lists[i].isEmpty())
                    ++empty;
            if (empty > 1)
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            QList<int> card_ids = player->getPile("kudaopile");
            room->fillAG(card_ids, player);
            QList<int> to_throw;
            while (!card_ids.isEmpty() && to_throw.length() < 3) {
                int card_id = room->askForAG(player, card_ids, false, "thsuilun");
                card_ids.removeOne(card_id);
                to_throw << card_id;
                // throw the rest cards that matches the same suit
                const Card *card = Sanguosha->getCard(card_id);
                Card::Suit suit = card->getSuit();

                QList<ServerPlayer *> _player;
                _player << player;
                room->takeAG(player, card_id, false, _player);

                QList<int> _card_ids = card_ids;
                foreach (int id, _card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    if (c->getSuit() == suit) {
                        card_ids.removeOne(id);
                        room->takeAG(NULL, id, false);
                    }
                }
            }
            room->clearAG(player);
            DummyCard *dummy = new DummyCard(to_throw);
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->gainAnExtraTurn();
        return false;
    }
};

ThRansangCard::ThRansangCard() {
}

bool ThRansangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThRansangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    bool success = source->pindian(target, "thransang");
    if (success) {
        room->setPlayerFlag(source, "thransang");
        room->acquireSkill(source, "thyanlun");
    } else
        room->setPlayerCardLimitation(source, "use", "TrickCard", true);
}

class ThRansangViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThRansangViewAsSkill(): ZeroCardViewAsSkill("thransang") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThRansangCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const {
        return new ThRansangCard;
    }
};

class ThRansang: public TriggerSkill {
public:
    ThRansang(): TriggerSkill("thransang") {
        events << EventPhaseChanging;
        view_as_skill = new ThRansangViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!player->hasFlag("thransang")) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive)
            room->detachSkillFromPlayer(player, "thyanlun", false, true);
        return QStringList();
    }
};

class ThYanlunViewAsSkill: public OneCardViewAsSkill {
public:
    ThYanlunViewAsSkill(): OneCardViewAsSkill("thyanlun") {
        response_or_use = true;
        filter_pattern = ".|red|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        FireAttack *fa = new FireAttack(Card::SuitToBeDecided, -1);
        fa->deleteLater();
        return fa->isAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        FireAttack *fa = new FireAttack(Card::SuitToBeDecided, -1);
        fa->addSubcard(originalCard);
        fa->setSkillName(objectName());
        return fa;
    }
};

class ThYanlun: public TriggerSkill {
public:
    ThYanlun(): TriggerSkill("thyanlun") {
        events << Damage;
        view_as_skill = new ThYanlunViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (TriggerSkill::triggerable(player) && damage.card->isKindOf("FireAttack"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName()))
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->drawCards(1);
        return false;
    }
};

class ThBazhiFilterSkill: public FilterSkill {
public:
    ThBazhiFilterSkill(): FilterSkill("thbazhi") {
    }

    virtual bool viewFilter(const Card* to_select) const {
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

        foreach(ServerPlayer *p, room->getOtherPlayers(splayer))
            if (splayer->getHp() > qMax(0, p->getHp()))
                return true;

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        FireSlash *fs = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        fs->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(fs);
        return card;
    }
};

class ThBazhi: public TriggerSkill {
public:
    ThBazhi(): TriggerSkill("thbazhi") {
        frequency = Compulsory;
        events << HpChanged;
        view_as_skill = new ThBazhiFilterSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        Room *room = target->getRoom();
        foreach (ServerPlayer *p, room->findPlayersBySkillName("thbazhi"))
            room->filterCards(p, p->getCards("he"), true);

        return false;
    }
};

ThYanxingCard::ThYanxingCard() {
    target_fixed = true;
}

void ThYanxingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const {
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

class ThYanxingViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThYanxingViewAsSkill(): ZeroCardViewAsSkill("thyanxing") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThYanxingCard");
    }

    virtual const Card *viewAs() const{
        return new ThYanxingCard;
    }
};

class ThYanxing: public TriggerSkill {
public:
    ThYanxing(): TriggerSkill("thyanxing") {
        events << EventPhaseChanging;
        view_as_skill = new ThYanxingViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!player->hasFlag("thyanxing")) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive)
            room->detachSkillFromPlayer(player, "thheyu", false, true);
        return QStringList();
    }
};

class ThHeyu: public TriggerSkill {
public:
    ThHeyu(): TriggerSkill("thheyu") {
        events << DamageComplete;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (!TriggerSkill::triggerable(damage.from))
            return QStringList();
        if (damage.card && damage.card->isKindOf("NatureSlash") && player->isAlive()) {
            ask_who = damage.from;
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        room->damage(DamageStruct(objectName(), ask_who, player));
        return false;
    }
};

ThMaihuoCard::ThMaihuoCard() {
    will_throw = false;
}

void ThMaihuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    ServerPlayer *target = targets.first();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thmaihuo", QString());
    room->obtainCard(target, this, reason);
    Card::Suit suit = room->askForSuit(source, "thmaihuo");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = source;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->showAllCards(target);
    QList<int> card_ids = target->handCards();
    int num = 0;
    foreach (int card_id, card_ids)
        if (Sanguosha->getCard(card_id)->getSuit() == suit)
            num += 1;

    if (num != 0)
        target->drawCards(qMin(3, num));
}

class ThMaihuo: public OneCardViewAsSkill {
public:
    ThMaihuo(): OneCardViewAsSkill("thmaihuo") {
        filter_pattern = ".|heart";
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThMaihuoCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        ThMaihuoCard *card = new ThMaihuoCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThWunian: public TriggerSkill {
public:
    ThWunian(): TriggerSkill("thwunian") {
        frequency = Compulsory;
        events << Predamage << CardEffected;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == Predamage) {
            return QStringList(objectName());
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if ((effect.card->isNDTrick() || effect.card->isKindOf("Slash")) && !effect.from->isWounded() && effect.from->getMaxHp() != 1)
                return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        if (triggerEvent == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.from = NULL;
            data = QVariant::fromValue(damage);
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            effect.nullified = true;
            data = QVariant::fromValue(effect);
        }

        return false;
    }
};

#include "jsonutils.h"

class ThDongxi: public TriggerSkill {
public:
    ThDongxi(): TriggerSkill("thdongxi") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (player->getPhase() == Player::Start && TriggerSkill::triggerable(player) && player->hasSkill("thsangzhi")) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                foreach (const Skill *skill, p->getVisibleSkillList()) {
                    if (skill->isLordSkill() || skill->isAttachedLordSkill()
                        || skill->getFrequency() == Skill::Limited
                        || skill->getFrequency() == Skill::Wake)
                        continue;
                    if (skill->objectName() == player->tag.value("ThDongxiLast").toString())
                        continue;
                    return QStringList(objectName());
                }
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            QStringList skills;
            foreach (const Skill *skill, p->getVisibleSkillList()) {
                if (skill->isLordSkill() || skill->isAttachedLordSkill()
                    || skill->getFrequency() == Skill::Limited
                    || skill->getFrequency() == Skill::Wake)
                    continue;
                if (skill->objectName() == player->tag.value("ThDongxiLast").toString())
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
                    player->tag["ThDongxi"] = QVariant::fromValue(choice);

                    Json::Value arg(Json::arrayValue);
                    arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                    arg[1] = QSanProtocol::Utils::toJsonString(player->objectName());
                    arg[2] = QSanProtocol::Utils::toJsonString(target->getGeneral()->objectName());
                    arg[3] = QSanProtocol::Utils::toJsonString(choice);
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

                    return true;
                }
            }
        }

        player->tag.remove("ThDongxi");
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->acquireSkill(player, player->tag["ThDongxi"].toString());
        return false;
    }
};

class ThDongxiClear: public TriggerSkill {
public:
    ThDongxiClear(): TriggerSkill("#thdongxi-clear") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (player->getPhase() == Player::NotActive && !player->tag.value("ThDongxiLast").toString().isEmpty()) {
            player->tag.remove("ThDongxiLast");
        } else if (player->getPhase() == Player::RoundStart && !player->tag.value("ThDongxi").toString().isEmpty()) {
            QString name = player->tag.value("ThDongxi").toString();
            player->tag.remove("ThDongxi");
            room->detachSkillFromPlayer(player, name, false, true);

            Json::Value arg(Json::arrayValue);
            arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            arg[1] = QSanProtocol::Utils::toJsonString(player->objectName());
            arg[2] = QSanProtocol::Utils::toJsonString(player->getGeneral()->objectName());
            arg[3] = QSanProtocol::Utils::toJsonString(QString());
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

            player->tag["ThDongxiLast"] = name;
        }

        return QStringList();
    }
};

ThSangzhiCard::ThSangzhiCard() {
}

void ThSangzhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    room->setPlayerMark(targets.first(), "@sangzhi", 1);
    foreach (ServerPlayer *pl, room->getAllPlayers())
        room->filterCards(pl, pl->getCards("he"), true);
    Json::Value args;
    args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    source->tag["ThSangzhiUsed"] = true;
}

class ThSangzhiViewAsSkill: public OneCardViewAsSkill {
public:
    ThSangzhiViewAsSkill(): OneCardViewAsSkill("thsangzhi") {
        filter_pattern = "Peach,EquipCard!";
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        ThSangzhiCard *card = new ThSangzhiCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThSangzhiCard");
    }
};

class ThSangzhi: public TriggerSkill {
public:
    ThSangzhi(): TriggerSkill("thsangzhi") {
        events << EventPhaseChanging << Death;
        view_as_skill = new ThSangzhiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!player || !player->tag["ThSangzhiUsed"].toBool()) return QStringList();

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
                Json::Value args;
                args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }

        return QStringList();
    }
};

class ThSangzhiInvalidity: public InvaliditySkill {
public:
    ThSangzhiInvalidity(): InvaliditySkill("#thsangzhi-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *) const{
        return player->getMark("@sangzhi") == 0;
    }
};

ThXinhuaCard::ThXinhuaCard() {
    will_throw = false;
    m_skillName = "thxinhuav";
    handling_method = MethodNone;
    mute = true;
}

bool ThXinhuaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && to_select->hasLordSkill("thxinhua")
            && to_select != Self && !to_select->hasFlag("ThXinhuaInvoked");
}

void ThXinhuaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    ServerPlayer *target = targets.first();
    if (target->hasLordSkill("thxinhua")) {
        room->setPlayerFlag(target, "ThXinhuaInvoked");
        room->broadcastSkillInvoke("thxinhua");
        room->notifySkillInvoked(target, "thxinhua");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "thxinhua", QString());
        room->obtainCard(target, this, reason);
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->isKongcheng())
                continue;
            victims << p;
        }
        if (!victims.isEmpty()) {
            ServerPlayer *victim = room->askForPlayerChosen(source, victims, "thxinhua");
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

class ThXinhuaViewAsSkill: public OneCardViewAsSkill {
public:
    ThXinhuaViewAsSkill(): OneCardViewAsSkill("thxinhuav") {
        attached_lord_skill = true;
        filter_pattern = "Weapon";
    }

    virtual bool shouldBeVisible(const Player *player) const{
        return player->getKingdom() == "kaze";
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return player->getKingdom() == "kaze" && !player->hasFlag("ForbidThXinhua");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThXinhuaCard *card = new ThXinhuaCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class ThXinhua: public TriggerSkill {
public:
    ThXinhua(): TriggerSkill("thxinhua$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!player) return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "thxinhua")) {
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
                if (!p->hasSkill("thxinhuav"))
                    room->attachSkillToPlayer(p, "thxinhuav");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "thxinhua") {
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
                if (p->hasSkill("thxinhuav"))
                    room->detachSkillFromPlayer(p, "thxinhuav", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
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
    :Package("touhou-kaze")
{
    General *kaze001 = new General(this, "kaze001$", "kaze", 3);
    kaze001->addSkill(new ThZhiji);
    kaze001->addSkill(new ThZhijiRecord);
    kaze001->addSkill(new ThJiyi);
    kaze001->addSkill(new ThHuadi);
    related_skills.insertMulti("thzhiji", "#thzhiji");

    General *kaze002 = new General(this, "kaze002", "kaze");
    kaze002->addSkill(new ThJilanwen);
    kaze002->addSkill(new ThJilanwenGet);
    related_skills.insertMulti("thjilanwen", "#thjilanwen");

    General *kaze003 = new General(this, "kaze003", "kaze", 3);
    kaze003->addSkill(new ThNianke);
    kaze003->addSkill(new ThJilan);

    General *kaze004 = new General(this, "kaze004", "kaze");
    kaze004->addSkill(new ThWangshou);
    kaze004->addSkill(new ThZhanye);

    General *kaze005 = new General(this, "kaze005", "kaze", 3, false);
    kaze005->addSkill(new ThEnan);
    kaze005->addSkill(new ThBeiyun);

    General *kaze006 = new General(this, "kaze006", "kaze");
    kaze006->addSkill(new ThMicai);
    kaze006->addSkill(new ThMicaiClear);
    related_skills.insertMulti("thmicai", "#thmicai");
    kaze006->addSkill(new ThQiaogong);

    General *kaze007 = new General(this, "kaze007", "kaze");
    kaze007->addSkill(new ThZhouhua);
    kaze007->addSkill(new ThXugu);
    kaze007->addSkill(new SlashNoDistanceLimitSkill("thxugu"));
    related_skills.insertMulti("thxugu", "#thxugu-slash-ndl");

    General *kaze008 = new General(this, "kaze008", "kaze", 3);
    kaze008->addSkill(new ThShenzhou);
    kaze008->addSkill(new ThTianliu);
    kaze008->addSkill(new ThQianyi);

    General *kaze009 = new General(this, "kaze009", "kaze", 3, false);
    kaze009->addSkill(new ThHuosui);
    kaze009->addSkill(new ThTiandi);
    kaze009->addSkill(new ThKunyi);

    General *kaze010 = new General(this, "kaze010", "kaze", 3);
    kaze010->addSkill(new ThCannue);
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
    addMetaObject<ThCannueCard>();
    addMetaObject<ThGelongCard>();
    addMetaObject<ThDasuiCard>();
    addMetaObject<ThRansangCard>();
    addMetaObject<ThYanxingCard>();
    addMetaObject<ThMaihuoCard>();
    addMetaObject<ThSangzhiCard>();
    addMetaObject<ThXinhuaCard>();

    skills << new ThMicaiGivenSkill << new ThZhouhuaGivenSkill << new ThYanlun
           << new ThHeyu << new ThXinhuaViewAsSkill;
}

ADD_PACKAGE(TouhouKaze)