#include "ikai-do.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "maneuvering.h"
#include "fantasy.h"

IkShenaiCard::IkShenaiCard() {
    target_fixed = true;
}

void IkShenaiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    QList<int> handCards = source->handCards();
    if (room->askForRende(source, handCards, "ikshenai", false, true, -1, QList<ServerPlayer *>(),
                          CardMoveReason(), "@ikshenai", false) >= 2)
        room->recover(source, RecoverStruct(source));
}

class IkShenai: public ZeroCardViewAsSkill {
public:
    IkShenai(): ZeroCardViewAsSkill("ikshenai") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkShenaiCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new IkShenaiCard;
    }
};

IkXinqiCard::IkXinqiCard() {
}

bool IkXinqiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

const Card *IkXinqiCard::validate(CardUseStruct &cardUse) const{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *liubei = cardUse.from;
    QList<ServerPlayer *> targets = cardUse.to;
    Room *room = liubei->getRoom();
    liubei->broadcastSkillInvoke(this);
    room->notifySkillInvoked(liubei, "ikxinqi");

    LogMessage log;
    log.from = liubei;
    log.to = targets;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    const Card *slash = NULL;

    QList<ServerPlayer *> lieges = room->getLieges("kaze", liubei);
    foreach (ServerPlayer *target, targets)
        target->setFlags("IkXinqiTarget");
    foreach (ServerPlayer *liege, lieges) {
        try {
            slash = room->askForCard(liege, "slash", "@ikxinqi-slash:" + liubei->objectName(),
                                     QVariant(), Card::MethodResponse, liubei, false, QString(), true);
        }
        catch (TriggerEvent triggerEvent) {
            if (triggerEvent == TurnBroken || triggerEvent == StageChange) {
                foreach (ServerPlayer *target, targets)
                    target->setFlags("-IkXinqiTarget");
            }
            throw triggerEvent;
        }

        if (slash) {
            foreach (ServerPlayer *target, targets)
                target->setFlags("-IkXinqiTarget");

            foreach (ServerPlayer *target, targets) {
                if (!liubei->canSlash(target, slash))
                    cardUse.to.removeOne(target);
            }
            if (cardUse.to.length() > 0)
                return slash;
            else {
                delete slash;
                return NULL;
            }
        }
    }
    foreach (ServerPlayer *target, targets)
        target->setFlags("-IkXinqiTarget");
    room->setPlayerFlag(liubei, "Global_IkXinqiFailed");
    return NULL;
}

IkXinqiViewAsSkill::IkXinqiViewAsSkill(): ZeroCardViewAsSkill("ikxinqi$") {
}

bool IkXinqiViewAsSkill::isEnabledAtPlay(const Player *player) const{
    return hasKazeGenerals(player) && !player->hasFlag("Global_IkXinqiFailed") && Slash::IsAvailable(player);
}

bool IkXinqiViewAsSkill::isEnabledAtResponse(const Player *player, const QString &pattern) const{
    return hasKazeGenerals(player)
           && (pattern == "slash" || pattern == "@ikxinqi")
           && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
           && !player->hasFlag("Global_IkXinqiFailed");
}

const Card *IkXinqiViewAsSkill::viewAs() const{
    return new IkXinqiCard;
}

bool IkXinqiViewAsSkill::hasKazeGenerals(const Player *player) {
    foreach (const Player *p, player->getAliveSiblings())
        if (p->getKingdom() == "kaze")
            return true;
    return false;
}

class IkXinqi: public TriggerSkill {
public:
    IkXinqi(): TriggerSkill("ikxinqi$") {
        events << CardAsked;
        view_as_skill = new IkXinqiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player && player->isAlive() && player->hasLordSkill("ikxinqi")) {
            QString pattern = data.toStringList().first();
            QString prompt = data.toStringList().at(1);
            if (pattern != "slash" || prompt.startsWith("@ikxinqi-slash"))
                return QStringList();
            QList<ServerPlayer *> lieges = room->getLieges("kaze", player);
            if (lieges.isEmpty())
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *liubei, QVariant &data, ServerPlayer *) const{
        if (liubei->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName(), getEffectIndex(liubei, NULL));
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liubei, QVariant &, ServerPlayer *) const{
        QList<ServerPlayer *> lieges = room->getLieges("kaze", liubei);
        foreach (ServerPlayer *liege, lieges) {
            const Card *slash = room->askForCard(liege, "slash", "@ikxinqi-slash:" + liubei->objectName(),
                                                 QVariant(), Card::MethodResponse, liubei, false, QString(), true);
            if (slash) {
                room->provide(slash);
                return true;
            }
        }
        return false;
    }
};

IkChilianDialog *IkChilianDialog::getInstance()
{
    static IkChilianDialog *instance;
    if (instance == NULL)
        instance = new IkChilianDialog();

    return instance;
}

IkChilianDialog::IkChilianDialog()
{
    setObjectName("ikchilian");
    setWindowTitle(Sanguosha->translate("ikchilian"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void IkChilianDialog::popup()
{
    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QStringList card_names;
    card_names << "slash" << "fire_slash";

    foreach (QString card_name, card_names) {
        QCommandLinkButton *button = new QCommandLinkButton;
        button->setText(Sanguosha->translate(card_name));
        button->setObjectName(card_name);
        group->addButton(button);

        bool can = true;
        const Card *c = Sanguosha->cloneCard(card_name);
        if (Self->isCardLimited(c, Card::MethodUse) || !c->isAvailable(Self))
            can = false;
        delete c;
        button->setEnabled(can);
        button_layout->addWidget(button);

        if (!map.contains(card_name)) {
            Card *c = Sanguosha->cloneCard(card_name);
            c->setParent(this);
            map.insert(card_name, c);
        }
    }

    exec();
}

void IkChilianDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag["ikchilian"] = QVariant::fromValue(card);
    emit onButtonClick();
    accept();
}

class IkChilian : public OneCardViewAsSkill
{
public:
    IkChilian() : OneCardViewAsSkill("ikchilian")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "slash";
    }

    virtual QDialog *getDialog() const
    {
        return IkChilianDialog::getInstance();
    }

    virtual bool viewFilter(const Card *card) const
    {
        if (!card->isRed())
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(card->getEffectiveId());
            slash->deleteLater();
            FireSlash *fire_slash = new FireSlash(Card::SuitToBeDecided, -1);
            fire_slash->addSubcard(card->getEffectiveId());
            fire_slash->deleteLater();
            return slash->isAvailable(Self) || fire_slash->isAvailable(Self);
        }
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        const Card *c = Self->tag.value("ikchilian").value<const Card *>();
        if (c) {
            Card *card = Sanguosha->cloneCard(c);
            card->addSubcard(originalCard);
            card->setSkillName(objectName());
            return card;
        }

        return NULL;
    }

    virtual int getEffectIndex(const ServerPlayer *target, const Card *) const
    {
        if (target->getGeneralName() == "snow052")
            return qrand() % 2 + 1;
        return 0;
    }
};

class IkZhenhong: public TriggerSkill {
public:
    IkZhenhong(): TriggerSkill("ikzhenhong") {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && use.card->isKindOf("Slash") && use.card->getSuit() == Card::Diamond)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to.toSet())
            p->addQinggangTag(use.card);
        return false;
    }
};

class IkZhenhongTargetMod: public TargetModSkill {
public:
    IkZhenhongTargetMod(): TargetModSkill("#ikzhenhong-target") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("ikzhenhong") && card->getSuit() == Card::Heart)
            return 1000;
        else
            return 0;
    }
};

class IkYipao: public TargetModSkill {
public:
    IkYipao(): TargetModSkill("ikyipao") {
        frequency = NotCompulsory;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class IkShijiu: public OneCardViewAsSkill {
public:
    IkShijiu(): OneCardViewAsSkill("ikshijiu") {
        filter_pattern = "Weapon,TrickCard+^DelayedTrick";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern.contains("analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *anal = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        anal->addSubcard(originalCard->getId());
        anal->setSkillName(objectName());
        return anal;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        int index = qrand() % 2 + 1;
        if (card->hasFlag("analeptic_recover"))
            index += 2;
        return index;
    }
};

class IkYuxi: public PhaseChangeSkill {
public:
    IkYuxi(): PhaseChangeSkill("ikyuxi") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *zhuge) const {
        Room *room = zhuge->getRoom();
        QList<int> guanxing = room->getNCards(getIkYuxiNum(room));

        LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = zhuge;
        log.card_str = IntList2StringList(guanxing).join("+");
        room->sendLog(log, zhuge);

        room->askForGuanxing(zhuge, guanxing);

        return false;
    }

    virtual int getIkYuxiNum(Room *room) const{
        return qMin(5, room->alivePlayerCount());
    }
};

class IkJingyou: public ProhibitSkill {
public:
    IkJingyou(): ProhibitSkill("ikjingyou") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("Slash") || card->isKindOf("Duel")) && to->isKongcheng();
    }
};

