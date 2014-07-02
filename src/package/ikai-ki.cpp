#include "ikai-ki.h"

#include "general.h"
#include "skill.h"
#include "maneuvering.h"
#include "engine.h"
#include "client.h"

class IkLiegong: public TriggerSkill {
public:
    IkLiegong(): TriggerSkill("ikliegong") {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (player != use.from || player->getPhase() != Player::Play || !use.card->isKindOf("Slash"))
            return QStringList();
        foreach (ServerPlayer *p, use.to) {
            int handcardnum = p->getHandcardNum();
            if (player->getHp() <= handcardnum || player->getAttackRange() >= handcardnum)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            int handcardnum = p->getHandcardNum();
            if ((player->getHp() <= handcardnum || player->getAttackRange() >= handcardnum)
                && player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.type = "#NoJink";
                log.from = p;
                room->sendLog(log);
                jink_list.replace(index, QVariant(0));
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class IkKuanggu: public TriggerSkill {
public:
    IkKuanggu(): TriggerSkill("ikkuanggu") {
        frequency = Compulsory;
        events << Damage << PreDamageDone;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *weiyan = damage.from;
            if (weiyan)
                weiyan->tag["InvokeIkKuanggu"] = (weiyan->distanceTo(damage.to) <= 1 && weiyan != player);
            return QStringList();
        }
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        bool invoke = player->tag.value("InvokeIkKuanggu", false).toBool();
        player->tag["InvokeIkKuanggu"] = false;
        if (invoke) {
            QStringList skills;
            for (int i = 0; i < damage.damage; i++)
                skills << objectName();
            return skills;
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        QStringList choices;
        if (player->isWounded())
            choices << "recover";
        choices << "draw";
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "recover")
            room->recover(player, RecoverStruct(player));
        else
            player->drawCards(2);
        return false;
    }
};

class IkFuyao: public OneCardViewAsSkill {
public:
    IkFuyao(): OneCardViewAsSkill("ikfuyao") {
        filter_pattern = ".|club";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }
};

class IkNiepan: public TriggerSkill {
public:
    IkNiepan(): TriggerSkill("ikniepan") {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@niepan";
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(pangtong) || pangtong->getMark("@niepan") == 0)
            return QStringList();
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != pangtong)
            return QStringList();
        if (pangtong->isDead() || pangtong->getHp() > 0)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data, ServerPlayer *) const{
        if (pangtong->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data, ServerPlayer *) const{
        room->removePlayerMark(pangtong, "@niepan");
        room->addPlayerMark(pangtong, "@niepanused");

        QList<const Card *> tricks = pangtong->getJudgingArea();
        foreach (const Card *trick, tricks) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, pangtong->objectName());
            room->throwCard(trick, reason, NULL);
        }

        if (!pangtong->faceUp())
            pangtong->turnOver();

        if (pangtong->isChained())
            room->setPlayerProperty(pangtong, "chained", false);

        pangtong->drawCards(3, objectName());

        room->recover(pangtong, RecoverStruct(pangtong, NULL, 3 - pangtong->getHp()));

        return false;
    }
};

class IkJingnie: public TriggerSkill {
public:
    IkJingnie(): TriggerSkill("ikjingnie") {
        events << GameStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == GameStart) {
            if (player) return QStringList();
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(Sanguosha->getSkill("eight_diagram"));
            room->getThread()->addTriggerSkill(trigger_skill);
        }
        return QStringList();
    }
};

class IkJianyan: public OneCardViewAsSkill {
public:
    IkJianyan(): OneCardViewAsSkill("ikjianyan") {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        return fire_attack;
    }
};

class IkXuanying: public OneCardViewAsSkill {
public:
    IkXuanying(): OneCardViewAsSkill("ikxuanying") {
        filter_pattern = ".|black|.|hand";
        response_pattern = "nullification";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        foreach (const Card *card, player->getHandcards()) {
            if (card->isBlack()) return true;
        }
        return false;
    }
};

IkTiaoxinCard::IkTiaoxinCard() {
}

bool IkTiaoxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->inMyAttackRange(Self);
}

void IkTiaoxinCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    bool use_slash = false;
    if (effect.to->canSlash(effect.from, NULL, false))
        use_slash = room->askForUseSlashTo(effect.to, effect.from, "@iktiaoxin-slash:" + effect.from->objectName());
    if (!use_slash && effect.from->canDiscard(effect.to, "he"))
        room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "iktiaoxin", false, Card::MethodDiscard), effect.to, effect.from);
}

class IkTiaoxin: public ZeroCardViewAsSkill {
public:
    IkTiaoxin(): ZeroCardViewAsSkill("iktiaoxin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkTiaoxinCard");
    }

    virtual const Card *viewAs() const{
        return new IkTiaoxinCard;
    }
};

class IkShengtian: public PhaseChangeSkill {
public:
    IkShengtian(): PhaseChangeSkill("ikshengtian") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("@shengtian") == 0
               && target->getPhase() == Player::Start
               && target->isKongcheng();
    }

    virtual bool onPhaseChange(ServerPlayer *jiangwei) const{
        Room *room = jiangwei->getRoom();
        room->notifySkillInvoked(jiangwei, objectName());

        LogMessage log;
        log.type = "#IkShengtianWake";
        log.from = jiangwei;
        log.arg = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(jiangwei, "@shengtian", 1);
        if (room->changeMaxHpForAwakenSkill(jiangwei)) {
            if (jiangwei->isWounded() && room->askForChoice(jiangwei, objectName(), "recover+draw") == "recover")
                room->recover(jiangwei, RecoverStruct(jiangwei));
            else
                room->drawCards(jiangwei, 2, objectName());
            room->handleAcquireDetachSkills(jiangwei, "ikxuanwu|ikmohua");
        }

        return false;
    }
};

class IkMohua: public FilterSkill {
public:
    IkMohua(): FilterSkill("ikmohua") {
    }

