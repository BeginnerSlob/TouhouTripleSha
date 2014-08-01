#include "ikai-ka.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"

IkZhijuCard::IkZhijuCard() {
}

bool IkZhijuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->canDiscard(to_select, "ej");
}

void IkZhijuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "ej", "ikzhiju", false, MethodDiscard);
    room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : effect.to, effect.from);
    ServerPlayer *target = room->askForPlayerChosen(effect.from, room->getAllPlayers(), objectName(), "@ikzhiju-chain");
    target->setChained(!target->isChained());
    room->broadcastProperty(target, "chained");
    room->setEmotion(target, "chain");
    room->getThread()->trigger(ChainStateChanged, room, target);
    if (effect.from->isWounded())
        room->recover(effect.from, RecoverStruct(effect.from));
    effect.from->setFlags("ikzhiju");
}

class IkZhijuViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkZhijuViewAsSkill(): ZeroCardViewAsSkill("ikzhiju") {
    }

    virtual const Card *viewAs() const{
        return new IkZhijuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkZhijuCard");
    }
};

class IkZhiju: public TriggerSkill {
public:
    IkZhiju(): TriggerSkill("ikzhiju") {
        events << EventPhaseStart;
        view_as_skill = new IkZhijuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (player->getPhase() == Player::Discard && player->hasFlag(objectName())) {
            player->setFlags("-" + objectName());
            if (player->isKongcheng()) return QStringList();
            const Card *card = room->askForExchange(player, objectName(), 2, 2, false, "@ikzhiju", true);
            if (!card) {
                QList<const Card *> cards = player->getHandcards();
                DummyCard *card = new DummyCard;
                if (cards.length() > 2) {
                    qShuffle(cards);
                    card->addSubcards(cards.mid(0, 2));
                } else {
                    card->addSubcards(cards);
                }
            }
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
            room->moveCardTo(card, NULL, Player::DrawPile, reason, false);
            delete card;
        }
        return QStringList();
    }
};

class IkYingqi: public TriggerSkill {
public:
    IkYingqi(): TriggerSkill("ikyingqi") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@ikyingqi", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkYingqiTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = player->tag["IkYingqiTarget"].value<ServerPlayer *>();
        player->tag.remove("IkYingqiTarget");
        if (target) {
            room->addPlayerMark(player, "@yingqi");
            room->addPlayerMark(target, "@yingqi");
        }
        return false;
    }
};

class IkYingqiEffect: public TriggerSkill {
public:
    IkYingqiEffect(): TriggerSkill("#ikyingqi") {
        events << EventPhaseEnd << EventPhaseSkipped;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player->getMark("@yingqi") == 0)
            return QStringList();
        if (triggerEvent == EventPhaseEnd)
            if (player->getPhase() != Player::Play)
                return QStringList();
        if (triggerEvent == EventPhaseSkipped) {
            Player::Phase phase = (Player::Phase)data.toInt();
            if (phase != Player::Play)
                return QStringList();
        }
        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
            while (player->getMark("@yingqi") > 0) {
                if (player->getHandcardNum() >= player->getHp())
                    break;
                room->removePlayerMark(player, "@yingqi");
                player->drawCards(1, "ikyingqi");
                if (current->isDead())
                    break;
            }
        }
        room->setPlayerMark(player, "@yingqi", 0);
        return QStringList();
    }
};

IkJilunCard::IkJilunCard() {
}

bool IkJilunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasEquip() && Self->inMyAttackRange(to_select);
}

void IkJilunCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (effect.to->hasEquip()) {
        int card_id = room->askForCardChosen(effect.to, effect.to, "e", "ikjilun");
        room->obtainCard(effect.to, card_id);
    }
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_ikjilun");
    if (effect.from->canSlash(effect.to, slash) && !effect.from->isCardLimited(slash, Card::MethodUse))
        room->useCard(CardUseStruct(slash, effect.from, effect.to), false);
    else
        delete slash;
}

class IkJilun: public OneCardViewAsSkill {
public:
    IkJilun(): OneCardViewAsSkill("ikjilun") {
        filter_pattern = ".|black|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkJilunCard *card = new IkJilunCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkJilunCard") && player->canDiscard(player, "h");
    }
};

class IkJiqiao: public TriggerSkill {
public:
    IkJiqiao(): TriggerSkill("ikjiqiao") {
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Draw
            && player->getHandcardNum() < player->getMaxHp();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        const Card *card = room->askForCard(player, "..", "@ikjiqiao", QVariant(), objectName());
        if (card) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkJiqiaoCard"] = QVariant::fromValue(card);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        const Card *card = player->tag["IkJiqiaoCard"].value<const Card *>();
        player->tag.remove("IkJiqiaoCard");
        if (card) {
            switch (card->getTypeId()) {
            case Card::TypeBasic: {
                    if (Slash::IsAvailable(player)) {
                        QList<ServerPlayer *> targets;
                        foreach (ServerPlayer *p, room->getOtherPlayers(player))
                            if (player->canSlash(p, false))
                                targets << p;

                        if (targets.isEmpty())
                            return false;

                        ServerPlayer *victim = room->askForPlayerChosen(player, targets, objectName(), "@ikjiqiao-basic");
                        Slash *slash = new Slash(Card::NoSuit, 0);
                        slash->setSkillName("_ikjiqiao");
                        room->useCard(CardUseStruct(slash, player, victim));
                    }
                    break;
                }
            case Card::TypeEquip: {
                    ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikjiqiao-equip");
                    target->drawCards(2, objectName());
                    break;
                }
            case Card::TypeTrick: {
                    QList<ServerPlayer *> targets;
                    foreach (ServerPlayer *p, room->getOtherPlayers(player))
                        if (p->isWounded())
                            targets << p;

                    if (targets.isEmpty())
                        return false;
                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@ikjiqiao-trick");
                    room->recover(target, RecoverStruct(player));
                    break;
                }
            default:
                    break;
            }
        }
        return false;
    }
};

