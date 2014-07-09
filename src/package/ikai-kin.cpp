#include "ikai-kin.h"

#include "general.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "settings.h"
#include "maneuvering.h"

class IkHuowen: public PhaseChangeSkill {
public:
    IkHuowen(): PhaseChangeSkill("ikhuowen") {
    }

    virtual bool triggerable(const ServerPlayer *fazheng) const{
        return PhaseChangeSkill::triggerable(fazheng)
            && fazheng->getPhase() == Player::Draw;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *to = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "ikhuowen-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkHuowenTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *fazheng) const{
        Room *room = fazheng->getRoom();
        ServerPlayer *to = fazheng->tag["IkHuowenTarget"].value<ServerPlayer *>();
        fazheng->tag.remove("IkHuowenTarget");
        if (to) {
            room->drawCards(to, 2, objectName());
            if (!fazheng->isAlive() || !to->isAlive())
                return true;
        
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *vic, room->getOtherPlayers(to)) {
                if (to->canSlash(vic))
                    targets << vic;
            }
            ServerPlayer *victim = NULL;
            if (!targets.isEmpty()) {
                victim = room->askForPlayerChosen(fazheng, targets, "ikhuowen_slash", "@dummy-slash2:" + to->objectName());
        
                LogMessage log;
                log.type = "#CollateralSlash";
                log.from = fazheng;
                log.to << victim;
                room->sendLog(log);
            }
        
            if (victim == NULL || !room->askForUseSlashTo(to, victim, "ikhuowen-slash::" + victim->objectName())) {
                if (to->isNude())
                    return true;
                room->setPlayerFlag(to, "ikhuowen_InTempMoving");
                int first_id = room->askForCardChosen(fazheng, to, "he", "ikhuowen");
                Player::Place original_place = room->getCardPlace(first_id);
                DummyCard *dummy = new DummyCard;
                dummy->addSubcard(first_id);
                to->addToPile("#ikhuowen", dummy, false);
                if (!to->isNude()) {
                    int second_id = room->askForCardChosen(fazheng, to, "he", "ikhuowen");
                    dummy->addSubcard(second_id);
                }

                //move the first card back temporarily
                room->moveCardTo(Sanguosha->getCard(first_id), to, original_place, false);
                room->setPlayerFlag(to, "-ikhuowen_InTempMoving");
                room->moveCardTo(dummy, fazheng, Player::PlaceHand, false);
                delete dummy;
            }

            return true;
        }

        return false;
    }
};

class IkEnyuan: public TriggerSkill {
public:
    IkEnyuan(): TriggerSkill("ikenyuan") {
        events << CardsMoveOneTime << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.from && move.from->isAlive() && move.from != move.to
                && move.card_ids.size() >= 2
                && move.reason.m_reason != CardMoveReason::S_REASON_PREVIEWGIVE)
                return QStringList(objectName());
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (!source || source == player) return QStringList();
            if (source->isDead()) return QStringList();
            QStringList skill;
            for (int i = 0; i < damage.damage; i++)
                skill << objectName();
            return skill;
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = NULL;
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            target = (ServerPlayer *)move.from;
        } else
            target = data.value<DamageStruct>().from;
        if (target && player->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            room->drawCards((ServerPlayer *)move.from, 1, objectName());
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            const Card *card = NULL;
            if (!source->isKongcheng())
                card = room->askForExchange(source, objectName(), 1, 1, false, "IkEnyuanGive::" + player->objectName(), true);
            if (card) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
                                      player->objectName(), objectName(), QString());
                reason.m_playerId = player->objectName();
                room->moveCardTo(card, source, player, Player::PlaceHand, reason);
                delete card;
            } else {
                room->loseHp(source);
            }
        }
        return false;
    }
};

IkXinchaoCard::IkXinchaoCard() {
    target_fixed = true;
}

void IkXinchaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    QList<int> cards = room->getNCards(3), left;

    LogMessage log;
    log.type = "$ViewDrawPile";
    log.from = source;
    log.card_str = IntList2StringList(cards).join("+");
    room->sendLog(log, source);

    left = cards;

    QList<int> hearts, non_hearts;
    foreach (int card_id, cards) {
        const Card *card = Sanguosha->getCard(card_id);
        if (card->getSuit() == Card::Heart)
            hearts << card_id;
        else
            non_hearts << card_id;
    }

    if (!hearts.isEmpty()) {
        DummyCard *dummy = new DummyCard;
        do {
            room->fillAG(left, source, non_hearts);
            int card_id = room->askForAG(source, hearts, true, "ikxinchao");
            if (card_id == -1) {
                room->clearAG(source);
                break;
            }

            hearts.removeOne(card_id);
            left.removeOne(card_id);

            dummy->addSubcard(card_id);
            room->clearAG(source);
        } while (!hearts.isEmpty());

        if (dummy->subcardsLength() > 0) {
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, Json::Value(room->getDrawPile().length() + dummy->subcardsLength()));
            source->obtainCard(dummy);
            foreach (int id, dummy->getSubcards())
                room->showCard(source, id);
        }
        delete dummy;
    }

    if (!left.isEmpty())
        room->askForGuanxing(source, left, Room::GuanxingUpOnly);
 }

class IkXinchao: public ZeroCardViewAsSkill {
public:
    IkXinchao(): ZeroCardViewAsSkill("ikxinchao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkXinchaoCard") && player->getHandcardNum() > player->getHp();
    }

    virtual const Card *viewAs() const{
        return new IkXinchaoCard;
    }
};

class IkShangshi: public TriggerSkill {
public:
    IkShangshi():TriggerSkill("ikshangshi") {
        events << Death;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL && player->hasSkill(objectName())) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
            ServerPlayer *killer = death.damage ? death.damage->from : NULL;
            if (killer && killer->isAlive() && killer != player && killer->canDiscard(killer, "he"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DeathStruct death = data.value<DeathStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(death.damage->from))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *killer = death.damage->from;
        killer->throwAllHandCardsAndEquips();

        return false;
    }
};

class IkMitu: public TriggerSkill {
public:
    IkMitu(): TriggerSkill("ikmitu") {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->getTypeId() == Card::TypeTrick) {
            if (triggerEvent == DamageInflicted && TriggerSkill::triggerable(player)) {
                return QStringList(objectName());
            } else if (triggerEvent == DamageCaused && damage.from && TriggerSkill::triggerable(damage.from)) {
                ask_who = damage.from;
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        LogMessage log;
        log.type = triggerEvent == DamageCaused ? "#IkMituGood" : "#IkMituBad";
        log.from = ask_who;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(ask_who, objectName());
        room->broadcastSkillInvoke(objectName());

        return true;
    }
};

IkSishiCard::IkSishiCard() {
}

bool IkSishiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void IkSishiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();

    QStringList choicelist;
    choicelist << "draw";
    if (effect.to->isWounded())
        choicelist << "recover";
    if (!effect.to->faceUp() || effect.to->isChained())
        choicelist << "reset";
    QString choice = room->askForChoice(effect.to, "iksishi", choicelist.join("+"));

    if (choice == "draw")
        effect.to->drawCards(2, "iksishi");
    else if (choice == "recover")
        room->recover(effect.to, RecoverStruct(effect.from));
    else if (choice == "reset") {
        if (effect.to->isChained())
            room->setPlayerProperty(effect.to, "chained", false);
        if (!effect.to->faceUp())
            effect.to->turnOver();
    }
}

class IkSishi: public OneCardViewAsSkill {
public:
    IkSishi(): OneCardViewAsSkill("iksishi") {
        filter_pattern = "^BasicCard!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkSishiCard") && player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkSishiCard *sishiCard = new IkSishiCard;
        sishiCard->addSubcard(originalCard);
        return sishiCard;
    }
};

