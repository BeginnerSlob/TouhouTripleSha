#include "ikai-kin.h"

#include "general.h"
#include "skill.h"
#include "engine.h"

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

    General *bloom022 = new General(this, "bloom022", "hana", 3, false);
    bloom022->addSkill(new IkLundao);
    bloom022->addSkill(new IkXuanwu);

    addMetaObject<IkXinchaoCard>();
}

ADD_PACKAGE(IkaiKin)