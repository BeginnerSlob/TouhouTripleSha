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

class IkTianwubaka: public ZeroCardViewAsSkill {
public:
    IkTianwubaka(): ZeroCardViewAsSkill("iktianwubaka") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@mailun") >= 6 && !player->hasUsed("IkTianwubakaCard");
    }

    virtual const Card *viewAs() const{
        return new IkTianwubakaCard;
    }
};

IkTianwubakaCard::IkTianwubakaCard() {
    target_fixed = true;
    mute = true;
}

void IkTianwubakaCard::use(Room *room, ServerPlayer *shenlvbu, QList<ServerPlayer *> &) const{
    room->broadcastSkillInvoke("iktianwubaka");
    shenlvbu->loseMark("@mailun", 6);

    QList<ServerPlayer *> players = room->getOtherPlayers(shenlvbu);
    foreach (ServerPlayer *player, players) {
        room->damage(DamageStruct("iktianwubaka", shenlvbu, player));
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
        room->askForDiscard(player, "iktianwubaka", 4, 4);
        if (delay)
            room->getThread()->delay();
    }

    shenlvbu->turnOver();
}

class IkRenjia: public TriggerSkill {
public:
    IkRenjia(): TriggerSkill("ikrenjia") {
        events << Damaged << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardsMoveOneTime) {
            if (player->getPhase() == Player::Discard) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.from == player && move.from_places.contains(Player::PlaceHand)
                    && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                    return QStringList(objectName());
            }
        } else if (triggerEvent == Damaged)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        int n = 0;
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            foreach (Player::Place place, move.from_places)
                if (place == Player::PlaceHand)
                    n++;
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            n = damage.damage;
        }
        player->gainMark("@tianjia", n);
        return false;
    }
};

class IkTiangai: public PhaseChangeSkill {
public:
    IkTiangai(): PhaseChangeSkill("iktiangai") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("@tiangai") == 0
               && target->getMark("@tianjia") >= 4;
    }

    virtual bool onPhaseChange(ServerPlayer *shensimayi) const{
        Room *room = shensimayi->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(shensimayi, objectName());

        LogMessage log;
        log.type = "#IkTiangaiWake";
        log.from = shensimayi;
        log.arg = QString::number(shensimayi->getMark("@tianjia"));
        room->sendLog(log);
        
        room->setPlayerMark(shensimayi, "@tiangai", 1);
        if (room->changeMaxHpForAwakenSkill(shensimayi))
            room->acquireSkill(shensimayi, "ikjilve");

        return false;
    }
};

IkJilveCard::IkJilveCard() {
    target_fixed = true;
    mute = true;
}

void IkJilveCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *shensimayi = card_use.from;
    shensimayi->loseMark("@tianjia");

    room->setPlayerFlag(shensimayi, "IkJilve");
    room->acquireSkill(shensimayi, "iksishideng");
}

class IkJilveViewAsSkill: public ZeroCardViewAsSkill {
public: // iksishideng
    IkJilveViewAsSkill(): ZeroCardViewAsSkill("ikjilve") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkJilveCard") && player->getMark("@tianjia") > 0;
    }

    virtual const Card *viewAs() const{
        return new IkJilveCard;
    }
};

