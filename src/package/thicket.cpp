#include "thicket.h"
#include "general.h"
#include "skill.h"
#include "room.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"

class IkTanwan: public TriggerSkill {
public:
    IkTanwan(): TriggerSkill("iktanwan") {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(caopi)) return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *player = death.who;
        if (player->isNude() || caopi == player)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &data, ServerPlayer *) const{
        if (caopi->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &data, ServerPlayer *) const{
        DeathStruct death = data.value<DeathStruct>();
        DummyCard *dummy = new DummyCard(death.who->handCards());
        QList <const Card *> equips = death.who->getEquips();
        foreach (const Card *card, equips)
            dummy->addSubcard(card);

        if (dummy->subcardsLength() > 0) {
            room->obtainCard(caopi, dummy, false);
        }
        delete dummy;

        return false;
    }
};

class IkBisuo: public MasochismSkill {
public:
    IkBisuo(): MasochismSkill("ikbisuo") {
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &data, ServerPlayer *) const{
        ServerPlayer *to = room->askForPlayerChosen(caopi, room->getOtherPlayers(caopi), objectName(),
                                                    "ikbisuo-invoke", caopi->getMark("JilveEvent") != int(Damaged), true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            caopi->tag["ThBisuoTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *caopi, const DamageStruct &) const{
        ServerPlayer *to = caopi->tag["ThBisuoTarget"].value<PlayerStar>();
        caopi->tag.remove("ThBisuoTarget");
        if (to) {
            if (caopi->isWounded())
                to->drawCards(caopi->getLostHp(), objectName());
            to->turnOver();
        }
    }
};

class IkSongwei: public TriggerSkill {
public:
    IkSongwei(): TriggerSkill("iksongwei$") {
        events << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player->getKingdom() != "hana")
            return QStringList();
        JudgeStar judge = data.value<JudgeStar>();

        if (judge->card->isBlack()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill("iksongwei"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        QList<ServerPlayer *> caopis;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasLordSkill(objectName()))
                caopis << p;
        }
        
        while (!caopis.isEmpty()) {
            ServerPlayer *caopi = room->askForPlayerChosen(player, caopis, objectName(), "@iksongwei-to", true);
            if (caopi) {
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(caopi, objectName());
                LogMessage log;
                log.type = "#InvokeOthersSkill";
                log.from = player;
                log.to << caopi;
                log.arg = objectName();
                room->sendLog(log);

                caopi->drawCards(1, objectName());
                caopis.removeOne(caopi);
            } else
                break;
        }

        return false;
    }
};

class IkKujie: public OneCardViewAsSkill {
public:
    IkKujie(): OneCardViewAsSkill("ikkujie") {
        filter_pattern = "^TrickCard|black";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
        shortage->setSkillName(objectName());
        shortage->addSubcard(originalCard);

        return shortage;
    }
};

class IkKujieTargetMod: public TargetModSkill {
public:
    IkKujieTargetMod(): TargetModSkill("#ikkujie-target") {
        pattern = "SupplyShortage";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill("ikkujie"))
            return 1;
        else
            return 0;
    }
};

class SavageAssaultAvoid: public TriggerSkill {
public:
    SavageAssaultAvoid(const QString &avoid_skill)
        : TriggerSkill("#sa_avoid_" + avoid_skill), avoid_skill(avoid_skill) {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player->isDead() || !player->hasSkill(avoid_skill)) return QStringList();
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isKindOf("SavageAssault"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->broadcastSkillInvoke(avoid_skill);
        room->notifySkillInvoked(player, avoid_skill);

        LogMessage log;
        log.type = "#SkillNullify";
        log.from = player;
        log.arg = avoid_skill;
        log.arg2 = "savage_assault";
        room->sendLog(log);

        return true;
    }

private:
    QString avoid_skill;
};