IkKangjinCard::IkKangjinCard() {
    will_throw = false;
    handling_method = MethodNone;
}

void IkKangjinCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    const Card *card = Sanguosha->getCard(getEffectiveId());
    if (effect.from->hasFlag("ikkangjin_red"))
        room->setPlayerFlag(effect.from, "-ikkangjin_red");
    if (effect.from->hasFlag("ikkangjin_black"))
        room->setPlayerFlag(effect.from, "-ikkangjin_black");
    if (card->isRed()) room->setPlayerFlag(effect.from, "ikkangjin_red");
    if (card->isBlack()) room->setPlayerFlag(effect.from, "ikkangjin_black");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "ikkangjin", QString());
    room->obtainCard(effect.to, this, reason);
    Duel *duel = new Duel(NoSuit, 0);
    duel->setSkillName("_ikkangjin");
    if (!effect.from->isProhibited(effect.to, duel) && !effect.from->isCardLimited(duel, Card::MethodUse))
        room->useCard(CardUseStruct(duel, effect.from, effect.to));
    else
        delete duel;
}

class IkKangjin: public OneCardViewAsSkill {
public:
    IkKangjin(): OneCardViewAsSkill("ikkangjin") {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const{
        if (to_select->isEquipped()) return false;
        if (Self->hasFlag("ikkangjin_red") && to_select->isRed()) return false;
        if (Self->hasFlag("ikkangjin_black") && to_select->isBlack()) return false;
        return true;
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkKangjinCard *card = new IkKangjinCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class IkKangjinTrigger: public TriggerSkill {
public:
    IkKangjinTrigger(): TriggerSkill("#ikkangjin") {
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Duel") && damage.card->getSkillName() == "ikkangjin") {
            ask_who = damage.by_user ? damage.from : player;
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(player->getLostHp(), objectName());
        return false;
    }
};

#include "roomscene.h"
SelectSuitDialog *SelectSuitDialog::getInstance() {
    static SelectSuitDialog *instance;
    if (instance == NULL)
        instance = new SelectSuitDialog();

    return instance;
}

SelectSuitDialog::SelectSuitDialog() {
    setObjectName("ikhunkao");
    setWindowTitle(tr("Please choose a suit"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectSuit(QAbstractButton *)));
}

void SelectSuitDialog::popup() {
    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }
    QSet<QString> suits;
    foreach (const Card *card, Self->getHandcards())
        suits << card->getSuitString();
    QStringList all_suit;
    all_suit << "spade" << "heart" << "club" << "diamond";
    foreach (QString suit, all_suit) {
        QCommandLinkButton *button = new QCommandLinkButton;
        button->setIcon(QIcon(QString("image/system/suit/%1.png").arg(suit)));
        button->setText(Sanguosha->translate(suit));
        button->setObjectName(suit);
        group->addButton(button);

        button->setEnabled(suits.contains(suit));
        button_layout->addWidget(button);
    }

    exec();
}

void SelectSuitDialog::selectSuit(QAbstractButton *button) {
    emit onButtonClick();
    RoomSceneInstance->getDashboard()->selectCards(".|" + button->objectName());
    accept();
}

IkHunkaoCard::IkHunkaoCard() {
}

bool IkHunkaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < subcardsLength() && to_select != Self;
}

void IkHunkaoCard::onUse(Room *room, const CardUseStruct &use) const{
    room->showAllCards(use.from);
    SkillCard::onUse(room, use);
}

void IkHunkaoCard::onEffect(const CardEffectStruct &effect) const{
    QString suit = Sanguosha->getCard(subcards.first())->getSuitString();
    QString pattern = ".|" + suit;
    const Card *card = NULL;
    Room *room = effect.from->getRoom();
    if (!effect.to->isNude())
        card = room->askForCard(effect.to, pattern, QString("@ikhunkao-give:%1::%2").arg(effect.from->objectName()).arg(suit),
                                QVariant(), MethodNone);
    if (card) {
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.to->objectName(), effect.from->objectName(), "ikhunkao", QString());
        room->obtainCard(effect.from, card, reason);
    } else {
        Slash *slash = new Slash(NoSuit, 0);
        slash->setSkillName("_ikhunkao");
        if (!effect.from->isCardLimited(slash, MethodUse) && effect.from->canSlash(effect.to, slash, false))
            room->useCard(CardUseStruct(slash, effect.from, effect.to), false);
        else
            delete slash;
    }
}

class IkHunkao: public ViewAsSkill {
public:
    IkHunkao(): ViewAsSkill("ikhunkao") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (to_select->isEquipped() || Self->isJilei(to_select))
            return false;
        return selected.isEmpty() || to_select->getSuit() == selected.first()->getSuit();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty()) return NULL;
        int n = 0;
        foreach (const Card *card, Self->getHandcards())
            if (card->getSuit() == cards.first()->getSuit())
                n++;
        if (cards.length() != n)
            return NULL;

        IkHunkaoCard *card = new IkHunkaoCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "h") && player->usedTimes("IkHunkaoCard") < 2;
    }

    virtual QDialog *getDialog() const {
        return SelectSuitDialog::getInstance();
    }
};

