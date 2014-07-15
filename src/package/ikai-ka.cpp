#include "ikai-ka.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"

IkZhijuCard::IkZhijuCard() {
    target_fixed = true;
    will_throw = false;
    handling_method = MethodNone;
}

void IkZhijuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, source->objectName(), QString(), "ikzhiju", QString());
    room->moveCardsAtomic(CardsMoveStruct(subcards, NULL, Player::DrawPile, reason), false);
    if (source->isAlive()) {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (source->canDiscard(p, "ej"))
                targets << p;
        if (targets.isEmpty()) return;
        ServerPlayer *target = room->askForPlayerChosen(source, targets, "ikzhiju", "@ikzhiju");
        int card_id = room->askForCardChosen(source, target, "ej", "ikzhiju", false, MethodDiscard);
        room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : target, source);
    }
}

class IkZhiju: public ViewAsSkill {
public:
    IkZhiju(): ViewAsSkill("ikzhiju") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        IkZhijuCard *card = new IkZhijuCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() >= 2;
    }
};

class IkYingqi: public TriggerSkill {
public:
    IkYingqi(): TriggerSkill("ikyingqi") {
        events << EventPhaseStart;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() == Player::Discard && player->getHandcardNum() >= player->getHp()) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->drawCards(1);
        return false;
    }
};

IkJilunCard::IkJilunCard() {
}

bool IkJilunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasEquip() && to_select != Self;
}

void IkJilunCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (effect.to->hasEquip()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "e", "ikjilun");
        room->obtainCard(effect.to, card_id);
    }
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_ikjilun");
    if (effect.from->canSlash(effect.to, slash, false) && !effect.from->isCardLimited(slash, Card::MethodUse))
        room->useCard(CardUseStruct(slash, effect.from, effect.to), false);
    else
        delete slash;
}

class IkJilun: public OneCardViewAsSkill {
public:
    IkJilun(): OneCardViewAsSkill("ikjilun") {
        filter_pattern = ".|.|.|hand!";
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Duel") && damage.card->getSkillName() == "ikkangjin") {
            ask_who = damage.by_user ? damage.from : player;
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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
    int count = 0;
    QString choice;
    foreach (QString suit, all_suit) {
        QCommandLinkButton *button = new QCommandLinkButton;
        button->setIcon(QIcon(QString("image/system/suit/%1.png").arg(suit)));
        button->setText(Sanguosha->translate(suit));
        button->setObjectName(suit);
        group->addButton(button);

        button->setEnabled(suits.contains(suit));

        if (button->isEnabled()) {
            count++;
            choice = suit;
        }
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (TriggerSkill::triggerable(player) && damage.from && damage.from->getKingdom() != player->getKingdom())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (TriggerSkill::triggerable(player) && damage.to && damage.to->isAlive() && damage.to->getKingdom() != player->getKingdom())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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
                card_ex = room->askForCard(player, "^" + QString::number(use.card->getEffectiveId()),
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *from = damage.from;
            if (from && from->getPhase() == Player::Play && from->hasFlag(objectName()))
                from->setFlags("-" + objectName());
        } else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Start)
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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
    room->setFixedDistance(effect.from, effect.to, 1);
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

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const{
        if (triggerEvent == CardFinished) {
            if (player->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        } else if (triggerEvent == Death) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikpaomu", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["IkPaomuTarget"] = QVariant::fromValue(target);
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const{
        if (triggerEvent == CardFinished) {
            ServerPlayer *victim = NULL;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@liebiao") > 0) {
                    victim = p;
                    break;
                }
            }
            if (victim) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->inMyAttackRange(victim)) continue;
                    if (!room->askForUseSlashTo(p, victim, QString("@ikpaomu-slash:%1:%2").arg(player->objectName()).arg(victim->objectName())))
                        player->drawCards(1, objectName());
                }
            }
        } else if (triggerEvent == Death) {
            ServerPlayer *target = player->tag["IkPaomuTarget"].value<ServerPlayer *>();
            player->tag.remove("IkPaomuTarget");
            if (target) {
                target->gainMark("@liebiao");
                room->setFixedDistance(player, target, 1);
            }
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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

IkaiKaPackage::IkaiKaPackage()
    :Package("ikai-ka")
{
    General *wind025 = new General(this, "wind025", "kaze", 3);
    wind025->addSkill(new IkZhiju);
    wind025->addSkill(new IkYingqi);

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

    General *bloom045 = new General(this, "bloom045", "hana");
    bloom045->addSkill(new IkDengpo);

    General *snow031 = new General(this, "snow031", "yuki", 3);
    snow031->addSkill(new IkLingyun);
    snow031->addSkill(new IkMiyao);

    addMetaObject<IkZhijuCard>();
    addMetaObject<IkJilunCard>();
    addMetaObject<IkKangjinCard>();
    addMetaObject<IkHunkaoCard>();
    addMetaObject<IkPaomuCard>();
    addMetaObject<IkDengpoCard>();
}

ADD_PACKAGE(IkaiKa)