class IkHuoshou: public TriggerSkill {
public:
    IkHuoshou(): TriggerSkill("ikhuoshou") {
        events << TargetSpecified << ConfirmDamage;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("SavageAssault"))
                foreach (ServerPlayer *menghuo, room->findPlayersBySkillName(objectName()))
                    if (menghuo && menghuo != use.from && menghuo->hasSkill("ikzailuan")) {
                        ask_who = menghuo;
                        return QStringList(objectName());
                    }
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.card || !damage.card->isKindOf("SavageAssault"))
                return QStringList();
            ServerPlayer *menghuo = NULL;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (damage.card->hasFlag("IkHuoshouDamage_" + p->objectName())) {
                    menghuo = p;
                    break;
                }
            }
            if (!menghuo) return QStringList();
            damage.from = menghuo->isAlive() ? menghuo : NULL;
            data = QVariant::fromValue(damage);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        CardUseStruct use = data.value<CardUseStruct>();

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = ask_who;
        log.arg = objectName();
        room->sendLog(log);

        room->notifySkillInvoked(ask_who, objectName());
        room->broadcastSkillInvoke(objectName());

        use.card->setFlags("IkHuoshouDamage_" + ask_who->objectName());

        return false;
    }
};

class IkLieren: public TriggerSkill {
public:
    IkLieren(): TriggerSkill("iklieren") {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *zhurong, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(zhurong)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel")) && !zhurong->isKongcheng()
            && !target->isKongcheng() && !target->hasFlag("Global_DebutFlag") && !damage.chain && !damage.transfer)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhurong, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        bool success = zhurong->pindian(target, "iklieren", NULL);
        if (!success) return false;

        if (!target->isNude()) {
            int card_id = room->askForCardChosen(zhurong, target, "he", objectName());
            room->obtainCard(zhurong, Sanguosha->getCard(card_id), false);
        }

        return false;
    }
};

class IkZailuan: public PhaseChangeSkill {
public:
    IkZailuan(): PhaseChangeSkill("ikzailuan") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Draw
            && target->isWounded();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *menghuo) const{
        Room *room = menghuo->getRoom();

        int x = menghuo->getLostHp();
        QList<int> ids = room->getNCards(x, false);
        CardsMoveStruct move(ids, menghuo, Player::PlaceTable,
                             CardMoveReason(CardMoveReason::S_REASON_TURNOVER, menghuo->objectName(), "ikzailuan", QString()));
        room->moveCardsAtomic(move, true);

        room->getThread()->delay();
        room->getThread()->delay();

        QList<int> card_to_throw;
        QList<int> card_to_gotback;
        for (int i = 0; i < x; i++) {
            if (Sanguosha->getCard(ids[i])->getSuit() == Card::Heart) {
                card_to_throw << ids[i];
                QStringList choices;
                if (menghuo->isWounded())
                    choices << "recover";
                choices << "draw";
                QString choice = room->askForChoice(menghuo, objectName(), choices.join("+"));
                if (choice == "recover")
                    room->recover(menghuo, RecoverStruct(menghuo));
                else
                    menghuo->drawCards(2);
            } else
                card_to_gotback << ids[i];
        }
        if (!card_to_throw.isEmpty()) {
            DummyCard *dummy = new DummyCard(card_to_throw);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, menghuo->objectName(), "ikzailuan", QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
        }
        if (!card_to_gotback.isEmpty()) {
            DummyCard *dummy2 = new DummyCard(card_to_gotback);
            room->obtainCard(menghuo, dummy2);
            delete dummy2;
        }

        return true;
    }
};

class IkJugui: public TriggerSkill {
public:
    IkJugui(): TriggerSkill("ikjugui") {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.card_ids.length() == 1 && move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
            && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
            CardStar card = move.reason.m_extraData.value<CardStar>();
            if (!card || !card->isKindOf("SavageAssault"))
                return QStringList();
            if (card->isVirtualCard()) {
                if (card->getSkillName() != "ikguihuo")
                    return QStringList();
            }
            if (player != move.from)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        CardStar card = move.reason.m_extraData.value<CardStar>();

        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        player->obtainCard(card);
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);

        return false;
    }
};

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
        ServerPlayer *to = sunjian->tag["IkLiangbanTarget"].value<PlayerStar>();
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

