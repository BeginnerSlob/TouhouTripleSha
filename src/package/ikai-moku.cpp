#include "ikai-moku.h"

#include "ai.h"
#include "client.h"
#include "engine.h"
#include "fantasy.h"
#include "general.h"
#include "maneuvering.h"
#include "settings.h"
#include "skill.h"

class IkLiegong : public TriggerSkill
{
public:
    IkLiegong()
        : TriggerSkill("ikliegong")
    {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (player != use.from || player->getPhase() != Player::Play || !use.card->isKindOf("Slash"))
            return QStringList();
        QStringList targets;
        foreach (ServerPlayer *to, use.to) {
            int handcard_num = to->getHandcardNum();
            if (handcard_num >= player->getHp() || handcard_num <= player->getAttackRange())
                targets << to->objectName();
        }
        if (!targets.isEmpty())
            return QStringList(objectName() + "->" + targets.join("+"));
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *player) const
    {
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(skill_target))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *p, QVariant &data, ServerPlayer *player) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();

        p->addMark("ikliegong");
        room->addPlayerMark(p, "@skill_invalidity");

        foreach (ServerPlayer *pl, room->getAllPlayers())
            room->filterCards(pl, pl->getCards("he"), true);
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        LogMessage log;
        log.type = "#NoJink";
        log.from = p;
        room->sendLog(log);
        jink_list.replace(use.to.indexOf(p), QVariant(0));
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class IkLiegongClear : public TriggerSkill
{
public:
    IkLiegongClear()
        : TriggerSkill("#ikliegong-clear")
    {
        events << EventPhaseChanging << Death;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != target || target != room->getCurrent())
                return QStringList();
        }
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players) {
            if (player->getMark("ikliegong") == 0)
                continue;
            room->removePlayerMark(player, "@skill_invalidity", player->getMark("ikliegong"));
            player->setMark("ikliegong", 0);

            foreach (ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), false);
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return QStringList();
    }
};

IkHuanghunCard::IkHuanghunCard()
{
    will_throw = false;
    can_recast = true;
    handling_method = Card::MethodRecast;
    target_fixed = true;
}

void IkHuanghunCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;

    CardMoveReason reason(CardMoveReason::S_REASON_RECAST, player->objectName());
    reason.m_skillName = this->getSkillName();
    room->moveCardTo(this, player, NULL, Player::DiscardPile, reason);
    player->broadcastSkillInvoke("@recast");

    int id = card_use.card->getSubcards().first();

    LogMessage log;
    log.type = "#UseCard_Recast";
    log.from = player;
    log.card_str = QString::number(id);
    room->sendLog(log);

    player->drawCards(1, "recast");
}

class IkHuanghun : public OneCardViewAsSkill
{
public:
    IkHuanghun()
        : OneCardViewAsSkill("ikhuanghun")
    {
        filter_pattern = "TrickCard";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (Self->isCardLimited(originalCard, Card::MethodRecast))
            return NULL;

        IkHuanghunCard *recast = new IkHuanghunCard;
        recast->addSubcard(originalCard);
        return recast;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }
};

class IkKuanggu : public TriggerSkill
{
public:
    IkKuanggu()
        : TriggerSkill("ikkuanggu")
    {
        frequency = Compulsory;
        events << Damage << PreDamageDone;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *weiyan = damage.from;
            if (weiyan) {
                if (weiyan->distanceTo(damage.to) != -1 && weiyan->distanceTo(damage.to) <= 1 && weiyan != player)
                    weiyan->tag["InvokeIkKuanggu"] = damage.damage;
                else
                    weiyan->tag.remove("InvokeIkKuanggu");
            }
            return QStringList();
        }
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        bool ok = false;
        int recorded_damage = player->tag["InvokeIkKuanggu"].toInt(&ok);
        DamageStruct damage = data.value<DamageStruct>();
        if (ok && recorded_damage > 0 && (player->isWounded() || !damage.card || !damage.card->isKindOf("Slash"))) {
            QStringList skills;
            for (int i = 0; i < damage.damage; i++)
                skills << objectName();
            return skills;
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        DamageStruct damage = data.value<DamageStruct>();

        QStringList choices;
        if (player->isWounded())
            choices << "recover";
        if (!damage.card || !damage.card->isKindOf("Slash"))
            choices << "draw";
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "recover")
            room->recover(player, RecoverStruct(player));
        else
            player->drawCards(2);
        return false;
    }
};

class IkFuhua : public OneCardViewAsSkill
{
public:
    IkFuhua()
        : OneCardViewAsSkill("ikfuhua")
    {
        filter_pattern = ".|club";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return qrand() % 2 + 1;
    }
};

class IkSuinie : public TriggerSkill
{
public:
    IkSuinie()
        : TriggerSkill("iksuinie")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@suinie";
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *pangtong, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(pangtong) || pangtong->getMark("@suinie") == 0)
            return QStringList();
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != pangtong)
            return QStringList();
        if (pangtong->isDead() || pangtong->getHp() > 0)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data, ServerPlayer *) const
    {
        if (pangtong->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &, ServerPlayer *) const
    {
        room->removePlayerMark(pangtong, "@suinie");
        room->addPlayerMark(pangtong, "@suinieused");

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

class IkJingnie : public TriggerSkill
{
public:
    IkJingnie()
        : TriggerSkill("ikjingnie")
    {
        events << GameStart << EventAcquireSkill;
        frequency = Compulsory;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == GameStart) {
            if (player)
                return QStringList();
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(Sanguosha->getSkill("eight_diagram"));
            room->getThread()->addTriggerSkill(trigger_skill);
        } else if (data.toString() == objectName()) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(Sanguosha->getSkill("eight_diagram"));
            room->getThread()->addTriggerSkill(trigger_skill);
        }
        return QStringList();
    }
};

class IkJianyan : public OneCardViewAsSkill
{
public:
    IkJianyan()
        : OneCardViewAsSkill("ikjianyan")
    {
        filter_pattern = ".|red";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        return fire_attack;
    }
};

class IkXuanying : public OneCardViewAsSkill
{
public:
    IkXuanying()
        : OneCardViewAsSkill("ikxuanying")
    {
        filter_pattern = ".|black";
        response_pattern = "nullification";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->isAlive();
    }
};

IkTiaoxinCard::IkTiaoxinCard()
{
}

bool IkTiaoxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->inMyAttackRange(Self);
}

void IkTiaoxinCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    bool use_slash = false;
    if (effect.to->canSlash(effect.from, NULL, false))
        use_slash = room->askForUseSlashTo(effect.to, effect.from, "@iktiaoxin-slash:" + effect.from->objectName());
    if (!use_slash && effect.from->canDiscard(effect.to, "he"))
        room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "iktiaoxin", false, Card::MethodDiscard), effect.to, effect.from);
}

class IkTiaoxin : public ZeroCardViewAsSkill
{
public:
    IkTiaoxin()
        : ZeroCardViewAsSkill("iktiaoxin")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkTiaoxinCard");
    }

    virtual const Card *viewAs() const
    {
        return new IkTiaoxinCard;
    }
};

class IkShengtian : public PhaseChangeSkill
{
public:
    IkShengtian()
        : PhaseChangeSkill("ikshengtian")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getMark("@shengtian") == 0 && target->getPhase() == Player::Start && target->isKongcheng();
    }

    virtual bool onPhaseChange(ServerPlayer *jiangwei) const
    {
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

class IkMohua : public FilterSkill
{
public:
    IkMohua()
        : FilterSkill("ikmohua")
    {
    }

    static WrappedCard *changeToClub(int cardId)
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("ikmohua");
        new_card->setSuit(Card::Club);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->getSuit() == Card::Diamond;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        return changeToClub(originalCard->getEffectiveId());
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return -2;
    }
};

class SavageAssaultAvoid : public TriggerSkill
{
public:
    SavageAssaultAvoid(const QString &avoid_skill)
        : TriggerSkill("#sa_avoid_" + avoid_skill)
        , avoid_skill(avoid_skill)
    {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player->isDead() || !player->hasSkill(avoid_skill))
            return QStringList();
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isKindOf("SavageAssault"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
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

class IkHuoshou : public TriggerSkill
{
public:
    IkHuoshou()
        : TriggerSkill("ikhuoshou")
    {
        events << TargetSpecified << ConfirmDamage;
        frequency = Compulsory;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *&ask_who) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("SavageAssault"))
                foreach (ServerPlayer *menghuo, room->findPlayersBySkillName(objectName()))
                    if (menghuo && menghuo != use.from) {
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
            if (!menghuo)
                return QStringList();
            damage.from = menghuo->isAlive() ? menghuo : NULL;
            data = QVariant::fromValue(damage);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        room->sendCompulsoryTriggerLog(ask_who, objectName());
        room->broadcastSkillInvoke(objectName());

        use.card->setFlags("IkHuoshouDamage_" + ask_who->objectName());

        return false;
    }
};

class IkZailuan : public PhaseChangeSkill
{
public:
    IkZailuan()
        : PhaseChangeSkill("ikzailuan")
    {
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Draw && target->isWounded();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *menghuo) const
    {
        Room *room = menghuo->getRoom();

        int x = menghuo->getLostHp();
        QList<int> ids = room->getNCards(x, false);
        CardsMoveStruct move(ids, menghuo, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, menghuo->objectName(), "ikzailuan", QString()));
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

class IkFule : public TriggerSkill
{
public:
    IkFule()
        : TriggerSkill("ikfule")
    {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *liushan, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(liushan) && use.card->isKindOf("Slash"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(liushan, objectName());

        if (!room->askForCard(use.from, ".Basic", "@ikfule-discard")) {
            use.nullified_list << liushan->objectName();
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

IkYoujiCard::IkYoujiCard()
{
}

bool IkYoujiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void IkYoujiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    ServerPlayer *liushan = effect.from, *player = effect.to;

    LogMessage log;
    log.type = "#IkYouji";
    log.from = liushan;
    log.to << player;
    room->sendLog(log);

    player->gainAnExtraTurn();
}

class IkYoujiViewAsSkill : public OneCardViewAsSkill
{
public:
    IkYoujiViewAsSkill()
        : OneCardViewAsSkill("ikyouji")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@ikyouji";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IkYoujiCard *ikyouji = new IkYoujiCard;
        ikyouji->addSubcard(originalCard);
        return ikyouji;
    }
};

class IkYouji : public TriggerSkill
{
public:
    IkYouji()
        : TriggerSkill("ikyouji")
    {
        events << EventPhaseChanging;
        view_as_skill = new IkYoujiViewAsSkill;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *liushan, QVariant &data, ServerPlayer *&) const
    {
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
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        switch (change.to) {
        case Player::Play: {
            if (liushan->askForSkillInvoke(objectName()))
                return true;
            break;
        }
        case Player::NotActive: {
            room->askForUseCard(liushan, "@@ikyouji", "@ikyouji-give", -1, Card::MethodDiscard);
            break;
        }
        default:
            break;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *liushan, QVariant &, ServerPlayer *) const
    {
        liushan->setFlags(objectName());
        liushan->skip(Player::Play, true);
        return false;
    }
};

class IkRuoyu : public PhaseChangeSkill
{
public:
    IkRuoyu()
        : PhaseChangeSkill("ikruoyu$")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        if (!target)
            return false;
        foreach (ServerPlayer *p, target->getRoom()->getAllPlayers()) {
            if (p->getHp() < target->getHp())
                return false;
        }
        return target->getPhase() == Player::Start && target->isAlive() && target->hasLordSkill("ikruoyu") && target->getMark("@ruoyu") == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *liushan) const
    {
        Room *room = liushan->getRoom();
        room->notifySkillInvoked(liushan, objectName());

        LogMessage log;
        log.type = "#IkRuoyuWake";
        log.from = liushan;
        log.arg = QString::number(liushan->getHp());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(liushan, "@ruoyu", 1);
        if (room->changeMaxHpForAwakenSkill(liushan, 1)) {
            room->recover(liushan, RecoverStruct(liushan));
            if (liushan->isLord())
                room->acquireSkill(liushan, "ikxinqi");
        }

        return false;
    }
};

class IkJugui : public TriggerSkill
{
public:
    IkJugui()
        : TriggerSkill("ikjugui")
    {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *card = move.reason.m_extraData.value<const Card *>();

        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        player->obtainCard(card);
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);

        return false;
    }
};

class IkLieren : public TriggerSkill
{
public:
    IkLieren()
        : TriggerSkill("iklieren")
    {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *zhurong, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(zhurong))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel")) && !zhurong->isKongcheng() && !target->isKongcheng()
            && !target->hasFlag("Global_DebutFlag") && !damage.chain && !damage.transfer)
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhurong, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        bool success = zhurong->pindian(target, "iklieren", NULL);
        if (!success)
            return false;

