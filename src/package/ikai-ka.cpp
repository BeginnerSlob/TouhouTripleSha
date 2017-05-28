#include "ikai-ka.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "standard-equips.h"
#include "maneuvering.h"
#include "client.h"
#include "touhou-hana.h"
#include "fantasy.h"

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
    room->setEmotion(target, "effects/iron_chain");
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
            room->addPlayerMark(player, "@arrangement");
            room->addPlayerMark(target, "@arrangement");
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
        if (player->getMark("@arrangement") == 0)
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
            while (player->getMark("@arrangement") > 0) {
                if (player->getHandcardNum() >= player->getHp())
                    break;
                room->removePlayerMark(player, "@arrangement");
                player->drawCards(1, "ikyingqi");
                if (current->isDead())
                    break;
            }
        }
        room->setPlayerMark(player, "@arrangement", 0);
        return QStringList();
    }
};

IkJilunCard::IkJilunCard() {
}

bool IkJilunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->canDiscard(to_select, "e") && Self->inMyAttackRange(to_select);
}

void IkJilunCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (effect.from->canDiscard(effect.to, "e")) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "e", "ikjilun", false, MethodDiscard);
        room->throwCard(card_id, effect.to, effect.from);
        Slash *slash = new Slash(NoSuit, 0);
        slash->setSkillName("_ikjilun");
        room->setPlayerFlag(effect.to, "ikjilun");
        if (effect.from->canSlash(effect.to, slash) && !effect.from->isCardLimited(slash, Card::MethodUse))
            room->useCard(CardUseStruct(slash, effect.from, effect.to), false);
        else
            delete slash;
        if (effect.to->hasFlag("ikjilun") && room->getCardPlace(card_id) == Player::DiscardPile) {
            room->sendCompulsoryTriggerLog(effect.from, "ikjilun");
            room->obtainCard(effect.to, card_id);
        }
    }
}

class IkJilunVS : public OneCardViewAsSkill
{
public:
    IkJilunVS() : OneCardViewAsSkill("ikjilun")
    {
        filter_pattern = ".|black|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IkJilunCard *card = new IkJilunCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkJilunCard") && player->canDiscard(player, "h");
    }
};

class IkJilun : public TriggerSkill
{
public:
    IkJilun() : TriggerSkill("ikjilun")
    {
        view_as_skill = new IkJilunVS;
        events << PreDamageDone;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (player->hasFlag(objectName())) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName())
                room->setPlayerFlag(player, "-ikjilun");
        }
        return QStringList();
    }
};

class IkJiqiao: public TriggerSkill {
public:
    IkJiqiao(): TriggerSkill("ikjiqiao") {
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Draw;
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

IkKangjinCard::IkKangjinCard()
{
    will_throw = false;
}

bool IkKangjinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select->getHp() <= Self->getHp())
        return false;
    Card *card = Sanguosha->cloneCard("duel");
    card->addSubcards(subcards);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card);
}

const Card *IkKangjinCard::validate(CardUseStruct &) const
{
    Card *card = Sanguosha->cloneCard("duel");
    card->addSubcards(subcards);
    card->setSkillName("ikkangjin");
    return card;
}

class IkKangjin: public OneCardViewAsSkill
{
public:
    IkKangjin(): OneCardViewAsSkill("ikkangjin")
    {
        response_or_use = true;
        filter_pattern = "BasicCard";
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        IkKangjinCard *card = new IkKangjinCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class IkYunjue: public PhaseChangeSkill
{
public:
    IkYunjue(): PhaseChangeSkill("ikyunjue")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Finish;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        while (true) {
            if (!player->isKongcheng())
                room->showAllCards(player);
            bool has_slash = false;
            foreach (const Card *c, player->getHandcards()) {
                if (c->isKindOf("Slash") || c->isKindOf("Peach")) {
                    has_slash = true;
                    break;
                }
            }
            if (has_slash) {
                room->loseHp(player);
                break;
            }
            player->drawCards(2, objectName());
        }
        return false;
    }
};

#include "roomscene.h"
SelectSuitDialog *SelectSuitDialog::getInstance()
{
    static SelectSuitDialog *instance;
    if (instance == NULL)
        instance = new SelectSuitDialog();

    return instance;
}

SelectSuitDialog::SelectSuitDialog()
{
    setObjectName("ikhunkao");
    setWindowTitle(tr("Please choose a suit"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectSuit(QAbstractButton *)));
}

void SelectSuitDialog::popup()
{
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

void SelectSuitDialog::selectSuit(QAbstractButton *button)
{
    emit onButtonClick();
    RoomSceneInstance->getDashboard()->selectCards(".|" + button->objectName());
    accept();
}

IkHunkaoCard::IkHunkaoCard()
{
}

bool IkHunkaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < subcardsLength() && targets.length() < 2 && Self->inMyAttackRange(to_select);
}

void IkHunkaoCard::onUse(Room *room, const CardUseStruct &use) const
{
    room->showAllCards(use.from);
    SkillCard::onUse(room, use);
}

void IkHunkaoCard::onEffect(const CardEffectStruct &effect) const
{
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
            room->useCard(CardUseStruct(slash, effect.from, effect.to));
        else
            delete slash;
    }
}

class IkHunkao: public ViewAsSkill
{
public:
    IkHunkao(): ViewAsSkill("ikhunkao")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
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

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "h") && player->usedTimes("IkHunkaoCard") < 2;
    }

    virtual QDialog *getDialog() const
    {
        return SelectSuitDialog::getInstance();
    }
};

IkHualanCard::IkHualanCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = MethodNone;
}

void IkHualanCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &) const
{
    room->moveCardTo(this, NULL, Player::DrawPile, false);
}

class IkHualanVS : public ViewAsSkill
{
public:
    IkHualanVS() : ViewAsSkill("ikhualan")
    {
        response_pattern = "@@ikhualan";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.size() < 3 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        IkHualanCard *card = new IkHualanCard;
        card->addSubcards(cards);
        return card;
    }
};

class IkHualan: public TriggerSkill {
public:
    IkHualan(): TriggerSkill("ikhualan") {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new IkHualanVS;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            if (!player->tag["IkHualanRecord"].toList().isEmpty())
                player->tag.remove("IkHualanRecord");
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        } else if (player->getPhase() == Player::RoundStart && !player->isKongcheng()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->isKongcheng())
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        return room->askForUseCard(ask_who, "@@ikhualan", "@ikhualan", -1, Card::MethodNone);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        room->addPlayerMark(ask_who, objectName());
        return false;
    }
};

class IkHualanDraw: public TriggerSkill {
public:
    IkHualanDraw(): TriggerSkill("#ikhualan-draw") {
        events << CardsMoveOneTime << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        TriggerList skill_list;
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from->getPhase() == Player::Discard
                && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                QVariantList list = player->tag["IkHualanRecord"].toList();
                int index = 0;
                foreach (int id, move.card_ids) {
                    if (!list.contains(id)) {
                        if (move.from_places[index] == Player::PlaceHand || move.from_places[index] == Player::PlaceEquip)
                            list << id;
                    }
                    ++index;
                }
                player->tag["IkHualanRecord"] = QVariant::fromValue(list);
            }
        } else if (player->getPhase() == Player::Discard) {
            QVariantList list = player->tag["IkHualanRecord"].toList();
            if (!list.isEmpty()) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    int n = p->getMark("ikhualan");
                    if (n > 0) {
                        QStringList skills;
                        for (int i = 0; i < n; ++i)
                            skills << objectName();
                        skill_list.insert(p, skills);
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        room->sendCompulsoryTriggerLog(ask_who, "ikhualan");
        room->broadcastSkillInvoke("ikhualan");
        QVariantList list = player->tag["IkHualanRecord"].toList();
        ask_who->drawCards(list.length(), "ikhualan");
        return false;
    }
};

class IkTianhua : public TriggerSkill
{
public:
    IkTianhua() : TriggerSkill("iktianhua")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
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

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        if (triggerEvent == CardsMoveOneTime || data.value<PhaseChangeStruct>().to == Player::NotActive) {
            room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
            QList<int> card_ids = room->getNCards(3, false);
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
            if (change.to != Player::Draw)
                room->broadcastSkillInvoke(objectName(), 3);
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
        events << EventPhaseStart << Damaged;
        view_as_skill = new IkHuangshiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play)
            return QStringList(objectName());
        else if (triggerEvent == Damaged) {
            QStringList skills;
            int damage = data.value<DamageStruct>().damage;
            for (int i = 0; i < damage; ++i)
                skills << objectName();
            return skills;
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (triggerEvent == Damaged)
            room->setPlayerFlag(player, "ikhuangshi_damage"); // for Audio
        else
            room->setPlayerFlag(player, "-ikhuangshi_damage");
        room->askForUseCard(player, "@@ikhuangshi", "@ikhuangshi", -1, Card::MethodDiscard);
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int index = qrand() % 2 + 1;
        if (player->hasFlag("ikhuangshi_damage"))
            index += 2;
        return index;
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
            player->drawCards(2, objectName());
            delete dummy;
        }
        return false;
    }
};

IkDongzhaoCard::IkDongzhaoCard()
{
}

void IkDongzhaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->loseHp(source);
    if (source->isAlive())
        SkillCard::use(room, source, targets);
}

void IkDongzhaoCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->setFlags("IkDongzhaoExtra");
    effect.from->tag["IkDongzhao"] = QVariant::fromValue(effect.to);
    effect.to->setFlags("IkDongzhaoUse");
}

class IkDongzhao : public ZeroCardViewAsSkill
{
public:
    IkDongzhao() : ZeroCardViewAsSkill("ikdongzhao")
    {
    }

    virtual const Card *viewAs() const
    {
        return new IkDongzhaoCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkDongzhaoCard");
    }
};

