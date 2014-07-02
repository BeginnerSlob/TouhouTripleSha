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

    addMetaObject<IkTiaoxinCard>();
    addMetaObject<IkBaishenCard>();
    addMetaObject<IkLiefengCard>();
    addMetaObject<IkMiaowuCard>();

    skills << new IkMohua;
}

ADD_PACKAGE(IkaiKi)
