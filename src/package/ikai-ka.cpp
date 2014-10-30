#include "ikai-ka.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "standard-equips.h"
#include "maneuvering.h"
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
                DummyCard *dummy = new DummyCard;
                if (cards.length() > 2) {
                    qShuffle(cards);
                    dummy->addSubcards(cards.mid(0, 2));
                } else {
                    dummy->addSubcards(cards);
                }
                card = dummy;
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
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikyingqi", true, true);
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
        player->drawCards(qMin(5, player->getLostHp()), objectName());
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
    return targets.length() < subcardsLength() && targets.length() < 2 && to_select != Self;
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

class IkTianhua: public TriggerSkill {
public:
    IkTianhua(): TriggerSkill("iktianhua") {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (room->getCurrent() == player && player->getPhase() != Player::NotActive)
                return QStringList();
            if (move.from == player && move.from_places.contains(Player::PlaceHand) && player->getHandcardNum() <= 1)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->getHandcardNum() <= 1)
                return QStringList(objectName());
            if (change.to == Player::Draw || change.to == Player::Discard)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        if (triggerEvent == CardsMoveOneTime || data.value<PhaseChangeStruct>().to == Player::NotActive) {
            QList<int> card_ids = room->getNCards(5, false);
            QList<int> to_get;
            room->fillAG(card_ids, player);
            forever {
                int id = room->askForAG(player, card_ids, !to_get.isEmpty(), objectName());
                if (id == -1)
                    break;
                QList<ServerPlayer *> _player;
                _player << player;
                room->takeAG(player, id, false, _player);
                card_ids.removeOne(id);
                to_get << id;
                if (card_ids.isEmpty())
                    break;
            }
            room->clearAG(player);
            DummyCard *dummy = new DummyCard(to_get);
            player->obtainCard(dummy, false);
            if (!card_ids.isEmpty())
                room->askForGuanxing(player, card_ids, Room::GuanxingUpOnly);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            player->skip(change.to);
        }
        return false;
    }
};

IkHuangshiCard::IkHuangshiCard() {
    will_throw = true;
}

bool IkHuangshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->canDiscard(to_select, "he") && !Self->inMyAttackRange(to_select) && to_select != Self;
}

void IkHuangshiCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->askForDiscard(effect.to, "ikhuangshi", subcardsLength(), subcardsLength(), false, true);
}

class IkHuangshiViewAsSkill: public ViewAsSkill {
public:
    IkHuangshiViewAsSkill(): ViewAsSkill("ikhuangshi") {
        response_pattern = "@@ikhuangshi";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (to_select->isEquipped() || Self->isJilei(to_select))
            return false;
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty()) return NULL;
        IkHuangshiCard *card = new IkHuangshiCard;
        card->addSubcards(cards);
        return card;
    }
};

class IkHuangshi: public TriggerSkill {
public:
    IkHuangshi(): TriggerSkill("ikhuangshi") {
        events << Damaged;
        view_as_skill = new IkHuangshiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && player->canDiscard(player, "h");
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->askForUseCard(player, "@@ikhuangshi", "@ikhuangshi", -1, Card::MethodDiscard);
        return false;
    }
};

class IkXizi: public TriggerSkill {
public:
    IkXizi(): TriggerSkill("ikxizi") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && !target->isKongcheng();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->showAllCards(player);
        QStringList choices;
        foreach (const Card *c, player->getHandcards()) {
            if (c->isBlack()) {
                choices << "black";
                break;
            }
        }
        foreach (const Card *c, player->getHandcards()) {
            if (c->isRed()) {
                choices << "red";
                break;
            }
        }
        if (choices.isEmpty()) return false;
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        QList<int> card_ids;
        if (choice == "black") {
            foreach (const Card *c, player->getHandcards()) {
                if (c->isBlack() && player->canDiscard(player, c->getEffectiveId()))
                    card_ids << c->getEffectiveId();
            }
        } else if (choice == "red") {
            foreach (const Card *c, player->getHandcards()) {
                if (c->isRed() && player->canDiscard(player, c->getEffectiveId()))
                    card_ids << c->getEffectiveId();
            }
        }
        if (!card_ids.isEmpty()) {
            DummyCard *dummy = new DummyCard(card_ids);
            room->throwCard(dummy, player);
            delete dummy;
            player->drawCards(2, objectName());
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
            && player->getPhase() == Player::Draw;
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
            bool has_slash = false;
            foreach (const Card *card, player->getHandcards()) {
                if (card->isKindOf("Slash")) {
                    has_slash = true;
                    break;
                }
            }
            if (has_slash)
                break;
            player->drawCards(1, objectName());
            if (!player->askForSkillInvoke(objectName()))
                break;
        }

        return true;
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
            if (player->inMyAttackRange(p) && player->canDiscard(p, "he"))
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

IkQingmuCard::IkQingmuCard() {
}

void IkQingmuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->removePlayerMark(effect.from, "@qingmu");
    room->addPlayerMark(effect.from, "@qingmuused");
    effect.to->gainMark("@qinghuo");
}

class IkQingmuViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkQingmuViewAsSkill(): ZeroCardViewAsSkill("ikqingmu") {
    }

    virtual const Card *viewAs() const{
        return new IkQingmuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@qingmu") > 0;
    }
};

class IkQingmu: public TriggerSkill {
public:
    IkQingmu(): TriggerSkill("ikqingmu") {
        events << CardFinished << Death;
        view_as_skill = new IkQingmuViewAsSkill;
        frequency = Limited;
        limit_mark = "@qingmu";
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (triggerEvent == CardFinished && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card || !use.card->isBlack() || !use.card->isKindOf("Slash"))
                return QStringList();
            foreach (ServerPlayer *to, use.to)
                if (to->getMark("@qinghuo") > 0)
                    return QStringList(objectName());
        } else if (triggerEvent == Death && player->getMark("@qinghuo") > 0) {
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
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikqingmu", false, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkQingmuTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        ServerPlayer *target = player->tag["IkQingmuTarget"].value<ServerPlayer *>();
        player->tag.remove("IkQingmuTarget");
        if (triggerEvent == CardFinished && target) {
            ServerPlayer *victim = NULL;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@qinghuo") > 0) {
                    victim = p;
                    break;
                }
            }
            bool use = false;
            if (target->canSlash(victim, false)) {
                QString prompt = QString("@ikqingmu-slash:%1:%2").arg(player->objectName()).arg(victim->objectName());
                use = room->askForUseSlashTo(target, victim, prompt, false);
            }
            if (!use)
                player->drawCards(1, objectName());
        } else if (triggerEvent == Death && target) {
            target->gainMark("@qinghuo");
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
                    room->removeFixedDistance(player, target, 1);
            }
        }
        return QStringList();
    }
};