class IkHudie: public TriggerSkill {
public:
    IkHudie(): TriggerSkill("ikhudie") {
        events << DamageInflicted;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (TriggerSkill::triggerable(player) && damage.from && damage.from->getKingdom() != player->getKingdom())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#ChangeKingdom";
        log.from = player;
        log.to << player;
        log.arg = player->getKingdom();
        log.arg2 = damage.from->getKingdom();
        room->sendLog(log);
        room->setPlayerProperty(player, "kingdom", damage.from->getKingdom());
        return false;
    }
};

class IkHualan: public TriggerSkill {
public:
    IkHualan(): TriggerSkill("ikhualan") {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (TriggerSkill::triggerable(player) && damage.to && damage.to->isAlive() && damage.to->getKingdom() != player->getKingdom())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();
        QStringList choices;
        choices << "draw";
        if (damage.to && damage.to->isAlive() && player->getKingdom() != "kaze")
            choices << "change";
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "change") {
            LogMessage log;
            log.type = "#ChangeKingdom";
            log.from = player;
            log.to << player;
            log.arg = player->getKingdom();
            log.arg2 = "kaze";
            room->sendLog(log);
            room->setPlayerProperty(player, "kingdom", "kaze");
        } else {
            player->drawCards(1);
        }
        return false;
    }
};

class IkFengxing: public TriggerSkill {
public:
    IkFengxing(): TriggerSkill("ikfengxing") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        forever {
            room->showAllCards(player);
            foreach (const Card *card, player->getHandcards())
                if (card->isKindOf("Slash"))
                    break;
            player->drawCards(1, objectName());
            if (!player->askForSkillInvoke(objectName()))
                break;
        }

        return false;
    }
};

class IkQizhong: public TriggerSkill {
public:
    IkQizhong(): TriggerSkill("ikqizhong") {
        events << CardUsed << EventPhaseChanging << CardFinished;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging)
            player->setMark(objectName(), 0);
        else if (triggerEvent == CardFinished) {
            const Card *card = data.value<CardUseStruct>().card;
            player->setMark(objectName(), card->getColor() + 1);
        } else if (triggerEvent == CardUsed && player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            int num = player->getMark(objectName());
            int color = -1;
            if (num == 0)
                return QStringList();
            else if (num == 1)
                color = Card::Red;
            else if (num == 2)
                color = Card::Black;
            else if (num == 3)
                color = Card::Colorless;
            if (use.card->getColor() != color)
                return QStringList(objectName());
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QList<int> ids = room->getNCards(1, false);
        CardsMoveStruct move(ids, player, Player::PlaceTable,
                             CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        int id = ids.first();
        const Card *card = Sanguosha->getCard(id);
        if (card->getColor() != use.card->getColor()) {
            CardMoveReason reason(CardMoveReason::S_REASON_DRAW, player->objectName(), objectName(), QString());
            room->obtainCard(player, card, reason);
        } else {
            const Card *card_ex = NULL;
            if (!player->isNude())
                card_ex = room->askForCard(player, QString("^%1|.|.|hand").arg(use.card->getEffectiveId()),
                                           "@ikqizhong-exchange:::" + card->objectName(),
                                           QVariant::fromValue(card), Card::MethodNone);
            if (card_ex) {
                CardMoveReason reason1(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
                CardMoveReason reason2(CardMoveReason::S_REASON_DRAW, player->objectName(), objectName(), QString());
                CardsMoveStruct move1(card_ex->getEffectiveId(), player, NULL, Player::PlaceUnknown, Player::DrawPile, reason1);
                CardsMoveStruct move2(ids, player, player, Player::PlaceUnknown, Player::PlaceHand, reason2);

                QList<CardsMoveStruct> moves;
                moves.append(move1);
                moves.append(move2);
                room->moveCardsAtomic(moves, false);
            } else {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
                room->throwCard(card, reason, NULL);
            }
        }

        return false;
    }
};

class IkDuduan: public TriggerSkill {
public:
    IkDuduan(): TriggerSkill("ikduduan") {
        events << EventPhaseStart << PreDamageDone;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *from = damage.from;
            if (from && from->getPhase() == Player::Play && from->hasFlag(objectName()))
                from->setFlags("-" + objectName());
        } else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Start)
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
        ServerPlayer *target = NULL;
        QList<ServerPlayer *> targets;
        targets << player;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (player->inMyAttackRange(p))
                targets << p;
        target = room->askForPlayerChosen(player, targets, objectName(), "@ikduduan", true);
        if (target) {
            int card_id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, target, player == target ? NULL : player);
        } else
            player->drawCards(1, objectName());
        player->setFlags(objectName());
        return false;
    }
};

class IkDuduanMaxCards: public MaxCardsSkill {
public:
    IkDuduanMaxCards(): MaxCardsSkill("#ikduduan") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasFlag("ikduduan"))
            return -2;
        else
            return 0;
    }
};

IkPaomuCard::IkPaomuCard() {
}

void IkPaomuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->removePlayerMark(effect.from, "@paomu");
    room->addPlayerMark(effect.from, "@paomuused");
    effect.to->gainMark("@liebiao");
}

class IkPaomuViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkPaomuViewAsSkill(): ZeroCardViewAsSkill("ikpaomu") {
    }

    virtual const Card *viewAs() const{
        return new IkPaomuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@paomu") > 0;
    }
};

