#include "ikai-ka.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"

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

class IkKangjin: public OneCardViewAsSkill {
public:
    IkKangjin(): OneCardViewAsSkill("ikkangjin") {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        Duel *duel = new Duel(originalcard->getSuit(), originalcard->getNumber());
        duel->addSubcard(originalcard);
        duel->setSkillName(objectName());
        return duel;
    }
};

class IkKangjinTrigger: public TriggerSkill {
public:
    IkKangjinTrigger(): TriggerSkill("#ikkangjin") {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Duel") && damage.card->getSkillName() == "ikkangjin"
            && damage.to && damage.to->isAlive()) {
            ask_who = damage.by_user ? player : damage.to;
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        damage.to->drawCards(damage.to->getLostHp());
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

bool IkHunkaoCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == subcardsLength();
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
        return player->canDiscard(player, "h");
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

    addMetaObject<IkZhijuCard>();
    addMetaObject<IkJilunCard>();
    addMetaObject<IkHunkaoCard>();
}

ADD_PACKAGE(IkaiKa)