        if (!target->isNude()) {
            int card_id = room->askForCardChosen(zhurong, target, "he", objectName());
            room->obtainCard(zhurong, Sanguosha->getCard(card_id), false);
        }

        return false;
    }
};

class IkQiyao : public TriggerSkill
{
public:
    IkQiyao()
        : TriggerSkill("ikqiyao")
    {
        frequency = Frequent;
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && !target->getPile("luminary").isEmpty() && target->getPhase() == Player::Draw;
    }

    static void Exchange(ServerPlayer *shenzhuge)
    {
        shenzhuge->exchangeFreelyFromPrivatePile("ikqiyao", "luminary");
    }

    static void DiscardStar(ServerPlayer *shenzhuge, int n, QString skillName)
    {
        Room *room = shenzhuge->getRoom();
        QList<int> luminary = shenzhuge->getPile("luminary");

        for (int i = 0; i < n; i++) {
            room->fillAG(luminary, shenzhuge);
            int card_id = room->askForAG(shenzhuge, luminary, false, "ikqiyao-discard");
            room->clearAG(shenzhuge);
            luminary.removeOne(card_id);
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), skillName, QString());
            room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
        }
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *shenzhuge, QVariant &, ServerPlayer *) const
    {
        if (shenzhuge->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *shenzhuge, QVariant &, ServerPlayer *) const
    {
        Exchange(shenzhuge);
        return false;
    }
};

class IkQiyaoStart : public TriggerSkill
{
public:
    IkQiyaoStart()
        : TriggerSkill("#ikqiyao")
    {
        events << DrawInitialCards << AfterDrawInitialCards;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *shenzhuge, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DrawInitialCards) {
            room->sendCompulsoryTriggerLog(shenzhuge, "ikqiyao");
            data = data.toInt() + 7;
        } else if (triggerEvent == AfterDrawInitialCards) {
            room->broadcastSkillInvoke("ikqiyao");
            const Card *exchange_card = room->askForExchange(shenzhuge, "ikqiyao", 7, 7);
            shenzhuge->addToPile("luminary", exchange_card->getSubcards(), false);
            delete exchange_card;
        }
        return false;
    }
};

IkLiefengCard::IkLiefengCard()
{
    handling_method = Card::MethodNone;
}

bool IkLiefengCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void IkLiefengCard::onEffect(const CardEffectStruct &effect) const
{
    IkQiyao::DiscardStar(effect.from, 1, "ikliefeng");
    effect.from->tag["IkLiefeng_user"] = true;
    effect.to->gainMark("@gale");
}

class IkLiefengViewAsSkill : public ZeroCardViewAsSkill
{
public:
    IkLiefengViewAsSkill()
        : ZeroCardViewAsSkill("ikliefeng")
    {
        response_pattern = "@@ikliefeng";
    }

    virtual const Card *viewAs() const
    {
        return new IkLiefengCard;
    }
};

class IkLiefeng : public TriggerSkill
{
public:
    IkLiefeng()
        : TriggerSkill("ikliefeng")
    {
        events << EventPhaseStart;
        view_as_skill = new IkLiefengViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && !target->getPile("luminary").isEmpty() && target->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *shenzhuge, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(shenzhuge, "@@ikliefeng", "@ikliefeng-card", -1, Card::MethodNone);
        return false;
    }
};

class IkLiefengTrigger : public TriggerSkill
{
public:
    IkLiefengTrigger()
        : TriggerSkill("#ikliefeng")
    {
        events << DamageForseen;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player->getMark("@gale") > 0) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature == DamageStruct::Fire)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
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

class IkLiefengClear : public TriggerSkill
{
public:
    IkLiefengClear()
        : TriggerSkill("#ikliefeng-clear")
    {
        events << EventPhaseStart << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
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
            player->loseAllMarks("@gale");
        player->tag.remove("IkLiefeng_user");

        return QStringList();
    }
};

IkMiaowuCard::IkMiaowuCard()
{
    handling_method = Card::MethodNone;
}

bool IkMiaowuCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    return targets.length() < Self->getPile("luminary").length();
}

void IkMiaowuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    int n = targets.length();
    IkQiyao::DiscardStar(source, n, "ikmiaowu");
    source->tag["IkMiaowu_user"] = true;

    foreach (ServerPlayer *target, targets)
        target->gainMark("@fog");
}

class IkMiaowuViewAsSkill : public ZeroCardViewAsSkill
{
public:
    IkMiaowuViewAsSkill()
        : ZeroCardViewAsSkill("ikmiaowu")
    {
        response_pattern = "@@ikmiaowu";
    }

    virtual const Card *viewAs() const
    {
        return new IkMiaowuCard;
    }
};

class IkMiaowu : public TriggerSkill
{
public:
    IkMiaowu()
        : TriggerSkill("ikmiaowu")
    {
        events << EventPhaseStart;
        view_as_skill = new IkMiaowuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && !target->getPile("luminary").isEmpty() && target->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *shenzhuge, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(shenzhuge, "@@ikmiaowu", "@ikmiaowu-card", -1, Card::MethodNone);
        return false;
    }
};

class IkMiaowuTrigger : public TriggerSkill
{
public:
    IkMiaowuTrigger()
        : TriggerSkill("#ikmiaowu")
    {
        events << DamageForseen;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player->getMark("@fog") > 0) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Thunder)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
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

class IkMiaowuClear : public TriggerSkill
{
public:
    IkMiaowuClear()
        : TriggerSkill("#ikmiaowu-clear")
    {
        events << EventPhaseStart << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
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
            player->loseAllMarks("@fog");
        player->tag.remove("IkMiaowu_user");

        return QStringList();
    }
};

class IkJuejing : public DrawCardsSkill
{
public:
    IkJuejing()
        : DrawCardsSkill("ikjuejing")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return DrawCardsSkill::triggerable(player) && player->isWounded();
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());

        LogMessage log;
        log.type = "#IkChenyanGood";
        log.from = player;
        log.arg = QString::number(player->getLostHp());
        log.arg2 = "ikjuejing";
        room->sendLog(log);
        return n + player->getLostHp();
    }
};

class IkJuejingKeep : public MaxCardsSkill
{
public:
    IkJuejingKeep()
        : MaxCardsSkill("#ikjuejing-keep")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill("ikjuejing"))
            return 2;
        else
            return 0;
    }
};

IkZhihun::IkZhihun()
    : ViewAsSkill("ikzhihun")
{
    response_or_use = true;
}

bool IkZhihun::isEnabledAtResponse(const Player *player, const QString &pattern) const
{
    return pattern == "slash" || pattern == "jink" || (pattern.contains("peach") && player->getMark("Global_PreventPeach") == 0) || pattern == "nullification";
}

bool IkZhihun::isEnabledAtPlay(const Player *player) const
{
    return player->isWounded() || Slash::IsAvailable(player);
}

bool IkZhihun::viewFilter(const QList<const Card *> &selected, const Card *card) const
{
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

const Card *IkZhihun::viewAs(const QList<const Card *> &cards) const
{
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

int IkZhihun::getEffectIndex(const ServerPlayer *player, const Card *card) const
{
    return static_cast<int>(player->getRoom()->getCard(card->getSubcards().first())->getSuit()) + 1;
}

bool IkZhihun::isEnabledAtNullification(const ServerPlayer *) const
{
    return true;
}

int IkZhihun::getEffHp(const Player *zhaoyun) const
{
    return qMax(1, zhaoyun->getHp());
}

IkXunyuCard::IkXunyuCard()
{
    mute = true;
}

bool IkXunyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("ikxunyu");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void IkXunyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
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

class IkXunyuViewAsSkill : public ViewAsSkill
{
public:
    IkXunyuViewAsSkill()
        : ViewAsSkill("ikxunyu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@ikxunyu");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1"))
            return false;
        else
            return selected.isEmpty() && !to_select->isKindOf("TrickCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
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

class IkXunyu : public TriggerSkill
{
public:
    IkXunyu()
        : TriggerSkill("ikxunyu")
    {
        events << EventPhaseChanging;
        view_as_skill = new IkXunyuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(xiahouyuan))
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge) && !xiahouyuan->isSkipped(Player::Draw)) {
            if (Slash::IsAvailable(xiahouyuan))
                return QStringList(objectName());
        } else if (Slash::IsAvailable(xiahouyuan) && change.to == Player::Play && !xiahouyuan->isSkipped(Player::Play)) {
            if (xiahouyuan->canDiscard(xiahouyuan, "he"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge) {
            if (room->askForUseCard(xiahouyuan, "@@ikxunyu1", "@ikxunyu1", 1))
                return true;
        } else if (room->askForUseCard(xiahouyuan, "@@ikxunyu2", "@ikxunyu2", 2, Card::MethodDiscard))
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge) {
            xiahouyuan->skip(Player::Judge, true);
            xiahouyuan->skip(Player::Draw, true);
        } else
            xiahouyuan->skip(Player::Play, true);
        return false;
    }
};

IkMancaiCard::IkMancaiCard()
{
    mute = true;
}

bool IkMancaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Player::Phase phase = (Player::Phase)Self->getMark("ikmancaiPhase");
    if (phase == Player::Draw)
        return targets.length() <= 2 && !targets.isEmpty();
    else if (phase == Player::Play)
        return targets.length() == 1;
    return false;
}

bool IkMancaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Player::Phase phase = (Player::Phase)Self->getMark("ikmancaiPhase");
    if (phase == Player::Draw)
        return targets.length() < 2 && to_select != Self && !to_select->isKongcheng();
    else if (phase == Player::Play)
        return targets.isEmpty() && (!to_select->getJudgingArea().isEmpty() || !to_select->getEquips().isEmpty());
    return false;
}

void IkMancaiCard::use(Room *room, ServerPlayer *zhanghe, QList<ServerPlayer *> &targets) const
{
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

        int card_id = room->askForCardChosen(zhanghe, from, "ej", "ikmancai");
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
            room->moveCardTo(card, from, to, place, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, zhanghe->objectName(), "ikmancai", QString()));
        room->removeTag("IkMancaiTarget");
    }
}

void IkMancaiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (!effect.to->isKongcheng()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "ikmancai");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
        room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
    }
}

class IkMancaiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    IkMancaiViewAsSkill()
        : ZeroCardViewAsSkill("ikmancai")
    {
        response_pattern = "@@ikmancai";
    }

    virtual const Card *viewAs() const
    {
        return new IkMancaiCard;
    }
};

class IkMancai : public TriggerSkill
{
public:
    IkMancai()
        : TriggerSkill("ikmancai")
    {
        events << EventPhaseChanging;
        view_as_skill = new IkMancaiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(target) && target->canDiscard(target, "h")) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            switch (change.to) {
            case Player::RoundStart:
            case Player::Start:
            case Player::Finish:
            case Player::NotActive:
                return QStringList();

            case Player::Judge:
            case Player::Draw:
            case Player::Play:
            case Player::Discard:
                return QStringList(objectName());
            case Player::PhaseNone:
                Q_ASSERT(false);
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        room->setPlayerMark(zhanghe, "ikmancaiPhase", (int)change.to);
        int index = 0;
        switch (change.to) {
        case Player::RoundStart:
        case Player::Start:
        case Player::Finish:
        case Player::NotActive:
            return false;

        case Player::Judge:
            index = 1;
            break;
        case Player::Draw:
            index = 2;
            break;
        case Player::Play:
            index = 3;
            break;
        case Player::Discard:
            index = 4;
            break;
        case Player::PhaseNone:
            Q_ASSERT(false);
        }

        QString discard_prompt = QString("#ikmancai-%1").arg(index);
        if (index > 0 && room->askForDiscard(zhanghe, objectName(), 1, 1, true, false, discard_prompt)) {
            room->broadcastSkillInvoke("ikmancai", index);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        room->setPlayerMark(zhanghe, "ikmancaiPhase", (int)change.to);
        int index = 0;
        switch (change.to) {
        case Player::RoundStart:
        case Player::Start:
        case Player::Finish:
        case Player::NotActive:
            return false;

        case Player::Judge:
            index = 1;
            break;
        case Player::Draw:
            index = 2;
            break;
        case Player::Play:
            index = 3;
            break;
        case Player::Discard:
            index = 4;
            break;
        case Player::PhaseNone:
            Q_ASSERT(false);
        }

        QString use_prompt = QString("@ikmancai-%1").arg(index);
        if (!zhanghe->isAlive())
            return false;
        if (!zhanghe->isSkipped(change.to) && (index == 2 || index == 3))
            room->askForUseCard(zhanghe, "@@ikmancai", use_prompt, index);
        zhanghe->skip(change.to, true);
        return false;
    }
};

class IkKujie : public OneCardViewAsSkill
{
public:
    IkKujie()
        : OneCardViewAsSkill("ikkujie")
    {
        filter_pattern = "^TrickCard|black";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
        shortage->setSkillName(objectName());
        shortage->addSubcard(originalCard);

        return shortage;
    }
};

class IkKujieTargetMod : public TargetModSkill
{
public:
    IkKujieTargetMod()
        : TargetModSkill("#ikkujie-target")
    {
        pattern = "SupplyShortage";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasSkill("ikkujie"))
            return 1;
        else
            return 0;
    }
};