class IkDongzhaoTrigger : public TriggerSkill
{
public:
    IkDongzhaoTrigger() : TriggerSkill("#ikdongzhao")
    {
        events << PreCardUsed << EventPhaseChanging << JinkEffect << NullificationEffect;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (e == EventPhaseChanging) {
            p->setFlags("-IkDongzhaoExtra");
            p->tag.remove("IkDongzhao");
            foreach (ServerPlayer *sp, r->getOtherPlayers(p))
                sp->setFlags("-IkDongzhaoUse");
        } else if (e == JinkEffect || e == NullificationEffect) {
            if (p->hasFlag("IkDongzhaoExtra")) {
                p->setFlags("-IkDongzhaoExtra");
                p->tag.remove("IkDongzhao");
            }
            if (p->hasFlag("IkDongzhaoUse")) {
                if (r->getCurrent())
                    r->sendCompulsoryTriggerLog(r->getCurrent(), "ikdongzhao");
                p->setFlags("-IkDongzhaoUse");
                return QStringList(objectName());
            }
        } else {
            CardUseStruct use = d.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill) {
                if (p->hasFlag("IkDongzhaoExtra") && (use.card->isNDTrick() || use.card->getTypeId() == Card::TypeBasic)) {
                    r->sendCompulsoryTriggerLog(p, "ikdongzhao");
                    p->setFlags("-IkDongzhaoExtra");
                    ServerPlayer *extra = p->tag["IkDongzhao"].value<ServerPlayer *>();
                    p->tag.remove("IkDongzhao");
                    if (extra && extra->isAlive() && !use.to.contains(extra)) {
                        use.to << extra;
                        if (use.card->isKindOf("Collateral")) {
                            QList<ServerPlayer *> victims;
                            foreach (ServerPlayer *p, r->getOtherPlayers(extra)) {
                                if (extra->canSlash(p))
                                    victims << p;
                            }
                            if (!victims.isEmpty()) {
                                ServerPlayer *victim = r->askForPlayerChosen(p, victims, "ikdongzhao", "@dummy-slash2:" + extra->objectName());
                                extra->tag["collateralVictim"] = QVariant::fromValue(victim);

                                LogMessage log;
                                log.type = "#CollateralSlash";
                                log.from = p;
                                log.to << victim;
                                r->sendLog(log);
                                r->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                            }
                        } else if (use.card->isKindOf("FeintAttack")) {
                            QList<ServerPlayer *> victims = r->getOtherPlayers(extra);
                            victims.removeOne(use.from);
                            if (!victims.isEmpty()) {
                                ServerPlayer *victim = r->askForPlayerChosen(p, victims, "ikdongzhao", "@feint-attack:" + extra->objectName());
                                extra->tag["feintTarget"] = QVariant::fromValue(victim);

                                LogMessage log;
                                log.type = "#FeintAttackWest";
                                log.from = p;
                                log.to << victim;
                                r->sendLog(log);
                                r->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                            }
                        }
                        r->sortByActionOrder(use.to);
                        d = QVariant::fromValue(use);
                    }
                }
                if (p->hasFlag("IkDongzhaoUse")) {
                    if (r->getCurrent())
                        r->sendCompulsoryTriggerLog(r->getCurrent(), "ikdongzhao");
                    p->setFlags("-IkDongzhaoUse");
                    use.nullified_list << "_ALL_TARGETS";
                    d = QVariant::fromValue(use);
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return true;
    }
};

class IkElu : public TriggerSkill
{
public:
    IkElu() : TriggerSkill("ikelu")
    {
        events << PreCardUsed << PreDamageDone << EventPhaseStart << CardFinished << ChoiceMade << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == PreCardUsed && player->hasFlag("IkEluLog") && data.canConvert<CardUseStruct>()) {
            room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            player->setFlags("-IkEluLog");
        } else if (triggerEvent == PreCardUsed) {
            if (!player->hasFlag("IkEluUsed"))
                return skill_list;

            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                player->setFlags("-IkEluUsed");
                room->setCardFlag(use.card, "ikelu_slash");
            }
        } else if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag("ikelu_slash")) {
                room->setCardFlag(damage.card, "-ikelu_slash");
                room->setPlayerCardLimitation(player, "use", "Slash,Duel", true);
            }
        } else if (triggerEvent == CardFinished && !player->hasFlag("Global_ProcessBroken")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->hasFlag("ikelu_slash")) {
                room->setCardFlag(use.card, "-ikelu_slash");
                ServerPlayer *current = room->getCurrent();
                if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
                    room->broadcastSkillInvoke(objectName(), 3);
                    room->addPlayerMark(current, "ikelu_" + player->objectName());
                    room->addPlayerMark(current, objectName());
                    room->insertAttackRangePair(current, player);
                }
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (player == owner)
                    continue;
                if (owner->canSlash(player, false))
                    skill_list.insert(owner, QStringList(objectName()));
            }
        } else if (triggerEvent == EventPhaseChanging && data.value<PhaseChangeStruct>().to == Player::NotActive) {
            if (player->getMark(objectName()) > 0) {
                room->setPlayerMark(player, objectName(), 0);
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    while (player->getMark("ikelu_" + p->objectName()) > 0) {
                        room->removePlayerMark(player, "ikelu_" + p->objectName());
                        room->removeAttackRangePair(player, p);
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        ask_who->setFlags("IkEluUsed");
        ask_who->setFlags("IkEluLog");
        bool invoke = room->askForUseSlashTo(ask_who, player, "@ikelu:" + player->objectName(), false);
        if (!invoke) {
            ask_who->setFlags("-IkEluUsed");
            ask_who->setFlags("-IkEluLog");
        }
        return false;
    }
};

class IkYinchou : public TriggerSkill
{
public:
    IkYinchou() : TriggerSkill("ikyinchou")
    {
        events << CardFinished << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, r->getAlivePlayers()) {
                if (p->getMark(objectName()) > 0)
                    p->setMark(objectName(), 0);
            }
        }
        if (TriggerSkill::triggerable(p) && !p->isKongcheng() && p->getMark(objectName()) < 3) {
            CardUseStruct use = d.value<CardUseStruct>();
            if (use.card && use.card->getTypeId() == Card::TypeBasic) {
                QStringList names;
                foreach (const Card *c, p->getHandcards()) {
                    QString name = c->objectName();
                    if (name.contains("slash"))
                        name = "slash";
                    if (!names.contains(name))
                        names << name;
                    else
                        return QStringList();
                }
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        if (p->askForSkillInvoke(objectName())) {
            r->broadcastSkillInvoke(objectName());
            p->addMark(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        if (!p->isKongcheng())
            r->showAllCards(p);
        p->drawCards(1, objectName());
        return false;
    }
};

class IkFengxing : public DrawCardsSkill
{
public:
    IkFengxing() : DrawCardsSkill("ikfengxing")
    {
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(p) && d.toInt() > 0)
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

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        player->setFlags("ikfengxing");
        -- n;
        return n;
    }
};

class IkFengxingShow : public TriggerSkill
{
public:
    IkFengxingShow() : TriggerSkill("#ikfengxing")
    {
        events << AfterDrawNCards;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *p, QVariant &, ServerPlayer *&) const
    {
        if (p->hasFlag("ikfengxing"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke("ikfengxing", "show")) {
            room->broadcastSkillInvoke("ikfengxing");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        forever {
            room->addPlayerMark(player, "ikfengxing");
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
            player->drawCards(1, "ikfengxing");
            if (!player->askForSkillInvoke("ikfengxing", "show"))
                break;
        }

        return true;
    }
};

class IkFengxingDistance : public DistanceSkill
{
public:
    IkFengxingDistance() : DistanceSkill("#ikfengxing-distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        return qMax(from->getMark("ikfengxing") - 2, 0);
    }
};

class IkFengxingClear : public TriggerSkill
{
public:
    IkFengxingClear() : TriggerSkill("#ikfengxing-clear")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (data.value<PhaseChangeStruct>().to == Player::NotActive)
            room->setPlayerMark(player, "ikfengxing", 0);
        return QStringList();
    }
};

class IkQizhong: public TriggerSkill
{
public:
    IkQizhong(): TriggerSkill("ikqizhong")
    {
        events << CardUsed << EventPhaseChanging << PreCardUsed << CardResponded << PreCardResponded;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseChanging)
            player->tag.remove("IkQizhongCard");
        else if (triggerEvent == PreCardUsed) {
            const Card *this_card = data.value<CardUseStruct>().card;
            if (this_card && this_card->getTypeId() != Card::TypeSkill) {
                const Card *last_card = player->tag["IkQizhongCard"].value<const Card *>();
                if (last_card && !this_card->sameColorWith(last_card))
                    room->setCardFlag(this_card, "IkQizhongInvoke");
                player->tag["IkQizhongCard"] = QVariant::fromValue(this_card);
            }
        } else if (triggerEvent == PreCardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                const Card *this_card = resp.m_card;
                if (this_card && this_card->getTypeId() != Card::TypeSkill) {
                    const Card *last_card = player->tag["IkQizhongCard"].value<const Card *>();
                    if (last_card && !this_card->sameColorWith(last_card))
                        room->setCardFlag(this_card, "IkQizhongInvoke");
                    player->tag["IkQizhongCard"] = QVariant::fromValue(this_card);
                }
            }
        } else if (triggerEvent == CardUsed && player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->hasFlag("IkQizhongInvoke"))
                return QStringList(objectName());
        } else if (triggerEvent == CardResponded && player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                if (resp.m_card->hasFlag("IkQizhongInvoke"))
                    return QStringList(objectName());
            }
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

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        const Card *use_card = NULL;
        if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse)
                use_card = resp.m_card;
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            use_card = use.card;
        }
        Q_ASSERT(use_card);
        if (!use_card)
            return false;
        QList<int> ids = room->getNCards(1, false);
        CardsMoveStruct move(ids, player, Player::PlaceTable,
                             CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        int id = ids.first();
        const Card *card = Sanguosha->getCard(id);
        if (card->getColor() != use_card->getColor()) {
            CardMoveReason reason(CardMoveReason::S_REASON_DRAW, player->objectName(), objectName(), QString());
            room->obtainCard(player, card, reason);
        } else {
            const Card *card_ex = NULL;
            if (!player->isNude())
                card_ex = room->askForCard(player, QString("^%1|.|.|hand").arg(use_card->getEffectiveId()),
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

IkJimuCard::IkJimuCard() {
    mute = true;
}

void IkJimuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->broadcastSkillInvoke("ikjimu", 1);
    room->removePlayerMark(effect.from, "@jimu");
    room->addPlayerMark(effect.from, "@jimuused");
    effect.to->gainMark("@speed");
    room->setFixedDistance(effect.from, effect.to, 1);
}

class IkJimuViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkJimuViewAsSkill(): ZeroCardViewAsSkill("ikjimu") {
    }

    virtual const Card *viewAs() const{
        return new IkJimuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@jimu") > 0;
    }
};

class IkJimu: public TriggerSkill {
public:
    IkJimu(): TriggerSkill("ikjimu") {
        events << CardFinished << Death;
        view_as_skill = new IkJimuViewAsSkill;
        frequency = Limited;
        limit_mark = "@jimu";
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (triggerEvent == CardFinished && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card || !use.card->isBlack() || !use.card->isKindOf("Slash"))
                return QStringList();
            foreach (ServerPlayer *to, use.to)
                if (to->getMark("@speed") > 0)
                    return QStringList(objectName());
        } else if (triggerEvent == Death && player->getMark("@speed") > 0) {
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

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikjimu", false, true);
        if (target) {
            if (triggerEvent == CardFinished)
                room->broadcastSkillInvoke(objectName(), qrand() % 2 + 4);
            else
                room->broadcastSkillInvoke(objectName(), 6);
            player->tag["IkJimuTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        ServerPlayer *target = player->tag["IkJimuTarget"].value<ServerPlayer *>();
        player->tag.remove("IkJimuTarget");
        if (triggerEvent == CardFinished && target) {
            ServerPlayer *victim = NULL;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@speed") > 0) {
                    victim = p;
                    break;
                }
            }
            bool use = false;
            if (target->canSlash(victim, false)) {
                QString prompt = QString("@ikjimu-slash:%1:%2").arg(player->objectName()).arg(victim->objectName());
                use = room->askForUseSlashTo(target, victim, prompt, false);
            }
            if (!use)
                player->drawCards(1, objectName());
        } else if (triggerEvent == Death && target) {
            target->gainMark("@speed");
            room->setFixedDistance(player, target, 1);
        }
        return false;
    }
};

class IkJimuEffect: public TriggerSkill {
public:
    IkJimuEffect(): TriggerSkill("#ikjimu-effect") {
        events << PreCardUsed;
        global = true;
        frequency = Compulsory;
    }

    virtual int getPriority(TriggerEvent) const{
        return 6;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || use.card->isKindOf("Snatch") || use.card->isKindOf("SupplyShortage")) {
            if (use.from->hasSkill("ikjimu") && use.to.length() == 1 && use.to.first()->getMark("@speed") > 0)
                room->broadcastSkillInvoke("ikjimu", qrand() % 2 + 2);
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
        if (!player->isKongcheng()) {
            int card_id = room->askForCardChosen(ask_who, player, "h", objectName());
            room->obtainCard(ask_who, card_id, false);
        }
        return false;
    }
};

class IkZuiyan: public TriggerSkill {
public:
    IkZuiyan(): TriggerSkill("ikzuiyan") {
        events << EventPhaseStart;
        owner_only_skill = true;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Play;
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
                room->useCard(CardUseStruct(anal, p, p), true);
        }
        return false;
    }
};