class IkPaomu: public TriggerSkill {
public:
    IkPaomu(): TriggerSkill("ikpaomu") {
        events << CardFinished << Death;
        view_as_skill = new IkPaomuViewAsSkill;
        frequency = Limited;
        limit_mark = "@paomu";
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (triggerEvent == CardFinished && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card || !use.card->isBlack() || !use.card->isKindOf("Slash"))
                return QStringList();
            foreach (ServerPlayer *to, use.to)
                if (to->getMark("@liebiao") > 0)
                    return QStringList(objectName());
        } else if (triggerEvent == Death && player->getMark("@liebiao") > 0) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player) {
                ServerPlayer *owner = room->findPlayerBySkillName(objectName());
                if (owner) {
                    ask_who = owner;
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikpaomu", false, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkPaomuTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        ServerPlayer *target = player->tag["IkPaomuTarget"].value<ServerPlayer *>();
        player->tag.remove("IkPaomuTarget");
        if (triggerEvent == CardFinished && target) {
            ServerPlayer *victim = NULL;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@liebiao") > 0) {
                    victim = p;
                    break;
                }
            }
            bool use = false;
            if (target->canSlash(victim)) {
                QString prompt = QString("@ikpaomu-slash:%1:%2").arg(player->objectName()).arg(victim->objectName());
                use = room->askForUseSlashTo(target, victim, prompt, false);
            }
            if (!use)
                player->drawCards(1, objectName());
        } else if (triggerEvent == Death && target) {
            target->gainMark("@liebiao");
        }
        return false;
    }
};

IkDengpoCard::IkDengpoCard() {
}

bool IkDengpoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getCardCount() >= subcardsLength() && to_select != Self;
}

void IkDengpoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    int x = subcardsLength();
    room->askForDiscard(effect.to, objectName(), x, x, false, true);
    effect.from->setFlags("ikdengpo");
    effect.to->setMark("ikdengpo", x);
    QString dis = getUserString();
    if (dis == "null") {
        effect.from->tag["IkDengpoTarget"] = QVariant::fromValue(effect.to);
        room->setFixedDistance(effect.from, effect.to, 1);
    }
}

class IkDengpoViewAsSkill: public ViewAsSkill {
public:
    IkDengpoViewAsSkill(): ViewAsSkill("ikdengpo") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Self->isJilei(to_select))
            return false;
        return selected.length() < 4;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty()) return NULL;
        bool distance = Self->hasEquip();
        foreach (const Card *cd, Self->getEquips())
            if (!cards.contains(cd)) {
                distance = false;
                break;
            }
        IkDengpoCard *card = new IkDengpoCard;
        card->addSubcards(cards);
        card->setUserString(distance ? "null" : QString());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("IkDengpoCard");
    }
};

class IkDengpo: public TriggerSkill {
public:
    IkDengpo(): TriggerSkill("ikdengpo") {
        events << EventPhaseEnd << EventPhaseChanging;
        view_as_skill = new IkDengpoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play && player->hasFlag(objectName())) {
            player->setFlags("-" + objectName());
            QList<ServerPlayer *> targets;
            targets << player;
            int x = 0;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (p->getMark(objectName()) > 0) {
                    targets << p;
                    x = p->getMark(objectName());
                    p->setMark(objectName(), 0);
                    break;
                }
            room->drawCards(targets, x, objectName());
        } else if (triggerEvent == EventPhaseChanging && !player->tag["IkDengpoTarget"].isNull()) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                ServerPlayer *target = player->tag["IkDengpoTarget"].value<ServerPlayer *>();
                player->tag.remove("IkDengpoTarget");
                if (target)
                    room->setFixedDistance(player, target, -1);
            }
        }
        return QStringList();
    }
};

class IkLingyun: public TriggerSkill {
public:
    IkLingyun(): TriggerSkill("iklingyun") {
        events << BeforeCardsMove;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player)
            return QStringList();
        if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE)) {
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if (card->isKindOf("BasicCard"))
                return QStringList(objectName());
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> card_ids = move.card_ids, disabled_ids;
        while (!card_ids.isEmpty()) {
            room->fillAG(card_ids, NULL, disabled_ids);
            int id = room->askForAG(player, card_ids, false, objectName());
            room->clearAG(NULL);
            card_ids.removeOne(id);
            disabled_ids << id;
            room->moveCardsAtomic(CardsMoveStruct(id,
                                                  NULL,
                                                  Player::DrawPile,
                                                  CardMoveReason(CardMoveReason::S_REASON_PUT,
                                                                 player->objectName(),
                                                                 objectName(),
                                                                 QString())),
                                  true);
        }
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);
        return false;
    }
};

class IkMiyao: public TriggerSkill {
public:
    IkMiyao(): TriggerSkill("ikmiyao") {
        events << EventPhaseChanging;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive) return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (owner == player) continue;
            if (owner->getHandcardNum() != player->getHandcardNum())
                skill_list.insert(owner, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        int x = player->getHandcardNum() - ask_who->getHandcardNum();
        if (x > 0) {
            ask_who->drawCards(x, objectName());
            if (x >= 2)
                room->loseHp(ask_who);
        } else if (x < 0) {
            x = -x;
            room->askForDiscard(ask_who, objectName(), x, x);
            if (x >= 2 && ask_who->isWounded())
                room->recover(ask_who, RecoverStruct(ask_who));
        }
        return false;
    }
};