    static WrappedCard *changeToClub(int cardId) {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("ikmohua");
        new_card->setSuit(Card::Club);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->getSuit() == Card::Diamond;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        return changeToClub(originalCard->getEffectiveId());
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return -2;
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

class IkManbo: public TriggerSkill {
public:
    IkManbo(): TriggerSkill("ikmanbo") {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *liushan, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(liushan) && use.card->isKindOf("Slash"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(liushan, objectName());

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = liushan;
        log.arg = objectName();
        room->sendLog(log);

        if (!room->askForCard(use.from, ".Basic", "@ikmanbo-discard")) {
            use.nullified_list << liushan->objectName();
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

IkBaishenCard::IkBaishenCard() {
}

bool IkBaishenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void IkBaishenCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    ServerPlayer *liushan = effect.from, *player = effect.to;

    LogMessage log;
    log.type = "#IkBaishen";
    log.from = liushan;
    log.to << player;
    room->sendLog(log);

    room->setTag("IkBaishenTarget", QVariant::fromValue(player));
}

class IkBaishenViewAsSkill: public OneCardViewAsSkill {
public:
    IkBaishenViewAsSkill(): OneCardViewAsSkill("ikbaishen") {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@ikbaishen";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkBaishenCard *ikbaishen = new IkBaishenCard;
        ikbaishen->addSubcard(originalCard);
        return ikbaishen;
    }
};

class IkBaishen: public TriggerSkill {
public:
    IkBaishen(): TriggerSkill("ikbaishen") {
        events << EventPhaseChanging << EventPhaseStart;
        view_as_skill = new IkBaishenViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            switch (change.to) {
            case Player::Play: {
                    if (!TriggerSkill::triggerable(liushan) || liushan->isSkipped(Player::Play))
                        return QStringList();
                    return QStringList(objectName());
                }
            case Player::NotActive: {
                    if (liushan->hasFlag(objectName())) {
                        if (!liushan->canDiscard(liushan, "h"))
                            return QStringList();
                        return QStringList(objectName());
                    }
                    break;
                }
            default:
                    break;
            }
        } else if (triggerEvent == EventPhaseStart && liushan->getPhase() == Player::NotActive) {
            Room *room = liushan->getRoom();
            if (!room->getTag("IkBaishenTarget").isNull()) {
                ServerPlayer *target = room->getTag("IkBaishenTarget").value<ServerPlayer *>();
                if (target->isAlive())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            switch (change.to) {
            case Player::Play: {
                if (liushan->askForSkillInvoke(objectName()))
                    return true;
                }
            case Player::NotActive: {
                room->askForUseCard(liushan, "@@ikbaishen", "@ikbaishen-give", -1, Card::MethodDiscard);
                }
            default:
                    break;
            }
        } else if (triggerEvent == EventPhaseStart)
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *liushan, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            liushan->setFlags(objectName());
            liushan->skip(Player::Play, true);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *target = room->getTag("IkBaishenTarget").value<ServerPlayer *>();
            room->removeTag("IkBaishenTarget");
            target->gainAnExtraTurn();
        }
        return false;
    }
};

class IkHunshou: public PhaseChangeSkill {
public:
    IkHunshou(): PhaseChangeSkill("ikhunshou$") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        bool can_invoke = true;
        foreach (ServerPlayer *p, target->getRoom()->getAllPlayers()) {
            if (target->getHp() > p->getHp()) {
                can_invoke = false;
                break;
            }
        }
        return can_invoke && target != NULL && target->getPhase() == Player::Start
               && target->hasLordSkill("ikhunshou")
               && target->isAlive()
               && target->getMark("@hunshou") == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *liushan) const{
        Room *room = liushan->getRoom();
        room->notifySkillInvoked(liushan, objectName());

        LogMessage log;
        log.type = "#IkHunshouWake";
        log.from = liushan;
        log.arg = QString::number(liushan->getHp());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(liushan, "@hunshou", 1);
        if (room->changeMaxHpForAwakenSkill(liushan, 1)) {
            room->recover(liushan, RecoverStruct(liushan));
            if (liushan->isLord())
                room->acquireSkill(liushan, "ikxinqi");
        }

        return false;
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
            const Card *card = move.reason.m_extraData.value<const Card *>();
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
        const Card *card = move.reason.m_extraData.value<const Card *>();

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

class IkJuejing: public DrawCardsSkill {
public:
    IkJuejing(): DrawCardsSkill("#ikjuejing-draw") {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return DrawCardsSkill::triggerable(player)
            && player->isWounded();
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, "ikjuejing");
        room->broadcastSkillInvoke("ikjuejing");

        LogMessage log;
        log.type = "#YongsiGood";
        log.from = player;
        log.arg = QString::number(player->getLostHp());
        log.arg2 = "ikjuejing";
        room->sendLog(log);
        return n + player->getLostHp();
    }
};

class IkJuejingKeep: public MaxCardsSkill {
public:
    IkJuejingKeep(): MaxCardsSkill("ikjuejing") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return 2;
        else
            return 0;
    }
};

IkZhihun::IkZhihun(): ViewAsSkill("ikzhihun") {
    response_or_use = true;
}

bool IkZhihun::isEnabledAtResponse(const Player *player, const QString &pattern) const{
    return pattern == "slash"
           || pattern == "jink"
           || (pattern.contains("peach") && player->getMark("Global_PreventPeach") == 0)
           || pattern == "nullification";
}

bool IkZhihun::isEnabledAtPlay(const Player *player) const{
    return player->isWounded() || Slash::IsAvailable(player);
}

bool IkZhihun::viewFilter(const QList<const Card *> &selected, const Card *card) const{
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
            else if (card->getSuit() == Card::Spade) {
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
                return card->getSuit() == Card::Diamond;
            else if (pattern == "peach" || pattern == "peach+analeptic")
                return card->getSuit() == Card::Heart;
            else if (pattern == "slash")
                return card->getSuit() == Card::Spade;
        }
    default:
        break;
    }

    return false;
}

const Card *IkZhihun::viewAs(const QList<const Card *> &cards) const{
    int n = getEffHp(Self);

    if (cards.length() != n)
        return NULL;

    const Card *card = cards.first();
    Card *new_card = NULL;

    switch (card->getSuit()) {
    case Card::Diamond: {
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
    case Card::Spade: {
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

int IkZhihun::getEffectIndex(const ServerPlayer *player, const Card *card) const{
    return static_cast<int>(player->getRoom()->getCard(card->getSubcards().first())->getSuit()) + 1;
}

bool IkZhihun::isEnabledAtNullification(const ServerPlayer *player) const{
    int n = getEffHp(player), count = 0;
    foreach (const Card *card, player->getHandcards() + player->getEquips()) {
        if (card->getSuit() == Card::Diamond) count++;
        if (count >= n) return true;
    }
    return false;
}

int IkZhihun::getEffHp(const Player *zhaoyun) const{
    return qMax(1, zhaoyun->getHp());
}

IkXunyuCard::IkXunyuCard() {
    mute = true;
}

bool IkXunyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("ikxunyu");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void IkXunyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
            targets.removeOne(target);
    }

    if (targets.length() > 0) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_ikxunyu");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}

class IkXunyuViewAsSkill: public ViewAsSkill {
public:
    IkXunyuViewAsSkill(): ViewAsSkill("ikxunyu") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern.startsWith("@@ikxunyu");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1"))
            return false;
        else
            return selected.isEmpty() && !to_select->isKindOf("TrickCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1")) {
            return cards.isEmpty() ? new IkXunyuCard : NULL;
        } else {
            if (cards.length() != 1)
                return NULL;

            IkXunyuCard *card = new IkXunyuCard;
            card->addSubcards(cards);

            return card;
        }
    }
};

class IkXunyu: public TriggerSkill {
public:
    IkXunyu(): TriggerSkill("ikxunyu") {
        events << EventPhaseChanging;
        view_as_skill = new IkXunyuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(xiahouyuan)) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge)
            && !xiahouyuan->isSkipped(Player::Draw)) {
            if (Slash::IsAvailable(xiahouyuan))
                return QStringList(objectName());
        } else if (Slash::IsAvailable(xiahouyuan) && change.to == Player::Play && !xiahouyuan->isSkipped(Player::Play)) {
            if (xiahouyuan->canDiscard(xiahouyuan, "he"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer *) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge) {
            if (room->askForUseCard(xiahouyuan, "@@ikxunyu1", "@ikxunyu1", 1))
                return true;
        } else if (room->askForUseCard(xiahouyuan, "@@ikxunyu2", "@ikxunyu2", 2, Card::MethodDiscard))
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer *) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge) {
            xiahouyuan->skip(Player::Judge, true);
            xiahouyuan->skip(Player::Draw, true);
        } else
            xiahouyuan->skip(Player::Play, true);
        return false;
    }
};