class IkZuiyanSlash: public TriggerSkill {
public:
    IkZuiyanSlash(): TriggerSkill("#ikzuiyan") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
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

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
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

IkLingchaDialog *IkLingchaDialog::getInstance() {
    static IkLingchaDialog *instance;
    if (instance == NULL)
        instance = new IkLingchaDialog();

    return instance;
}

IkLingchaDialog::IkLingchaDialog() {
    setObjectName("iklingcha");
    setWindowTitle(Sanguosha->translate("iklingcha"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void IkLingchaDialog::popup() {
    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QStringList card_names;
    card_names << "slash" << "ex_nihilo";
    if (Self->getMark("jnyanwang") > 0)
        card_names << "dismantlement";

    foreach (QString card, card_names) {
        QCommandLinkButton *button = new QCommandLinkButton;
        button->setText(Sanguosha->translate(card));
        button->setObjectName(card);
        group->addButton(button);

        bool can = true;
        if (can) {
            const Card *c = Sanguosha->cloneCard(card);
            if (Self->isCardLimited(c, Card::MethodUse) || !c->isAvailable(Self))
                can = false;
            delete c;
        }
        button->setEnabled(can);
        button_layout->addWidget(button);

        if (!map.contains(card)) {
            Card *c = Sanguosha->cloneCard(card, Card::NoSuit, 0);
            c->setParent(this);
            map.insert(card, c);
        }
    }

    exec();
}

void IkLingchaDialog::selectCard(QAbstractButton *button) {
    const Card *card = map.value(button->objectName());
    Self->tag["iklingcha"] = QVariant::fromValue(card);
    emit onButtonClick();
    accept();
}

IkLingchaCard::IkLingchaCard() {
    will_throw = false;
}

bool IkLingchaCard::targetFixed() const {
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("iklingcha").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->addSubcards(subcards);
    new_card->setSkillName("iklingcha");
    new_card->deleteLater();
    return new_card->targetFixed();
}

bool IkLingchaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("iklingcha").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->addSubcards(subcards);
    new_card->setSkillName("iklingcha");
    new_card->deleteLater();
    return new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool IkLingchaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->deleteLater();
        }
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("iklingcha").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->addSubcards(subcards);
    new_card->setSkillName("iklingcha");
    new_card->deleteLater();
    return new_card->targetsFeasible(targets, Self);
}

const Card *IkLingchaCard::validate(CardUseStruct &use) const{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("iklingcha");
    use_card->addSubcards(subcards);
    use.from->getRoom()->addPlayerMark(use.from, "iklingcha_count");
    return use_card;
}

const Card *IkLingchaCard::validateInResponse(ServerPlayer *player) const{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("iklingcha");
    use_card->addSubcards(subcards);
    player->getRoom()->addPlayerMark(player, "iklingcha_count");
    return use_card;
}

class IkLingchaVS: public ViewAsSkill {
public:
    IkLingchaVS(): ViewAsSkill("iklingcha") {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const {
        return !to_select->isEquipped();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (player->getPhase() != Player::Play || player->getMark("iklingcha_count") >= getUseTimes(player))
            return false;
        return pattern == "slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        if (player->getPhase() != Player::Play || player->getMark("iklingcha_count") >= getUseTimes(player))
            return false;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        ExNihilo *ex_nihilo = new ExNihilo(Card::NoSuit, 0);
        ex_nihilo->deleteLater();
        Dismantlement *chai = new Dismantlement(Card::NoSuit, 0);
        chai->deleteLater();
        return slash->isAvailable(player) || ex_nihilo->isAvailable(player) || (player->getMark("jnyanwang") > 0 && chai->isAvailable(player));
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (cards.isEmpty())
            return NULL;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            IkLingchaCard *card = new IkLingchaCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcards(cards);
            return card;
        }

        const Card *c = Self->tag.value("iklingcha").value<const Card *>();
        if (c) {
            IkLingchaCard *card = new IkLingchaCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    int getUseTimes(const Player *player) const{
        return player->getMark("jnyanwang") > 0 ? 2 : 1;
    }
};

class IkLingcha: public TriggerSkill {
public:
    IkLingcha(): TriggerSkill("iklingcha") {
        events << EventPhaseChanging;
        view_as_skill = new IkLingchaVS;
    }

    virtual QDialog *getDialog() const {
        return IkLingchaDialog::getInstance();
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        int step = -2;
        if (card->isKindOf("Slash"))
            step = 0;
        else if (card->isKindOf("ExNihilo"))
            step = 2;
        else if (card->isKindOf("Dismantlement"))
            step = 4;
        if (step == -2)
            return step;
        return qrand() % 2 + 1 + step;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        room->setPlayerMark(player, "iklingcha_count", 0);
        return QStringList();
    }
};

class IkLingchaTrigger: public TriggerSkill {
public:
    IkLingchaTrigger(): TriggerSkill("#iklingcha") {
        events << TrickMissed << SlashMissed;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player && player->isAlive()) {
            const Card *card = NULL;
            if (triggerEvent == TrickMissed)
                card = data.value<CardEffectStruct>().card;
            else if (triggerEvent == SlashMissed)
                card = data.value<SlashEffectStruct>().slash;
            if (card && card->getSkillName() == "iklingcha")
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, "iklingcha");
        room->broadcastSkillInvoke("iklingcha", qrand() % 2 + 7);

        const Card *card = NULL;
        if (triggerEvent == TrickMissed)
            card = data.value<CardEffectStruct>().card;
        else if (triggerEvent == SlashMissed)
            card = data.value<SlashEffectStruct>().slash;
        player->drawCards(card->subcardsLength(), "iklingcha");
        return false;
    }
};

class IkMingshi: public TriggerSkill {
public:
    IkMingshi(): TriggerSkill("ikmingshi") {
        events << BeforeCardsMove;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.to_place == Player::DiscardPile) {
                foreach (int id, move.card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (card->isKindOf("Jink") || card->isKindOf("Weapon"))
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids, disabled_ids;
        foreach (int id, move.card_ids) {
            const Card *card = Sanguosha->getCard(id);
            if (card->isKindOf("Jink") || card->isKindOf("Weapon"))
                ids << id;
            else
                disabled_ids << id;
        }
        if (!ids.isEmpty()) {
            room->fillAG(move.card_ids, NULL, disabled_ids);
            do {
                int id = room->askForAG(player, ids, true, objectName());
                if (id == -1)
                    break;
                ids.removeOne(id);
                ServerPlayer *target = room->askForPlayerChosen(player,
                                                                room->getOtherPlayers(player),
                                                                objectName(),
                                                                "@ikmingshi",
                                                                true, 
                                                                true);
                if (target) {
                    QList<int> _id;
                    _id << id;
                    move.removeCardIds(_id);
                    room->takeAG(target, id, false);
                    room->obtainCard(target, id);
                } else {
                    room->takeAG(NULL, id, false);
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);
                    player->drawCards(1, objectName());
                }
            } while (!ids.isEmpty());
            room->clearAG();
            data = QVariant::fromValue(move);
        }
        return false;   
    }
};

IkDuanniCard::IkDuanniCard()
{
}

bool IkDuanniCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isAllNude() && to_select->inMyAttackRange(Self);
}

void IkDuanniCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "hej", "ikduanni");
    Slash *slash = new Slash(SuitToBeDecided, -1);
    slash->addSubcard(card_id);
    slash->setSkillName("_ikduanni");
    QList<ServerPlayer *> extras;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
        if (p == effect.from)
            continue;
        if (effect.to->canSlash(p, slash, false))
            extras << p;
    }
    QList<ServerPlayer *> targets;
    targets << effect.from;
    if (!extras.isEmpty()) {
        ServerPlayer *extra = room->askForPlayerChosen(effect.from, extras, "ikduanni", "@slash_extra_targets", true);
        targets << extra;
    }
    room->sortByActionOrder(targets);
    room->useCard(CardUseStruct(slash, effect.to, targets));
}

class IkDuanni : public ZeroCardViewAsSkill
{
public:
    IkDuanni() : ZeroCardViewAsSkill("ikduanni")
    {
    }

    virtual const Card *viewAs() const
    {
        return new IkDuanniCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("IkDuanniCard");
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return -2;
        return -1;
    }
};

class IkMeitong : public TriggerSkill
{
public:
    IkMeitong() : TriggerSkill("ikmeitong")
    {
        events << TargetConfirming;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *p, QVariant &d, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(p)) {
            CardUseStruct use = d.value<CardUseStruct>();
            if (use.from && !use.from->isKongcheng() && use.to.contains(p) && use.card->isKindOf("Slash"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(use.from))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        LogMessage log;
        log.type = "$IkLingtongView";
        log.from = player;
        log.to << use.from;
        log.arg = "iklingtong:handcards";
        room->sendLog(log, room->getOtherPlayers(player));

        LogMessage log2;
        log2.type = "$ViewAllCards";
        log2.from = player;
        log2.to << use.from;
        log2.card_str = IntList2StringList(use.from->handCards()).join("+");
        room->sendLog(log2, player);

        int id = room->askForCardChosen(player, use.from, "h", objectName(), true, Card::MethodDiscard);
        room->throwCard(id, use.from, player);
        use.from->drawCards(1, objectName());
        return false;
    }
};

class IkSheji: public TriggerSkill
{
public:
    IkSheji(): TriggerSkill("iksheji")
    {
        events << EventPhaseStart << PreCardUsed << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (use.to.contains(p))
                        continue;
                    if (use.from->isProhibited(p, use.card))
                        continue;
                    if (use.from->getMark("iksheji_" + p->objectName()) > 0) {
                        use.to << p;
                        if (use.card->isKindOf("Collateral")) {
                            QList<ServerPlayer *> victims;
                            foreach (ServerPlayer *p2, room->getOtherPlayers(p)) {
                                if (p->canSlash(p2))
                                    victims << p2;
                            }
                            if (!victims.isEmpty()) {
                                ServerPlayer *collateral_victim = room->askForPlayerChosen(use.from, victims, "iksheji", "@iksheji-collateral:" + p->objectName());
                                p->tag["collateralVictim"] = QVariant::fromValue(collateral_victim);

                                LogMessage log;
                                log.type = "#CollateralSlash";
                                log.from = player;
                                log.to << collateral_victim;
                                room->sendLog(log);
                                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), collateral_victim->objectName());
                            } else {
                                p->tag.remove("collateralVictim");

                                LogMessage log;
                                log.type = "#CollateralNoSlash";
                                log.from = p;
                                room->sendLog(log);
                            }
                        } else if (use.card->isKindOf("FeintAttack")) {
                            QList<ServerPlayer *> victims = room->getOtherPlayers(p);
                            victims.removeOne(use.from);
                            if (!victims.isEmpty()) {
                                ServerPlayer *collateral_victim = room->askForPlayerChosen(use.from, victims, "iksheji", "@iksheji-feint-attack:" + p->objectName());
                                p->tag["feintTarget"] = QVariant::fromValue(collateral_victim);

                                LogMessage log;
                                log.type = "#FeintAttackWest";
                                log.from = player;
                                log.to << collateral_victim;
                                room->sendLog(log);
                                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), collateral_victim->objectName());
                            } else {
                                p->tag.remove("feintTarget");

                                LogMessage log;
                                log.type = "#FeintAttackNoWest";
                                log.from = p;
                                room->sendLog(log);
                            }
                        }
                    }
                }
                room->sortByActionOrder(use.to);
                data = QVariant::fromValue(use);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getPlayers()) {
                    if (p->getMark("@sheji") > 0) {
                        room->setPlayerMark(p, "@sheji", 0);
                        player->setMark("iksheji_" + p->objectName(), 0);
                    }
                }
            }
        } else if (player && player->isAlive() && player->getPhase() == Player::Play && !player->isKongcheng()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != player && !p->isKongcheng())
                    list.insert(p, QStringList(objectName()));
            }
        }
        return list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        bool success = ask_who->pindian(player, objectName());
        if (success) {
            ServerPlayer *target = room->askForPlayerChosen(ask_who, room->getAlivePlayers(), objectName());
            player->addMark("iksheji_" + target->objectName());
            room->addPlayerMark(target, "@sheji");
        }
        return false;
    }
};

