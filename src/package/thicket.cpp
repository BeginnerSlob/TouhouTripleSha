#include "thicket.h"
#include "general.h"
#include "skill.h"
#include "room.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"

class IkLiangban: public PhaseChangeSkill {
public:
    IkLiangban(): PhaseChangeSkill("ikliangban") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *to = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "ikliangban-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkLiangbanTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *sunjian) const{
        Room *room = sunjian->getRoom();
        ServerPlayer *to = sunjian->tag["IkLiangbanTarget"].value<ServerPlayer *>();
        sunjian->tag.remove("IkLiangbanTarget");
        if (to) {
            int x = qMax(sunjian->getLostHp(), 1);
            if (x == 1) {
                to->drawCards(1, objectName());
                room->askForDiscard(to, objectName(), 1, 1, false, true);
            } else {
                QString choice = room->askForChoice(sunjian, objectName(), "d1tx+dxt1");
                if (choice == "d1tx") {
                    to->drawCards(1, objectName());
                    room->askForDiscard(to, objectName(), x, x, false, true);
                } else {
                    to->drawCards(x, objectName());
                    room->askForDiscard(to, objectName(), 1, 1, false, true);
                }
            }
        }
        return false;
    }
};

IkShenenCard::IkShenenCard() {
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
    m_skillName = "_ikshenen";
}

bool IkShenenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    return to_select->getHandcardNum() == Self->getMark("ikshenen");
}

void IkShenenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
                          targets.first()->objectName(), "ikshenen", QString());
    room->moveCardTo(this, targets.first(), Player::PlaceHand, reason);
}

class IkShenenViewAsSkill: public ViewAsSkill {
public:
    IkShenenViewAsSkill(): ViewAsSkill("ikshenen") {
        response_pattern = "@@ikshenen!";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (to_select->isEquipped())
            return false;

        int length = Self->getHandcardNum() / 2;
        return selected.length() < length;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != Self->getHandcardNum() / 2)
            return NULL;

        IkShenenCard *card = new IkShenenCard;
        card->addSubcards(cards);
        return card;
    }
};

class IkShenen: public DrawCardsSkill {
public:
    IkShenen(): DrawCardsSkill("ikshenen") {
        view_as_skill = new IkShenenViewAsSkill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual int getDrawNum(ServerPlayer *lusu, int n) const{
        lusu->setFlags("ikshenen");
        return n + 2;
    }
};

class IkShenenGive: public TriggerSkill {
public:
    IkShenenGive(): TriggerSkill("#ikshenen-give") {
        events << AfterDrawNCards;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *lusu) const{
        return lusu && lusu->hasFlag("ikshenen") && lusu->getHandcardNum() > 5;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *lusu, QVariant &, ServerPlayer *) const{
        lusu->setFlags("-ikshenen");

        QList<ServerPlayer *> other_players = room->getOtherPlayers(lusu);
        int least = 1000;
        foreach (ServerPlayer *player, other_players)
            least = qMin(player->getHandcardNum(), least);
        room->setPlayerMark(lusu, "ikshenen", least);
        bool used = room->askForUseCard(lusu, "@@ikshenen!", "@ikshenen", -1, Card::MethodNone);

        if (!used) {
            // force lusu to give his half cards
            ServerPlayer *beggar = NULL;
            foreach (ServerPlayer *player, other_players) {
                if (player->getHandcardNum() == least) {
                    beggar = player;
                    break;
                }
            }

            int n = lusu->getHandcardNum() / 2;
            QList<int> to_give = lusu->handCards().mid(0, n);
            IkShenenCard *ikshenen_card = new IkShenenCard;
            ikshenen_card->addSubcards(to_give);
            QList<ServerPlayer *> targets;
            targets << beggar;
            ikshenen_card->use(room, lusu, targets);
            delete ikshenen_card;
        }

        return false;
    }
};

IkDimengCard::IkDimengCard() {
}

bool IkDimengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (to_select == Self)
        return false;

    if (targets.isEmpty())
        return true;

    if (targets.length() == 1) {
        return qAbs(to_select->getHandcardNum() - targets.first()->getHandcardNum()) == subcardsLength();
    }

    return false;
}

bool IkDimengCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

