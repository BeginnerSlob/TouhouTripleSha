#include "god.h"
#include "client.h"
#include "engine.h"
#include "maneuvering.h"
#include "general.h"
#include "settings.h"

class Wushen: public FilterSkill {
public:
    Wushen(): FilterSkill("wushen") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return to_select->getSuit() == Card::Heart && place == Player::PlaceHand;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class WushenTargetMod: public TargetModSkill {
public:
    WushenTargetMod(): TargetModSkill("#wushen-target") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("wushen") && card->getSuit() == Card::Heart)
            return 1000;
        else
            return 0;
    }
};

class Wuhun: public TriggerSkill {
public:
    Wuhun(): TriggerSkill("wuhun") {
        events << PreDamageDone;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.from && damage.from != player) {
            damage.from->gainMark("@nightmare", damage.damage);
            damage.from->getRoom()->broadcastSkillInvoke(objectName(), 1);
            room->notifySkillInvoked(player, objectName());
        }

        return false;
    }
};

class WuhunRevenge: public TriggerSkill {
public:
    WuhunRevenge(): TriggerSkill("#wuhun") {
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill("wuhun");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *shenguanyu, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != shenguanyu)
            return false;

        QList<ServerPlayer *> players = room->getOtherPlayers(shenguanyu);

        int max = 0;
        foreach (ServerPlayer *player, players)
            max = qMax(max, player->getMark("@nightmare"));
        if (max == 0) return false;

        QList<ServerPlayer *> foes;
        foreach (ServerPlayer *player, players) {
            if (player->getMark("@nightmare") == max)
                foes << player;
        }

        if (foes.isEmpty())
            return false;

        ServerPlayer *foe;
        if (foes.length() == 1)
            foe = foes.first();
        else
            foe = room->askForPlayerChosen(shenguanyu, foes, "wuhun", "@wuhun-revenge");

        room->notifySkillInvoked(shenguanyu, "wuhun");

        JudgeStruct judge;
        judge.pattern = "Peach,GodSalvation";
        judge.good = true;
        judge.negative = true;
        judge.reason = "wuhun";
        judge.who = foe;

        room->judge(judge);

        if (judge.isBad()) {
            room->broadcastSkillInvoke("wuhun", 2);
            room->doLightbox("$WuhunAnimate", 3000);

            LogMessage log;
            log.type = "#WuhunRevenge";
            log.from = shenguanyu;
            log.to << foe;
            log.arg = QString::number(max);
            log.arg2 = "wuhun";
            room->sendLog(log);

            room->killPlayer(foe);
        } else
            room->broadcastSkillInvoke("wuhun", 3);
        QList<ServerPlayer *> killers = room->getAllPlayers();
        foreach (ServerPlayer *player, killers)
            player->loseAllMarks("@nightmare");

        return false;
    }
};

static bool CompareBySuit(int card1, int card2) {
    const Card *c1 = Sanguosha->getCard(card1);
    const Card *c2 = Sanguosha->getCard(card2);

    int a = static_cast<int>(c1->getSuit());
    int b = static_cast<int>(c2->getSuit());

    return a < b;
}

class IkLvejue: public PhaseChangeSkill {
public:
    IkLvejue(): PhaseChangeSkill("iklvejue") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Draw;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *shenlvmeng) const{
        Room *room = shenlvmeng->getRoom();
        QList<int> card_ids = room->getNCards(5);
        qSort(card_ids.begin(), card_ids.end(), CompareBySuit);
        room->fillAG(card_ids);

        QList<int> to_get, to_throw;
        while (!card_ids.isEmpty()) {
            int card_id = room->askForAG(shenlvmeng, card_ids, false, "iklvejue");
            card_ids.removeOne(card_id);
            to_get << card_id;
            // throw the rest cards that matches the same suit
            const Card *card = Sanguosha->getCard(card_id);
            Card::Suit suit = card->getSuit();

            room->takeAG(shenlvmeng, card_id, false);

            QList<int> _card_ids = card_ids;
            foreach (int id, _card_ids) {
                const Card *c = Sanguosha->getCard(id);
                if (c->getSuit() == suit) {
                    card_ids.removeOne(id);
                    room->takeAG(NULL, id, false);
                    to_throw.append(id);
                }
            }
        }
        DummyCard *dummy = new DummyCard;
        if (!to_get.isEmpty()) {
            dummy->addSubcards(to_get);
            shenlvmeng->obtainCard(dummy);
        }
        dummy->clearSubcards();
        if (!to_throw.isEmpty()) {
            dummy->addSubcards(to_throw);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, shenlvmeng->objectName(), objectName(), QString());
            room->throwCard(dummy, reason, NULL);
        }
        delete dummy;
        room->clearAG();
        return true;
    }
};

IkLingshiCard::IkLingshiCard() {
}