class IkPingwei : public TriggerSkill
{
public:
    IkPingwei() : TriggerSkill("ikpingwei")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList list;
        if (player && player->isAlive() && player->getPhase() == Player::Finish) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p == player)
                    continue;
                if (player->tag["IkPingwei"].toStringList().toSet().size() > p->getHp())
                    list.insert(p, QStringList(objectName()));
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        ask_who->drawCards(1, objectName());
        return false;
    }
};

class IkPingweiRecord : public TriggerSkill
{
public:
    IkPingweiRecord() : TriggerSkill("#ikpingwei")
    {
        events << CardUsed << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive)
                player->tag.remove("IkPingwei");
        } else if (player && player->isAlive() && player == room->getCurrent() && player->getPhase() != Player::NotActive) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.to.isEmpty() && use.card->getTypeId() != Card::TypeSkill)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QStringList targets = player->tag["IkPingwei"].toStringList();
        foreach (ServerPlayer *p, data.value<CardUseStruct>().to)
            targets << p->objectName();
        player->tag["IkPingwei"] = QVariant::fromValue(targets);
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

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
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

IkLixinCard::IkLixinCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkLixinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->setSkillName("iklixin");
        }
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *card = Sanguosha->cloneCard(Self->tag.value("iklixin").value<const Card *>()->objectName());
    card->addSubcards(subcards);
    card->setSkillName("iklixin");
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool IkLixinCard::targetFixed() const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("iklixin").value<const Card *>();
    return card && card->targetFixed();
}

bool IkLixinCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcards(subcards);
            card->setSkillName("iklixin");
        }
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *card = Sanguosha->cloneCard(Self->tag.value("iklixin").value<const Card *>()->objectName());
    card->addSubcards(subcards);
    card->setSkillName("iklixin");
    return card && card->targetsFeasible(targets, Self);
}

const Card *IkLixinCard::validate(CardUseStruct &card_use) const{
    ServerPlayer *wenyang = card_use.from;
    Room *room = wenyang->getRoom();

    QString to_iklixin = user_string;
    if (user_string == "slash"
        && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QStringList iklixin_list;
            iklixin_list << "slash";
            if (!ServerInfo.Extensions.contains("!maneuvering"))
                iklixin_list << "thunder_slash" << "fire_slash";
            to_iklixin = room->askForChoice(wenyang, "iklixin_slash", iklixin_list.join("+"));
    }

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(wenyang)) {
        if (!p->hasFlag("IkLixinUsed"))
            targets << p;
    }
    if (targets.length() < 2)
        room->setPlayerFlag(wenyang, "IkLixinDisabled");
    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(wenyang, targets, "iklixin", "@iklixin");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, wenyang->objectName(), target->objectName(), "iklixin", QString());
        room->obtainCard(target, this, reason);
    } else
        return NULL;

    Card *use_card = Sanguosha->cloneCard(to_iklixin, NoSuit, 0);
    use_card->setSkillName("iklixin");
    return use_card;
}

const Card *IkLixinCard::validateInResponse(ServerPlayer *wenyang) const{
    Room *room = wenyang->getRoom();

    QString to_iklixin;
    if (user_string == "peach+analeptic") {
        QStringList iklixin_list;
        iklixin_list << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            iklixin_list << "analeptic";
        to_iklixin = room->askForChoice(wenyang, "iklixin_saveself", iklixin_list.join("+"));
    } else if (user_string == "slash") {
        QStringList iklixin_list;
        iklixin_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            iklixin_list << "thunder_slash" << "fire_slash";
        to_iklixin = room->askForChoice(wenyang, "iklixin_slash", iklixin_list.join("+"));
    }
    else
        to_iklixin = user_string;

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(wenyang)) {
        if (!p->hasFlag("IkLixinUsed"))
            targets << p;
    }
    if (targets.length() < 2)
        room->setPlayerFlag(wenyang, "IkLixinDisabled");
    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(wenyang, targets, "iklixin", "@iklixin");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, wenyang->objectName(), target->objectName(), "iklixin", QString());
        room->obtainCard(target, this, reason);
    } else
        return NULL;

    Card *use_card = Sanguosha->cloneCard(to_iklixin, NoSuit, 0);
    use_card->setSkillName("iklixin");
    return use_card;
}

class IkLixinVS : public ViewAsSkill
{
public:
    IkLixinVS() : ViewAsSkill("iklixin")
    {
        owner_only_skill = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 2)
            return false;
        if (selected.isEmpty())
            return true;
        if (selected.length() == 1)
            return selected.first()->getTypeId() != to_select->getTypeId();
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getCardCount() < 2)
            return false;
        if (player->hasFlag("IkLixinDisabled"))
            return false;
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
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
        if (pattern.contains("analeptic") || pattern == "slash" || pattern == "jink")
            return true;
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            IkLixinCard *tianyan_card = new IkLixinCard;
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";
            tianyan_card->setUserString(pattern);
            tianyan_card->addSubcards(cards);
            return tianyan_card;
        }

        const Card *c = Self->tag["iklixin"].value<const Card *>();
        if (c) {
            IkLixinCard *card = new IkLixinCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getCardCount() < 2)
            return false;
        if (player->hasFlag("IkLixinDisabled"))
            return false;
        QList<const Card *> cards = player->getHandcards() + player->getEquips();
        Card::CardType type = cards.first()->getTypeId();
        foreach (const Card *cd, cards)
            if (cd->getTypeId() != type)
                return true;
        return false;
    }
};

#include "touhou-hana.h"
class IkLixin : public TriggerSkill
{
public:
    IkLixin() : TriggerSkill("iklixin")
    {
        events << EventPhaseChanging;
        view_as_skill = new IkLixinVS;
    }

    virtual QDialog *getDialog() const
    {
        return ThMimengDialog::getInstance("iklixin", true, false);
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &) const
    {
        if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("IkLixinDisabled"))
                    room->setPlayerFlag(p, "-IkLixinDisabled");
                if (p->hasFlag("IkLixinUsed"))
                    room->setPlayerFlag(p, "-IkLixinUsed");
            }
        }
        return QStringList();
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

IkMingwangCard::IkMingwangCard() {
    handling_method = MethodUse;
}

bool IkMingwangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    LureTiger *lure_tiger = new LureTiger(SuitToBeDecided, 0);
    lure_tiger->addSubcards(subcards);
    lure_tiger->deleteLater();
    return lure_tiger->targetFilter(targets, to_select, Self);
}

const Card *IkMingwangCard::validate(CardUseStruct &) const{
    LureTiger *lure_tiger = new LureTiger(SuitToBeDecided, 0);
    lure_tiger->addSubcards(subcards);
    lure_tiger->setSkillName("ikmingwang");
    return lure_tiger;
}

class IkMingwang: public OneCardViewAsSkill {
public:
    IkMingwang(): OneCardViewAsSkill("ikmingwang") {
        filter_pattern = ".|spade";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkMingwangCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkMingwangCard *card = new IkMingwangCard;
        card->addSubcard(originalCard);
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

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Self->getMark(objectName()) > 0) {
            Card::Color color = (Card::Color)(Self->getMark(objectName()) - 1);
            return selected.length() < 3 && !Self->isJilei(to_select) && to_select->getColor() == color && !to_select->isEquipped();
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

class IkLvyanViewAsSkill: public ViewAsSkill {
public:
    IkLvyanViewAsSkill(): ViewAsSkill("iklvyan") {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() < 2)
            return NULL;

        Slash *card = new Slash(Card::SuitToBeDecided, -1);
        card->addSubcards(cards);
        card->setSkillName(objectName());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getPhase() == Player::Play && Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
        return player->getPhase() == Player::Play && pattern == "slash";
    }
};

class IkLvyan : public TriggerSkill
{
public:
    IkLvyan() : TriggerSkill("iklvyan")
    {
        events << SlashMissed;
        view_as_skill = new IkLvyanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (TriggerSkill::triggerable(player) && effect.slash && effect.slash->getSkillName() == objectName()
            && effect.slash->subcardsLength() > 0)
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        player->drawCards(effect.slash->subcardsLength(), objectName());
        return false;
    }
};

class IkWuming: public TriggerSkill {
public:
    IkWuming(): TriggerSkill("ikwuming") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Finish
            && player->getHandcardNum() != 2
            && !player->isKongcheng();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        int delta = player->getHandcardNum() - 2;
        if (delta < 0) {
            room->broadcastSkillInvoke(objectName(), 1);
            player->drawCards(-delta, objectName());
        } else {
            room->broadcastSkillInvoke(objectName(), 2);
            room->askForDiscard(player, objectName(), delta, delta);
        }
        return false;
    }
};

class IkLianxiao: public TriggerSkill {
public:
    IkLianxiao(): TriggerSkill("iklianxiao") {
        events << EventPhaseEnd;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() != Player::Play)
            return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (owner->getMark(objectName()) == 0 && (owner->inMyAttackRange(player) || owner == player))
                skill_list.insert(owner, QStringList(objectName()));
        }
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
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == CardFinished && player->isAlive() && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill) {
                foreach (ServerPlayer *p, use.to)
                    p->setMark("iklianxiao", 1);
            }
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
        QString choice = Card::Suit2String(room->askForSuit(damage.from, objectName()));
        LogMessage log;
        log.type = "#IkSaoxiaoChoice";
        log.from = damage.from;
        log.arg = choice;
        room->sendLog(log);
        const Card *hand = room->askForCard(player, ".|^" + choice, "@iksaoxiao", data, Card::MethodNone);
        if (!hand)
            return false;
        room->showCard(player, hand->getEffectiveId());

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
        return false;
    }
};

IkXiaowuCard::IkXiaowuCard()
{
    target_fixed = true;
    will_throw = false;
}

const Card *IkXiaowuCard::validate(CardUseStruct &card_use) const
{
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

class IkXiaowuViewAsSkill: public OneCardViewAsSkill
{
public:
    IkXiaowuViewAsSkill(): OneCardViewAsSkill("ikxiaowu")
    {
        filter_pattern = ".|diamond#^TrickCard|black";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
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

    virtual const Card *viewAs(const Card *originalCard) const
    {
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

class IkXiaowu : public TriggerSkill
{
public:
    IkXiaowu() : TriggerSkill("ikxiaowu")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new IkXiaowuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
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

class IkXiaowuSlash: public TriggerSkill
{
public:
    IkXiaowuSlash(): TriggerSkill("#ikxiaowu")
    {
        events << CardFinished << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified) {
            if (player && player->isAlive() && use.card->isKindOf("Slash") && use.card->getSkillName(false) == "_ikxiaowu")
                return QStringList(objectName());
        } else if (use.card->getSkillName(false) == "ikxiaowu")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetSpecified) {
            room->sendCompulsoryTriggerLog(player, "ikxiaowu");
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to.toSet())
                p->addQinggangTag(use.card);
            return false;
        }
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
            room->useCard(use, false);
        } else
            delete slash;
        return false;
    }
};

class IkWanmiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkWanmiViewAsSkill(): ZeroCardViewAsSkill("ikwanmi") {
        response_pattern = "@@ikwanmi";
    }

    virtual const Card *viewAs() const {
        const Card *card = Card::Parse(Self->property("ikwanmi_card").toString());
        Card *use_card = Sanguosha->cloneCard(card->objectName());
        use_card->addSubcards(card->getSubcards());
        use_card->setSkillName(objectName());
        return use_card;
    }
};