class IkZhaihun : public TriggerSkill
{
public:
    IkZhaihun()
        : TriggerSkill("ikzhaihun")
    {
        events << EventPhaseStart;
    }

    static int getWeaponCount(ServerPlayer *caoren)
    {
        int n = 0;
        foreach (ServerPlayer *p, caoren->getRoom()->getAlivePlayers()) {
            if (p->getWeapon())
                n++;
        }
        return n;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return (TriggerSkill::triggerable(target) && target->getPhase() == Player::Finish) || (target->getMark("@geek") > 0 && target->getPhase() == Player::Draw);
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caoren, QVariant &, ServerPlayer *) const
    {
        if (caoren->getPhase() == Player::Finish) {
            if (caoren->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
            return false;
        } else
            return true;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *caoren, QVariant &) const
    {
        if (caoren->getPhase() == Player::Finish) {
            int n = getWeaponCount(caoren);
            caoren->drawCards(n + 2, objectName());
            caoren->turnOver();
            room->setPlayerMark(caoren, "@geek", 1);
        } else {
            room->removePlayerMark(caoren, "@geek");
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

class IkFojiao : public OneCardViewAsSkill
{
public:
    IkFojiao()
        : OneCardViewAsSkill("ikfojiao")
    {
        filter_pattern = ".|.|.|equipped";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "nullification" && player->getHandcardNum() > player->getHp();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Nullification *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->getHandcardNum() > player->getHp() && !player->getEquips().isEmpty();
    }
};

IkQiangxiCard::IkQiangxiCard()
{
}

void IkQiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    if (subcards.isEmpty())
        room->loseHp(effect.from);

    room->damage(DamageStruct("ikqiangxi", effect.from, effect.to));
}

class IkQiangxi : public ViewAsSkill
{
public:
    IkQiangxi()
        : ViewAsSkill("ikqiangxi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkQiangxiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
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

IkYushenCard::IkYushenCard()
{
}

bool IkYushenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->getHp() > Self->getHp() && !to_select->isKongcheng();
}

void IkYushenCard::use(Room *room, ServerPlayer *xunyu, QList<ServerPlayer *> &targets) const
{
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

class IkYushen : public ZeroCardViewAsSkill
{
public:
    IkYushen()
        : ZeroCardViewAsSkill("ikyushen")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkYushenCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new IkYushenCard;
    }
};

class IkJieming : public MasochismSkill
{
public:
    IkJieming()
        : MasochismSkill("ikjieming")
    {
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *to = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "ikjieming-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkJiemingTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *xunyu, const DamageStruct &) const
    {
        ServerPlayer *to = xunyu->tag["IkJiemingTarget"].value<ServerPlayer *>();
        xunyu->tag.remove("IkJiemingTarget");
        if (to) {
            int upper = qMin(5, to->getMaxHp());
            int x = upper - to->getHandcardNum();
            if (x <= 0)
                return;
            to->drawCards(x, objectName());
        }
    }
};

class IkTanwan : public TriggerSkill
{
public:
    IkTanwan()
        : TriggerSkill("iktanwan")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *caopi, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(caopi))
            return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *player = death.who;
        if (player->isNude() || caopi == player)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &, ServerPlayer *) const
    {
        if (caopi->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &data, ServerPlayer *) const
    {
        DeathStruct death = data.value<DeathStruct>();
        DummyCard *dummy = new DummyCard(death.who->handCards());
        QList<const Card *> equips = death.who->getEquips();
        foreach (const Card *card, equips)
            dummy->addSubcard(card);

        if (dummy->subcardsLength() > 0) {
            room->obtainCard(caopi, dummy, false);
        }
        delete dummy;

        return false;
    }
};

class IkBisuo : public MasochismSkill
{
public:
    IkBisuo()
        : MasochismSkill("ikbisuo")
    {
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *caopi, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *to = room->askForPlayerChosen(caopi, room->getOtherPlayers(caopi), objectName(), "ikbisuo-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            caopi->tag["ThBisuoTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *caopi, const DamageStruct &) const
    {
        ServerPlayer *to = caopi->tag["ThBisuoTarget"].value<ServerPlayer *>();
        caopi->tag.remove("ThBisuoTarget");
        if (to) {
            if (caopi->isWounded())
                to->drawCards(caopi->getLostHp(), objectName());
            to->turnOver();
        }
    }
};

class IkSongwei : public TriggerSkill
{
public:
    IkSongwei()
        : TriggerSkill("iksongwei$")
    {
        events << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        QStringList skill_list;
        if (player->getKingdom() != "hana")
            return skill_list;
        JudgeStruct *judge = data.value<JudgeStruct *>();

        if (judge->card->isBlack()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill("iksongwei"))
                    skill_list << p->objectName() + "'" + objectName();
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *skill_invoker) const
    {
        if (skill_invoker->askForSkillInvoke(objectName(), QVariant::fromValue(skill_target))) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(skill_target, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = skill_invoker;
            log.to << skill_target;
            log.arg = objectName();
            room->sendLog(log);

            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *skill_target, QVariant &, ServerPlayer *) const
    {
        skill_target->drawCards(1, objectName());
        return false;
    }
};

class IkYindie : public TriggerSkill
{
public:
    IkYindie()
        : TriggerSkill("ikyindie")
    {
        events << CardsMoveOneTime << FinishJudge;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || (room->getCurrent() == player && player->getPhase() != Player::NotActive))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        return false;
    }
};

class IkYindieMove : public TriggerSkill
{
public:
    IkYindieMove()
        : TriggerSkill("#ikyindie-move")
    {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        player->addToPile("assassinate", judge->card->getEffectiveId());

        return false;
    }
};

class IkYindieDistance : public DistanceSkill
{
public:
    IkYindieDistance()
        : DistanceSkill("#ikyindie-dist")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill("ikyindie"))
            return -from->getPile("assassinate").length();
        else
            return 0;
    }
};

class IkGuiyue : public PhaseChangeSkill
{
public:
    IkGuiyue()
        : PhaseChangeSkill("ikguiyue")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start && target->getMark("@guiyue") == 0 && target->getPile("assassinate").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *dengai) const
    {
        Room *room = dengai->getRoom();
        room->notifySkillInvoked(dengai, objectName());

        LogMessage log;
        log.type = "#IkGuiyueWake";
        log.from = dengai;
        log.arg = QString::number(dengai->getPile("assassinate").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(dengai, "@guiyue", 1);
        if (room->changeMaxHpForAwakenSkill(dengai))
            room->acquireSkill(dengai, "ikhuanwu");

        return false;
    }
};

class IkHuanwu : public OneCardViewAsSkill
{
public:
    IkHuanwu()
        : OneCardViewAsSkill("ikhuanwu")
    {
        expand_pile = "assassinate";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("assassinate").isEmpty();
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return Self->getPile("assassinate").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Snatch *snatch = new Snatch(originalCard->getSuit(), originalCard->getNumber());
        snatch->addSubcard(originalCard);
        snatch->setSkillName(objectName());
        return snatch;
    }
};

IkYihuoCard::IkYihuoCard()
{
    will_throw = false;
    handling_method = MethodNone;
    m_skillName = "ikyihuov";
    mute = true;
}

bool IkYihuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasSkill("ikyihuo") && (to_select->hasEquip() || !to_select->faceUp()) && !to_select->hasFlag("IkYihuoInvoked");
}

void IkYihuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
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

class IkYihuoViewAsSkill : public OneCardViewAsSkill
{
public:
    IkYihuoViewAsSkill()
        : OneCardViewAsSkill("ikyihuov")
    {
        attached_lord_skill = true;
        filter_pattern = ".|.|.|hand";
    }

    virtual bool shouldBeVisible(const Player *) const
    {
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IkYihuoCard *card = new IkYihuoCard();
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("ForbidIkYihuo");
    }
};

class IkYihuo : public TriggerSkill
{
public:
    IkYihuo()
        : TriggerSkill("ikyihuo")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if ((triggerEvent == GameStart) || (triggerEvent == EventAcquireSkill && data.toString() == "ikyihuo")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty())
                return QStringList();

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
            if (lords.length() > 2)
                return QStringList();

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
            if (phase_change.from != Player::Play && phase_change.to != Player::Play)
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

class IkGuixin : public MasochismSkill
{
public:
    IkGuixin()
        : MasochismSkill("ikguixin")
    {
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return MasochismSkill::triggerable(target) && (target->aliveCount() < 4 || target->faceUp());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *shencc, const DamageStruct &) const
    {
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

class IkRenjia : public TriggerSkill
{
public:
    IkRenjia()
        : TriggerSkill("ikrenjia")
    {
        events << Damaged << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
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

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

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
        player->gainMark("@fetter", n);
        return false;
    }
};

class IkTiangai : public PhaseChangeSkill
{
public:
    IkTiangai()
        : PhaseChangeSkill("iktiangai")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start && target->getMark("@tiangai") == 0 && target->getMark("@fetter") >= 4;
    }

    virtual bool onPhaseChange(ServerPlayer *shensimayi) const
    {
        Room *room = shensimayi->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(shensimayi, objectName());

        LogMessage log;
        log.type = "#IkTiangaiWake";
        log.from = shensimayi;
        log.arg = QString::number(shensimayi->getMark("@fetter"));
        room->sendLog(log);

        room->setPlayerMark(shensimayi, "@tiangai", 1);
        if (room->changeMaxHpForAwakenSkill(shensimayi))
            room->acquireSkill(shensimayi, "ikjilve");

        return false;
    }
};

IkJilveCard::IkJilveCard()
{
    target_fixed = true;
    mute = true;
}

void IkJilveCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *shensimayi = card_use.from;
    shensimayi->loseMark("@fetter");

    room->setPlayerFlag(shensimayi, "IkJilve");
    room->acquireSkill(shensimayi, "iksishideng");
}

class IkJilveViewAsSkill : public ZeroCardViewAsSkill
{
public: // iksishideng
    IkJilveViewAsSkill()
        : ZeroCardViewAsSkill("ikjilve")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkJilveCard") && player->getMark("@fetter") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new IkJilveCard;
    }
};

class IkJilve : public TriggerSkill
{
public:
    IkJilve()
        : TriggerSkill("ikjilve")
    {
        events << CardUsed // IkHuiquan
               << EventPhaseStart // IkXushi
               << CardsMoveOneTime; // ThXijing
        view_as_skill = new IkJilveViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getMark("@fetter") == 0)
            return QStringList();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->getTypeId() == Card::TypeTrick)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Start)
                return QStringList(objectName());
        } else if (triggerEvent == CardsMoveOneTime) {
            if ((room->getCurrent() == player && player->getPhase() != Player::NotActive) || player->isKongcheng() || player->hasFlag("thxijing_using"))
                return QStringList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player || move.to_place != Player::DiscardPile)
                return QStringList();

            for (int i = 0; i < move.card_ids.length(); i++) {
                int id = move.card_ids[i];
                if (move.from_places[i] != Player::PlaceJudge && move.from_places[i] != Player::PlaceSpecial && !Sanguosha->getCard(id)->isKindOf("EquipCard")
                    && room->getCardPlace(id) == Player::DiscardPile) {
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == CardUsed) {
            const TriggerSkill *jizhi = Sanguosha->getTriggerSkill("ikhuiquan");
            if (jizhi && jizhi->cost(triggerEvent, room, player, data, player)) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                player->loseMark("@fetter");
                return true;
            }
        } else if (triggerEvent == EventPhaseStart) {
            const TriggerSkill *guanxing = Sanguosha->getTriggerSkill("ikxushi");
            if (guanxing && guanxing->cost(triggerEvent, room, player, data, player)) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                player->loseMark("@fetter");
                return true;
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            for (int i = 0; i < move.card_ids.length(); i++) {
                int id = move.card_ids[i];
                if (move.from_places[i] != Player::PlaceJudge && move.from_places[i] != Player::PlaceSpecial && !Sanguosha->getEngineCard(id)->isKindOf("EquipCard")
                    && room->getCardPlace(id) == Player::DiscardPile) {
                    const Card *c = Sanguosha->getEngineCard(id);
                    QString prompt = "@thxijing:" + c->getSuitString() + ":" + QString::number(c->getNumber()) + ":" + c->objectName();
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

                        player->loseMark("@fetter");
                        room->setPlayerFlag(player, "thxijing_using");
                        CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
                        room->throwCard(card, reason, player);
                        room->setPlayerFlag(player, "-thxijing_using");
                        room->obtainCard(player, Sanguosha->getCard(id));
                    }
                }
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == CardUsed) {
            const TriggerSkill *jizhi = Sanguosha->getTriggerSkill("ikhuiquan");
            if (jizhi)
                return jizhi->effect(triggerEvent, room, player, data, player);
        } else if (triggerEvent == EventPhaseStart) {
            const TriggerSkill *guanxing = Sanguosha->getTriggerSkill("ikxushi");
            if (guanxing)
                return guanxing->effect(triggerEvent, room, player, data, player);
        }

        return false;
    }
};