class IkYufeng: public TriggerSkill {
public:
    IkYufeng(): TriggerSkill("ikyufeng") {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        QList<ServerPlayer *> tos;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive()) break;
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());
                if (!tos.contains(p)) {
                    p->addMark("ikyufeng");
                    room->addPlayerMark(p, "@skill_invalidity");
                    tos << p;

                    foreach (ServerPlayer *pl, room->getAllPlayers())
                        room->filterCards(pl, pl->getCards("he"), true);
                    JsonArray args;
                    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                }

                JudgeStruct judge;
                judge.pattern = ".";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;
                judge.play_animation = false;

                room->judge(judge);

                if ((p->isAlive() && !p->canDiscard(p, "he"))
                    || !room->askForCard(p, ".|" + judge.pattern, "@ikyufeng-discard:::" + judge.pattern, data, Card::MethodDiscard)) {
                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);
                    jink_list.replace(index, QVariant(0));
                }
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class IkYufengClear: public TriggerSkill {
public:
    IkYufengClear(): TriggerSkill("#ikyufeng-clear") {
        events << EventPhaseChanging << Death << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "ikyufeng")
                judge->pattern = judge->card->getSuitString();
            return QStringList();
        } else if (triggerEvent == EventPhaseChanging) {
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
            if (player->getMark("ikyufeng") == 0) continue;
            room->removePlayerMark(player, "@skill_invalidity", player->getMark("ikyufeng"));
            player->setMark("ikyufeng", 0);

            foreach (ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), false);
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return QStringList();
    }
};

class NonCompulsoryInvalidity: public InvaliditySkill {
public:
    NonCompulsoryInvalidity(): InvaliditySkill("#non-compulsory-invalidity") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return player->getMark("@skill_invalidity") == 0 || skill->getFrequency() == Skill::Compulsory;
    }
};

class IkHuiquan: public TriggerSkill {
public:
    IkHuiquan(): TriggerSkill("ikhuiquan") {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *yueying, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(yueying) && use.card->isKindOf("TrickCard"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &, ServerPlayer *) const{
        if (yueying->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *yueying, QVariant &, ServerPlayer *) const{
        yueying->drawCards(1, objectName());
        return false;
    }
};

class IkBaoou: public TriggerSkill {
public:
    IkBaoou(): TriggerSkill("ikbaoou") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (player == NULL) return skill_list;
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            if (player->hasFlag("IkBaoouDamage"))
                foreach (ServerPlayer *xushu, room->findPlayersBySkillName(objectName()))
                    if (xushu != player && xushu->canSlash(player, false))
                        skill_list.insert(xushu, QStringList(objectName()));
        } else if (triggerEvent == ChoiceMade && player->hasFlag("IkBaoouSlash") && data.canConvert<CardUseStruct>()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            player->setFlags("-IkBaoouSlash");
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *xushu) const{
        xushu->setFlags("IkBaoouSlash");
        QString prompt = QString("@ikbaoou-slash:%1:%2").arg(xushu->objectName()).arg(player->objectName());
        if (!room->askForUseSlashTo(xushu, player, prompt, false))
            xushu->setFlags("-IkBaoouSlash");
        return false;
    }
};

class IkBaoouRecord: public TriggerSkill {
public:
    IkBaoouRecord(): TriggerSkill("#ikbaoou-record") {
        events << PreDamageDone;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->getPhase() != Player::NotActive && !damage.from->hasFlag("IkBaoouDamage"))
            damage.from->setFlags("IkBaoouDamage");
        return QStringList();
    }
};

class IkYehua: public TriggerSkill {
public:
    IkYehua(): TriggerSkill("ikyehua") {
        events << Damage;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
               && target->getMark("@yehua") == 0
               && target->isWounded();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#IkYehuaWake";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        room->setPlayerMark(player, "@yehua", 1);
        if (room->changeMaxHpForAwakenSkill(player))
            room->acquireSkill(player, "ikxingyu");

        return false;
    }
};

IkXingyuCard::IkXingyuCard() {
    target_fixed = true;
}

void IkXingyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    QStringList choice_list, pattern_list;
    choice_list << "basic" << "trick" << "equip" << "red" << "black";
    pattern_list << "BasicCard" << "TrickCard" << "EquipCard" << ".|red" << ".|black";

    QString choice = room->askForChoice(source, "ikxingyu", choice_list.join("+"));
    QString pattern = pattern_list.at(choice_list.indexOf(choice));

    LogMessage log;
    log.type = "#IkXingyuChoice";
    log.from = source;
    log.arg = choice;
    room->sendLog(log);

    QList<int> cardIds;
    while (true) {
        int id = room->drawCard();
        cardIds << id;
        CardsMoveStruct move(id, NULL, Player::PlaceTable,
                             CardMoveReason(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "ikxingyu", QString()));
        room->moveCardsAtomic(move, true);
        room->getThread()->delay();

        const Card *card = Sanguosha->getCard(id);
        if (Sanguosha->matchExpPattern(pattern, NULL, card)) {
            QList<int> ids;
            ids << id;
            cardIds.removeOne(id);
            room->fillAG(ids, source);
            ServerPlayer *target = room->askForPlayerChosen(source, room->getAlivePlayers(), "ikxingyu",
                                                            QString("@ikxingyu-give:::%1:%2\\%3").arg(card->objectName())
                                                                                                .arg(card->getSuitString() + "_char")
                                                                                                .arg(card->getNumberString()));
            room->clearAG(source);
            room->obtainCard(target, card);
            break;
        }
    }
    if (!cardIds.isEmpty()) {
        DummyCard *dummy = new DummyCard(cardIds);
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, source->objectName(), "ikxingyu", QString());
        room->throwCard(dummy, reason, NULL);
        delete dummy;
    }
}

class IkXingyu: public ZeroCardViewAsSkill {
public:
    IkXingyu(): ZeroCardViewAsSkill("ikxingyu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkXingyuCard");
    }

    virtual const Card *viewAs() const{
        return new IkXingyuCard;
    }
};

class IkJiaoman: public MasochismSkill {
public:
    IkJiaoman(): MasochismSkill("ikjiaoman") {
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!MasochismSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        const Card *card = damage.card;
        if (card) {
            QList<int> ids;
            if (card->isVirtualCard())
                ids = card->getSubcards();
            else
                ids << card->getEffectiveId();
            if (ids.length() > 0) {
                bool all_place_table = true;
                foreach (int id, ids) {
                    if (room->getCardPlace(id) != Player::PlaceTable) {
                        all_place_table = false;
                        break;
                    }
                }
                if (all_place_table) return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        QList<ServerPlayer *> targets = damage.from ? room->getOtherPlayers(damage.from) : room->getAlivePlayers();
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@ikjiaoman", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkJiaomanTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const{
        ServerPlayer *target = caocao->tag["IkJiaomanTarget"].value<ServerPlayer *>();
        caocao->tag.remove("IkJiaomanTarget");
        if (target)
            target->obtainCard(damage.card);
    }
};

class IkHuanwei: public TriggerSkill {
public:
    IkHuanwei(): TriggerSkill("ikhuanwei$") {
        events << CardAsked;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player && player->isAlive() && player->hasLordSkill(objectName())) {
            QString pattern = data.toStringList().first();
            QString prompt = data.toStringList().at(1);
            if (pattern != "jink" || prompt.startsWith("@ikhuanwei-jink"))
                return QStringList();

            QList<ServerPlayer *> lieges = room->getLieges("hana", player);
            if (lieges.isEmpty())
                return QStringList();

            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        QVariant tohelp = QVariant::fromValue(player);
        foreach (ServerPlayer *liege, room->getLieges("hana", player)) {
            const Card *jink = room->askForCard(liege, "jink", "@ikhuanwei-jink:" + player->objectName(),
                                                tohelp, Card::MethodResponse, player, false, QString(), true);
            if (jink) {
                room->provide(jink);
                return true;
            }
        }

        return false;
    }
};

class IkTiansuo: public TriggerSkill {
public:
    IkTiansuo(): TriggerSkill("iktiansuo") {
        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && !player->isNude();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@iktiansuo-card" << judge->who->objectName()
                    << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, ".." , prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkTiansuoCard"] = QVariant::fromValue(card);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        const Card *card = player->tag["IkTiansuoCard"].value<const Card *>();
        player->tag.remove("IkTiansuoCard");
        if (card)
            room->retrial(card, player, data.value<JudgeStruct *>(), objectName());
        return false;
    }
};

class IkHuanji: public MasochismSkill {
public:
    IkHuanji(): MasochismSkill("ikhuanji") {
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (!TriggerSkill::triggerable(player)) return skill;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && !damage.from->isAllNude()) {
            for (int i = 0; i < damage.damage; ++i)
                skill << objectName();
        }
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.from))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const{
        int card_id = simayi->getRoom()->askForCardChosen(simayi, damage.from, "hej", "ikhuanji");
        simayi->obtainCard(Sanguosha->getCard(card_id), false);
    }
};

