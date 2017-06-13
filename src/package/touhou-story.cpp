#include "touhou-story.h"

#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "fantasy.h"
#include "general.h"
#include "maneuvering.h"

ThChayinCard::ThChayinCard()
{
}

bool ThChayinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (!targets.isEmpty())
        return false;
    Suit suit = Sanguosha->getCard(subcards.first())->getSuit();
    foreach (const Card *c, to_select->getEquips()) {
        if (c->getSuit() == suit)
            return true;
    }
    foreach (const Card *c, to_select->getJudgingArea()) {
        if (c->getSuit() == suit)
            return true;
    }
    return false;
}

void ThChayinCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    bool has_suit = false;
    QList<int> disabled_ids;
    Suit suit = Sanguosha->getCard(subcards.first())->getSuit();
    foreach (const Card *c, effect.to->getCards("ej")) {
        if (c->getSuit() != suit)
            disabled_ids << c->getId();
        else
            has_suit = true;
    }
    if (!has_suit)
        return;
    int id = room->askForCardChosen(effect.from, effect.to, "ej", "thchayin", false, MethodNone, disabled_ids);
    if (id != -1)
        room->obtainCard(effect.from, id);
}

class ThChayinViewAsSkill : public OneCardViewAsSkill
{
public:
    ThChayinViewAsSkill()
        : OneCardViewAsSkill("thchayin")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@thchayin";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThChayinCard *card = new ThChayinCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThChayin : public TriggerSkill
{
public:
    ThChayin()
        : TriggerSkill("thchayin")
    {
        view_as_skill = new ThChayinViewAsSkill;
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start && target->canDiscard(target, "h");
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@thchayin", "@thchayin", -1, Card::MethodDiscard);
    }
};

class ThXuanyan : public TriggerSkill
{
public:
    ThXuanyan()
        : TriggerSkill("thxuanyan")
    {
        events << DamageInflicted << TurnedOver;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data,
                                    ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.damage > 1)
                return QStringList(objectName());
        } else if (triggerEvent == TurnedOver)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage = 1;
            data = QVariant::fromValue(damage);
        } else
            player->drawCards(1, objectName());
        return false;
    }
};

class ThShenghuan : public OneCardViewAsSkill
{
public:
    ThShenghuan()
        : OneCardViewAsSkill("thshenghuan")
    {
        filter_pattern = "Armor,Weapon,Horse";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getPhase() != Player::Play)
            return false;
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard->isKindOf("Armor")) {
            Analeptic *anal = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
            anal->setSkillName(objectName());
            anal->addSubcard(originalCard);
            return anal;
        } else if (originalCard->isKindOf("Weapon")) {
            KnownBoth *kb = new KnownBoth(originalCard->getSuit(), originalCard->getNumber());
            kb->setSkillName(objectName());
            kb->addSubcard(originalCard);
            return kb;
        } else if (originalCard->isKindOf("Horse")) {
            ExNihilo *en = new ExNihilo(originalCard->getSuit(), originalCard->getNumber());
            en->setSkillName(objectName());
            en->addSubcard(originalCard);
            return en;
        } else
            return NULL;
    }
};

ThShenmieDialog *ThShenmieDialog::getInstance()
{
    static ThShenmieDialog *instance;
    if (instance == NULL)
        instance = new ThShenmieDialog();

    return instance;
}