class IkWanhun: public TriggerSkill {
public:
    IkWanhun(): TriggerSkill("ikwanhun") {
        events << DamageCaused;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (player->distanceTo(damage.to) <= 2 && damage.by_user && !damage.chain && !damage.transfer
            && damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel")))
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();

        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.who = player;
        judge.reason = objectName();

        room->judge(judge);
        if (judge.isGood()) {
            room->loseMaxHp(damage.to);
            return true;
        }
        
        return false;
    }
};

class IkMeiying: public TriggerSkill {
public:
    IkMeiying(): TriggerSkill("ikmeiying") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::RoundStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *liaohua, QVariant &) const{
        room->broadcastSkillInvoke(objectName());
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = liaohua;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(liaohua, objectName());

        ServerPlayer *target = NULL;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(liaohua))
            if (!p->isKongcheng())
                targets << p;
        if (!targets.isEmpty())
            target = room->askForPlayerChosen(liaohua, targets, objectName(), "@ikmeiying", true, true);
        if (target)
            room->showAllCards(target, liaohua);
        else {
            liaohua->setPhase(Player::Play);
            room->broadcastProperty(liaohua, "phase");
            RoomThread *thread = room->getThread();
            if (!thread->trigger(EventPhaseStart, room, liaohua))
                thread->trigger(EventPhaseProceeding, room, liaohua);
            thread->trigger(EventPhaseEnd, room, liaohua);

            liaohua->setPhase(Player::RoundStart);
            room->broadcastProperty(liaohua, "phase");
        }

        return false;
    }
};

class IkFansheng: public TriggerSkill {
public:
    IkFansheng(): TriggerSkill("ikfansheng") {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@fansheng";
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(target)) return QStringList();
        if (target->getHp() > 0 || target->getMark("@fansheng") <= 0) return QStringList();
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != target)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    int getKingdoms(Room *room) const{
        QSet<QString> kingdom_set;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            kingdom_set << p->getKingdom();
        return kingdom_set.size();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liaohua, QVariant &data, ServerPlayer *) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        room->removePlayerMark(liaohua, "@fansheng");
        room->addPlayerMark(liaohua, "@fanshengused");
        liaohua->drawCards(2);
        room->recover(liaohua, RecoverStruct(liaohua, NULL, getKingdoms(room) - liaohua->getHp()));
        liaohua->turnOver();
        
        return false;
    }
};

class IkLiyaoViewAsSkill: public OneCardViewAsSkill {
public:
    IkLiyaoViewAsSkill(): OneCardViewAsSkill("ikliyao") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player) && player->getPhase() == Player::Play;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" && player->getPhase() == Player::Play;
    }

    virtual bool viewFilter(const Card *card) const{
        if (card->getSuit() != Card::Diamond)
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(card->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard->getId());
        slash->setSkillName(objectName());
        return slash;
    }
};

class IkLiyao: public TriggerSkill {
public:
    IkLiyao(): TriggerSkill("ikliyao") {
        events << PreCardUsed;
        view_as_skill = new IkLiyaoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            if (player->hasFlag("ikliyao_black"))
                room->setPlayerFlag(player, "-ikliyao_black");
            else if (player->hasFlag("ikliyao_red"))
                room->setPlayerFlag(player, "-ikliyao_red");
            else if (player->hasFlag("ikliyao_nocolor"))
                room->setPlayerFlag(player, "-ikliyao_nocolor");
            if (use.card->isRed())
                room->setPlayerFlag(player, "ikliyao_red");
            else if (use.card->isBlack())
                room->setPlayerFlag(player, "ikliyao_black");
            else
                room->setPlayerFlag(player, "ikliyao_nocolor");
        }
        return QStringList();
    }
};

class IkLiyaoTargetMod: public TargetModSkill {
public:
    IkLiyaoTargetMod(): TargetModSkill("#ikliyao-target") {
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (from->hasSkill("ikliyao")) {
            if (card->hasFlag("Global_SlashAvailabilityChecker"))
                return 1000;
            if (from->hasFlag("ikliyao_black") && !card->isBlack())
                return 1000;
            else if (from->hasFlag("ikliyao_red") && !card->isRed())
                return 1000;
            else if (from->hasFlag("ikliyao_nocolor")) {
                if (card->isRed())
                    return 1000;
                if (card->isBlack())
                    return 1000;
                return 0;
            }
        }
        return 0;
    }
};

IkXianyuCard::IkXianyuCard() {
}

bool IkXianyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.length() < 2 && !to_select->isNude();
}

void IkXianyuCard::onEffect(const CardEffectStruct &effect) const{
    if (effect.to->isNude()) return;
    int id = effect.from->getRoom()->askForCardChosen(effect.from, effect.to, "he", "ikxianyu");
    effect.from->addToPile("ikxianyupile", id);
}

class IkXianyuViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkXianyuViewAsSkill(): ZeroCardViewAsSkill("ikxianyu") {
        response_pattern = "@@ikxianyu";
    }

    virtual const Card *viewAs() const{
        return new IkXianyuCard;
    }
};

class IkXianyu: public TriggerSkill {
public:
    IkXianyu(): TriggerSkill("ikxianyu") {
        events << EventPhaseStart;
        view_as_skill = new IkXianyuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        return room->askForUseCard(player, "@@ikxianyu", "@ikxianyu-card");
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        int index = qrand() % 2 + 1;
        if (card->isKindOf("Slash"))
            index += 2;
        return index;
    }
};

class IkXianyuAttach: public TriggerSkill {
public:
    IkXianyuAttach(): TriggerSkill("#ikxianyu-attach") {
        events << GameStart << EventAcquireSkill << EventLoseSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player == NULL) return QStringList();
        if ((triggerEvent == GameStart && player && player->isAlive() && player->hasSkill("ikxianyu"))
             || (triggerEvent == EventAcquireSkill && data.toString() == "ikxianyu")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill("ikxianyu"))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("ikxianyu_slash"))
                    room->attachSkillToPlayer(p, "ikxianyu_slash");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "ikxianyu") {
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
                if (p->hasSkill("ikxianyu_slash"))
                    room->detachSkillFromPlayer(p, "ikxianyu_slash", true);
            }
        }

        return QStringList();
    }
};

IkXianyuSlashCard::IkXianyuSlashCard() {
    m_skillName = "ikxianyu_slash";
}

bool IkXianyuSlashCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    if (targets.isEmpty()) {
        bool filter = to_select->hasSkill("ikxianyu") && to_select->getPile("ikxianyupile").length() >= 2
                      && slash->targetFilter(QList<const Player *>(), to_select, Self);
        delete slash;
        return filter;
    } else {
        slash->addSpecificAssignee(targets.first());
        bool filter = slash->targetFilter(targets, to_select, Self);
        delete slash;
        return filter;
    }
    return false;
}

const Card *IkXianyuSlashCard::validate(CardUseStruct &cardUse) const{
    Room *room = cardUse.from->getRoom();
    ServerPlayer *liufeng = cardUse.to.first();
    if (liufeng->getPile("ikxianyupile").length() < 2) return NULL;
    ServerPlayer *source = cardUse.from;

    DummyCard *dummy = new DummyCard;
    if (liufeng->getPile("ikxianyupile").length() == 2) {
        dummy->addSubcard(liufeng->getPile("ikxianyupile").first());
        dummy->addSubcard(liufeng->getPile("ikxianyupile").last());
    } else {
        int ai_delay = Config.AIDelay;
        Config.AIDelay = 0;

        QList<int> ids = liufeng->getPile("ikxianyupile");
        for (int i = 0; i < 2; i++) {
            room->fillAG(ids, source);
            int id = room->askForAG(source, ids, false, "ikxuanyu");
            dummy->addSubcard(id);
            ids.removeOne(id);
            room->clearAG(source);
        }

        Config.AIDelay = ai_delay;

    }

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "ikxianyu", QString());
    room->throwCard(dummy, reason, NULL);
    delete dummy;

    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    slash->setSkillName("_ikxianyu");

    QList<ServerPlayer *> targets = cardUse.to;
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, slash))
            cardUse.to.removeOne(target);
    }
    if (cardUse.to.length() > 0)
        return slash;
    else {
        delete slash;
        return NULL;
    }
}

class IkXianyuSlashViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkXianyuSlashViewAsSkill(): ZeroCardViewAsSkill("ikxianyu_slash") {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player) && canSlashLiufeng(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash"
               && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
               && canSlashLiufeng(player);
    }

    virtual const Card *viewAs() const{
        return new IkXianyuSlashCard;
    }