class IkAoli: public TriggerSkill {
public:
    IkAoli(): TriggerSkill("ikaoli") {
        events << Damaged << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *xiahou, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return skill;
            judge->pattern = QString::number(int(judge->card->getSuit()));
            return skill;
        }
        if (!TriggerSkill::triggerable(xiahou)) return skill;
        DamageStruct damage = data.value<DamageStruct>();
        for (int i = 0; i < damage.damage; i++)
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *xiahou, QVariant &, ServerPlayer *) const{
        if (xiahou->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *xiahou, QVariant &data, ServerPlayer *) const{
        JudgeStruct judge;
        judge.pattern = ".";
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = xiahou;

        room->judge(judge);
        DamageStruct damage = data.value<DamageStruct>();

        if (!damage.from || damage.from->isDead()) return false;
        Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
        switch (suit) {
        case Card::Heart:
        case Card::Diamond: {
                room->damage(DamageStruct(objectName(), xiahou, damage.from));
                break;
            }
        case Card::Club:
        case Card::Spade: {
                if (xiahou->canDiscard(damage.from, "hej")) {
                    int id = room->askForCardChosen(xiahou, damage.from, "hej", objectName(), false, Card::MethodDiscard);
                    room->throwCard(id, room->getCardPlace(id) == Player::PlaceDelayedTrick ? NULL : damage.from, xiahou);
                }
                break;
            }
        default:
                break;
        }

        return false;
    }
};

class IkQingjian: public TriggerSkill {
public:
    IkQingjian(): TriggerSkill("ikqingjian") {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        } else if (TriggerSkill::triggerable(player) && player->getMark(objectName()) == 0) {
            ServerPlayer *current = room->getCurrent();
            if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw
                    && move.to == player && move.to_place == Player::PlaceHand) {
                    QList<int> ids;
                    foreach (int id, move.card_ids)
                        if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                            return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                ids << id;
        }
        if (room->askForYiji(player, ids, objectName(), false, true, true, -1,
            QList<ServerPlayer *>(), CardMoveReason(), "@ikqingjian-distribute", true)) {
            room->addPlayerMark(player, objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                ids << id;
        }
        if (ids.isEmpty())
            return false;
        while (room->askForYiji(player, ids, objectName(), false, false, true, -1,
                                QList<ServerPlayer *>(), CardMoveReason(), "@ikqingjian-distribute", false)) {
            room->notifySkillInvoked(player, objectName());
            if (player->isDead()) return false;
        }

        return false;
    }
};

IkChibaoCard::IkChibaoCard() {
}

bool IkChibaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() >= Self->getMark("ikchibao") || to_select->getHandcardNum() < Self->getHandcardNum() || to_select == Self)
        return false;

    return !to_select->isKongcheng();
}

void IkChibaoCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->setFlags("IkChibaoTarget");
}

class IkChibaoViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkChibaoViewAsSkill(): ZeroCardViewAsSkill("ikchibao") {
        response_pattern = "@@ikchibao";
    }

    virtual const Card *viewAs() const{
        return new IkChibaoCard;
    }
};

class IkChibao: public DrawCardsSkill {
public:
    IkChibao(): DrawCardsSkill("ikchibao") {
        view_as_skill = new IkChibaoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(zhangliao)) return QStringList();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->getHandcardNum() >= zhangliao->getHandcardNum())
                targets << p;
        int num = qMin(targets.length(), data.toInt());
        if (num > 0)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &data, ServerPlayer *) const{
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->getHandcardNum() >= zhangliao->getHandcardNum())
                targets << p;
        int num = qMin(targets.length(), data.toInt());
        room->setPlayerMark(zhangliao, "ikchibao", num);
        if (room->askForUseCard(zhangliao, "@@ikchibao", "@ikchibao-card:::" + QString::number(num))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        } else
            room->setPlayerMark(zhangliao, "ikchibao", 0);
        return false;
    }

    virtual int getDrawNum(ServerPlayer *zhangliao, int n) const{
        Room *room = zhangliao->getRoom();
        int count = 0;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->hasFlag("IkChibaoTarget")) count++;

        return n - count;
    }
};

class IkChibaoAct: public TriggerSkill {
public:
    IkChibaoAct(): TriggerSkill("#ikchibao") {
        events << AfterDrawNCards << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                p->setFlags("-IkChibaoTarget");
            return QStringList();
        }
        if (zhangliao->getMark("ikchibao") == 0) return QStringList();
        return QStringList(objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &, ServerPlayer *) const{
        room->setPlayerMark(zhangliao, "ikchibao", 0);

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao)) {
            if (p->hasFlag("IkChibaoTarget")) {
                p->setFlags("-IkChibaoTarget");
                targets << p;
            }
        }
        foreach (ServerPlayer *p, targets) {
            if (!zhangliao->isAlive())
                break;
            if (p->isAlive() && !p->isKongcheng()) {
                int card_id = room->askForCardChosen(zhangliao, p, "h", "ikchibao");

                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, zhangliao->objectName());
                room->obtainCard(zhangliao, Sanguosha->getCard(card_id), reason, false);
            }
        }
        return false;
    }
};

class IkLuoyi: public TriggerSkill {
public:
    IkLuoyi(): TriggerSkill("ikluoyi") {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (TriggerSkill::triggerable(player) && change.to == Player::Draw && !player->isSkipped(Player::Draw))
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->skip(Player::Draw, true);
        room->setPlayerMark(player, "@nude", 1);

        QList<int> ids = room->getNCards(3, false);
        CardsMoveStruct move(ids, player, Player::PlaceTable,
                             CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "ikluoyi", QString()));
        room->moveCardsAtomic(move, true);

        room->getThread()->delay();
        room->getThread()->delay();

        QList<int> card_to_throw;
        QList<int> card_to_gotback;
        for (int i = 0; i < 3; i++) {
            const Card *card = Sanguosha->getCard(ids[i]);
            if (card->getTypeId() == Card::TypeBasic || card->isKindOf("Weapon") || card->isKindOf("Duel"))
                card_to_gotback << ids[i];
            else
                card_to_throw << ids[i];
        }
        if (!card_to_throw.isEmpty()) {
            DummyCard *dummy = new DummyCard(card_to_throw);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "ikluoyi", QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
        }
        if (!card_to_gotback.isEmpty()) {
            DummyCard *dummy = new DummyCard(card_to_gotback);
            room->obtainCard(player, dummy);
            delete dummy;
        }
        return false;
    }
};

class IkLuoyiBuff: public TriggerSkill {
public:
    IkLuoyiBuff(): TriggerSkill("#ikluoyi") {
        events << DamageCaused;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *xuchu, QVariant &data, ServerPlayer* &) const{
        if (!xuchu || xuchu->getMark("@nude") == 0 || xuchu->isDead()) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer) return QStringList();
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel")))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#IkLuoyiBuff";
        log.from = xuchu;
        log.to << damage.to;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(++damage.damage);
        room->sendLog(log);

        data = QVariant::fromValue(damage);

        return false;
    }
};

class IkLuoyiClear: public TriggerSkill {
public:
    IkLuoyiClear(): TriggerSkill("#ikluoyi-clear") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (player != NULL && player->isAlive() && player->getPhase() == Player::RoundStart && player->getMark("@nude") > 0)
            room->setPlayerMark(player, "@nude", 0);
        return QStringList();
    }
};