class IkWanmi: public TriggerSkill {
public:
    IkWanmi(): TriggerSkill("ikwanmi") {
        events << CardsMoveOneTime << EventPhaseChanging << PreCardUsed;
        view_as_skill = new IkWanmiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == PreCardUsed) {
            if (player->hasFlag("IkWanmiUsed")) {
                player->setFlags("-IkWanmiUsed");
                player->addMark(objectName());
            }
            return QStringList();
        }
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setMark(objectName(), 0);
            }
            return QStringList();
        }
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (room->getTag("FirstRound").toBool() || player->getMark(objectName()) >= 2)
            return QStringList();
        if (move.to == player && (move.to_place == Player::PlaceHand
                                  || move.to_place == Player::PlaceEquip
                                  || move.to_place == Player::PlaceDelayedTrick)) {
            QList<int> ids;
            foreach (int id, move.card_ids) {
                if (room->getCardOwner(id) == player && room->getCardPlace(id) == move.to_place)
                    ids << id;
            }
            if (ids.isEmpty())
                return QStringList();
            // -- Slash
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcards(ids);
            slash->setSkillName(objectName());
            slash->deleteLater();
            if (slash->isAvailable(player))
                return QStringList(objectName());
            // -- Jink
            // nothing
            // -- Analeptic
            Analeptic *anal = new Analeptic(Card::SuitToBeDecided, -1);
            anal->addSubcards(ids);
            anal->setSkillName(objectName());
            anal->deleteLater();
            if (anal->isAvailable(player))
                return QStringList(objectName());
            // -- Peach
            Peach *peach = new Peach(Card::SuitToBeDecided, -1);
            peach->addSubcards(ids);
            peach->setSkillName(objectName());
            peach->deleteLater();
            if (peach->isAvailable(player))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QString place_str = "";
        if (move.to_place == Player::PlaceHand)
            place_str = "ikwanmi_h";
        else if (move.to_place == Player::PlaceEquip)
            place_str = "ikwanmi_e";
        else if (move.to_place == Player::PlaceDelayedTrick)
            place_str = "ikwanmi_j";
        if (player->askForSkillInvoke("ikwanmi_invoke", "invoke:::" + place_str))
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (room->getCardOwner(id) == player && room->getCardPlace(id) == move.to_place)
                ids << id;
        }
        if (ids.isEmpty())
            return false;
        QStringList choices;
        // -- Slash
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        slash->addSubcards(ids);
        slash->setSkillName(objectName());
        slash->deleteLater();
        if (slash->isAvailable(player)) {
            choices << "slash";
            if (!ServerInfo.Extensions.contains("!maneuvering"))
                choices << "thunder_slash" << "fire_slash";
        }
        // -- Jink
        // nothing
        // -- Analeptic
        Analeptic *anal = new Analeptic(Card::SuitToBeDecided, -1);
        anal->addSubcards(ids);
        anal->setSkillName(objectName());
        anal->deleteLater();
        if (anal->isAvailable(player))
            choices << "analeptic";
        // -- Peach
        Peach *peach = new Peach(Card::SuitToBeDecided, -1);
        peach->addSubcards(ids);
        peach->setSkillName(objectName());
        peach->deleteLater();
        if (peach->isAvailable(player))
            choices << "peach";
        Q_ASSERT(!choices.isEmpty());
        QString card_name = room->askForChoice(player, objectName(), choices.join("+"));
        Card *use_card = Sanguosha->cloneCard(card_name);
        use_card->addSubcards(ids);
        use_card->setSkillName(objectName());
        room->setPlayerProperty(player, "ikwanmi_card", use_card->toString());
        player->setFlags("IkWanmiUsed");
        if (!room->askForUseCard(player, "@@ikwanmi", "@ikwanmi", -1, Card::MethodUse, false))
            player->setFlags("-IkWanmiUsed");
        delete use_card;
        room->setPlayerProperty(player, "ikwanmi_card", QVariant());
        return false;
    }
};

class IkWanmiTargetMod: public TargetModSkill {
public:
    IkWanmiTargetMod(): TargetModSkill("#ikwanmi-tar") {
        pattern = "BasicCard";
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const{
        if (card->getSkillName() == "ikwanmi")
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *, const Card *card) const{
        if (card->getSkillName() == "ikwanmi")
            return 1000;
        else
            return 0;
    }
};

class IkGuichan: public TriggerSkill {
public:
    IkGuichan(): TriggerSkill("ikguichan") {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player && player->isDead() && player->hasSkill(objectName())) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player)
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        Card::Suit suit = room->askForSuit(player, objectName());
        QString suit_str = Card::Suit2String(suit);
        LogMessage log;
        log.type = "#ChooseSuit";
        log.from = player;
        log.arg = suit_str;
        room->sendLog(log);
        foreach (ServerPlayer *p, room->getAllPlayers())
            room->setPlayerCardLimitation(p, "use,response", ".|" + suit_str, true);
        return false;
    }
};

IkHuanlveCard::IkHuanlveCard() {
    will_throw = false;
}

bool IkHuanlveCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *card = Self->tag.value("ikhuanlve").value<const Card *>();
    Card *mutable_card = const_cast<Card *>(card);
    if (mutable_card)
        mutable_card->addSubcards(this->subcards);
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool IkHuanlveCard::targetFixed() const{
    const Card *card = Self->tag.value("ikhuanlve").value<const Card *>();
    Card *mutable_card = const_cast<Card *>(card);
    if (mutable_card)
        mutable_card->addSubcards(this->subcards);
    return mutable_card && mutable_card->targetFixed();
}

bool IkHuanlveCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    const Card *card = Self->tag.value("ikhuanlve").value<const Card *>();
    Card *mutable_card = const_cast<Card *>(card);
    if (mutable_card)
        mutable_card->addSubcards(this->subcards);
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *IkHuanlveCard::validate(CardUseStruct &card_use) const{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("ikhuanlve");
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
    bool all_tricks = true;
    foreach (int id, subcards) {
        if (Sanguosha->getCard(id)->getTypeId() != Card::TypeTrick) {
            all_tricks = false;
            break;
        }
    }
    Room *room = card_use.from->getRoom();
    if (all_tricks)
        card_use.from->gainMark("@thought", 1);
    room->addPlayerMark(card_use.from, "IkHuanlveTimes");
    return use_card;
}

const Card *IkHuanlveCard::validateInResponse(ServerPlayer *user) const{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("ikhuanlve");
    use_card->addSubcards(this->subcards);
    use_card->deleteLater();

    bool all_tricks = true;
    foreach (int id, subcards) {
        if (Sanguosha->getCard(id)->getTypeId() != Card::TypeTrick) {
            all_tricks = false;
            break;
        }
    }
    Room *room = user->getRoom();
    if (all_tricks)
        user->gainMark("@thought", 1);
    room->addPlayerMark(user, "IkHuanlveTimes");
    return use_card;
}

class IkHuanlve: public ViewAsSkill {
public:
    IkHuanlve(): ViewAsSkill("ikhuanlve") {
        response_or_use = true;
    }

    virtual QDialog *getDialog() const{
        return ThMimengDialog::getInstance("ikhuanlve", false);
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() >= 2)
            return false;
        if (selected.isEmpty())
            return to_select->isKindOf("TrickCard");
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;
        bool trick = false;
        foreach (const Card *card, cards) {
            if (card->isKindOf("TrickCard")) {
                trick = true;
                break;
            }
        }
        if (!trick)
            return NULL;

        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
            IkHuanlveCard *card = new IkHuanlveCard;
            card->setUserString(Sanguosha->getCurrentCardUsePattern());
            card->addSubcards(cards);
            return card;
        }

        const Card *c = Self->tag.value("ikhuanlve").value<const Card *>();
        if (c) {
            IkHuanlveCard *card = new IkHuanlveCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getPhase() == Player::Play && player->getMark("IkHuanlveTimes") < 2;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getPhase() != Player::Play || player->getMark("IkHuanlveTimes") >= 2) return false;
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
        return pattern == "nullification";
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return player->getPhase() == Player::Play && player->getMark("IkHuanlveTimes") < 2;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return qrand() % 2 + 1;
    }
};

class IkHuanlveTrigger: public TriggerSkill {
public:
    IkHuanlveTrigger(): TriggerSkill("#ikhuanlve") {
        events << EventPhaseStart << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging)
            room->setPlayerMark(player, "IkHuanlveTimes", 0);
        else if (triggerEvent == EventPhaseStart) {
            if (player->isAlive() && player->getMark("@thought") > 0 && player->getPhase() == Player::Finish)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, "thhuanlve");
        player->loseMark("@thought");
        player->drawCards(2, objectName());
        return false;
    }
};

class IkMuguang: public TriggerSkill {
public:
    IkMuguang(): TriggerSkill("ikmuguang") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.nature == DamageStruct::Normal) == player->isKongcheng())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Normal)
            room->recover(player, RecoverStruct(damage.from, damage.card));

        return true;
    }
};

IkLihunCard::IkLihunCard() {
    will_throw = false;
    handling_method = MethodNone;
}

bool IkLihunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getHandcardNum() < Self->getHandcardNum();
}

void IkLihunCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    DummyCard *dummy_card = new DummyCard(effect.from->handCards());
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(),
                          effect.to->objectName(), "iklihun", QString());
    room->obtainCard(effect.to, dummy_card, reason, false);
    delete dummy_card;
    if (!effect.to->isKongcheng()) {
        const Card *dummy = room->askForExchange(effect.to, "iklihun", 998, 1, false, "@iklihun-showcard", false);
        if (!dummy)
            dummy = new DummyCard(QList<int>() << effect.to->getRandomHandCardId());
        foreach (int id, dummy->getSubcards())
            room->showCard(effect.to, id);
        QString choice = room->askForChoice(effect.from, "iklihun", "showcard+noshowcard");
        if (choice == "showcard") {
            room->obtainCard(effect.from, dummy);
        } else {
            QList<int> ids = effect.to->handCards();
            foreach (int id, dummy->getSubcards())
                ids.removeOne(id);
            DummyCard *dummy2 = new DummyCard(ids);
            room->obtainCard(effect.from, dummy2, false);
            delete dummy2;
        }
        delete dummy;
    }
}

class IkLihun: public ZeroCardViewAsSkill {
public:
    IkLihun(): ZeroCardViewAsSkill("iklihun") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("IkLihunCard");
    }

    virtual const Card *viewAs() const{
        return new IkLihunCard;
    }
};

class IkTianzuo : public TriggerSkill
{
public:
    IkTianzuo() : TriggerSkill("iktianzuo")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *p) const
    {
        return TriggerSkill::triggerable(p) && p->getPhase() == Player::Discard;
    }

    virtual bool cost(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        if (p->askForSkillInvoke(objectName())) {
            r->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        if (r->askForChoice(p, objectName(), "draw1+draw2") == "draw1")
            p->drawCards(1, objectName());
        else
            p->drawCards(2, objectName());
        r->setPlayerFlag(p, "iktianzuo");
        return false;
    }
};