IkMancaiCard::IkMancaiCard() {
    mute = true;
}

bool IkMancaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    Player::Phase phase = (Player::Phase)Self->getMark("ikmancaiPhase");
    if (phase == Player::Draw)
        return targets.length() <= 2 && !targets.isEmpty();
    else if (phase == Player::Play)
        return targets.length() == 1;
    return false;
}

bool IkMancaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Player::Phase phase = (Player::Phase)Self->getMark("ikmancaiPhase");
    if (phase == Player::Draw)
        return targets.length() < 2 && to_select != Self && !to_select->isKongcheng();
    else if (phase == Player::Play)
        return targets.isEmpty()
               && (!to_select->getJudgingArea().isEmpty() || !to_select->getEquips().isEmpty());
    return false;
}

void IkMancaiCard::use(Room *room, ServerPlayer *zhanghe, QList<ServerPlayer *> &targets) const{
    Player::Phase phase = (Player::Phase)zhanghe->getMark("ikmancaiPhase");
    if (phase == Player::Draw) {
        if (targets.isEmpty())
            return;

        foreach (ServerPlayer *target, targets) {
            if (zhanghe->isAlive() && target->isAlive())
                room->cardEffect(this, zhanghe, target);
        }
    } else if (phase == Player::Play) {
        if (targets.isEmpty())
            return;

        ServerPlayer *from = targets.first();
        if (!from->hasEquip() && from->getJudgingArea().isEmpty())
            return;

        int card_id = room->askForCardChosen(zhanghe, from , "ej", "ikmancai");
        const Card *card = Sanguosha->getCard(card_id);
        Player::Place place = room->getCardPlace(card_id);

        int equip_index = -1;
        if (place == Player::PlaceEquip) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equip->location());
        }

        QList<ServerPlayer *> tos;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (equip_index != -1) {
                if (p->getEquip(equip_index) == NULL)
                    tos << p;
            } else {
                if (!zhanghe->isProhibited(p, card) && !p->containsTrick(card->objectName()))
                    tos << p;
            }
        }

        room->setTag("IkMancaiTarget", QVariant::fromValue(from));
        ServerPlayer *to = room->askForPlayerChosen(zhanghe, tos, "ikmancai", "@ikmancai-to:::" + card->objectName());
        if (to)
            room->moveCardTo(card, from, to, place,
                             CardMoveReason(CardMoveReason::S_REASON_TRANSFER,
                                            zhanghe->objectName(), "ikmancai", QString()));
        room->removeTag("IkMancaiTarget");
    }
}

void IkMancaiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (!effect.to->isKongcheng()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "ikmancai");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
        room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
    }
}

class IkMancaiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkMancaiViewAsSkill(): ZeroCardViewAsSkill("ikmancai") {
        response_pattern = "@@ikmancai";
    }

    virtual const Card *viewAs() const{
        return new IkMancaiCard;
    }
};

class IkMancai: public TriggerSkill {
public:
    IkMancai(): TriggerSkill("ikmancai") {
        events << EventPhaseChanging;
        view_as_skill = new IkMancaiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(target) && target->canDiscard(target, "h")) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            switch (change.to) {
            case Player::RoundStart:
            case Player::Start:
            case Player::Finish:
            case Player::NotActive: return QStringList();

            case Player::Judge:
            case Player::Draw:
            case Player::Play:
            case Player::Discard: return QStringList(objectName());
            case Player::PhaseNone: Q_ASSERT(false);
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data, ServerPlayer *) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        room->setPlayerMark(zhanghe, "ikmancaiPhase", (int)change.to);
        int index = 0;
        switch (change.to) {
        case Player::RoundStart:
        case Player::Start:
        case Player::Finish:
        case Player::NotActive: return false;

        case Player::Judge: index = 1 ;break;
        case Player::Draw: index = 2; break;
        case Player::Play: index = 3; break;
        case Player::Discard: index = 4; break;
        case Player::PhaseNone: Q_ASSERT(false);
        }