IkShidaoCard::IkShidaoCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkShidaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->setSkillName("ikshidao");
        }
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *card = Sanguosha->cloneCard(Self->tag.value("ikshidao").value<const Card *>()->objectName());
    card->addSubcards(subcards);
    card->setSkillName("ikshidao");
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool IkShidaoCard::targetFixed() const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("ikshidao").value<const Card *>();
    return card && card->targetFixed();
}

bool IkShidaoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->setSkillName("ikshidao");
        }
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *card = Sanguosha->cloneCard(Self->tag.value("ikshidao").value<const Card *>()->objectName());
    card->addSubcards(subcards);
    card->setSkillName("ikshidao");
    return card && card->targetsFeasible(targets, Self);
}

const Card *IkShidaoCard::validate(CardUseStruct &card_use) const{
    ServerPlayer *wenyang = card_use.from;
    Room *room = wenyang->getRoom();

    QString to_ikshidao = user_string;
    if (user_string == "slash"
        && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QStringList ikshidao_list;
            ikshidao_list << "slash";
            if (!Config.BanPackages.contains("maneuvering"))
                ikshidao_list << "thunder_slash" << "fire_slash";
            to_ikshidao = room->askForChoice(wenyang, "ikshidao_slash", ikshidao_list.join("+"));
    }

    ServerPlayer *target = room->askForPlayerChosen(wenyang, room->getOtherPlayers(wenyang), "ikshidao", "@ikshidao");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, wenyang->objectName(), target->objectName(), "ikshidao", QString());
    room->obtainCard(target, this, reason);

    Card *use_card = Sanguosha->cloneCard(to_ikshidao, NoSuit, 0);
    use_card->setSkillName("ikshidao");
    use_card->deleteLater();
    return use_card;
}

const Card *IkShidaoCard::validateInResponse(ServerPlayer *wenyang) const{
    Room *room = wenyang->getRoom();

    QString to_ikshidao;
    if (user_string == "peach+analeptic") {
        QStringList ikshidao_list;
        ikshidao_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            ikshidao_list << "analeptic";
        to_ikshidao = room->askForChoice(wenyang, "ikshidao_saveself", ikshidao_list.join("+"));
    } else if (user_string == "slash") {
        QStringList ikshidao_list;
        ikshidao_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            ikshidao_list << "thunder_slash" << "fire_slash";
        to_ikshidao = room->askForChoice(wenyang, "ikshidao_slash", ikshidao_list.join("+"));
    }
    else
        to_ikshidao = user_string;

    ServerPlayer *target = room->askForPlayerChosen(wenyang, room->getOtherPlayers(wenyang), "ikshidao", "@ikshidao");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, wenyang->objectName(), target->objectName(), "ikshidao", QString());
    room->obtainCard(target, this, reason);

    Card *use_card = Sanguosha->cloneCard(to_ikshidao, NoSuit, 0);
    use_card->setSkillName("ikshidao");
    use_card->deleteLater();
    return use_card;
}

#include "touhou-hana.h"
class IkShidao: public ViewAsSkill {
public:
    IkShidao(): ViewAsSkill("ikshidao") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() >= 2)
            return false;
        else if (selected.isEmpty())
            return true;
        else if (selected.length() == 1)
            return selected.first()->getTypeId() != to_select->getTypeId();
        else
            return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getCardCount() < 2) return false;
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) return false;
        QList<const Card *> cards = player->getHandcards() + player->getEquips();
        Card::CardType type = cards.first()->getTypeId();
        bool can = false;
        foreach (const Card *cd, cards)
            if (cd->getTypeId() != type) {
                can = true;
                break;
            }
        if (!can)
            return false;
        if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic") || pattern == "slash" || pattern == "jink")
            return true;
        return false;
    }

    virtual QDialog *getDialog() const{
        return ThMimengDialog::getInstance("ikshidao", true, false);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2) return NULL;
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            IkShidaoCard *tianyan_card = new IkShidaoCard;
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";
            tianyan_card->setUserString(pattern);
            tianyan_card->addSubcards(cards);
            return tianyan_card;
        }

        const Card *c = Self->tag["ikshidao"].value<const Card *>();
        if (c) {
            IkShidaoCard *card = new IkShidaoCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getCardCount() < 2) return false;
        QList<const Card *> cards = player->getHandcards() + player->getEquips();
        Card::CardType type = cards.first()->getTypeId();
        foreach (const Card *cd, cards)
            if (cd->getTypeId() != type)
                return true;
        return false;
    }
};

class IkShenshu: public OneCardViewAsSkill {
public:
    IkShenshu(): OneCardViewAsSkill("ikshenshu") {
        response_or_use = true;
        filter_pattern = "Peach";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        GodSalvation *card = new GodSalvation(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class IkQiyi: public TriggerSkill {
public:
    IkQiyi(): TriggerSkill("ikqiyi") {
        events << HpRecover;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &ask_who) const{
        ServerPlayer *current = room->getCurrent();
        QStringList skill;
        if (TriggerSkill::triggerable(current) && current->getPhase() != Player::NotActive) {
            ask_who = current;
            RecoverStruct recover = data.value<RecoverStruct>();
            for (int i = 0; i < recover.recover; i++)
                skill << objectName();
        }
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        QStringList choices;
        choices << "draw";
        if (ask_who->canDiscard(player, "he"))
            choices << "discard";
        QString choice = room->askForChoice(ask_who, objectName(), choices.join("+"));
        if (choice == "discard") {
            int card_id = room->askForCardChosen(ask_who, player, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, player, ask_who == player ? NULL : ask_who);
        } else
            ask_who->drawCards(1, objectName());
        return false;
    }
};

IkLinghuiCard::IkLinghuiCard() {
}

bool IkLinghuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Self->getMark("iklinghui") > 0)
        return targets.length() < subcardsLength();
    return targets.isEmpty() && to_select != Self;
}

bool IkLinghuiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Self->getMark("iklinghui") > 0)
        return targets.length() == subcardsLength();
    return targets.length() == 1;
}