class IkGuoshang: public TriggerSkill {
public:
    IkGuoshang(): TriggerSkill("ikguoshang") {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &ask_who) const{
        if (player->getMark("drank") == 0)
            return QStringList();
        ServerPlayer *current = room->getCurrent();
        if (player != current && TriggerSkill::triggerable(current) && current->getPhase() != Player::NotActive) {
            ask_who = current;
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        room->broadcastSkillInvoke(objectName());
        room->setPlayerMark(player, "drank", 0);
        if (!player->isNude()) {
            int card_id = room->askForCardChosen(ask_who, player, "he", objectName());
            room->obtainCard(ask_who, card_id, false);
        }
        return false;
    }
};

class IkZuiyan: public TriggerSkill {
public:
    IkZuiyan(): TriggerSkill("ikzuiyan") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player) && player->hasSkill("ikguoshang")
            && player->getPhase() == Player::Play;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const{
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            Analeptic *anal = new Analeptic(Card::NoSuit, 0);
            anal->setSkillName("_ikzuiyan");
            if (p->isProhibited(p, anal))
                delete anal;
            else
                room->useCard(CardUseStruct(anal, p, p));
        }
        return false;
    }
};

class IkZuiyanSlash: public TriggerSkill {
public:
    IkZuiyanSlash(): TriggerSkill("#ikzuiyan") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == ChoiceMade) {
            if (player->hasFlag("IkZuiyanSlash") && data.canConvert<CardUseStruct>()) {
                player->setFlags("-IkZuiyanSlash");
                ServerPlayer *owner;
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (p->getPhase() == Player::Finish) {
                        owner = p;
                        break;
                    }
                room->notifySkillInvoked(owner, "ikzuiyan");
                if (owner == player) {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = "ikzuiyan";
                    room->sendLog(log);
                } else {
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = player;
                    log.to << owner;
                    log.arg = "ikzuiyan";
                    room->sendLog(log);
                }
            }
            return skill_list;
        }
        if (!TriggerSkill::triggerable(player) || !player->hasSkill("ikguoshang") || player->getPhase() != Player::Finish)
            return skill_list;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("drank") > 0)
                skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        ask_who->setFlags("IkZuiyanSlash");
        QString prompt = QString("@ikzuiyan-slash:%1:%2").arg(player->objectName());
        if (!room->askForUseCard(ask_who, "slash", prompt))
            ask_who->setFlags("-IkZuiyanSlash");
        return false;
    }
};

IkQihunCard::IkQihunCard() {
}

bool IkQihunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

const Card *IkQihunCard::validate(CardUseStruct &cardUse) const{
    ServerPlayer *liubei = cardUse.from;
    QList<ServerPlayer *> targets = cardUse.to;
    Room *room = liubei->getRoom();

    ServerPlayer *current = room->getCurrent();
    if (!liubei->isKongcheng()
        && current && current->getPhase() != Player::NotActive && current->hasSkill("ikqihun")
        && current->askForSkillInvoke("ikqihun_slash", "obtain:" + liubei->objectName())) {
        Card *dummy = liubei->wholeHandCards();
        current->obtainCard(dummy, false);
        delete dummy;

        LogMessage log;
        log.from = liubei;
        log.to = targets;
        log.type = "#UseCard";
        log.card_str = toString();
        room->sendLog(log);

        room->broadcastSkillInvoke("ikqihun");
        room->notifySkillInvoked(current, "ikqihun");

        Slash *slash = new Slash(NoSuit, 0);
        slash->setSkillName("ikqihun");
        return slash;
    } else
        room->setPlayerFlag(liubei, "Global_IkQihunFailed");
    return NULL;
}

class IkQihunViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkQihunViewAsSkill(): ZeroCardViewAsSkill("ikqihunv") {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        bool invoke = false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("ikqihun") && p->getPhase() != Player::NotActive) {
                invoke = true;
                break;
            }
        }
        return invoke && !player->isKongcheng()
               && pattern == "slash"
               && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
               && !player->hasFlag("Global_IkQihunFailed");
    }

    virtual const Card *viewAs() const{
        return new IkQihunCard;
    }
};

class IkQihun: public TriggerSkill {
public:
    IkQihun(): TriggerSkill("ikqihun") {
        events << GameStart << EventAcquireSkill << EventLoseSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player) return QStringList();
        if ((triggerEvent == GameStart)
            || (triggerEvent == EventAcquireSkill && data.toString() == "ikqihun")) {
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
                if (!p->hasSkill("ikqihunv"))
                    room->attachSkillToPlayer(p, "ikqihunv");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "ikqihun") {
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
                if (p->hasSkill("ikqihunv"))
                    room->detachSkillFromPlayer(p, "ikqihunv", true);
            }
        }
        return QStringList();
    }
};

class IkDiebei: public TriggerSkill {
public:
    IkDiebei(): TriggerSkill("ikdiebei") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Start;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        if (player->getHandcardNum() != player->getHp()) {
            player->drawCards(2, objectName());
        } else {
            room->loseHp(player);
            player->setFlags("ikdiebei");
        }
        return false;
    }
};

class IkDiebeiMaxCards: public MaxCardsSkill {
public:
    IkDiebeiMaxCards(): MaxCardsSkill("#ikdiebei") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasFlag("ikdiebei"))
            return 1;
        else
            return 0;
    }
};