        QString discard_prompt = QString("#ikmancai-%1").arg(index);
        if (index > 0 && room->askForDiscard(zhanghe, objectName(), 1, 1, true, false, discard_prompt)) {
            room->broadcastSkillInvoke("ikmancai", index);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data, ServerPlayer *) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        room->setPlayerMark(zhanghe, "ikmancaiPhase", (int)change.to);
        int index = 0;
        switch (change.to) {
        case Player::RoundStart:
        case Player::Start:
        case Player::Finish:
        case Player::NotActive: return false;

        case Player::Judge: index = 1 ;break;
        case Player::Draw: index = 2; break;
        case Player::Play: index = 3; break;
        case Player::Discard: index = 4; break;
        case Player::PhaseNone: Q_ASSERT(false);
        }

        QString use_prompt = QString("@ikmancai-%1").arg(index);
        if (!zhanghe->isAlive()) return false;
        if (!zhanghe->isSkipped(change.to) && (index == 2 || index == 3))
            room->askForUseCard(zhanghe, "@@ikmancai", use_prompt, index);
        zhanghe->skip(change.to, true);
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

class IkZhaihun: public TriggerSkill {
public:
    IkZhaihun(): TriggerSkill("ikzhaihun") {
        events << EventPhaseStart;
    }

    static int getWeaponCount(ServerPlayer *caoren) {
        int n = 0;
        foreach (ServerPlayer *p, caoren->getRoom()->getAlivePlayers()) {
            if (p->getWeapon()) n++;
        }
        return n;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (TriggerSkill::triggerable(target) && target->getPhase() == Player::Finish)
            || (target->getMark("@zhaihun") > 0 && target->getPhase() == Player::Draw);
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caoren, QVariant &, ServerPlayer *) const{
        if (caoren->getPhase() == Player::Finish) {
            if (caoren->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
            return false;
        } else
            return true;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *caoren, QVariant &) const{
        if (caoren->getPhase() == Player::Finish) {
            int n = getWeaponCount(caoren);
            caoren->drawCards(n + 2, objectName());
            caoren->turnOver();
            room->setPlayerMark(caoren, "@zhaihun", 1);
        } else {
            room->removePlayerMark(caoren, "@zhaihun");
            int n = getWeaponCount(caoren);
            if (n > 0) {
                LogMessage log;
                log.type = "#IkZhaihunDiscard";
                log.from = caoren;
                log.arg = QString::number(n);
                log.arg2 = objectName();
                room->sendLog(log);

                room->askForDiscard(caoren, objectName(), n, n, false, true);
            }
        }
        return false;
    }
};

class IkFojiao: public OneCardViewAsSkill {
public:
    IkFojiao(): OneCardViewAsSkill("ikfojiao") {
        filter_pattern = ".|.|.|equipped";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "nullification" && player->getHandcardNum() > player->getHp();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Nullification *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return player->getHandcardNum() > player->getHp() && !player->getEquips().isEmpty();
    }
};

IkQiangxiCard::IkQiangxiCard() {
}

bool IkQiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    int rangefix = 0;
    if (!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);;
    }

    return Self->inMyAttackRange(to_select, rangefix);
}

void IkQiangxiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    if (subcards.isEmpty())
        room->loseHp(effect.from);

    room->damage(DamageStruct("ikqiangxi", effect.from, effect.to));
}

class IkQiangxi: public ViewAsSkill {
public:
    IkQiangxi(): ViewAsSkill("ikqiangxi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkQiangxiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return new IkQiangxiCard;
        else if (cards.length() == 1) {
            IkQiangxiCard *card = new IkQiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

IkYushenCard::IkYushenCard() {
}

bool IkYushenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getHp() > Self->getHp() && !to_select->isKongcheng();
}

void IkYushenCard::use(Room *room, ServerPlayer *xunyu, QList<ServerPlayer *> &targets) const{
    ServerPlayer *tiger = targets.first();

    bool success = xunyu->pindian(tiger, "ikyushen", NULL);
    if (success) {
        QList<ServerPlayer *> players = room->getOtherPlayers(tiger), wolves;
        foreach (ServerPlayer *player, players) {
            if (tiger->inMyAttackRange(player))
                wolves << player;
        }

        if (wolves.isEmpty()) {
            LogMessage log;
            log.type = "#IkYushenNoWolf";
            log.from = xunyu;
            log.to << tiger;
            room->sendLog(log);

            return;
        }

        ServerPlayer *wolf = room->askForPlayerChosen(xunyu, wolves, "ikyushen", QString("@ikyushen-damage:%1").arg(tiger->objectName()));
        room->damage(DamageStruct("ikyushen", tiger, wolf));
    } else {
        room->damage(DamageStruct("ikyushen", tiger, xunyu));
    }
}

class IkYushen: public ZeroCardViewAsSkill {
public:
    IkYushen(): ZeroCardViewAsSkill("ikyushen") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkYushenCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new IkYushenCard;
    }
};

class IkJieming: public MasochismSkill {
public:
    IkJieming(): MasochismSkill("ikjieming") {
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *to = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "ikjieming-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkJiemingTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *xunyu, const DamageStruct &) const{
        ServerPlayer *to = xunyu->tag["IkJiemingTarget"].value<ServerPlayer *>();
        xunyu->tag.remove("IkJiemingTarget");
        if (to) {
            int upper = qMin(5, to->getMaxHp());
            int x = upper - to->getHandcardNum();
            if (x <= 0) return ;
            to->drawCards(x, objectName());
        }
    }
};

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
                                                    "ikbisuo-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            caopi->tag["ThBisuoTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *caopi, const DamageStruct &) const{
        ServerPlayer *to = caopi->tag["ThBisuoTarget"].value<ServerPlayer *>();
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
        JudgeStruct *judge = data.value<JudgeStruct *>();

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

class IkYindie: public TriggerSkill {
public:
    IkYindie(): TriggerSkill("ikyindie") {
        events << CardsMoveOneTime << FinishJudge;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player) || (room->getCurrent() == player && player->getPhase() != Player::NotActive))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        return false;
    }
};

class IkYindieMove: public TriggerSkill {
public:
    IkYindieMove(): TriggerSkill("#ikyindie-move") {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "ikyindie") {
                if (judge->isGood()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        player->addToPile("ikyindiepile", judge->card->getEffectiveId());

        return false;
    }
};

class IkYindieDistance: public DistanceSkill {
public:
    IkYindieDistance(): DistanceSkill("#ikyindie-dist") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill("ikyindie"))
            return -from->getPile("ikyindiepile").length();
        else
            return 0;
    }
};