class Wansha: public TriggerSkill {
public:
    Wansha(): TriggerSkill("wansha") {
        // just to broadcast audio effects and to send log messages
        // main part in the AskForPeaches trigger of Game Rule
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const{
        return 7;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player == room->getAllPlayers().first()) {
            DyingStruct dying = data.value<DyingStruct>();
            ServerPlayer *jiaxu = room->getCurrent();
            if (!jiaxu || !TriggerSkill::triggerable(jiaxu) || jiaxu->getPhase() == Player::NotActive)
                return false;
            if (jiaxu->hasInnateSkill("wansha") || !jiaxu->hasSkill("jilve"))
                room->broadcastSkillInvoke(objectName());
            else
                room->broadcastSkillInvoke("jilve", 3);

            room->notifySkillInvoked(jiaxu, objectName());

            LogMessage log;
            log.from = jiaxu;
            log.arg = objectName();
            if (jiaxu != dying.who) {
                log.type = "#WanshaTwo";
                log.to << dying.who;
            } else {
                log.type = "#WanshaOne";
            }
            room->sendLog(log);
        }
        return false;
    }
};

class Luanwu: public ZeroCardViewAsSkill {
public:
    Luanwu(): ZeroCardViewAsSkill("luanwu") {
        frequency = Limited;
        limit_mark = "@chaos";
    }

    virtual const Card *viewAs() const{
        return new LuanwuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@chaos") >= 1;
    }
};

LuanwuCard::LuanwuCard() {
    mute = true;
    target_fixed = true;
}

void LuanwuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@chaos");
    room->broadcastSkillInvoke("luanwu");
    QString lightbox = "$LuanwuAnimate";
    if (source->getGeneralName() != "jiaxu" && (source->getGeneralName() == "sp_jiaxu" || source->getGeneral2Name() == "sp_jiaxu"))
        lightbox = lightbox + "SP";
    room->doLightbox(lightbox, 3000);

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach (ServerPlayer *player, players) {
        if (player->isAlive())
            room->cardEffect(this, source, player);
            room->getThread()->delay();
    }
}

void LuanwuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    QList<ServerPlayer *> players = room->getOtherPlayers(effect.to);
    QList<int> distance_list;
    int nearest = 1000;
    foreach (ServerPlayer *player, players) {
        int distance = effect.to->distanceTo(player);
        distance_list << distance;
        nearest = qMin(nearest, distance);
    }

    QList<ServerPlayer *> luanwu_targets;
    for (int i = 0; i < distance_list.length(); i++) {
        if (distance_list[i] == nearest && effect.to->canSlash(players[i], NULL, false))
            luanwu_targets << players[i];
    }

    if (luanwu_targets.isEmpty() || !room->askForUseSlashTo(effect.to, luanwu_targets, "@luanwu-slash"))
        room->loseHp(effect.to);
}

class Weimu: public ProhibitSkill {
public:
    Weimu(): ProhibitSkill("weimu") {
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
    General *bloom010 = new General(this, "bloom010", "hana");
    bloom010->addSkill(new IkKujie);
    bloom010->addSkill(new IkKujieTargetMod);
    related_skills.insertMulti("ikkujie", "#ikkujie-target");

    General *bloom014 = new General(this, "bloom014$", "hana", 3);
    bloom014->addSkill(new IkTanwan);
    bloom014->addSkill(new IkBisuo);
    bloom014->addSkill(new IkSongwei);

    General *wind013 = new General(this, "wind013", "kaze");
    wind013->addSkill(new SavageAssaultAvoid("ikhuoshou"));
    wind013->addSkill(new IkHuoshou);
    wind013->addSkill(new IkZailuan);
    related_skills.insertMulti("ikhuoshou", "#sa_avoid_ikhuoshou");

    General *wind015 = new General(this, "wind015", "kaze", 4, false);
    wind015->addSkill(new SavageAssaultAvoid("ikjugui"));
    wind015->addSkill(new IkJugui);
    wind015->addSkill(new IkLieren);
    related_skills.insertMulti("ikjugui", "#sa_avoid_ikjugui");

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

    General *jiaxu = new General(this, "jiaxu", "qun", 3); // QUN 007
    jiaxu->addSkill(new Wansha);
    jiaxu->addSkill(new Luanwu);
    jiaxu->addSkill(new Weimu);

    addMetaObject<IkDimengCard>();
    addMetaObject<LuanwuCard>();
    addMetaObject<IkShenenCard>();
}

ADD_PACKAGE(Thicket)