class IkXunfeng: public TriggerSkill {
public:
    IkXunfeng(): TriggerSkill("ikxunfeng") {
        events << CardFinished;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() != Card::TypeSkill && player->getPhase() == Player::Play) {
            int n = use.card->tag["ikxunfeng_count"].toInt();
            if (n == 1)
                return QStringList(objectName());
            else if (n == 2 && player->hasFlag("IkXunfengUsed"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        int n = use.card->tag["ikxunfeng_count"].toInt();
        if (n == 2) {
            room->sendCompulsoryTriggerLog(player, objectName());
            room->broadcastSkillInvoke(objectName());
            return true;
        } else if (n == 1 && player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        int n = use.card->tag["ikxunfeng_count"].toInt();
        if (n == 2)
            room->askForDiscard(player, objectName(), 1, 1, false, true);
        else if (n == 1) {
            player->drawCards(1, objectName());
            player->setFlags("IkXunfengUsed");
        }
        return false;
    }
};

class IkXunfengRecord: public TriggerSkill {
public:
    IkXunfengRecord(): TriggerSkill("#ikxunfeng-record") {
        events << PreCardUsed << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            player->setMark("ikxunfeng_count", 0);
            if (player->hasFlag("IkXunfengUsed"))
                player->setFlags("-IkXunfengUsed");
            return QStringList();
        }
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() != Card::TypeSkill)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        player->addMark("ikxunfeng_count");
        CardUseStruct use = data.value<CardUseStruct>();
        use.card->setTag("ikxunfeng_count", player->getMark("ikxunfeng_count"));
        return false;
    }
};

class IkLuhua: public TriggerSkill {
public:
    IkLuhua(): TriggerSkill("ikluhua") {
        events << CardFinished;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() != Card::TypeSkill && player->getPhase() == Player::Play) {
            int n = use.card->tag["ikluhua_count"].toInt();
            if (n == 3) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (player == p) continue;
                    skill_list.insert(p, QStringList(objectName()));
                }
            } else if (n == 4) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasFlag("IkLuhuaUsed"))
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        CardUseStruct use = data.value<CardUseStruct>();
        int n = use.card->tag["ikluhua_count"].toInt();
        if (n == 4) {
            room->sendCompulsoryTriggerLog(ask_who, objectName());
            room->broadcastSkillInvoke(objectName());
            return true;
        } else if (n == 3 && ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        CardUseStruct use = data.value<CardUseStruct>();
        int n = use.card->tag["ikluhua_count"].toInt();
        if (n == 4)
            ask_who->drawCards(1, objectName());
        else if (n == 3) {
            player->drawCards(1, objectName());
            ask_who->setFlags("IkLuhuaUsed");
        }
        return false;
    }
};

class IkLuhuaRecord: public TriggerSkill {
public:
    IkLuhuaRecord(): TriggerSkill("#ikluhua-record") {
        events << PreCardUsed << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            player->setMark("ikluhua_count", 0);
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag("IkLuhuaUsed"))
                    p->setFlags("-IkLuhuaUsed");
            }
            return QStringList();
        }
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() != Card::TypeSkill)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        player->addMark("ikluhua_count");
        CardUseStruct use = data.value<CardUseStruct>();
        use.card->setTag("ikluhua_count", player->getMark("ikluhua_count"));
        return false;
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
            if (card && card->getTypeId() == Card::TypeBasic)
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
            const Card *card = room->askForCard(effect.to, ".", "@iklinghui-discard");
            if (!card) {
                card = effect.to->getRandomHandCard();
                room->throwCard(card, effect.to);
            }
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
        LogMessage log;
        log.type = "$EnterDiscardPile";
        log.card_str = IntList2StringList(card_ids).join("+");
        room->sendLog(log);
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
        frequency = Compulsory;
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
        return !player->isKongcheng() && !player->hasUsed("IkDianyanCard") && player->getPile("ikdianyanpile").length() < 3;
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
            if (owner->getMark(objectName()) == 0 && owner->distanceTo(player) == 1)
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

class IkQile: public TriggerSkill {
public:
    IkQile(): TriggerSkill("ikqile") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Play
            && player->canDiscard(player, "h");
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (room->askForCard(player, "BasicCard", "@ikqile", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(2, objectName());
        room->setPlayerCardLimitation(player, "use", "Slash", true);
        return false;
    }
};

class IkSaoxiaoViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkSaoxiaoViewAsSkill(): ZeroCardViewAsSkill("iksaoxiao") {
        response_pattern = "@@iksaoxiao!";
    }

    virtual const Card *viewAs() const{
        if (Self->property("iksaoxiao").isNull())
            return NULL;
        const Card *card = Card::Parse(Self->property("iksaoxiao").toString());
        Card *use_card = Sanguosha->cloneCard(card);
        use_card->setSkillName("_iksaoxiao");
        return use_card;
    }
};

class IkSaoxiao: public TriggerSkill {
public:
    IkSaoxiao(): TriggerSkill("iksaoxiao") {
        events << DamageComplete;
        view_as_skill = new IkSaoxiaoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player) || player->isKongcheng())
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || damage.from->isDead())
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        const Card *hand = room->askForCard(player, ".!", "@iksaoxiao", data, Card::MethodNone);
        if (!hand)
            hand = player->getRandomHandCard();
        QString choice = room->askForChoice(damage.from, objectName(), "red+black");
        LogMessage log;
        log.type = "#IkSaoxiaoChoice";
        log.from = damage.from;
        log.arg = choice;
        room->sendLog(log);
        room->showCard(player, hand->getEffectiveId());
        if ((choice == "red" && !hand->isRed()) || (choice == "black" && !hand->isBlack())) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (hand->isAvailable(p)) {
                    if (hand->targetFixed())
                        targets << p;
                    else {
                        bool can_use = false;
                        foreach (ServerPlayer *p2, room->getAlivePlayers())
                            if (hand->targetFilter(QList<const Player *>(), p2, p)) {
                                can_use = true;
                                break;
                            }
                        if (can_use)
                            targets << p;
                    }
                }
            }
            if (!targets.isEmpty()) {
                ServerPlayer *user = room->askForPlayerChosen(player, targets, objectName(), "@iksaoxiao-choose", true);
                if (user) {
                    room->setPlayerProperty(user, "iksaoxiao", hand->toString());
                    room->askForUseCard(user, "@@iksaoxiao!", "@iksaoxiao-use");
                    room->setPlayerProperty(user, "iksaoxiao", QVariant());
                }
            }
        }
        return false;
    }
};

IkXiaowuCard::IkXiaowuCard() {
    target_fixed = true;
    will_throw = false;
}