#include "jsonutils.h"
void IkDimengCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const{
    ServerPlayer *a = targets.at(0);
    ServerPlayer *b = targets.at(1);
    a->setFlags("IkDimengTarget");
    b->setFlags("IkDimengTarget");

    int n1 = a->getHandcardNum();
    int n2 = b->getHandcardNum();

    try {
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p != a && p != b)
                room->doNotify(p, QSanProtocol::S_COMMAND_EXCHANGE_KNOWN_CARDS,
                               QSanProtocol::Utils::toJsonArray(a->objectName(), b->objectName()));
        }
        QList<CardsMoveStruct> exchangeMove;
        CardsMoveStruct move1(a->handCards(), b, Player::PlaceHand,
                              CardMoveReason(CardMoveReason::S_REASON_SWAP, a->objectName(), b->objectName(), "ikdimeng", QString()));
        CardsMoveStruct move2(b->handCards(), a, Player::PlaceHand,
                              CardMoveReason(CardMoveReason::S_REASON_SWAP, b->objectName(), a->objectName(), "ikdimeng", QString()));
        exchangeMove.push_back(move1);
        exchangeMove.push_back(move2);
        room->moveCardsAtomic(exchangeMove, false);

        LogMessage log;
        log.type = "#IkDimeng";
        log.from = a;
        log.to << b;
        log.arg = QString::number(n1);
        log.arg2 = QString::number(n2);
        room->sendLog(log);
        room->getThread()->delay();

        a->setFlags("-IkDimengTarget");
        b->setFlags("-IkDimengTarget");
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange) {
            a->setFlags("-IkDimengTarget");
            b->setFlags("-IkDimengTarget");
        }
        throw triggerEvent;
    }
}

class IkDimeng: public ViewAsSkill {
public:
    IkDimeng(): ViewAsSkill("ikdimeng") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.size() < 3 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        IkDimengCard *card = new IkDimengCard;
        foreach (const Card *c, cards)
            card->addSubcard(c);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkDimengCard");
    }
};

class IkSishideng: public TriggerSkill {
public:
    IkSishideng(): TriggerSkill("iksishideng") {
        // just to broadcast audio effects and to send log messages
        // main part in the AskForPeaches trigger of Game Rule
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player && player == room->getAllPlayers().first()) {
            DyingStruct dying = data.value<DyingStruct>();
            ServerPlayer *jiaxu = room->getCurrent();
            if (jiaxu && TriggerSkill::triggerable(jiaxu) && jiaxu->getPhase() != Player::NotActive)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual int getPriority(TriggerEvent) const{
        return 7;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *jiaxu = room->getCurrent();
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(jiaxu, objectName());

        LogMessage log;
        log.from = jiaxu;
        log.arg = objectName();
        if (jiaxu != dying.who) {
            log.type = "#IkSishidengTwo";
            log.to << dying.who;
        } else {
            log.type = "#IkSishidengOne";
        }
        room->sendLog(log);
        
        return false;
    }
};

class IkWenle: public ZeroCardViewAsSkill {
public:
    IkWenle(): ZeroCardViewAsSkill("ikwenle") {
        frequency = Limited;
        limit_mark = "@wenle";
    }

    virtual const Card *viewAs() const{
        return new IkWenleCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@wenle") >= 1;
    }
};

IkWenleCard::IkWenleCard() {
    target_fixed = true;
}

void IkWenleCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@wenle");
    room->addPlayerMark(source, "@wenleused");

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach (ServerPlayer *player, players) {
        if (player->isAlive()) {
            room->cardEffect(this, source, player);
            room->getThread()->delay();
        }
    }
}

void IkWenleCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    QList<ServerPlayer *> players = room->getOtherPlayers(effect.to);
    QList<int> distance_list;
    int nearest = 1000;
    foreach (ServerPlayer *player, players) {
        int distance = effect.to->distanceTo(player);
        distance_list << distance;
        nearest = qMin(nearest, distance);
    }

    QList<ServerPlayer *> ikwenle_targets;
    for (int i = 0; i < distance_list.length(); i++) {
        if (distance_list[i] == nearest && effect.to->canSlash(players[i], NULL, false))
            ikwenle_targets << players[i];
    }

    if (ikwenle_targets.isEmpty() || !room->askForUseSlashTo(effect.to, ikwenle_targets, "@ikwenle-slash"))
        room->loseHp(effect.to);
}

class IkMoyudeng: public ProhibitSkill {
public:
    IkMoyudeng(): ProhibitSkill("ikmoyudeng") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("TrickCard") || card->isKindOf("QiceCard"))
               && card->isBlack() && card->getSkillName() != "ikguihuo"; // Be care!!!!!!
    }
};