class IkGuiyue: public PhaseChangeSkill {
public:
    IkGuiyue(): PhaseChangeSkill("ikguiyue") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("@guiyue") == 0
               && target->getPile("ikyindiepile").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *dengai) const{
        Room *room = dengai->getRoom();
        room->notifySkillInvoked(dengai, objectName());

        LogMessage log;
        log.type = "#IkGuiyueWake";
        log.from = dengai;
        log.arg = QString::number(dengai->getPile("ikyindiepile").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(dengai, "@guiyue", 1);
        if (room->changeMaxHpForAwakenSkill(dengai))
            room->acquireSkill(dengai, "ikhuanwu");

        return false;
    }
};

class IkHuanwu: public OneCardViewAsSkill {
public:
    IkHuanwu(): OneCardViewAsSkill("ikhuanwu") {
        expand_pile = "ikyindiepile";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("ikyindiepile").isEmpty();
    }

    virtual bool viewFilter(const Card *to_select) const{
        return Self->getPile("ikyindiepile").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Snatch *snatch = new Snatch(originalCard->getSuit(), originalCard->getNumber());
        snatch->addSubcard(originalCard);
        snatch->setSkillName(objectName());
        return snatch;
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

IkZhihuiCard::IkZhihuiCard() {
}

void IkZhihuiCard::onEffect(const CardEffectStruct &effect) const{
    DamageStruct damage = effect.from->tag.value("IkZhihuiDamage").value<DamageStruct>();
    damage.to = effect.to;
    damage.transfer = true;
    damage.transfer_reason = "ikzhihui";
    effect.from->tag["TransferDamage"] = QVariant::fromValue(damage);
}

class IkZhihuiViewAsSkill: public OneCardViewAsSkill {
public:
    IkZhihuiViewAsSkill(): OneCardViewAsSkill("ikzhihui") {
        filter_pattern = ".|heart|.|hand!";
        response_pattern = "@@ikzhihui";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkZhihuiCard *ikzhihuiCard = new IkZhihuiCard;
        ikzhihuiCard->addSubcard(originalCard);
        return ikzhihuiCard;
    }
};

class IkZhihui: public TriggerSkill {
public:
    IkZhihui(): TriggerSkill("ikzhihui") {
        events << DamageInflicted;
        view_as_skill = new IkZhihuiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *xiaoqiao) const{
        return TriggerSkill::triggerable(xiaoqiao)
            && xiaoqiao->canDiscard(xiaoqiao, "h");
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data, ServerPlayer *) const{
        xiaoqiao->tag["IkZhihuiDamage"] = data;
        return room->askForUseCard(xiaoqiao, "@@ikzhihui", "@ikzhihui-card", -1, Card::MethodDiscard);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data, ServerPlayer *) const{
        return true;
    }
};

class IkZhihuiDraw: public TriggerSkill {
public:
    IkZhihuiDraw(): TriggerSkill("#ikzhihui") {
        events << DamageComplete;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (player->isAlive() && damage.transfer && damage.transfer_reason == "ikzhihui")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(player->getLostHp(), objectName());
        return false;
    }
};

class IkChiqiu: public FilterSkill {
public:
    IkChiqiu(): FilterSkill("ikchiqiu") {
    }

    static WrappedCard *changeToHeart(int cardId) {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("ikchiqiu");
        new_card->setSuit(Card::Heart);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->getSuit() == Card::Spade;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        return changeToHeart(originalCard->getEffectiveId());
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return -2;
    }
};

IkJianmieCard::IkJianmieCard() {
}

bool IkJianmieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void IkJianmieCard::use(Room *room, ServerPlayer *taishici, QList<ServerPlayer *> &targets) const{
    bool success = taishici->pindian(targets.first(), "ikjianmie", NULL);
    if (success)
        room->setPlayerFlag(taishici, "IkJianmieSuccess");
    else
        room->setPlayerCardLimitation(taishici, "use", "Slash", true);
}

class IkJianmie: public ZeroCardViewAsSkill {
public:
    IkJianmie(): ZeroCardViewAsSkill("ikjianmie") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkJianmieCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new IkJianmieCard;
    }
};

class IkJianmieTargetMod: public TargetModSkill {
public:
    IkJianmieTargetMod(): TargetModSkill("#ikjianmie-target") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("IkJianmieSuccess"))
            return 1;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasFlag("IkJianmieSuccess"))
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasFlag("IkJianmieSuccess"))
            return 1;
        else
            return 0;
    }
};

class IkSusheng: public TriggerSkill {
public:
    IkSusheng(): TriggerSkill("iksusheng") {
        events << HpChanged;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(zhoutai)
            && !data.isNull() && !data.canConvert<RecoverStruct>() && zhoutai->getHp() < 1)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data, ServerPlayer *) const{
        if (zhoutai->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data, ServerPlayer *) const{
        QList<int> iksusheng = zhoutai->getPile("iksushengpile");

        int need = 1 - zhoutai->getHp(); // the buqu cards that should be turned over
        int n = need - iksusheng.length();
        if (n > 0) {
            QList<int> card_ids = room->getNCards(n, false);
            zhoutai->addToPile("iksushengpile", card_ids);
        }
        iksusheng = zhoutai->getPile("iksushengpile");
        QList<int> duplicate_numbers;

        QSet<int> numbers;
        foreach (int card_id, iksusheng) {
            const Card *card = Sanguosha->getCard(card_id);
            int number = card->getNumber();

            if (numbers.contains(number))
                duplicate_numbers << number;
            else
                numbers << number;
        }

        if (duplicate_numbers.isEmpty())
            return true;
        else
            zhoutai->clearOnePrivatePile("iksushengpile");
        
        return false;
    }
};

class IkSushengRemove: public TriggerSkill {
public:
    IkSushengRemove(): TriggerSkill("#iksusheng-remove") {
        events << HpRecover;
        frequency = Compulsory;
    }