ThShenmieDialog::ThShenmieDialog()
{
    setObjectName("thshenmie");
    setWindowTitle(Sanguosha->translate("thshenmie"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, (void (QButtonGroup::*)(QAbstractButton *))(&QButtonGroup::buttonClicked), this,
            &ThShenmieDialog::selectCard);
}

void ThShenmieDialog::popup()
{
    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QStringList card_names;
    card_names << "slash"
               << "duel";

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

void ThShenmieDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag["thshenmie"] = QVariant::fromValue(card);
    emit onButtonClick();
    accept();
}

ThShenmieCard::ThShenmieCard()
{
    will_throw = false;
}

bool ThShenmieCard::targetFixed() const
{
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

    const Card *card = Self->tag.value("thshenmie").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->addSubcards(subcards);
    new_card->setSkillName("thshenmie");
    new_card->deleteLater();
    return new_card->targetFixed();
}

bool ThShenmieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
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

    const Card *card = Self->tag.value("thshenmie").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->addSubcards(subcards);
    new_card->setSkillName("thshenmie");
    new_card->deleteLater();
    return new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool ThShenmieCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
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

    const Card *card = Self->tag.value("thshenmie").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->addSubcards(subcards);
    new_card->setSkillName("thshenmie");
    new_card->deleteLater();
    return new_card->targetsFeasible(targets, Self);
}

const Card *ThShenmieCard::validate(CardUseStruct &) const
{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("thshenmie");
    use_card->addSubcards(subcards);
    return use_card;
}

const Card *ThShenmieCard::validateInResponse(ServerPlayer *) const
{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("thshenmie");
    use_card->addSubcards(subcards);
    return use_card;
}

class ThShenmie : public ViewAsSkill
{
public:
    ThShenmie()
        : ViewAsSkill("thshenmie")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() > 1)
            return false;
        if (to_select->isEquipped())
            return false;
        if (selected.isEmpty())
            return true;
        return to_select->getSuit() == selected.first()->getSuit();
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        Duel *duel = new Duel(Card::NoSuit, 0);
        duel->deleteLater();
        return slash->isAvailable(player) || duel->isAvailable(player);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            ThShenmieCard *card = new ThShenmieCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcards(cards);
            return card;
        }

        const Card *c = Self->tag.value("thshenmie").value<const Card *>();
        if (c) {
            ThShenmieCard *card = new ThShenmieCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual SkillDialog *getDialog() const
    {
        return ThShenmieDialog::getInstance();
    }
};

class ThShenmieTargetMod : public TargetModSkill
{
public:
    ThShenmieTargetMod()
        : TargetModSkill("#thshenmie-tar")
    {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasSkill("thshenmie"))
            return 2;
        else
            return 0;
    }
};

class ThHongdao : public DrawCardsSkill
{
public:
    ThHongdao()
        : DrawCardsSkill("thhongdao")
    {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());

        LogMessage log;
        log.type = "#IkChenyanGood";
        log.from = player;
        log.arg = "3";
        log.arg2 = objectName();
        room->sendLog(log);
        return n + 3;
    }
};

class ThHongdaoKeep : public MaxCardsSkill
{
public:
    ThHongdaoKeep()
        : MaxCardsSkill("#thhongdao-keep")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill("thhongdao"))
            return 3;
        else
            return 0;
    }
};

HulaopassPackage::HulaopassPackage()
    : Package("hulaopass")
{
    General *story002 = new General(this, "story002", "kami", 8, true, true);
    story002->addSkill("thjibu");
    story002->addSkill("ikwushuang");
    story002->addSkill(new ThChayin);
    story002->addSkill(new ThXuanyan);

    General *story003 = new General(this, "story003", "kami", 4, true, true);
    story003->addSkill("thjibu");
    story003->addSkill("ikwushuang");
    story003->addSkill("thchayin");
    story003->addSkill("thxuanyan");
    story003->addSkill(new ThShenghuan);
    story003->addSkill(new ThShenmie);
    story003->addSkill(new ThShenmieTargetMod);
    related_skills.insertMulti("thshenmie", "#thshenmie-tar");
    story003->addSkill(new ThHongdao);
    story003->addSkill(new ThHongdaoKeep);
    related_skills.insertMulti("thhongdao", "#thhongdao-keep");

    addMetaObject<ThChayinCard>();
    addMetaObject<ThShenmieCard>();
}

ADD_PACKAGE(Hulaopass)