class IkTiandu: public TriggerSkill {
public:
    IkTiandu(): TriggerSkill("iktiandu") {
        frequency = Frequent;
        events << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *guojia, QVariant &data, ServerPlayer* &) const {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (TriggerSkill::triggerable(guojia) && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *guojia, QVariant &data, ServerPlayer *) const {
        if (guojia->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *guojia, QVariant &data, ServerPlayer *) const {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        guojia->obtainCard(judge->card);

        return false;
    }
};

IkYumeng::IkYumeng(): MasochismSkill("ikyumeng") {
    frequency = Frequent;
    n = 2;
}

void IkYumeng::onDamaged(ServerPlayer *guojia, const DamageStruct &damage) const{
    Room *room = guojia->getRoom();
    int x = damage.damage;
    for (int i = 0; i < x; i++) {
        if (!guojia->isAlive() || !room->askForSkillInvoke(guojia, objectName()))
            return;
        room->broadcastSkillInvoke("ikyumeng");

        QList<ServerPlayer *> _guojia;
        _guojia.append(guojia);
        QList<int> yiji_cards = room->getNCards(n, false);

        CardsMoveStruct move(yiji_cards, NULL, guojia, Player::PlaceTable, Player::PlaceHand,
                             CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
        QList<CardsMoveStruct> moves;
        moves.append(move);
        room->notifyMoveCards(true, moves, false, _guojia);
        room->notifyMoveCards(false, moves, false, _guojia);

        QList<int> origin_yiji = yiji_cards;
        while (room->askForYiji(guojia, yiji_cards, objectName(), true, false, true, -1, room->getAlivePlayers())) {
            CardsMoveStruct move(QList<int>(), guojia, NULL, Player::PlaceHand, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
            foreach (int id, origin_yiji) {
                if (room->getCardPlace(id) != Player::DrawPile) {
                    move.card_ids << id;
                    yiji_cards.removeOne(id);
                }
            }
            origin_yiji = yiji_cards;
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);
            if (!guojia->isAlive())
                return;
        }

        if (!yiji_cards.isEmpty()) {
            CardsMoveStruct move(yiji_cards, guojia, NULL, Player::PlaceHand, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);

            DummyCard *dummy = new DummyCard(yiji_cards);
            guojia->obtainCard(dummy, false);
            delete dummy;
        }
    }
}

class IkMengyang: public TriggerSkill {
public:
    IkMengyang(): TriggerSkill("ikmengyang") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (player->getPhase() == Player::Start){
            if (TriggerSkill::triggerable(player))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())){
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        JudgeStruct judge;
        forever {
            judge.pattern = ".|black";
            judge.good = true;
            judge.reason = objectName();
            judge.play_animation = false;
            judge.who = player;
            judge.time_consuming = true;

            room->judge(judge);
            if ((judge.isGood() && !player->askForSkillInvoke(objectName())) || judge.isBad())
                break;
        }

        return false;
    }
};

class IkMengyangMove: public TriggerSkill {
public:
    IkMengyangMove(): TriggerSkill("#ikmengyang-move") {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "ikmengyang") {
                if (judge->isGood()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        room->moveCardTo(judge->card, judge->who, Player::PlaceHand, true);

        return false;
    }
};

class IkZhongyan: public OneCardViewAsSkill {
public:
    IkZhongyan(): OneCardViewAsSkill("ikzhongyan") {
        filter_pattern = ".|black|.|hand";
        response_pattern = "jink";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(originalCard->getId());
        return jink;
    }
};

class IkXunxun: public PhaseChangeSkill {
public:
    IkXunxun(): PhaseChangeSkill("ikxunxun") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Draw;
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())){
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *lidian) const{
        Room *room = lidian->getRoom();

        QList<ServerPlayer *> p_list;
        p_list << lidian;
        QList<int> card_ids = room->getNCards(4);
        QList<int> obtained;
        room->fillAG(card_ids, lidian);
        int id1 = room->askForAG(lidian, card_ids, false, objectName());
        card_ids.removeOne(id1);
        obtained << id1;
        room->takeAG(lidian, id1, false, p_list);
        int id2 = room->askForAG(lidian, card_ids, false, objectName());
        card_ids.removeOne(id2);
        obtained << id2;
        room->clearAG(lidian);

        room->askForGuanxing(lidian, card_ids, Room::GuanxingDownOnly);
        DummyCard *dummy = new DummyCard(obtained);
        lidian->obtainCard(dummy, false);
        delete dummy;

        return true;
    }
};

class IkWangxi: public TriggerSkill {
public:
    IkWangxi(): TriggerSkill("ikwangxi") {
        events << Damage << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = NULL;
        if (triggerEvent == Damage && !damage.to->hasFlag("Global_DebutFlag"))
            target = damage.to;
        else if (triggerEvent == Damaged)
            target = damage.from;
        if (!target || target == player || target->isDead()) return QStringList();
        QStringList skill;
        for (int i = 1; i <= damage.damage; i++)
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = NULL;
        if (triggerEvent == Damage)
            target = damage.to;
        else if (triggerEvent == Damaged)
            target = damage.from;
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = NULL;
        if (triggerEvent == Damage)
            target = damage.to;
        else if (triggerEvent == Damaged)
            target = damage.from;
        QList<ServerPlayer *> players;
        players << player << target;
        room->sortByActionOrder(players);

        room->drawCards(players, 1, objectName());

        return false;
    }
};

IkZhihengCard::IkZhihengCard() {
    target_fixed = true;
}

void IkZhihengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    if (source->isAlive())
        room->drawCards(source, subcards.length(), "ikzhiheng");
}

class IkZhiheng: public ViewAsSkill {
public:
    IkZhiheng(): ViewAsSkill("ikzhiheng") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() >= Self->getMaxHp())
            return false;
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        IkZhihengCard *ikzhiheng_card = new IkZhihengCard;
        ikzhiheng_card->addSubcards(cards);
        ikzhiheng_card->setSkillName(objectName());
        return ikzhiheng_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("IkZhihengCard");
    }
};

class IkJiyuan: public TriggerSkill {
public:
    IkJiyuan(): TriggerSkill("ikjiyuan$") {
        events << TargetSpecified << PreHpRecover;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Peach") && player->getKingdom() == "yuki") {
                foreach (ServerPlayer *p, use.to) {
                    if (player == p) continue;
                    if (p->hasLordSkill("ikjiyuan"))
                        room->setCardFlag(use.card, "ikjiyuan");
                }
            }
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct rec = data.value<RecoverStruct>();
            if (rec.card && rec.card->hasFlag("ikjiyuan"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *sunquan, QVariant &data, ServerPlayer *) const{
        RecoverStruct rec = data.value<RecoverStruct>();

        room->notifySkillInvoked(sunquan, "ikjiyuan");
        room->broadcastSkillInvoke("ikjiyuan");

        LogMessage log;
        log.type = "#IkJiyuanExtraRecover";
        log.from = sunquan;
        log.to << rec.who;
        log.arg = objectName();
        room->sendLog(log);

        rec.recover++;
        data = QVariant::fromValue(rec);

        return false;
    }
};

class IkKuipo: public OneCardViewAsSkill {
public:
    IkKuipo(): OneCardViewAsSkill("ikkuipo") {
        filter_pattern = ".|black";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Dismantlement *dismantlement = new Dismantlement(originalCard->getSuit(), originalCard->getNumber());
        dismantlement->addSubcard(originalCard->getId());
        dismantlement->setSkillName(objectName());
        return dismantlement;
    }
};

IkGuisiCard::IkGuisiCard() {
}

bool IkGuisiCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const{
    QStringList targetslist = Self->property("ikguisi_targets").toString().split("+");
    return targetslist.contains(to_select->objectName());
}

void IkGuisiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->removePlayerMark(source, "@guisi");
    room->addPlayerMark(source, "@guisiused");

    CardUseStruct use = source->tag["ikguisi"].value<CardUseStruct>();
    foreach (ServerPlayer *p, targets)
        use.nullified_list << p->objectName();
    source->tag["ikguisi"] = QVariant::fromValue(use);
}

class IkGuisiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkGuisiViewAsSkill():ZeroCardViewAsSkill("ikguisi") {
        response_pattern = "@@ikguisi";
    }

    virtual const Card *viewAs() const{
        return new IkGuisiCard;
    }
};

class IkGuisi: public TriggerSkill {
public:
    IkGuisi(): TriggerSkill("ikguisi") {
        events << TargetSpecifying;
        view_as_skill = new IkGuisiViewAsSkill;
        frequency = Limited;
        limit_mark = "@guisi";
    }

    virtual QMap<ServerPlayer *,QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        QMap<ServerPlayer *,QStringList> skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() <= 1 || !use.card->isNDTrick())
            return skill_list;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            if (p->getMark("@guisi") > 0)
                skill_list.insert(p, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ganning) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList target_list;
        foreach (ServerPlayer *p, use.to)
            target_list << p->objectName();
        room->setPlayerProperty(ganning, "ikguisi_targets", target_list.join("+"));
        ganning->tag["ikguisi"] = data;
        room->askForUseCard(ganning, "@@ikguisi", "@ikguisi-card");
        data = ganning->tag["ikguisi"];

        return false;
    }
};