private:
    static bool canSlashLiufeng(const Player *player) {
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("ikxianyu") && p->getPile("ikxianyupile").length() > 1) {
                if (slash->targetFilter(QList<const Player *>(), p, player)) {
                    delete slash;
                    return true;
                }
            }
        }
        delete slash;
        return false;
    }
};

ExtraCollateralCard::ExtraCollateralCard() {
}

bool ExtraCollateralCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *coll = Card::Parse(Self->property("extra_collateral").toString());
    if (!coll) return false;
    QStringList tos = Self->property("extra_collateral_current_targets").toString().split("+");

    if (targets.isEmpty())
        return !tos.contains(to_select->objectName())
               && !Self->isProhibited(to_select, coll) && coll->targetFilter(targets, to_select, Self);
    else
        return coll->targetFilter(targets, to_select, Self);
}

void ExtraCollateralCard::onUse(Room *, const CardUseStruct &card_use) const{
    Q_ASSERT(card_use.to.length() == 2);
    ServerPlayer *killer = card_use.to.first();
    ServerPlayer *victim = card_use.to.last();

    killer->setFlags("ExtraCollateralTarget");
    killer->tag["collateralVictim"] = QVariant::fromValue(victim);
}

IkQizhiCard::IkQizhiCard() {
}

bool IkQizhiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void IkQizhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    bool success = source->pindian(targets.first(), "ikqizhi", NULL);
    if (success)
        source->setFlags("IkQizhiSuccess");
    else
        room->setPlayerCardLimitation(source, "use", "TrickCard", true);
}

class IkQizhiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkQizhiViewAsSkill(): ZeroCardViewAsSkill("ikqizhi") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern.startsWith("@@ikqizhi");
    }

    virtual const Card *viewAs() const{
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern.endsWith("!"))
            return new ExtraCollateralCard;
        else
            return new IkQizhiCard;
    }
};

class IkQizhi: public PhaseChangeSkill {
public:
    IkQizhi(): PhaseChangeSkill("ikqizhi") {
        view_as_skill = new IkQizhiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *jianyong) const {
        foreach (ServerPlayer *p, jianyong->getRoom()->getAllPlayers()) {
            if (p == jianyong) continue;
            if (!p->isKongcheng())
                return jianyong->getPhase() == Player::Play && !jianyong->isKongcheng();
        }
        return false;
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *jianyong, QVariant &, ServerPlayer *) const{
        return room->askForUseCard(jianyong, "@@ikqizhi", "@ikqizhi-card", 1);
    }

    virtual bool onPhaseChange(ServerPlayer *) const{
        return false;
    }
};

class IkQizhiUse: public TriggerSkill {
public:
    IkQizhiUse(): TriggerSkill("#ikqizhi-use") {
        events << PreCardUsed;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *jianyong, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(jianyong)) return QStringList();
        if (!jianyong->hasFlag("IkQizhiSuccess")) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isNDTrick() || use.card->isKindOf("BasicCard"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *jianyong, QVariant &data, ServerPlayer *) const{
        jianyong->setFlags("-IkQizhiSuccess");
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY)
            return false;

        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> available_targets;
        if (!use.card->isKindOf("AOE") && !use.card->isKindOf("GlobalEffect")) {
            room->setPlayerFlag(jianyong, "IkQizhiExtraTarget");
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (use.to.contains(p) || room->isProhibited(jianyong, p, use.card)) continue;
                if (use.card->targetFixed()) {
                    if (!use.card->isKindOf("Peach") || p->isWounded())
                        available_targets << p;
                } else {
                    if (use.card->targetFilter(QList<const Player *>(), p, jianyong))
                        available_targets << p;
                }
            }
            room->setPlayerFlag(jianyong, "-IkQizhiExtraTarget");
        }
        QStringList choices;
        choices << "cancel";
        if (use.to.length() > 1) choices.prepend("remove");
        if (!available_targets.isEmpty()) choices.prepend("add");
        if (choices.length() == 1) return false;

        QString choice = room->askForChoice(jianyong, "ikqizhi", choices.join("+"), data);
        if (choice == "cancel")
            return false;
        else if (choice == "add") {
            ServerPlayer *extra = NULL;
            if (!use.card->isKindOf("Collateral"))
                extra = room->askForPlayerChosen(jianyong, available_targets, "ikqizhi", "@thyongye-add:::" + use.card->objectName());
            else {
                QStringList tos;
                foreach (ServerPlayer *t, use.to)
                    tos.append(t->objectName());
                room->setPlayerProperty(jianyong, "extra_collateral", use.card->toString());
                room->setPlayerProperty(jianyong, "extra_collateral_current_targets", tos.join("+"));
                room->askForUseCard(jianyong, "@@ikqizhi!", "@thyongye-add:::collateral");
                room->setPlayerProperty(jianyong, "extra_collateral", QString());
                room->setPlayerProperty(jianyong, "extra_collateral_current_targets", QString("+"));
                foreach (ServerPlayer *p, room->getOtherPlayers(jianyong)) {
                    if (p->hasFlag("ExtraCollateralTarget")) {
                        p->setFlags("-ExtraCollateralTarget");
                        extra = p;
                        break;
                    }
                }
                if (extra == NULL) {
                    extra = available_targets.at(qrand() % available_targets.length() - 1);
                    QList<ServerPlayer *> victims;
                    foreach (ServerPlayer *p, room->getOtherPlayers(extra)) {
                        if (extra->canSlash(p)
                            && (!(p == jianyong && p->hasSkill("ikjingyou") && p->isLastHandCard(use.card, true)))) {
                            victims << p;
                        }
                    }
                    Q_ASSERT(!victims.isEmpty());
                    extra->tag["collateralVictim"] = QVariant::fromValue(victims.at(qrand() % victims.length()));
                }
            }
            use.to.append(extra);
            room->sortByActionOrder(use.to);

            LogMessage log;
            log.type = "#ThYongyeAdd";
            log.from = jianyong;
            log.to << extra;
            log.card_str = use.card->toString();
            log.arg = "ikqizhi";
            room->sendLog(log);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, jianyong->objectName(), extra->objectName());

            if (use.card->isKindOf("Collateral")) {
                ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                if (victim) {
                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = jianyong;
                    log.to << victim;
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                }
            }
        } else {
            ServerPlayer *removed = room->askForPlayerChosen(jianyong, use.to, "ikqizhi", "@thyongye-remove:::" + use.card->objectName());
            use.to.removeOne(removed);

            LogMessage log;
            log.type = "#ThYongyeRemove";
            log.from = jianyong;
            log.to << removed;
            log.card_str = use.card->toString();
            log.arg = "ikqizhi";
            room->sendLog(log);
        }

        data = QVariant::fromValue(use);

        return false;
    }
};

class IkQizhiTargetMod: public TargetModSkill {
public:
    IkQizhiTargetMod(): TargetModSkill("#ikqizhi-target") {
        frequency = NotFrequent;
        pattern = "Slash,TrickCard+^DelayedTrick";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasFlag("IkQizhiExtraTarget"))
            return 1000;
        else
            return 0;
    }
};