const Card *IkXiaowuCard::validate(CardUseStruct &card_use) const{
    const Card *card = Sanguosha->getCard(getEffectiveId());
    QString cardname;
    if(card->isRed())
        cardname = "indulgence";
    else
        cardname = "supply_shortage";

    Card *newcard = Sanguosha->cloneCard(cardname, card->getSuit(), card->getNumber());
    newcard->addSubcard(card);
    newcard->setSkillName("ikxiaowu");
    card_use.to << card_use.from;
    return newcard;
}

class IkXiaowuViewAsSkill: public OneCardViewAsSkill {
public:
    IkXiaowuViewAsSkill(): OneCardViewAsSkill("ikxiaowu") {
        filter_pattern = ".|diamond#^TrickCard|black";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        if (player->hasUsed("IkXiaowuCard")) return false;
        bool can_le = true, can_bing = true;
        if (!player->containsTrick("indulgence")) {
            Card *le = Sanguosha->cloneCard("indulgence");
            le->deleteLater();
            if (player->isCardLimited(le, Card::MethodUse))
                can_le = false;
        } else
            can_le = false;

        if (!player->containsTrick("supply_shortage")) {
            Card *bing = Sanguosha->cloneCard("supply_shortage");
            bing->deleteLater();
            if (player->isCardLimited(bing, Card::MethodUse))
                can_bing = false;
        } else
            can_bing = false;

        return can_le || can_bing;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        QString cardname;
        if(originalCard->isRed())
            cardname = "indulgence";
        else
            cardname = "supply_shortage";
        Card *trick = Sanguosha->cloneCard(cardname);
        trick->addSubcard(originalCard);
        trick->deleteLater();
        if (Self->containsTrick(cardname) || Self->isCardLimited(trick, Card::MethodUse))
            return NULL;

        IkXiaowuCard *card = new IkXiaowuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class IkXiaowu: public TriggerSkill {
public:
    IkXiaowu(): TriggerSkill("ikxiaowu") {
        events << EventPhaseChanging << Death;
        view_as_skill = new IkXiaowuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player != NULL && player->tag["IkXiaowuTarget"].value<ServerPlayer *>() != NULL) {
            if (triggerEvent == EventPhaseChanging) {
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.to != Player::NotActive)
                    return QStringList();
            }
            ServerPlayer *target = player->tag["IkXiaowuTarget"].value<ServerPlayer *>();
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player) {
                    if (death.who == target) {
                        room->removeFixedDistance(player, target, 1);
                        player->tag.remove("IkXiaowuTarget");
                    }
                    return QStringList();
                }
            }
            if (target) {
                room->removeFixedDistance(player, target, 1);
                player->tag.remove("IkXiaowuTarget");
            }
        }
        return QStringList();
    }
};

class IkXiaowuSlash: public TriggerSkill {
public:
    IkXiaowuSlash(): TriggerSkill("#ikxiaowu") {
        events << CardFinished;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName(false) == "ikxiaowu")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->drawCards(1);

        ServerPlayer *victim = room->askForPlayerChosen(player, room->getOtherPlayers(player), "ikxiaowu");
        room->setFixedDistance(player, victim, 1);
        player->tag["IkXiaowuTarget"] = QVariant::fromValue(victim);
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_ikxiaowu");
        if (player->canSlash(victim, slash, false)) {
            CardUseStruct use;
            use.card = slash;
            use.from = player;
            use.to << victim;
            victim->addQinggangTag(slash);
            room->useCard(use, false);
        } else
            delete slash;
        return false;
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
        if (player->isDead() || player->property("ikqisi").isNull())
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
        if (player->isDead())
            return skill_list;
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (owner == player) continue;
            if (player->getHandcardNum() <= owner->getHandcardNum() && owner->getHandcardNum() % 2 == 0)
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
        if (winner->getHandcardNum() < winner->getMaxHp())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        winner->drawCards(winner->getMaxHp() - winner->getHandcardNum(), objectName());
        return false;
    }
};

IkXianlvCard::IkXianlvCard() {
    will_throw = false;
    handling_method = MethodNone;
}

bool IkXianlvCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.isEmpty() && to_select->getPhase() == Player::Draw;
}

void IkXianlvCard::onEffect(const CardEffectStruct &effect) const{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "ikxianlv", QString());
    Room *room = effect.from->getRoom();
    if (effect.to == effect.from) {
        LogMessage log;
        log.type = "$MoveCard";
        log.from = effect.from;
        log.to << effect.to;
        log.card_str = IntList2StringList(subcards).join("+");
        room->sendLog(log);
    }
    room->obtainCard(effect.to, this, reason);
    effect.from->drawCards(1, "ikxianlv");
    room->addPlayerMark(effect.to, "ikxianlv");
}

class IkXianlvViewAsSkill: public ViewAsSkill {
public:
    IkXianlvViewAsSkill(): ViewAsSkill("ikxianlv") {
        expand_pile = "music";
        response_pattern = "@@ikxianlv";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return Self->getPile("music").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;
        IkXianlvCard *card = new IkXianlvCard;
        card->addSubcards(cards);
        return card;
    }
};

class IkXianlv: public TriggerSkill {
public:
    IkXianlv(): TriggerSkill("ikxianlv") {
        events << DrawNCards << EventPhaseChanging << EventPhaseStart << FinishJudge;
        view_as_skill = new IkXianlvViewAsSkill;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == DrawNCards) {
            if (player->getMark(objectName()) > 0) {
                data = data.toInt() - player->getMark(objectName());
                room->setPlayerMark(player, objectName(), 0);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            room->setPlayerMark(player, objectName(), 0);
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName())
                judge->pattern = QString::number(int(judge->card->getSuit()));
        } else if (player->getPhase() == Player::Draw) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                if (!owner->getPile("music").isEmpty())
                    skill_list.insert(owner, QStringList(objectName()));
        } else if (TriggerSkill::triggerable(player) && (player->getPhase() == Player::Start || player->getPhase() == Player::Finish)) {
            skill_list.insert(player, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *owner) const{
        if (player->getPhase() == Player::Draw)
            room->askForUseCard(owner, "@@ikxianlv", "@ikxianlv", -1, Card::MethodNone);
        else if (owner->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.who = player;
        judge.reason = objectName();
        room->judge(judge);

        Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
        bool has = false;
        foreach (int id, player->getPile("music"))
            if (Sanguosha->getCard(id)->getSuit() == suit) {
                has = true;
                break;
            }
        if (!has)
            player->addToPile("music", judge.card);
        return false;
    }
};

IkLianwuCard::IkLianwuCard() {
}

bool IkLianwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->canDiscard(to_select, "he") && to_select != Self;
}