class IkJilveClear : public TriggerSkill
{
public:
    IkJilveClear()
        : TriggerSkill("#ikjilve-clear")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
    {
        if (!target->hasFlag("IkJilve"))
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return QStringList();
        room->detachSkillFromPlayer(target, "iksishideng", false, true);
        return QStringList();
    }
};

class IkLiangban : public PhaseChangeSkill
{
public:
    IkLiangban()
        : PhaseChangeSkill("ikliangban")
    {
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *to = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "ikliangban-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkLiangbanTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *sunjian) const
    {
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

IkZhuoshiCard::IkZhuoshiCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
    m_skillName = "_ikzhuoshi";
}

bool IkZhuoshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    return to_select->getHandcardNum() == Self->getMark("ikzhuoshi");
}

void IkZhuoshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), targets.first()->objectName(), "ikzhuoshi", QString());
    room->moveCardTo(this, targets.first(), Player::PlaceHand, reason);
}

class IkZhuoshiViewAsSkill : public ViewAsSkill
{
public:
    IkZhuoshiViewAsSkill()
        : ViewAsSkill("ikzhuoshi")
    {
        response_pattern = "@@ikzhuoshi!";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;

        int length = Self->getHandcardNum() / 2;
        return selected.length() < length;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != Self->getHandcardNum() / 2)
            return NULL;

        IkZhuoshiCard *card = new IkZhuoshiCard;
        card->addSubcards(cards);
        return card;
    }
};

class IkZhuoshi : public DrawCardsSkill
{
public:
    IkZhuoshi()
        : DrawCardsSkill("ikzhuoshi")
    {
        view_as_skill = new IkZhuoshiViewAsSkill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual int getDrawNum(ServerPlayer *lusu, int n) const
    {
        lusu->setFlags("ikzhuoshi");
        return n + 2;
    }
};

class IkZhuoshiGive : public TriggerSkill
{
public:
    IkZhuoshiGive()
        : TriggerSkill("#ikzhuoshi-give")
    {
        events << AfterDrawNCards;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *lusu) const
    {
        return lusu && lusu->hasFlag("ikzhuoshi") && lusu->getHandcardNum() > 5;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *lusu, QVariant &, ServerPlayer *) const
    {
        lusu->setFlags("-ikzhuoshi");

        QList<ServerPlayer *> other_players = room->getOtherPlayers(lusu);
        int least = 1000;
        foreach (ServerPlayer *player, other_players)
            least = qMin(player->getHandcardNum(), least);
        room->setPlayerMark(lusu, "ikzhuoshi", least);
        bool used = room->askForUseCard(lusu, "@@ikzhuoshi!", "@ikzhuoshi", -1, Card::MethodNone);

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
            IkZhuoshiCard *ikzhuoshi_card = new IkZhuoshiCard;
            ikzhuoshi_card->addSubcards(to_give);
            QList<ServerPlayer *> targets;
            targets << beggar;
            ikzhuoshi_card->use(room, lusu, targets);
            delete ikzhuoshi_card;
        }

        return false;
    }
};

IkYuanjieCard::IkYuanjieCard()
{
}

bool IkYuanjieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self)
        return false;

    if (targets.isEmpty())
        return true;

    if (targets.length() == 1) {
        return qAbs(to_select->getHandcardNum() - targets.first()->getHandcardNum()) == subcardsLength();
    }

    return false;
}

bool IkYuanjieCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void IkYuanjieCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *a = targets.at(0);
    ServerPlayer *b = targets.at(1);
    a->setFlags("IkYuanjieTarget");
    b->setFlags("IkYuanjieTarget");

    int n1 = a->getHandcardNum();
    int n2 = b->getHandcardNum();

    try {
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p != a && p != b) {
                JsonArray args;
                args << a->objectName() << b->objectName();
                room->doNotify(p, QSanProtocol::S_COMMAND_EXCHANGE_KNOWN_CARDS, args);
            }
        }
        QList<CardsMoveStruct> exchangeMove;
        CardsMoveStruct move1(a->handCards(), b, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_SWAP, a->objectName(), b->objectName(), "ikyuanjie", QString()));
        CardsMoveStruct move2(b->handCards(), a, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_SWAP, b->objectName(), a->objectName(), "ikyuanjie", QString()));
        exchangeMove.push_back(move1);
        exchangeMove.push_back(move2);
        room->moveCardsAtomic(exchangeMove, false);

        LogMessage log;
        log.type = "#IkYuanjie";
        log.from = a;
        log.to << b;
        log.arg = QString::number(n1);
        log.arg2 = QString::number(n2);
        room->sendLog(log);
        room->getThread()->delay();

        a->setFlags("-IkYuanjieTarget");
        b->setFlags("-IkYuanjieTarget");
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange) {
            a->setFlags("-IkYuanjieTarget");
            b->setFlags("-IkYuanjieTarget");
        }
        throw triggerEvent;
    }
}

class IkYuanjie : public ViewAsSkill
{
public:
    IkYuanjie()
        : ViewAsSkill("ikyuanjie")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.size() < 3 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        IkYuanjieCard *card = new IkYuanjieCard;
        foreach (const Card *c, cards)
            card->addSubcard(c);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkYuanjieCard");
    }
};

IkZhihuiCard::IkZhihuiCard()
{
}

void IkZhihuiCard::onEffect(const CardEffectStruct &effect) const
{
    DamageStruct damage = effect.from->tag.value("IkZhihuiDamage").value<DamageStruct>();
    damage.to = effect.to;
    damage.transfer = true;
    damage.transfer_reason = "ikzhihui";
    effect.from->tag["TransferDamage"] = QVariant::fromValue(damage);
}

class IkZhihuiViewAsSkill : public OneCardViewAsSkill
{
public:
    IkZhihuiViewAsSkill()
        : OneCardViewAsSkill("ikzhihui")
    {
        filter_pattern = ".|heart|.|hand!";
        response_pattern = "@@ikzhihui";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IkZhihuiCard *ikzhihuiCard = new IkZhihuiCard;
        ikzhihuiCard->addSubcard(originalCard);
        return ikzhihuiCard;
    }
};

class IkZhihui : public TriggerSkill
{
public:
    IkZhihui()
        : TriggerSkill("ikzhihui")
    {
        events << DamageInflicted;
        view_as_skill = new IkZhihuiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *xiaoqiao) const
    {
        return TriggerSkill::triggerable(xiaoqiao) && xiaoqiao->canDiscard(xiaoqiao, "h");
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data, ServerPlayer *) const
    {
        xiaoqiao->tag["IkZhihuiDamage"] = data;
        return room->askForUseCard(xiaoqiao, "@@ikzhihui", "@ikzhihui-card", -1, Card::MethodDiscard);
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return true;
    }
};

class IkZhihuiDraw : public TriggerSkill
{
public:
    IkZhihuiDraw()
        : TriggerSkill("#ikzhihui")
    {
        events << DamageComplete;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (player->isAlive() && damage.transfer && damage.transfer_reason == "ikzhihui")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(player->getLostHp(), objectName());
        return false;
    }
};

class IkChiqiu : public FilterSkill
{
public:
    IkChiqiu()
        : FilterSkill("ikchiqiu")
    {
    }

    static WrappedCard *changeToHeart(int cardId)
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("ikchiqiu");
        new_card->setSuit(Card::Heart);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->getSuit() == Card::Spade;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        return changeToHeart(originalCard->getEffectiveId());
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return -2;
    }
};

IkJianlveCard::IkJianlveCard()
{
}

bool IkJianlveCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void IkJianlveCard::use(Room *room, ServerPlayer *taishici, QList<ServerPlayer *> &targets) const
{
    bool success = taishici->pindian(targets.first(), "ikjianlve", NULL);
    if (success) {
        room->setPlayerFlag(taishici, "IkJianlveSuccess");
        room->acquireSkill(taishici, "ikkongyun");
    } else {
        room->setPlayerCardLimitation(taishici, "use", "Slash", true);
    }
}

class IkJianlveViewAsSkill : public ZeroCardViewAsSkill
{
public:
    IkJianlveViewAsSkill()
        : ZeroCardViewAsSkill("ikjianlve")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkJianlveCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new IkJianlveCard;
    }
};

class IkJianlve : public TriggerSkill
{
public:
    IkJianlve()
        : TriggerSkill("ikjianlve")
    {
        events << EventPhaseChanging;
        view_as_skill = new IkJianlveViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!player->hasFlag("IkJianlveSuccess"))
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            room->setPlayerFlag(player, "-IkJianlveSuccess");
            room->detachSkillFromPlayer(player, "ikkongyun");
        }
        return QStringList();
    }
};

class IkKongyun : public TargetModSkill
{
public:
    IkKongyun()
        : TargetModSkill("ikkongyun")
    {
        frequency = NotCompulsory;
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(objectName()))
            return 1;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(objectName()))
            return 1;
        else
            return 0;
    }
};

class IkSusheng : public TriggerSkill
{
public:
    IkSusheng()
        : TriggerSkill("iksusheng")
    {
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != zhoutai)
            return false;

        if (zhoutai->getHp() > 0)
            return false;
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(zhoutai, objectName());

        int id = room->drawCard();
        int num = Sanguosha->getCard(id)->getNumber();
        bool duplicate = false;
        foreach (int card_id, zhoutai->getPile("flower")) {
            if (Sanguosha->getCard(card_id)->getNumber() == num) {
                duplicate = true;
                break;
            }
        }
        zhoutai->addToPile("flower", id);
        if (duplicate) {
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
            room->throwCard(Sanguosha->getCard(id), reason, NULL);
        } else {
            room->recover(zhoutai, RecoverStruct(zhoutai, NULL, 1 - zhoutai->getHp()));
        }
        return false;
    }
};

class IkSushengMaxCards : public MaxCardsSkill
{
public:
    IkSushengMaxCards()
        : MaxCardsSkill("#iksusheng")
    {
    }

    virtual int getFixed(const Player *target) const
    {
        int len = target->getPile("flower").length();
        if (target->hasSkill(objectName()) && len > 0)
            return len;
        else
            return -1;
    }
};

class IkHuapan : public TriggerSkill
{
public:
    IkHuapan()
        : TriggerSkill("ikhuapan")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getHp() > 0 && move.from && move.from->isAlive() && move.from_places.contains(Player::PlaceHand)
            && ((move.reason.m_reason == CardMoveReason::S_REASON_DISMANTLE && move.reason.m_playerId != move.reason.m_targetId)
                || (move.to && move.to != move.from && move.to_place == Player::PlaceHand && move.reason.m_reason != CardMoveReason::S_REASON_GIVE
                    && move.reason.m_reason != CardMoveReason::S_REASON_SWAP)))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        move.from->setFlags("IkHuapanMoveFrom"); // For AI
        bool invoke = player->askForSkillInvoke(objectName(), data);
        move.from->setFlags("-IkHuapanMoveFrom");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->loseHp(player);
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from->isAlive())
            room->drawCards((ServerPlayer *)move.from, 2, objectName());
        return false;
    }
};

class IkHeyi : public TriggerSkill
{
public:
    IkHeyi()
        : TriggerSkill("ikheyi")
    {
        events << TargetSpecified << TargetConfirmed;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *sunce, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(sunce))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified || (triggerEvent == TargetConfirmed && use.to.contains(sunce)))
            if (use.card->isKindOf("Duel") || (use.card->isKindOf("Slash") && use.card->isRed()))
                return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *sunce, QVariant &, ServerPlayer *) const
    {
        if (sunce->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *sunce, QVariant &, ServerPlayer *) const
    {
        sunce->drawCards(1, objectName());
        return false;
    }
};