void IkLinghuiCard::onEffect(const CardEffectStruct &effect) const{
    if (effect.from->getMark("iklinghui") > 0) {
        effect.to->drawCards(2, "iklinghui");
    } else {
        effect.to->drawCards(1, "iklinghui");
        if (!effect.to->isKongcheng()) {
            Room *room = effect.from->getRoom();
            const Card *card = room->askForCardShow(effect.to, effect.from, "iklinghui");
            room->showCard(effect.to, card->getEffectiveId());
            room->setPlayerMark(effect.from, "iklinghui", card->getColor() + 1);
            room->askForUseCard(effect.from, "@@iklinghui", "@iklinghui", -1, Card::MethodDiscard, false);
            room->setPlayerMark(effect.from, "iklinghui", 0);
        }
    }
}

class IkLinghui: public ViewAsSkill {
public:
    IkLinghui(): ViewAsSkill("iklinghui") {
        response_pattern = "@@iklinghui";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        if (Self->getMark(objectName()) > 0) {
            Card::Color color = (Card::Color)(Self->getMark(objectName()) - 1);
            return !Self->isJilei(to_select) && to_select->getColor() == color && !to_select->isEquipped();
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (Self->getMark(objectName()) > 0) {
            if (cards.isEmpty())
                return NULL;
            IkLinghuiCard *card = new IkLinghuiCard;
            card->addSubcards(cards);
            return card;
        }
        if (!cards.isEmpty())
            return NULL;
        return new IkLinghuiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkLinghuiCard");
    }
};

class IkHuaiji: public TriggerSkill {
public:
    IkHuaiji(): TriggerSkill("ikhuaiji") {
        events << EventPhaseEnd;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() == Player::Play && player->getMark(objectName()) > 0)
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                if (player->getMark(objectName()) > owner->getHp())
                    skill_list.insert(owner, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *owner) const{
        if (owner->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *owner) const{
        QList<int> card_ids = room->getNCards(3, false);
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, owner->objectName());
        CardsMoveStruct move(card_ids, NULL, Player::DiscardPile, reason);
        room->moveCardsAtomic(move, true);
        bool heart = false;
        foreach (int id, card_ids)
            if (Sanguosha->getCard(id)->getSuit() == Card::Heart) {
                heart = true;
                break;
            }
        if (heart) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers())
                if (p->isWounded())
                    targets << p;
            if (!targets.isEmpty()) {
                ServerPlayer *target = room->askForPlayerChosen(owner, targets, objectName(), "ikhuaiji-recover", true);
                if (target)
                    room->recover(target, RecoverStruct(owner));
            }
        }
        return false;
    }
};

class IkHuaijiRecord: public TriggerSkill {
public:
    IkHuaijiRecord(): TriggerSkill("#ikhuaiji") {
        events << CardsMoveOneTime << EventPhaseChanging;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging)
            player->setMark("ikhuaiji", 0);
        else if (triggerEvent == CardsMoveOneTime) {
            if (player != room->getCurrent() || player->getPhase() != Player::Play)
                return QStringList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        player->addMark("ikhuaiji", move.card_ids.length());
        return false;
    }
};

IkDianyanCard::IkDianyanCard() {
    will_throw = false;
    target_fixed = true;
    handling_method = MethodNone;
}

void IkDianyanCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->addToPile("ikdianyanpile", this);
}

class IkDianyan: public OneCardViewAsSkill {
public:
    IkDianyan(): OneCardViewAsSkill("ikdianyan") {
        filter_pattern = ".|.|.|hand";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkDianyanCard *card = new IkDianyanCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("IkDianyanCard");
    }
};

IkDianyanPutCard::IkDianyanPutCard() {
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void IkDianyanPutCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->moveCardsAtomic(CardsMoveStruct(subcards,
                                          NULL,
                                          Player::DrawPile,
                                          CardMoveReason(CardMoveReason::S_REASON_PUT, source->objectName())),
                          true);
}

class IkDianyanPut: public ViewAsSkill {
public:
    IkDianyanPut(): ViewAsSkill("ikdianyanput") {
        response_pattern = "@@ikdianyanput!";
        expand_pile = "ikdianyanpile";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const {
        return Self->getPile("ikdianyanpile").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (Self->getPile("ikdianyanpile").length() != cards.length())
            return NULL;

        Card *card = new IkDianyanPutCard;
        card->addSubcards(cards);
        return card;
    }
};

class IkDianyanTrigger: public TriggerSkill {
public:
    IkDianyanTrigger(): TriggerSkill("#ikdianyan") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        QList<int> dianyan_cards = StringList2IntList(player->tag["IkDianyanIds"].toString().split("+"));
        if (dianyan_cards.isEmpty())
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!move.to || !move.from_places.contains(Player::DrawPile))
            return QStringList();
        QStringList skills;
        foreach (int id, move.card_ids) {
            int i = move.card_ids.indexOf(id);
            if (move.from_places[i] == Player::DrawPile && dianyan_cards.contains(id))
                skills << objectName();
        }
        return skills;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = (ServerPlayer *)move.to;
        room->sendCompulsoryTriggerLog(player, "ikdianyan");
        QString choice = room->askForChoice(player, "ikdianyan", "draw+lose");
        if (choice == "draw")
            target->drawCards(2, "ikdianyan");
        else
            room->loseHp(target);
        return false;
    }
};