    static void Remove(ServerPlayer *zhoutai) {
        Room *room = zhoutai->getRoom();
        QList<int> iksusheng(zhoutai->getPile("iksushengpile"));

        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "iksusheng", QString());
        int need = 1 - zhoutai->getHp();
        if (need <= 0) {
            // clear all the buqu cards
            foreach (int card_id, iksusheng) {
                LogMessage log;
                log.type = "$IkSushengRemove";
                log.from = zhoutai;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
            }
        } else {
            int to_remove = iksusheng.length() - need;
            for (int i = 0; i < to_remove; i++) {
                room->fillAG(iksusheng);
                int card_id = room->askForAG(zhoutai, iksusheng, false, "iksusheng");

                LogMessage log;
                log.type = "$IkSushengRemove";
                log.from = zhoutai;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                iksusheng.removeOne(card_id);
                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
                room->clearAG();
            }
        }
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && !target->getPile("iksushengpile").isEmpty();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *zhoutai, QVariant &, ServerPlayer *) const{
        Remove(zhoutai);
        return false;
    }
};

class IkSushengClear: public DetachEffectSkill {
public:
    IkSushengClear(): DetachEffectSkill("iksusheng") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        if (player->getHp() <= 0)
            room->enterDying(player, NULL);
    }
};

class IkEli: public TriggerSkill {
public:
    IkEli(): TriggerSkill("ikeli") {
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        QVariantList cards = target->tag["IkEli"].toList();
        return cards.length() >= 2 && TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Discard;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        SavageAssault *savage_assault = new SavageAssault(Card::NoSuit, 0);
        savage_assault->setSkillName("_ikeli");
        if (!savage_assault->isAvailable(player)) {
            delete savage_assault;
            return false;
        }
        room->useCard(CardUseStruct(savage_assault, player, QList<ServerPlayer *>()));
        return false;
    }
};

class IkEliRecord: public TriggerSkill {
public:
    IkEliRecord(): TriggerSkill("#theli-record") {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging)
            player->tag.remove("IkEli");
        else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player)
                return QStringList();

            if (player->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                QVariantList cards = player->tag["IkEli"].toList();
                foreach (int id, move.card_ids)
                    if (!cards.contains(id))
                        cards << id;
                player->tag["IkEli"] = cards;
            }
        }

        return QStringList();
    }
};

class IkHeyi: public TriggerSkill {
public:
    IkHeyi(): TriggerSkill("ikheyi") {
        events << TargetSpecified << TargetConfirmed;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *sunce, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(sunce)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified || (triggerEvent == TargetConfirmed && use.to.contains(sunce)))
            if (use.card->isKindOf("Duel") || (use.card->isKindOf("Slash") && use.card->isRed()))
                return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *sunce, QVariant &, ServerPlayer *) const{
        if (sunce->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *sunce, QVariant &, ServerPlayer *) const{ 
        sunce->drawCards(1, objectName());
        return false;
    }
};

class IkChizhu: public PhaseChangeSkill {
public:
    IkChizhu(): PhaseChangeSkill("ikchizhu") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("@chizhu") == 0
               && target->getPhase() == Player::Start
               && target->getHp() <= 2;
    }

    virtual bool onPhaseChange(ServerPlayer *sunce) const{
        Room *room = sunce->getRoom();
        room->notifySkillInvoked(sunce, objectName());

        LogMessage log;
        log.type = "#IkChizhuWake";
        log.from = sunce;
        log.arg = objectName();
        log.arg2 = QString::number(sunce->getHp());
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(sunce, "@chizhu", 1);
        if (room->changeMaxHpForAwakenSkill(sunce))
            room->handleAcquireDetachSkills(sunce, "ikchenhong|ikliangban");
        return false;
    }
};

IkBianshengCard::IkBianshengCard() {
    mute = true;
    m_skillName = "ikbiansheng_pindian";
}

bool IkBianshengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("ikbiansheng") && to_select != Self
           && !to_select->isKongcheng() && !to_select->hasFlag("IkBianshengInvoked");
}

void IkBianshengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *sunce = targets.first();
    room->setPlayerFlag(sunce, "IkBianshengInvoked");
    room->notifySkillInvoked(sunce, "ikbiansheng");
    if (sunce->getMark("@chizhu") > 0 && room->askForChoice(sunce, "ikbiansheng_pindian", "accept+reject") == "reject") {
        LogMessage log;
        log.type = "#IkBianshengReject";
        log.from = sunce;
        log.to << source;
        log.arg = "ikbiansheng_pindian";
        room->sendLog(log);

        return;
    }

    source->pindian(sunce, "ikbiansheng_pindian", NULL);
    QList<ServerPlayer *> sunces;
    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach (ServerPlayer *p, players) {
        if (p->hasLordSkill("ikbiansheng") && !p->hasFlag("IkBianshengInvoked"))
            sunces << p;
    }
    if (sunces.isEmpty())
        room->setPlayerFlag(source, "ForbidIkBiansheng");
}

class IkBianshengPindian: public ZeroCardViewAsSkill {
public:
    IkBianshengPindian(): ZeroCardViewAsSkill("ikbiansheng_pindian") {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getKingdom() == "yuki" && !player->isKongcheng() && !player->hasFlag("ForbidIkBiansheng");
    }

    virtual const Card *viewAs() const{
        return new IkBianshengCard;
    }
};

class IkBiansheng: public TriggerSkill {
public:
    IkBiansheng(): TriggerSkill("ikbiansheng$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Pindian << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (player == NULL) return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "ikbiansheng")) {
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
                if (!p->hasSkill("ikbiansheng_pindian"))
                    room->attachSkillToPlayer(p, "ikbiansheng_pindian");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "ikbiansheng") {
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
                if (p->hasSkill("ikbiansheng_pindian"))
                    room->detachSkillFromPlayer(p, "ikbiansheng_pindian", true);
            }
        } else if (triggerEvent == Pindian) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason != "ikbiansheng_pindian" || !pindian->to->hasLordSkill(objectName()))
                return QStringList();
            if (!pindian->isSuccess()) {
                ask_who = pindian->to;
                return QStringList(objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return QStringList();
            if (player->hasFlag("ForbidIkBiansheng"))
                room->setPlayerFlag(player, "-ForbidIkBiansheng");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("IkBianshengInvoked"))
                    room->setPlayerFlag(p, "-IkBianshengInvoked");
            }
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const{
        return ask_who->askForSkillInvoke(objectName(), "pindian");
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (room->getCardPlace(pindian->from_card->getEffectiveId()) == Player::PlaceTable)
            ask_who->obtainCard(pindian->from_card);
        if (room->getCardPlace(pindian->to_card->getEffectiveId()) == Player::PlaceTable)
            ask_who->obtainCard(pindian->to_card);
        return false;
    }
};