class IkChizhu : public PhaseChangeSkill
{
public:
    IkChizhu()
        : PhaseChangeSkill("ikchizhu")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getMark("@chizhu") == 0 && target->getPhase() == Player::Start && target->getHp() == 1;
    }

    virtual bool onPhaseChange(ServerPlayer *sunce) const
    {
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
        if (room->changeMaxHpForAwakenSkill(sunce)) {
            if (sunce->isWounded() && room->askForChoice(sunce, objectName(), "recover+draw") == "recover")
                room->recover(sunce, RecoverStruct(sunce));
            else
                sunce->drawCards(2, objectName());
            room->handleAcquireDetachSkills(sunce, "ikchenhong|ikliangban");
        }
        return false;
    }
};

IkBianshengCard::IkBianshengCard()
{
    mute = true;
    m_skillName = "ikbiansheng_pindian";
}

bool IkBianshengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("ikbiansheng") && to_select != Self && !to_select->isKongcheng() && !to_select->hasFlag("IkBianshengInvoked");
}

void IkBianshengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
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

class IkBianshengPindian : public ZeroCardViewAsSkill
{
public:
    IkBianshengPindian()
        : ZeroCardViewAsSkill("ikbiansheng_pindian")
    {
        attached_lord_skill = true;
    }

    virtual bool shouldBeVisible(const Player *player) const
    {
        return player && player->getKingdom() == "yuki";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return shouldBeVisible(player) && !player->isKongcheng() && !player->hasFlag("ForbidIkBiansheng");
    }

    virtual const Card *viewAs() const
    {
        return new IkBianshengCard;
    }
};

class IkBiansheng : public TriggerSkill
{
public:
    IkBiansheng()
        : TriggerSkill("ikbiansheng$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Pindian << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&ask_who) const
    {
        if (player == NULL)
            return QStringList();
        if ((triggerEvent == GameStart && player->isLord()) || (triggerEvent == EventAcquireSkill && data.toString() == "ikbiansheng")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty())
                return QStringList();

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
            if (lords.length() > 2)
                return QStringList();

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
            if (phase_change.from != Player::Play && phase_change.to != Player::Play)
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

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        return ask_who->askForSkillInvoke(objectName(), "pindian");
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (room->getCardPlace(pindian->from_card->getEffectiveId()) == Player::PlaceTable)
            ask_who->obtainCard(pindian->from_card);
        if (room->getCardPlace(pindian->to_card->getEffectiveId()) == Player::PlaceTable)
            ask_who->obtainCard(pindian->to_card);
        return false;
    }
};

IkJibanCard::IkJibanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkJibanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void IkJibanCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *erzhang = effect.from;
    Room *room = erzhang->getRoom();
    room->moveCardTo(this, erzhang, effect.to, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_PUT, erzhang->objectName(), "ikjiban", QString()));

    LogMessage log;
    log.type = "$IkJibanEquip";
    log.from = effect.to;
    log.card_str = QString::number(getEffectiveId());
    room->sendLog(log);

    erzhang->drawCards(1, "ikjiban");
}

class IkJiban : public OneCardViewAsSkill
{
public:
    IkJiban()
        : OneCardViewAsSkill("ikjiban")
    {
        filter_pattern = "EquipCard|.|.|hand";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IkJibanCard *ikjiban_card = new IkJibanCard();
        ikjiban_card->addSubcard(originalCard);
        return ikjiban_card;
    }
};

class IkJizhou : public TriggerSkill
{
public:
    IkJizhou()
        : TriggerSkill("ikjizhou")
    {
        events << EventPhaseEnd << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard)
                foreach (ServerPlayer *erzhang, room->findPlayersBySkillName(objectName())) {
                    erzhang->tag.remove("IkJizhouToGet");
                    erzhang->tag.remove("IkJizhouOther");
                }
        } else if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() != Player::Discard)
                return skill_list;
            QList<ServerPlayer *> erzhangs = room->findPlayersBySkillName(objectName());

            foreach (ServerPlayer *erzhang, erzhangs) {
                QVariantList ikjizhou_cardsToGet = erzhang->tag["IkJizhouToGet"].toList();
                foreach (QVariant card_data, ikjizhou_cardsToGet) {
                    int card_id = card_data.toInt();
                    if (room->getCardPlace(card_id) == Player::DiscardPile)
                        skill_list.insert(erzhang, QStringList(objectName()));
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *erzhang) const
    {
        if (erzhang->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *erzhang) const
    {
        QVariantList ikjizhou_cardsToGet = erzhang->tag["IkJizhouToGet"].toList();
        QVariantList ikjizhou_cardsOther = erzhang->tag["IkJizhouOther"].toList();
        erzhang->tag.remove("IkJizhouToGet");
        erzhang->tag.remove("IkJizhouOther");

        QList<int> cardsToGet;
        foreach (QVariant card_data, ikjizhou_cardsToGet) {
            int card_id = card_data.toInt();
            if (room->getCardPlace(card_id) == Player::DiscardPile)
                cardsToGet << card_id;
        }
        QList<int> cardsOther;
        foreach (QVariant card_data, ikjizhou_cardsOther) {
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

class IkJizhouRecord : public TriggerSkill
{
public:
    IkJizhouRecord()
        : TriggerSkill("#ikjizhou-record")
    {
        events << CardsMoveOneTime;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *erzhang, QVariant &data, ServerPlayer *&) const
    {
        if (!erzhang || !erzhang->isAlive() || !erzhang->hasSkill("ikjizhou"))
            return QStringList();
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

static bool CompareBySuit(int card1, int card2)
{
    const Card *c1 = Sanguosha->getCard(card1);
    const Card *c2 = Sanguosha->getCard(card2);

    int a = static_cast<int>(c1->getSuit());
    int b = static_cast<int>(c2->getSuit());

    return a < b;
}

class IkLvejue : public PhaseChangeSkill
{
public:
    IkLvejue()
        : PhaseChangeSkill("iklvejue")
    {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Draw;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *shenlvmeng) const
    {
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

IkLingshiCard::IkLingshiCard()
{
}

bool IkLingshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void IkLingshiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (!effect.to->isKongcheng()) {
        QList<int> ids;
        foreach (const Card *card, effect.to->getHandcards()) {
            if (card->getSuit() == Card::Heart)
                ids << card->getEffectiveId();
        }

        int card_id = room->doGongxin(effect.from, effect.to, ids);
        if (card_id == -1)
            return;

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

class IkLingshi : public ZeroCardViewAsSkill
{
public:
    IkLingshi()
        : ZeroCardViewAsSkill("iklingshi")
    {
    }

    virtual const Card *viewAs() const
    {
        return new IkLingshiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkLingshiCard");
    }
};

class IkLongxi : public TriggerSkill
{
public:
    IkLongxi()
        : TriggerSkill("iklongxi")
    {
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Discard && target->getMark("iklongxi") >= 2;
    }

    void perform(ServerPlayer *shenzhouyu) const
    {
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
        perform(player);
        return false;
    }
};

class IkLongxiRecord : public TriggerSkill
{
public:
    IkLongxiRecord()
        : TriggerSkill("#iklongxi-record")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *shenzhouyu, QVariant &data, ServerPlayer *&) const
    {
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *shenzhouyu, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        shenzhouyu->addMark("iklongxi", move.card_ids.size());
        return false;
    }
};

void IkYeyanCard::damage(ServerPlayer *shenzhouyu, ServerPlayer *target, int point) const
{
    shenzhouyu->getRoom()->damage(DamageStruct("ikyeyan", shenzhouyu, target, point, DamageStruct::Fire));
}

GreatIkYeyanCard::GreatIkYeyanCard()
{
    mute = true;
    m_skillName = "ikyeyan";
}

bool GreatIkYeyanCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const
{
    Q_ASSERT(false);
    return false;
}

bool GreatIkYeyanCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    if (subcards.length() != 4)
        return false;
    QList<Card::Suit> allsuits;
    foreach (int cardId, subcards) {
        const Card *card = Sanguosha->getCard(cardId);
        if (allsuits.contains(card->getSuit()))
            return false;
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

bool GreatIkYeyanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *, int &maxVotes) const
{
    int i = 0;
    foreach (const Player *player, targets)
        if (player == to_select)
            i++;
    maxVotes = qMax(3 - targets.size(), 0) + i;
    return maxVotes > 0;
}

void GreatIkYeyanCard::use(Room *room, ServerPlayer *shenzhouyu, QList<ServerPlayer *> &targets) const
{
    int criticaltarget = 0;
    int totalvictim = 0;
    QMap<ServerPlayer *, int> map;

    foreach (ServerPlayer *sp, targets)
        map[sp]++;

    if (targets.size() == 1)
        map[targets.first()] += 2;

    foreach (ServerPlayer *sp, map.keys()) {
        if (map[sp] > 1)
            criticaltarget++;
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

SmallIkYeyanCard::SmallIkYeyanCard()
{
    mute = true;
    m_skillName = "ikyeyan";
}

bool SmallIkYeyanCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return !targets.isEmpty();
}

bool SmallIkYeyanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.length() < 3;
}

void SmallIkYeyanCard::use(Room *room, ServerPlayer *shenzhouyu, QList<ServerPlayer *> &targets) const
{
    room->broadcastSkillInvoke("ikyeyan", 3);
    room->removePlayerMark(shenzhouyu, "@yeyan");
    room->addPlayerMark(shenzhouyu, "@yeyanused");
    Card::use(room, shenzhouyu, targets);
}

void SmallIkYeyanCard::onEffect(const CardEffectStruct &effect) const
{
    damage(effect.from, effect.to, 1);
}

class IkYeyan : public ViewAsSkill
{
public:
    IkYeyan()
        : ViewAsSkill("ikyeyan")
    {
        frequency = Limited;
        limit_mark = "@yeyan";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@yeyan") >= 1;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
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

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return new SmallIkYeyanCard;
        if (cards.length() != 4)
            return NULL;

        GreatIkYeyanCard *card = new GreatIkYeyanCard;
        card->addSubcards(cards);

        return card;
    }
};

class IkFusheng : public OneCardViewAsSkill
{
public:
    IkFusheng()
        : OneCardViewAsSkill("ikfusheng")
    {
        filter_pattern = ".|spade";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.contains("analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());
        return analeptic;
    }
};

class IkHuanbei : public TriggerSkill
{
public:
    IkHuanbei()
        : TriggerSkill("ikhuanbei")
    {
        events << TargetConfirmed << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
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

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
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

class IkBenghuai : public PhaseChangeSkill
{
public:
    IkBenghuai()
        : PhaseChangeSkill("ikbenghuai")
    {
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish)
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (player->getHp() > p->getHp())
                    return QStringList(objectName());
        return QStringList();
    }

    virtual bool onPhaseChange(ServerPlayer *dongzhuo) const
    {
        Room *room = dongzhuo->getRoom();
        room->sendCompulsoryTriggerLog(dongzhuo, objectName());

        QString result = room->askForChoice(dongzhuo, "ikbenghuai", "hp+maxhp");
        room->broadcastSkillInvoke(objectName());
        if (result == "hp")
            room->loseHp(dongzhuo);
        else
            room->loseMaxHp(dongzhuo);

        return false;
    }
};

class IkWuhua : public TriggerSkill
{
public:
    IkWuhua()
        : TriggerSkill("ikwuhua$")
    {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        QStringList skill_list;
        if (player->tag.value("InvokeIkWuhua", false).toBool()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill("ikwuhua"))
                    skill_list << p->objectName() + "'" + objectName();
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *skill_invoker) const
    {
        if (skill_invoker->askForSkillInvoke(objectName(), QVariant::fromValue(skill_target))) {
            room->notifySkillInvoked(skill_target, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = skill_invoker;
            log.to << skill_target;
            log.arg = objectName();
            room->sendLog(log);

            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *skill_invoker) const
    {
        JudgeStruct judge;
        judge.pattern = ".|spade";
        judge.good = true;
        judge.reason = objectName();
        judge.who = skill_invoker;

        room->judge(judge);

        if (judge.isGood()) {
            room->broadcastSkillInvoke(objectName());
            room->recover(skill_target, RecoverStruct(skill_invoker));
        }
        return false;
    }
};

class IkWuhuaRecord : public TriggerSkill
{
public:
    IkWuhuaRecord()
        : TriggerSkill("#ikwuhua-record")
    {
        events << PreDamageDone;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *qun = damage.from;
        if (qun)
            qun->tag["InvokeIkWuhua"] = qun->getKingdom() == "tsuki";
        return QStringList();
    }
};

class IkXinghuang : public ViewAsSkill
{
public:
    IkXinghuang()
        : ViewAsSkill("ikxinghuang")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.isEmpty())
            return !to_select->isEquipped();
        else if (selected.length() == 1) {
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ArcheryAttack *aa = new ArcheryAttack(Card::SuitToBeDecided, 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        } else
            return NULL;
    }
};

IkXuzhaoCard::IkXuzhaoCard()
{
    will_throw = false;
}

bool IkXuzhaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    LureTiger *lure_tiger = new LureTiger(SuitToBeDecided, -1);
    lure_tiger->addSubcards(subcards);
    lure_tiger->deleteLater();
    return lure_tiger->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, lure_tiger);
}

const Card *IkXuzhaoCard::validate(CardUseStruct &use) const
{
    Room *room = use.from->getRoom();
    room->loseHp(use.from);
    if (use.from->isDead())
        return NULL;
    LureTiger *lure_tiger = new LureTiger(SuitToBeDecided, -1);
    lure_tiger->addSubcards(subcards);
    lure_tiger->setSkillName("ikxuzhao");
    return lure_tiger;
}

class IkXuzhao : public OneCardViewAsSkill
{
public:
    IkXuzhao()
        : OneCardViewAsSkill("ikxuzhao")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *card) const
    {
        LureTiger *lure_tiger = new LureTiger(Card::SuitToBeDecided, -1);
        lure_tiger->addSubcard(card);
        lure_tiger->setSkillName(objectName());
        lure_tiger->deleteLater();
        if (lure_tiger->isAvailable(Self)) {
            IkXuzhaoCard *skill = new IkXuzhaoCard;
            skill->addSubcard(card);
            return skill;
        }
        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkXuzhaoCard");
    }
};

class IkJingfa : public TriggerSkill
{
public:
    IkJingfa()
        : TriggerSkill("ikjingfa")
    {
        events << PreDamageDone << EventPhaseEnd;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->getPhase() == Player::Play)
                damage.from->setFlags("IkJingfaDamageInPlayPhase");
        } else if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play) {
            if (!player->hasFlag("IkJingfaDamageInPlayPhase"))
                return QStringList(objectName());
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
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (player->canDiscard(p, "ej"))
                targets << p;
        }
        ServerPlayer *target = NULL;
        if (!targets.isEmpty())
            target = room->askForPlayerChosen(player, targets, objectName(), "@ikjingfa", true);

        if (target) {
            int card_id = room->askForCardChosen(player, target, "ej", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : target, player);
        } else
            player->drawCards(1, objectName());

        return false;
    }
};

class IkQiyuViewAsSkill : public OneCardViewAsSkill
{
public:
    IkQiyuViewAsSkill()
        : OneCardViewAsSkill("ikqiyu")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("ikqiyu") != 0 && !player->isKongcheng();
    }

    virtual bool viewFilter(const Card *card) const
    {
        if (card->isEquipped())
            return false;

        int value = Self->getMark("ikqiyu");
        if (value == 1)
            return card->isBlack();
        else if (value == 2)
            return card->isRed();

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Duel *duel = new Duel(originalCard->getSuit(), originalCard->getNumber());
        duel->addSubcard(originalCard);
        duel->setSkillName(objectName());
        return duel;
    }
};

class IkQiyu : public TriggerSkill
{
public:
    IkQiyu()
        : TriggerSkill("ikqiyu")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new IkQiyuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Start)
                room->setPlayerMark(player, "ikqiyu", 0);
            else if (player->getPhase() == Player::Draw && TriggerSkill::triggerable(player))
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->hasFlag("ikqiyu"))
                room->setPlayerFlag(player, "-ikqiyu");
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke("ikqiyu", 1);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->setPlayerFlag(player, "ikqiyu");
        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.reason = objectName();
        judge.pattern = ".";
        judge.who = player;

        room->judge(judge);
        room->setPlayerMark(player, "ikqiyu", judge.pattern == "red" ? 1 : 2);

        return true;
    }
};

class IkQiyuGet : public TriggerSkill
{
public:
    IkQiyuGet()
        : TriggerSkill("#ikqiyu")
    {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player != NULL) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "ikqiyu") {
                judge->pattern = (judge->card->isRed() ? "red" : "black");
                if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        judge->who->obtainCard(judge->card);

        return false;
    }
};