void IkLianwuCard::onUse(Room *room, const CardUseStruct &use) const{
    if (subcards.isEmpty())
        room->loseHp(use.from);
    SkillCard::onUse(room, use);
}

void IkLianwuCard::onEffect(const CardEffectStruct &effect) const{
    if (!effect.from->isAlive())
        return;
    Room *room = effect.from->getRoom();
    int id = room->askForCardChosen(effect.from, effect.to, "he", "iklianwu", false, MethodDiscard);
    CardType type = Sanguosha->getCard(id)->getTypeId();
    room->throwCard(id, effect.to, effect.from);
    switch (type) {
    case TypeTrick :
        room->askForUseCard(effect.from, "@@iklianwu1", "@iklianwu1", 1);
        break;
    case TypeBasic:
        room->setPlayerFlag(effect.from, "iklianwu2");
        break;
    case TypeEquip:
        room->setPlayerFlag(effect.from, "iklianwu3");
        break;
    default:
        break;
    }
}

IkLianwuDrawCard::IkLianwuDrawCard() {
    m_skillName = "iklianwu";
}

bool IkLianwuDrawCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const{
    return targets.isEmpty() || targets.length() < Self->getLostHp();
}

void IkLianwuDrawCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(1, "iklianwu");
}

class IkLianwuViewAsSkill: public ViewAsSkill {
public:
    IkLianwuViewAsSkill(): ViewAsSkill("iklianwu") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern.startsWith("@@iklianwu");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1"))
            return false;
        else
            return selected.isEmpty() && !to_select->isKindOf("BasicCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1")) {
            return cards.isEmpty() ? new IkLianwuDrawCard : NULL;
        } else {
            IkLianwuCard *card = new IkLianwuCard;
            card->addSubcards(cards);
            return card;
        }
    }
};

class IkLianwu: public TriggerSkill {
public:
    IkLianwu(): TriggerSkill("iklianwu") {
        events << EventPhaseStart;
        view_as_skill = new IkLianwuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->askForUseCard(player, "@@iklianwu0", "@iklianwu0", 0, Card::MethodDiscard);
        return false;
    }
};

class IkLianwuDistance: public DistanceSkill {
public:
    IkLianwuDistance(): DistanceSkill("#iklianwu-dist") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasFlag("iklianwu2"))
            return -qMax(1, from->getLostHp());
        else
            return 0;
    }
};

class IkLianwuTargetMod: public TargetModSkill {
public:
    IkLianwuTargetMod(): TargetModSkill("#iklianwu-tar") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("iklianwu3"))
            return qMax(1, from->getLostHp());
        else
            return 0;
    }
};

class IkMoshanFilter: public FilterSkill {
public:
    IkMoshanFilter(): FilterSkill("ikmoshan") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        ServerPlayer *owner = Sanguosha->currentRoom()->getCardOwner(to_select->getEffectiveId());
        if (!owner || !owner->hasSkill("thyanmeng"))
            return false;
        return to_select->isKindOf("Jink");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        EightDiagram *ed = new EightDiagram(originalCard->getSuit(), originalCard->getNumber());
        ed->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(ed);
        return card;
    }
};

class IkMoshan: public TriggerSkill {
public:
    IkMoshan(): TriggerSkill("ikmoshan") {
        events << BeforeCardsMove;
        view_as_skill = new IkMoshanFilter;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && player->hasSkill("thyanmeng") && !player->hasFlag("ikmoshan")) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player)
                return QStringList();
            int i = 0;
            foreach (int card_id, move.card_ids) {
                if (room->getCardOwner(card_id) == move.from
                    && move.from_places[i] == Player::PlaceEquip
                    && room->getCard(card_id)->isKindOf("EightDiagram")) {
                    return QStringList(objectName());
                }
                i++;
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids;
        ids << player->getArmor()->getEffectiveId();
        move.removeCardIds(ids);
        data = QVariant::fromValue(move);
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikmoshan");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), target->objectName(), objectName(), QString());
        player->setFlags("ikmoshan");
        room->obtainCard(target, player->getArmor(), reason);
        player->setFlags("-ikmoshan");
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (player->canDiscard(p, "he"))
                victims << p;
        if (!victims.isEmpty()) {
            ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
            int id = room->askForCardChosen(player, victim, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, victim, player);
        }
        return false;
    }
};

IkXiekeCard::IkXiekeCard() {
}

bool IkXiekeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("ikxieke").value<const Card *>();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool IkXiekeCard::targetFixed() const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("ikxieke").value<const Card *>();
    return card && card->targetFixed();
}

bool IkXiekeCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("ikxieke").value<const Card *>();
    return card && card->targetsFeasible(targets, Self);
}

const Card *IkXiekeCard::validate(CardUseStruct &card_use) const{
    ServerPlayer *wenyang = card_use.from;
    Room *room = wenyang->getRoom();

    QString to_ikshidao = user_string;
    if (user_string == "slash"
        && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QStringList ikshidao_list;
            ikshidao_list << "slash";
            if (!Config.BanPackages.contains("maneuvering"))
                ikshidao_list << "thunder_slash" << "fire_slash";
            to_ikshidao = room->askForChoice(wenyang, "ikxieke_slash", ikshidao_list.join("+"));
    }

    Card *use_card = Sanguosha->cloneCard(to_ikshidao);
    use_card->setSkillName("ikxieke");
    return use_card;
}

const Card *IkXiekeCard::validateInResponse(ServerPlayer *wenyang) const{
    Room *room = wenyang->getRoom();

    QString to_ikshidao;
    if (user_string == "peach+analeptic") {
        QStringList ikshidao_list;
        ikshidao_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            ikshidao_list << "analeptic";
        to_ikshidao = room->askForChoice(wenyang, "ikxieke_saveself", ikshidao_list.join("+"));
    } else if (user_string == "slash") {
        QStringList ikshidao_list;
        ikshidao_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            ikshidao_list << "thunder_slash" << "fire_slash";
        to_ikshidao = room->askForChoice(wenyang, "ikxieke_slash", ikshidao_list.join("+"));
    } else
        to_ikshidao = user_string;

    Card *use_card = Sanguosha->cloneCard(to_ikshidao);
    use_card->setSkillName("ikxieke");
    return use_card;
}