class IkBiju: public TriggerSkill {
public:
    IkBiju(): TriggerSkill("ikbiju") {
        events << EventPhaseChanging;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *lvmeng, QVariant &data, ServerPlayer* &) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (TriggerSkill::triggerable(lvmeng) && change.to == Player::Discard && !lvmeng->hasFlag("IkBijuSlashInPlayPhase"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &, ServerPlayer *) const{
        if (lvmeng->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &, ServerPlayer *) const{
        QStringList choices;
        choices << "draw";
        if (!lvmeng->isSkipped(Player::Discard))
            choices << "skip";
        if (room->askForChoice(lvmeng, objectName(), choices.join("+")) == "skip")
            lvmeng->skip(Player::Discard);
        else
            lvmeng->drawCards(1, objectName());
        return false;
    }
};

class IkBijuRecord: public TriggerSkill {
public:
    IkBijuRecord(): TriggerSkill("#ikbiju-record") {
        events << PreCardUsed << CardResponded;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *lvmeng, QVariant &data, ServerPlayer* &) const{
        if (lvmeng->getPhase() == Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;
            if (card->isKindOf("Slash"))
                lvmeng->setFlags("IkBijuSlashInPlayPhase");
        }
        return QStringList();
    }
};

class IkPojian: public TriggerSkill {
public:
    IkPojian(): TriggerSkill("ikpojian") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && target->getMark("ikpojian") >= 4
            && target->getMark("@pojian") == 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#IkPojianWake";
        log.from = player;
        log.arg = QString::number(player->getMark("ikpojian"));
        log.arg2 = objectName();
        room->sendLog(log);
        room->addPlayerMark(player, "@pojian");

        room->changeMaxHpForAwakenSkill(player);
        if (player->isWounded() && room->askForChoice(player, objectName(), "recover+draw") == "recover")
            room->recover(player, RecoverStruct(player));
        else
            player->drawCards(1, objectName());

        room->acquireSkill(player, "ikqinghua");
        return false;
    }
};

class IkPojianRecord: public TriggerSkill {
public:
    IkPojianRecord(): TriggerSkill("#ikpojian-record") {
        events << PreCardUsed << CardResponded << EventPhaseStart;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *lvmeng, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseStart) {
            if (lvmeng->getPhase() == Player::RoundStart)
                lvmeng->setMark("ikpojian", 0);
        } else {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (card && !card->isKindOf("EquipCard"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *lvmeng, QVariant &, ServerPlayer *) const{
        lvmeng->addMark("ikpojian");
        return false;
    }
};

IkQinghuaCard::IkQinghuaCard() {
}

bool IkQinghuaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void IkQinghuaCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    const Card *card = room->askForCardShow(effect.to, effect.from, "ikqinghua");
    room->showCard(effect.to, card->getEffectiveId());
    QString suit = card->getSuitString();
    if (room->askForCard(effect.from, ".|" + suit, "@ikqinghua-discard:::" + suit)) {
        room->throwCard(card, effect.to);
        QList<ServerPlayer *> targets;
        targets << effect.from << effect.to;
        room->sortByActionOrder(targets);
        foreach (ServerPlayer *p, targets)
            room->recover(p, RecoverStruct(effect.from));
    }
}

class IkQinghua: public ZeroCardViewAsSkill {
public:
    IkQinghua(): ZeroCardViewAsSkill("ikqinghua") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkQinghuaCard");
    }

    virtual const Card *viewAs() const{
        return new IkQinghuaCard;
    }
};

IkKurouCard::IkKurouCard() {
    target_fixed = true;
}

void IkKurouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->loseHp(source);
    if (source->isAlive())
        room->drawCards(source, 2, "ikkurou");
}

class IkKurou: public ZeroCardViewAsSkill {
public:
    IkKurou(): ZeroCardViewAsSkill("ikkurou") {
    }

    virtual const Card *viewAs() const{
        return new IkKurouCard;
    }
};

class IkZaiqi: public TriggerSkill {
public:
    IkZaiqi(): TriggerSkill("ikzaiqi") {
        events << HpRecover;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::Play || !player->hasFlag("Global_Dying"))
            return QStringList();
        QStringList skill;
        RecoverStruct recover;
        for (int i = 0; i < recover.recover; i++)
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(1, objectName());
        return true;
    }
};

IkGuidengCard::IkGuidengCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void IkGuidengCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();
    Card::Suit suit = getSuit();

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, zhouyu->objectName(), target->objectName(), "ikguideng", QString());
    room->obtainCard(target, this, reason);

    if (target->isAlive()) {
        if (target->isNude()) {
            room->loseHp(target);
        } else {
            if (room->askForSkillInvoke(target, "ikguideng_discard", "prompt:::" + Card::Suit2String(suit))) {
                room->showAllCards(target);
                DummyCard *dummy = new DummyCard;
                foreach (const Card *card, target->getCards("he")) {
                    if (card->getSuit() == suit)
                        dummy->addSubcard(card);
                }
                if (dummy->subcardsLength() > 0)
                    room->throwCard(dummy, target);
                delete dummy;
            } else {
                room->loseHp(target);
            }
        }
    }
}

class IkGuideng: public OneCardViewAsSkill {
public:
    IkGuideng(): OneCardViewAsSkill("ikguideng") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("IkGuidengCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkGuidengCard *card = new IkGuidengCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class IkChenhong: public DrawCardsSkill {
public:
    IkChenhong(): DrawCardsSkill("ikchenhong") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *zhouyu, int n) const{
        Room *room = zhouyu->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(zhouyu, objectName());

        return n + 1;
    }
};

class IkChenhongMaxCards: public MaxCardsSkill {
public:
    IkChenhongMaxCards(): MaxCardsSkill("#ikchenhong") {
    }

    virtual int getFixed(const Player *target) const{
        if (target->hasSkill("ikchenhong"))
            return target->getMaxHp();
        else
            return -1;
    }
};

IkWanmeiCard::IkWanmeiCard() {
    handling_method = Card::MethodNone;
}

bool IkWanmeiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty()) return false;
    int id = getEffectiveId();

    Indulgence *indulgence = new Indulgence(getSuit(), getNumber());
    indulgence->addSubcard(id);
    indulgence->setSkillName("ikwanmei");
    indulgence->deleteLater();

    bool canUse = !Self->isLocked(indulgence);
    if (canUse && to_select != Self && !to_select->containsTrick("indulgence") && !Self->isProhibited(to_select, indulgence))
        return true;
    bool canDiscard = false;
    foreach (const Card *card, Self->getHandcards()) {
        if (card->getEffectiveId() == id && !Self->isJilei(Sanguosha->getCard(id))) {
            canDiscard = true;
            break;
        }
    }
    if (!canDiscard || !to_select->containsTrick("indulgence"))
        return false;
    foreach (const Card *card, to_select->getJudgingArea()) {
        if (card->isKindOf("Indulgence") && Self->canDiscard(to_select, card->getEffectiveId()))
            return true;
    }
    return false;
}

const Card *IkWanmeiCard::validate(CardUseStruct &cardUse) const{
    ServerPlayer *to = cardUse.to.first();
    if (!to->containsTrick("indulgence")) {
        Indulgence *indulgence = new Indulgence(getSuit(), getNumber());
        indulgence->addSubcard(getEffectiveId());
        indulgence->setSkillName("ikwanmei");
        return indulgence;
    }
    return this;
}

void IkWanmeiCard::onUse(Room *room, const CardUseStruct &use) const{
    CardUseStruct card_use = use;

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();
    thread->trigger(PreCardUsed, room, card_use.from, data);
    card_use = data.value<CardUseStruct>();

    LogMessage log;
    log.from = card_use.from;
    log.to = card_use.to;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString();
    room->sendLog(log);

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), "ikwanmei", QString());
    room->moveCardTo(this, card_use.from, NULL, Player::DiscardPile, reason, true);

    thread->trigger(CardUsed, room, card_use.from, data);
    card_use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, card_use.from, data);
}

void IkWanmeiCard::onEffect(const CardEffectStruct &effect) const{
    foreach (const Card *judge, effect.to->getJudgingArea()) {
        if (judge->isKindOf("Indulgence") && effect.from->canDiscard(effect.to, judge->getEffectiveId())) {
            effect.from->getRoom()->throwCard(judge, NULL, effect.from);
            effect.from->drawCards(1, "ikawanmei");
            return;
        }
    }
}

class IkWanmeiViewAsSkill: public OneCardViewAsSkill {
public:
    IkWanmeiViewAsSkill(): OneCardViewAsSkill("ikwanmei") {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkWanmeiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkWanmeiCard *card = new IkWanmeiCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class IkWanmei: public TriggerSkill {
public:
    IkWanmei(): TriggerSkill("ikwanmei") {
        events << CardFinished;
        view_as_skill = new IkWanmeiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Indulgence") && use.card->getSkillName() == objectName())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(1, objectName());
        return false;
    }
};

IkXuanhuoCard::IkXuanhuoCard() {
}

bool IkXuanhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty())
        return false;

    if (to_select->hasFlag("IkXuanhuoSlashSource") || to_select == Self)
        return false;

    const Player *from = NULL;
    foreach (const Player *p, Self->getAliveSiblings()) {
        if (p->hasFlag("IkXuanhuoSlashSource")) {
            from = p;
            break;
        }
    }

    const Card *slash = Card::Parse(Self->property("ikxuanhuo").toString());
    if (from && !from->canSlash(to_select, slash, false))
        return false;

    int card_id = subcards.first();
    int range_fix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getId() == card_id) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        range_fix += weapon->getRange() - Self->getAttackRange(false);
    } else if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getId() == card_id) {
        range_fix += 1;
    }

    return Self->inMyAttackRange(to_select, range_fix);
}

void IkXuanhuoCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->setFlags("IkXuanhuoTarget");
}

class IkXuanhuoViewAsSkill: public OneCardViewAsSkill {
public:
    IkXuanhuoViewAsSkill(): OneCardViewAsSkill("ikxuanhuo") {
        filter_pattern = ".!";
        response_pattern = "@@ikxuanhuo";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkXuanhuoCard *ikxuanhuo_card = new IkXuanhuoCard;
        ikxuanhuo_card->addSubcard(originalCard);
        return ikxuanhuo_card;
    }
};