class IkLianxiao: public TriggerSkill {
public:
    IkLianxiao(): TriggerSkill("iklianxiao") {
        events << EventPhaseEnd;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() != Player::Play) return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
            if (owner->getMark(objectName()) == 0)
                skill_list.insert(owner, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        QString choice = room->askForChoice(ask_who, objectName(), "draw+max");
        if (choice == "draw")
            player->drawCards(1, objectName());
        else
            room->addPlayerMark(player, "iklianxiao_max");
        return false;
    }
};

class IkLianxiaoRecord: public TriggerSkill {
public:
    IkLianxiaoRecord(): TriggerSkill("#iklianxiao-record") {
        events << CardFinished << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == CardFinished && player->isAlive() && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill)
                foreach (ServerPlayer *p, use.to)
                    p->setMark("iklianxiao", 1);
        } else if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                p->setMark("iklianxiao", 0);
            if (data.value<PhaseChangeStruct>().to == Player::NotActive)
                room->setPlayerMark(player, "iklianxiao_max", 0);
        }
        return QStringList();
    }
};

class IkLianxiaoMaxCards: public MaxCardsSkill {
public:
    IkLianxiaoMaxCards(): MaxCardsSkill("#iklianxiao-max") {
    }

    virtual int getExtra(const Player *target) const{
        return -target->getMark("iklianxiao_max");
    }
};

class IkLingcu: public TriggerSkill {
public:
    IkLingcu(): TriggerSkill("iklingcu") {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play && !player->isSkipped(Player::Play))
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
        player->skip(Player::Play, true);
        player->turnOver();

        QList<Card::Suit> suits;
        forever {
            JudgeStruct judge;
            judge.play_animation = false;
            judge.who = player;
            judge.reason = objectName();
            room->judge(judge);

            player->addToPile(objectName(), judge.card);
            Card::Suit suit = judge.card->getSuit();
            if (suits.contains(suit)) {
                suits << suit;
                break;
            }
            suits << suit;
        }

        int n = suits.length();
        for (int i = 0; i < n; i++) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *target, room->getOtherPlayers(player))
                if (player->canSlash(target))
                    targets << target;
            if (targets.isEmpty())
                break;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@dummy-slash");
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_iklingcu");
            room->useCard(CardUseStruct(slash, player, target));
        }
        player->clearOnePrivatePile(objectName());
        return false;
    }
};

IkQisiCard::IkQisiCard(){
    target_fixed = true;
    will_throw = false;
}

const Card *IkQisiCard::validate(CardUseStruct &cardUse) const{
    cardUse.from->drawCards(1, "ikqisi");

    Nullification *use_card = new Nullification(NoSuit, 0);
    use_card->setSkillName("ikqisi");
    return use_card;
}

const Card *IkQisiCard::validateInResponse(ServerPlayer *user) const{
    user->drawCards(1, "ikqisi");
    Nullification *use_card = new Nullification(NoSuit, 0);
    use_card->setSkillName("ikqisi");
    return use_card;
}

class IkQisiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkQisiViewAsSkill(): ZeroCardViewAsSkill("ikqisi") {
    }

    virtual const Card *viewAs() const{
        return new IkQisiCard;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (pattern != "nullification") return false;
        if (player->property("ikqisi").isNull())
            return false;
        QString obj_name = player->property("ikqisi").toString();
        if (obj_name.isEmpty() || player->objectName() == obj_name)
            return false;
        const Player *from = NULL;
        foreach (const Player *p, player->getAliveSiblings())
            if (p->objectName() == obj_name) {
                from = p;
                break;
            }
        if (from && from->getHandcardNum() >= player->getHandcardNum())
            return player->getHandcardNum() % 2;
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        if (player->property("ikqisi").isNull())
            return false;
        QString obj_name = player->property("ikqisi").toString();
        if (obj_name.isEmpty() || player->objectName() == obj_name)
            return false;
        ServerPlayer *from = player->getRoom()->findPlayer(obj_name);
        if (from && from->getHandcardNum() >= player->getHandcardNum())
            return player->getHandcardNum() % 2;
        return false;
    }
};

class IkQisi: public TriggerSkill {
public:
    IkQisi(): TriggerSkill("ikqisi") {
        events << TrickCardCanceling;
        view_as_skill = new IkQisiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (!effect.from)
            room->setPlayerProperty(player, "ikqisi", QString());
        else
            room->setPlayerProperty(player, "ikqisi", effect.from->objectName());
        return QStringList();
    }
};

class IkMiaoxiang: public TriggerSkill {
public:
    IkMiaoxiang(): TriggerSkill("ikmiaoxiang") {
        events << Damaged;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (owner == player) continue;
            if (owner->getHandcardNum() <= owner->getHandcardNum() && owner->getHandcardNum() % 2 == 0)
                skill_list.insert(owner, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *owner) const{
        if (room->askForCard(owner, "^BasicCard", "@ikmiaoxiang", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *owner) const{
        room->recover(player, RecoverStruct(owner));
        return false;
    }
};

class IkJichang: public TriggerSkill {
public:
    IkJichang(): TriggerSkill("ikjichang") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        int n = 4;
        if (!player->isKongcheng()) {
            room->showAllCards(player);
            QSet<Card::Suit> suits;
            foreach (const Card *c, player->getHandcards())
                suits << c->getSuit();
            n -= suits.size();
        }
        if (n == 0)
            return false;
        player->drawCards(n, objectName());
        if (n >= 2) {
            player->skip(Player::Judge);
            player->skip(Player::Draw);
        }
        return false;
    }
};

IkManwuCard::IkManwuCard() {
}

bool IkManwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void IkManwuCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->pindian(effect.to, "ikmanwu");
}

class IkManwuViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkManwuViewAsSkill(): ZeroCardViewAsSkill("ikmanwu") {
    }