class IkZongshi: public TriggerSkill {
public:
    IkZongshi(): TriggerSkill("ikzongshi") {
        events << Pindian;
        frequency = Frequent;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        const Card *to_obtain = NULL;
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (TriggerSkill::triggerable(pindian->from)) {
            if (pindian->from_number > pindian->to_number)
                to_obtain = pindian->to_card;
            else
                to_obtain = pindian->from_card;
            if (room->getCardPlace(to_obtain->getEffectiveId()) == Player::PlaceTable)
                skill_list.insert(pindian->from, QStringList(objectName()));
        }
        if (TriggerSkill::triggerable(pindian->to)) {
            if (pindian->from_number < pindian->to_number)
                to_obtain = pindian->from_card;
            else
                to_obtain = pindian->to_card;
            if (room->getCardPlace(to_obtain->getEffectiveId()) == Player::PlaceTable)
                skill_list.insert(pindian->to, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        const Card *to_obtain = NULL;
        if (ask_who == pindian->from) {
            if (pindian->from_number > pindian->to_number)
                to_obtain = pindian->to_card;
            else
                to_obtain = pindian->from_card;
        } else if (ask_who == pindian->to) {
            if (pindian->from_number < pindian->to_number)
                to_obtain = pindian->from_card;
            else
                to_obtain = pindian->to_card;
        }
        ask_who->obtainCard(to_obtain);

        return false;
    }
};

class IkYaolun: public TriggerSkill {
public:
    IkYaolun(): TriggerSkill("ikyaolun") {
        events << CardUsed;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                foreach (ServerPlayer *guanping, room->findPlayersBySkillName(objectName()))
                    if (guanping->canDiscard(guanping, "he"))
                        skill_list.insert(guanping, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *guanping) const{
        if (room->askForCard(guanping, "..", "@ikyaolun", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *guanping) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.m_addHistory) {
            room->addPlayerHistory(player, use.card->getClassName(), -1);
            use.m_addHistory = false;
            data = QVariant::fromValue(use);
        }
        if (use.card->isRed())
            guanping->drawCards(1, objectName());
        return false;
    }
};

class IkQiansha: public TriggerSkill {
public:
    IkQiansha(): TriggerSkill("ikqiansha") {
        events << EventPhaseStart << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Start)
            return QStringList(objectName());
        else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName() || !target->isAlive()) return QStringList();

            QString color = judge->card->isRed() ? "red" : "black";
            target->tag[objectName()] = QVariant::fromValue(color);
            judge->pattern = color;
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer *) const{
        JudgeStruct judge;
        judge.reason = objectName();
        judge.play_animation = false;
        judge.who = target;

        room->judge(judge);
        if (!target->isAlive()) return false;
        QString color = judge.pattern;
        QList<ServerPlayer *> to_choose;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (target->distanceTo(p) == 1)
                to_choose << p;
        }
        if (to_choose.isEmpty())
            return false;

        ServerPlayer *victim = room->askForPlayerChosen(target, to_choose, objectName());
        QString pattern = QString(".|%1|.|hand$0").arg(color);

        room->setPlayerFlag(victim, "IkQianshaTarget");
        room->addPlayerMark(victim, QString("@qiansha_%1").arg(color));
        room->setPlayerCardLimitation(victim, "use,response", pattern, false);

        LogMessage log;
        log.type = "#IkQiansha";
        log.from = victim;
        log.arg = QString("no_suit_%1").arg(color);
        room->sendLog(log);

        return false;
    }
};

class IkQianshaClear: public TriggerSkill {
public:
    IkQianshaClear(): TriggerSkill("#ikqiansha-clear") {
        events << EventPhaseChanging << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player->tag["ikqiansha"].toString().isNull()) {
            if (triggerEvent == EventPhaseChanging) {
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.to != Player::NotActive)
                    return QStringList();
            } else if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return QStringList();
            }

            QString color = player->tag["ikqiansha"].toString();
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasFlag("IkQianshaTarget")) {
                    room->removePlayerCardLimitation(p, "use,response", QString(".|%1|.|hand$0").arg(color));
                    room->setPlayerMark(p, QString("@qiansha_%1").arg(color), 0);
                }
            }
        }
        return QStringList();
    }
};

class IkLichiViewAsSkill: public ViewAsSkill {
public:
    IkLichiViewAsSkill(): ViewAsSkill("iklichi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE) return false;
        return pattern == "slash";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        Slash *slash = new Slash(Card::SuitToBeDecided, 0);
        slash->setSkillName(objectName());
        slash->addSubcards(cards);

        return slash;
    }
};

class IkLichi: public TriggerSkill {
public:
    IkLichi(): TriggerSkill("iklichi") {
        events << DamageComplete << EventPhaseChanging;
        view_as_skill = new IkLichiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == DamageComplete) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!TriggerSkill::triggerable(damage.from)) return QStringList();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()
                && damage.from->getPhase() == Player::Play) {
                QStringList skill;
                if (!damage.from->hasSkill("ikchilian"))
                    skill << "ikchilian";
                if (!damage.from->hasSkill("iklipao"))
                    skill << "iklipao";
                if (!skill.isEmpty())
                    room->handleAcquireDetachSkills(damage.from, skill);
                damage.from->setFlags(objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->hasFlag(objectName()))
                room->handleAcquireDetachSkills(player, "-ikchilian|-iklipao", true);
        }

        return QStringList();
    }
};

class IkXuanren: public OneCardViewAsSkill {
public:
    IkXuanren(): OneCardViewAsSkill("ikxuanren") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const Card *to_select) const{
        if (to_select->getTypeId() != Card::TypeEquip)
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(to_select->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard);
        slash->setSkillName(objectName());
        return slash;
    }
};

class IkXuanrenTargetMod: public TargetModSkill {
public:
    IkXuanrenTargetMod(): TargetModSkill("#ikxuanren-target") {
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const{
        if (card->getSkillName() == "ikxuanren")
            return 1000;
        else
            return 0;
    }
};

class IkLanjian: public TriggerSkill {
public:
    IkLanjian(): TriggerSkill("iklanjian") {
        events << SlashMissed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        const Card *jink = effect.jink;
        if (!jink) return QStringList();
        QList<int> ids;
        if (!jink->isVirtualCard()) {
            if (room->getCardPlace(jink->getEffectiveId()) == Player::DiscardPile)
                ids << jink->getEffectiveId();
        } else {
            foreach (int id, jink->getSubcards()) {
                if (room->getCardPlace(id) == Player::DiscardPile)
                    ids << id;
            }
        }
        if (ids.isEmpty()) return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        const Card *jink = effect.jink;
        QList<int> ids;
        if (!jink->isVirtualCard()) {
            if (room->getCardPlace(jink->getEffectiveId()) == Player::DiscardPile)
                ids << jink->getEffectiveId();
        } else {
            foreach (int id, jink->getSubcards()) {
                if (room->getCardPlace(id) == Player::DiscardPile)
                    ids << id;
            }
        }
        room->fillAG(ids, player);
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(effect.to), objectName(),
                                                        "iklanjian-invoke:" + effect.to->objectName(), true, true);
        room->clearAG(player);
        if (!target) return false;
        room->broadcastSkillInvoke(objectName());
        player->tag["IkLanjianTarget"] = QVariant::fromValue(target);
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = player->tag["IkLanjianTarget"].value<ServerPlayer *>();
        player->tag.remove("IkLanjianTarget");
        if (!target) return false;
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        const Card *jink = effect.jink;
        QList<int> ids;
        if (!jink->isVirtualCard()) {
            if (room->getCardPlace(jink->getEffectiveId()) == Player::DiscardPile)
                ids << jink->getEffectiveId();
        } else {
            foreach (int id, jink->getSubcards()) {
                if (room->getCardPlace(id) == Player::DiscardPile)
                    ids << id;
            }
        }
        DummyCard *dummy = new DummyCard(ids);
        room->obtainCard(target, dummy);
        delete dummy;

        if (player->isAlive() && effect.to->isAlive() && target != player) {
            if (!player->canSlash(effect.to, NULL, false))
                return false;
            if (room->askForUseSlashTo(player, effect.to, QString("iklanjian-slash:%1").arg(effect.to->objectName()), false, true))
                return true;
        }
        return false;
    }
};

class IkQiangshi: public TriggerSkill {
public:
    IkQiangshi(): TriggerSkill("ikqiangshi") {
        events << EventPhaseStart << CardUsed << CardResponded;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->isKongcheng())
                        return QStringList(objectName());
                }
            } else
                player->setMark(objectName(), 0);
        } else if (player->getMark(objectName()) > 0 && (triggerEvent == CardUsed || CardUsed == CardResponded)) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed) {
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (card && int(card->getTypeId()) == player->getMark(objectName()))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == EventPhaseStart) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isKongcheng())
                    targets << p;
            }
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "ikqiangshi-invoke", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["IkQiangshiTarget"] = QVariant::fromValue(target);
                return true;
            }
        } else {
            if (player->askForSkillInvoke(objectName(), data)) {
                if (!player->hasSkill(objectName())) {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);
                }
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *target = player->tag["IkQiangshiTarget"].value<ServerPlayer *>();
            player->tag.remove("IkQiangshiTarget");
            if (target) {
                int id = room->askForCardChosen(player, target, "h", objectName());
                room->showCard(target, id);
                player->setMark(objectName(), int(Sanguosha->getCard(id)->getTypeId()));
            }
        } else {
            player->drawCards(1, objectName());
        }
        return false;
    }
};