class IkSishideng : public TriggerSkill
{
public:
    IkSishideng()
        : TriggerSkill("iksishideng")
    {
        // just to broadcast audio effects and to send log messages
        // main part in the AskForPeaches trigger of Game Rule
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player && player == room->getAllPlayers().first()) {
            ServerPlayer *jiaxu = room->getCurrent();
            if (jiaxu && TriggerSkill::triggerable(jiaxu) && jiaxu->getPhase() != Player::NotActive)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 7;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
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

class IkWenle : public ZeroCardViewAsSkill
{
public:
    IkWenle()
        : ZeroCardViewAsSkill("ikwenle")
    {
        frequency = Limited;
        limit_mark = "@wenle";
    }

    virtual const Card *viewAs() const
    {
        return new IkWenleCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@wenle") >= 1;
    }
};

IkWenleCard::IkWenleCard()
{
    target_fixed = true;
}

void IkWenleCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
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

void IkWenleCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    QList<ServerPlayer *> players = room->getOtherPlayers(effect.to);
    QList<int> distance_list;
    int nearest = 1000;
    foreach (ServerPlayer *player, players) {
        int distance = effect.to->distanceTo(player);
        distance_list << distance;
        if (distance != -1)
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

class IkMoyu : public ProhibitSkill
{
public:
    IkMoyu()
        : ProhibitSkill("ikmoyu")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(objectName()) && (card->isKindOf("TrickCard") || card->isKindOf("IkMiceCard")) && card->isBlack()
            && card->getSkillName() != "ikguihuo"; // Be care!!!!!!
    }
};

class IkKongsa : public TriggerSkill
{
public:
    IkKongsa()
        : TriggerSkill("ikkongsa")
    {
        events << SlashMissed << Damage;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.to->isAlive() && player->canDiscard(effect.to, "he"))
                return QStringList(objectName());
        } else if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.transfer && !damage.chain && damage.card && damage.card->isKindOf("Slash") && damage.card->isRed())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *pangde, QVariant &, ServerPlayer *) const
    {
        if (pangde->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *pangde, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            int to_throw = room->askForCardChosen(pangde, effect.to, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(Sanguosha->getCard(to_throw), effect.to, pangde);
        } else
            pangde->drawCards(1);

        return false;
    }
};

class IkHuanshen : public PhaseChangeSkill
{
public:
    IkHuanshen()
        : PhaseChangeSkill("ikhuanshen")
    {
        frequency = Frequent;
    }

    static void playAudioEffect(ServerPlayer *zuoci, const QString &skill_name)
    {
        zuoci->getRoom()->broadcastSkillInvoke(skill_name, zuoci->isMale(), -1);
    }

    static void AcquireGenerals(ServerPlayer *zuoci, int n)
    {
        Room *room = zuoci->getRoom();
        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        QStringList list = GetAvailableGenerals(zuoci);
        qShuffle(list);
        if (list.isEmpty())
            return;
        n = qMin(n, list.length());

        QStringList acquired = list.mid(0, n);
        foreach (QString name, acquired) {
            huashens << name;
            const General *general = Sanguosha->getGeneral(name);
            if (general) {
                foreach (const TriggerSkill *skill, general->getTriggerSkills()) {
                    if (skill->isVisible())
                        room->getThread()->addTriggerSkill(skill);
                }
            }
        }
        zuoci->tag["IkHuanshens"] = huashens;

        QStringList hidden;
        for (int i = 0; i < n; i++)
            hidden << "unknown";
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p == zuoci)
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), acquired.join(":"), QList<ServerPlayer *>() << p);
            else
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), hidden.join(":"), QList<ServerPlayer *>() << p);
        }

        LogMessage log;
        log.type = "#GetIkHuanshen";
        log.from = zuoci;
        log.arg = QString::number(n);
        log.arg2 = QString::number(huashens.length());
        room->sendLog(log);

        LogMessage log2;
        log2.type = "#GetIkHuanshenDetail";
        log2.from = zuoci;
        log2.arg = acquired.join("\\, \\");
        room->sendLog(log2, zuoci);

        room->setPlayerMark(zuoci, "@huanshen", huashens.length());
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci)
    {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames().toSet();
        Room *room = zuoci->getRoom();
        if (isNormalGameMode(room->getMode()) || room->getMode().contains("_mini_") || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        } else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/KOF", "").toStringList().toSet());
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }
        QSet<QString> huashen_set, room_set;
        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        foreach (QVariant huashen, huashens)
            huashen_set << huashen.toString();
        foreach (ServerPlayer *player, room->getAlivePlayers()) {
            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty())
                    name = fname;
            }
            room_set << name;

            if (!player->getGeneral2())
                continue;

            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty())
                    name = fname;
            }
            room_set << name;
        }

        static QSet<QString> banned;
        if (banned.isEmpty())
            banned << "luna009"
                   << "snow034";

        return (all - banned - huashen_set - room_set).toList();
    }

    static void SelectSkill(ServerPlayer *zuoci)
    {
        Room *room = zuoci->getRoom();
        playAudioEffect(zuoci, "ikhuanshen");
        QStringList ac_dt_list;

        QString huashen_skill = zuoci->tag["IkHuanshenSkill"].toString();
        if (!huashen_skill.isEmpty())
            ac_dt_list.append("-" + huashen_skill);

        QVariantList huashens = zuoci->tag["IkHuanshens"].toList();
        if (huashens.isEmpty())
            return;

        QStringList huashen_generals;
        foreach (QVariant huashen, huashens)
            huashen_generals << huashen.toString();

        QStringList skill_names;
        QString skill_name;
        const General *general = NULL;
        AI *ai = zuoci->getAI();
        if (ai) {
            QHash<QString, const General *> hash;
            foreach (QString general_name, huashen_generals) {
                const General *general = Sanguosha->getGeneral(general_name);
                foreach (const Skill *skill, general->getVisibleSkillList()) {
                    if (skill->isLordSkill() || skill->isOwnerOnlySkill() || skill->getFrequency(zuoci) == Skill::Limited || skill->getFrequency(zuoci) == Skill::Wake)
                        continue;

                    if (!skill_names.contains(skill->objectName())) {
                        hash[skill->objectName()] = general;
                        skill_names << skill->objectName();
                    }
                }
            }
            if (skill_names.isEmpty())
                return;
            skill_name = ai->askForChoice("ikhuanshen", skill_names.join("+"), QVariant());
            general = hash[skill_name];
            Q_ASSERT(general != NULL);
        } else {
            QString general_name = room->askForGeneral(zuoci, huashen_generals);
            general = Sanguosha->getGeneral(general_name);

            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill() || skill->isOwnerOnlySkill() || skill->getFrequency(zuoci) == Skill::Limited || skill->getFrequency(zuoci) == Skill::Wake)
                    continue;

                skill_names << skill->objectName();
            }

            if (!skill_names.isEmpty())
                skill_name = room->askForChoice(zuoci, "ikhuanshen", skill_names.join("+"));
        }
        //Q_ASSERT(!skill_name.isNull() && !skill_name.isEmpty());

        QString kingdom = general->getKingdom();
        if (zuoci->getKingdom() != kingdom) {
            if (kingdom == "kami")
                kingdom = room->askForKingdom(zuoci);
            room->setPlayerProperty(zuoci, "kingdom", kingdom);
        }

        if (zuoci->getGender() != general->getGender())
            zuoci->setGender(general->getGender());

        JsonArray args;
        args << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        args << zuoci->objectName();
        args << general->objectName();
        args << skill_name;
        args << true;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        zuoci->tag["IkHuanshenSkill"] = skill_name;
        if (!skill_name.isEmpty())
            ac_dt_list.append(skill_name);
        room->handleAcquireDetachSkills(zuoci, ac_dt_list, true);
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && (target->getPhase() == Player::RoundStart || target->getPhase() == Player::NotActive);
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *zuoci, QVariant &, ServerPlayer *) const
    {
        return zuoci->askForSkillInvoke(objectName());
    }

    virtual bool onPhaseChange(ServerPlayer *zuoci) const
    {
        SelectSkill(zuoci);
        return false;
    }

    virtual QDialog *getHuanshenDialog() const
    {
        static IkHuanshenDialog *dialog;

        if (dialog == NULL)
            dialog = new IkHuanshenDialog;

        return dialog;
    }
};

class IkHuanshenStart : public GameStartSkill
{
public:
    IkHuanshenStart()
        : GameStartSkill("#ikhuanshen-start")
    {
        frequency = Compulsory;
    }

    virtual void onGameStart(ServerPlayer *zuoci) const
    {
        zuoci->getRoom()->notifySkillInvoked(zuoci, "ikhuanshen");
        IkHuanshen::AcquireGenerals(zuoci, 3);
        IkHuanshen::SelectSkill(zuoci);
    }
};