class IkJilve: public TriggerSkill {
public:
    IkJilve(): TriggerSkill("ikjilve") {
        events << CardUsed // IkHuiquan
               << EventPhaseStart // IkYuxi
               << CardsMoveOneTime; // ThXijing
        view_as_skill = new IkJilveViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player) || player->getMark("@tianjia") == 0)
            return QStringList();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->getTypeId() == Card::TypeTrick)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Start)
                return QStringList(objectName());
        } else if (triggerEvent == CardsMoveOneTime) {
            if ((room->getCurrent() == player && player->getPhase() != Player::NotActive)
                || player->isKongcheng() || player->hasFlag("thxijing_using"))
                return QStringList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player || move.to_place != Player::DiscardPile)
                return QStringList();
            
            for (int i = 0; i < move.card_ids.length(); i++) {
                int id = move.card_ids[i];
                if (move.from_places[i] != Player::PlaceJudge && move.from_places[i] != Player::PlaceSpecial
                    && !Sanguosha->getCard(id)->isKindOf("EquipCard")
                    && room->getCardPlace(id) == Player::DiscardPile) {
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == CardUsed) {
            const TriggerSkill *jizhi = Sanguosha->getTriggerSkill("ikhuiquan");
            if (jizhi && jizhi->cost(triggerEvent, room, player, data, player)) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                player->loseMark("@tianjia");
                return true;
            }
        } else if (triggerEvent == EventPhaseStart) {
            const TriggerSkill *guanxing = Sanguosha->getTriggerSkill("ikyuxi");
            if (guanxing && guanxing->cost(triggerEvent, room, player, data, player)) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                player->loseMark("@tianjia");
                return true;
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            for (int i = 0; i < move.card_ids.length(); i++) {
                int id = move.card_ids[i];
                if (move.from_places[i] != Player::PlaceJudge && move.from_places[i] != Player::PlaceSpecial
                    && !Sanguosha->getEngineCard(id)->isKindOf("EquipCard")
                    && room->getCardPlace(id) == Player::DiscardPile) {
                    const Card *c = Sanguosha->getEngineCard(id);
                    QString prompt = "@thxijing:" + c->getSuitString()
                                                  + ":" + QString::number(c->getNumber())
                                                  + ":" + c->objectName();
                    QString pattern = ".";
                    if (c->isBlack())
                        pattern = ".black";
                    else if (c->isRed())
                        pattern = ".red";
                    const Card *card = room->askForCard(player, pattern, prompt, QVariant(), Card::MethodNone);
                    
                    if (card) {
                        LogMessage log;
                        log.type = "#InvokeSkill";
                        log.from = player;
                        log.arg = objectName();
                        room->sendLog(log);

                        player->loseMark("@tianjia");
                        room->setPlayerFlag(player, "thxijing_using");
                        CardMoveReason reason(CardMoveReason::S_REASON_PUT,
                                              player->objectName(),
                                              objectName(),
                                              QString());
                        room->throwCard(card, reason, player);
                        room->setPlayerFlag(player, "-thxijing_using");
                        room->obtainCard(player, Sanguosha->getCard(id));
                    }
                }
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == CardUsed) {
            const TriggerSkill *jizhi = Sanguosha->getTriggerSkill("ikhuiquan");
            if (jizhi)
                return jizhi->effect(triggerEvent, room, player, data, player);
        } else if (triggerEvent == EventPhaseStart) {
            const TriggerSkill *guanxing = Sanguosha->getTriggerSkill("ikyuxi");
            if (guanxing)
                return guanxing->effect(triggerEvent, room, player, data, player);
        }

        return false;
    }
};

class IkJilveClear: public TriggerSkill {
public:
    IkJilveClear(): TriggerSkill("#ikjilve-clear") {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
        if (!target->hasFlag("IkJilve")) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return QStringList();
        room->detachSkillFromPlayer(target, "iksishideng", false, true);
        return QStringList();
    }
};

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

    General *bloom029 = new General(this, "bloom029", "hana", 3);
    bloom029->addSkill(new IkYihuo);
    bloom029->addSkill(new IkGuixin);

    General *luna029 = new General(this, "luna029", "tsuki", 5);
    luna029->addSkill(new IkZhuohuo);
    luna029->addSkill(new MarkAssignSkill("@mailun", 2));
    luna029->addSkill(new IkWumou);
    luna029->addSkill(new IkSuikong);
    luna029->addSkill(new IkTianwubaka);
    related_skills.insertMulti("ikzhuohuo", "#@mailun-2");

    General *bloom030 = new General(this, "bloom030", "hana");
    bloom030->addSkill(new IkRenjia);
    bloom030->addSkill(new IkTiangai);
    bloom030->addRelateSkill("ikjilve");
    related_skills.insertMulti("ikjilve", "#ikjilve-clear");

    addMetaObject<IkLingshiCard>();
    addMetaObject<IkYeyanCard>();
    addMetaObject<IkTianwubakaCard>();
    addMetaObject<GreatIkYeyanCard>();
    addMetaObject<SmallIkYeyanCard>();
    addMetaObject<IkYihuoCard>();
    addMetaObject<IkSuikongCard>();
    addMetaObject<IkJilveCard>();

    skills << new IkJilve << new IkJilveClear << new IkYihuoViewAsSkill;
}

ADD_PACKAGE(God)