    virtual const Card *viewAs() const{
        return new IkManwuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkManwuCard") && !player->isKongcheng();
    }
};

class IkManwu: public TriggerSkill {
public:
    IkManwu(): TriggerSkill("ikmanwu") {
        events << Pindian;
        view_as_skill = new IkManwuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != objectName() || pindian->from_number == pindian->to_number)
            return QStringList();

        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        if (winner->getHandcardNum() < winner->getHp())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        winner->drawCards(winner->getHp() - winner->getHandcardNum(), objectName());
        return false;
    }
};

IkaiKaPackage::IkaiKaPackage()
    :Package("ikai-ka")
{
    General *wind025 = new General(this, "wind025", "kaze", 3);
    wind025->addSkill(new IkZhiju);
    wind025->addSkill(new IkYingqi);
    wind025->addSkill(new IkYingqiEffect);
    related_skills.insertMulti("ikyingqi", "#ikyingqi");

    General *wind033 = new General(this, "wind033", "kaze");
    wind033->addSkill(new IkJilun);

    General *wind034 = new General(this, "wind034", "kaze");
    wind034->addSkill(new IkJiqiao);

    General *wind035 = new General(this, "wind035", "kaze");
    wind035->addSkill(new IkKangjin);
    wind035->addSkill(new IkKangjinTrigger);
    related_skills.insertMulti("ikkangjin", "#ikkangjin");

    General *wind036 = new General(this, "wind036", "kaze");
    wind036->addSkill(new IkHunkao);

    General *wind045 = new General(this, "wind045", "kaze");
    wind045->addSkill(new IkHudie);
    wind045->addSkill(new Skill("ikyinsha", Skill::Compulsory));
    wind045->addSkill(new IkHualan);

    General *bloom032 = new General(this, "bloom032", "hana");
    bloom032->addSkill(new IkFengxing);

    General *bloom034 = new General(this, "bloom034", "hana");
    bloom034->addSkill(new IkQizhong);

    General *bloom035 = new General(this, "bloom035", "hana");
    bloom035->addSkill(new IkDuduan);
    bloom035->addSkill(new IkDuduanMaxCards);
    related_skills.insertMulti("ikduduan", "#ikduduan");

    General *bloom036 = new General(this, "bloom036", "hana");
    bloom036->addSkill(new IkPaomu);

    General *bloom045 = new General(this, "bloom045", "hana", 4, false);
    bloom045->addSkill(new IkDengpo);

    General *snow031 = new General(this, "snow031", "yuki", 3);
    snow031->addSkill(new IkLingyun);
    snow031->addSkill(new IkMiyao);

    General *snow034 = new General(this, "snow034", "yuki");
    snow034->addSkill(new IkShidao);

    General *snow035 = new General(this, "snow035", "yuki");
    snow035->addSkill(new IkShenshu);
    snow035->addSkill(new IkQiyi);

    General *snow036 = new General(this, "snow036", "yuki", 3);
    snow036->addSkill("ikmitu");
    snow036->addSkill(new IkLinghui);

    General *snow045 = new General(this, "snow045", "yuki");
    snow045->addSkill(new IkHuaiji);
    snow045->addSkill(new IkHuaijiRecord);
    related_skills.insertMulti("ikhuaiji", "#ikhuaiji");
    snow045->addSkill(new IkDianyan);
    snow045->addSkill(new IkDianyanTrigger);
    related_skills.insertMulti("ikdianyan", "#ikdianyan");

    General *snow046 = new General(this, "snow046", "yuki");
    snow046->addSkill(new IkLianxiao);
    snow046->addSkill(new IkLianxiaoRecord);
    snow046->addSkill(new IkLianxiaoMaxCards);
    related_skills.insertMulti("iklianxiao", "#iklianxiao-record");
    related_skills.insertMulti("iklianxiao", "#iklianxiao-max");

    General *luna030 = new General(this, "luna030", "tsuki");
    luna030->addSkill(new IkLingcu);

    General *luna031 = new General(this, "luna031", "tsuki", 3);
    luna031->addSkill(new IkQisi);
    luna031->addSkill(new IkMiaoxiang);

    General *luna032 = new General(this, "luna032", "tsuki", 3);
    luna032->addSkill(new IkJichang);
    luna032->addSkill(new IkManwu);

    addMetaObject<IkZhijuCard>();
    addMetaObject<IkJilunCard>();
    addMetaObject<IkKangjinCard>();
    addMetaObject<IkHunkaoCard>();
    addMetaObject<IkPaomuCard>();
    addMetaObject<IkDengpoCard>();
    addMetaObject<IkShidaoCard>();
    addMetaObject<IkLinghuiCard>();
    addMetaObject<IkDianyanCard>();
    addMetaObject<IkDianyanPutCard>();
    addMetaObject<IkQisiCard>();
    addMetaObject<IkManwuCard>();

    skills << new IkDianyanPut;
}

ADD_PACKAGE(IkaiKa)