class IkXuanhuo: public TriggerSkill {
public:
    IkXuanhuo(): TriggerSkill("ikxuanhuo") {
        events << TargetConfirming;
        view_as_skill = new IkXuanhuoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(daqiao)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isKindOf("Slash") && use.to.contains(daqiao) && daqiao->canDiscard(daqiao, "he")) {
            QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
            players.removeOne(use.from);

            foreach (ServerPlayer *p, players) {
                if (use.from->canSlash(p, use.card, false) && daqiao->inMyAttackRange(p))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "@ikxuanhuo:" + use.from->objectName();
        room->setPlayerFlag(use.from, "IkXuanhuoSlashSource");
        // a temp nasty trick
        daqiao->tag["ikxuanhuo-card"] = QVariant::fromValue(use.card); // for the server (AI)
        room->setPlayerProperty(daqiao, "ikxuanhuo", use.card->toString()); // for the client (UI)
        if (room->askForUseCard(daqiao, "@@ikxuanhuo", prompt, -1, Card::MethodDiscard))
            return true;
        else {
            daqiao->tag.remove("ikxuanhuo-card");
            room->setPlayerProperty(daqiao, "ikxuanhuo", QString());
            room->setPlayerFlag(use.from, "-IkXuanhuoSlashSource");
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
        players.removeOne(use.from);

        daqiao->tag.remove("ikxuanhuo-card");
        room->setPlayerProperty(daqiao, "ikxuanhuo", QString());
        room->setPlayerFlag(use.from, "-IkXuanhuoSlashSource");
        foreach (ServerPlayer *p, players) {
            if (p->hasFlag("IkXuanhuoTarget")) {
                p->setFlags("-IkXuanhuoTarget");
                if (!use.from->canSlash(p, false))
                    return false;
                use.to.removeOne(daqiao);
                use.to.append(p);
                room->sortByActionOrder(use.to);
                data = QVariant::fromValue(use);
                room->getThread()->trigger(TargetConfirming, room, p, data);
                return false;
            }
        }

        return false;
    }
};

class IkWujie: public TriggerSkill {
public:
    IkWujie(): TriggerSkill("ikwujie") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *luxun, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(luxun)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == luxun && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *luxun, QVariant &, ServerPlayer *) const{
        if (luxun->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *luxun, QVariant &, ServerPlayer *) const{
        luxun->drawCards(1, objectName());
        return false;
    }
};

IkYuanheCard::IkYuanheCard() {
}

void IkYuanheCard::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    use.to << use.from;
    SkillCard::onUse(room, use);
}

void IkYuanheCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *p, targets)
        p->drawCards(2, "ikyuanhe");
    foreach (ServerPlayer *p, targets)
        room->askForDiscard(p, "ikyuanhe", 2, 2, false, true);
}

class IkYuanhe: public OneCardViewAsSkill {
public:
    IkYuanhe(): OneCardViewAsSkill("ikyuanhe") {
        filter_pattern = ".|red|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkYuanheCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkYuanheCard *await = new IkYuanheCard;
        await->addSubcard(originalcard->getId());
        return await;
    }
};

IkHuanluCard::IkHuanluCard() {
}

bool IkHuanluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty())
        return false;

    return to_select->isMale() && to_select->isWounded() && to_select != Self;
}

void IkHuanluCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    RecoverStruct recover(effect.from);
    room->recover(effect.from, recover, true);
    room->recover(effect.to, recover, true);
}

class IkHuanlu: public ViewAsSkill {
public:
    IkHuanlu(): ViewAsSkill("ikhuanlu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() >= 2 && !player->hasUsed("IkHuanluCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() > 1 || Self->isJilei(to_select))
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        IkHuanluCard *cuilu_card = new IkHuanluCard();
        cuilu_card->addSubcards(cards);
        return cuilu_card;
    }
};

class IkCangyou: public TriggerSkill {
public:
    IkCangyou(): TriggerSkill("ikcangyou") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceEquip))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *sunshangxiang, QVariant &, ServerPlayer *) const{
        if (sunshangxiang->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *sunshangxiang, QVariant &, ServerPlayer *) const{
        sunshangxiang->drawCards(2, objectName());
        return false;
    }
};

class IkWushuang: public TriggerSkill {
public:
    IkWushuang(): TriggerSkill("ikwushuang") {
        events << TargetSpecified << CardFinished;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Duel")) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->tag.remove("IkWushuang_" + use.card->toString());
            }
        } else if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && TriggerSkill::triggerable(player))
                skill_list.insert(player, QStringList(objectName()));
            else if (use.card->isKindOf("Duel")) {
                if (TriggerSkill::triggerable(player))
                    skill_list.insert(player, QStringList(objectName()));
                foreach (ServerPlayer *p, use.to.toSet())
                    if (TriggerSkill::triggerable(p))
                        skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            room->sendCompulsoryTriggerLog(ask_who, objectName());
            room->broadcastSkillInvoke(objectName());

            QVariantList jink_list = ask_who->tag["Jink_" + use.card->toString()].toList();
            for (int i = 0; i < use.to.length(); i++) {
                if (jink_list.at(i).toInt() == 1)
                    jink_list.replace(i, QVariant(2));
            }
            ask_who->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        } else if (use.card->isKindOf("Duel")) {
            if (use.from == ask_who) {
                room->sendCompulsoryTriggerLog(ask_who, objectName());
                room->broadcastSkillInvoke(objectName());

                QStringList ikwushuang_tag;
                foreach (ServerPlayer *to, use.to)
                    ikwushuang_tag << to->objectName();
                ask_who->tag["IkWushuang_" + use.card->toString()] = ikwushuang_tag;
            } else {
                room->sendCompulsoryTriggerLog(ask_who, objectName());
                room->broadcastSkillInvoke(objectName());

                ask_who->tag["IkWushuang_" + use.card->toString()] = QStringList(use.from->objectName());
            }
        }

        return false;
    }
};

IkWudiCard::IkWudiCard() {
    will_throw = false;
}

bool IkWudiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Duel *duel = new Duel(SuitToBeDecided, -1);
    duel->addSubcards(subcards);
    duel->deleteLater();
    return duel->targetFilter(targets, to_select, Self);
}

const Card *IkWudiCard::validate(CardUseStruct &) const{
    Duel *duel = new Duel(SuitToBeDecided, -1);
    duel->addSubcards(subcards);
    duel->setSkillName("ikwudi");
    return duel;
}

class IkWudi: public ViewAsSkill {
public:
    IkWudi(): ViewAsSkill("ikwudi") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkWudiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.isEmpty())
            return !to_select->isEquipped();
        else if (selected.length() == 1) {
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 2) {
            Duel *duel = new Duel(Card::SuitToBeDecided, -1);
            duel->addSubcards(cards);
            duel->setSkillName(objectName());
            duel->deleteLater();
            if (duel->isAvailable(Self)) {
                IkWudiCard *card = new IkWudiCard;
                card->addSubcards(cards);
                return card;
            }
        }
        return NULL;
    }
};

IkQingguoCard::IkQingguoCard(){
    mute = true;
}

bool IkQingguoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!to_select->isMale())
        return false;

    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->deleteLater();
    if (targets.isEmpty() && Self->isProhibited(to_select, duel))
        return false;

    if (targets.length() == 1 && to_select->isCardLimited(duel, Card::MethodUse))
        return false;

    return targets.length() < 2 && to_select != Self;
}

bool IkQingguoCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

void IkQingguoCard::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, card_use.from, data);
    use = data.value<CardUseStruct>();

    room->broadcastSkillInvoke("ikqingguo");

    LogMessage log;
    log.from = use.from;
    log.to << use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, use.from->objectName(), QString(), "ikqingguo", QString());
    room->moveCardTo(this, use.from, NULL, Player::DiscardPile, reason, true);

    thread->trigger(CardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, use.from, data);
}

void IkQingguoCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const{
    ServerPlayer *to = targets.at(0);
    ServerPlayer *from = targets.at(1);

    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->setCancelable(false);
    duel->setSkillName("_ikqingguo");
    if (!from->isCardLimited(duel, Card::MethodUse) && !from->isProhibited(to, duel))
        room->useCard(CardUseStruct(duel, from, to));
    else
        delete duel;
}