bool IkLingshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void IkLingshiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (!effect.to->isKongcheng()) {
        QList<int> ids;
        foreach (const Card *card, effect.to->getHandcards()) {
            if (card->getSuit() == Card::Heart)
                ids << card->getEffectiveId();
        }

        int card_id = room->doGongxin(effect.from, effect.to, ids);
        if (card_id == -1) return;

        QString result = room->askForChoice(effect.from, "iklingshi", "discard+put");
        effect.from->tag.remove("iklingshi");
        if (result == "discard") {
            CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, effect.from->objectName(), QString(), "iklingshi", QString());
            room->throwCard(Sanguosha->getCard(card_id), reason, effect.to, effect.from);
        } else {
            effect.from->setFlags("Global_GongxinOperator");
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, effect.from->objectName(), QString(), "iklingshi", QString());
            room->moveCardTo(Sanguosha->getCard(card_id), effect.to, NULL, Player::DrawPile, reason, true);
            effect.from->setFlags("-Global_GongxinOperator");
        }
    }
}

class IkLingshi: public ZeroCardViewAsSkill {
public:
    IkLingshi(): ZeroCardViewAsSkill("iklingshi") {
    }

    virtual const Card *viewAs() const{
        return new IkLingshiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkLingshiCard");
    }
};

void IkYeyanCard::damage(ServerPlayer *shenzhouyu, ServerPlayer *target, int point) const{
    shenzhouyu->getRoom()->damage(DamageStruct("ikyeyan", shenzhouyu, target, point, DamageStruct::Fire));
}

GreatIkYeyanCard::GreatIkYeyanCard() {
    mute = true;
    m_skillName = "ikyeyan";
}

bool GreatIkYeyanCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const{
    Q_ASSERT(false);
    return false;
}

bool GreatIkYeyanCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    if (subcards.length() != 4) return false;
    QList<Card::Suit> allsuits;
    foreach (int cardId, subcards) {
        const Card *card = Sanguosha->getCard(cardId);
        if (allsuits.contains(card->getSuit())) return false;
        allsuits.append(card->getSuit());
    }
    
    //We can only assign 2 damage to one player
    //If we select only one target only once, we assign 3 damage to the target
    if (targets.toSet().size() == 1)
        return true;
    else if (targets.toSet().size() == 2)
        return targets.size() == 3;
    return false;
}

bool GreatIkYeyanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select,
                                  const Player *, int &maxVotes) const{
    int i = 0;
    foreach (const Player *player, targets)
        if (player == to_select) i++;
    maxVotes = qMax(3 - targets.size(), 0) + i;
    return maxVotes > 0;
}

void GreatIkYeyanCard::use(Room *room, ServerPlayer *shenzhouyu, QList<ServerPlayer *> &targets) const{
    int criticaltarget = 0;
    int totalvictim = 0;
    QMap<ServerPlayer *, int> map;

    foreach (ServerPlayer *sp, targets)
        map[sp]++;

    if (targets.size() == 1)
        map[targets.first()] += 2;

    foreach (ServerPlayer *sp, map.keys()) {
        if (map[sp] > 1) criticaltarget++;
        totalvictim++;
    }
    if (criticaltarget > 0) {
        room->removePlayerMark(shenzhouyu, "@yeyan");
        room->loseHp(shenzhouyu, 3);

        room->broadcastSkillInvoke("ikyeyan", (totalvictim > 1) ? 2 : 1);

        QList<ServerPlayer *> targets = map.keys();
        room->sortByActionOrder(targets);
        foreach (ServerPlayer *sp, targets)
            damage(shenzhouyu, sp, map[sp]);
    }
}

SmallIkYeyanCard::SmallIkYeyanCard() {
    mute = true;
    m_skillName = "ikyeyan";
}

bool SmallIkYeyanCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return !targets.isEmpty();
}

bool SmallIkYeyanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.length() < 3;
}

void SmallIkYeyanCard::use(Room *room, ServerPlayer *shenzhouyu, QList<ServerPlayer *> &targets) const{
    room->broadcastSkillInvoke("ikyeyan", 3);
    room->removePlayerMark(shenzhouyu, "@yeyan");
    room->addPlayerMark(shenzhouyu, "@yeyanused");
    Card::use(room, shenzhouyu, targets);
}

void SmallIkYeyanCard::onEffect(const CardEffectStruct &effect) const{
    damage(effect.from, effect.to, 1);
}

class IkYeyan: public ViewAsSkill {
public:
    IkYeyan(): ViewAsSkill("ikyeyan") {
        frequency = Limited;
        limit_mark = "@yeyan";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@yeyan") >= 1;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() >= 4)
            return false;

        if (to_select->isEquipped())
            return false;

        if (Self->isJilei(to_select))
            return false;

        foreach (const Card *item, selected) {
            if (to_select->getSuit() == item->getSuit())
                return false;
        }

        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length()  == 0) 
            return new SmallIkYeyanCard;
        if (cards.length() != 4)
            return NULL;