class IkFengxin: public TriggerSkill {
public:
    IkFengxin(): TriggerSkill("ikfengxin") {
        events << EventPhaseStart << EventPhaseEnd << Death;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                if (p != player)
                    skill_list.insert(p, QStringList(objectName()));
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.damage && death.damage->from && death.damage->from->getPhase() == Player::Play)
                death.damage->from->addMark("IkFengxinKill");
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play) {
            QList<ServerPlayer *> zhangsongs;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("IkFengxinInvoked")) {
                    p->setFlags("-IkFengxinInvoked");
                    zhangsongs << p;
                }
            }
            if (player->getMark("IkFengxinKill") > 0) {
                player->setMark("IkFengxinKill", 0);
                return skill_list;
            }
            foreach (ServerPlayer *zs, zhangsongs) {
                LogMessage log;
                log.type = "#IkFengxin";
                log.from = player;
                log.to << zs;
                log.arg = objectName();
                room->sendLog(log);

                if (zs->getCardCount(false) < 2 || !room->askForDiscard(zs, objectName(), 2, 2, true, false, "ikfengxin-discard"))
                    room->loseHp(zs);
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *p) const{
        if (p->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *p) const{
        p->setFlags("IkFengxinInvoked");
        p->drawCards(2, objectName());
        if (p->isAlive() && player->isAlive()) {
            if (!p->isNude()) {
                int num = qMin(2, p->getCardCount(true));
                const Card *to_give = room->askForExchange(p, objectName(), num, num, true,
                                                           QString("@ikfengxin-give::%1:%2").arg(player->objectName()).arg(num));
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, p->objectName(),
                                      player->objectName(), objectName(), QString());
                room->obtainCard(player, to_give, reason, false);
                delete to_give;
            }
        }
        return false;
    }
};

class IkShensha: public TriggerSkill {
public:
    IkShensha(): TriggerSkill("ikshensha") {
        events << EventPhaseChanging << CardFinished << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(player, "@shensha", 0);
                room->setPlayerMark(player, "ikshensha", 0);
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill
                && player->isAlive() && player->getPhase() != Player::NotActive)
                return QStringList(objectName());
        } else if (triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill) {
            QString name = data.toString();
            if (name != objectName()) return QStringList();
            int num = (triggerEvent == EventAcquireSkill) ? player->getMark("ikshensha") : 0;
            room->setPlayerMark(player, "@shensha", num);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->addPlayerMark(player, "ikshensha");
        if (TriggerSkill::triggerable(player))
            room->setPlayerMark(player, "@shensha", player->getMark("ikshensha"));
        return false;
    }
};

// the part of Armor ignorance is coupled in Player::hasArmorEffect

class IkShenshaTargetMod: public TargetModSkill {
public:
    IkShenshaTargetMod(): TargetModSkill("#ikshensha-target") {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const{
        if (from->hasSkill("ikshensha") && isAllAdjacent(from, card))
            return 1;
        else
            return 0;
    }

private:
    bool isAllAdjacent(const Player *from, const Card *card) const{
        int rangefix = 0;
        if (card->isVirtualCard() && from->getOffensiveHorse()
            && card->getSubcards().contains(from->getOffensiveHorse()->getEffectiveId()))
            rangefix = 1;
        foreach (const Player *p, from->getAliveSiblings()) {
            if (from->distanceTo(p, rangefix) != 1)
                return false;
        }
        return true;
    }
};

class IkShenshaDistance: public DistanceSkill {
public:
    IkShenshaDistance(): DistanceSkill("#ikshensha-dist") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill("ikshensha") && from->getPhase() != Player::NotActive)
            return -from->getMark("ikshensha");
        return 0;
    }
};

class IkShihua: public TriggerSkill {
public:
    IkShihua(): TriggerSkill("ikshihua") {
        events << BeforeCardsMove;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *caozhi, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(caozhi)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == caozhi || move.from == NULL)
            return QStringList();
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                ||move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE)) {
            int i = 0;
            foreach (int card_id, move.card_ids) {
                if (Sanguosha->getCard(card_id)->getSuit() == Card::Club
                    && ((move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE
                         && move.from_places[i] == Player::PlaceJudge
                         && move.to_place == Player::DiscardPile)
                        || (move.reason.m_reason != CardMoveReason::S_REASON_JUDGEDONE
                            && room->getCardOwner(card_id) == move.from
                            && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))))
                    return QStringList(objectName());
                i++;
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caozhi, QVariant &, ServerPlayer *) const{
        if (caozhi->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *caozhi, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> card_ids;
        int i = 0;
        foreach (int card_id, move.card_ids) {
            if (Sanguosha->getCard(card_id)->getSuit() == Card::Club
                && ((move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE
                     && move.from_places[i] == Player::PlaceJudge
                     && move.to_place == Player::DiscardPile)
                    || (move.reason.m_reason != CardMoveReason::S_REASON_JUDGEDONE
                        && room->getCardOwner(card_id) == move.from
                        && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))))
                card_ids << card_id;
            i++;
        }

        int ai_delay = Config.AIDelay;
        Config.AIDelay = 0;
        while (card_ids.length() > 1) {
            room->fillAG(card_ids, caozhi);
            int id = room->askForAG(caozhi, card_ids, true, objectName());
            if (id == -1) {
                room->clearAG(caozhi);
                break;
            }
            card_ids.removeOne(id);
            room->clearAG(caozhi);
        }
        Config.AIDelay = ai_delay;

        if (!card_ids.isEmpty()) {
            move.removeCardIds(card_ids);
            data = QVariant::fromValue(move);
            DummyCard *dummy = new DummyCard(card_ids);
            room->moveCardTo(dummy, caozhi, Player::PlaceHand, move.reason, true);
            delete dummy;
        }

        return false;
    }
};

IkJiushiCard::IkJiushiCard() {
    target_fixed = true;
}

const Card *IkJiushiCard::validate(CardUseStruct &card_use) const{
    card_use.from->turnOver();
    Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
    analeptic->setSkillName("ikjiushi");
    return analeptic;
}

const Card *IkJiushiCard::validateInResponse(ServerPlayer *player) const{
    player->turnOver();
    Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
    analeptic->setSkillName("ikjiushi");
    return analeptic;
}

class IkJiushiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkJiushiViewAsSkill(): ZeroCardViewAsSkill("ikjiushi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player) && player->faceUp();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.contains("analeptic") && player->faceUp();
    }

    virtual const Card *viewAs() const{
        return new IkJiushiCard;
    }
};

class IkJiushi: public TriggerSkill {
public:
    IkJiushi(): TriggerSkill("ikjiushi") {
        events << PreDamageDone << DamageComplete;
        view_as_skill = new IkJiushiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == PreDamageDone) {
            player->tag["PredamagedFace"] = !player->faceUp();
        } else if (triggerEvent == DamageComplete && TriggerSkill::triggerable(player)) {
            if (player->tag.value("PredamagedFace").toBool() && !player->faceUp())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caozhi, QVariant &, ServerPlayer *) const{
        if (caozhi->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *caozhi, QVariant &, ServerPlayer *) const{
        caozhi->turnOver();
        return false;
    }
};

class IkZhuyan: public TriggerSkill {
public:
    IkZhuyan(): TriggerSkill("ikzhuyan") {
        events << SlashEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && player->getArmor() == NULL) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->isBlack() && effect.nature == DamageStruct::Normal)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());

        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        LogMessage log;
        log.type = "#SkillNullify";
        log.from = player;
        log.arg = objectName();
        log.arg2 = effect.slash->objectName();
        room->sendLog(log);

        return true;
    }
};