class IkXieke: public ZeroCardViewAsSkill {
public:
    IkXieke(): ZeroCardViewAsSkill("ikxieke") {
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getPhase() != Player::Play || player->isChained()) return false;
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
        if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (player->aliveCount() != 2 && (pattern.contains("analeptic") || pattern == "jink" || pattern == "nullification"))
            return true;
        else if (pattern == "slash")
            return true;
        return false;
    }

    virtual QDialog *getDialog() const{
        return ThMimengDialog::getInstance("ikxieke", true, false);
    }

    virtual const Card *viewAs() const{
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            IkXiekeCard *tianyan_card = new IkXiekeCard;
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";
            tianyan_card->setUserString(pattern);
            return tianyan_card;
        }

        const Card *c = Self->tag["ikxieke"].value<const Card *>();
        if (c) {
            IkXiekeCard *card = new IkXiekeCard;
            card->setUserString(c->objectName());
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isChained() && (player->aliveCount() != 2 || Slash::IsAvailable(player));
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return !player->isChained() && player->getPhase() == Player::Play && player->aliveCount() != 2;
    }
};

class IkXiekeChain: public TriggerSkill {
public:
    IkXiekeChain(): TriggerSkill("#ikxieke") {
        events << CardFinished;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player->isChained())
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card && use.card->getSkillName() == "ikxieke")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->setChained(true);
        room->setEmotion(player, "chain");
        room->broadcastProperty(player, "chained");
        room->getThread()->trigger(ChainStateChanged, room, player);

        return false;
    }
};

class IkYunmai: public TriggerSkill {
public:
    IkYunmai(): TriggerSkill("ikyunmai") {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && data.value<PhaseChangeStruct>().to == Player::NotActive)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        int half = (room->alivePlayerCount() + 1) / 2;
        QList<ServerPlayer *> chained, not_chained, targets;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (p->isChained())
                chained << p;
            else
                not_chained << p;
        if (chained.length() >= half)
            targets += chained;
        if (not_chained.length() >= half)
            targets += not_chained;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@ikyunmai", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkYunmaiTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = player->tag["IkYunmaiTarget"].value<ServerPlayer *>();
        player->tag.remove("IkYunmaiTarget");
        if (target) {
            target->setChained(!target->isChained());
            room->setEmotion(target, "chain");
            room->broadcastProperty(target, "chained");
            room->getThread()->trigger(ChainStateChanged, room, target);
        }
        return false;
    }
};

class IkLunyao: public TriggerSkill {
public:
    IkLunyao(): TriggerSkill("iklunyao") {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player)
            return QStringList();
        if ((move.to && move.to != player && move.to_place == Player::PlaceHand)
            || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                && move.reason.m_playerId != player->objectName()))
            foreach (Player::Place place, move.from_places)
                if (place == Player::PlaceHand || place == Player::PlaceEquip)
                    return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids;
        int i = 0;
        foreach (int id, move.card_ids) {
            if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)
                ids << id;
            i++;
        }
        QList<int> pile_ids = room->getNCards(ids.length(), false);
        room->moveCardsAtomic(CardsMoveStruct(pile_ids, NULL, move.to, Player::DrawPile, move.to_place, move.reason), false);
        move.removeCardIds(ids);
        data = QVariant::fromValue(move);
        return false;
    }
};

class IkQimu: public TriggerSkill {
public:
    IkQimu(): TriggerSkill("ikqimu") {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.to_place == Player::DiscardPile
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE) {
            const Card *yaopu_card = move.reason.m_extraData.value<const Card *>();
            if (!yaopu_card || !yaopu_card->hasFlag("ikqimu"))
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
        const Card *card = move.reason.m_extraData.value<const Card *>();
        target->obtainCard(card);
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);
        return false;
    }
};

class IkQimuRecord: public TriggerSkill {
public:
    IkQimuRecord(): TriggerSkill("#ikqimu-record") {
        events << PreCardUsed << CardResponded;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        const Card *card = NULL;
        if (triggerEvent == PreCardUsed)
            card = data.value<CardUseStruct>().card;
        else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
               card = response.m_card;
        }
        if (card && card->getHandlingMethod() == Card::MethodUse) {
            ServerPlayer *current = room->getCurrent();
            if (current && current->getPhase() != Player::NotActive && !current->hasFlag("ikqimu")) {
                current->setFlags("ikqimu");
                QList<int> ids;
                if (!card->isVirtualCard())
                    ids << card->getEffectiveId();
                else if (card->subcardsLength() > 0)
                    ids = card->getSubcards();
                if (!ids.isEmpty())
                    room->setCardFlag(card, "ikqimu");
            }
        }

        return QStringList();
    }
};

class IkYuanji: public TriggerSkill {
public:
    IkYuanji(): TriggerSkill("ikyuanji") {
        events << GameStart << DrawNCards << PreCardUsed << Death;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player && player->isDead() && player->hasSkill(objectName(), true))
                return QStringList(objectName());
            if (TriggerSkill::triggerable(player) && death.who->getMark("@shuling") > 0)
                return QStringList(objectName());
            return QStringList();
        }
        if (triggerEvent == GameStart && TriggerSkill::triggerable(player))
            return QStringList(objectName());
        else if (triggerEvent == DrawNCards && player->getMark("@shuling") > 0)
            return QStringList(objectName());
        else if (triggerEvent == PreCardUsed && player->getMark("@shuling") > 0
                 && player->getPhase() == Player::Play && !player->hasFlag("shuling_slash")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player) {
                room->sendCompulsoryTriggerLog(player, objectName());
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("@shuling") > 0)
                        p->loseAllMarks("@shuling");
                }
                if (player->getMark("@shuling") > 0)
                    player->loseAllMarks("@shuling");
            } else {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->gainMark("@shuling");
                death.who->loseAllMarks("@shuling");
            }
        } else if (triggerEvent == GameStart) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->gainMark("@shuling");
        } else if (triggerEvent == DrawNCards) {
            ServerPlayer *target = room->findPlayerBySkillName(objectName());
            if (target)
                room->sendCompulsoryTriggerLog(target, objectName(), false);
            room->notifySkillInvoked(player, objectName());
            data = data.toInt() + 1;
        } else if (triggerEvent == PreCardUsed) {
            ServerPlayer *target = room->findPlayerBySkillName(objectName());
            if (target)
                room->sendCompulsoryTriggerLog(target, objectName(), false);
            room->notifySkillInvoked(player, objectName());
            CardUseStruct use = data.value<CardUseStruct>();
            room->addPlayerHistory(player, use.card->getClassName(), -1);
            player->setFlags("shuling_slash");
        }
        return false;
    }
};