        GreatIkYeyanCard *card = new GreatIkYeyanCard;
        card->addSubcards(cards);

        return card;
    }
};

class IkLongxi: public TriggerSkill {
public:
    IkLongxi(): TriggerSkill("iklongxi") {
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Discard
            && target->getMark("iklongxi") >= 2;
    }

    void perform(ServerPlayer *shenzhouyu) const{
        Room *room = shenzhouyu->getRoom();
        QStringList choices;
        QList<ServerPlayer *> all_players = room->getAllPlayers();
        foreach (ServerPlayer *player, all_players) {
            if (player->isWounded()) {
                choices << "up";
                break;
            }
        }
        choices << "down";
        QString result = room->askForChoice(shenzhouyu, objectName(), choices.join("+"));
        if (result == "up") {
            foreach (ServerPlayer *player, all_players)
                room->recover(player, RecoverStruct(shenzhouyu));
        } else if (result == "down") {
            foreach (ServerPlayer *player, all_players)
                room->loseHp(player);
        }
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        perform(player);
        return false;
    }
};

class IkLongxiRecord: public TriggerSkill {
public:
    IkLongxiRecord(): TriggerSkill("#iklongxi-record") {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *shenzhouyu, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (shenzhouyu->getPhase() == Player::Discard && move.from == shenzhouyu
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            shenzhouyu->setMark("iklongxi", 0);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *shenzhouyu, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        shenzhouyu->addMark("iklongxi", move.card_ids.size());
        return false;
    }
};

IkYihuoCard::IkYihuoCard() {
    m_skillName = "ikyihuov";
    mute = true;
}

bool IkYihuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasSkill("ikyihuo")
           && (to_select->hasEquip() || !to_select->faceUp())
           && !to_select->hasFlag("IkYihuoInvoked");
}

void IkYihuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    ServerPlayer *target = targets.first();
    if (target->hasSkill("ikyihuo")) {
        room->setPlayerFlag(target, "IkYihuoInvoked");

        room->broadcastSkillInvoke("ikyihuo");
        room->notifySkillInvoked(target, "ikyihuo");

        room->showCard(source, getEffectiveId(), target);
        const Card *card = room->askForCard(target, "EquipCard", "@ikyihuo-equip:" + source->objectName(), QVariant(), MethodNone);
        if (card) {
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), source->objectName(), "ikyihuo", QString());
            room->obtainCard(source, card, reason);
            target->obtainCard(this);
            target->drawCards(1);
        }

        QList<ServerPlayer *> targets;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasSkill("ikyihuo") && !p->hasFlag("IkYihuoInvoked"))
                targets << p;
        }
        if (targets.isEmpty())
            room->setPlayerFlag(source, "ForbidIkYihuo");
    }
}

class IkYihuoViewAsSkill: public OneCardViewAsSkill{
public:
    IkYihuoViewAsSkill(): OneCardViewAsSkill("ikyihuov") {
        attached_lord_skill = true;
        filter_pattern = ".|.|.|hand";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkYihuoCard *card = new IkYihuoCard();
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasFlag("ForbidIkYihuo");
    }
};

class IkYihuo: public TriggerSkill {
public:
    IkYihuo(): TriggerSkill("ikyihuo") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if ((triggerEvent == GameStart)
            || (triggerEvent == EventAcquireSkill && data.toString() == "ikyihuo")) {
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
                if (!p->hasSkill("ikyihuov"))
                    room->attachSkillToPlayer(p, "ikyihuov");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "ikyihuo") {
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
                if (p->hasSkill("ikyihuov"))
                    room->detachSkillFromPlayer(p, "ikyihuov", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                  return QStringList();
            if (player->hasFlag("ForbidIkYihuo"))
                room->setPlayerFlag(player, "-ForbidIkYihuo");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("IkYihuoInvoked"))
                    room->setPlayerFlag(p, "-IkYihuoInvoked");
            }
        }
        return QStringList();
    }
};

class IkGuixin: public MasochismSkill {
public:
    IkGuixin(): MasochismSkill("ikguixin") {
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return MasochismSkill::triggerable(target)
            && (target->aliveCount() < 4 || target->faceUp());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *shencc, const DamageStruct &damage) const{
        Room *room = shencc->getRoom();
        foreach (ServerPlayer *player, room->getOtherPlayers(shencc)) {
            if (player->isAlive() && !player->isAllNude()) {
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, shencc->objectName());
                int card_id = room->askForCardChosen(shencc, player, "hej", objectName());
                room->obtainCard(shencc, Sanguosha->getCard(card_id), reason, false);
            }
        }

        shencc->turnOver();
    }
};

class IkZhuohuo: public TriggerSkill {
public:
    IkZhuohuo(): TriggerSkill("ikzhuohuo") {
        events << Damage << Damaged;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = triggerEvent == Damage ? "#IkZhuohuoDamage" : "#IkZhuohuoDamaged";
        log.from = player;
        log.arg = QString::number(damage.damage);
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());