IkHuanshenDialog::IkHuanshenDialog()
{
    setWindowTitle(Sanguosha->translate("ikhuanshen"));
}

void IkHuanshenDialog::popup()
{
    QVariantList huashen_list = Self->tag["IkHuanshens"].toList();
    QList<const General *> huashens;
    foreach (QVariant huashen, huashen_list)
        huashens << Sanguosha->getGeneral(huashen.toString());

    fillGenerals(huashens);
    show();
}

class IkHuanshenClear : public DetachEffectSkill
{
public:
    IkHuanshenClear()
        : DetachEffectSkill("ikhuanshen")
    {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const
    {
        if (player->getKingdom() != player->getGeneral()->getKingdom() && player->getGeneral()->getKingdom() != "kami")
            room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
        if (player->getGender() != player->getGeneral()->getGender())
            player->setGender(player->getGeneral()->getGender());
        QString huashen_skill = player->tag["IkHuanshenSkill"].toString();
        if (!huashen_skill.isEmpty())
            room->detachSkillFromPlayer(player, huashen_skill, false, true);
        player->tag.remove("IkHuanshens");
        room->setPlayerMark(player, "@huanshen", 0);
    }
};

class IkLingqi : public MasochismSkill
{
public:
    IkLingqi()
        : MasochismSkill("iklingqi")
    {
        frequency = Frequent;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *zuoci, QVariant &data, ServerPlayer *&) const
    {
        if (!MasochismSkill::triggerable(zuoci))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        QStringList skills;
        for (int i = 0; i < damage.damage; ++i)
            skills << objectName();
        return skills;
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *zuoci, QVariant &, ServerPlayer *) const
    {
        if (zuoci->askForSkillInvoke(objectName())) {
            IkHuanshen::playAudioEffect(zuoci, objectName());
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *zuoci, const DamageStruct &) const
    {
        IkHuanshen::AcquireGenerals(zuoci, 1);
    }
};

#include "touhou-hana.h"

IkGuihuoCard::IkGuihuoCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkGuihuoCard::ikguihuo(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);
    QSet<ServerPlayer *> questioned;

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    foreach (int card_id, getSubcards())
        used_cards << card_id;
    room->setTag("IkGuihuoType", user_string);

    foreach (ServerPlayer *player, players) {
        if (player->getHp() <= 0) {
            LogMessage log;
            log.type = "#IkGuihuoCannotQuestion";
            log.from = player;
            log.arg = QString::number(player->getHp());
            room->sendLog(log);

            room->setEmotion(player, "no-question");
            continue;
        }

        QString choice = room->askForChoice(player, "ikguihuo", "noquestion+question");
        if (choice == "question") {
            room->setEmotion(player, "question");
            questioned << player;
        } else
            room->setEmotion(player, "no-question");

        LogMessage log;
        log.type = "#IkGuihuoQuery";
        log.from = player;
        log.arg = choice;

        room->sendLog(log);
    }

    LogMessage log;
    log.type = "$IkGuihuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    bool success = false;
    if (questioned.isEmpty()) {
        success = true;
        foreach (ServerPlayer *player, players)
            room->setEmotion(player, ".");

        CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "ikguihuo");
        reason.m_extraData = QVariant::fromValue((const Card *)this);
        CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
        moves.append(move);
        room->moveCardsAtomic(moves, true);
    } else {
        const Card *card = Sanguosha->getCard(subcards.first());
        bool real;
        if (user_string == "peach+analeptic")
            real = card->objectName() == yuji->tag["IkGuihuoSaveSelf"].toString();
        else if (user_string == "slash")
            real = card->objectName().contains("slash");
        else if (user_string == "normal_slash")
            real = card->objectName() == "slash";
        else
            real = card->match(user_string);

        success = real && card->getSuit() == Card::Heart;
        if (success) {
            CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "ikguihuo");
            reason.m_extraData = QVariant::fromValue((const Card *)this);
            CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
            moves.append(move);
            room->moveCardsAtomic(moves, true);
        } else {
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), "ikguihuo"), true);
        }
        foreach (ServerPlayer *player, players) {
            room->setEmotion(player, ".");
            if (questioned.contains(player)) {
                if (real)
                    room->loseHp(player);
                else
                    player->drawCards(1, "ikguihuo");
            }
        }
    }
    yuji->tag.remove("IkGuihuoSaveSelf");
    yuji->tag.remove("IkGuihuoSlash");
    return success;
}

bool IkGuihuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *_card = Self->tag.value("ikguihuo").value<const Card *>();
    Card *card = NULL;
    if (_card) {
        card = Sanguosha->cloneCard(_card->objectName());
        card->setSkillName("ikguihuo");
        card->setCanRecast(false);
        card->deleteLater();
    }
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool IkGuihuoCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *_card = Self->tag.value("ikguihuo").value<const Card *>();
    Card *card = NULL;
    if (_card) {
        card = Sanguosha->cloneCard(_card->objectName());
        card->setSkillName("ikguihuo");
        card->setCanRecast(false);
        card->deleteLater();
    }
    return card && card->targetFixed();
}

bool IkGuihuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *_card = Self->tag.value("ikguihuo").value<const Card *>();
    Card *card = NULL;
    if (_card) {
        card = Sanguosha->cloneCard(_card->objectName());
        card->setSkillName("ikguihuo");
        card->setCanRecast(false);
        card->deleteLater();
    }
    return card && card->targetsFeasible(targets, Self);
}

const Card *IkGuihuoCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *yuji = card_use.from;
    Room *room = yuji->getRoom();

    QString to_ikguihuo = user_string;
    if (user_string == "slash" && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList ikguihuo_list;
        ikguihuo_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            ikguihuo_list << "normal_slash"
                          << "thunder_slash"
                          << "fire_slash";
        to_ikguihuo = room->askForChoice(yuji, "ikguihuo_slash", ikguihuo_list.join("+"));
        yuji->tag["IkGuihuoSlash"] = QVariant(to_ikguihuo);
    }
    room->broadcastSkillInvoke("ikguihuo");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#IkGuihuoNoTarget" : "#IkGuihuo";
    log.from = yuji;
    log.to = card_use.to;
    log.arg = to_ikguihuo;
    log.arg2 = "ikguihuo";

    room->sendLog(log);

    if (ikguihuo(card_use.from)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_ikguihuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_ikguihuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_ikguihuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("ikguihuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

const Card *IkGuihuoCard::validateInResponse(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    room->broadcastSkillInvoke("ikguihuo");

    QString to_ikguihuo;
    if (user_string == "peach+analeptic") {
        QStringList ikguihuo_list;
        ikguihuo_list << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            ikguihuo_list << "analeptic";
        to_ikguihuo = room->askForChoice(yuji, "ikguihuo_saveself", ikguihuo_list.join("+"));
        yuji->tag["IkGuihuoSaveSelf"] = QVariant(to_ikguihuo);
    } else if (user_string == "slash") {
        QStringList ikguihuo_list;
        ikguihuo_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            ikguihuo_list << "normal_slash"
                          << "thunder_slash"
                          << "fire_slash";
        to_ikguihuo = room->askForChoice(yuji, "ikguihuo_slash", ikguihuo_list.join("+"));
        yuji->tag["IkGuihuoSlash"] = QVariant(to_ikguihuo);
    } else
        to_ikguihuo = user_string;

    LogMessage log;
    log.type = "#IkGuihuoNoTarget";
    log.from = yuji;
    log.arg = to_ikguihuo;
    log.arg2 = "ikguihuo";
    room->sendLog(log);

    if (ikguihuo(yuji)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_ikguihuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_ikguihuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_ikguihuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("ikguihuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

class IkGuihuo : public OneCardViewAsSkill
{
public:
    IkGuihuo()
        : OneCardViewAsSkill("ikguihuo")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->isKongcheng() || pattern.startsWith(".") || pattern.startsWith("@"))
            return false;
        if (pattern == "peach" && player->getMark("Global_PreventPeach") > 0)
            return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit())
                return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        CardUseStruct::CardUseReason reason = Sanguosha->currentRoomState()->getCurrentCardUseReason();
        if (reason == CardUseStruct::CARD_USE_REASON_RESPONSE || reason == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern.contains("+"))
                pattern = pattern.split("+").first();
            const Card *c = Sanguosha->cloneCard(pattern);
            Card::HandlingMethod method = reason == CardUseStruct::CARD_USE_REASON_RESPONSE ? Card::MethodResponse : Card::MethodUse;
            if (Self->isCardLimited(c, method)) {
                delete c;
                return NULL;
            }

            IkGuihuoCard *card = new IkGuihuoCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        const Card *c = Self->tag.value("ikguihuo").value<const Card *>();
        if (c) {
            IkGuihuoCard *card = new IkGuihuoCard;
            if (!c->objectName().contains("slash"))
                card->setUserString(c->objectName());
            else
                card->setUserString(Self->tag["IkGuihuoSlash"].toString());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    virtual SkillDialog *getDialog() const
    {
        return ThMimengDialog::getInstance("ikguihuo");
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (!card->isKindOf("IkGuihuoCard"))
            return -2;
        else
            return -1;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !player->isKongcheng();
    }
};

class IkHuiyao : public TriggerSkill
{
public:
    IkHuiyao()
        : TriggerSkill("ikhuiyao")
    {
        events << Damaged << FinishJudge;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL || !damage.card->isKindOf("Slash") || damage.to->isDead())
                return skill_list;

            foreach (ServerPlayer *caiwenji, room->findPlayersBySkillName(objectName())) {
                if (caiwenji->canDiscard(caiwenji, "he"))
                    skill_list.insert(caiwenji, QStringList(objectName()));
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName())
                return skill_list;
            judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *caiwenji) const
    {
        if (room->askForCard(caiwenji, "..", "@ikhuiyao", data, objectName())) {
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *caiwenji) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.who = player;
        judge.reason = objectName();

        room->judge(judge);

        Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
        switch (suit) {
        case Card::Heart: {
            room->broadcastSkillInvoke(objectName(), 3);
            room->recover(player, RecoverStruct(caiwenji));

            break;
        }
        case Card::Diamond: {
            room->broadcastSkillInvoke(objectName(), 1);
            player->drawCards(2, objectName());
            break;
        }
        case Card::Club: {
            room->broadcastSkillInvoke(objectName(), 2);
            if (damage.from && damage.from->isAlive())
                room->askForDiscard(damage.from, "ikhuiyao", 2, 2, false, true);

            break;
        }
        case Card::Spade: {
            room->broadcastSkillInvoke(objectName(), 4);
            if (damage.from && damage.from->isAlive())
                damage.from->turnOver();

            break;
        }
        default:
            break;
        }

        return false;
    }
};

class IkQihuang : public TriggerSkill
{
public:
    IkQihuang()
        : TriggerSkill("ikqihuang")
    {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;

        if (death.damage && death.damage->from) {
            if (death.damage->from->hasSkill("thyanmeng")) {
                int index = qrand() % 2 + 1;
                if (death.damage->from->getGeneralName() == "luna037")
                    room->broadcastSkillInvoke("thyanmeng", index);
            } else if (death.damage->from->hasSkill("thxuanyan"))
                room->broadcastSkillInvoke("thxuanyan");
            else {
                LogMessage log;
                log.type = "#IkQihuangLoseSkills";
                log.from = player;
                log.to << death.damage->from;
                log.arg = objectName();
                room->sendLog(log);
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(player, objectName());

                QList<const Skill *> skills = death.damage->from->getVisibleSkillList(false, true);
                QStringList detachList;
                foreach (const Skill *skill, skills) {
                    if (!skill->inherits("SPConvertSkill") && !skill->isAttachedLordSkill())
                        detachList.append("-" + skill->objectName());
                }
                room->handleAcquireDetachSkills(death.damage->from, detachList);
                if (death.damage->from->isAlive())
                    death.damage->from->addMark("@qihuang");
            }
        }

        return false;
    }
};

class IkLeiji : public TriggerSkill
{
public:
    IkLeiji()
        : TriggerSkill("ikleiji")
    {
        events << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *zhangjiao, QVariant &data, ServerPlayer *&) const
    {
        const Card *card_star = data.value<CardResponseStruct>().m_card;
        if (TriggerSkill::triggerable(zhangjiao) && card_star->isKindOf("Jink"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(zhangjiao, room->getAlivePlayers(), objectName(), "ikleiji-invoke", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            zhangjiao->tag["IkLeijiTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = zhangjiao->tag["IkLeijiTarget"].value<ServerPlayer *>();
        zhangjiao->tag.remove("IkLeijiTarget");
        if (target) {
            JudgeStruct judge;
            judge.pattern = ".|black";
            judge.good = false;
            judge.negative = true;
            judge.reason = objectName();
            judge.who = target;

            room->judge(judge);

            if (judge.isBad()) {
                if (judge.card->getSuit() == Card::Spade)
                    room->damage(DamageStruct(objectName(), zhangjiao, target, 2, DamageStruct::Thunder));
                else if (judge.card->getSuit() == Card::Club) {
                    room->damage(DamageStruct(objectName(), zhangjiao, target, 1, DamageStruct::Thunder));
                    if (zhangjiao->isWounded())
                        room->recover(zhangjiao, RecoverStruct(zhangjiao));
                }
            }
        }

        return false;
    }
};

class IkTianshi : public TriggerSkill
{
public:
    IkTianshi()
        : TriggerSkill("iktianshi")
    {
        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        if (!TriggerSkill::triggerable(target))
            return false;

        if (target->isKongcheng()) {
            bool has_black = false;
            for (int i = 0; i < 5; i++) {
                const EquipCard *equip = target->getEquip(i);
                if (equip && equip->isBlack()) {
                    has_black = true;
                    break;
                }
            }
            return has_black;
        } else
            return true;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@iktianshi-card" << judge->who->objectName() << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, ".|black", prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkTianshiCard"] = QVariant::fromValue(card);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        const Card *card = player->tag["IkTianshiCard"].value<const Card *>();
        player->tag.remove("IkTianshiCard");
        if (card) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            room->retrial(card, player, judge, objectName(), true);
        }
        return false;
    }
};

IkYujiCard::IkYujiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "ikyujiv";
    mute = true;
}

void IkYujiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *zhangjiao = targets.first();
    if (zhangjiao->hasLordSkill("ikyuji")) {
        room->setPlayerFlag(zhangjiao, "IkYujiInvoked");

        room->broadcastSkillInvoke("ikyuji");

        room->notifySkillInvoked(zhangjiao, "ikyuji");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), zhangjiao->objectName(), "ikyuji", QString());
        room->obtainCard(zhangjiao, this, reason);
        QList<ServerPlayer *> zhangjiaos;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasLordSkill("ikyuji") && !p->hasFlag("IkYujiInvoked"))
                zhangjiaos << p;
        }
        if (zhangjiaos.isEmpty())
            room->setPlayerFlag(source, "ForbidIkYuji");
    }
}