class IkTianzuoRecord : public TriggerSkill
{
public:
    IkTianzuoRecord() : TriggerSkill("#iktianzuo-record")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *, ServerPlayer *p, QVariant &d, ServerPlayer* &) const
    {
        if (e == EventPhaseChanging)
            p->tag.remove("IkTianzuo");
        else if (e == CardsMoveOneTime) {
            if (p->getPhase() == Player::Discard) {
                CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
                if (move.from == p && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                    && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *p, QVariant &d, ServerPlayer *) const
    {
        QVariantList list = p->tag["IkTianzuo"].toList();
        CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
        list += IntList2VariantList(move.card_ids);
        p->tag["IkTianzuo"] = QVariant::fromValue(list);
        return false;
    }
};

class IkTianzuoRecover : public TriggerSkill
{
public:
    IkTianzuoRecover() : TriggerSkill("#iktianzuo")
    {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer* &) const
    {
        if (p && p->isAlive() && p->hasFlag("iktianzuo") && p->getPhase() == Player::Discard) {
            QVariantList list = p->tag["IkTianzuo"].toList();
            if (list.isEmpty())
                return QStringList();
            foreach (ServerPlayer *sp, r->getAlivePlayers()) {
                if (sp->isWounded() && sp->getHp() < list.length() && p->tag["IkYewu"].value<ServerPlayer *>() != sp)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        r->sendCompulsoryTriggerLog(p, "iktianzuo");
        foreach (ServerPlayer *sp, r->getAllPlayers()) {
            QVariantList list = p->tag["IkTianzuo"].toList();
            if (sp->isWounded() && sp->getHp() < list.length() && p->tag["IkYewu"].value<ServerPlayer *>() != sp)
                r->recover(sp, RecoverStruct(p));
        }
        return false;
    }
};

class IkYewu : public TriggerSkill
{
public:
    IkYewu() : TriggerSkill("ikyewu")
    {
        events << EventPhaseStart << Death;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *, ServerPlayer *p, QVariant &d, ServerPlayer* &) const
    {
        if (e == Death && !p->tag["IkYewu"].isNull()) {
            DeathStruct death = d.value<DeathStruct>();
            if (p->tag["IkYewu"].value<ServerPlayer *>() == death.who)
                p->tag.remove("IkYewu");
        } else if (e == EventPhaseStart) {
            if (TriggerSkill::triggerable(p) && p->tag["IkYewu"].isNull() && p->getPhase() == Player::Finish) {
                QVariantList list = p->tag["IkYewuCards"].toList();
                if (!list.isEmpty())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *r, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = r->askForPlayerChosen(p, r->getOtherPlayers(p), objectName(), "@ikyewu", true, true);
        if (target) {
            r->broadcastSkillInvoke(objectName());
            p->tag["IkYewuTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *p, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = p->tag["IkYewuTarget"].value<ServerPlayer *>();
        p->tag.remove("IkYewuTarget");
        if (target) {
            DummyCard *dummy = new DummyCard(VariantList2IntList(p->tag["IkYewuCards"].toList()));
            target->obtainCard(dummy);
            p->tag["IkYewu"] = QVariant::fromValue(target);
            delete dummy;
        }
        return false;
    }
};

class IkYewuRecord : public TriggerSkill
{
public:
    IkYewuRecord() : TriggerSkill("#ikyewu")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *, ServerPlayer *p, QVariant &d, ServerPlayer* &) const
    {
        if (e == EventPhaseChanging) {
            if (d.value<PhaseChangeStruct>().to == Player::NotActive)
                p->tag.remove("IkYewuCards");
        } else if (e == CardsMoveOneTime) {
            if (p->getPhase() == Player::Discard) {
                CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
                if (move.from == p && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                    && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *p, QVariant &d, ServerPlayer *) const
    {
        QVariantList list = p->tag["IkYewuCards"].toList();
        CardsMoveOneTimeStruct move = d.value<CardsMoveOneTimeStruct>();
        list += IntList2VariantList(move.card_ids);
        p->tag["IkYewuCards"] = QVariant::fromValue(list);
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

            player->addToPile("cluster", judge.card);
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
        player->clearOnePrivatePile("cluster");
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

class IkJilian: public TriggerSkill {
public:
    IkJilian(): TriggerSkill("ikjilian") {
        events << Damaged;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
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
        if (room->askForCard(owner, "^BasicCard", "@ikjilian", data, objectName())) {
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

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
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
        if (!owner)
            return false;
        return to_select->isKindOf("Jink") || to_select->isKindOf("NatureSlash");
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
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && !player->hasFlag("ikmoshan")) {
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
        room->broadcastSkillInvoke(objectName(), qrand() % 2 + 3);
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
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isNude())
                victims << p;
        }
        if (!victims.isEmpty()) {
            ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
            int id = room->askForCardChosen(player, victim, "he", objectName());
            room->obtainCard(player, id, false);
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return qrand() % 2 + 1;
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

    QString to_ikxieke = user_string;
    if (user_string == "slash"
        && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QStringList ikxieke_list;
            ikxieke_list << "slash";
            if (!ServerInfo.Extensions.contains("!maneuvering"))
                ikxieke_list << "thunder_slash" << "fire_slash";
            to_ikxieke = room->askForChoice(wenyang, "ikxieke_slash", ikxieke_list.join("+"));
    }

    Card *use_card = Sanguosha->cloneCard(to_ikxieke);
    use_card->setSkillName("ikxieke");
    return use_card;
}

const Card *IkXiekeCard::validateInResponse(ServerPlayer *wenyang) const{
    Room *room = wenyang->getRoom();

    QString to_ikxieke;
    if (user_string == "peach+analeptic") {
        QStringList ikxieke_list;
        ikxieke_list << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            ikxieke_list << "analeptic";
        to_ikxieke = room->askForChoice(wenyang, "ikxieke_saveself", ikxieke_list.join("+"));
    } else if (user_string == "slash") {
        QStringList ikxieke_list;
        ikxieke_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            ikxieke_list << "thunder_slash" << "fire_slash";
        to_ikxieke = room->askForChoice(wenyang, "ikxieke_slash", ikxieke_list.join("+"));
    } else
        to_ikxieke = user_string;

    Card *use_card = Sanguosha->cloneCard(to_ikxieke);
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
        room->setEmotion(player, "effects/iron_chain");
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
            room->setEmotion(target, "effects/iron_chain");
            room->broadcastProperty(target, "chained");
            room->getThread()->trigger(ChainStateChanged, room, target);
        }
        return false;
    }
};

class IkLunyao: public TriggerSkill {
public:
    IkLunyao(): TriggerSkill("iklunyao") {
        events << BeforeCardsMove << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setMark(objectName(), 0);
            }
        } else {
            if (!TriggerSkill::triggerable(player) || player->getMark(objectName()) >= 3)
                return QStringList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player)
                return QStringList();
            if ((move.to && move.to != player && move.to_place == Player::PlaceHand)
                || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                && move.reason.m_playerId != QString() && move.reason.m_playerId != player->objectName())) {
                foreach (Player::Place place, move.from_places) {
                    if (place == Player::PlaceHand || place == Player::PlaceEquip)
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());
        player->addMark(objectName());
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
        events << BeforeCardsMove << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("ikqimu"))
                        p->setFlags("-ikqimu");
                }
            }
            return QStringList();
        }
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
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player || player->hasFlag("ikqimu"))
            return QStringList();
        const Card *card = NULL;
        if (triggerEvent == PreCardUsed)
            card = data.value<CardUseStruct>().card;
        else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
               card = response.m_card;
        }
        if (card && card->getHandlingMethod() == Card::MethodUse && card->getTypeId() != Card::TypeSkill) {
            ServerPlayer *current = room->getCurrent();
            if (current && current->getPhase() != Player::NotActive) {
                player->setFlags("ikqimu");
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
            if (TriggerSkill::triggerable(player) && death.who->getMark("@canal") > 0)
                return QStringList(objectName());
            return QStringList();
        }
        if (triggerEvent == GameStart && TriggerSkill::triggerable(player))
            return QStringList(objectName());
        else if (triggerEvent == DrawNCards && player->getMark("@canal") > 0)
            return QStringList(objectName());
        else if (triggerEvent == PreCardUsed && player->getMark("@canal") > 0
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
                    if (p->getMark("@canal") > 0)
                        p->loseAllMarks("@canal");
                }
                if (player->getMark("@canal") > 0)
                    player->loseAllMarks("@canal");
            } else {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->gainMark("@canal");
                death.who->loseAllMarks("@canal");
            }
        } else if (triggerEvent == GameStart) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->gainMark("@canal");
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

class IkShuluo : public TriggerSkill
{
public:
    IkShuluo(): TriggerSkill("ikshuluo")
    {
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const
    {
        if (player->isDead() || player->getMark("@canal") == 0)
            return QStringList();
        ServerPlayer *p = room->findPlayerBySkillName(objectName());
        if (!p)
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || damage.from->isDead() || player == damage.from)
            return QStringList();
        ask_who = damage.from;
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName())) {
            if (!ask_who->hasSkill(objectName())) {
                LogMessage log;
                log.type = "#InvokeOthersSkill";
                log.from = ask_who;
                log.to << room->findPlayerBySkillName(objectName());
                log.arg = objectName();
                room->sendLog(log);
            }
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        room->setPlayerMark(player, "@canal", 0);
        ask_who->gainMark("@canal");
        if (!ask_who->hasSkill(objectName()) && player->canDiscard(ask_who, "he")) {
            room->setPlayerFlag(ask_who, "ikshuluo_InTempMoving");
            int first_id = room->askForCardChosen(player, ask_who, "he", "ikshuluo", false, Card::MethodDiscard);
            Player::Place original_place = room->getCardPlace(first_id);
            DummyCard *dummy = new DummyCard;
            dummy->addSubcard(first_id);
            ask_who->addToPile("#ikshuluo", dummy, false);
            if (player->canDiscard(ask_who, "he")) {
                int second_id = room->askForCardChosen(player, ask_who, "he", "ikshuluo", false, Card::MethodDiscard);
                dummy->addSubcard(second_id);
            }

            //move the first card back temporarily
            room->moveCardTo(Sanguosha->getCard(first_id), ask_who, original_place, false);
            room->setPlayerFlag(ask_who, "-ikshuluo_InTempMoving");
            room->throwCard(dummy, ask_who, player);
            delete dummy;
        }
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

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
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

        if (red) {
            QStringList choices;
            if (player->isWounded())
                choices << "recover";
            choices << "draw";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"));
            if (choice == "recover")
                room->recover(player, RecoverStruct(ask_who));
            else
                player->drawCards(1, objectName());
        }
        player->tag["IkWuyu"] = ikwuyu;
        return false;
    }
};

class IkWuyuRecord : public TriggerSkill {
public:
    IkWuyuRecord() : TriggerSkill("#ikwuyu-record") {
        events << CardsMoveOneTime;
        global = true;
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

class IkKezhan : public TriggerSkill {
public:
    IkKezhan() : TriggerSkill("ikkezhan") {
        events << EventPhaseChanging << PreDamageDone;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && !damage.from->hasFlag("IkKezhanDamage")) {
                ServerPlayer *current = room->getCurrent();
                if (current == damage.from && current->getPhase() != Player::NotActive)
                    current->setFlags("IkKezhanDamage");
            }
            return QStringList();
        }
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        if (player->hasFlag("IkKezhanDamage"))
            room->addPlayerMark(player, "@lookup");
        if (!player->isKongcheng()) {
            const Card *card = room->askForCard(player, "Slash|black|.|hand", "@ikkezhan", QVariant(), Card::MethodNone);
            if (card) {
                room->showCard(player, card->getEffectiveId());
                if (card->isKindOf("Slash"))
                    room->addPlayerMark(player, "@lookup");
            }
        }
        foreach (ServerPlayer *p, room->getPlayers()) {
            if (p->isDead() && p->getRole() == "renegade") {
                room->addPlayerMark(player, "@lookup");
                break;
            }
        }

        if (player->getMark("@lookup") > 0)
            player->drawCards(player->getMark("@lookup"), objectName());
        else
            room->loseHp(player);

        room->setPlayerMark(player, "@lookup", 0);
        return false;
    }
};

class IkHuwu: public TriggerSkill {
public:
    IkHuwu(): TriggerSkill("ikhuwu") {
        events << DamageCaused;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
        if (player->isAlive() && player->getPhase() != Player::NotActive && player == room->getCurrent()) {
            if (player->getMark("ikhuwu") == 2) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->canDiscard(p, "he"))
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (room->askForCard(ask_who, "..", "@ikhuwu:" + player->objectName(), data, objectName())) {
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

class IkHuwuRecord: public TriggerSkill {
public:
    IkHuwuRecord(): TriggerSkill("#ikhuwu-record") {
        events << DamageCaused << EventPhaseChanging;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive)
                player->setMark("ikhuwu", 0);
        } else if (triggerEvent == DamageCaused) {
            if (player->isAlive() && player->getPhase() != Player::NotActive && player == room->getCurrent())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->addMark("ikhuwu");
        return false;
    }
};

class IkMosu: public TriggerSkill {
public:
    IkMosu(): TriggerSkill("ikmosu") {
        events << DamageInflicted;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player) && !player->hasFlag("IkMosuUsed")) {
            ServerPlayer *current = room->getCurrent();
            if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
                DamageStruct damage = data.value<DamageStruct>();
                if (damage.from && damage.from->getMark("ikmosu") != 1)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->setPlayerFlag(player, "IkMosuUsed");
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->isWounded())
            room->recover(player, RecoverStruct(player));

        QList<ServerPlayer *> players;
        players << data.value<DamageStruct>().from << player;
        room->sortByActionOrder(players);

        room->drawCards(players, 1, objectName());

        return true;
    }
};

class IkMosuRecord: public TriggerSkill {
public:
    IkMosuRecord(): TriggerSkill("#ikmosu-record") {
        events << DamageInflicted << EventPhaseChanging;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("IkMosuUsed"))
                        room->setPlayerFlag(p, "-IkMosuUsed");
                    p->setMark("ikmosu", 0);
                }
            }
        } else if (triggerEvent == DamageInflicted) {
            ServerPlayer *current = room->getCurrent();
            if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
                DamageStruct damage = data.value<DamageStruct>();
                if (damage.from)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        data.value<DamageStruct>().from->addMark("ikmosu");
        return false;
    }
};