        room->addPlayerMark(player, "@mailun", damage.damage);
        room->broadcastSkillInvoke(objectName(), triggerEvent == Damage ? 1 : 2);
        return false;
    }
};

class IkWumou: public TriggerSkill {
public:
    IkWumou(): TriggerSkill("ikwumou") {
        frequency = Compulsory;
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && use.card->isNDTrick())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());

        int num = player->getMark("@mailun");
        if (num >= 1 && room->askForChoice(player, objectName(), "discard+losehp") == "discard") {
            player->loseMark("@mailun");
        } else
            room->loseHp(player);

        return false;
    }
};

IkSuikongCard::IkSuikongCard() {
}

void IkSuikongCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    effect.from->loseMark("@mailun", 2);
    room->acquireSkill(effect.from, "ikwushuang");
    effect.from->setFlags("IkSuikongSource");
    effect.to->setFlags("IkSuikongTarget");
    room->addPlayerMark(effect.to, "Armor_Nullified");
}

class IkSuikongViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkSuikongViewAsSkill(): ZeroCardViewAsSkill("iksuikong") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@mailun") >= 2;
    }

    virtual const Card *viewAs() const{
        return new IkSuikongCard;
    }
};

class IkSuikong: public TriggerSkill {
public:
    IkSuikong(): TriggerSkill("iksuikong") {
        events << EventPhaseChanging << Death;
        view_as_skill = new IkSuikongViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player->hasFlag("IkSukongSource")) return QStringList();
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

        foreach (ServerPlayer *p , room->getAllPlayers()) {
            if (p->hasFlag("IkSukongTarget")) {
                p->setFlags("-IkSukongTarget");
                if (p->getMark("Armor_Nullified") > 0)
                    room->removePlayerMark(p, "Armor_Nullified");
            }
        }
        room->detachSkillFromPlayer(player, "ikwushuang", false, true);

        return QStringList();
    }
};

class IkTianwu: public ZeroCardViewAsSkill {
public:
    IkTianwu(): ZeroCardViewAsSkill("iktianwu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@mailun") >= 6 && !player->hasUsed("IkTianwuCard");
    }

    virtual const Card *viewAs() const{
        return new IkTianwuCard;
    }
};

IkTianwuCard::IkTianwuCard() {
    target_fixed = true;
    mute = true;
}

void IkTianwuCard::use(Room *room, ServerPlayer *shenlvbu, QList<ServerPlayer *> &) const{
    room->broadcastSkillInvoke("iktianwu");
    shenlvbu->loseMark("@mailun", 6);

    QList<ServerPlayer *> players = room->getOtherPlayers(shenlvbu);
    foreach (ServerPlayer *player, players) {
        room->damage(DamageStruct("iktianwu", shenlvbu, player));
        room->getThread()->delay();
    }

    foreach (ServerPlayer *player, players) {
        QList<const Card *> equips = player->getEquips();
        player->throwAllEquips();
        if (!equips.isEmpty())
            room->getThread()->delay();
    }

    foreach (ServerPlayer *player, players) {
        bool delay = !player->isKongcheng();
        room->askForDiscard(player, "iktianwu", 4, 4);
        if (delay)
            room->getThread()->delay();
    }

    shenlvbu->turnOver();
}

class IkQiyao: public TriggerSkill {
public:
    IkQiyao(): TriggerSkill("ikqiyao") {
        frequency = Frequent;
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && !target->getPile("stars").isEmpty()
            && target->getPhase() == Player::Draw;
    }

    static void Exchange(ServerPlayer *shenzhuge) {
        shenzhuge->exchangeFreelyFromPrivatePile("ikqiyao", "stars");
    }

    static void DiscardStar(ServerPlayer *shenzhuge, int n, QString skillName) {
        Room *room = shenzhuge->getRoom();
        QList<int> stars = shenzhuge->getPile("stars");

        for (int i = 0; i < n; i++) {
            room->fillAG(stars, shenzhuge);
            int card_id = room->askForAG(shenzhuge, stars, false, "ikqiyao-discard");
            room->clearAG(shenzhuge);
            stars.removeOne(card_id);
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), skillName, QString());
            room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
        }
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *shenzhuge, QVariant &, ServerPlayer *) const{
        if (shenzhuge->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *shenzhuge, QVariant &, ServerPlayer *) const{
        Exchange(shenzhuge);
        return false;
    }
};

class IkQiyaoStart: public TriggerSkill {
public:
    IkQiyaoStart(): TriggerSkill("#ikqiyao") {
        events << DrawInitialCards << AfterDrawInitialCards;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *shenzhuge, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == DrawInitialCards) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = shenzhuge;
            log.arg = "ikqiyao";
            room->sendLog(log);
            room->notifySkillInvoked(shenzhuge, "ikqiyao");