bool IkYujiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("ikyuji") && to_select != Self && !to_select->hasFlag("IkYujiInvoked");
}

class IkYujiViewAsSkill : public OneCardViewAsSkill
{
public:
    IkYujiViewAsSkill()
        : OneCardViewAsSkill("ikyujiv")
    {
        attached_lord_skill = true;
        filter_pattern = "Jink,EightDiagram";
    }

    virtual bool shouldBeVisible(const Player *player) const
    {
        return player && player->getKingdom() == "tsuki";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return shouldBeVisible(player) && !player->hasFlag("ForbidIkYuji");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IkYujiCard *card = new IkYujiCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class IkYuji : public TriggerSkill
{
public:
    IkYuji()
        : TriggerSkill("ikyuji$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!player)
            return QStringList();
        if ((triggerEvent == GameStart && player->isLord()) || (triggerEvent == EventAcquireSkill && data.toString() == "ikyuji")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty())
                return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("ikyujiv"))
                    room->attachSkillToPlayer(p, "ikyujiv");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "ikyuji") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2)
                return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("ikyujiv"))
                    room->detachSkillFromPlayer(p, "ikyujiv", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play && phase_change.to != Player::Play)
                return QStringList();
            if (player->hasFlag("ForbidIkYuji"))
                room->setPlayerFlag(player, "-ForbidIkYuji");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("IkYujiInvoked"))
                    room->setPlayerFlag(p, "-IkYujiInvoked");
            }
        }
        return QStringList();
    }
};

class IkZhuohuo : public TriggerSkill
{
public:
    IkZhuohuo()
        : TriggerSkill("ikzhuohuo")
    {
        events << Damage << Damaged;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = triggerEvent == Damage ? "#IkZhuohuoDamage" : "#IkZhuohuoDamaged";
        log.from = player;
        log.arg = QString::number(damage.damage);
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());

        room->addPlayerMark(player, "@blaze", damage.damage);
        room->broadcastSkillInvoke(objectName(), triggerEvent == Damage ? 1 : 2);
        return false;
    }
};

class IkWumou : public TriggerSkill
{
public:
    IkWumou()
        : TriggerSkill("ikwumou")
    {
        frequency = Compulsory;
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && use.card->isNDTrick())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        int num = player->getMark("@blaze");
        if (num >= 1 && room->askForChoice(player, objectName(), "discard+losehp") == "discard") {
            player->loseMark("@blaze");
        } else
            room->loseHp(player);

        return false;
    }
};

IkSuikongCard::IkSuikongCard()
{
}

void IkSuikongCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    effect.from->loseMark("@blaze", 2);
    room->acquireSkill(effect.from, "ikwushuang");
    effect.from->setFlags("IkSuikongSource");
    effect.to->setFlags("IkSuikongTarget");
    room->addPlayerMark(effect.to, "Armor_Nullified");
}

class IkSuikongViewAsSkill : public ZeroCardViewAsSkill
{
public:
    IkSuikongViewAsSkill()
        : ZeroCardViewAsSkill("iksuikong")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@blaze") >= 2;
    }

    virtual const Card *viewAs() const
    {
        return new IkSuikongCard;
    }
};

class IkSuikong : public TriggerSkill
{
public:
    IkSuikong()
        : TriggerSkill("iksuikong")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new IkSuikongViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!player->hasFlag("IkSuikongSource"))
            return QStringList();
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

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasFlag("IkSuikongTarget")) {
                p->setFlags("-IkSuikongTarget");
                if (p->getMark("Armor_Nullified") > 0)
                    room->removePlayerMark(p, "Armor_Nullified");
            }
        }
        room->detachSkillFromPlayer(player, "ikwushuang", false, true);

        return QStringList();
    }
};

class IkTianwu : public ZeroCardViewAsSkill
{
public:
    IkTianwu()
        : ZeroCardViewAsSkill("iktianwu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@blaze") >= 6 && !player->hasUsed("IkTianwuCard");
    }

    virtual const Card *viewAs() const
    {
        return new IkTianwuCard;
    }
};

IkTianwuCard::IkTianwuCard()
{
    target_fixed = true;
}

void IkTianwuCard::use(Room *room, ServerPlayer *shenlvbu, QList<ServerPlayer *> &) const
{
    shenlvbu->loseMark("@blaze", 6);

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

IkaiMokuPackage::IkaiMokuPackage()
    : Package("ikai-moku")
{
    General *wind008 = new General(this, "wind008", "kaze");
    wind008->addSkill(new IkLiegong);
    wind008->addSkill(new IkLiegongClear);
    related_skills.insertMulti("ikliegong", "#ikliegong-clear");
    wind008->addSkill(new IkHuanghun);

    General *wind009 = new General(this, "wind009", "kaze");
    wind009->addSkill(new IkKuanggu);

    General *wind010 = new General(this, "wind010", "kaze", 3);
    wind010->addSkill(new IkFuhua);
    wind010->addSkill(new IkSuinie);
    wind010->addRelateSkill("ikmopan");

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
    wind014->addSkill(new IkFule);
    wind014->addSkill(new IkYouji);
    wind014->addSkill(new IkRuoyu);

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
    wind030->addSkill(new IkJuejing);
    wind030->addSkill(new IkJuejingKeep);
    wind030->addSkill(new IkZhihun);
    related_skills.insertMulti("ikjuejing", "#ikjuejing-keep");

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
    snow010->addSkill(new IkZhuoshi);
    snow010->addSkill(new IkZhuoshiGive);
    related_skills.insertMulti("ikzhuoshi", "#ikzhuoshi-give");
    snow010->addSkill(new IkYuanjie);

    General *snow011 = new General(this, "snow011", "yuki", 3, false);
    snow011->addSkill(new IkZhihui);
    snow011->addSkill(new IkZhihuiDraw);
    snow011->addSkill(new IkChiqiu);
    related_skills.insertMulti("ikzhihui", "#ikzhihui");

    General *snow012 = new General(this, "snow012", "yuki");
    snow012->addSkill(new IkJianlve);
    snow012->addRelateSkill("ikkongyun");

    General *snow013 = new General(this, "snow013", "yuki");
    snow013->addSkill(new IkSusheng);
    snow013->addSkill(new IkSushengMaxCards);
    related_skills.insertMulti("iksusheng", "#iksusheng");
    snow013->addSkill(new IkHuapan);

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

    General *luna001 = new General(this, "luna001$", "tsuki", 8);
    luna001->addSkill(new IkFusheng);
    luna001->addSkill(new IkHuanbei);
    luna001->addSkill(new IkBenghuai);
    luna001->addSkill(new IkWuhua);
    luna001->addSkill(new IkWuhuaRecord);
    related_skills.insertMulti("ikwuhua", "#ikwuhua-record");

    General *luna004 = new General(this, "luna004", "tsuki");
    luna004->addSkill(new IkXinghuang);
    luna004->addSkill(new IkXuzhao);

    General *luna005 = new General(this, "luna005", "tsuki");
    luna005->addSkill(new IkJingfa);
    luna005->addSkill(new IkQiyu);
    luna005->addSkill(new IkQiyuGet);
    related_skills.insertMulti("ikqiyu", "#ikqiyu");

    General *luna007 = new General(this, "luna007", "tsuki", 3);
    luna007->addSkill(new IkSishideng);
    luna007->addSkill(new IkWenle);
    luna007->addSkill(new IkMoyu);

    General *luna008 = new General(this, "luna008", "tsuki");
    luna008->addSkill("thjibu");
    luna008->addSkill(new IkKongsa);

    General *luna009 = new General(this, "luna009", "tsuki", 3);
    luna009->addSkill(new IkHuanshen);
    luna009->addSkill(new IkHuanshenStart);
    luna009->addSkill(new IkHuanshenClear);
    related_skills.insertMulti("ikhuanshen", "#ikhuanshen-start");
    related_skills.insertMulti("ikhuanshen", "#ikhuanshen-clear");
    luna009->addSkill(new IkLingqi);

    General *luna011 = new General(this, "luna011", "tsuki");
    luna011->addSkill(new IkGuihuo);

    General *luna012 = new General(this, "luna012", "tsuki", 3, false);
    luna012->addSkill(new IkHuiyao);
    luna012->addSkill(new IkQihuang);

    General *luna014 = new General(this, "luna014$", "tsuki", 3);
    luna014->addSkill(new IkLeiji);
    luna014->addSkill(new IkTianshi);
    luna014->addSkill(new IkYuji);

    General *luna029 = new General(this, "luna029", "tsuki", 5);
    luna029->addSkill(new IkZhuohuo);
    luna029->addSkill(new MarkAssignSkill("@blaze", 2));
    luna029->addSkill(new IkWumou);
    luna029->addSkill(new IkSuikong);
    luna029->addSkill(new IkTianwu);
    related_skills.insertMulti("ikzhuohuo", "#@blaze-2");

    addMetaObject<IkHuanghunCard>();
    addMetaObject<IkTiaoxinCard>();
    addMetaObject<IkYoujiCard>();
    addMetaObject<IkLiefengCard>();
    addMetaObject<IkMiaowuCard>();
    addMetaObject<IkXunyuCard>();
    addMetaObject<IkMancaiCard>();
    addMetaObject<IkQiangxiCard>();
    addMetaObject<IkYushenCard>();
    addMetaObject<IkYihuoCard>();
    addMetaObject<IkJilveCard>();
    addMetaObject<IkZhuoshiCard>();
    addMetaObject<IkYuanjieCard>();
    addMetaObject<IkZhihuiCard>();
    addMetaObject<IkJianlveCard>();
    addMetaObject<IkBianshengCard>();
    addMetaObject<IkJibanCard>();
    addMetaObject<IkLingshiCard>();
    addMetaObject<IkYeyanCard>();
    addMetaObject<GreatIkYeyanCard>();
    addMetaObject<SmallIkYeyanCard>();
    addMetaObject<IkXuzhaoCard>();
    addMetaObject<IkWenleCard>();
    addMetaObject<IkGuihuoCard>();
    addMetaObject<IkYujiCard>();
    addMetaObject<IkSuikongCard>();
    addMetaObject<IkTianwuCard>();

    skills << new IkMohua << new IkHuanwu << new IkYihuoViewAsSkill << new IkJilve << new IkJilveClear << new IkKongyun << new IkBianshengPindian << new IkYujiViewAsSkill;
}

ADD_PACKAGE(IkaiMoku)