class IkPiaohu: public TriggerSkill {
public:
    IkPiaohu(): TriggerSkill("ikpiaohu") {
        events << TargetConfirming;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isKindOf("Slash") && use.to.contains(player))
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player || owner == use.from || (owner == room->getCurrent() && owner->getPhase() != Player::NotActive)) continue;
                if (use.from->canSlash(owner, use.card, false) && owner->inMyAttackRange(player))
                    if (owner->canDiscard(owner, "he") || (!owner->isChained() && !player->isChained()))
                        skill_list.insert(owner, QStringList(objectName()));
            }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (ask_who->canDiscard(ask_who, "he") && room->askForCard(ask_who, "Armor", "@ikpiaohu:" + player->objectName(), data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        } else if (!player->isChained() && !ask_who->isChained() && ask_who->askForSkillInvoke(objectName(), "chain:" + player->objectName())) {
            room->broadcastSkillInvoke(objectName());
            
            QList<ServerPlayer *> p_list;
            p_list << player << ask_who;
            room->sortByActionOrder(p_list);
            foreach (ServerPlayer *p, p_list)
                if (!p->isChained()) {
                    p->setChained(true);
                    room->broadcastProperty(p, "chained");
                    room->setEmotion(p, "chain");
                    room->getThread()->trigger(ChainStateChanged, room, p);
                }
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        CardUseStruct use = data.value<CardUseStruct>();
        use.to.removeOne(player);
        use.to.append(ask_who);
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};

class IkXuwu: public TriggerSkill {
public:
    IkXuwu(): TriggerSkill("ikxuwu") {
        frequency = Compulsory;
        events << Predamage;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = zhangchunhua;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(zhangchunhua, objectName());
        room->broadcastSkillInvoke(objectName());
        room->loseHp(damage.to, damage.damage);

        return true;
    }
};

IkNvelian::IkNvelian(): TriggerSkill("iknvelian") {
    events << HpChanged << MaxHpChanged << CardsMoveOneTime;
    frequency = Frequent;
}

int IkNvelian::getMaxLostHp(ServerPlayer *zhangchunhua) const{
    return qMin(2, zhangchunhua->getLostHp());
}

QStringList IkNvelian::triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data, ServerPlayer* &) const{
    if (!TriggerSkill::triggerable(zhangchunhua)) return QStringList();
    int losthp = getMaxLostHp(zhangchunhua);
    if (triggerEvent == CardsMoveOneTime) {
        bool can_invoke = false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == zhangchunhua && move.from_places.contains(Player::PlaceHand))
            can_invoke = true;
        if (move.to == zhangchunhua && move.to_place == Player::PlaceHand)
            can_invoke = true;
        if (!can_invoke)
            return QStringList();
    }

    if (zhangchunhua->getHandcardNum() < losthp)
        return QStringList(objectName());
    return QStringList();
}

bool IkNvelian::cost(TriggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &, ServerPlayer *) const{
    if (zhangchunhua->askForSkillInvoke(objectName())) {
        room->broadcastSkillInvoke(objectName());
        return true;
    }
    return false;
}

bool IkNvelian::effect(TriggerEvent, Room *, ServerPlayer *zhangchunhua, QVariant &, ServerPlayer *) const{
    int losthp = getMaxLostHp(zhangchunhua);
    zhangchunhua->drawCards(losthp - zhangchunhua->getHandcardNum(), objectName());

    return false;
}

class IkBengshang: public MasochismSkill {
public:
    IkBengshang(): MasochismSkill("ikbengshang") {
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();

        QStringList skill;
        for (int i = 0; i < damage.damage; i++)
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const{
        Room *room = zhonghui->getRoom();
        room->drawCards(zhonghui, 1, objectName());
        if (!zhonghui->isKongcheng()) {
            int card_id;
            if (zhonghui->getHandcardNum() == 1) {
                room->getThread()->delay();
                card_id = zhonghui->handCards().first();
            } else {
                const Card *card = room->askForExchange(zhonghui, "ikbengshang", 1, 1, false, "IkBengshangPush");
                card_id = card->getEffectiveId();
                delete card;
            }
            zhonghui->addToPile("ikbengshangpile", card_id);
        }
    }
};

class IkBengshangKeep: public MaxCardsSkill {
public:
    IkBengshangKeep(): MaxCardsSkill("#ikbengshang") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill("ikbengshang"))
            return target->getPile("ikbengshangpile").length();
        else
            return 0;
    }
};

class IkAnhun: public PhaseChangeSkill {
public:
    IkAnhun(): PhaseChangeSkill("ikanhun") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("@anhun") == 0
               && target->getPile("ikbengshangpile").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#IkAnhunWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("ikbengshangpile").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(zhonghui, "@anhun", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui)) {
            if (zhonghui->isWounded() && room->askForChoice(zhonghui, objectName(), "recover+draw") == "recover")
                room->recover(zhonghui, RecoverStruct(zhonghui));
            else
                room->drawCards(zhonghui, 2, objectName());
            room->acquireSkill(zhonghui, "ikzhuyi");
        }

        return false;
    }
};

IkZhuyiCard::IkZhuyiCard() {
}

bool IkZhuyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void IkZhuyiCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhonghui = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhonghui->getRoom();
    room->drawCards(target, 2, "ikzhuyi");
    if (target->getHandcardNum() > zhonghui->getHandcardNum())
        room->damage(DamageStruct("ikzhuyi", zhonghui, target));
}

class IkZhuyi: public OneCardViewAsSkill {
public:
    IkZhuyi(): OneCardViewAsSkill("ikzhuyi") {
        filter_pattern = ".|.|.|ikbengshangpile";
        expand_pile = "ikbengshangpile";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("ikbengshangpile").isEmpty() && !player->hasUsed("IkZhuyiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkZhuyiCard *card = new IkZhuyiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

IkMiceCard::IkMiceCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkMiceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *card = Self->tag.value("ikmice").value<const Card *>();
    Card *mutable_card = const_cast<Card *>(card);
    if (mutable_card)
        mutable_card->addSubcards(this->subcards);
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool IkMiceCard::targetFixed() const{
    const Card *card = Self->tag.value("ikmice").value<const Card *>();
    Card *mutable_card = const_cast<Card *>(card);
    if (mutable_card)
        mutable_card->addSubcards(this->subcards);
    return mutable_card && mutable_card->targetFixed();
}

bool IkMiceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    const Card *card = Self->tag.value("ikmice").value<const Card *>();
    Card *mutable_card = const_cast<Card *>(card);
    if (mutable_card)
        mutable_card->addSubcards(this->subcards);
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *IkMiceCard::validate(CardUseStruct &card_use) const{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("ikmice");
    use_card->addSubcards(this->subcards);
    bool available = true;
    foreach (ServerPlayer *to, card_use.to)
        if (card_use.from->isProhibited(to, use_card)) {
            available = false;
            break;
        }
    available = available && use_card->isAvailable(card_use.from);
    use_card->deleteLater();
    if (!available) return NULL;
    return use_card;
}

#include "touhou-hana.h"
class IkMice: public ViewAsSkill {
public:
    IkMice(): ViewAsSkill("ikmice") {
    }

    virtual QDialog *getDialog() const{
        return ThMimengDialog::getInstance("ikmice", false);
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() < Self->getHandcardNum())
            return NULL;

        const Card *c = Self->tag.value("ikmice").value<const Card *>();
        if (c) {
            IkMiceCard *card = new IkMiceCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->isKongcheng())
            return false;
        else
            return !player->hasUsed("IkMiceCard");
    }
};

class IkZhiyu: public MasochismSkill {
public:
    IkZhiyu(): MasochismSkill("ikzhiyu") {
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        target->drawCards(1, objectName());

        Room *room = target->getRoom();

        if (target->isKongcheng())
            return;
        room->showAllCards(target);

        QList<const Card *> cards = target->getHandcards();
        Card::Color color = cards.first()->getColor();
        bool same_color = true;
        foreach (const Card *card, cards) {
            if (card->getColor() != color) {
                same_color = false;
                break;
            }
        }

        if (same_color && damage.from && damage.from->canDiscard(damage.from, "h"))
            room->askForDiscard(damage.from, objectName(), 1, 1);
    }
};

class IkGuanchong: public DrawCardsSkill {
public:
    IkGuanchong(): DrawCardsSkill("ikguanchong") {
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual int getDrawNum(ServerPlayer *caozhang, int n) const{
        Room *room = caozhang->getRoom();
        QString choice = room->askForChoice(caozhang, objectName(), "guan+chong");
        LogMessage log;
        log.from = caozhang;
        log.arg = objectName();
        if (choice == "guan") {
            log.type = "#IkGuanchong1";
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName(), 1);
            room->setPlayerCardLimitation(caozhang, "use,response", "Slash", true);
            return n + 1;
        } else {
            log.type = "#IkGuanchong2";
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName(), 2);
            room->setPlayerFlag(caozhang, "IkGuanchongInvoke");
            return n - 1;
        }
    }
};

class IkGuanchongTargetMod: public TargetModSkill {
public:
    IkGuanchongTargetMod(): TargetModSkill("#ikguanchong-target") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("IkGuanchongInvoke"))
            return 1;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasFlag("IkGuanchongInvoke"))
            return 1000;
        else
            return 0;
    }
};