            data = data.toInt() + 7;
        } else if (triggerEvent == AfterDrawInitialCards) {
            room->broadcastSkillInvoke("ikqiyao");
            const Card *exchange_card = room->askForExchange(shenzhuge, "ikqiyao", 7, 7);
            shenzhuge->addToPile("stars", exchange_card->getSubcards(), false);
            delete exchange_card;
        }
        return false;
    }
};

IkLiefengCard::IkLiefengCard() {
    handling_method = Card::MethodNone;
}

bool IkLiefengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void IkLiefengCard::onEffect(const CardEffectStruct &effect) const{
    IkQiyao::DiscardStar(effect.from, 1, "ikliefeng");
    effect.from->tag["IkLiefeng_user"] = true;
    effect.to->gainMark("@liefeng");
}

class IkLiefengViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkLiefengViewAsSkill(): ZeroCardViewAsSkill("ikliefeng") {
        response_pattern = "@@ikliefeng";
    }

    virtual const Card *viewAs() const{
        return new IkLiefengCard;
    }
};

class IkLiefeng: public TriggerSkill {
public:
    IkLiefeng(): TriggerSkill("ikliefeng") {
        events << EventPhaseStart;
        view_as_skill = new IkLiefengViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && !target->getPile("stars").isEmpty()
            && target->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *shenzhuge, QVariant &, ServerPlayer *) const{
        room->askForUseCard(shenzhuge, "@@ikliefeng", "@ikliefeng-card", -1, Card::MethodNone);
        return false;
    }
};

class IkLiefengTrigger: public TriggerSkill {
public:
    IkLiefengTrigger(): TriggerSkill("#ikliefeng") {
        events << DamageForseen;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player->getMark("@liefeng") > 0) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature == DamageStruct::Fire)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = "#IkLiefengPower";
        log.from = player;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(++damage.damage);
        room->sendLog(log);

        data = QVariant::fromValue(damage);

        return false;
    }
};

class IkLiefengClear: public TriggerSkill {
public:
    IkLiefengClear(): TriggerSkill("#ikliefeng-clear") {
        events << EventPhaseStart << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        }
        if (!player->tag.value("IkLiefeng_user", false).toBool())
            return QStringList();
        bool invoke = false;
        if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) || triggerEvent == Death)
            invoke = true;
        if (!invoke)
            return QStringList();
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players)
            player->loseAllMarks("@liefeng");
        player->tag.remove("IkLiefeng_user");

        return QStringList();
    }
};

IkMiaowuCard::IkMiaowuCard() {
    handling_method = Card::MethodNone;
}

bool IkMiaowuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < Self->getPile("stars").length();
}

void IkMiaowuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    int n = targets.length();
    IkQiyao::DiscardStar(source, n, "ikmiaowu");
    source->tag["IkMiaowu_user"] = true;

    foreach (ServerPlayer *target, targets)
        target->gainMark("@miaowu");
}

class IkMiaowuViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkMiaowuViewAsSkill(): ZeroCardViewAsSkill("ikmiaowu") {
        response_pattern = "@@ikmiaowu";
    }

    virtual const Card *viewAs() const{
        return new IkMiaowuCard;
    }
};

class IkMiaowu: public TriggerSkill {
public:
    IkMiaowu(): TriggerSkill("ikmiaowu") {
        events << EventPhaseStart;
        view_as_skill = new IkMiaowuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && !target->getPile("stars").isEmpty()
            && target->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *shenzhuge, QVariant &, ServerPlayer *) const{
        room->askForUseCard(shenzhuge, "@@ikmiaowu", "@ikmiaowu-card", -1, Card::MethodNone);
        return false;
    }
};

class IkMiaowuTrigger: public TriggerSkill {
public:
    IkMiaowuTrigger(): TriggerSkill("#ikmiaowu") {
        events << DamageForseen;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player->getMark("@miaowu") > 0) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Thunder)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = "#IkMiaowuProtect";
        log.from = player;
        log.arg = QString::number(damage.damage);
        if (damage.nature == DamageStruct::Normal)
            log.arg2 = "normal_nature";
        else if (damage.nature == DamageStruct::Fire)
            log.arg2 = "fire_nature";
        room->sendLog(log);

        return true;
    }
};

class IkMiaowuClear: public TriggerSkill {
public:
    IkMiaowuClear(): TriggerSkill("#ikmiaowu-clear") {
        events << EventPhaseStart << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        }
        if (!player->tag.value("IkMiaowu_user", false).toBool())
            return QStringList();
        bool invoke = false;
        if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) || triggerEvent == Death)
            invoke = true;
        if (!invoke)
            return QStringList();
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players)
            player->loseAllMarks("@miaowu");
        player->tag.remove("IkMiaowu_user");

        return QStringList();
    }
};