class IkQingguo: public OneCardViewAsSkill {
public:
    IkQingguo(): OneCardViewAsSkill("ikqingguo") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getAliveSiblings().length() > 1
               && player->canDiscard(player, "he") && !player->hasUsed("IkQingguoCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkQingguoCard *lijian_card = new IkQingguoCard;
        lijian_card->addSubcard(originalCard->getId());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

class IkBiyue: public PhaseChangeSkill {
public:
    IkBiyue(): PhaseChangeSkill("ikbiyue") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *diaochan) const{
        diaochan->drawCards(1, objectName());
        return false;
    }
};

class IkHuichun: public OneCardViewAsSkill {
public:
    IkHuichun(): OneCardViewAsSkill("ikhuichun") {
        filter_pattern = ".|red";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.contains("peach") && player->getMark("Global_PreventPeach") == 0
                && player->getPhase() == Player::NotActive && player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
        peach->addSubcard(originalCard->getId());
        peach->setSkillName(objectName());
        return peach;
    }
};

IkQingnangCard::IkQingnangCard() {
}

bool IkQingnangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.isEmpty() && to_select->isWounded();
}

bool IkQingnangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.value(0, Self)->isWounded();
}

void IkQingnangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.value(0, source);
    room->cardEffect(this, source, target);
}

void IkQingnangCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->getRoom()->recover(effect.to, RecoverStruct(effect.from));
}

class IkQingnang: public OneCardViewAsSkill {
public:
    IkQingnang(): OneCardViewAsSkill("ikqingnang") {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "h") && !player->hasUsed("IkQingnangCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkQingnangCard *qingnang_card = new IkQingnangCard;
        qingnang_card->addSubcard(originalCard->getId());
        return qingnang_card;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        return player->getGeneral()->objectName() == "wind003" ? (qrand() % 2 + 1) : 0;
    }
};

IkZiqiangCard::IkZiqiangCard() {
    target_fixed = true;
}

void IkZiqiangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->loseHp(source);
}

class IkZiqiang: public OneCardViewAsSkill {
public:
    IkZiqiang(): OneCardViewAsSkill("ikziqiang") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkZiqiangCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkZiqiangCard *card = new IkZiqiangCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class IkLingshili: public TriggerSkill {
public:
    IkLingshili(): TriggerSkill("iklingshili") {
        events << HpLost << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (triggerEvent == HpLost && TriggerSkill::triggerable(player)) {
            int lose = data.toInt();
            for (int i = 0; i < lose; i++)
                skill << objectName();
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive || change.to == Player::RoundStart)
                room->setPlayerMark(player, objectName(), 0);
        }
        return skill;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        player->drawCards(3, objectName());
        if (player->getPhase() == Player::Play)
            room->addPlayerMark(player, objectName());

        return false;
    }
};

class IkLingshiliRedSlash: public TriggerSkill {
public:
    IkLingshiliRedSlash(): TriggerSkill("#iklingshili") {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player && player->isAlive() && player->getMark("iklingshili") > 0) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash") || !use.card->isRed())
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, "iklingshili");

        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            LogMessage log;
            log.type = "#NoJink";
            log.from = p;
            room->sendLog(log);
            jink_list.replace(index, QVariant(0));
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class IkLingshiliTargetMod: public TargetModSkill {
public:
    IkLingshiliTargetMod(): TargetModSkill("#iklingshili-target") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        return from->getMark("iklingshili");
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (card->isRed() && from->getMark("iklingshili") > 0)
            return 1000;
        else
            return 0;
    }
};

class IkZhuji: public DistanceSkill {
public:
    IkZhuji(): DistanceSkill("ikzhuji") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        int correct = 0;
        if (from->hasSkill(objectName()) && from->getHp() > 2)
            correct--;
        if (to->hasSkill(objectName()) && to->getHp() <= 2)
            correct++;

        return correct;
    }
};

class IkZhujiEffect: public TriggerSkill {
public:
    IkZhujiEffect(): TriggerSkill("#ikzhuji-effect") {
        events << HpChanged;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        int hp = player->getHp();
        int index = 0;
        int reduce = 0;
        if (data.canConvert<RecoverStruct>()) {
            int rec = data.value<RecoverStruct>().recover;
            if (hp > 2 && hp - rec < 2)
                index = 1;
        } else {
            if (data.canConvert<DamageStruct>()) {
                DamageStruct damage = data.value<DamageStruct>();
                reduce = damage.damage;
            } else if (!data.isNull()) {
                reduce = data.toInt();
            }
            if (hp <= 2 && hp + reduce > 2)
                index = 2;
        }
        if (player->getGeneralName() == "gongsunzan"
            || (player->getGeneralName() != "st_gongsunzan" && player->getGeneral2Name() == "gongsunzan"))
            index += 2;

        if (index > 0)
            room->broadcastSkillInvoke("ikzhuji", index);
        return false;
    }
};

class IkBenyin : public OneCardViewAsSkill
{
public:
    IkBenyin() : OneCardViewAsSkill("ikbenyin")
    {
        filter_pattern = "BasicCard|red";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        PurpleSong *ps = new PurpleSong(originalCard->getSuit(), originalCard->getNumber());
        ps->addSubcard(originalCard);
        ps->setSkillName(objectName());
        return ps;
    }
};

class IkGuijiao: public TriggerSkill {
public:
    IkGuijiao(): TriggerSkill("ikguijiao") {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
        if (player->getPhase() == Player::Start) {
            if (player->getMark("@wayward") > 0) {
                foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                    skill_list.insert(owner, QStringList(objectName()));
            } else {
                bool can_invoke = true;
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (p->getMark("@wayward") > 0) {
                        can_invoke = false;
                        break;
                    }
                if (can_invoke && TriggerSkill::triggerable(player))
                    skill_list.insert(player, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        if (player->getMark("@wayward") > 0) {
            if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        } else {
            ServerPlayer *target = room->askForPlayerChosen(ask_who, room->getOtherPlayers(ask_who), objectName(), "@ikguijiao", true, true);
            if (target) {
                ask_who->tag["ThGuijiaoTarget"] = QVariant::fromValue(target);
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        if (player->getMark("@wayward") > 0) {
            ask_who->drawCards(1, objectName());
            room->setPlayerFlag(player, "IkGuijiaoDecMaxCards");
        } else {
            ServerPlayer *target = ask_who->tag["ThGuijiaoTarget"].value<ServerPlayer *>();
            ask_who->tag.remove("ThGuijiaoTarget");
            if (target)
                target->gainMark("@wayward");
        }
        return false;
    }
};

class IkGuijiaoMaxCards: public MaxCardsSkill {
public:
    IkGuijiaoMaxCards(): MaxCardsSkill("#ikguijiao-maxcard") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasFlag("IkGuijiaoDecMaxCards"))
            return -1;
        else
            return 0;
    }
};

class IkJinlian: public ProhibitSkill {
public:
    IkJinlian(): ProhibitSkill("ikjinlian") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        if (card->isKindOf("Slash")) {
            // get rangefix
            int rangefix = 0;
            if (card->isVirtualCard()) {
                QList<int> subcards = card->getSubcards();
                if (from->getWeapon() && subcards.contains(from->getWeapon()->getId())) {
                    const Weapon *weapon = qobject_cast<const Weapon *>(from->getWeapon()->getRealCard());
                    rangefix += weapon->getRange() - from->getAttackRange(false);
                }

                if (from->getOffensiveHorse() && subcards.contains(from->getOffensiveHorse()->getId()))
                    rangefix += 1;
            }
            // find yuanshu
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->hasSkill(objectName()) && p != to && p->getHandcardNum() > p->getHp()
                    && from->inMyAttackRange(p, rangefix)) {
                    return true;
                }
            }
        }
        return false;
    }
};

IkYaogeCard::IkYaogeCard() {
}

bool IkYaogeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (to_select == Self) return false;
    QSet<QString> kingdoms;
    foreach (const Player *p, targets)
        kingdoms << p->getKingdom();
    return Self->canDiscard(to_select, "he") && !kingdoms.contains(to_select->getKingdom());
}

void IkYaogeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> draw_card;
    if (Sanguosha->getCard(getEffectiveId())->getSuit() == Card::Spade)
        draw_card << source;
    foreach (ServerPlayer *target, targets) {
        if (!source->canDiscard(target, "he")) continue;
        int id = room->askForCardChosen(source, target, "he", "ikyaoge", false, Card::MethodDiscard);
        if (Sanguosha->getCard(id)->getSuit() == Card::Spade)
            draw_card << target;
        room->throwCard(id, target, source);
    }

    foreach (ServerPlayer *p, draw_card)
        room->drawCards(p, 1, "ikyaoge");
}

class IkYaoge: public OneCardViewAsSkill {
public:
    IkYaoge(): OneCardViewAsSkill("ikyaoge") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("IkYaogeCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkYaogeCard *yaoge_card = new IkYaogeCard;
        yaoge_card->addSubcard(originalCard);
        return yaoge_card;
    }
};