IkJibanCard::IkJibanCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkJibanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void IkJibanCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *erzhang = effect.from;
    erzhang->getRoom()->moveCardTo(this, erzhang, effect.to, Player::PlaceEquip,
                                   CardMoveReason(CardMoveReason::S_REASON_PUT,
                                                  erzhang->objectName(), "ikjiban", QString()));

    LogMessage log;
    log.type = "$IkJibanEquip";
    log.from = effect.to;
    log.card_str = QString::number(getEffectiveId());
    erzhang->getRoom()->sendLog(log);

    erzhang->drawCards(1, "ikjiban");
}

class IkJiban: public OneCardViewAsSkill {
public:
    IkJiban():OneCardViewAsSkill("ikjiban") {
        filter_pattern = "EquipCard|.|.|hand";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkJibanCard *ikjiban_card = new IkJibanCard();
        ikjiban_card->addSubcard(originalCard);
        return ikjiban_card;
    }
};

class IkJizhou: public TriggerSkill {
public:
    IkJizhou(): TriggerSkill("ikjizhou") {
        events << EventPhaseEnd << EventPhaseChanging;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard)
                foreach (ServerPlayer *erzhang, room->findPlayersBySkillName(objectName())) {
                    erzhang->tag.remove("IkJizhouToGet");
                    erzhang->tag.remove("IkJizhouOther");
                }
        } else if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() != Player::Discard) return skill_list;
            QList<ServerPlayer *> erzhangs = room->findPlayersBySkillName(objectName());

            foreach(ServerPlayer *erzhang, erzhangs) {
                QVariantList ikjizhou_cardsToGet = erzhang->tag["IkJizhouToGet"].toList();
                foreach(QVariant card_data, ikjizhou_cardsToGet) {
                    int card_id = card_data.toInt();
                    if (room->getCardPlace(card_id) == Player::DiscardPile)
                        skill_list.insert(erzhang, QStringList(objectName()));
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *erzhang) const {
        if (erzhang->askForSkillInvoke(objectName())){
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *erzhang) const{
        QVariantList ikjizhou_cardsToGet = erzhang->tag["IkJizhouToGet"].toList();
        QVariantList ikjizhou_cardsOther = erzhang->tag["IkJizhouOther"].toList();
        erzhang->tag.remove("IkJizhouToGet");
        erzhang->tag.remove("IkJizhouOther");

        QList<int> cardsToGet;
        foreach(QVariant card_data, ikjizhou_cardsToGet) {
            int card_id = card_data.toInt();
            if (room->getCardPlace(card_id) == Player::DiscardPile)
                cardsToGet << card_id;
        }
        QList<int> cardsOther;
        foreach(QVariant card_data, ikjizhou_cardsOther) {
            int card_id = card_data.toInt();
            if (room->getCardPlace(card_id) == Player::DiscardPile)
                cardsOther << card_id;
        }

        if (cardsToGet.isEmpty())
            return false;

        QList<int> cards = cardsToGet + cardsOther;

        room->fillAG(cards, erzhang, cardsOther);

        int to_back = room->askForAG(erzhang, cardsToGet, false, objectName());
        player->obtainCard(Sanguosha->getCard(to_back));

        cards.removeOne(to_back);

        room->clearAG(erzhang);

        DummyCard dummy(cards);
        room->obtainCard(erzhang, &dummy);

        return false;
    }
};

class IkJizhouRecord : public TriggerSkill {
public:
    IkJizhouRecord() : TriggerSkill("#ikjizhou-record") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *erzhang, QVariant &data, ServerPlayer * &) const{
        if (!erzhang || !erzhang->isAlive() || !erzhang->hasSkill("ikjizhou")) return QStringList();
        ServerPlayer *current = room->getCurrent();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (!current || erzhang == current || current->getPhase() != Player::Discard)
            return QStringList();

        if (current->getPhase() == Player::Discard) {
            QVariantList ikjizhouToGet = erzhang->tag["IkJizhouToGet"].toList();
            QVariantList ikjizhouOther = erzhang->tag["IkJizhouOther"].toList();

            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                int i = 0;
                foreach (int card_id, move.card_ids) {
                    if (move.from == current && move.from_places[i] == Player::PlaceHand)
                        ikjizhouToGet << card_id;
                    else if (!ikjizhouToGet.contains(card_id))
                        ikjizhouOther << card_id;
                    i++;
                }
            }

            erzhang->tag["IkJizhouToGet"] = ikjizhouToGet;
            erzhang->tag["IkJizhouOther"] = ikjizhouOther;
        }

        return QStringList();
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

IkaiKiPackage::IkaiKiPackage()
    :Package("ikai-ki")
{
    General *wind008 = new General(this, "wind008", "kaze");
    wind008->addSkill("thxiagong");
    wind008->addSkill(new IkLiegong);

    General *wind009 = new General(this, "wind009", "kaze");
    wind009->addSkill(new IkKuanggu);

    General *wind010 = new General(this, "wind010", "kaze", 3);
    wind010->addSkill(new IkFuyao);
    wind010->addSkill(new IkNiepan);

    General *wind011 = new General(this, "wind011", "kaze", 3);
    wind011->addSkill(new IkJingnie);
    wind011->addSkill(new IkJianyan);
    wind011->addSkill(new IkXuanying);

    General *wind012 = new General(this, "wind012", "kaze");
    wind012->addSkill(new IkTiaoxin);
    wind012->addSkill(new IkShengtian);
    wind012->addRelateSkill("ikxuanwu");
    wind012->addRelateSkill("ikmohua");

    General *wind013 = new General(this, "wind013", "kaze");
    wind013->addSkill(new SavageAssaultAvoid("ikhuoshou"));
    wind013->addSkill(new IkHuoshou);
    wind013->addSkill(new IkZailuan);
    related_skills.insertMulti("ikhuoshou", "#sa_avoid_ikhuoshou");

    General *wind014 = new General(this, "wind014$", "kaze", 3);
    wind014->addSkill(new IkManbo);
    wind014->addSkill(new IkBaishen);
    wind014->addSkill(new IkHunshou);

    General *wind015 = new General(this, "wind015", "kaze", 4, false);
    wind015->addSkill(new SavageAssaultAvoid("ikjugui"));
    wind015->addSkill(new IkJugui);
    wind015->addSkill(new IkLieren);
    related_skills.insertMulti("ikjugui", "#sa_avoid_ikjugui");

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

    General *wind030 = new General(this, "wind030", "kaze", 2);
    wind030->addSkill(new IkJuejingKeep);
    wind030->addSkill(new IkJuejing);
    wind030->addSkill(new IkZhihun);
    related_skills.insertMulti("ikjuejing", "#ikjuejing-draw");

    General *bloom008 = new General(this, "bloom008", "hana");
    bloom008->addSkill(new IkXunyu);
    bloom008->addSkill(new SlashNoDistanceLimitSkill("ikxunyu"));
    related_skills.insertMulti("ikxunyu", "#ikxunyu-slash-ndl");

    General *bloom009 = new General(this, "bloom009", "hana");
    bloom009->addSkill(new IkMancai);

    General *bloom010 = new General(this, "bloom010", "hana");
    bloom010->addSkill(new IkKujie);
    bloom010->addSkill(new IkKujieTargetMod);
    related_skills.insertMulti("ikkujie", "#ikkujie-target");

    General *bloom011 = new General(this, "bloom011", "hana");
    bloom011->addSkill(new IkZhaihun);
    bloom011->addSkill(new IkFojiao);

    General *bloom012 = new General(this, "bloom012", "hana");
    bloom012->addSkill(new IkQiangxi);

    General *bloom013 = new General(this, "bloom013", "hana", 3);
    bloom013->addSkill(new IkYushen);
    bloom013->addSkill(new IkJieming);

    General *bloom014 = new General(this, "bloom014$", "hana", 3);
    bloom014->addSkill(new IkTanwan);
    bloom014->addSkill(new IkBisuo);
    bloom014->addSkill(new IkSongwei);

    General *bloom015 = new General(this, "bloom015", "hana");
    bloom015->addSkill(new IkYindie);
    bloom015->addSkill(new IkYindieMove);
    bloom015->addSkill(new IkYindieDistance);
    related_skills.insertMulti("ikyindie", "#ikyindie-move");
    related_skills.insertMulti("ikyindie", "#ikyindie-dist");
    bloom015->addSkill(new IkGuiyue);
    bloom015->addRelateSkill("ikhuanwu");

    General *bloom029 = new General(this, "bloom029", "hana", 3);
    bloom029->addSkill(new IkYihuo);
    bloom029->addSkill(new IkGuixin);

    General *bloom030 = new General(this, "bloom030", "hana");
    bloom030->addSkill(new IkRenjia);
    bloom030->addSkill(new IkTiangai);
    bloom030->addRelateSkill("ikjilve");
    related_skills.insertMulti("ikjilve", "#ikjilve-clear");

    General *snow009 = new General(this, "snow009", "yuki");
    snow009->addSkill(new IkLiangban);

    General *snow010 = new General(this, "snow010", "yuki", 3);
    snow010->addSkill(new IkShenen);
    snow010->addSkill(new IkShenenGive);
    related_skills.insertMulti("ikshenen", "#ikshenen-give");
    snow010->addSkill(new IkDimeng);

    General *snow011 = new General(this, "snow011", "yuki", 3, false);
    snow011->addSkill(new IkZhihui);
    snow011->addSkill(new IkZhihuiDraw);
    snow011->addSkill(new IkChiqiu);
    related_skills.insertMulti("ikzhihui", "#ikzhihui");

    General *snow012 = new General(this, "snow012", "yuki");
    snow012->addSkill(new IkJianmie);
    snow012->addSkill(new IkJianmieTargetMod);
    related_skills.insertMulti("ikjianmie", "#ikjianmie-target");

    General *snow013 = new General(this, "snow013", "yuki", 3);
    snow013->addSkill(new IkSusheng);
    snow013->addSkill(new IkSushengRemove);
    snow013->addSkill(new IkSushengClear);
    related_skills.insertMulti("iksusheng", "#iksusheng-remove");
    related_skills.insertMulti("iksusheng", "#iksusheng-clear");
    snow013->addSkill(new IkEli);
    snow013->addSkill(new IkEliRecord);
    related_skills.insertMulti("ikeli", "#ikeli-record");

    General *snow014 = new General(this, "snow014$", "yuki");
    snow014->addSkill(new IkHeyi);
    snow014->addSkill(new IkChizhu);
    snow014->addSkill(new IkBiansheng);

    General *snow015 = new General(this, "snow015", "yuki", 3);
    snow015->addSkill(new IkJiban);
    snow015->addSkill(new IkJizhou);
    snow015->addSkill(new IkJizhouRecord);
    related_skills.insertMulti("ikjizhou", "#ikjizhou-record");

    General *snow029 = new General(this, "snow029", "yuki", 3);
    snow029->addSkill(new IkLvejue);
    snow029->addSkill(new IkLingshi);

    General *snow030 = new General(this, "snow030", "yuki");
    snow030->addSkill(new IkLongxi);
    snow030->addSkill(new IkLongxiRecord);
    related_skills.insertMulti("iklongxi", "#iklongxi-record");
    snow030->addSkill(new IkYeyan);

    addMetaObject<IkTiaoxinCard>();
    addMetaObject<IkBaishenCard>();
    addMetaObject<IkLiefengCard>();
    addMetaObject<IkMiaowuCard>();
    addMetaObject<IkXunyuCard>();
    addMetaObject<IkMancaiCard>();
    addMetaObject<IkQiangxiCard>();
    addMetaObject<IkYushenCard>();
    addMetaObject<IkYihuoCard>();
    addMetaObject<IkJilveCard>();
    addMetaObject<IkShenenCard>();
    addMetaObject<IkDimengCard>();
    addMetaObject<IkZhihuiCard>();
    addMetaObject<IkJianmieCard>();
    addMetaObject<IkBianshengCard>();
    addMetaObject<IkJibanCard>();
    addMetaObject<IkLingshiCard>();
    addMetaObject<IkYeyanCard>();
    addMetaObject<GreatIkYeyanCard>();
    addMetaObject<SmallIkYeyanCard>();

    skills << new IkMohua << new IkHuanwu << new IkYihuoViewAsSkill << new IkJilve << new IkJilveClear << new IkBianshengPindian;
}

ADD_PACKAGE(IkaiKi)