class Renjie: public TriggerSkill {
public:
    Renjie(): TriggerSkill("renjie") {
        events << Damaged << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            if (player->getPhase() == Player::Discard) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.from == player && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    int n = move.card_ids.length();
                    if (n > 0) {
                        room->broadcastSkillInvoke(objectName());
                        room->notifySkillInvoked(player, objectName());
                        player->gainMark("@bear", n);
                    }
                }
            }
        } else if (triggerEvent == Damaged) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            DamageStruct damage = data.value<DamageStruct>();
            player->gainMark("@bear", damage.damage);
        }

        return false;
    }
};

class Baiyin: public PhaseChangeSkill {
public:
    Baiyin(): PhaseChangeSkill("baiyin") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("baiyin") == 0
               && target->getMark("@bear") >= 4;
    }

    virtual bool onPhaseChange(ServerPlayer *shensimayi) const{
        Room *room = shensimayi->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(shensimayi, objectName());
        room->doLightbox("$BaiyinAnimate");

        LogMessage log;
        log.type = "#BaiyinWake";
        log.from = shensimayi;
        log.arg = QString::number(shensimayi->getMark("@bear"));
        room->sendLog(log);
        
        room->setPlayerMark(shensimayi, "baiyin", 1);
        if (room->changeMaxHpForAwakenSkill(shensimayi) && shensimayi->getMark("baiyin") == 1)
            room->acquireSkill(shensimayi, "jilve");

        return false;
    }
};

JilveCard::JilveCard() {
    target_fixed = true;
    mute = true;
}

void JilveCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *shensimayi = card_use.from;

    QStringList choices;
    if (!shensimayi->hasFlag("JilveIkZhiheng") && shensimayi->canDiscard(shensimayi, "he"))
        choices << "ikzhiheng";
    if (!shensimayi->hasFlag("JilveIkSishideng"))
        choices << "iksishideng";
    choices << "cancel";

    if (choices.length() == 1)
        return;

    QString choice = room->askForChoice(shensimayi, "jilve", choices.join("+"));
    if (choice == "cancel") {
        room->addPlayerHistory(shensimayi, "JilveCard", -1);
        return;
    }

    shensimayi->loseMark("@bear");
    room->notifySkillInvoked(shensimayi, "jilve");

    if (choice == "iksishideng") {
        room->setPlayerFlag(shensimayi, "JilveIkSishideng");
        room->acquireSkill(shensimayi, "iksishideng");
    } else {
        room->setPlayerFlag(shensimayi, "JilveIkZhiheng");
        room->askForUseCard(shensimayi, "@ikzhiheng", "@jilve-ikzhiheng", -1, Card::MethodDiscard);
    }
}

class JilveViewAsSkill: public ZeroCardViewAsSkill {
public: // iksishideng & ikzhiheng
    JilveViewAsSkill(): ZeroCardViewAsSkill("jilve") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->usedTimes("JilveCard") < 2 && player->getMark("@bear") > 0;
    }

    virtual const Card *viewAs() const{
        return new JilveCard;
    }
};

class Jilve: public TriggerSkill {
public:
    Jilve(): TriggerSkill("jilve") {
        events << CardUsed // JiZhi
               << AskForRetrial // IkTiansuo
               << Damaged; // IkBisuo
        view_as_skill = new JilveViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("@bear") > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        player->setMark("JilveEvent", (int)triggerEvent);
        try {
            if (triggerEvent == CardUsed) {
                const TriggerSkill *jizhi = Sanguosha->getTriggerSkill("jizhi");
                CardUseStruct use = data.value<CardUseStruct>();
                if (jizhi && use.card && use.card->getTypeId() == Card::TypeTrick && player->askForSkillInvoke("jilve_jizhi", data)) {
                    room->notifySkillInvoked(player, objectName());
                    player->loseMark("@bear");
                    jizhi->trigger(triggerEvent, room, player, data);
                }
            } else if (triggerEvent == AskForRetrial) {
                const TriggerSkill *iktiansuo = Sanguosha->getTriggerSkill("iktiansuo");
                if (iktiansuo && !player->isKongcheng() && player->askForSkillInvoke("jilve_iktiansuo", data)) {
                    room->notifySkillInvoked(player, objectName());
                    player->loseMark("@bear");
                    iktiansuo->trigger(triggerEvent, room, player, data);
                }
            } else if (triggerEvent == Damaged) {
                const TriggerSkill *ikbisuo = Sanguosha->getTriggerSkill("ikbisuo");
                if (ikbisuo && player->askForSkillInvoke("jilve_ikbisuo", data)) {
                    room->notifySkillInvoked(player, objectName());
                    player->loseMark("@bear");
                    ikbisuo->trigger(triggerEvent, room, player, data);
                }
            }
            player->setMark("JilveEvent", 0);
        }
        catch (TriggerEvent triggerEvent) {
            if (triggerEvent == StageChange || triggerEvent == TurnBroken)
                player->setMark("JilveEvent", 0);
            throw triggerEvent;
        }

        return false;
    }
};