class IkShuluo: public TriggerSkill {
public:
    IkShuluo(): TriggerSkill("ikshuluo") {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player->isDead() || player->getCardCount() < 2)
            return QStringList();
        ServerPlayer *p = room->findPlayerBySkillName(objectName());
        if (!p)
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (player == damage.to || damage.to->getMark("@shuling") == 0)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->hasSkill(objectName())) {
            if (player->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        } else {
            const Card *dummy = room->askForExchange(player, objectName(), 2, 2, true, "@ikshuluo", true);
            if (dummy && dummy->subcardsLength() > 0) {
                LogMessage log;
                log.type = "$DiscardCardWithSkill";
                log.from = player;
                log.card_str = IntList2StringList(dummy->getSubcards()).join("+");
                log.arg = objectName();
                room->sendLog(log);
                CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName());
                room->moveCardTo(dummy, NULL, Player::DiscardPile, reason, true);
                delete dummy;
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        room->setPlayerMark(damage.to, "@shuling", 0);
        player->gainMark("@shuling");
        return false;
    }
};

class IkZhiwang: public PhaseChangeSkill {
public:
    IkZhiwang(): PhaseChangeSkill("ikzhiwang") {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Play;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (player->canSlash(p, false) && player->inMyAttackRange(p))
                targets << p;
        }
        targets << player;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_ikzhiwang");
        room->useCard(CardUseStruct(slash, player, target));
        return false;
    }
};

class IkLianlong: public DistanceSkill {
public:
    IkLianlong(): DistanceSkill("iklianlong") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName())) {
            if (from->getHandcardNum() % 2 == 1)
                return 1;
            else
                return -1;
        }
        return 0;
    }
};

class IkHuanxian: public TriggerSkill {
public:
    IkHuanxian(): TriggerSkill("ikhuanxian") {
        events << CardAsked;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return QStringList();
        QString pattern = data.toStringList().first();
        if (pattern == "jink")
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
        QString pattern = data.toStringList().first();
        bool jink = true;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            const Card *card = room->askForCard(p, "..", "@ikhuanxian-give:" + player->objectName(), QVariant(), Card::MethodNone, player);
            if (card) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, p->objectName(), player->objectName(), objectName());
                room->obtainCard(player, card, reason, false);
                jink = false;
            }
        }
        if (jink) {
            Card *card = new Jink(Card::NoSuit, 0);
            card->setSkillName(objectName());
            room->provide(card);
            return true;
        }
        return false;
    }
};