IkSuyiCard::IkSuyiCard() {
}

void IkSuyiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    QStringList choices;
    choices << "draw";
    if (effect.to->canDiscard(effect.to, "h"))
        choices << "discard";
    QString choice = room->askForChoice(effect.from, "iksuyi", choices.join("+"));
    if (choice == "discard")
        room->askForDiscard(effect.to, "iksuyi", 1, 1);
    else
        effect.to->drawCards(1, "iksuyi");
}

class IkSuyi: public ZeroCardViewAsSkill {
public:
    IkSuyi(): ZeroCardViewAsSkill("iksuyi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkSuyiCard");
    }

    virtual const Card *viewAs() const{
        return new IkSuyiCard;
    }
};

class IkYihui: public TriggerSkill {
public:
    IkYihui(): TriggerSkill("ikyihui") {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player || player->isAlive() || !player->hasSkill(objectName()))
            return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@ikyihui", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkYihuiTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = player->tag["IkYihuiTarget"].value<ServerPlayer *>();
        player->tag.remove("IkYihuiTarget");
        if (target)
            room->handleAcquireDetachSkills(target, "iksuyi|ikyihui");
        return false;
    }
};

IkQiansheCard::IkQiansheCard() {
    will_throw = false;
}

bool IkQiansheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getHandcardNum() > Self->getHandcardNum();
}

void IkQiansheCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    const Card *dummy = room->askForExchange(effect.to, "ikqianshe", 998, 1, false, "@ikqianshe-discard", true);
    if (!dummy)
        dummy = new DummyCard;
    QList<CardsMoveStruct> moves;
    CardMoveReason(CardMoveReason::S_REASON_DISCARD, effect.from->objectName(), "ikqianshe", QString());
    if (subcardsLength() > 0) {
        moves << CardsMoveStruct(subcards,
                                 NULL,
                                 Player::DiscardPile,
                                 CardMoveReason(CardMoveReason::S_REASON_DISCARD,
                                                effect.from->objectName(),
                                                "ikqianshe",
                                                QString()));
        LogMessage log;
        log.type = "$DiscardCard";
        log.from = effect.from;
        log.card_str = IntList2StringList(subcards).join("+");
        room->sendLog(log);
    }
    if (dummy->subcardsLength() > 0) {
        moves << CardsMoveStruct(dummy->getSubcards(),
                                 NULL,
                                 Player::DiscardPile,
                                 CardMoveReason(CardMoveReason::S_REASON_DISCARD,
                                                effect.to->objectName(),
                                                "ikqianshe",
                                                QString()));
        LogMessage log;
        log.type = "$DiscardCard";
        log.from = effect.to;
        log.card_str = IntList2StringList(dummy->getSubcards()).join("+");
        room->sendLog(log);
    }
    if (!moves.isEmpty())
        room->moveCardsAtomic(moves, true);
    if (subcardsLength() > dummy->subcardsLength()) {
        room->broadcastSkillInvoke("ikqianshe", 3);
        room->damage(DamageStruct("ikqianshe", effect.from, effect.to));
    }
    delete dummy;
}

class IkQianshe: public ViewAsSkill {
public:
    IkQianshe(): ViewAsSkill("ikqianshe") {
        response_pattern = "@@ikqianshe";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkQiansheCard");
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        Card *card = new IkQiansheCard;
        card->addSubcards(cards);
        return card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return qrand() % 2 + 1;
    }
};

IkDaoleiCard::IkDaoleiCard() {
    will_throw = false;
}

bool IkDaoleiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getHandcardNum() < Self->getHandcardNum();
}

void IkDaoleiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "ikdaolei", QString());
    room->obtainCard(effect.to, this, reason);
    int max = effect.to->getHandcardNum();
    QStringList choices;
    for (int i = 1; i <= max; ++i)
        choices << QString::number(i);
    QString choice = room->askForChoice(effect.from, "ikdaolei_show", choices.join("+"));
    int n = choice.toInt();
    QList<int> original_ids = effect.to->handCards();
    qShuffle(original_ids);
    QList<int> ids = original_ids.mid(0, n);
    foreach (int id, ids)
        room->showCard(effect.to, id);
    bool spade = false;
    foreach (int id, ids) {
        const Card *card = Sanguosha->getCard(id);
        if (card->getSuit() == Card::Spade) {
            spade = true;
            break;
        }
    }
    if (spade) {
        QList<int> obtain;
        foreach (int id, original_ids) {
            if (!ids.contains(id))
                obtain << id;
        }
        if (!obtain.isEmpty()) {
            room->broadcastSkillInvoke("ikdaolei", 3);
            Card *dummy = new DummyCard(obtain);
            room->obtainCard(effect.from, dummy, false);
            delete dummy;
        }
    }
}

class IkDaolei: public OneCardViewAsSkill {
public:
    IkDaolei(): OneCardViewAsSkill("ikdaolei") {
        filter_pattern = ".|spade";
        response_pattern = "@@ikdaolei";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkDaoleiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new IkDaoleiCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return qrand() % 2 + 1;
    }
};

class IkYuanshou: public TriggerSkill {
public:
    IkYuanshou(): TriggerSkill("ikyuanshou") {
        events << EventPhaseStart << TargetConfirming;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("Slash") && player->getMark("@protect") > 0) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (player->getMark("yuanshou_" + p->objectName()) > 0)
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        } else {
            if (player->getPhase() == Player::RoundStart && !player->tag["IkYuanshouTarget"].isNull()) {
                ServerPlayer *target = player->tag["IkYuanshouTarget"].value<ServerPlayer *>();
                player->tag.remove("IkYuanshouTarget");
                room->setPlayerMark(player, "yuanshou_" + player->objectName(), 0);
                room->removePlayerMark(player, "@protect");
                if (target) {
                    room->setPlayerMark(target, "yuanshou_" + player->objectName(), 0);
                    room->removePlayerMark(target, "@protect");
                }
            } else if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Start)
                skill_list.insert(player, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@ikyuanshou", true, true);
            if (p) {
                room->broadcastSkillInvoke(objectName());
                player->tag["IkYuanshouTarget"] = QVariant::fromValue(p);
                return true;
            }
        } else if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *target = player->tag["IkYuanshouTarget"].value<ServerPlayer *>();
            room->addPlayerMark(player, "yuanshou_" + player->objectName());
            room->addPlayerMark(player, "@protect");
            if (target) {
                room->addPlayerMark(target, "yuanshou_" + player->objectName());
                room->addPlayerMark(target, "@protect");
            }
        } else {
            if (!room->askForCard(ask_who, "..", "@ikyuanshou-discard", QVariant(), Card::MethodDiscard))
                room->loseHp(ask_who);
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue(use);
            if (use.from && use.from->isAlive() && ask_who->isAlive()) {
                Duel *duel = new Duel(Card::NoSuit, 0);
                duel->setSkillName("_ikyuanshou");
                if (ask_who->isProhibited(use.from, duel))
                    delete duel;
                else
                    room->useCard(CardUseStruct(duel, ask_who, use.from));
            }
        }

        return false;
    }
};

class IkZhuxue : public TriggerSkill
{
public:
    IkZhuxue() : TriggerSkill("ikzhuxue")
    {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        QStringList skills;
        if (!TriggerSkill::triggerable(player))
            return skills;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place == Player::DiscardPile) {
            foreach (int id, move.card_ids) {
                const Card *card = Sanguosha->getCard(id);
                if (card->isKindOf("Peach"))
                    skills << objectName();
            }
        }

        return skills;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->gainMark("@blood");
        return false;
    }
};

class IkSheluo : public TriggerSkill
{
public:
    IkSheluo() : TriggerSkill("iksheluo")
    {
        events << EventMarksGot << AfterSwapPile;
        frequency = Compulsory;
        owner_only_skill = true;
    }