class JilveClear: public TriggerSkill {
public:
    JilveClear(): TriggerSkill("#jilve-clear") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag("JilveIkSishideng");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->detachSkillFromPlayer(target, "iksishideng", false, true);
        return false;
    }
};

class LianpoCount: public TriggerSkill {
public:
    LianpoCount(): TriggerSkill("#lianpo-count") {
        events << Death << TurnStart;
        //global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
            ServerPlayer *killer = death.damage ? death.damage->from : NULL;
            ServerPlayer *current = room->getCurrent();

            if (killer && current && (current->isAlive() || death.who == current)
                && current->getPhase() != Player::NotActive) {
                killer->addMark("lianpo");

                if (TriggerSkill::triggerable(player)) {
                    LogMessage log;
                    log.type = "#LianpoRecord";
                    log.from = killer;
                    log.to << player;

                    log.arg = current->getGeneralName();
                    room->sendLog(log);
                }
            }
        } else {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                p->setMark("lianpo", 0);
        }

        return false;
    }
};

class Lianpo: public PhaseChangeSkill {
public:
    Lianpo(): PhaseChangeSkill("lianpo") {
        frequency = Frequent;
    }

    virtual int getPriority(TriggerEvent) const{
        return 1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (player->getPhase() == Player::NotActive) {
            ServerPlayer *shensimayi = room->findPlayerBySkillName("lianpo");
            if (shensimayi == NULL || shensimayi->getMark("lianpo") <= 0 || !shensimayi->askForSkillInvoke("lianpo"))
                return false;

            LogMessage log;
            log.type = "#LianpoCanInvoke";
            log.from = shensimayi;
            log.arg = QString::number(shensimayi->getMark("lianpo"));
            log.arg2 = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke(objectName());
            shensimayi->gainAnExtraTurn();
        }
        return false;
    }
};

class Juejing: public DrawCardsSkill {
public:
    Juejing(): DrawCardsSkill("#juejing-draw") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        if (player->isWounded()) {
            Room *room = player->getRoom();
            room->notifySkillInvoked(player, "juejing");
            room->broadcastSkillInvoke("juejing");

            LogMessage log;
            log.type = "#YongsiGood";
            log.from = player;
            log.arg = QString::number(player->getLostHp());
            log.arg2 = "juejing";
            room->sendLog(log);
        }
        return n + player->getLostHp();
    }
};

class JuejingKeep: public MaxCardsSkill {
public:
    JuejingKeep(): MaxCardsSkill("juejing") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return 2;
        else
            return 0;
    }
};

Longhun::Longhun(): ViewAsSkill("longhun") {
    response_or_use = true;
}

bool Longhun::isEnabledAtResponse(const Player *player, const QString &pattern) const{
    return pattern == "slash"
           || pattern == "jink"
           || (pattern.contains("peach") && player->getMark("Global_PreventPeach") == 0)
           || pattern == "nullification";
}

bool Longhun::isEnabledAtPlay(const Player *player) const{
    return player->isWounded() || Slash::IsAvailable(player);
}

bool Longhun::viewFilter(const QList<const Card *> &selected, const Card *card) const{
    int n = qMax(1, Self->getHp());

    if (selected.length() >= n || card->hasFlag("using"))
        return false;

    if (n > 1 && !selected.isEmpty()) {
        Card::Suit suit = selected.first()->getSuit();
        return card->getSuit() == suit;
    }

    switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
    case CardUseStruct::CARD_USE_REASON_PLAY: {
            if (Self->isWounded() && card->getSuit() == Card::Heart)
                return true;
            else if (card->getSuit() == Card::Diamond) {
                FireSlash *slash = new FireSlash(Card::SuitToBeDecided, -1);
                slash->addSubcards(selected);
                slash->addSubcard(card->getEffectiveId());
                slash->deleteLater();
                return slash->isAvailable(Self);
            } else
                return false;
        }
    case CardUseStruct::CARD_USE_REASON_RESPONSE:
    case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "jink")
                return card->getSuit() == Card::Club;
            else if (pattern == "nullification")
                return card->getSuit() == Card::Spade;
            else if (pattern == "peach" || pattern == "peach+analeptic")
                return card->getSuit() == Card::Heart;
            else if (pattern == "slash")
                return card->getSuit() == Card::Diamond;
        }
    default:
        break;
    }

    return false;
}

const Card *Longhun::viewAs(const QList<const Card *> &cards) const{
    int n = getEffHp(Self);

    if (cards.length() != n)
        return NULL;

    const Card *card = cards.first();
    Card *new_card = NULL;

    switch (card->getSuit()) {
    case Card::Spade: {
            new_card = new Nullification(Card::SuitToBeDecided, 0);
            break;
        }
    case Card::Heart: {
            new_card = new Peach(Card::SuitToBeDecided, 0);
            break;
        }
    case Card::Club: {
            new_card = new Jink(Card::SuitToBeDecided, 0);
            break;
        }
    case Card::Diamond: {
            new_card = new FireSlash(Card::SuitToBeDecided, 0);
            break;
        }
    default:
        break;
    }

    if (new_card) {
        new_card->setSkillName(objectName());
        new_card->addSubcards(cards);
    }

    return new_card;
}