class IkWuyu: public TriggerSkill {
public:
    IkWuyu(): TriggerSkill("ikwuyu") {
        events << EventPhaseEnd << EventPhaseChanging;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == EventPhaseChanging) {
            player->tag.remove("IkWuyu");
        } else if (triggerEvent == EventPhaseEnd) {
            if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::Discard)
                return skill_list;
            QVariantList ikwuyu = player->tag["IkWuyu"].toList();
            foreach (QVariant card_data, ikwuyu) {
                int card_id = card_data.toInt();
                if (room->getCardPlace(card_id) == Player::DiscardPile) {
                    foreach (ServerPlayer *p, room->getAllPlayers()) {
                        if (player->inMyAttackRange(p))
                            skill_list.insert(p, QStringList(objectName()));
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        return ask_who->askForSkillInvoke("ikwuyu_get");
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        QVariantList ikwuyu = player->tag["IkWuyu"].toList();

        QList<int> cards;
        foreach (QVariant card_data, ikwuyu) {
            int card_id = card_data.toInt();
            if (room->getCardPlace(card_id) == Player::DiscardPile)
                cards << card_id;
        }

        if (cards.isEmpty())
            return false;

        room->fillAG(cards, ask_who);

        int to_get = room->askForAG(ask_who, cards, true, objectName());
        room->clearAG(ask_who);
        if (to_get == -1)
            return false;
        room->broadcastSkillInvoke(objectName());
        LogMessage log;
        log.type = "#InvokeOthersSkill";
        log.from = ask_who;
        log.to << player;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());

        bool red = Sanguosha->getCard(to_get)->isRed();
        ask_who->obtainCard(Sanguosha->getCard(to_get));

        ikwuyu.removeAll(to_get);

        if (red && player->isWounded())
            room->recover(player, RecoverStruct(ask_who));
        player->tag["IkWuyu"] = ikwuyu;
        return false;
    }
};

class IkWuyuRecord : public TriggerSkill {
public:
    IkWuyuRecord() : TriggerSkill("#ikwuyu-record") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *erzhang, QVariant &data, ServerPlayer * &) const{
        if (!erzhang || !erzhang->isAlive() || !erzhang->hasSkill("ikwuyu")) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (erzhang->getPhase() == Player::Discard) {
            QVariantList ikwuyu = erzhang->tag["IkWuyu"].toList();

            if (move.to_place == Player::DiscardPile) {
                foreach (int id, move.card_ids) {
                    if (!ikwuyu.contains(id))
                        ikwuyu << id;
                }
            }

            erzhang->tag["IkWuyu"] = ikwuyu;
        }

        return QStringList();
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
    wind033->addSkill(new SlashNoDistanceLimitSkill("ikjilun"));
    related_skills.insertMulti("ikjilun", "#ikjilun-slash-ndl");

    General *wind034 = new General(this, "wind034", "kaze");
    wind034->addSkill(new IkJiqiao);
    wind034->addSkill(new SlashNoDistanceLimitSkill("ikjiqiao"));
    related_skills.insertMulti("ikjiqiao", "#ikjiqiao-slash-ndl");

    General *wind035 = new General(this, "wind035", "kaze");
    wind035->addSkill(new IkKangjin);
    wind035->addSkill(new IkKangjinTrigger);
    related_skills.insertMulti("ikkangjin", "#ikkangjin");

    General *wind036 = new General(this, "wind036", "kaze");
    wind036->addSkill(new IkHunkao);
    wind036->addSkill(new SlashNoDistanceLimitSkill("ikhunkao"));
    related_skills.insertMulti("ikhunkao", "#ikhunkao-slash-ndl");

    General *wind045 = new General(this, "wind045", "kaze");
    wind045->addSkill(new IkHudie);
    wind045->addSkill(new Skill("ikyinsha", Skill::Compulsory));
    wind045->addSkill(new IkHualan);

    General *wind047 = new General(this, "wind047", "kaze", 3);
    wind047->addSkill(new IkTianhua);
    wind047->addSkill(new IkHuangshi);

    General *wind048 = new General(this, "wind048", "kaze");
    wind048->addSkill(new IkXizi);

    General *bloom032 = new General(this, "bloom032", "hana");
    bloom032->addSkill(new IkFengxing);

    General *bloom034 = new General(this, "bloom034", "hana");
    bloom034->addSkill(new IkQizhong);

    General *bloom035 = new General(this, "bloom035", "hana");
    bloom035->addSkill(new IkDuduan);
    bloom035->addSkill(new IkDuduanMaxCards);
    related_skills.insertMulti("ikduduan", "#ikduduan");

    General *bloom036 = new General(this, "bloom036", "hana");
    bloom036->addSkill(new IkQingmu);

    General *bloom045 = new General(this, "bloom045", "hana", 4, false);
    bloom045->addSkill(new IkDengpo);

    General *bloom047 = new General(this, "bloom047", "hana", 3);
    bloom047->addSkill(new IkGuoshang);
    bloom047->addSkill(new IkZuiyan);
    bloom047->addSkill(new IkZuiyanSlash);
    related_skills.insertMulti("ikzuiyan", "#ikzuiyan");
    bloom047->addSkill(new IkQihun);

    General *bloom048 = new General(this, "bloom048", "hana");
    bloom048->addSkill(new IkDiebei);
    bloom048->addSkill(new IkDiebeiMaxCards);
    related_skills.insertMulti("ikdiebei", "#ikdiebei");

    General *bloom049 = new General(this, "bloom049", "hana", 3);
    bloom049->addSkill(new IkXunfeng);
    bloom049->addSkill(new IkXunfengRecord);
    related_skills.insertMulti("ikxunfeng", "#ikxunfeng-record");
    bloom049->addSkill(new IkLuhua);
    bloom049->addSkill(new IkLuhuaRecord);
    related_skills.insertMulti("ikluhua", "#ikluhua-record");

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

    General *snow048 = new General(this, "snow048", "yuki");
    snow048->addSkill(new IkQile);
    snow048->addSkill(new IkSaoxiao);

    General *snow049 = new General(this, "snow049", "yuki");
    snow049->addSkill(new IkXiaowu);
    snow049->addSkill(new IkXiaowuSlash);
    related_skills.insertMulti("ikxiaowu", "#ikxiaowu");

    General *luna030 = new General(this, "luna030", "tsuki");
    luna030->addSkill(new IkLingcu);

    General *luna031 = new General(this, "luna031", "tsuki", 3);
    luna031->addSkill(new IkQisi);
    luna031->addSkill(new IkMiaoxiang);

    General *luna032 = new General(this, "luna032", "tsuki", 3, false);
    luna032->addSkill(new IkJichang);
    luna032->addSkill(new IkManwu);

    General *luna033 = new General(this, "luna033", "tsuki");
    luna033->addSkill(new IkXianlv);

    General *luna036 = new General(this, "luna036", "tsuki");
    luna036->addSkill(new IkLianwu);
    luna036->addSkill(new IkLianwuDistance);
    luna036->addSkill(new IkLianwuTargetMod);
    related_skills.insertMulti("iklianwu", "#iklianwu-dist");
    related_skills.insertMulti("iklianwu", "#iklianwu-tar");

    General *luna037 = new General(this, "luna037", "tsuki");
    luna037->addSkill(new IkMoshan);
    luna037->addSkill("thyanmeng");

    General *luna045 = new General(this, "luna045", "tsuki");
    luna045->addSkill(new IkXieke);
    luna045->addSkill(new IkXiekeChain);
    related_skills.insertMulti("ikxieke", "#ikxieke");
    luna045->addSkill(new IkYunmai);

    General *luna046 = new General(this, "luna046", "tsuki", 3);
    luna046->addSkill(new IkLunyao);
    luna046->addSkill(new IkQimu);
    luna046->addSkill(new IkQimuRecord);
    related_skills.insertMulti("ikqimu", "#ikqimu-record");

    General *luna047 = new General(this, "luna047", "tsuki");
    luna047->addSkill(new IkYuanji);
    luna047->addSkill(new IkShuluo);

    General *luna048 = new General(this, "luna048", "tsuki");
    luna048->addSkill(new IkZhiwang);
    luna048->addSkill(new SlashNoDistanceLimitSkill("ikzhiwang"));
    related_skills.insertMulti("ikzhiwang", "#ikzhiwang-slash-ndl");
    luna048->addSkill(new IkLianlong);

    General *luna049 = new General(this, "luna049", "tsuki", 3);
    luna049->addSkill(new IkHuanxian);
    luna049->addSkill(new IkWuyu);
    luna049->addSkill(new IkWuyuRecord);
    related_skills.insertMulti("ikwuyu", "#ikwuyu-record");

    addMetaObject<IkZhijuCard>();
    addMetaObject<IkJilunCard>();
    addMetaObject<IkKangjinCard>();
    addMetaObject<IkHunkaoCard>();
    addMetaObject<IkHuangshiCard>();
    addMetaObject<IkQingmuCard>();
    addMetaObject<IkDengpoCard>();
    addMetaObject<IkQihunCard>();
    addMetaObject<IkShidaoCard>();
    addMetaObject<IkLinghuiCard>();
    addMetaObject<IkDianyanCard>();
    addMetaObject<IkDianyanPutCard>();
    addMetaObject<IkXiaowuCard>();
    addMetaObject<IkQisiCard>();
    addMetaObject<IkManwuCard>();
    addMetaObject<IkXianlvCard>();
    addMetaObject<IkLianwuCard>();
    addMetaObject<IkLianwuDrawCard>();
    addMetaObject<IkXiekeCard>();

    skills << new IkQihunViewAsSkill << new IkDianyanPut;
}

ADD_PACKAGE(IkaiKa)