    virtual TriggerList triggerable(TriggerEvent e, Room *r, ServerPlayer *p, QVariant &d) const
    {
        TriggerList list;
        if (e == EventMarksGot && TriggerSkill::triggerable(p)) {
            if (d.toString() == "@blood" && (p->getMark("@blood") == 1
                                             || p->getMark("@blood") == 4
                                             || p->getMark("@blood") == 7))
                list.insert(p, QStringList(objectName()));
        } else if (e == AfterSwapPile) {
            foreach (ServerPlayer *sp, r->findPlayersBySkillName(objectName())) {
                if (sp->getMark("@blood") > 0)
                    list.insert(sp, QStringList(objectName()));
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        if (triggerEvent == EventMarksGot) {
            int n = player->getMark("@blood");
            QStringList skills = player->tag["IkSheluoSkills"].toStringList();
            if (n == 1 && !player->hasSkill("ikchenqing")) {
                room->acquireSkill(player, "ikchenqing");
                skills << "ikchenqing";
            } else if (n == 4 && !player->hasSkill("ikyuanjie")) {
                room->acquireSkill(player, "ikyuanjie");
                skills << "ikyuanjie";
            } else if (n == 7 && !player->hasSkill("ikhunkao")) {
                room->acquireSkill(player, "ikhunkao");
                skills << "ikhunkao";
            } else
                return false;
            if (player->getMark("@zhonggui") && skills.last() != "ikchenqing") {
                QStringList choices;
                foreach (const Skill *skill, player->getVisibleSkillList()) {
                    if (!skill->isOwnerOnlySkill())
                        choices << skill->objectName();
                }
                if (!choices.isEmpty()) {
                    QString choice = room->askForChoice(player, objectName(), choices.join("+"));
                    room->detachSkillFromPlayer(player, choice);
                    if (skills.contains(choice))
                        skills.removeOne(choice);
                }
            }
            player->tag["IkSheluoSkills"] = QVariant::fromValue(skills);
        } else {
            ask_who->loseAllMarks("@blood");
            QStringList losts;
            foreach (QString skill, ask_who->tag["IkSheluoSkills"].toStringList())
                losts << "-" + skill;
            if (!losts.isEmpty())
                room->handleAcquireDetachSkills(ask_who, losts, true);
            ask_who->tag["IkSheluoSkills"] = QVariant::fromValue(QStringList());
        }
        return false;
    }
};

class IkZhonggui : public TriggerSkill
{
public:
    IkZhonggui() : TriggerSkill("ikzhonggui")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Start
                && player->getRoom()->getTag("SwapPile").toInt() > 0 && player->getMark("@zhonggui") == 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        player->drawCards(2, objectName());
        room->setPlayerMark(player, "@zhonggui", 1);
        room->changeMaxHpForAwakenSkill(player, 1);
        return false;
    }
};

class IkJuexiang: public PhaseChangeSkill {
public:
    IkJuexiang(): PhaseChangeSkill("ikjuexiang") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && target->canDiscard(target, "h");
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        QList<const Card *> cards = player->getHandcards();
        player->throwAllHandCards();
        if (cards.length() >= player->getHp())
            player->drawCards(1, objectName());
        foreach (const Card *c, cards) {
            if (c->getTypeId() != Card::TypeBasic || c->isKindOf("Slash"))
                return false;
        }
        player->gainAnExtraTurn();
        player->addMark("ikjuexiang");
        return false;
    }
};

class IkJuexiangClear: public TriggerSkill {
public:
    IkJuexiangClear(): TriggerSkill("#ikjuexiang-clear") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer* &) const {
        if (triggerEvent == EventPhaseStart && target->getPhase() == Player::RoundStart && target->getMark("ikjuexiang") > 0) {
            target->removeMark("ikjuexiang");
            room->setPlayerFlag(target, "IkJuexiang");
            target->skip(Player::Discard);
            target->skip(Player::Finish);
        }
        return QStringList();
    }
};

class IkJuexiangTargetMod: public TargetModSkill {
public:
    IkJuexiangTargetMod(): TargetModSkill("#ikjuexiang-tar") {
        pattern = "^SkillCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasFlag("IkJuexiang"))
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("IkJuexiang"))
            return 1000;
        else
            return 0;
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
    wind035->addSkill(new IkYunjue);

    General *wind036 = new General(this, "wind036", "kaze");
    wind036->addSkill(new IkHunkao);
    wind036->addSkill(new SlashNoDistanceLimitSkill("ikhunkao"));
    related_skills.insertMulti("ikhunkao", "#ikhunkao-slash-ndl");

    General *wind045 = new General(this, "wind045", "kaze");
    wind045->addSkill(new IkHualan);
    wind045->addSkill(new IkHualanDraw);
    related_skills.insertMulti("ikhualan", "#ikhualan-draw");

    General *wind047 = new General(this, "wind047", "kaze", 3);
    wind047->addSkill(new IkTianhua);
    wind047->addSkill(new IkHuangshi);

    General *wind048 = new General(this, "wind048", "kaze");
    wind048->addSkill(new IkXizi);

    General *wind050 = new General(this, "wind050", "kaze");
    wind050->addSkill(new IkDongzhao);
    wind050->addSkill(new IkDongzhaoTrigger);
    related_skills.insertMulti("ikdongzhao", "#ikdongzhao");

    General *wind051 = new General(this, "wind051", "kaze");
    wind051->addSkill(new IkElu);
    wind051->addRelateSkill("iktanyan");

    General *wind057 = new General(this, "wind057", "kaze");
    wind057->addSkill(new IkYinchou);

    General *bloom032 = new General(this, "bloom032", "hana");
    bloom032->addSkill(new IkFengxing);
    bloom032->addSkill(new IkFengxingShow);
    bloom032->addSkill(new IkFengxingDistance);
    bloom032->addSkill(new IkFengxingClear);
    related_skills.insertMulti("ikfengxing", "#ikfengxing");
    related_skills.insertMulti("ikfengxing", "#ikfengxing-distance");
    related_skills.insertMulti("ikfengxing", "#ikfengxing-clear");

    General *bloom034 = new General(this, "bloom034", "hana");
    bloom034->addSkill(new IkQizhong);

    General *bloom035 = new General(this, "bloom035", "hana");
    bloom035->addSkill(new IkDuduan);
    bloom035->addSkill(new IkDuduanMaxCards);
    related_skills.insertMulti("ikduduan", "#ikduduan");

    General *bloom036 = new General(this, "bloom036", "hana");
    bloom036->addSkill(new IkJimu);
    bloom036->addSkill(new IkJimuEffect);
    related_skills.insertMulti("ikjimu", "#ikjimu-effect");
    bloom036->addRelateSkill("ikxunlv");

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

    General *bloom052 = new General(this, "bloom052", "hana");
    bloom052->addSkill(new IkLingcha);
    bloom052->addSkill(new IkLingchaTrigger);
    related_skills.insertMulti("iklingcha", "#iklingcha");

    General *bloom053 = new General(this, "bloom053", "hana");
    bloom053->addSkill(new IkMingshi);

    General *bloom054 = new General(this, "bloom054", "hana", 3);
    bloom054->addSkill(new IkDuanni);
    bloom054->addSkill(new IkMeitong);

    General *bloom056 = new General(this, "bloom056", "hana", 3);
    bloom056->addSkill(new IkSheji);
    bloom056->addSkill(new IkPingwei);
    bloom056->addSkill(new IkPingweiRecord);
    related_skills.insertMulti("ikpingwei", "#ikpingwei");

    General *snow031 = new General(this, "snow031", "yuki", 3);
    snow031->addSkill(new IkLingyun);
    snow031->addSkill(new IkMiyao);

    General *snow034 = new General(this, "snow034", "yuki");
    snow034->addSkill(new IkLixin);

    General *snow035 = new General(this, "snow035", "yuki");
    snow035->addSkill(new IkShenshu);
    snow035->addSkill(new IkMingwang);
    snow035->addSkill(new IkQiyi);

    General *snow036 = new General(this, "snow036", "yuki", 3);
    snow036->addSkill("ikmitu");
    snow036->addSkill(new IkLinghui);

    General *snow045 = new General(this, "snow045", "yuki");
    snow045->addSkill(new IkLvyan);
    snow045->addSkill(new IkWuming);

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

    General *snow051 = new General(this, "snow051", "yuki", 3);
    snow051->addSkill(new IkWanmi);
    snow051->addSkill(new IkWanmiTargetMod);
    related_skills.insertMulti("ikwanmi", "#ikwanmi-tar");
    snow051->addSkill(new IkGuichan);

    General *snow052 = new General(this, "snow052", "yuki", 3, false);
    snow052->addSkill(new IkHuanlve);
    snow052->addSkill(new IkHuanlveTrigger);
    related_skills.insertMulti("ikhuanlve", "#ikhuanlve");
    snow052->addSkill(new IkMuguang);
    snow052->addRelateSkill("thjizhi");
    snow052->addRelateSkill("ikchilian");

    General *snow054 = new General(this, "snow054", "yuki", 4);
    snow054->addSkill(new IkLihun);

    General *snow057 = new General(this, "snow057", "yuki", 3);
    snow057->addSkill(new IkTianzuo);
    snow057->addSkill(new IkTianzuoRecord);
    snow057->addSkill(new IkTianzuoRecover);
    related_skills.insertMulti("iktianzuo", "#iktianzuo-record");
    related_skills.insertMulti("iktianzuo", "#iktianzuo");
    snow057->addSkill(new IkYewu);
    snow057->addSkill(new IkYewuRecord);
    related_skills.insertMulti("ikyewu", "#ikyewu");

    General *luna030 = new General(this, "luna030", "tsuki");
    luna030->addSkill(new IkLingcu);

    General *luna031 = new General(this, "luna031", "tsuki", 3);
    luna031->addSkill(new IkQisi);
    luna031->addSkill(new IkJilian);

    General *luna032 = new General(this, "luna032", "tsuki", 3, false);
    luna032->addSkill(new IkJichang);
    luna032->addSkill(new IkManwu);

    General *luna033 = new General(this, "luna033", "tsuki", 3);
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
    luna047->addSkill(new FakeMoveSkill("ikshuluo"));
    related_skills.insertMulti("ikshuluo", "#ikshuluo-fake-move");

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

    General *luna051 = new General(this, "luna051", "tsuki");
    luna051->addSkill(new IkKezhan);

    General *luna053 = new General(this, "luna053", "tsuki", 3, false);
    luna053->addSkill(new IkHuwu);
    luna053->addSkill(new IkHuwuRecord);
    related_skills.insertMulti("ikhuwu", "#ikhuwu-record");
    luna053->addSkill(new IkMosu);
    luna053->addSkill(new IkMosuRecord);
    related_skills.insertMulti("ikmosu", "#ikmosu-record");

    General *luna054 = new General(this, "luna054", "tsuki");
    luna054->addSkill(new IkSuyi);
    luna054->addSkill(new IkYihui);

    General *luna055 = new General(this, "luna055", "tsuki", 3);
    luna055->addSkill(new IkQianshe);
    luna055->addSkill(new IkDaolei);

    General *luna056 = new General(this, "luna056", "tsuki");
    luna056->addSkill(new IkYuanshou);

    General *luna057 = new General(this, "luna057", "tsuki", 3);
    luna057->addSkill(new IkZhuxue);
    luna057->addSkill(new IkSheluo);
    luna057->addSkill(new IkZhonggui);

    General *luna058 = new General(this, "luna058", "tsuki");
    luna058->addSkill(new IkJuexiang);
    luna058->addSkill(new IkJuexiangClear);
    luna058->addSkill(new IkJuexiangTargetMod);
    related_skills.insertMulti("ikjuexiang", "#ikjuexiang-clear");
    related_skills.insertMulti("ikjuexiang", "#ikjuexiang-tar");

    addMetaObject<IkZhijuCard>();
    addMetaObject<IkJilunCard>();
    addMetaObject<IkKangjinCard>();
    addMetaObject<IkHunkaoCard>();
    addMetaObject<IkHualanCard>();
    addMetaObject<IkHuangshiCard>();
    addMetaObject<IkDongzhaoCard>();
    addMetaObject<IkJimuCard>();
    addMetaObject<IkDengpoCard>();
    addMetaObject<IkQihunCard>();
    addMetaObject<IkLingchaCard>();
    addMetaObject<IkDuanniCard>();
    addMetaObject<IkLixinCard>();
    addMetaObject<IkMingwangCard>();
    addMetaObject<IkLinghuiCard>();
    addMetaObject<IkXiaowuCard>();
    addMetaObject<IkHuanlveCard>();
    addMetaObject<IkLihunCard>();
    addMetaObject<IkQisiCard>();
    addMetaObject<IkManwuCard>();
    addMetaObject<IkXianlvCard>();
    addMetaObject<IkLianwuCard>();
    addMetaObject<IkLianwuDrawCard>();
    addMetaObject<IkXiekeCard>();
    addMetaObject<IkSuyiCard>();
    addMetaObject<IkQiansheCard>();
    addMetaObject<IkDaoleiCard>();

    skills << new IkQihunViewAsSkill;
}

ADD_PACKAGE(IkaiKa)