class IkLundao: public TriggerSkill {
public:
    IkLundao(): TriggerSkill("iklundao") {
        events << AskForRetrial;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        room->retrial(Sanguosha->getCard(room->drawCard()), player, judge, objectName());
        return false;
    }
};

class IkXuanwu: public PhaseChangeSkill {
public:
    IkXuanwu(): PhaseChangeSkill("ikxuanwu") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *wangyi) const{
        return PhaseChangeSkill::triggerable(wangyi)
            && wangyi->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *wangyi) const{
        Room *room = wangyi->getRoom();
        JudgeStruct judge;
        judge.pattern = ".|black";
        judge.good = true;
        judge.reason = objectName();
        judge.who = wangyi;

        room->judge(judge);

        if (judge.isGood() && wangyi->isAlive()) {
            QList<int> pile_ids = room->getNCards(wangyi->getLostHp() + 1, false);
            room->fillAG(pile_ids, wangyi);
            ServerPlayer *target = room->askForPlayerChosen(wangyi, room->getAllPlayers(), objectName());
            room->clearAG(wangyi);

            DummyCard *dummy = new DummyCard(pile_ids);
            wangyi->setFlags("Global_GongxinOperator");
            target->obtainCard(dummy, false);
            wangyi->setFlags("-Global_GongxinOperator");
            delete dummy;
        }

        return false;
    }
};

IkXingshi::IkXingshi(): MasochismSkill("ikxingshi") {
    frequency = Frequent;
    total_point = 13;
}

bool IkXingshi::cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
    if (player->askForSkillInvoke(objectName())) {
        room->broadcastSkillInvoke(objectName());
        return true;
    }
    return false;
}

void IkXingshi::onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
    Room *room = target->getRoom();

    QList<int> card_ids = room->getNCards(4);
    room->fillAG(card_ids);

    QList<int> to_get, to_throw;
    while (true) {
        int sum = 0;
        foreach (int id, to_get)
            sum += Sanguosha->getCard(id)->getNumber();
        foreach (int id, card_ids) {
            if (sum + Sanguosha->getCard(id)->getNumber() > total_point) {
                room->takeAG(NULL, id, false);
                card_ids.removeOne(id);
                to_throw << id;
            }
        }
        if (card_ids.isEmpty()) break;

        int card_id = room->askForAG(target, card_ids, card_ids.length() < 4, objectName());
        if (card_id == -1) break;
        card_ids.removeOne(card_id);
        to_get << card_id;
        room->takeAG(target, card_id, false);
        if (card_ids.isEmpty()) break;
    }
    DummyCard *dummy = new DummyCard;
    if (!to_get.isEmpty()) {
        dummy->addSubcards(to_get);
        target->obtainCard(dummy);
    }
    dummy->clearSubcards();
    if (!to_throw.isEmpty() || !card_ids.isEmpty()) {
        dummy->addSubcards(to_throw + card_ids);
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, target->objectName(), objectName(), QString());
        room->throwCard(dummy, reason, NULL);
    }
    delete dummy;

    room->clearAG();
}