int Longhun::getEffectIndex(const ServerPlayer *player, const Card *card) const{
    return static_cast<int>(player->getRoom()->getCard(card->getSubcards().first())->getSuit()) + 1;
}

bool Longhun::isEnabledAtNullification(const ServerPlayer *player) const{
    int n = getEffHp(player), count = 0;
    foreach (const Card *card, player->getHandcards() + player->getEquips()) {
        if (card->getSuit() == Card::Spade) count++;
        if (count >= n) return true;
    }
    return false;
}

int Longhun::getEffHp(const Player *zhaoyun) const{
    return qMax(1, zhaoyun->getHp());
}

GodPackage::GodPackage()
    : Package("god")
{
    General *shenguanyu = new General(this, "shenguanyu", "god", 5); // LE 001
    shenguanyu->addSkill(new Wushen);
    shenguanyu->addSkill(new WushenTargetMod);
    shenguanyu->addSkill(new Wuhun);
    shenguanyu->addSkill(new WuhunRevenge);
    related_skills.insertMulti("wushen", "#wushen-target");
    related_skills.insertMulti("wuhun", "#wuhun");

    General *snow029 = new General(this, "snow029", "yuki", 3);
    snow029->addSkill(new IkLvejue);
    snow029->addSkill(new IkLingshi);

    General *snow030 = new General(this, "snow030", "yuki");
    snow030->addSkill(new IkLongxi);
    snow030->addSkill(new IkLongxiRecord);
    related_skills.insertMulti("iklongxi", "#iklongxi-record");
    snow030->addSkill(new IkYeyan);

    General *wind029 = new General(this, "wind029", "kaze", 3);
    wind029->addSkill(new IkQiyao);
    wind029->addSkill(new IkQiyaoStart);
    wind029->addSkill(new FakeMoveSkill("ikqiyao"));
    related_skills.insertMulti("ikqiyao", "#ikqiyao");
    related_skills.insertMulti("ikqiyao", "#ikqiyao-fake-move");
    wind029->addSkill(new IkLiefeng);
    wind029->addSkill(new IkLiefengTrigger);
    wind029->addSkill(new IkLiefengClear);
    related_skills.insertMulti("ikliefeng", "#ikliefeng");
    related_skills.insertMulti("ikliefeng", "#ikliefeng-clear");
    wind029->addSkill(new IkMiaowu);
    wind029->addSkill(new IkMiaowuTrigger);
    wind029->addSkill(new IkMiaowuClear);
    related_skills.insertMulti("ikmiaowu", "#ikmiaowu");
    related_skills.insertMulti("ikmiaowu", "#ikmiaowu-clear");

    General *bloom029 = new General(this, "bloom029", "hana", 3);
    bloom029->addSkill(new IkYihuo);
    bloom029->addSkill(new IkGuixin);

    General *luna029 = new General(this, "luna029", "tsuki", 5);
    luna029->addSkill(new IkZhuohuo);
    luna029->addSkill(new MarkAssignSkill("@mailun", 2));
    luna029->addSkill(new IkWumou);
    luna029->addSkill(new IkSuikong);
    luna029->addSkill(new IkTianwu);
    related_skills.insertMulti("ikzhuohuo", "#@mailun-2");

    General *shenzhaoyun = new General(this, "shenzhaoyun", "god", 2); // LE 007
    shenzhaoyun->addSkill(new JuejingKeep);
    shenzhaoyun->addSkill(new Juejing);
    shenzhaoyun->addSkill(new Longhun);
    related_skills.insertMulti("juejing", "#juejing-draw");

    General *shensimayi = new General(this, "shensimayi", "god", 4); // LE 008
    shensimayi->addSkill(new Renjie);
    shensimayi->addSkill(new Baiyin);
    shensimayi->addRelateSkill("jilve");
    related_skills.insertMulti("jilve", "#jilve-clear");
    shensimayi->addSkill(new Lianpo);
    shensimayi->addSkill(new LianpoCount);
    related_skills.insertMulti("lianpo", "#lianpo-count");

    addMetaObject<IkLingshiCard>();
    addMetaObject<IkYeyanCard>();
    addMetaObject<IkTianwuCard>();
    addMetaObject<GreatIkYeyanCard>();
    addMetaObject<SmallIkYeyanCard>();
    addMetaObject<IkLiefengCard>();
    addMetaObject<IkMiaowuCard>();
    addMetaObject<IkYihuoCard>();
    addMetaObject<IkSuikongCard>();
    addMetaObject<JilveCard>();

    skills << new Jilve << new JilveClear << new IkYihuoViewAsSkill;
}

ADD_PACKAGE(God)