class IkFusheng: public OneCardViewAsSkill {
public:
    IkFusheng(): OneCardViewAsSkill("ikfusheng") {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return  pattern.contains("analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());
        return analeptic;
    }
};

class IkHuanbei: public TriggerSkill {
public:
    IkHuanbei(): TriggerSkill("ikhuanbei") {
        events << TargetConfirmed << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            if (triggerEvent == TargetSpecified) {
                foreach (ServerPlayer *p, use.to)
                    if (p->isFemale())
                        return QStringList(objectName());
            } else if (triggerEvent == TargetConfirmed && use.from->isFemale()) {
                if (use.to.contains(player))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        LogMessage log;
        log.from = player;
        log.arg = objectName();
        log.type = "#TriggerSkill";
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());

        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        if (triggerEvent == TargetSpecified) {
            foreach (ServerPlayer *p, use.to) {
                if (p->isFemale()) {
                    if (jink_list.at(index).toInt() == 1)
                        jink_list.replace(index, QVariant(2));
                }
                index++;
            }
            use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        } else if (triggerEvent == TargetConfirmed && use.from->isFemale()) {
            foreach (ServerPlayer *p, use.to) {
                if (p == player) {
                    if (jink_list.at(index).toInt() == 1)
                        jink_list.replace(index, QVariant(2));
                }
                index++;
            }
            use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        }

        return false;
    }
};

class IkBenghuai: public PhaseChangeSkill {
public:
    IkBenghuai(): PhaseChangeSkill("ikbenghuai") {
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish)
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (player->getHp() > p->getHp())
                    return QStringList(objectName());
        return QStringList();
    }

    virtual bool onPhaseChange(ServerPlayer *dongzhuo) const {
        Room *room = dongzhuo->getRoom();

        LogMessage log;
        log.from = dongzhuo;
        log.arg = objectName();
        log.type = "#TriggerSkill";
        room->sendLog(log);
        room->notifySkillInvoked(dongzhuo, objectName());

        QString result = room->askForChoice(dongzhuo, "ikbenghuai", "hp+maxhp");
        room->broadcastSkillInvoke(objectName());
        if (result == "hp")
            room->loseHp(dongzhuo);
        else
            room->loseMaxHp(dongzhuo);

        return false;
    }
};

class IkWuhua: public TriggerSkill {
public:
    IkWuhua(): TriggerSkill("ikwuhua$") {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player->tag.value("InvokeIkWuhua", false).toBool()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill(objectName()))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        QList<ServerPlayer *> dongzhuos;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasLordSkill(objectName()))
                dongzhuos << p;
        }

        while (!dongzhuos.isEmpty()) {
            ServerPlayer *dongzhuo = room->askForPlayerChosen(player, dongzhuos, objectName(), "@ikwuhua-to", true);
            if (dongzhuo) {
                dongzhuos.removeOne(dongzhuo);

                LogMessage log;
                log.type = "#InvokeOthersSkill";
                log.from = player;
                log.to << dongzhuo;
                log.arg = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(dongzhuo, objectName());

                JudgeStruct judge;
                judge.pattern = ".|spade";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                room->judge(judge);

                if (judge.isGood()) {
                    room->broadcastSkillInvoke(objectName());
                    room->recover(dongzhuo, RecoverStruct(player));
                }
            } else
                break;
        }

        return false;
    }
};

class IkWuhuaRecord: public TriggerSkill {
public:
    IkWuhuaRecord(): TriggerSkill("#ikwuhua-record") {
        events << PreDamageDone;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *qun = damage.from;
        if (qun)
            qun->tag["InvokeIkWuhua"] = qun->getKingdom() == "tsuki";
        return QStringList();
    }
};

ThicketPackage::ThicketPackage()
    : Package("thicket")
{

    General *snow009 = new General(this, "snow009", "yuki");
    snow009->addSkill(new IkLiangban);

    General *snow010 = new General(this, "snow010", "yuki", 3);
    snow010->addSkill(new IkShenen);
    snow010->addSkill(new IkShenenGive);
    snow010->addSkill(new IkDimeng);
    related_skills.insertMulti("ikshenen", "#ikshenen-give");

    General *luna001 = new General(this, "luna001$", "tsuki", 8);
    luna001->addSkill(new IkFusheng);
    luna001->addSkill(new IkHuanbei);
    luna001->addSkill(new IkBenghuai);
    luna001->addSkill(new IkWuhua);
    luna001->addSkill(new IkWuhuaRecord);
    related_skills.insertMulti("ikwuhua", "#ikwuhua-record");

    General *luna007 = new General(this, "luna007", "tsuki", 3);
    luna007->addSkill(new IkSishideng);
    luna007->addSkill(new IkWenle);
    luna007->addSkill(new IkMoyudeng);

    addMetaObject<IkDimengCard>();
    addMetaObject<IkWenleCard>();
    addMetaObject<IkShenenCard>();
}

ADD_PACKAGE(Thicket)