class IkShouyan: public TriggerSkill {
public:
    IkShouyan(): TriggerSkill("ikshouyan") {
        events << DamageInflicted;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        if (player->getHp() == 1) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p == player) continue;
                if (p->canDiscard(p, "he"))
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *p) const{
        if (room->askForCard(p, ".Equip", "@ikshouyan-card:" + player->objectName(), data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *p) const{
        p->turnOver();
        LogMessage log;
        log.type = "#IkShouyan";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        return true;
    }
};

class IkJingce: public TriggerSkill {
public:
    IkJingce(): TriggerSkill("ikjingce") {
        events << EventPhaseEnd;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Play
            && player->getMark(objectName()) >= player->getHp();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (room->askForSkillInvoke(player, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(2, objectName());
        return false;
    }
};

class IkJingceRecord: public TriggerSkill {
public:
    IkJingceRecord(): TriggerSkill("#ikjingce-record") {
        events << PreCardUsed << CardResponded << EventPhaseStart;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if ((triggerEvent == PreCardUsed || triggerEvent == CardResponded) && player->getPhase() <= Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                   card = response.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            player->setMark("ikjingce", 0);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->addMark("ikjingce");
        return false;
    }
};

IkBingyanCard::IkBingyanCard() {
}

void IkBingyanCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    if (!target->isAlive()) return;

    QString type_name[4] = { QString(), "BasicCard", "TrickCard", "EquipCard" };
    QStringList types;
    types << "BasicCard" << "TrickCard" << "EquipCard";
    foreach (int id, subcards) {
        const Card *c = Sanguosha->getCard(id);
        types.removeOne(type_name[c->getTypeId()]);
        if (types.isEmpty()) break;
    }
    if (!target->canDiscard(target, "h") || types.isEmpty()
        || !room->askForCard(target, types.join(",") + "|.|.|hand", "@ikbingyan-discard")) {
        target->turnOver();
        target->drawCards(subcards.length(), "ikbingyan");
    }
}

class IkBingyan: public ViewAsSkill {
public:
    IkBingyan(): ViewAsSkill("ikbingyan") {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        IkBingyanCard *card = new IkBingyanCard;
        card->addSubcards(cards);
        card->setSkillName(objectName());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "h") && !player->hasUsed("IkBingyanCard");
    }
};

class IkXuelian: public MasochismSkill {
public:
    IkXuelian(): MasochismSkill("ikxuelian") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return MasochismSkill::triggerable(target)
            && !target->isKongcheng();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        const Card *card = room->askForCard(player, ".", "@ikxuelian-show", data, Card::MethodNone);
        if (card) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            LogMessage log;
            log.from = player;
            log.type = "#InvokeSkill";
            log.arg = objectName();
            room->sendLog(log);

            player->tag["IkXuelianCard"] = QVariant::fromValue(card);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        const Card *card = target->tag["IkXuelianCard"].value<const Card *>();
        target->tag.remove("IkXuelianCard");
        if (card) {
            room->showCard(target, card->getEffectiveId());
            if (!damage.from || damage.from->isDead()) return;

            QString type_name[4] = { QString(), "BasicCard", "TrickCard", "EquipCard" };
            QStringList types;
            types << "BasicCard" << "TrickCard" << "EquipCard";
            types.removeOne(type_name[card->getTypeId()]);
            if (!damage.from->canDiscard(damage.from, "h")
                || !room->askForCard(damage.from, types.join(",") + "|.|.|hand",
                                     QString("@ikxuelian-discard:%1::%2:%3")
                                             .arg(target->objectName())
                                             .arg(types.first()).arg(types.last()),
                                     QVariant())) {
                room->recover(target, RecoverStruct(target));
            }
        }
    }
};

class IkQingguo: public TriggerSkill {
public:
    IkQingguo(): TriggerSkill("ikqingguo") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(target) && triggerEvent == EventPhaseStart
            && target->getPhase() == Player::Finish && target->isWounded())
            return QStringList(objectName());
        else if (triggerEvent == ChoiceMade) {
            QString str = data.toString();
            if (str.startsWith("Yiji:" + objectName()) && target->hasFlag("IkQingguoUse")) {
                target->setFlags("-IkQingguoUse");
                target->addMark(objectName(), str.split(":").last().split("+").length());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (room->askForSkillInvoke(player, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer *) const{
        QStringList draw_num;
        for (int i = 1; i <= target->getLostHp(); draw_num << QString::number(i++)) {}
        int num = room->askForChoice(target, "ikqingguo_draw", draw_num.join("+")).toInt();
        target->drawCards(num, objectName());
        target->setMark(objectName(), 0);
        if (!target->isKongcheng()) {
            forever {
                int n = target->getMark(objectName());
                if (n < num && !target->isKongcheng()) {
                    QList<int> handcards = target->handCards();
                    target->setFlags("IkQingguoUse");
                    if (!room->askForYiji(target, handcards, objectName(), false, false, false, num - n)) {
                        target->setFlags("-IkQingguoUse");
                        break;
                    }
                } else {
                    break;
                }
            }
            // give the rest cards randomly
            if (target->getMark(objectName()) < num && !target->isKongcheng()) {
                int rest_num = num - target->getMark(objectName());
                forever {
                    QList<int> handcard_list = target->handCards();
                    qShuffle(handcard_list);
                    int give = qrand() % rest_num + 1;
                    rest_num -= give;
                    QList<int> to_give = handcard_list.length() < give ? handcard_list : handcard_list.mid(0, give);
                    ServerPlayer *receiver = room->getOtherPlayers(target).at(qrand() % (target->aliveCount() - 1));
                    DummyCard *dummy = new DummyCard(to_give);
                    room->obtainCard(receiver, dummy, false);
                    delete dummy;
                    if (rest_num == 0 || target->isKongcheng())
                        break;
                }
            }
        }
        return false;
    }
};

class IkJingshi: public TriggerSkill {
public:
    IkJingshi(): TriggerSkill("ikjingshi") {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.contains(player) && use.from != player) {
                if (use.card->isKindOf("Slash") || use.card->isNDTrick())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->loseHp(player);
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << player->objectName();
        data = QVariant::fromValue(use);
        if (player->isAlive()) {
            if (player->canDiscard(use.from, "he")) {
                int id = room->askForCardChosen(player, use.from, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(id, use.from, player);
            }
        }
        return false;
    }
};

IkaiKinPackage::IkaiKinPackage()
    :Package("ikai-kin")
{
    General *wind016 = new General(this, "wind016", "kaze", 3);
    wind016->addSkill(new IkHuowen);
    wind016->addSkill(new FakeMoveSkill("ikhuowen"));
    related_skills.insertMulti("ikhuowen", "#ikhuowen-fake-move");
    wind016->addSkill(new IkEnyuan);

    General *wind017 = new General(this, "wind017", "kaze", 3);
    wind017->addSkill(new IkXinchao);
    wind017->addSkill(new IkShangshi);

    General *wind018 = new General(this, "wind018", "kaze", 3);
    wind018->addSkill(new IkMitu);
    wind018->addSkill(new IkSishi);

    General *wind019 = new General(this, "wind019", "kaze");
    wind019->addSkill("thxiagong");
    wind019->addSkill(new IkWanhun);

    General *wind020 = new General(this, "wind020", "kaze");
    wind020->addSkill(new IkMeiying);
    wind020->addSkill(new IkFansheng);

    General *wind021 = new General(this, "wind021", "kaze");
    wind021->addSkill(new IkLiyao);
    wind021->addSkill(new IkLiyaoTargetMod);
    related_skills.insertMulti("ikliyao", "#ikliyao-target");

    General *wind026 = new General(this, "wind026", "kaze");
    wind026->addSkill(new IkXianyu);
    wind026->addSkill(new IkXianyuAttach);
    related_skills.insertMulti("ikxianyu", "#ikxianyu-attach");

    General *wind027 = new General(this, "wind027", "kaze", 3);
    wind027->addSkill(new IkQizhi);
    wind027->addSkill(new IkQizhiUse);
    wind027->addSkill(new IkQizhiTargetMod);
    related_skills.insertMulti("ikqizhi", "#ikqizhi-use");
    related_skills.insertMulti("ikqizhi", "#ikqizhi-target");
    wind027->addSkill(new IkZongshi);

    General *wind028 = new General(this, "wind028", "kaze");
    wind028->addSkill(new IkYaolun);

    General *wind031 = new General(this, "wind031", "kaze");
    wind031->addSkill("thjibu");
    wind031->addSkill(new IkQiansha);
    wind031->addSkill(new IkQianshaClear);
    related_skills.insertMulti("ikqiansha", "#ikqiansha-clear");

    General *wind032 = new General(this, "wind032", "kaze");
    wind032->addSkill(new IkLichi);

    General *wind037 = new General(this, "wind037", "kaze");
    wind037->addSkill(new IkXuanren);
    wind037->addSkill(new IkXuanrenTargetMod);
    related_skills.insertMulti("ikxuanren", "#ikxuanren-target");
    wind037->addSkill(new IkLanjian);

    General *wind038 = new General(this, "wind038", "kaze", 3);
    wind038->addSkill(new IkQiangshi);
    wind038->addSkill(new IkFengxin);

    General *wind039 = new General(this, "wind039", "kaze");
    wind039->addSkill(new IkShensha);
    wind039->addSkill(new IkShenshaTargetMod);
    wind039->addSkill(new IkShenshaDistance);
    related_skills.insertMulti("ikshensha", "#ikshensha-target");
    related_skills.insertMulti("ikshensha", "#ikshensha-dist");

    General *bloom016 = new General(this, "bloom016", "hana", 3);
    bloom016->addSkill(new IkShihua);
    bloom016->addSkill(new IkJiushi);

    General *bloom017 = new General(this, "bloom017", "hana");
    bloom017->addSkill(new IkZhuyan);
    bloom017->addSkill(new IkPiaohu);

    General *bloom018 = new General(this, "bloom018", "hana", 3, false);
    bloom018->addSkill(new IkXuwu);
    bloom018->addSkill(new IkNvelian);

    General *bloom019 = new General(this, "bloom019", "hana");
    bloom019->addSkill(new IkBengshang);
    bloom019->addSkill(new IkBengshangKeep);
    bloom019->addSkill(new IkAnhun);
    bloom019->addRelateSkill("ikzhuyi");
    related_skills.insertMulti("ikbengshang", "#ikbengshang");

    General *bloom020 = new General(this, "bloom020", "hana", 3);
    bloom020->addSkill(new IkMice);
    bloom020->addSkill(new IkZhiyu);

    General *bloom021 = new General(this, "bloom021", "hana");
    bloom021->addSkill(new IkGuanchong);
    bloom021->addSkill(new IkGuanchongTargetMod);
    related_skills.insertMulti("ikguanchong", "#ikguanchong-target");

    General *bloom022 = new General(this, "bloom022", "hana", 3, false);
    bloom022->addSkill(new IkLundao);
    bloom022->addSkill(new IkXuanwu);

    General *bloom025 = new General(this, "bloom025", "hana", 3);
    bloom025->addSkill(new IkXingshi);
    bloom025->addSkill(new IkShouyan);

    General *bloom026 = new General(this, "bloom026", "hana");
    bloom026->addSkill(new IkJingce);
    bloom026->addSkill(new IkJingceRecord);
    related_skills.insertMulti("ikjingce", "#ikjingce-record");

    General *bloom027 = new General(this, "bloom027", "hana", 3);
    bloom027->addSkill(new IkBingyan);
    bloom027->addSkill(new IkXuelian);

    General *bloom031 = new General(this, "bloom031", "hana", 3, false);
    bloom031->addSkill(new IkQingguo);
    bloom031->addSkill(new IkJingshi);

    addMetaObject<IkXinchaoCard>();
    addMetaObject<IkSishiCard>();
    addMetaObject<ExtraCollateralCard>();
    addMetaObject<IkXianyuCard>();
    addMetaObject<IkXianyuSlashCard>();
    addMetaObject<IkQizhiCard>();
    addMetaObject<IkJiushiCard>();
    addMetaObject<IkZhuyiCard>();
    addMetaObject<IkMiceCard>();
    addMetaObject<IkBingyanCard>();

    skills << new IkXianyuSlashViewAsSkill << new IkZhuyi;
}

ADD_PACKAGE(IkaiKin)