IkaiDoPackage::IkaiDoPackage()
    :Package("ikai-do")
{
    General *wind001 = new General(this, "wind001$", "kaze");
    wind001->addSkill(new IkShenai);
    wind001->addSkill(new IkXinqi);

    General *wind002 = new General(this, "wind002", "kaze");
    wind002->addSkill(new IkChilian);
    wind002->addSkill(new IkZhenhong);
    wind002->addSkill(new IkZhenhongTargetMod);
    related_skills.insertMulti("ikzhenhong", "#ikzhenhong-target");

    General *wind003 = new General(this, "wind003", "kaze");
    wind003->addSkill(new IkYipao);
    wind003->addSkill(new IkShijiu);
    wind003->addRelateSkill("ikqingnang");

    General *wind004 = new General(this, "wind004", "kaze", 3);
    wind004->addSkill(new IkYuxi);
    wind004->addSkill(new IkJingyou);

    General *wind006 = new General(this, "wind006", "kaze");
    wind006->addSkill("thjibu");
    wind006->addSkill(new IkYufeng);
    wind006->addSkill(new IkYufengClear);
    related_skills.insertMulti("ikyufeng", "#ikyufeng-clear");

    General *wind007 = new General(this, "wind007", "kaze", 3, false);
    wind007->addSkill(new IkHuiquan);
    wind007->addSkill("thjizhi");
    wind007->addSkill(new Skill("ikhugu", Skill::Compulsory)); // Player::canDiscard

    General *wind042 = new General(this, "wind042", "kaze");
    wind042->addSkill(new IkBaoou);
    wind042->addSkill(new IkBaoouRecord);
    wind042->addSkill(new IkYehua);
    wind042->addRelateSkill("ikxingyu");
    related_skills.insertMulti("ikbaoou", "#ikbaoou-record");

    General *bloom001 = new General(this, "bloom001$", "hana");
    bloom001->addSkill(new IkJiaoman);
    bloom001->addSkill(new IkHuanwei);

    General *bloom002 = new General(this, "bloom002", "hana", 3);
    bloom002->addSkill(new IkTiansuo);
    bloom002->addSkill(new IkHuanji);

    General *bloom003 = new General(this, "bloom003", "hana");
    bloom003->addSkill(new IkAoli);
    bloom003->addSkill(new IkQingjian);

    General *bloom004 = new General(this, "bloom004", "hana");
    bloom004->addSkill(new IkChibao);
    bloom004->addSkill(new IkChibaoAct);
    related_skills.insertMulti("ikchibao", "#ikchibao");

    General *bloom005 = new General(this, "bloom005", "hana");
    bloom005->addSkill(new IkLuoyi);
    bloom005->addSkill(new IkLuoyiBuff);
    bloom005->addSkill(new IkLuoyiClear);
    related_skills.insertMulti("ikluoyi", "#ikluoyi");
    related_skills.insertMulti("ikluoyi", "#ikluoyi-clear");

    General *bloom006 = new General(this, "bloom006", "hana", 3);
    bloom006->addSkill(new IkTiandu);
    bloom006->addSkill(new IkYumeng);

    General *bloom007 = new General(this, "bloom007", "hana", 3, false);
    bloom007->addSkill(new IkMengyang);
    bloom007->addSkill(new IkMengyangMove);
    related_skills.insertMulti("ikmengyang", "#ikmengyang-move");
    bloom007->addSkill(new IkZhongyan);

    General *bloom042 = new General(this, "bloom042", "hana", 3);
    bloom042->addSkill(new IkXunxun);
    bloom042->addSkill(new IkWangxi);

    General *snow001 = new General(this, "snow001$", "yuki");
    snow001->addSkill(new IkZhiheng);
    snow001->addSkill(new IkJiyuan);

    General *snow002 = new General(this, "snow002", "yuki");
    snow002->addSkill(new IkKuipo);
    snow002->addSkill(new IkGuisi);

    General *snow003 = new General(this, "snow003", "yuki");
    snow003->addSkill(new IkBiju);
    snow003->addSkill(new IkBijuRecord);
    related_skills.insertMulti("ikbiju", "#ikbiju-record");
    snow003->addSkill(new IkPojian);
    snow003->addSkill(new IkPojianRecord);
    related_skills.insertMulti("ikpojian", "#ikpojian-record");
    snow003->addRelateSkill("ikqinghua");

    General *snow004 = new General(this, "snow004", "yuki");
    snow004->addSkill(new IkKurou);
    snow004->addSkill(new IkZaiqi);

    General *snow005 = new General(this, "snow005", "yuki", 3);
    snow005->addSkill(new IkGuideng);
    snow005->addSkill(new IkChenhong);
    snow005->addSkill(new IkChenhongMaxCards);
    related_skills.insertMulti("ikchenhong", "#ikchenhong");

    General *snow006 = new General(this, "snow006", "yuki", 3, false);
    snow006->addSkill(new IkWanmei);
    snow006->addSkill(new IkXuanhuo);

    General *snow007 = new General(this, "snow007", "yuki", 3);
    snow007->addSkill(new IkWujie);
    snow007->addSkill(new IkYuanhe);

    General *snow008 = new General(this, "snow008", "yuki", 3, false);
    snow008->addSkill(new IkHuanlu);
    snow008->addSkill(new IkCangyou);

    General *snow042 = new General(this, "snow042", "yuki");
    snow042->addSkill(new IkZiqiang);
    snow042->addSkill(new IkLingshili);
    snow042->addSkill(new IkLingshiliRedSlash);
    snow042->addSkill(new IkLingshiliTargetMod);
    related_skills.insertMulti("iklingshili", "#iklingshili");
    related_skills.insertMulti("iklingshili", "#iklingshili-target");

    General *luna002 = new General(this, "luna002", "tsuki");
    luna002->addSkill(new IkWushuang);
    luna002->addSkill(new IkWudi);

    General *luna003 = new General(this, "luna003", "tsuki", 3, false);
    luna003->addSkill(new IkQingguo);
    luna003->addSkill(new IkBiyue);

    General *luna006 = new General(this, "luna006", "tsuki", 3);
    luna006->addSkill(new IkHuichun);
    luna006->addSkill(new IkQingnang);

    General *luna018 = new General(this, "luna018", "tsuki", 3);
    luna018->addSkill(new IkZhuji);
    luna018->addSkill(new IkZhujiEffect);
    related_skills.insertMulti("ikzhuji", "#ikzhuji-effect");
    luna018->addSkill(new IkBenyin);

    General *luna034 = new General(this, "luna034", "tsuki");
    luna034->addSkill(new IkGuijiao);
    luna034->addSkill(new IkGuijiaoMaxCards);
    related_skills.insertMulti("ikguijiao", "#ikguijiao-maxcard");
    luna034->addSkill(new IkJinlian);

    General *luna042 = new General(this, "luna042", "tsuki", 3);
    luna042->addSkill("ikhuichun");
    luna042->addSkill(new IkYaoge);

    addMetaObject<IkShenaiCard>();
    addMetaObject<IkXinqiCard>();
    addMetaObject<IkXingyuCard>();
    addMetaObject<IkChibaoCard>();
    addMetaObject<IkZhihengCard>();
    addMetaObject<IkGuisiCard>();
    addMetaObject<IkQinghuaCard>();
    addMetaObject<IkKurouCard>();
    addMetaObject<IkGuidengCard>();
    addMetaObject<IkWanmeiCard>();
    addMetaObject<IkXuanhuoCard>();
    addMetaObject<IkYuanheCard>();
    addMetaObject<IkHuanluCard>();
    addMetaObject<IkZiqiangCard>();
    addMetaObject<IkWudiCard>();
    addMetaObject<IkQingguoCard>();
    addMetaObject<IkQingnangCard>();
    addMetaObject<IkYaogeCard>();

    skills << new NonCompulsoryInvalidity << new IkXingyu << new IkQinghua;

    patterns["."] = new ExpPattern(".|.|.|hand");
    patterns[".S"] = new ExpPattern(".|spade|.|hand");
    patterns[".C"] = new ExpPattern(".|club|.|hand");
    patterns[".H"] = new ExpPattern(".|heart|.|hand");
    patterns[".D"] = new ExpPattern(".|diamond|.|hand");

    patterns[".black"] = new ExpPattern(".|black|.|hand");
    patterns[".red"] = new ExpPattern(".|red|.|hand");

    patterns[".."] = new ExpPattern(".");
    patterns["..S"] = new ExpPattern(".|spade");
    patterns["..C"] = new ExpPattern(".|club");
    patterns["..H"] = new ExpPattern(".|heart");
    patterns["..D"] = new ExpPattern(".|diamond");

    patterns[".Basic"] = new ExpPattern("BasicCard");
    patterns[".Trick"] = new ExpPattern("TrickCard");
    patterns[".Equip"] = new ExpPattern("EquipCard");

    patterns[".Weapon"] = new ExpPattern("Weapon");
    patterns["slash"] = new ExpPattern("Slash");
    patterns["jink"] = new ExpPattern("Jink");
    patterns["peach"] = new ExpPattern("Peach");
    patterns["analeptic"] = new ExpPattern("Analeptic");
    patterns["nullification"] = new ExpPattern("Nullification");
    patterns["peach+analeptic"] = new ExpPattern("Peach,Analeptic");
}

ADD_PACKAGE(IkaiDo)
