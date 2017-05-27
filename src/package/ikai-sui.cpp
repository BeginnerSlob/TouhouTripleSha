#include "ikai-sui.h"

#include "general.h"
#include "engine.h"
#include "client.h"
#include "settings.h"
#include "maneuvering.h"
#include "touhou-hana.h"

class IkHuahuan: public OneCardViewAsSkill {
public:
    IkHuahuan(): OneCardViewAsSkill("ikhuahuan") {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const{
        const Card *card = to_select;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
                return card->isKindOf("Jink");
            }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                if (pattern == "slash")
                    return card->isKindOf("Jink");
                else if (pattern == "jink")
                    return card->isKindOf("Slash");
            }
        default:
            return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard->isKindOf("Slash")) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        } else if (originalCard->isKindOf("Jink")) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        } else
            return NULL;
    }
};

class IkQizhou: public TriggerSkill {
public:
    IkQizhou(): TriggerSkill("ikqizhou") {
        events << CardResponded << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Jink") && resp.m_card->isVirtualCard() && resp.m_card->subcardsLength() > 0)
                return QStringList(objectName());
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->isVirtualCard() && use.card->subcardsLength() > 0)
                foreach (ServerPlayer *p, use.to)
                    if (!p->isKongcheng())
                        return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (triggerEvent == CardResponded && player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        } else if (triggerEvent == TargetSpecified) {
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == CardResponded)
            player->drawCards(1);
        else {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to) {
                if (p->isKongcheng()) continue;
                if (!player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) continue;
                int card_id = room->askForCardChosen(player, p, "h", objectName());
                player->obtainCard(Sanguosha->getCard(card_id), false);
            }
        }
        return false;
    }
};

class IkShushen: public TriggerSkill {
public:
    IkShushen(): TriggerSkill("ikshushen") {
        events << HpRecover;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (!TriggerSkill::triggerable(player)) return skill;
        RecoverStruct recover = data.value<RecoverStruct>();
        for (int i = 0; i < recover.recover; i++)
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@ikshushen", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkShushenTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = player->tag["IkShushenTarget"].value<ServerPlayer *>();
        player->tag.remove("IkShushenTarget");
        if (target) {
            QList<ServerPlayer *> targets;
            targets << target;
            if (target != player)
                targets << player;
            room->sortByActionOrder(targets);
            room->drawCards(targets, 2, objectName());
            foreach (ServerPlayer *p, targets) {
                if (p->canDiscard(p, "he"))
                    room->askForDiscard(p, objectName(), 1, 1, false, true);
            }
        }
        return false;
    }
};

class IkQiaoxia : public PhaseChangeSkill
{
public:
    IkQiaoxia() : PhaseChangeSkill("ikqiaoxia")
    {
    }

    virtual bool triggerable(const ServerPlayer *ganfuren) const
    {
        foreach (const Card *card, ganfuren->getHandcards()) {
            if (ganfuren->isJilei(card))
                return false;
        }
        return PhaseChangeSkill::triggerable(ganfuren)
                && ganfuren->getPhase() == Player::Start
                && !ganfuren->isKongcheng();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *ganfuren) const
    {
        Room *room = ganfuren->getRoom();
        int handcard_num = ganfuren->getHandcardNum();
        ganfuren->throwAllHandCards();
        if (handcard_num >= ganfuren->getHp()) {
            QStringList choices;
            if (ganfuren->isWounded())
                choices << "recover";
            choices << "draw";
            QString choice = room->askForChoice(ganfuren, objectName(), choices.join("+"));
            if (choice == "recover")
                room->recover(ganfuren, RecoverStruct(ganfuren));
            else
                ganfuren->drawCards(2, objectName());
        }
        return false;
    }
};

IkXielunCard::IkXielunCard() {
}

bool IkXielunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() >= Self->getLostHp())
        return false;

    int range_fix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getEffectiveId() == getEffectiveId()) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        range_fix += weapon->getRange() - Self->getAttackRange(false);
    } else if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getEffectiveId() == getEffectiveId())
        range_fix += 1;

    return Self->getMark("@jiuming") > 0 || Self->inMyAttackRange(to_select, range_fix);
}

void IkXielunCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    DamageStruct damage;
    damage.from = source;
    damage.reason = "ikxielun";

    foreach (ServerPlayer *p, targets) {
        damage.to = p;
        room->damage(damage);
    }
    foreach (ServerPlayer *p, targets) {
        if (p->isAlive())
            p->drawCards(1, "ikxielun");
    }
}

class IkXielun: public OneCardViewAsSkill {
public:
    IkXielun(): OneCardViewAsSkill("ikxielun") {
        filter_pattern = ".|red!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getLostHp() > 0 && player->canDiscard(player, "he") && !player->hasUsed("IkXielunCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkXielunCard *first = new IkXielunCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class IkCanyue: public TargetModSkill {
public:
    IkCanyue(): TargetModSkill("ikcanyue") {
        frequency = NotCompulsory;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return from->getMark(objectName());
        else
            return 0;
    }
};

class IkCanyueCount: public TriggerSkill {
public:
    IkCanyueCount(): TriggerSkill("#ikcanyue-count") {
        events << SlashMissed << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player || player->isDead() || !player->hasSkill("ikcanyue")) return QStringList();
        if (triggerEvent == SlashMissed) {
            if (player->getPhase() == Player::Play)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                if (player->getMark("ikcanyue") > 0)
                    room->setPlayerMark(player, "ikcanyue", 0);
            }
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->addPlayerMark(player, "ikcanyue");
        return false;
    }
};

class IkCanyueClear: public DetachEffectSkill {
public:
    IkCanyueClear(): DetachEffectSkill("ikcanyue") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        room->setPlayerMark(player, "ikcanyue", 0);
    }
};

class IkJiuming: public PhaseChangeSkill {
public:
    IkJiuming(): PhaseChangeSkill("ikjiuming") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Finish
               && target->getMark("@jiuming") == 0
               && target->getMark("ikjiuming_damage") >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#IkJiumingWake";
        log.from = player;
        log.arg = QString::number(player->getMark("ikjiuming_damage"));
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(player, "@jiuming", 1);
        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            room->recover(player, RecoverStruct(player));
            room->detachSkillFromPlayer(player, "ikcanyue");
        }

        return false;
    }
};

class IkJiumingCount: public TriggerSkill {
public:
    IkJiumingCount(): TriggerSkill("#ikjiuming-count") {
        events << PreDamageDone << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.from == room->getCurrent() && damage.from->getMark("@jiuming") == 0)
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                if (player->getMark("ikjiuming_damage") > 0)
                    room->setPlayerMark(player, "ikjiuming_damage", 0);
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        room->addPlayerMark(damage.from, "ikjiuming_damage", damage.damage);
        return false;
    }
};

class IkXiewang: public TriggerSkill {
public:
    IkXiewang(): TriggerSkill("ikxiewang") {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player == NULL) return QStringList();
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName()) {
                QStringList xiewang_skills = player->tag["IkXiewangSkills"].toStringList();
                QStringList detachList;
                foreach (QString skill_name, xiewang_skills)
                    detachList.append("-" + skill_name);
                room->handleAcquireDetachSkills(player, detachList, true, true);
                player->tag["IkXiewangSkills"] = QVariant();
            }
            return QStringList();
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return QStringList();
        }

        if (!player->isAlive() || !player->hasSkill(objectName(), true)) return QStringList();

        acquired_skills.clear();
        detached_skills.clear();
        IkXiewangChange(room, player, 1, "thshenyou");
        IkXiewangChange(room, player, 2, "thkuangqi");
        IkXiewangChange(room, player, 3, "iktiaoxin");
        if (!acquired_skills.isEmpty() || !detached_skills.isEmpty())
            room->handleAcquireDetachSkills(player, acquired_skills + detached_skills, true, true);
        return QStringList();
    }

private:
    void IkXiewangChange(Room *room, ServerPlayer *player, int hp, const QString &skill_name) const{
        QStringList xiewang_skills = player->tag["IkXiewangSkills"].toStringList();
        if (player->getHp() <= hp) {
            if (!xiewang_skills.contains(skill_name)) {
                room->notifySkillInvoked(player, "ikxiewang");
                if (player->getHp() == hp)
                    room->broadcastSkillInvoke("ikxiewang", 4 - hp);
                acquired_skills.append(skill_name);
                xiewang_skills << skill_name;
            }
        } else {
            if (xiewang_skills.contains(skill_name)) {
                detached_skills.append("-" + skill_name);
                xiewang_skills.removeOne(skill_name);
            }
        }
        player->tag["IkXiewangSkills"] = QVariant::fromValue(xiewang_skills);
    }

    mutable QStringList acquired_skills, detached_skills;
};

class IkShengzunViewAsSkill: public ViewAsSkill {
public:
    IkShengzunViewAsSkill(): ViewAsSkill("ikshengzun") {
    }

    static QList<const ViewAsSkill *> getLordViewAsSkills(const Player *player) {
        const Player *lord = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->isLord()) {
                lord = p;
                break;
            }
        }
        if (!lord) return QList<const ViewAsSkill *>();

        QList<const ViewAsSkill *> vs_skills;
        foreach (const Skill *skill, lord->getVisibleSkillList()) {
            if (skill->isLordSkill() && player->hasLordSkill(skill->objectName())) {
                const ViewAsSkill *vs = ViewAsSkill::parseViewAsSkill(skill);
                if (vs)
                    vs_skills << vs;
            }
        }
        return vs_skills;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtPlay(player))
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtResponse(player, pattern))
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtNullification(player))
                return true;
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        QString skill_name = Self->tag["ikshengzun"].toString();
        if (skill_name.isEmpty()) return false;
        const ViewAsSkill *vs_skill = Sanguosha->getViewAsSkill(skill_name);
        if (vs_skill) return vs_skill->viewFilter(selected, to_select);
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        QString skill_name = Self->tag["ikshengzun"].toString();
        if (skill_name.isEmpty()) return NULL;
        const ViewAsSkill *vs_skill = Sanguosha->getViewAsSkill(skill_name);
        if (vs_skill) return vs_skill->viewAs(cards);
        return NULL;
    }
};

#include <QHBoxLayout>
#include <QCommandLinkButton>

IkShengzunDialog *IkShengzunDialog::getInstance() {
    static IkShengzunDialog *instance;
    if (instance == NULL)
        instance = new IkShengzunDialog();

    return instance;
}

IkShengzunDialog::IkShengzunDialog() {
    setObjectName("ikshengzun");
    setWindowTitle(Sanguosha->translate("ikshengzun"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectSkill(QAbstractButton *)));
}

void IkShengzunDialog::popup() {
    Self->tag.remove(objectName());
    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QList<const ViewAsSkill *> vs_skills = IkShengzunViewAsSkill::getLordViewAsSkills(Self);
    int count = 0;
    QString name;
    foreach (const ViewAsSkill *skill, vs_skills) {
        QAbstractButton *button = createSkillButton(skill->objectName());
        button->setEnabled(skill->isAvailable(Self, Sanguosha->currentRoomState()->getCurrentCardUseReason(),
                                              Sanguosha->currentRoomState()->getCurrentCardUsePattern()));
        if (button->isEnabled()) {
            count++;
            name = skill->objectName();
        }
        button_layout->addWidget(button);
    }

    if (count == 0) {
        emit onButtonClick();
        return;
    } else if (count == 1) {
        Self->tag[objectName()] = name;
        emit onButtonClick();
        return;
    }

    exec();
}

void IkShengzunDialog::selectSkill(QAbstractButton *button) {
    Self->tag[objectName()] = button->objectName();
    emit onButtonClick();
    accept();
}

QAbstractButton *IkShengzunDialog::createSkillButton(const QString &skill_name) {
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (!skill) return NULL;

    QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(skill_name));
    button->setObjectName(skill_name);
    button->setToolTip(skill->getDescription(Self->getSkillStep(skill_name)));

    group->addButton(button);
    return button;
}

class IkShengzun: public TriggerSkill {
public:
    IkShengzun(): TriggerSkill("ikshengzun") {
        frequency = Compulsory;
        view_as_skill = new IkShengzunViewAsSkill;
    }

    virtual QDialog *getDialog() const{
        return IkShengzunDialog::getInstance();
    }
};

class IkYanyu: public TriggerSkill {
public:
    IkYanyu(): TriggerSkill("ikyanyu") {
        events << EventPhaseStart << BeforeCardsMove << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play) {
                foreach (ServerPlayer *xiahou, room->findPlayersBySkillName(objectName())) {
                    if (!xiahou->canDiscard(xiahou, "he")) continue;
                    skill_list.insert(xiahou, QStringList(objectName()));
                }
            }
        } else if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)) {
            ServerPlayer *current = room->getCurrent();
            if (!current || (current->getPhase() != Player::Play && current->getPhase() != Player::Discard)) return skill_list;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                foreach (int id, move.card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (!card->isKindOf("Slash") && !card->isKindOf("Jink")) continue;
                    if (player->getMark("IkYanyuDiscard" + QString::number(card->getTypeId())) > 0) {
                        skill_list.insert(player, QStringList(objectName()));
                        break;
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    p->setMark("IkYanyuDiscard1", 0);
                    p->setMark("IkYanyuDiscard2", 0);
                    p->setMark("IkYanyuDiscard3", 0);
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *xiahou) const{
        if (triggerEvent == EventPhaseStart) {
            const Card *card = room->askForCard(xiahou, "Slash,Jink", "@ikyanyu-discard", QVariant(), objectName());
            if (card) {
                room->broadcastSkillInvoke(objectName());
                xiahou->addMark("IkYanyuDiscard" + QString::number(card->getTypeId()), 3);
            }
        } else if (triggerEvent == BeforeCardsMove)
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        QList<int> ids, disabled;
        QList<int> all_ids = move.card_ids;
        foreach (int id, move.card_ids) {
            const Card *card = Sanguosha->getCard(id);
            if (!card->isKindOf("Slash") && !card->isKindOf("Jink")) {
                disabled << id;
                continue;
            }
            if (player->getMark("IkYanyuDiscard" + QString::number(card->getTypeId())) > 0)
                ids << id;
            else
                disabled << id;
        }
        if (ids.isEmpty())
            return false;
        while (!ids.isEmpty()) {
            room->fillAG(all_ids, player, disabled);
            bool only = (all_ids.length() == 1);
            int card_id = -1;
            if (only)
                card_id = ids.first();
            else
                card_id = room->askForAG(player, ids, true, objectName());
            room->clearAG(player);
            if (card_id == -1) break;
            const Card *card = Sanguosha->getCard(card_id);
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                                                            QString("@ikyanyu-give:::%1:%2\\%3").arg(card->objectName())
                                                                                              .arg(card->getSuitString() + "_char")
                                                                                              .arg(card->getNumberString()),
                                                            only, true);
            if (target) {
                player->removeMark("IkYanyuDiscard" + QString::number(card->getTypeId()));
                Player::Place place = move.from_places.at(move.card_ids.indexOf(card_id));
                QList<int> _card_id;
                _card_id << card_id;
                move.removeCardIds(_card_id);
                data = QVariant::fromValue(move);
                ids.removeOne(card_id);
                disabled << card_id;
                foreach (int id, ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (player->getMark("IkYanyuDiscard" + QString::number(card->getTypeId())) == 0) {
                        ids.removeOne(id);
                        disabled << id;
                    }
                }
                if (move.from && move.from->objectName() == target->objectName() && place != Player::PlaceTable) {
                    // just indicate which card she chose...
                    LogMessage log;
                    log.type = "$MoveCard";
                    log.from = target;
                    log.to << target;
                    log.card_str = QString::number(card_id);
                    room->sendLog(log);
                }
                room->obtainCard(target, card, move.reason);
                foreach (int id, ids) {
                    if (!move.card_ids.contains(id)) {
                        ids.removeOne(id);
                        disabled << id;
                        continue;
                    }
                    if (room->getCardPlace(id) != move.from_places.at(move.card_ids.indexOf(id))) {
                        ids.removeOne(id);
                        disabled << id;
                    }
                }
            } else
                break;
        }

        return false;
    }
};

class IkWuyue: public TriggerSkill {
public:
    IkWuyue(): TriggerSkill("ikwuyue") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() == Player::NotActive && move.from && move.from->isAlive() && move.from != player
            && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            foreach (int id, move.card_ids) {
                const Card *card = Sanguosha->getCard(id);
                if (card->getTypeId() == Card::TypeBasic)
                    return QStringList(objectName());
            }
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
        player->drawCards(1, objectName());
        if (player->getHandcardNum() > 4)
            room->askForDiscard(player, objectName(), 1, 1, false, true);
        return false;
    }
};

IkJuechongCard::IkJuechongCard() {
    target_fixed = true;
}

void IkJuechongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    JudgeStruct judge;
    judge.pattern = ".";
    judge.who = source;
    judge.reason = "ikjuechong";
    judge.play_animation = false;
    room->judge(judge);

    bool ok = false;
    int num = judge.pattern.toInt(&ok);
    if (ok)
        room->setPlayerMark(source, "ikjuechong", num);
}

class IkJuechongViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkJuechongViewAsSkill(): ZeroCardViewAsSkill("ikjuechong") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkJuechongCard");
    }

    virtual const Card *viewAs() const{
        return new IkJuechongCard;
    }
};

class IkJuechong: public TriggerSkill {
public:
    IkJuechong(): TriggerSkill("ikjuechong") {
        events << EventPhaseChanging << PreCardUsed << FinishJudge;
        view_as_skill = new IkJuechongViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "ikjuechong", 0);
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName())
                judge->pattern = QString::number(judge->card->getNumber());
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && player->getMark("ikjuechong") > 0
                && use.card->getNumber() > player->getMark("ikjuechong")) {
                if (use.m_addHistory)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        room->addPlayerHistory(player, use.card->getClassName(), -1);
        use.m_addHistory = false;
        data = QVariant::fromValue(use);

        return false;
    }
};

class IkJuechongTargetMod: public TargetModSkill {
public:
    IkJuechongTargetMod(): TargetModSkill("#ikjuechong-target") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (card->getNumber() < from->getMark("ikjuechong"))
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (from->getMark("ikjuechong") > 0
            && (card->getNumber() > from->getMark("ikjuechong") || card->hasFlag("Global_SlashAvailabilityChecker")))
            return 1000;
        else
            return 0;
    }
};

IkXinhuiCard::IkXinhuiCard() {
}

bool IkXinhuiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.size() < 2;
}

bool IkXinhuiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.size() == 2;
}

void IkXinhuiCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(1, "ikxinhui");
}

class IkXinhuiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkXinhuiViewAsSkill(): ZeroCardViewAsSkill("ikxinhui") {
        response_pattern = "@@ikxinhui";
    }

    virtual const Card *viewAs() const{
        return new IkXinhuiCard;
    }
};

class IkXinhui: public TriggerSkill {
public:
    IkXinhui(): TriggerSkill("ikxinhui") {
        events << PreDamageDone << EventPhaseEnd;
        view_as_skill = new IkXinhuiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseEnd) {
            if (player->hasFlag("IkXinhuiDamageInPlayPhase"))
                return QStringList();
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play)
                return QStringList(objectName());
        } else if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->getPhase() == Player::Play && !damage.from->hasFlag("IkXinhuiDamageInPlayPhase"))
                damage.from->setFlags("IkXinhuiDamageInPlayPhase");
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->askForUseCard(player, "@@ikxinhui", "@ikxinhui", -1, Card::MethodNone);
        return false;
    }
};

class IkYongji: public TriggerSkill {
public:
    IkYongji(): TriggerSkill("ikyongji") {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                p->setMark(objectName(), 0);
            return QStringList();
        }
        if (!TriggerSkill::triggerable(player) || player->getMark(objectName()) >= 3) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from && move.from->isAlive() && move.from->getPhase() == Player::NotActive
            && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard)
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        player->addMark(objectName());
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *from = (ServerPlayer *)move.from;
        from->drawCards(1, objectName());
        return false;
    }
};

IkMoqiCard::IkMoqiCard() {
    target_fixed = true;
}

void IkMoqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@moqi");
    room->addPlayerMark(source, "@moqiused");
    source->drawCards(2, "ikmoqi");
}

class IkMoqiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkMoqiViewAsSkill(): ZeroCardViewAsSkill("ikmoqi") {
    }

    virtual const Card *viewAs() const{
        return new IkMoqiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@moqi") >= 1;
    }
};

class IkMoqi: public PhaseChangeSkill {
public:
    IkMoqi(): PhaseChangeSkill("ikmoqi") {
        frequency = Limited;
        limit_mark = "@moqi";
        view_as_skill = new IkMoqiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return PhaseChangeSkill::triggerable(player)
            && player->getMark("@moqi") >= 1
            && player->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->removePlayerMark(player, "@moqi");
        room->addPlayerMark(player, "@moqiused");
        room->addPlayerMark(player, "ikmoqi");
        player->drawCards(2, objectName());

        return false;
    }
};

class IkMoqiFinish: public PhaseChangeSkill {
public:
    IkMoqiFinish(): PhaseChangeSkill("#ikmoqi") {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return player && player->isAlive()
            && player->getMark("ikmoqi") >= 1
            && player->getPhase() == Player::Finish;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->removePlayerMark(player, "ikmoqi");
        player->drawCards(2, "ikmoqi");

        return false;
    }
};

IkTianbeiCard::IkTianbeiCard() {
}

bool IkTianbeiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void IkTianbeiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->removePlayerMark(effect.from, "@tianbei");
    room->addPlayerMark(effect.from, "@tianbeiused");
    room->handleAcquireDetachSkills(effect.from, "-ikmoqi|-iktianbei");
    if (effect.from->isWounded())
        room->recover(effect.from, RecoverStruct(effect.from));
    room->addPlayerMark(effect.to, "@anshen");
    room->acquireSkill(effect.to, "ikanshen");
    if (effect.to != effect.from)
        effect.to->drawCards(2, "iktianbei");
}

class IkTianbei: public ZeroCardViewAsSkill {
public:
    IkTianbei(): ZeroCardViewAsSkill("iktianbei") {
        frequency = Limited;
        limit_mark = "@tianbei";
    }

    virtual const Card *viewAs() const{
        return new IkTianbeiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@tianbei") >= 1;
    }
};

class IkAnshen: public TriggerSkill {
public:
    IkAnshen(): TriggerSkill("ikanshen") {
        events << BeforeCardsMove;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
            && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
            const Card *anshen_card = move.reason.m_extraData.value<const Card *>();
            if (!anshen_card || !anshen_card->isKindOf("Slash") || !anshen_card->hasFlag("ikanshen"))
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *anshen_user = room->getTag("ikanshen_user").value<ServerPlayer *>();
        if (anshen_user) {
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(anshen_user))) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *anshen_user = room->getTag("ikanshen_user").value<ServerPlayer *>();
        room->removeTag("ikanshen_user");
        if (anshen_user) {
            const Card *anshen_card = move.reason.m_extraData.value<const Card *>();
            anshen_user->obtainCard(anshen_card);
            move.removeCardIds(move.card_ids);
            data = QVariant::fromValue(move);
        }
        return false;
    }
};

class IkAnshenRecord: public TriggerSkill {
public:
    IkAnshenRecord(): TriggerSkill("#ikanshen-record") {
        events << PreCardUsed << CardResponded << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if ((triggerEvent == PreCardUsed || triggerEvent == CardResponded) && player->getPhase() == Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                   card = response.m_card;
            }
            if (card && card->getHandlingMethod() == Card::MethodUse
                && player->getPhase() == Player::Play && player->getMark("ikanshen") == 0) {
                player->addMark("ikanshen");
                if (card->isKindOf("Slash")) {
                    QList<int> ids;
                    if (!card->isVirtualCard())
                        ids << card->getEffectiveId();
                    else if (card->subcardsLength() > 0)
                        ids = card->getSubcards();
                    if (!ids.isEmpty()) {
                        room->setCardFlag(card, "ikanshen");
                        room->setTag("ikanshen_user", QVariant::fromValue(player));
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Play)
                player->setMark("ikanshen", 0);
        }

        return QStringList();
    }
};

class IkJingmu: public TriggerSkill {
public:
    IkJingmu(): TriggerSkill("ikjingmu") {
        events << BeforeCardsMove;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.card_ids.isEmpty() || move.to_place != Player::DiscardPile) return QStringList();
        const Card *to_obtain = NULL;
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
            if (player == move.from)
                return QStringList();
            to_obtain = move.reason.m_extraData.value<const Card *>();
            if (!to_obtain || !to_obtain->isKindOf("Slash"))
                return QStringList();
        }
        if (to_obtain)
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
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *to_obtain = move.reason.m_extraData.value<const Card *>();
        room->obtainCard(player, to_obtain);
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);

        return false;
    }
};

IkDuanmengCard::IkDuanmengCard() {
    target_fixed = true;
}

void IkDuanmengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    QString kingdom = room->askForKingdom(source);
    room->setPlayerMark(source, "ikduanmeng_" + kingdom, 1);
    room->addPlayerMark(source, "@dream");
}

class IkDuanmengViewAsSkill: public OneCardViewAsSkill {
public:
    IkDuanmengViewAsSkill(): OneCardViewAsSkill("ikduanmeng") {
        filter_pattern = "Slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("IkDuanmengCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkDuanmengCard *card = new IkDuanmengCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class IkDuanmeng: public TriggerSkill {
public:
    IkDuanmeng(): TriggerSkill("ikduanmeng") {
        events << TargetSpecified << EventPhaseStart;
        view_as_skill = new IkDuanmengViewAsSkill;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill && use.card->isBlack())
                foreach (ServerPlayer *to, use.to) {
                    if (to == use.from) continue;
                    if (to->getMark("ikduanmeng_" + use.from->getKingdom()) > 0)
                        skill_list.insert(to, QStringList(objectName()));
                }
        } else {
            if (player->getPhase() == Player::RoundStart) {
                foreach (QString kingdom, Sanguosha->getKingdoms()) {
                    QString markname = "ikduanmeng_" + kingdom;
                    if (player->getMark(markname) > 0) {
                        room->removePlayerMark(player, "@dream", player->getMark(markname));
                        room->setPlayerMark(player, markname, 0);
                    }
                }
            }
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        player->drawCards(2, objectName());
        return false;
    }
};

IkFanzhongCard::IkFanzhongCard() {
}

bool IkFanzhongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty()) return false;
    QList<const Player *> players = Self->getAliveSiblings();
    players << Self;
    int max = -1000;
    foreach (const Player *p, players) {
        if (max < p->getHp()) max = p->getHp();
    }
    return to_select->getHp() == max;
}

void IkFanzhongCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->damage(DamageStruct("ikfanzhong", effect.from, effect.to));
}

class IkFanzhong: public OneCardViewAsSkill {
public:
    IkFanzhong(): OneCardViewAsSkill("ikfanzhong") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("IkFanzhongCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkFanzhongCard *first = new IkFanzhongCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class IkYuanyuan: public TriggerSkill {
public:
    IkYuanyuan(): TriggerSkill("ikyuanyuan") {
        events << DamageCaused;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = "#IkYuanyuan";
        log.from = player;
        log.arg = objectName();
        log.to << damage.to;
        room->sendLog(log);

        if (damage.to->getEquips().isEmpty() && damage.to->getJudgingArea().isEmpty())
            return true;
        int card_id = room->askForCardChosen(player, damage.to, "ej", objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
        room->obtainCard(player, Sanguosha->getCard(card_id), reason);
        return true;
    }
};

class IkShangshiShu: public TriggerSkill {
public:
    IkShangshiShu(): TriggerSkill("ikshangshishu") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        QList<ServerPlayer *> players = target->getRoom()->getAllPlayers();
        foreach (ServerPlayer *p, players) {
            if (p->isWounded())
                return TriggerSkill::triggerable(target) && target->getPhase() == Player::Draw;
        }
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        int n = 0;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isWounded())
                ++n;
        }

        player->setFlags(objectName());
        player->drawCards(n, objectName());
        return true;
    }
};

class IkShangshiShuTrigger: public TriggerSkill {
public:
    IkShangshiShuTrigger(): TriggerSkill("#ikshangshishu") {
        events << CardUsed << CardResponded;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player != NULL && player->isAlive() && player->hasFlag("ikshangshishu")) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (card && (card->getTypeId() == Card::TypeBasic || card->getTypeId() == Card::TypeTrick) && player->canDiscard(player, "he"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->sendCompulsoryTriggerLog(player, "ikshangshishu");
        room->broadcastSkillInvoke("ikshangshishu", 3);
        room->askForDiscard(player, objectName(), 1, 1, false, true, "@ikshangshishu-discard");
        return false;
    }
};

class IkChouhai : public TriggerSkill {
public:
    IkChouhai() : TriggerSkill("ikchouhai") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->isKongcheng();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        room->sendCompulsoryTriggerLog(player, objectName());
        room->broadcastSkillInvoke(objectName());

        DamageStruct damage = data.value<DamageStruct>();
        ++damage.damage;
        data = QVariant::fromValue(damage);

        return false;
    }
};

class IkJueche : public PhaseChangeSkill
{
public:
    IkJueche() : PhaseChangeSkill("ikjueche")
    {
    }

    virtual Frequency getFrequency(const Player *player) const
    {
        if (player && player->getSkillStep(objectName()) == 1)
            return NotFrequent;
        return Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && target->getHp() > 0;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (getFrequency(player) == Compulsory) {
            room->sendCompulsoryTriggerLog(player, objectName());
            room->broadcastSkillInvoke(objectName());
            return true;
        } else {
            if (player->askForSkillInvoke(objectName())) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->loseHp(player);
        player->drawCards(2, objectName());
        return false;
    }
};

class IkHewu: public TriggerSkill {
public:
    IkHewu(): TriggerSkill("ikhewu") {
        events << Dying;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->getMark("@hewu") == 0) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who == player && player->getHp() <= 0)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        LogMessage log;
        log.type = "#IkHewuWake";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());

        room->addPlayerMark(player, "@hewu");

        room->setPlayerProperty(player, "ikjueche_step", 1);

        if (room->changeMaxHpForAwakenSkill(player)) {
            room->recover(player, RecoverStruct(player, NULL, 2 - player->getHp()));
            room->handleAcquireDetachSkills(player, "ikjingnie|iktiaoxin");
        }
        return false;
    }
};

IkYuzhiCard::IkYuzhiCard()
{
    target_fixed = true;
}

void IkYuzhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    int n = subcardsLength() * 2;
    QList<int> card_ids = room->getNCards(n, false);
    CardMoveReason reason1(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "ikyuzhi", QString());
    CardsMoveStruct move(card_ids, NULL, Player::PlaceTable, reason1);
    room->moveCardsAtomic(move, true);
    room->getThread()->delay();
    room->getThread()->delay();
    
    DummyCard get;
    DummyCard thro;

    foreach (int id, card_ids) {
        if (Sanguosha->getCard(id)->isKindOf("TrickCard"))
            get.addSubcard(id);
        else
            thro.addSubcard(id);
    }

    if (get.subcardsLength() > 0)
        source->obtainCard(&get);

    if (thro.subcardsLength() > 0) {
        CardMoveReason reason2(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "ikyuzhi", QString());
        room->throwCard(&thro, reason2, NULL);
    }
}

class IkYuzhiVS : public ViewAsSkill
{
public:
    IkYuzhiVS() : ViewAsSkill("ikyuzhi")
    {
        response_pattern = "@@ikyuzhi";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return NULL;

        IkYuzhiCard *jq = new IkYuzhiCard;
        jq->addSubcards(cards);
        return jq;
    }
};

class IkYuzhi : public TriggerSkill
{
public:
    IkYuzhi() : TriggerSkill("ikyuzhi")
    {
        events << EventPhaseStart;
        view_as_skill = new IkYuzhiVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Play;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!player->canDiscard(player, "he"))
            return false;

        return room->askForUseCard(player, "@@ikyuzhi", "@ikyuzhi", -1, Card::MethodDiscard);
    }
};

class IkLinglong : public TriggerSkill
{
public:
    IkLinglong() : TriggerSkill("iklinglong")
    {
        events << GameStart << EventAcquireSkill;
        frequency = Compulsory;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == GameStart) {
            if (player) return QStringList();
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(Sanguosha->getSkill("eight_diagram"));
            room->getThread()->addTriggerSkill(trigger_skill);
        } else if (data.toString() == objectName()) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(Sanguosha->getSkill("eight_diagram"));
            room->getThread()->addTriggerSkill(trigger_skill);
        }
        return QStringList();
    }
};

class IkLinglongMax : public MaxCardsSkill
{
public:
    IkLinglongMax() : MaxCardsSkill("#iklinglong-horse")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill("iklinglong") && !target->getDefensiveHorse() && !target->getOffensiveHorse())
            return 1;
        return 0;
    }
};

class IkLinglongTreasure : public TriggerSkill
{
public:
    IkLinglongTreasure() : TriggerSkill("#iklinglong-treasure")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventLoseSkill && data.toString() == "iklinglong") {
            room->handleAcquireDetachSkills(player, "-thjizhi", true, true);
            player->setMark("iklinglong_thjizhi", 0);
        } else if ((triggerEvent == EventAcquireSkill && data.toString() == "iklinglong") || (triggerEvent == GameStart && TriggerSkill::triggerable(player))) {
            if (player->getTreasure() == NULL) {
                room->notifySkillInvoked(player, objectName());
                room->handleAcquireDetachSkills(player, "thjizhi", true, true);
                player->setMark("iklinglong_thjizhi", 1);
            }
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill("iklinglong", true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceEquip)) {
                if (!player->getTreasure() && player->getMark("iklinglong_thjizhi") == 0) {
                    room->notifySkillInvoked(player, "iklinglong");
                    room->handleAcquireDetachSkills(player, "thjizhi", true, true);
                    player->setMark("iklinglong_thjizhi", 1);
                }
            } else if (move.to == player && move.to_place == Player::PlaceEquip) {
                if (player->getTreasure() && player->getMark("iklinglong_thjizhi") == 1) {
                    room->handleAcquireDetachSkills(player, "-thjizhi", true, true);
                    player->setMark("iklinglong_thjizhi", 0);
                }
            }
        }
        return QStringList();
    }
};

class IkBashou: public TriggerSkill {
public:
    IkBashou(): TriggerSkill("ikbashou") {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        TriggerList skill;
        if (player->getPhase() != Player::Finish)
            return skill;
        foreach (ServerPlayer *yuejin, room->findPlayersBySkillName(objectName())) {
            if (yuejin == player) continue;
            if (yuejin->canDiscard(yuejin, "h"))
                skill.insert(yuejin, QStringList(objectName()));
        }
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *yuejin) const{
        if (room->askForCard(yuejin, ".Basic", "@ikbashou", QVariant(), objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *yuejin) const{
        if (!room->askForCard(player, "^BasicCard", "@ikbashou-discard"))
            room->damage(DamageStruct(objectName(), yuejin, player));
        return false;
    }
};

IkXinbanCard::IkXinbanCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkXinbanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    if (!targets.isEmpty())
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void IkXinbanCard::onUse(Room *room, const CardUseStruct &card_use) const{
    int index = -1;
    if (card_use.to.first() == card_use.from)
        index = 5;
    else if (card_use.to.first()->getGeneralName().contains("caocao"))
        index = 4;
    else {
        const Card *card = Sanguosha->getCard(card_use.card->getSubcards().first());
        if (card->isKindOf("Weapon"))
            index = 1;
        else if (card->isKindOf("Armor"))
            index = 2;
        else if (card->isKindOf("Horse"))
            index = 3;
    }
    room->broadcastSkillInvoke("ikxinban", index);
    SkillCard::onUse(room, card_use);
}

void IkXinbanCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *caohong = effect.from;
    Room *room = caohong->getRoom();
    room->moveCardTo(this, caohong, effect.to, Player::PlaceEquip,
                     CardMoveReason(CardMoveReason::S_REASON_PUT, caohong->objectName(), "ikxinban", QString()));

    const Card *card = Sanguosha->getCard(subcards.first());

    LogMessage log;
    log.type = "$IkJibanEquip";
    log.from = effect.to;
    log.card_str = QString::number(card->getEffectiveId());
    room->sendLog(log);

    if (card->isKindOf("Weapon")) {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if ((effect.to == p || effect.to->inMyAttackRange(p)) && caohong->canDiscard(p, "hej"))
                targets << p;
        }
        if (!targets.isEmpty()) {
            ServerPlayer *to_dismantle = room->askForPlayerChosen(caohong, targets, "ikxinban", "@ikxinban-discard:" + effect.to->objectName());
            int card_id = room->askForCardChosen(caohong, to_dismantle, "hej", "ikxinban", false, Card::MethodDiscard);
            room->throwCard(Sanguosha->getCard(card_id), to_dismantle, caohong);
        }
    } else if (card->isKindOf("Armor")) {
        effect.to->drawCards(2, "ikxinban");
    } else if (card->isKindOf("Horse")) {
        room->recover(effect.to, RecoverStruct(effect.from));
    }
}

class IkXinban: public OneCardViewAsSkill {
public:
    IkXinban(): OneCardViewAsSkill("ikxinban") {
        filter_pattern = "EquipCard";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkXinbanCard *first = new IkXinbanCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkXinbanCard") && player->canDiscard(player, "he");
    }
};

IkHuyinCard::IkHuyinCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void IkHuyinCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->addToPile("rhythm", this);
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.to, "@arc");
    room->addPlayerMark(effect.to, "huyin_" + effect.from->objectName());
}

class IkHuyin: public OneCardViewAsSkill
{
public:
    IkHuyin(): OneCardViewAsSkill("ikhuyin") {
        filter_pattern = ".";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkHuyinCard *first = new IkHuyinCard;
        first->addSubcard(originalcard);
        return first;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkHuyinCard") && !player->isNude();
    }
};

class IkHuyinTrigger: public TriggerSkill {
public:
    IkHuyinTrigger(): TriggerSkill("#ikhuyin") {
        events << CardUsed << CardResponded << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        QStringList type_list;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill)
                type_list << use.card->getType();
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->getTypeId() != Card::TypeSkill)
                type_list << resp.m_card->getType();
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                int index = 0;
                foreach (int id, move.card_ids) {
                    if (move.from_places[index] == Player::PlaceHand || move.from_places[index] == Player::PlaceEquip) {
                        const Card *card = Sanguosha->getCard(id);
                        type_list << card->getType();
                    }
                    ++index;
                }
            }
        }
        if (!type_list.isEmpty()) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (player->getMark("huyin_" + p->objectName()) > 0) {
                    foreach (int id, p->getPile("rhythm")) {
                        if (type_list.contains(Sanguosha->getCard(id)->getType())) {
                            skill_list.insert(p, QStringList(objectName()));
                            break;
                        }
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const{
        room->sendCompulsoryTriggerLog(ask_who, objectName());

        room->removePlayerMark(player, "huyin_" + ask_who->objectName());
        room->removePlayerMark(player, "@arc");

        DummyCard *dummy = new DummyCard(ask_who->getPile("rhythm"));
        player->obtainCard(dummy);
        delete dummy;
        room->loseHp(player);

        return false;
    }
};

class IkHuyinClear: public TriggerSkill {
public:
    IkHuyinClear(): TriggerSkill("#ikhuyin-clear") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseStart && target->getPhase() == Player::RoundStart && !target->getPile("rhythm").isEmpty()) {
            DummyCard *dummy = new DummyCard(target->getPile("rhythm"));
            target->obtainCard(dummy);
            delete dummy;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("huyin_" + target->objectName()) > 0) {
                    int n = p->getMark("huyin_" + target->objectName());
                    room->removePlayerMark(p, "@arc", n);
                    room->setPlayerMark(p, "huyin_" + target->objectName(), 0);
                }
            }
        }

        return QStringList();
    }
};

class IkHongcai: public TriggerSkill {
public:
    IkHongcai(): TriggerSkill("ikhongcai") {
        events << TargetSpecified;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        TriggerList skill;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard") && use.to.length() > 1) {
            foreach (ServerPlayer *yangxiu, room->findPlayersBySkillName(objectName()))
                skill.insert(yangxiu, QStringList(objectName()));
        }
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *yangxiu) const{
        if (yangxiu->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *player) const{
        player->drawCards(1, objectName());
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.contains(player)) {
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

IkXunyuyouliCard::IkXunyuyouliCard()
{
}

bool IkXunyuyouliCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.length() < 3;
}

void IkXunyuyouliCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    QStringList choices;
    if (!effect.to->hasSkill("thfeiying"))
        choices << "thfeiying";
    if (!effect.to->hasSkill("rhlingwei"))
        choices << "rhlingwei";
    if (!effect.to->hasSkill("ikzhichi"))
        choices << "ikzhichi";
    if (choices.isEmpty())
        return;
    room->setPlayerProperty(effect.to, "ikxunyuyouli_from", QVariant::fromValue(effect.from->objectName()));
    QString choice = room->askForChoice(effect.from, "ikxunyuyouli", choices.join("+"));
    effect.to->tag["IkXunyuyouli_" + effect.from->objectName()] = choice;
    room->acquireSkill(effect.to, choice);
}

class IkXunyuyouliViewAsSkill : public ZeroCardViewAsSkill
{
public:
    IkXunyuyouliViewAsSkill() : ZeroCardViewAsSkill("ikxunyuyouli")
    {
        response_pattern = "@@ikxunyuyouli";
    }

    virtual const Card *viewAs() const
    {
        return new IkXunyuyouliCard;
    }
};

class IkXunyuyouli : public TriggerSkill
{
public:
    IkXunyuyouli() : TriggerSkill("ikxunyuyouli")
    {
        events << EventPhaseChanging << Death << EventPhaseStart;
        view_as_skill = new IkXunyuyouliViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(objectName(), true))
                return QStringList();
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->property("ikxunyuyouli_from").toString() == player->objectName()) {
                    room->setPlayerProperty(p, "ikxunyuyouli_from", QVariant());
                    QString skill_name = p->tag["IkXunyuyouli_" + player->objectName()].toString();
                    p->tag["IkXunyuyouli_" + player->objectName()] = QString();
                    if (!skill_name.isEmpty())
                        room->detachSkillFromPlayer(p, skill_name, false, true);
                }
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->property("ikxunyuyouli_from").toString() == player->objectName()) {
                        room->setPlayerProperty(p, "ikxunyuyouli_from", QVariant());
                        QString skill_name = p->tag["IkXunyuyouli_" + player->objectName()].toString();
                        p->tag["IkXunyuyouli_" + player->objectName()] = QString();
                        if (!skill_name.isEmpty())
                            room->detachSkillFromPlayer(p, skill_name, false, true);
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
            if (!TriggerSkill::triggerable(player))
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(player, "@@ikxunyuyouli", "@ikxunyuyouli");
        return false;
    }
};

class IkTianzuoyounai: public TriggerSkill {
public:
    IkTianzuoyounai(): TriggerSkill("iktianzuoyounai") {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        player->drawCards(3, objectName());
        return false;
    }
};

class IkShenji: public PhaseChangeSkill {
public:
    IkShenji(): PhaseChangeSkill("ikshenji") {
        frequency = Limited;
        limit_mark = "@shenji";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("@shenji") > 0;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->removePlayerMark(player, "@shenji");
        room->addPlayerMark(player, "@shenjiused");

        player->drawCards(3, objectName());

        QList<const Card *> tricks = player->getJudgingArea();
        foreach (const Card *trick, tricks) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName());
            room->throwCard(trick, reason, NULL);
        }

        return false;
    }
};

class IkKuanglu: public PhaseChangeSkill {
public:
    IkKuanglu(): PhaseChangeSkill("ikkuanglu") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Play
            && target->isWounded();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        player->drawCards(1, objectName());
        room->setPlayerFlag(player, "IkKuangluInvoke");
        return false;
    }
};

class IkKuangluRecord: public TriggerSkill {
public:
    IkKuangluRecord(): TriggerSkill("#ikkuanglu-record") {
        events << PreDamageDone << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, "ikkuanglu", 0);
            }
            return QStringList();
        }
        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        room->addPlayerMark(damage.from, "ikkuanglu", damage.damage);
        return false;
    }
};

class IkKuangluMaxCards: public MaxCardsSkill {
public:
    IkKuangluMaxCards(): MaxCardsSkill("#ikkuanglu") {
    }

    virtual int getFixed(const Player *target) const{
        if (target->hasFlag("IkKuangluInvoke"))
            return qMin(target->getMark("ikkuanglu"), 4);
        return -1;
    }
};

class IkBenhua: public TriggerSkill {
public:
    IkBenhua(): TriggerSkill("ikbenhua") {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                int dis = player->distanceTo(to);
                if (dis != -1 && dis <= 1) {
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *to, use.to) {
            int dis = player->distanceTo(to);
            if (dis != -1 && dis <= 1 && TriggerSkill::triggerable(player)) {
                if (!room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to))) continue;
                room->broadcastSkillInvoke(objectName());
                if (!player->isNude() && player != to) {
                    const Card *card = NULL;
                    if (player->getCardCount() > 1) {
                        card = room->askForCard(player, "..!", "@ikbenhua-give:" + to->objectName(), data, Card::MethodNone);
                        if (!card)
                            card = player->getCards("he").at(qrand() % player->getCardCount());
                    } else {
                        Q_ASSERT(player->getCardCount() == 1);
                        card = player->getCards("he").first();
                    }
                    to->obtainCard(card);
                    if (card->getTypeId() == Card::TypeEquip && room->getCardOwner(card->getEffectiveId()) == to
                        && !to->isLocked(card))
                        if (room->askForSkillInvoke(to, "ikbenhua_use", "use"))
                            room->useCard(CardUseStruct(card, to, to));
                }
                player->drawCards(1, objectName());
            }
        }

        return false;
    }
};

IkYaochengCard::IkYaochengCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkYaochengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    if (!targets.isEmpty())
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void IkYaochengCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *caohong = effect.from;
    Room *room = caohong->getRoom();
    room->moveCardTo(this, caohong, effect.to, Player::PlaceEquip,
                     CardMoveReason(CardMoveReason::S_REASON_PUT, caohong->objectName(), "ikyaocheng", QString()));

    const Card *card = Sanguosha->getCard(subcards.first());

    LogMessage log;
    log.type = "$IkJibanEquip";
    log.from = effect.to;
    log.card_str = QString::number(card->getEffectiveId());
    room->sendLog(log);

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (effect.to->distanceTo(p) == 2 && caohong->canDiscard(p, "hej"))
            targets << p;
    }
    if (!targets.isEmpty()) {
        ServerPlayer *to_dismantle = room->askForPlayerChosen(caohong, targets, "ikyaocheng", "@ikyaocheng-discard:" + effect.to->objectName());
        int card_id = room->askForCardChosen(caohong, to_dismantle, "he", "ikyaocheng", false, Card::MethodDiscard);
        room->throwCard(Sanguosha->getCard(card_id), to_dismantle, caohong);
    }
}

class IkYaochengViewAsSkill: public OneCardViewAsSkill {
public:
    IkYaochengViewAsSkill(): OneCardViewAsSkill("ikyaocheng") {
        filter_pattern = "EquipCard";
        response_pattern = "@@ikyaocheng";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkYaochengCard *first = new IkYaochengCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class IkYaocheng: public PhaseChangeSkill {
public:
    IkYaocheng(): PhaseChangeSkill("ikyaocheng") {
        view_as_skill = new IkYaochengViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Discard
            && !target->isNude();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer *) const{
        return room->askForUseCard(target, "@@ikyaocheng", "@ikyaocheng-equip", -1, Card::MethodNone);
    }

    virtual bool onPhaseChange(ServerPlayer *) const{
        return false;
    }
};

IkHaobiCard::IkHaobiCard() {
}

bool IkHaobiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.length() < 2;
}

bool IkHaobiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

void IkHaobiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;

    LogMessage log;
    log.from = use.from;
    log.to << card_use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardUsed, room, use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, use.from, data);
}

void IkHaobiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    source->setFlags("ikhaobi");
    source->tag["IkHaobiSource"] = true;
    QList<ServerPlayer *> players = room->getAllPlayers();
    int index1 = players.indexOf(targets.first()), index2 = players.indexOf(targets.last());
    int index_self = players.indexOf(source);
    QList<ServerPlayer *> cont_targets;
    if (index1 == index_self || index2 == index_self) {
        forever {
            cont_targets.append(players.at(index1));
            if (index1 == index2) break;
            index1++;
            if (index1 >= players.length())
                index1 -= players.length();
        }
    } else {
        if (index1 > index2)
            qSwap(index1, index2);
        if (index_self > index1 && index_self < index2) {
            for (int i = index1; i <= index2; i++)
                cont_targets.append(players.at(i));
        } else {
            forever {
                cont_targets.append(players.at(index2));
                if (index1 == index2) break;
                index2++;
                if (index2 >= players.length())
                    index2 -= players.length();
            }
        }
    }
    cont_targets.removeOne(source);
    QStringList list;
    foreach(ServerPlayer *p, cont_targets) {
        if (!p->isAlive()) continue;
        list.append(p->objectName());
        source->tag["ikhaobi"] = QVariant::fromValue(list);
        QStringList p_from = p->tag["ikhaobi_source"].toStringList();
        p_from.append(source->objectName());
        p->tag["ikhaobi_source"] = QVariant::fromValue(p_from);
        if (!p->hasSkill("thfeiying"))
            room->acquireSkill(p, "thfeiying");
    }
}

class IkHaobiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkHaobiViewAsSkill(): ZeroCardViewAsSkill("ikhaobi") {
        response_pattern = "@@ikhaobi";
    }

    virtual const Card *viewAs() const{
        return new IkHaobiCard;
    }
};

class IkHaobi: public TriggerSkill {
public:
    IkHaobi(): TriggerSkill("ikhaobi") {
        events << EventPhaseChanging << Death;
        view_as_skill = new IkHaobiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        }
        if (!player->hasFlag("ikhaobi") && player->tag["IkHaobiSource"].toBool()) {
            player->tag["IkHaobiSource"] = false;
            QStringList list = player->tag[objectName()].toStringList();
            player->tag.remove(objectName());
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (list.contains(p->objectName())) {
                    QStringList p_from = p->tag["ikhaobi_source"].toStringList();
                    p_from.removeOne(player->objectName());
                    p->tag["ikhaobi_source"] = QVariant::fromValue(p_from);
                    if (p_from.isEmpty())
                        room->detachSkillFromPlayer(p, "thfeiying", false, true);
                }
            }
        }
        if (TriggerSkill::triggerable(player) && triggerEvent == EventPhaseChanging)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        return room->askForUseCard(player, "@@ikhaobi", "@ikhaobi");
    }
};

class IkLingxue: public MasochismSkill {
public:
    IkLingxue(): MasochismSkill("iklingxue") {
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        QStringList skill;
        ServerPlayer *current = room->getCurrent();
        if (!current || current->isDead() || current->getPhase() == Player::NotActive)
            return skill;
        for (int i = 1; i <= damage.damage; i++)
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *current = room->getCurrent();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(current))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &) const{
        Room *room = target->getRoom();
        ServerPlayer *current = room->getCurrent();
        room->addPlayerMark(current, "@blizzard");
        current->setFlags("lingxue_" + target->objectName());
    }
};

class IkLingxueDraw: public TriggerSkill {
public:
    IkLingxueDraw(): TriggerSkill("#iklingxue-draw") {
        events << TurnStart << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == TurnStart) {
            room->setPlayerMark(player, "@blizzard", 0);
            player->setMark("iklingxue_discard", 0);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && player == move.from && player->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                skill_list.insert(player, QStringList(objectName()));
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return skill_list;
            foreach (ServerPlayer *zangba, room->findPlayersBySkillName("iklingxue"))
                if (player->hasFlag("lingxue_" + zangba->objectName())) {
                    if (player->getMark("iklingxue_discard") == 0)
                        skill_list.insert(zangba, QStringList(objectName()));
                }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ask_who->addMark("iklingxue_discard", move.card_ids.length());
        } else if (triggerEvent == EventPhaseChanging) {
            int n = qMin(player->getMark("@blizzard"), 2);
            room->setPlayerMark(player, "@blizzard", 0);
            LogMessage log;
            log.type = "#IkLingxueDraw";
            log.from = player;
            log.to << ask_who;
            log.arg = "iklingxue";
            log.arg2 = QString::number(player->getMark("iklingxue_discard"));
            room->sendLog(log);
            QStringList choices;
            if (!player->isNude())
                choices << "obtain";
            choices << "draw";
            for (int i = 0; i < n; ++i) {
                QString choice = room->askForChoice(ask_who, "iklingxue", choices.join("+"));
                if (choice == "obtain") {
                    int id = room->askForCardChosen(ask_who, player, "he", "iklingxue", false, Card::MethodNone);
                    room->obtainCard(ask_who, id, false);
                } else
                    ask_who->drawCards(1, "iklingxue");
                choices.removeOne(choice);
                if (player->isNude())
                    choices.removeOne("obtain");
                if (choices.isEmpty())
                    break;
            }
            player->setMark("iklingxue_discard", 0);
        }
        return false;
    }
};

class IkLingxueMaxCards: public MaxCardsSkill {
public:
    IkLingxueMaxCards(): MaxCardsSkill("#iklingxue-maxcard") {
    }

    virtual int getExtra(const Player *target) const{
        return -target->getMark("@blizzard");
    }
};

class IkGonghu: public TriggerSkill {
public:
    IkGonghu(): TriggerSkill("ikgonghu") {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);

        LogMessage log2;
        log2.type = "#GainMaxHp";
        log2.from = player;
        log2.arg = "1";
        room->sendLog(log2);

        if (player->isWounded()) {
            room->recover(player, RecoverStruct(player));
        } else {
            LogMessage log3;
            log3.type = "#GetHp";
            log3.from = player;
            log3.arg = QString::number(player->getHp());
            log3.arg2 = QString::number(player->getMaxHp());
            room->sendLog(log3);
        }
        return false;
    }
};

class IkXuewu: public TriggerSkill {
public:
    IkXuewu(): TriggerSkill("ikxuewu") {
        events << MaxHpChanged << HpLost;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (player->canDiscard(p, "he"))
                targets << p;
        ServerPlayer *target = NULL;
        if (!targets.isEmpty())
            target = room->askForPlayerChosen(player, targets, objectName(), "@ikxuewu", true, true);
        if (target) {
            int card_id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, target, player);
        } else
            player->drawCards(1, objectName());
        return false;
    }
};

class IkQingwei: public TriggerSkill {
public:
    IkQingwei(): TriggerSkill("ikqingwei") {
        events << TargetConfirming << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") || (use.card->getTypeId() == Card::TypeTrick && use.card->isBlack())) {
                if (use.to.length() == 1) {
                    foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                        if (owner->getHp() > player->getHp())
                            skill_list.insert(owner, QStringList(objectName()));
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->tag["ikqingwei_cards"].toList().isEmpty()) {
                        DummyCard *dummy = new DummyCard(VariantList2IntList(p->tag["ikqingwei_cards"].toList()));
                        p->obtainCard(dummy);
                        delete dummy;
                        p->tag.remove("ikqingwei_cards");
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *owner) const{
        if (room->askForCard(owner, "..", "@ikqingwei", data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *owner) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QString choice = room->askForChoice(owner, objectName(), "null+draw");
        if (choice == "null") {
            use.nullified_list << "_ALL_TARGETS";
            QList<int> card_ids;
            if (!use.card->isVirtualCard())
                card_ids << use.card->getId();
            else
                card_ids = use.card->getSubcards();
            if (!card_ids.isEmpty()) {
                owner->addToPile("endure", use.card);
                if (use.from) {
                    QVariantList ikqingwei_cards = use.from->tag["ikqingwei_cards"].toList();
                    ikqingwei_cards << IntList2VariantList(card_ids);
                    use.from->tag["ikqingwei_cards"] = QVariant::fromValue(ikqingwei_cards);
                }
            }
            data = QVariant::fromValue(use);
        } else {
            owner->drawCards(1, objectName());
            if (use.from->isProhibited(owner, use.card))
                return false;
            if (use.card->isKindOf("Slash")) {
                if (!use.from->canSlash(owner, use.card, false))
                    return false;
            } else if (use.card->isKindOf("Collateral")) {
                QList<ServerPlayer *> victims;
                foreach (ServerPlayer *p, room->getOtherPlayers(owner)) {
                    if (owner->canSlash(p))
                        victims << p;
                }
                if (victims.isEmpty())
                    return false;
                ServerPlayer *victim = room->askForPlayerChosen(use.from, victims, objectName(), "@dummy-slash2:" + owner->objectName());
                use.to.first()->tag.remove("collateralVictim");
                owner->tag["collateralVictim"] = QVariant::fromValue(victim);
                
                LogMessage log;
                log.type = "#CollateralSlash";
                log.from = use.from;
                log.to << victim;
                room->sendLog(log);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, owner->objectName(), victim->objectName());
            } else if (use.card->isKindOf("FeintAttack")) {
                QList<ServerPlayer *> victims = room->getOtherPlayers(owner);
                victims.removeOne(use.from);
                if (!victims.isEmpty()) {
                    ServerPlayer *victim = room->askForPlayerChosen(use.from, victims, objectName(), "@feint-attack:" + owner->objectName());
                    use.to.first()->tag.remove("feintTarget");
                    owner->tag["feintTarget"] = QVariant::fromValue(victim);

                    LogMessage log;
                    log.type = "#FeintAttackWest";
                    log.from = use.from;
                    log.to << victim;
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, owner->objectName(), victim->objectName());
                }
            }

            if (use.card->isKindOf("DelayedTrick"))
                room->moveCardTo(use.card, owner, Player::PlaceDelayedTrick, true);
            else {
                use.to.clear();
                use.to << owner;
            }
            data = QVariant::fromValue(use);
            return true;
        }
        return false;
    }
};

IkZhiyuCard::IkZhiyuCard() {
    target_fixed = true;
}

void IkZhiyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->loseHp(source);
    if (source->isDead())
        return;
    QString choice = room->askForChoice(source, "ikzhiyu", "basic+trick+equip");
    if (choice == "basic") {
        room->setPlayerFlag(source, "IkZhiyu1");
        // update Dialog
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    } else if (choice == "trick") {
        source->drawCards(2, "ikzhiyu");
        room->setPlayerFlag(source, "IkZhiyu2");
    } else if (choice == "equip") {
        source->drawCards(1, "ikzhiyu");
        if (room->askForCard(source, "EquipCard", "@ikzhiyu-equip", QVariant(), "ikzhiyu"))
            room->setPlayerFlag(source, "IkZhiyu3");
    }
}

IkZhiyuBasicCard::IkZhiyuBasicCard() {
    will_throw = false;
    m_skillName = "ikzhiyu";
}

bool IkZhiyuBasicCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcard(this);
            card->deleteLater();
        }
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;

    const Card *card = Self->tag.value("ikzhiyu").value<const Card *>();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool IkZhiyuBasicCard::targetFixed() const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcard(this);
            card->deleteLater();
        }
        return card && card->targetFixed();
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = Self->tag.value("ikzhiyu").value<const Card *>();
    return card && card->targetFixed();
}

bool IkZhiyuBasicCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            card = Sanguosha->cloneCard(user_string.split("+").first());
            card->addSubcard(this);
            card->deleteLater();
        }
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;

    const Card *card = Self->tag.value("ikzhiyu").value<const Card *>();
    return card && card->targetsFeasible(targets, Self);
}

const Card *IkZhiyuBasicCard::validate(CardUseStruct &card_use) const{
    ServerPlayer *wenyang = card_use.from;
    Room *room = wenyang->getRoom();

    QString to_iklixin = user_string;
    if (user_string == "slash"
        && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QStringList iklixin_list;
            iklixin_list << "slash";
            if (!ServerInfo.Extensions.contains("!maneuvering"))
                iklixin_list << "thunder_slash" << "fire_slash";
            to_iklixin = room->askForChoice(wenyang, "ikzhiyu_slash", iklixin_list.join("+"));
    }

    Card *use_card = Sanguosha->cloneCard(to_iklixin);
    use_card->addSubcard(this);
    use_card->setSkillName("ikzhiyu");
    return use_card;
}

const Card *IkZhiyuBasicCard::validateInResponse(ServerPlayer *wenyang) const{
    Room *room = wenyang->getRoom();

    QString to_iklixin;
    if (user_string == "peach+analeptic") {
        QStringList iklixin_list;
        iklixin_list << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            iklixin_list << "analeptic";
        to_iklixin = room->askForChoice(wenyang, "ikzhiyu_saveself", iklixin_list.join("+"));
    } else if (user_string == "slash") {
        QStringList iklixin_list;
        iklixin_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            iklixin_list << "thunder_slash" << "fire_slash";
        to_iklixin = room->askForChoice(wenyang, "ikzhiyu_slash", iklixin_list.join("+"));
    } else
        to_iklixin = user_string;

    Card *use_card = Sanguosha->cloneCard(to_iklixin);
    use_card->addSubcard(this);
    use_card->setSkillName("ikxieke");
    return use_card;
}

class IkZhiyuViewAsSkill: public ViewAsSkill {
public:
    IkZhiyuViewAsSkill(): ViewAsSkill("ikzhiyu") {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (!Self->hasFlag("IkZhiyu1"))
            return false;
        return selected.isEmpty() && to_select->getTypeId() == Card::TypeBasic;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (!player->hasFlag("IkZhiyu1")) return false;
        if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic") || pattern == "jink" || pattern == "slash")
            return true;
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (!Self->hasFlag("IkZhiyu1")) {
            if (cards.isEmpty())
                return new IkZhiyuCard;
            else
                return NULL;
        }
        if (cards.length() != 1)
            return NULL;

        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
            IkZhiyuBasicCard *tianyan_card = new IkZhiyuBasicCard;
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";
            tianyan_card->addSubcards(cards);
            tianyan_card->setUserString(pattern);
            return tianyan_card;
        }

        const Card *c = Self->tag["ikzhiyu"].value<const Card *>();
        if (c) {
            IkZhiyuBasicCard *card = new IkZhiyuBasicCard;
            card->addSubcards(cards);
            card->setUserString(c->objectName());
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (!player->hasUsed("IkZhiyuCard"))
            return player->canDiscard(player, "he");

        if (!player->hasFlag("IkZhiyu1"))
            return false;

        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        if (slash->isAvailable(player))
            return true;

        Peach *peach = new Peach(Card::NoSuit, 0);
        peach->deleteLater();
        if (peach->isAvailable(player))
            return true;

        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->deleteLater();
        if (analeptic->isAvailable(player))
            return true;

        return false;
    }
};

class IkZhiyu: public TriggerSkill {
public:
    IkZhiyu(): TriggerSkill("ikzhiyu") {
        events << TargetSpecified << EventPhaseChanging;
        view_as_skill = new IkZhiyuViewAsSkill;
    }

    virtual QDialog *getDialog() const{
        if (Self->hasFlag("IkZhiyu1"))
            return ThMimengDialog::getInstance("ikzhiyu", true, false);
        return NULL;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive && player->hasFlag("IkZhiyu1")) {
                room->setPlayerFlag(player, "-IkZhiyu1");
                // update Dialog
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }
            return QStringList();
        }
        if (!player || player->isDead() || !player->hasFlag("IkZhiyu3"))
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to) {
            if (!player->canDiscard(p, "he"))
                continue;
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->setPlayerFlag(p, "ikzhiyu_InTempMoving");
                DummyCard *dummy = new DummyCard;
                QList<int> card_ids;
                QList<Player::Place> original_places;
                for (int i = 0; i < 2; i++) {
                    if (!player->canDiscard(p, "he"))
                        break;
                    card_ids << room->askForCardChosen(player, p, "he", "ikzhiyu", false, Card::MethodDiscard);
                    original_places << room->getCardPlace(card_ids[i]);
                    dummy->addSubcard(card_ids[i]);
                    p->addToPile("#ikzhiyu", card_ids[i], false);
                }
                for (int i = 0; i < dummy->subcardsLength(); i++)
                    room->moveCardTo(Sanguosha->getCard(card_ids[i]), p, original_places[i], false);
                room->setPlayerFlag(p, "-ikzhiyu_InTempMoving");
                if (dummy->subcardsLength() > 0)
                    room->throwCard(dummy, p, player);
                delete dummy;
            }
        }
        return false;
    }
};

class IkZhiyuTargetMod: public TargetModSkill {
public:
    IkZhiyuTargetMod(): TargetModSkill("#ikzhiyu-tar") {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const {
        if (from->hasFlag("IkZhiyu2"))
            return 1000;
        else
            return 0;
    }
};

class IkChenqing : public TriggerSkill
{
public:
    IkChenqing() : TriggerSkill("ikchenqing")
    {
        events << AskForPeaches << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart)
                player->setMark(objectName(), 0);
        } else {
            if (TriggerSkill::triggerable(player) && player->getMark(objectName()) == 0) {
                DyingStruct dying = data.value<DyingStruct>();
                if (dying.who->isAlive() && dying.who->getHp() < 1) {
                    if (dying.who == player || room->alivePlayerCount() > 2)
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        QList<ServerPlayer *> players = room->getOtherPlayers(player);
        players.removeAll(dying.who);
        ServerPlayer *target = room->askForPlayerChosen(player, players, objectName(), "@ikchenqing", true, true);
        if (target) {
            player->tag["IkChenqingTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag["IkChenqingTarget"].value<ServerPlayer *>();
        player->tag.remove("IkChenqingTarget");
        if (target) {
            player->addMark(objectName());
            target->drawCards(4, objectName());
            if (!target->isNude()) {
                const Card *card = room->askForExchange(target, "ikchenqing", 4, 4, true, "@ikchenqing-discard");
                if (!card) {
                    DummyCard *dummy = new DummyCard;
                    QList<const Card *> cards = target->getCards("he");
                    dummy->addSubcards(cards.mid(0, 4));
                    card = dummy;
                }
                QSet<Card::Suit> suit;
                foreach (int id, card->getSubcards()) {
                    const Card *c = Sanguosha->getCard(id);
                    if (!c)
                        continue;
                    suit << c->getSuit();
                }
                room->throwCard(card, target);
                delete card;

                DyingStruct dying = data.value<DyingStruct>();
                if (suit.count() == 4 && room->getCurrentDyingPlayer() == dying.who) {
                    Peach *peach = new Peach(Card::NoSuit, 0);
                    peach->setSkillName("_ikchenqing");
                    bool jiaxu = false;
                    ServerPlayer *current = room->getCurrent();
                    if (current && current->isAlive() && current->getPhase() != Player::NotActive
                            && current->hasSkill("iksishideng") && current != target && dying.who != target)
                        jiaxu = true;
                    if (jiaxu || target->isCardLimited(peach, Card::MethodUse)
                            || target->isProhibited(dying.who, peach)) {
                        delete peach;
                        return false;
                    }
                    room->useCard(CardUseStruct(peach, target, dying.who));
                }
            }
        }
        return false;
    }
};

class IkMojingViewAsSkill : public OneCardViewAsSkill
{
public:
    IkMojingViewAsSkill() : OneCardViewAsSkill("ikmojing")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
        response_pattern = "@@ikmojing";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        const Card *ori = Self->tag[objectName()].value<const Card *>();
        if (ori == NULL) return NULL;
        if (Self->property("allowed_thmimeng_dialog_buttons").toString().split("+").contains(ori->objectName())) {
            Card *a = Sanguosha->cloneCard(ori->objectName());
            a->addSubcard(originalCard);
            a->setSkillName(objectName());
            return a;
        }
        return NULL;
    }
};

class IkMojing : public TriggerSkill
{
public:
    IkMojing() : TriggerSkill("ikmojing")
    {
        view_as_skill = new IkMojingViewAsSkill;
        events << EventPhaseStart << PreCardUsed << EventPhaseChanging;
    }

    virtual QDialog *getDialog() const
    {
        return ThMimengDialog::getInstance(objectName(), true, true, false, false, true);
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive)
                player->tag.remove("ikmojing_list");
        } else if (triggerEvent == PreCardUsed) {
            if (player->getPhase() == Player::Play) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->getTypeId() != Card::TypeEquip && use.card->getTypeId() != Card::TypeSkill) {
                    QStringList list = player->tag["ikmojing_list"].toStringList();
                    list << use.card->objectName();
                    player->tag["ikmojing_list"] = list;
                }
            }
        } else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish) {
            QStringList list = player->tag["ikmojing_list"].toStringList();
            if (!list.isEmpty())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QStringList list = player->tag["ikmojing_list"].toStringList();
        try {
            if (!list.isEmpty()) {
                room->setPlayerProperty(player, "allowed_thmimeng_dialog_buttons", list.join("+"));
                const Card *card = room->askForUseCard(player, "@@ikmojing", "@ikmojing_ask_first");
                if (card) {
                    list.removeAll(card->objectName());
                    if (!list.isEmpty()) {
                        room->setPlayerProperty(player, "allowed_thmimeng_dialog_buttons", list.join("+"));
                        room->askForUseCard(player, "@@ikmojing", "@ikmojing_ask_second");
                    }
                }
            }
        }
        catch (TriggerEvent e) {
            if (e == TurnBroken || e == StageChange) {
                room->setPlayerProperty(player, "allowed_thmimeng_dialog_buttons", QString());
            }
            throw e;
        }
        return false;
    }
};

IkFenxunCard::IkFenxunCard() {
}

bool IkFenxunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void IkFenxunCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    effect.from->tag["IkFenxunTarget"] = QVariant::fromValue(effect.to);
    room->setFixedDistance(effect.from, effect.to, 1);
    if (Sanguosha->getCard(subcards.first())->getTypeId() == TypeBasic)
        effect.from->setFlags("IkFenxunSlash");
}

class IkFenxunViewAsSkill: public OneCardViewAsSkill {
public:
    IkFenxunViewAsSkill(): OneCardViewAsSkill("ikfenxun") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("IkFenxunCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkFenxunCard *first = new IkFenxunCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class IkFenxun: public TriggerSkill {
public:
    IkFenxun(): TriggerSkill("ikfenxun") {
        events << EventPhaseChanging << Death << PreCardUsed << CardsMoveOneTime;
        view_as_skill = new IkFenxunViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *dingfeng, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging || triggerEvent == Death) {
            if (!dingfeng->tag["IkFenxunTarget"].value<ServerPlayer *>())
                return QStringList();
            if (triggerEvent == EventPhaseChanging) {
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.to != Player::NotActive)
                    return QStringList();
            } else if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != dingfeng)
                    return QStringList();
            }
            ServerPlayer *target = dingfeng->tag["IkFenxunTarget"].value<ServerPlayer *>();
            dingfeng->tag.remove("IkFenxunTarget");
            if (target)
                room->removeFixedDistance(dingfeng, target, 1);
        } else {
            if (triggerEvent == PreCardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->isKindOf("Slash") && dingfeng->hasFlag("IkFenxunSlash")) {
                    dingfeng->setFlags("-IkFenxunSlash");
                    use.card->setFlags("ikfenxun");
                }
            } else {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.to_place == Player::DiscardPile && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE) {
                    const Card *card = move.reason.m_extraData.value<const Card *>();
                    if (card->isKindOf("Slash") && card->hasFlag("ikfenxun") && dingfeng->hasSkill(objectName()))
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *dingfeng, QVariant &, ServerPlayer *) const
    {
        if (dingfeng->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *dingfeng, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *card = move.reason.m_extraData.value<const Card *>();
        dingfeng->obtainCard(card);
        move.removeCardIds(move.card_ids);
        data = QVariant::fromValue(move);
        return false;
    }
};

class OthersNullifiedDistance: public DistanceSkill {
public:
    OthersNullifiedDistance(): DistanceSkill("others-nullified-distance"){
    }

    virtual int getCorrect(const Player *from, const Player *to) const {
        int n = 0;
        bool invoke = false, including_horse = false;
        if (from->hasSkill("ikyixiang") && to->hasFlag("ikyixiang")) {
            invoke = true;
            including_horse = true;
        }
        if (invoke) {
            int x = from->originalRightDistanceTo(to);
            int y = from->aliveCount(false) - x;
            n = 1 - qMin(x, y);
            if (including_horse) {
                if (from->getOffensiveHorse())
                    ++n;
                if (to->getDefensiveHorse())
                    --n;
            }
        }
        return n;
    }
};

IkHongrouCard::IkHongrouCard() {
}

bool IkHongrouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return to_select != Self && targets.length() < 2;
}

void IkHongrouCard::onEffect(const CardEffectStruct &effect) const{
   effect.to->drawCards(1, "ikhongrou");
}

class IkHongrouVS : public OneCardViewAsSkill
{
public:
    IkHongrouVS() : OneCardViewAsSkill("ikhongrou")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@ikhongrou";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        IkHongrouCard *first = new IkHongrouCard;
        first->addSubcard(originalcard->getId());
        return first;
    }
};

class IkHongrou : public PhaseChangeSkill
{
public:
    IkHongrou() : PhaseChangeSkill("ikhongrou")
    {
        view_as_skill = new IkHongrouVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && (target->getPhase() == Player::Start || target->getPhase() == Player::Finish);
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForUseCard(player, "@@ikhongrou", "@ikhongrou", -1, Card::MethodDiscard);
    }

    virtual bool onPhaseChange(ServerPlayer *) const
    {
        return false;
    }
};

class IkHuaxiao: public TriggerSkill {
public:
    IkHuaxiao(): TriggerSkill("ikhuaxiao") {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (!TriggerSkill::triggerable(player)) return skill;
        if (player->getPhase() != Player::NotActive) return skill;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player) return skill;
        CardMoveReason reason = move.reason;
        if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
            || (reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
            || (reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
            int i = 0;
            foreach (int card_id, move.card_ids) {
                const Card *card = Sanguosha->getCard(card_id);
                if (card->isRed() && (move.from_places[i] == Player::PlaceHand
                                      || move.from_places[i] == Player::PlaceEquip))
                    skill << objectName();
                i++;
            }
        }
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName(), "@ikhuaxiao", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkHuaxiaoTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = player->tag["IkHuaxiaoTarget"].value<ServerPlayer *>();
        player->tag.remove("IkHuaxiaoTarget");
        if (target)
            target->drawCards(1, objectName());
        return false;
    }
};

class IkQiwu: public TriggerSkill {
public:
    IkQiwu(): TriggerSkill("ikqiwu") {
        events << EventPhaseStart << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Discard) {
                if (player->isKongcheng())
                    return QStringList();
                return QStringList(objectName());
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && player->getPile("step").length() >= 3) {
                player->clearOnePrivatePile("step");
                QList<ServerPlayer *> males;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->isMale())
                        males << p;
                }
                if (males.isEmpty()) return QStringList();

                ServerPlayer *target = room->askForPlayerChosen(player, males, objectName(), "@ikqiwu-choose");
                room->damage(DamageStruct(objectName(), player, target, 2));

                if (!player->isAlive()) return QStringList();
                QList<const Card *> equips = target->getEquips();
                if (!equips.isEmpty()) {
                    DummyCard *dummy = new DummyCard;
                    dummy->addSubcards(equips);
                    if (dummy->subcardsLength() > 0)
                        room->throwCard(dummy, target, player);
                    delete dummy;
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        const Card *card = room->askForCard(player, ".|.|.|hand", "@ikqiwu", QVariant(), Card::MethodNone);
        if (card) {
            room->notifySkillInvoked(player, objectName());
            room->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            player->tag["IkQiwuCard"] = QVariant::fromValue(card);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        const Card *card = player->tag["IkQiwuCard"].value<const Card *>();
        player->tag.remove("IkQiwuCard");
        if (card)
            player->addToPile("step", card);
        return false;
    }
};

class IkShendao: public TriggerSkill {
public:
    IkShendao(): TriggerSkill("ikshendao") {
        events << CardsMoveOneTime << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventLoseSkill && data.toString() == objectName()) {
            room->handleAcquireDetachSkills(player, "-ikzhihui|-ikxuanhuo", true);
        } else if (triggerEvent == EventAcquireSkill && data.toString() == objectName()) {
            if (!player->getPile("step").isEmpty()) {
                room->notifySkillInvoked(player, objectName());
                room->handleAcquireDetachSkills(player, "ikzhihui|ikxuanhuo", true, true);
            }
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill(objectName(), true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && move.to_pile_name == "step") {
                if (player->getPile("step").length() == 1) {
                    room->notifySkillInvoked(player, objectName());
                    room->handleAcquireDetachSkills(player, "ikzhihui|ikxuanhuo", true, true);
                }
            } else if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                       && move.from_pile_names.contains("step")) {
                if (player->getPile("step").isEmpty())
                    room->handleAcquireDetachSkills(player, "-ikzhihui|-ikxuanhuo", true, true);
            }
        }
        return QStringList();
    }
};

class IkTianyanViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkTianyanViewAsSkill(): ZeroCardViewAsSkill("iktianyan") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getPhase() != Player::NotActive || player->hasFlag("Global_IkTianyanFailed")) return false;
        if (pattern == "slash")
            return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
        else if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic"))
            return true;
        return false;
    }

    virtual const Card *viewAs() const{
        IkTianyanCard *tianyan_card = new IkTianyanCard;
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
            pattern = "analeptic";
        tianyan_card->setUserString(pattern);
        return tianyan_card;
    }
};

class IkTianyan: public TriggerSkill {
public:
    IkTianyan(): TriggerSkill("iktianyan") {
        events << CardAsked;
        view_as_skill = new IkTianyanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        QString pattern = data.toStringList().first();
        if (player->getPhase() == Player::NotActive
            && (pattern == "slash" || pattern == "jink"))
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
        QList<int> ids = room->getNCards(2, false);
        QList<int> enabled, disabled;
        foreach (int id, ids) {
            if (Sanguosha->getCard(id)->objectName().contains(pattern))
                enabled << id;
            else
                disabled << id;
        }
        int id = IkTianyan::view(room, player, ids, enabled, disabled);
        if (id != -1) {
            const Card *card = Sanguosha->getCard(id);
            room->provide(card);
            return true;
        }

        return false;
    }

    static int view(Room *room, ServerPlayer *player, QList<int> &ids, QList<int> &enabled, QList<int> &disabled) {
        int result = -1, index = -1;
        LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = player;
        log.card_str = IntList2StringList(ids).join("+");
        room->sendLog(log, player);

        room->broadcastSkillInvoke("iktianyan");
        room->notifySkillInvoked(player, "iktianyan");
        if (enabled.isEmpty()) {
            JsonArray args;
            args << ".";
            args << false;
            args << JsonUtils::toJsonArray(ids);
            room->doNotify(player, QSanProtocol::S_COMMAND_SHOW_ALL_CARDS, args);
        } else {
            room->fillAG(ids, player, disabled);
            int id = room->askForAG(player, enabled, true, "iktianyan");
            if (id != -1) {
                index = ids.indexOf(id);
                ids.removeOne(id);
                result = id;
            }
            room->clearAG(player);
        }

        QList<int> &drawPile = room->getDrawPile();
        for (int i = ids.length() - 1; i >= 0; i--)
            drawPile.prepend(ids.at(i));
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(drawPile.length()));
        if (result == -1)
            room->setPlayerFlag(player, "Global_IkTianyanFailed");
        else {
            LogMessage log;
            log.type = "#IkTianyanUse";
            log.from = player;
            log.arg = "iktianyan";
            log.arg2 = QString("CAPITAL(%1)").arg(index + 1);
            room->sendLog(log);
        }
        return result;
    }
};

IkTianyanCard::IkTianyanCard() {
}

bool IkTianyanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool IkTianyanCard::targetFixed() const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFixed();
}

bool IkTianyanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetsFeasible(targets, Self);
}

const Card *IkTianyanCard::validateInResponse(ServerPlayer *user) const{
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled, disabled;
    foreach (int id, ids) {
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;
        else
            disabled << id;
    }

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "iktianyan";
    room->sendLog(log);

    int id = IkTianyan::view(room, user, ids, enabled, disabled);
    return Sanguosha->getCard(id);
}

const Card *IkTianyanCard::validate(CardUseStruct &cardUse) const{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled, disabled;
    foreach (int id, ids) {
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;
        else
            disabled << id;
    }

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "iktianyan";
    room->sendLog(log);

    int id = IkTianyan::view(room, user, ids, enabled, disabled);
    return Sanguosha->getCard(id);
}

IkCangwuCard::IkCangwuCard() {
}

bool IkCangwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || qMax(0, to_select->getHp()) != subcardsLength())
        return false;

    if (Self->getWeapon() && subcards.contains(Self->getWeapon()->getId())) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        int distance_fix = weapon->getRange() - Self->getAttackRange(false);
        if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
            distance_fix += 1;
        return Self->inMyAttackRange(to_select, distance_fix);
    } else if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId())) {
        return Self->inMyAttackRange(to_select, 1);
    } else
        return Self->inMyAttackRange(to_select);
}

void IkCangwuCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->damage(DamageStruct("ikcangwu", effect.from, effect.to));
    if (effect.from->hasFlag("IkCangwuEnterDying"))
        effect.from->getRoom()->loseHp(effect.from, 1);
}

class IkCangwuViewAsSkill: public ViewAsSkill {
public:
    IkCangwuViewAsSkill(): ViewAsSkill("ikcangwu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasFlag("IkCangwuEnterDying");
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        IkCangwuCard *ikcangwu = new IkCangwuCard;
        if (!cards.isEmpty())
            ikcangwu->addSubcards(cards);
        return ikcangwu;
    }
};

class IkCangwu: public TriggerSkill {
public:
    IkCangwu(): TriggerSkill("ikcangwu") {
        events << QuitDying;
        view_as_skill = new IkCangwuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "ikcangwu" && !dying.damage->chain && !dying.damage->transfer) {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive()) {
                room->setPlayerFlag(from, "IkCangwuEnterDying");
            }
        }
        return QStringList();
    }
};

IkLingzhouCard::IkLingzhouCard() {
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void IkLingzhouCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->addToPile("summon", this);
}

class IkLingzhouViewAsSkill: public ViewAsSkill {
public:
    IkLingzhouViewAsSkill(): ViewAsSkill("iklingzhou") {
        response_pattern = "@@iklingzhou";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *) const{
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 0) return NULL;

        Card *acard = new IkLingzhouCard;
        acard->addSubcards(cards);
        acard->setSkillName(objectName());
        return acard;
    }
};

class IkLingzhou: public TriggerSkill {
public:
    IkLingzhou(): TriggerSkill("iklingzhou") {
        events << EventPhaseStart;
        view_as_skill = new IkLingzhouViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Discard
            && !player->isNude();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        return room->askForUseCard(player, "@@iklingzhou", "@iklingzhou", -1, Card::MethodNone);
    }
};

class IkLingzhouClear: public TriggerSkill {
public:
    IkLingzhouClear(): TriggerSkill("#iklingzhou") {
        events << Damaged;
        frequency = NotCompulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player) || player->getPile("summon").isEmpty()) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel")))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, "iklingzhou");

        QList<int> ids = player->getPile("summon");
        room->fillAG(ids, player);
        int id = room->askForAG(player, ids, false, objectName());
        room->clearAG(player);
        room->obtainCard(player, id);

        return false;
    }
};

class IkMoqizhou: public PhaseChangeSkill {
public:
    IkMoqizhou(): PhaseChangeSkill("ikmoqizhou") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start
               && !target->getPile("summon").isEmpty();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        QList<ServerPlayer *> playerlist;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getHp() <= target->getHp())
                playerlist << p;
        }
        ServerPlayer *to_give = NULL;
        if (!playerlist.isEmpty())
            to_give = room->askForPlayerChosen(target, playerlist, objectName(), "@ikmoqizhou", true);
        if (to_give) {
            DummyCard *dummy = new DummyCard(target->getPile("summon"));
            room->obtainCard(to_give, dummy);
            delete dummy;
            room->recover(to_give, RecoverStruct(target));
        } else {
            int len = target->getPile("summon").length();
            target->clearOnePrivatePile("summon");
            if (target->isAlive())
                room->drawCards(target, len, objectName());
        }
        return false;
    }
};

IkLingtongCard::IkLingtongCard() {
}

void IkLingtongCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    ServerPlayer *player = effect.to;
    room->showAllCards(effect.from, player);
    QStringList choicelist;
    if (!player->isKongcheng())
        choicelist.append("handcards");
    if (!player->isLord())
        choicelist.append("role");
    if (choicelist.isEmpty()) return;
    QString choice = room->askForChoice(effect.from, "iklingtong", choicelist.join("+"), QVariant::fromValue(player));

    LogMessage log;
    log.type = "$IkLingtongView";
    log.from = effect.from;
    log.to << effect.to;
    log.arg = "iklingtong:" + choice;
    room->sendLog(log, room->getOtherPlayers(effect.from));

    if (choice == "handcards") {
        QList<int> ids;
        foreach (const Card *card, player->getHandcards()) {
            if (card->isBlack())
                ids << card->getEffectiveId();
        }

        int card_id = room->doGongxin(effect.from, player, ids, "iklingtong");
        if (card_id == -1) return;
        effect.from->tag.remove("iklingtong");
        CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, effect.from->objectName(), QString(), "iklingtong", QString());
        room->throwCard(Sanguosha->getCard(card_id), reason, effect.to, effect.from);
    } else if (choice == "role") {
        JsonArray args;
        args << player->objectName();
        args << player->getRole();
        room->doNotify(effect.from, QSanProtocol::S_COMMAND_SET_EMOTION, args);

        LogMessage log;
        log.type = "$ViewRole";
        log.from = effect.from;
        log.to << player;
        log.arg = player->getRole();
        room->sendLog(log, effect.from);
    }
}

class IkLingtong: public ZeroCardViewAsSkill {
public:
    IkLingtong(): ZeroCardViewAsSkill("iklingtong") {
    }

    virtual const Card *viewAs() const{
        return new IkLingtongCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("IkLingtongCard");
    }
};

class IkXuexia: public TriggerSkill {
public:
    IkXuexia(): TriggerSkill("ikxuexia") {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from->isAlive())
            for (int i = 0; i < use.to.length(); i++) {
                ServerPlayer *to = use.to.at(i);
                if (to->isAlive() && to->isAdjacentTo(player) && to->isAdjacentTo(use.from))
                    return QStringList(objectName());
            }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
        for (int i = 0; i < use.to.length(); i++) {
            ServerPlayer *to = use.to.at(i);
            if (to->isAlive() && to->isAdjacentTo(player) && to->isAdjacentTo(use.from)
                && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to))) {
                room->broadcastSkillInvoke(objectName());
                if (jink_list.at(i).toInt() == 1)
                    jink_list.replace(i, QVariant(2));
            }
        }
        use.from->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);

        return false;
    }
};

IkLunkeCard::IkLunkeCard() {
}

bool IkLunkeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isChained() && to_select != Self;
}

void IkLunkeCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if (!effect.to->isChained()) {
        effect.to->setChained(true);
        room->broadcastProperty(effect.to, "chained");
        room->setEmotion(effect.to, "effects/iron_chain");
        room->getThread()->trigger(ChainStateChanged, room, effect.to);
    }
    if (!effect.from->isChained()) {
        effect.from->setChained(true);
        room->broadcastProperty(effect.from, "chained");
        room->setEmotion(effect.from, "effects/iron_chain");
        room->getThread()->trigger(ChainStateChanged, room, effect.from);
    }
}

class IkLunke: public ZeroCardViewAsSkill {
public:
    IkLunke(): ZeroCardViewAsSkill("iklunke") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->hasUsed("IkLunkeCard"))
            return false;
        foreach (const Player *p, player->getAliveSiblings())
            if (!p->isChained())
                return true;
        return false;
    }

    virtual const Card *viewAs() const{
        return new IkLunkeCard;
    }
};

class IkCangmie: public PhaseChangeSkill {
public:
    IkCangmie(): PhaseChangeSkill("ikcangmie") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && target->isChained();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (!target->isAlive())
                break;
            if (!p->isAlive() || !p->isChained() || !target->canDiscard(p, "he"))
                continue;
            if (p == target) {
                room->askForDiscard(target, objectName(), 1, 1, false, true);
            } else {
                int id = room->askForCardChosen(target, p, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(id, p, target);
            }
        }
        return false;
    }
};

class IkLinbu: public TriggerSkill {
public:
    IkLinbu(): TriggerSkill("iklinbu") {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                QVariantList sunluyus = player->tag[objectName()].toList();
                foreach (QVariant sunluyu, sunluyus) {
                    ServerPlayer *s = sunluyu.value<ServerPlayer *>();
                    room->removeAttackRangePair(player, s);
                }
                room->setPlayerMark(player, "@linbu", 0);
                room->detachSkillFromPlayer(player, "#iklinbu", false, true);
                room->filterCards(player, player->getCards("he"), true);
            }
            return skill_list;
        }
        if (player->getPhase() == Player::Play) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player) continue;
                if (!player->inMyAttackRange(owner))
                    skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *owner) const{
        if (owner->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *owner) const{
        room->addPlayerMark(player, "@linbu", 1);
        if (!player->hasSkill("#iklinbu", true)) {
            room->acquireSkill(player, "#iklinbu", false);
            room->filterCards(player, player->getCards("he"), false);
        }
        QVariantList sunluyus = player->tag[objectName()].toList();
        sunluyus << QVariant::fromValue(owner);
        player->tag[objectName()] = QVariant::fromValue(sunluyus);
        room->insertAttackRangePair(player, owner);
        return false;
    }
};

class IkLinbuFilter: public FilterSkill {
public:
    IkLinbuFilter(): FilterSkill("#iklinbu") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        return to_select->getTypeId() == Card::TypeTrick;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName("iklinbu");
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class IkMumu : public TriggerSkill
{
public:
    IkMumu() : TriggerSkill("ikmumu")
    {
        events << PreDamageDone << EventPhaseStart;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->hasFlag("IkMumuDamageInPlayPhase"))
                return QStringList();
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getWeapon() && player->canDiscard(p, p->getWeapon()->getEffectiveId()))
                        return QStringList(objectName());
                    if (p->getArmor())
                        return QStringList(objectName());
                }
            }
        } else if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->getPhase() == Player::Play && !damage.from->hasFlag("IkMumuDamageInPlayPhase"))
                damage.from->setFlags("IkMumuDamageInPlayPhase");
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getWeapon() && player->canDiscard(p, p->getWeapon()->getEffectiveId())) {
                victims << p;
                continue;
            }
            if (p->getArmor())
                victims << p;
        }
        ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName(), "@ikmumu", true, true);
        if (victim) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkMumuTarget"] = QVariant::fromValue(victim);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *victim = player->tag["IkMumuTarget"].value<ServerPlayer *>();
        player->tag.remove("IkMumuTarget");
        if (victim) {
            QStringList choices;
            if (victim->getWeapon() && player->canDiscard(victim, victim->getWeapon()->getEffectiveId()))
                choices << "weapon";
            if (victim->getArmor())
                choices << "armor";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"));
            if (choice == "weapon") {
                room->throwCard(victim->getWeapon(), victim, player);
                player->drawCards(1, objectName());
            } else if (choice == "armor") {
                int id = victim->getArmor()->getEffectiveId();
                player->obtainCard(victim->getArmor());
                if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand) {
                    if (Sanguosha->getCard(id)->isKindOf("Armor"))
                        room->useCard(CardUseStruct(Sanguosha->getCard(id), player, QList<ServerPlayer *>()));
                }
            }
        }
        return false;
    }
};

class IkYanhuo: public TriggerSkill {
public:
    IkYanhuo(): TriggerSkill("ikyanhuo") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (player->getPhase() == Player::Start && player->getHandcardNum() <= 1) {
            if (room->findPlayerBySkillName(objectName()))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        QList<ServerPlayer *> chenglais;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->hasSkill(objectName()))
                chenglais << p;
        }

        while (!chenglais.isEmpty()) {
            ServerPlayer *chenglai = room->askForPlayerChosen(player, chenglais, objectName(), "@ikyanhuo-to", true);
            if (chenglai) {
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(chenglai, objectName());
                LogMessage log;
                log.type = "#InvokeOthersSkill";
                log.from = player;
                log.to << chenglai;
                log.arg = objectName();
                room->sendLog(log);

                effect(NonTrigger, room, player, data, chenglai);
                chenglais.removeOne(chenglai);
            } else
                break;

            if (player->getHandcardNum() > 1)
                break;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *chenglai) const{
        player->drawCards(1, objectName());
        if (!player->isKongcheng()) {
            int n = player->getHandcardNum();
            DummyCard *dummy = player->wholeHandCards();
            chenglai->obtainCard(dummy, false);
            delete dummy;
            if (chenglai != player) {
                const Card *card = room->askForExchange(chenglai, objectName(), n, n, true, "@ikyanhuo-return:" + player->objectName());
                if (!card)
                    return false;
                player->obtainCard(card, false);
                delete card;
            }
        }
        return false;
    }
};

IkYaoyinCard::IkYaoyinCard() {
}

bool IkYaoyinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.length() < subcardsLength() && to_select->isWounded();
}

void IkYaoyinCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (int id, subcards) {
        const Card *c = Sanguosha->getCard(id);
        if (c->isBlack() && c->getTypeId() != TypeEquip) {
            room->loseHp(source);
            break;
        }
    }
    room->sortByActionOrder(targets);
    foreach (ServerPlayer *p, targets) {
        if (p->isDead())
            continue;
        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = p;
        onEffect(effect);
    }
}

void IkYaoyinCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->getRoom()->recover(effect.to, RecoverStruct(effect.from));
}

class IkYaoyin: public ViewAsSkill {
public:
    IkYaoyin(): ViewAsSkill("ikyaoyin") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->isWounded() && player->getCardCount() >= player->getLostHp() && !player->hasUsed("IkYaoyinCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *card) const{
        if (selected.length() >= Self->getLostHp())
            return false;
        if (Self->isJilei(card))
            return false;
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != Self->getLostHp())
            return NULL;

        IkYaoyinCard *card = new IkYaoyinCard;
        card->addSubcards(cards);

        return card;
    }
};

class IkCiyu: public TriggerSkill {
public:
    IkCiyu(): TriggerSkill("ikciyu") {
        events << HpRecover;
        frequency = Frequent;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
        if (player && player->isAlive() && player == room->getCurrent() && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                skill_list.insert(p, QStringList(objectName()));
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
        QString choice = room->askForChoice(ask_who, objectName(), "draw+letdraw");
        if (choice == "draw") {
            ask_who->drawCards(1, objectName());
            ask_who->gainMark("@forgive");
        } else {
            player->drawCards(2, objectName());
            player->gainMark("@forgive");
        }
        return false;
    }
};

class IkQingshe: public PhaseChangeSkill {
public:
    IkQingshe(): PhaseChangeSkill("ikqingshe") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        if (PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start && target->getMark("@qingshe") == 0) {
            if (target->isWounded() && target->getMark("@forgive") > 0)
                return true;
            foreach (const Player *p, target->getAliveSiblings()) {
                if (p->isWounded() && p->getMark("@forgive") > 0)
                    return true;
            }
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#IkQingsheWake";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());

        room->setPlayerMark(player, "@qingshe", 1);
        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            room->recover(player, RecoverStruct(player));
            room->acquireSkill(player, "ikbingling");
        }

        return false;
    }
};

IkBinglingCard::IkBinglingCard() {
}

bool IkBinglingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.isEmpty() && to_select->canDiscard(to_select, "hej") && to_select != Self;
}

void IkBinglingCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    QStringList choices;
    if (effect.to->canDiscard(effect.to, "h"))
        choices << "h";
    if (effect.to->canDiscard(effect.to, "e"))
        choices << "e";
    if (effect.to->canDiscard(effect.to, "j"))
        choices << "j";
    QString choice = room->askForChoice(effect.from, "ikbingling", choices.join("+"));
    QList<int> card_ids;
    if (choice == "h")
        card_ids = effect.to->handCards();
    else if (choice == "e") {
        foreach (const Card *cd, effect.to->getEquips())
            card_ids << cd->getId();
    } else if (choice == "j")
        card_ids = effect.to->getJudgingAreaID();
    if (!card_ids.isEmpty()) {
        DummyCard *dummy = new DummyCard(card_ids);
        if (choice == "j")
            room->throwCard(dummy, NULL, effect.to);
        else
            room->throwCard(dummy, effect.to);
        delete dummy;
        effect.to->drawCards(card_ids.length(), "ikbingling");
        room->setPlayerFlag(effect.from, "IkBingling");
    }
}

class IkBingling: public ZeroCardViewAsSkill {
public:
    IkBingling(): ZeroCardViewAsSkill("ikbingling") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkBinglingCard");
    }

    virtual const Card *viewAs() const{
        return new IkBinglingCard;
    }
};

class IkBinglingMaxCards: public MaxCardsSkill {
public:
    IkBinglingMaxCards(): MaxCardsSkill("#ikbingling-max-cards") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasFlag("IkBingling"))
            return -1;
        else
            return 0;
    }
};

IkHuangpoCard::IkHuangpoCard()
{
}

bool IkHuangpoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && !to_select->isAllNude();
}

void IkHuangpoCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    int id = room->askForCardChosen(effect.from, effect.to, "hej", "ikhuangpo");
    if (id != -1) {
        effect.from->addToPile("flap", id);
        QVariantList list = effect.to->tag["IkHuangpoIds"].toList();
        list << id;
        effect.to->tag["IkHuangpoIds"] = list;
    }
}

class IkHuangpo: public ZeroCardViewAsSkill
{
public:
    IkHuangpo(): ZeroCardViewAsSkill("ikhuangpo")
    {
    }

    virtual const Card *viewAs() const
    {
        return new IkHuangpoCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->usedTimes("IkHuangpoCard") < 2;
    }
};

class IkHuangpoReturn: public TriggerSkill
{
public:
    IkHuangpoReturn(): TriggerSkill("#ikhuangpo")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList list;
        if (player && player->isAlive() && player->getPhase() == Player::Finish && !player->getPile("flap").isEmpty()) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                foreach (QVariant _id, p->tag["IkHuangpoIds"].toList()) {
                    bool ok = false;
                    int id = _id.toInt(&ok);
                    if (ok && player->getPile("flap").contains(id)) {
                        list.insert(p, QStringList(objectName()));
                        break;
                    }
                }
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        QList<int> ids;
        QVariantList list = ask_who->tag["IkHuangpoIds"].toList();
        QVariantList list_new;
        foreach (QVariant _id, list) {
            bool ok = false;
            int id = _id.toInt(&ok);
            if (ok && player->getPile("flap").contains(id))
                ids << id;
            else
                list_new << id;
        }
        ask_who->tag["IkHuangpoIds"] = list_new;
        DummyCard dummy(ids);
        ask_who->obtainCard(&dummy);
        return false;
    }
};

class IkYixiang : public TriggerSkill
{
public:
    IkYixiang() : TriggerSkill("ikyixiang")
    {
        events << EventPhaseChanging << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!p->tag["IkYixiangTargets"].toStringList().isEmpty()) {
                        QStringList targets = p->tag["IkYixiangTargets"].toStringList();
                        p->tag.remove("IkYixiangTargets");
                        foreach (QString target, targets)
                            room->removeFixedDistance(p, room->findPlayer(target), 1);
                    }
                }
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (TriggerSkill::triggerable(player) && player == move.from) {
                if (move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
                    ServerPlayer *current = room->getCurrent();
                    if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
                        QStringList targets = player->tag["IkYixiangTargets"].toStringList();
                        targets << move.from->objectName();
                        player->tag["IkYixiangTargets"] = QVariant::fromValue(targets);
                        room->setFixedDistance(player, move.from, 1);
                    }
                }
            }
        }
        return QStringList();
    }
};

IkChenyan::IkChenyan(): TriggerSkill("ikchenyan") {
    events << DrawNCards << EventPhaseStart;
    frequency = Compulsory;
}

int IkChenyan::getKingdoms(ServerPlayer *yuanshu) const{
    QSet<QString> kingdom_set;
    Room *room = yuanshu->getRoom();
    foreach (ServerPlayer *p, room->getAlivePlayers())
        kingdom_set << p->getKingdom();

    return qMax(kingdom_set.size(), 2);
}

QStringList IkChenyan::triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *yuanshu, QVariant &, ServerPlayer* &) const{
    if (!TriggerSkill::triggerable(yuanshu)) return QStringList();
    if (triggerEvent == DrawNCards)
        return QStringList(objectName());
    else if (triggerEvent == EventPhaseStart && yuanshu->getPhase() == Player::Discard && !yuanshu->isNude())
        return QStringList(objectName());
    return QStringList();
}

bool IkChenyan::effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *yuanshu, QVariant &data, ServerPlayer *) const{
    if (triggerEvent == DrawNCards) {
        int x = getKingdoms(yuanshu);
        data = data.toInt() + x;

        Room *room = yuanshu->getRoom();
        LogMessage log;
        log.type = "#IkChenyanGood";
        log.from = yuanshu;
        log.arg = QString::number(x);
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(yuanshu, objectName());
        room->broadcastSkillInvoke(objectName());
    } else if (triggerEvent == EventPhaseStart) {
        int x = getKingdoms(yuanshu) + 1;
        LogMessage log;
        log.type = yuanshu->getCardCount() > x ? "#IkChenyanBad" : "#IkChenyanWorst";
        log.from = yuanshu;
        log.arg = QString::number(log.type == "#IkChenyanBad" ? x : yuanshu->getCardCount());
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(yuanshu, objectName());
        room->broadcastSkillInvoke(objectName());
        room->askForDiscard(yuanshu, "ikchenyan", x, x, false, true);
    }

    return false;
}

class IkMoliao: public TriggerSkill {
public:
    IkMoliao(): TriggerSkill("ikmoliao") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player) || !player->isKongcheng())
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage > 1)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, objectName());
        DamageStruct damage = data.value<DamageStruct>();
        damage.damage = 1;
        data = QVariant::fromValue(damage);
        return false;
    }
};

IkZhangeCard::IkZhangeCard() {
    mute = true;
}

bool IkZhangeCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const{
    if (Self->aliveCount() > 5)
        return targets.length() < 3;
    else
        return targets.length() < 2;
}

void IkZhangeCard::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    if (card_use.to.length() == 1 && card_use.to.contains(card_use.from))
        room->broadcastSkillInvoke("ikzhange", 2);
    else
        room->broadcastSkillInvoke("ikzhange", 1);
    room->removePlayerMark(use.from, "@zhange");
    room->addPlayerMark(use.from, "@zhangeused");
    SkillCard::onUse(room, use);
}

void IkZhangeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *p, targets)
        p->drawCards(3, "ikzhange");
    if (targets.length() == 1 && targets.contains(source) && source->isWounded())
        room->recover(source, RecoverStruct(source));
}

class IkZhange: public ZeroCardViewAsSkill {
public:
    IkZhange(): ZeroCardViewAsSkill("ikzhange") {
        frequency = Limited;
        limit_mark = "@zhange";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@zhange") >= 1;
    }

    virtual const Card *viewAs() const{
        return new IkZhangeCard;
    }
};

class IkZhizhai : public TriggerSkill
{
public:
    IkZhizhai() : TriggerSkill("ikzhizhai")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        DamageStruct damage = data.value<DamageStruct>();
        QStringList choices;
        choices << "reduce";
        if (!damage.from->isNude())
            choices << "show";
        if (room->askForChoice(damage.from, objectName(), choices.join("+")) == "show") {
            if (!damage.from->isKongcheng())
                room->showAllCards(damage.from);
            const Card *card = room->askForCard(damage.from, "..!", "@ikzhizhai", QVariant(), Card::MethodNone);
            if (!card) {
                QList<const Card *> cards = damage.from->getCards("he");
                card = cards.at(qrand() % cards.length());
            }
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, damage.from->objectName(), player->objectName(), objectName(), QString());
            room->obtainCard(player, card, reason);
        } else {
            LogMessage log;
            log.type = "#IkZhizhai";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(--damage.damage);
            room->sendLog(log);

            if (damage.damage < 1)
                return true;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class IkLihui: public TriggerSkill {
public:
    IkLihui(): TriggerSkill("iklihui") {
        events << BeforeCardsMove;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *kongrong, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(kongrong)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != kongrong)
            return QStringList();
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)) {

            int i = 0;
            foreach (int card_id, move.card_ids) {
                if (room->getCardOwner(card_id) == move.from
                    && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)) {
                        return QStringList(objectName());
                }
                i++;
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *kongrong, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        int i = 0;
        QList<int> lihui_card;
        foreach (int card_id, move.card_ids) {
            if (room->getCardOwner(card_id) == move.from
                && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)) {
                    lihui_card << card_id;
            }
            i++;
        }

        if (lihui_card.isEmpty())
            return false;

        QList<int> original_lihui = lihui_card;
        while (room->askForYiji(kongrong, lihui_card, objectName(), false, true, true, -1,
                                QList<ServerPlayer *>(), move.reason, "@iklihui-distribute", true)) {
            if (kongrong->isDead()) break;
        }

        QList<int> ids;
        foreach (int card_id, original_lihui) {
            if (!lihui_card.contains(card_id))
                ids << card_id;
        }
        move.removeCardIds(ids);
        data = QVariant::fromValue(move);

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *target, const Card *dummy) const{
        ServerPlayer *owner = target->getRoom()->getCardOwner(dummy->getEffectiveId());
        if (owner && owner->getGeneralName() == "snow039")
            return qrand() % 2 + 1;
        return 0;
    }
};

class IkCunyang: public TriggerSkill {
public:
    IkCunyang(): TriggerSkill("ikcunyang") {
        events << PindianVerifying;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data) const{
        TriggerList skill_list;
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (TriggerSkill::triggerable(pindian->from))
            skill_list.insert(pindian->from, QStringList(objectName()));
        if (TriggerSkill::triggerable(pindian->to))
            skill_list.insert(pindian->to, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        if (ask_who->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (ask_who == pindian->from) {
            QStringList choices;
            if (pindian->from_number < 13)
                choices << "up";
            if (pindian->from_number > 1)
                choices << "down";
            QString choice = room->askForChoice(pindian->from, objectName(), choices.join("+"), data);
            if (choice == "up") {
                pindian->from_number = qMin(pindian->from_number + 3, 13);
                doIkCunyangLog(room, pindian->from, choice, pindian->from_number);
                data = QVariant::fromValue(pindian);
            } else if (choice == "down") {
                pindian->from_number = qMax(pindian->from_number - 3, 1);
                doIkCunyangLog(room, pindian->from, choice, pindian->from_number);
                data = QVariant::fromValue(pindian);
            }
        }
        if (ask_who == pindian->to) {
            QStringList choices;
            if (pindian->to_number < 13)
                choices << "up";
            if (pindian->to_number > 1)
                choices << "down";
            QString choice = room->askForChoice(pindian->to, objectName(), "up+down+cancel", data);
            if (choice == "up") {
                pindian->to_number = qMin(pindian->to_number + 3, 13);
                doIkCunyangLog(room, pindian->to, choice, pindian->to_number);
                data = QVariant::fromValue(pindian);
            } else if (choice == "down") {
                pindian->to_number = qMax(pindian->to_number - 3, 1);
                doIkCunyangLog(room, pindian->to, choice, pindian->to_number);
                data = QVariant::fromValue(pindian);
            }
        }
        return false;
    }

private:
    QString getNumberString(int number) const{
        if (number == 10)
            return "10";
        else {
            static const char *number_string = "-A23456789-JQK";
            return QString(number_string[number]);
        }
    }

    void doIkCunyangLog(Room *room, ServerPlayer *player, const QString &choice, int number) const{
        room->notifySkillInvoked(player, objectName());
        if (choice == "up") {
            LogMessage log;
            log.type = "#IkCunyangUp";
            log.from = player;
            log.arg = getNumberString(number);
            room->sendLog(log);
        } else if (choice == "down") {
            LogMessage log;
            log.type = "#IkCunyangDown";
            log.from = player;
            log.arg = getNumberString(number);
            room->sendLog(log);
        }
    }
};

IkFeishanCard::IkFeishanCard() {
}

bool IkFeishanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void IkFeishanCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    bool success = effect.from->pindian(effect.to, "ikfeishan", NULL);
    if (success) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_ikfeishan");
        if (!effect.from->canSlash(effect.to, slash, false))
            delete slash;
        room->useCard(CardUseStruct(slash, effect.from, effect.to));
    } else {
        room->setPlayerFlag(effect.from, "IkFeishanDisableSlashTo" + effect.to->objectName());
    }
}

class IkFeishanViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkFeishanViewAsSkill(): ZeroCardViewAsSkill("ikfeishan") {
        response_pattern = "@@ikfeishan";
    }

    virtual const Card *viewAs() const{
        return new IkFeishanCard;
    }
};

class IkFeishan: public PhaseChangeSkill {
public:
    IkFeishan(): PhaseChangeSkill("ikfeishan") {
        view_as_skill = new IkFeishanViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *jiling) const{
        foreach (ServerPlayer *player, jiling->getRoom()->getAllPlayers()) {
            if (player == jiling) continue;
            if (!player->isKongcheng())
                return PhaseChangeSkill::triggerable(jiling) && jiling->getPhase() == Player::Play && !jiling->isKongcheng();
        }
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *jiling, QVariant &, ServerPlayer *) const{
        return room->askForUseCard(jiling, "@@ikfeishan", "@ikfeishan-card");
    }

    virtual bool onPhaseChange(ServerPlayer *) const{
        return false;
    }
};

class IkFeishanProhibit: public ProhibitSkill {
public:
    IkFeishanProhibit(): ProhibitSkill("#ikfeishan") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        return card->isKindOf("Slash") && from->hasFlag("IkFeishanDisableSlashTo" + to->objectName());
    }
};

class IkNifa : public TriggerSkill
{
public:
    IkNifa() : TriggerSkill("iknifa")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *tianfeng, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(tianfeng)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (((move.from == tianfeng && move.from_places.contains(Player::PlaceHand))
            || (move.to == tianfeng && move.to_place == Player::PlaceHand))
            && tianfeng->getHandcardNum() < qMax(tianfeng->getLostHp(), 1)) {
            foreach (ServerPlayer *p, room->getOtherPlayers(tianfeng)) {
                if (tianfeng->canDiscard(p, "he"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *tianfeng, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> other_players = room->getOtherPlayers(tianfeng);
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, other_players) {
            if (tianfeng->canDiscard(p, "he"))
                targets << p;
        }
        ServerPlayer *to = room->askForPlayerChosen(tianfeng, targets, objectName(), "iknifa-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            tianfeng->tag["IkNifaTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *tianfeng, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *to = tianfeng->tag["IkNifaTarget"].value<ServerPlayer *>();
        tianfeng->tag.remove("IkNifaTarget");
        if (to) {
            int card_id = room->askForCardChosen(tianfeng, to, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, to, tianfeng);
        }
        return false;
    }
};

class IkGuyi : public TriggerSkill
{
public:
    IkGuyi() : TriggerSkill("ikguyi")
    {
        events << HpChanged << Death;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == HpChanged && player->isAlive()) {
            if (!data.isNull() && !data.canConvert<RecoverStruct>()) {
                int n = 0;
                bool ok = false;
                if (data.toInt(&ok) && ok)
                    n = data.toInt();
                else if (data.canConvert<DamageStruct>())
                    n = data.value<DamageStruct>().damage;
                if (n == 0) return skill_list;
                foreach (ServerPlayer *tianfeng, room->findPlayersBySkillName(objectName())) {
                    if (tianfeng == player) continue;
                    int x = qMax(tianfeng->getLostHp(), 1);
                    if (player->getHp() < x) {
                        QStringList skill;
                        for (int i = 0; i < qMin(n, x - player->getHp()); i++)
                            skill << objectName();
                        skill_list.insert(tianfeng, skill);
                    }
                }
            }
        } else if (triggerEvent == Death && TriggerSkill::triggerable(player)) {
            skill_list.insert(player, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        if (triggerEvent == HpChanged) {
            room->broadcastSkillInvoke(objectName(), 1);
            ask_who->drawCards(1, objectName());
        } else {
            room->broadcastSkillInvoke(objectName(), 2);
            room->loseHp(ask_who);
        }
        return false;
    }
};

class IkShunqie: public TriggerSkill {
public:
    IkShunqie(): TriggerSkill("ikshunqie") {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *panfeng, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(panfeng))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (target->hasEquip() && !target->hasFlag("Global_DebutFlag") && panfeng != target)
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *panfeng, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        int id = room->askForCardChosen(panfeng, target, "e", objectName());
        room->obtainCard(panfeng, id);

        return false;
    }
};

class IkLongya: public TriggerSkill {
public:
    IkLongya(): TriggerSkill("iklongya") {
        events << TargetSpecified << CardFinished;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return QStringList();
            return QStringList(objectName());
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return QStringList();
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, objectName() + use.card->toString(), 0);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to) {
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                QString choice;
                if (!player->canDiscard(p, "he"))
                    choice = "draw";
                else
                    choice = room->askForChoice(player, objectName(), "draw+discard", QVariant::fromValue(p));
                room->broadcastSkillInvoke(objectName());
                if (choice == "draw") {
                    player->drawCards(1, objectName());
                } else {
                    int disc = room->askForCardChosen(player, p, "he", objectName(), false, Card::MethodDiscard);
                    room->throwCard(disc, p, player);
                }
                room->addPlayerMark(p, objectName() + use.card->toString());
            }
        }

        return false;
    }
};

class IkLongyaMiss: public TriggerSkill {
public:
    IkLongyaMiss(): TriggerSkill("#iklongya-miss") {
        events << SlashMissed;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isDead() || effect.to->getMark("iklongya" + effect.slash->toString()) <= 0)
            return QStringList();
        if (!effect.from->isAlive() || !effect.to->isAlive() || !effect.to->canDiscard(effect.from, "he"))
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        int disc = room->askForCardChosen(effect.to, effect.from, "he", "iklongya", false, Card::MethodDiscard);
        room->throwCard(disc, effect.from, effect.to);
        room->removePlayerMark(effect.to, "iklongya" + effect.slash->toString());
        return false;
    }
};

class IkHengmou: public TriggerSkill {
public:
    IkHengmou(): TriggerSkill("ikhengmou") {
        events << TargetConfirming;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash"))
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
        room->askForDiscard(player, objectName(), 2, 2, false, true);
        player->drawCards(2, objectName());

        int max = -1000;
        foreach (ServerPlayer *p, room->getAllPlayers())
            if (p->getHp() > max)
                max = p->getHp();
        if (player->getHp() == max)
            return false;

        QList<ServerPlayer *> maxs;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getHp() == max)
                maxs << p;
            if (maxs.size() > 1)
                return false;
        }
        ServerPlayer *mosthp = maxs.first();
        if (mosthp->askForSkillInvoke(objectName())) {
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = mosthp;
            log.to << player;
            log.arg = objectName();
            room->sendLog(log);
            room->askForDiscard(mosthp, objectName(), 2, 2, false, true);
            mosthp->drawCards(2, objectName());
        }

        return false;
    }
};

IkXincaoCard::IkXincaoCard() {
}

void IkXincaoCard::onEffect(const CardEffectStruct &effect) const{
    DummyCard *handcards = effect.from->wholeHandCards();
    effect.to->obtainCard(handcards, false);
    delete handcards;
    if (effect.to->isKongcheng()) return;

    Room *room = effect.from->getRoom();

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.to))
        if (!p->isKongcheng())
            targets << p;

    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "ikxincao", "@ikxincao-pindian:" + effect.to->objectName());
        effect.to->pindian(target, "ikxincao", NULL);
    }
}

class IkXincaoViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkXincaoViewAsSkill(): ZeroCardViewAsSkill("ikxincao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("IkXincaoCard");
    }

    virtual const Card *viewAs() const{
        return new IkXincaoCard;
    }
};

class IkXincao: public TriggerSkill {
public:
    IkXincao(): TriggerSkill("ikxincao") {
        events << Pindian;
        view_as_skill = new IkXincaoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != objectName() || pindian->from_number == pindian->to_number)
            return QStringList();

        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        ServerPlayer *loser = pindian->isSuccess() ? pindian->to : pindian->from;
        if (winner->canSlash(loser, NULL, false))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        ServerPlayer *loser = pindian->isSuccess() ? pindian->to : pindian->from;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_ikxincao");
        room->useCard(CardUseStruct(slash, winner, loser));

        return false;
    }
};

class IkXincaoSlashNoDistanceLimit: public TargetModSkill {
public:
    IkXincaoSlashNoDistanceLimit(): TargetModSkill("#ikxincao-slash-ndl") {
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const{
        if (card->isKindOf("Slash") && card->getSkillName() == "ikxincao")
            return 1000;
        else
            return 0;
    }
};

IkKouzhuCard::IkKouzhuCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void IkKouzhuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    QStringList list = target->tag["IkKouzhuSource"].toStringList();
    list << source->objectName();
    target->tag["IkKouzhuSource"] = QVariant::fromValue(list);
    source->addToPile("satire", this, false);
    room->addPlayerMark(target, "@kouzhu");
}

class IkKouzhuViewAsSkill: public OneCardViewAsSkill {
public:
    IkKouzhuViewAsSkill(): OneCardViewAsSkill("ikkouzhu") {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@ikkouzhu";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        Card *card = new IkKouzhuCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class IkKouzhu: public TriggerSkill {
public:
    IkKouzhu(): TriggerSkill("ikkouzhu") {
        events << EventPhaseStart;
        view_as_skill = new IkKouzhuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && !target->isKongcheng()
            && target->getPile("satire").isEmpty();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->askForUseCard(player, "@@ikkouzhu", "@ikkouzhu-remove", -1, Card::MethodNone);
        return false;
    }
};

class IkKouzhuTrigger: public TriggerSkill {
public:
    IkKouzhuTrigger(): TriggerSkill("#ikkouzhu") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return player->getPhase() == Player::RoundStart
            && !player->tag["IkKouzhuSource"].isNull();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        QStringList objnames = player->tag["IkKouzhuSource"].toStringList();
        player->tag.remove("IkKouzhuSource");
        QList<ServerPlayer *> chenlins;
        foreach (QString objname, objnames) {
            ServerPlayer *chenlin = room->findPlayer(objname);
            if (chenlin)
                chenlins << chenlin;
        }
        room->sortByActionOrder(chenlins);
        foreach (ServerPlayer *chenlin, chenlins) {
            int card_id = chenlin->getPile("satire").first();
            room->showCard(chenlin, card_id);
            const Card *cd = Sanguosha->getCard(card_id);
            QString pattern;
            if (cd->isKindOf("BasicCard"))
                pattern = "BasicCard";
            else if (cd->isKindOf("TrickCard"))
                pattern = "TrickCard";
            else if (cd->isKindOf("EquipCard"))
                pattern = "EquipCard";
            pattern.append("|.|.|hand");
            const Card *to_give = NULL;
            if (!player->isKongcheng() && chenlin && chenlin->isAlive())
                to_give = room->askForCard(player, pattern, "@ikkouzhu-give", QVariant(), Card::MethodNone, chenlin);
            if (chenlin && to_give) {
                CardMoveReason reasonG(CardMoveReason::S_REASON_GIVE, player->objectName(), chenlin->objectName(), "ikkouzhu", QString());
                room->obtainCard(chenlin, to_give, reasonG, false);
                CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, player->objectName(), "ikkouzhu", QString());
                room->obtainCard(player, cd, reason, false);
            } else {
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                room->throwCard(cd, reason, NULL);
                room->loseHp(player);
            }
            room->removePlayerMark(player, "@kouzhu");
        }
        return false;
    }
};

class IkKouzhuClear: public TriggerSkill {
public:
    IkKouzhuClear(): TriggerSkill("#ikkouzhu-clear") {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player->tag["IkKouzhuSource"].isNull()) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player) {
                QStringList objnames = player->tag["IkKouzhuSource"].toStringList();
                foreach (QString objname, objnames) {
                    ServerPlayer *chenlin = room->findPlayer(objname);
                    if (chenlin)
                        chenlin->clearOnePrivatePile("satire");
                }
            }
        }
        return QStringList();
    }
};

IkJiaojinCard::IkJiaojinCard() {
    mute = true;
}

bool IkJiaojinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getMark("ikjiaojin" + Self->objectName()) == 0 && to_select->getHandcardNum() != to_select->getHp();
}

void IkJiaojinCard::onEffect(const CardEffectStruct &effect) const{
    int handcard_num = effect.to->getHandcardNum();
    int hp = effect.to->getHp();
    effect.to->gainMark("@jiaojin");
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.to, "ikjiaojin" + effect.from->objectName());
    if (handcard_num > hp) {
        room->askForDiscard(effect.to, "ikjiaojin", 2, 2, false, true);
    } else if (handcard_num < hp) {
        effect.to->drawCards(2, "ikjiaojin");
    }
}

class IkJiaojinViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkJiaojinViewAsSkill(): ZeroCardViewAsSkill("ikjiaojin") {
    }

    virtual const Card *viewAs() const{
        return new IkJiaojinCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getMark("ikjiaojin" + player->objectName()) == 0 && player->getHandcardNum() != player->getHp()) return true;
        foreach (const Player *sib, player->getAliveSiblings())
            if (sib->getMark("ikjiaojin" + player->objectName()) == 0 && sib->getHandcardNum() != sib->getHp())
                return true;
        return false;
    }
};

class IkJiaojin: public TriggerSkill {
public:
    IkJiaojin(): TriggerSkill("ikjiaojin") {
        events << Death;
        view_as_skill = new IkJiaojinViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player) return QStringList();
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("ikjiaojin" + player->objectName()) > 0) {
                room->setPlayerMark(p, "ikjiaojin" + player->objectName(), 0);
                room->removePlayerMark(p, "@jiaojin");
            }
        }
        return QStringList();
    }
};

IkSheqieCard::IkSheqieCard() {
}

bool IkSheqieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && Self->inMyAttackRange(to_select);
}

void IkSheqieCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    ServerPlayer *hejin = effect.from, *target = effect.to;
    if (target->isKongcheng()) return;

    const Card *card = NULL;
    if (target->getHandcardNum() > 1) {
        card = room->askForCard(target, ".!", "@iksheqie-give:" + hejin->objectName(), QVariant(), Card::MethodNone);
        if (!card)
            card = target->getHandcards().at(qrand() % target->getHandcardNum());
    } else {
        card = target->getHandcards().first();
    }
    Q_ASSERT(card != NULL);
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), hejin->objectName(), "iksheqie", QString());
    room->obtainCard(hejin, card, reason, false);
    if (!hejin->isAlive() || !target->isAlive()) return;
    if (hejin->getHandcardNum() > target->getHandcardNum()) {
        QStringList choicelist;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_iksheqie");
        Duel *duel = new Duel(Card::NoSuit, 0);
        duel->setSkillName("_iksheqie");
        if (!target->isLocked(slash) && target->canSlash(hejin, slash, false))
            choicelist.append("slash");
        if (!target->isLocked(duel) && !target->isProhibited(hejin, duel))
            choicelist.append("duel");
        if (choicelist.isEmpty()) {
            delete slash;
            delete duel;
            return;
        }
        QString choice = room->askForChoice(target, "iksheqie", choicelist.join("+"));
        CardUseStruct use;
        use.from = target;
        use.to << hejin;
        if (choice == "slash") {
            delete duel;
            use.card = slash;
        } else {
            delete slash;
            use.card = duel;
        }
        room->useCard(use);
    }
}

class IkSheqie: public ZeroCardViewAsSkill {
public:
    IkSheqie(): ZeroCardViewAsSkill("iksheqie") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkSheqieCard");
    }

    virtual const Card *viewAs() const{
        return new IkSheqieCard;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (card->isKindOf("IkSheqieCard"))
            return -1;
        else
            return -2;
    }
};

class IkYanzhou: public TriggerSkill {
public:
    IkYanzhou(): TriggerSkill("ikyanzhou") {
        events << BeforeGameOverJudge;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (player && !player->isAlive() && player->hasSkill(objectName()) && !player->isNude()) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->canDiscard(p, "he"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (player->canDiscard(p, "he"))
                targets << p;
        ServerPlayer *killer = room->askForPlayerChosen(player, targets, objectName(), "ikyanzhou-invoke", true, true);
        if (killer) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkYanzhouTarget"] = QVariant::fromValue(killer);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        int n = player->getCardCount();
        ServerPlayer *target = player->tag["IkYanzhouTarget"].value<ServerPlayer *>();
        player->tag.remove("IkYanzhouTarget");
        if (target) {
            for (int i = 0; i < n; i++) {
                if (player->canDiscard(target, "he")) {
                    int card_id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
                    room->throwCard(Sanguosha->getCard(card_id), target, player);
                } else {
                    break;
                }
            }
        }
        return false;
    }
};

class IkHongta: public TriggerSkill {
public:
    IkHongta(): TriggerSkill("ikhongta") {
        events << CardFinished;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        TriggerList skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Weapon") && !use.card->isKindOf("Horse"))
            return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
            if (use.from->inMyAttackRange(owner))
                skill_list.insert(owner, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *owner) const{
        room->sendCompulsoryTriggerLog(owner, objectName());
        room->broadcastSkillInvoke(objectName());
        owner->drawCards(1, objectName());
        return false;
    }
};

class IkNicuViewAsSkill: public OneCardViewAsSkill {
public:
    IkNicuViewAsSkill(): OneCardViewAsSkill("iknicu") {
        filter_pattern = ".|black";
        response_or_use = true;
        response_pattern = "@@iknicu";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        slash->addSubcard(originalCard);
        slash->setSkillName("iknicu");
        return slash;
    }
};

class IkNicu: public TriggerSkill {
public:
    IkNicu(): TriggerSkill("iknicu") {
        events << EventPhaseStart;
        view_as_skill = new IkNicuViewAsSkill;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
        if (player->getPhase() == Player::Finish)
            foreach (ServerPlayer *hansui, room->findPlayersBySkillName(objectName()))
                if (hansui->inMyAttackRange(player) && hansui->canSlash(player, false)
                    && (player->getHp() > hansui->getHp() || hansui->hasFlag("IkNicuSlashTarget")))
                    skill_list.insert(hansui, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *hansui) const{
        room->setPlayerFlag(hansui, "slashTargetFix");
        room->setPlayerFlag(hansui, "slashNoDistanceLimit");
        room->setPlayerFlag(hansui, "slashTargetFixToOne");
        room->setPlayerFlag(player, "SlashAssignee");

        bool slash = room->askForUseCard(hansui, "@@iknicu", "@iknicu-slash:" + player->objectName());
        if (!slash) {
            room->setPlayerFlag(hansui, "-slashTargetFix");
            room->setPlayerFlag(hansui, "-slashNoDistanceLimit");
            room->setPlayerFlag(hansui, "-slashTargetFixToOne");
            room->setPlayerFlag(player, "-SlashAssignee");
        }

        return slash;
    }
};

class IkNicuRecord: public TriggerSkill {
public:
    IkNicuRecord(): TriggerSkill("#iknicu-record") {
        events << TargetSpecified << EventPhaseStart;
        global = true;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == TargetSpecified && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                foreach (ServerPlayer *to, use.to) {
                    if (!to->hasFlag("IkNicuSlashTarget"))
                        to->setFlags("IkNicuSlashTarget");
                }
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                if (p->hasFlag("IkNicuSlashTarget"))
                    p->setFlags("-IkNicuSlashTarget");
        }
        return QStringList();
    }
};

class IkZhoudu : public TriggerSkill
{
public:
    IkZhoudu() : TriggerSkill("ikzhoudu")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() != Player::Play)
            return skill_list;
        foreach (ServerPlayer *hetaihou, room->findPlayersBySkillName(objectName())) {
            if (!hetaihou->canDiscard(hetaihou, "h") || hetaihou == player)
                continue;
            skill_list.insert(hetaihou, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *hetaihou) const
    {
        return room->askForCard(hetaihou, ".", "@ikzhoudu-discard", QVariant(), objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *hetaihou) const
    {
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName("_ikzhoudu");
        if (analeptic->isAvailable(player))
            room->useCard(CardUseStruct(analeptic, player, QList<ServerPlayer *>()), true);
        if (player->isAlive())
            room->damage(DamageStruct(objectName(), hetaihou, player));

        return false;
    }
};

class IkKuangdi: public TriggerSkill {
public:
    IkKuangdi(): TriggerSkill("ikkuangdi") {
        events << Death << EventPhaseStart;
        frequency = Frequent;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return skill_list;
            ServerPlayer *killer = death.damage ? death.damage->from : NULL;
            ServerPlayer *current = room->getCurrent();

            if (killer && current && (current->isAlive() || death.who == current)
                && current->getPhase() != Player::NotActive)
                killer->setMark(objectName(), 1);
        } else {
            if (player->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->setMark(objectName(), 0);
            } else if (player->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *hetaihou, room->findPlayersBySkillName(objectName()))
                    if (hetaihou->getMark(objectName()) > 0) {
                        QStringList list;
                        for (int i = 0; i < hetaihou->getMark(objectName()); i++)
                            list << objectName();
                        skill_list.insert(hetaihou, list);
                    }
            }
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *player) const{
        player->drawCards(3, objectName());
        return false;
    }
};

class IkBengying: public TriggerSkill {
public:
    IkBengying(): TriggerSkill("ikbengying") {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player && !player->isAlive() && player->hasSkill(objectName()) && !player->isNude()) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player) {
                DamageStruct *damage = death.damage;
                if (damage && damage->from && damage->from->isAlive())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DeathStruct death = data.value<DeathStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(death.damage->from))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        int n = player->getCardCount();
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *target = death.damage->from;
        for (int i = 0; i < n; i++) {
            if (target->isDead()) break;
            JudgeStruct judge;
            judge.pattern = ".|spade|2~9";
            judge.good = false;
            judge.reason = objectName();
            judge.who = target;
            judge.negative = true;
            room->judge(judge);
            if (judge.isBad())
                room->damage(DamageStruct(objectName(), NULL, target, 3, DamageStruct::Fire));
        }
        return false;
    }
};

class IkMingzhen: public TriggerSkill {
public:
    IkMingzhen(): TriggerSkill("ikmingzhen") {
        events << DamageCaused << EventPhaseStart << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("mingzhen_" + player->objectName()) > 0) {
                        room->removePlayerMark(p, "@mingzhen", p->getMark("mingzhen_" + player->objectName()));
                        room->setPlayerMark(p, "mingzhen_" + player->objectName(), 0);
                    }
                }
            }
            return QStringList();
        }
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("mingzhen_" + player->objectName()) > 0) {
                        room->removePlayerMark(p, "@mingzhen", p->getMark("mingzhen_" + player->objectName()));
                        room->setPlayerMark(p, "mingzhen_" + player->objectName(), 0);
                    }
                }
            }
            return QStringList();
        }
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))
            && !damage.chain && !damage.transfer && damage.by_user) {
            if (!damage.to->hasSkill("thyanmeng") && !damage.to->hasSkill("thxuanyan")) {
                foreach (const Skill *skill, damage.to->getVisibleSkillList()) {
                    if (!skill->isAttachedLordSkill())
                        return QStringList(objectName());
                }
            }
            if (damage.to->canDiscard(damage.to, "e"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        QStringList choices;
        if (!damage.to->hasSkill("thyanmeng") && !damage.from->hasSkill("thxuanyan")) {
            foreach (const Skill *skill, damage.to->getVisibleSkillList()) {
                if (!skill->isAttachedLordSkill()) {
                    choices << "null";
                    break;
                }
            }
        }
        if (damage.to->canDiscard(damage.to, "e"))
            choices << "throw";

        if (!choices.isEmpty()) {
            QString choice = room->askForChoice(damage.to, objectName(), choices.join("+"), data);
            if (choice == "throw") {
                damage.to->throwAllEquips();
                if (damage.to->isAlive())
                    room->loseHp(damage.to);
            } else {
                room->addPlayerMark(damage.to, "@mingzhen");
                room->addPlayerMark(damage.to, "mingzhen_" + player->objectName());
            }
        }

        return true;
    }
};

class IkMingzhenInvalidity: public InvaliditySkill {
public:
    IkMingzhenInvalidity(): InvaliditySkill("#ikmingzhen-inv") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *) const{
        return player->getMark("@mingzhen") == 0;
    }
};

class IkPianxiang: public TriggerSkill {
public:
    IkPianxiang(): TriggerSkill("ikpianxiang") {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from->isAlive()) {
            for (int i = 0; i < use.to.length(); i++) {
                ServerPlayer *to = use.to.at(i);
                if (to->isAlive() && to->isAdjacentTo(player) && to->isAdjacentTo(use.from)
                    && !to->getEquips().isEmpty()) {
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        for (int i = 0; i < use.to.length(); i++) {
            ServerPlayer *to = use.to.at(i);
            if (to->isAlive() && to->isAdjacentTo(player) && to->isAdjacentTo(use.from)
                && !to->getEquips().isEmpty()) {
                bool invoke = room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to));
                if (!invoke) continue;
                room->broadcastSkillInvoke(objectName());
                int id = -1;
                if (to->getEquips().length() == 1)
                    id = to->getEquips().first()->getEffectiveId();
                else
                    id = room->askForCardChosen(to, to, "e", objectName(), false, Card::MethodDiscard);
                room->throwCard(id, to);
            }
        }

        return false;
    }
};

class IkTingmai: public TriggerSkill {
public:
    IkTingmai(): TriggerSkill("iktingmai") {
        events << DamageCaused << FinishJudge;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                judge->pattern = (judge->card->isRed() ? "red" : "black");
                if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge && judge->card->isRed())
                    player->obtainCard(judge->card);
            }
            return skill_list;
        }
        DamageStruct damage = data.value<DamageStruct>();
        if (player->isAlive() && damage.nature == DamageStruct::Thunder && !damage.to->isChained()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                skill_list.insert(p, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *owner) const{
        if (owner->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *owner) const{
        JudgeStruct judge;
        judge.who = player;
        judge.reason = objectName();
        judge.pattern = ".";
        judge.good = true;
        judge.play_animation = false;
        room->judge(judge);

        if (judge.pattern == "black") {
            DamageStruct damage = data.value<DamageStruct>();
            LogMessage log;
            log.type = "#IkTingmai";
            log.from = owner;
            log.to << player;
            log.arg  = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class IkHuangzhen: public OneCardViewAsSkill {
public:
    IkHuangzhen(): OneCardViewAsSkill("ikhuangzhen") {
        filter_pattern = "Slash";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
               && pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *acard = new ThunderSlash(originalCard->getSuit(), originalCard->getNumber());
        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

IkCangliuCard::IkCangliuCard()
{
    target_fixed = true;
}

void IkCangliuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->showAllCards(source);
    bool flag = true;
    foreach (const Card *card, source->getHandcards()) {
        if (card->isKindOf("Slash")) {
            flag = false;
            break;
        }
    }
    if (flag) {
        room->setPlayerMark(source, "ikcangliu_handcardnum", source->getHandcardNum());
        room->setPlayerMark(source, "IkCangliuInvoke", 1);
        room->setPlayerMark(source, "ikcangliucount", 0);
    }
}

IkCangliuSlash::IkCangliuSlash()
{
    handling_method = MethodUse;
}

bool IkCangliuSlash::targetFixed() const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("ikcangliu");
    slash->deleteLater();
    return slash->targetFixed();
}

bool IkCangliuSlash::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("ikcangliu");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

bool IkCangliuSlash::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("ikcangliu");
    slash->deleteLater();
    return slash->targetsFeasible(targets, Self);
}

const Card *IkCangliuSlash::validate(CardUseStruct &use) const
{
    use.from->getRoom()->addPlayerMark(use.from, "ikcangliucount");
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("ikcangliu");
    return slash;
}

const Card *IkCangliuSlash::validateInResponse(ServerPlayer *user) const
{
    user->getRoom()->addPlayerMark(user, "ikcangliucount");
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("ikcangliu");
    return slash;
}

class IkCangliuVS : public ZeroCardViewAsSkill
{
public:
    IkCangliuVS() : ZeroCardViewAsSkill("ikcangliu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (!player->hasUsed("IkCangliuCard"))
            return new IkCangliuCard;
        return Slash::IsAvailable(player) && player->getMark("IkCangliuInvoke") > 0 && player->getMark("ikcangliucount") < 3;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "slash" && player->getMark("IkCangliuInvoke") > 0 && player->getMark("ikcangliucount") < 3;
    }

    virtual const Card *viewAs() const
    {
        if (!Self->hasUsed("IkCangliuCard"))
            return new IkCangliuCard;
        return new IkCangliuSlash;
    }
};

class IkCangliu : public TriggerSkill
{
public:
    IkCangliu() : TriggerSkill("ikcangliu")
    {
        events << EventPhaseChanging << CardsMoveOneTime << Death;
        view_as_skill = new IkCangliuVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerMark(p, "ikcangliucount", 0);
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (player->getMark("IkCangliuInvoke") > 0
                && ((move.to == player && move.to_place == Player::PlaceHand)
                    || (move.from == player && move.from_places.contains(Player::PlaceHand) && player->getMark("jnlinglie") == 0)))
                room->setPlayerMark(player, "IkCangliuInvoke", 0);
        } else if (triggerEvent == Death) {
            if (player->getMark("IkCangliuInvoke") > 0)
                room->setPlayerMark(player, "IkCangliuInvoke", 0);
        }
        return QStringList();
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const
    {
        if (card->isKindOf("IkCangliuCard"))
            return qrand() % 2 + 1;
        return player->getMark("ikcangliucount") + 2;
    }
};

IkaiSuiPackage::IkaiSuiPackage()
    :Package("ikai-sui")
{
    General *wind005 = new General(this, "wind005", "kaze", 3);
    wind005->addSkill(new IkHuahuan);
    wind005->addSkill(new IkQizhou);

    General *wind022 = new General(this, "wind022", "kaze", 3, false);
    wind022->addSkill(new IkShushen);
    wind022->addSkill(new IkQiaoxia);

    General *wind023 = new General(this, "wind023", "kaze", 3, false);
    wind023->addSkill(new IkXielun);
    wind023->addSkill(new IkCanyue);
    wind023->addSkill(new IkCanyueCount);
    wind023->addSkill(new IkCanyueClear);
    related_skills.insertMulti("ikcanyue", "#ikcanyue-count");
    related_skills.insertMulti("ikcanyue", "#ikcanyue-clear");
    wind023->addSkill(new IkJiuming);
    wind023->addSkill(new IkJiumingCount);
    related_skills.insertMulti("ikjiuming", "#ikjiuming-count");

    General *wind024 = new General(this, "wind024", "kaze");
    wind024->addSkill(new IkXiewang);
    wind024->addSkill(new IkShengzun);

    General *wind040 = new General(this, "wind040", "kaze", 3, false);
    wind040->addSkill(new IkYanyu);

    General *wind041 = new General(this, "wind041", "kaze", 3, false);
    wind041->addSkill(new IkWuyue);
    wind041->addSkill(new IkJuechong);
    wind041->addSkill(new IkJuechongTargetMod);
    related_skills.insertMulti("ikjuechong", "#ikjuechong-target");

    General *wind043 = new General(this, "wind043", "kaze", 3);
    wind043->addSkill(new IkXinhui);
    wind043->addSkill(new IkYongji);

    General *wind044 = new General(this, "wind044", "kaze", 3, false);
    wind044->addSkill(new IkMoqi);
    wind044->addSkill(new IkMoqiFinish);
    related_skills.insertMulti("ikmoqi", "#ikmoqi");
    wind044->addSkill(new IkTianbei);
    wind044->addRelateSkill("ikanshen");

    General *wind046 = new General(this, "wind046", "kaze", 3);
    wind046->addSkill(new IkJingmu);
    wind046->addSkill(new IkDuanmeng);

    General *wind049 = new General(this, "wind049", "kaze", 3);
    wind049->addSkill(new IkFanzhong);
    wind049->addSkill(new IkYuanyuan);

    General *wind052 = new General(this, "wind052", "kaze", 5);
    wind052->addSkill(new IkShangshiShu);
    wind052->addSkill(new IkShangshiShuTrigger);
    related_skills.insertMulti("ikshangshishu", "#ikshangshishu");
    wind052->addSkill(new IkChouhai);
    wind052->addSkill(new Skill("ikguiming", Skill::Compulsory));

    General *wind053 = new General(this, "wind053", "kaze");
    wind053->addSkill(new IkJueche);
    wind053->addSkill(new IkHewu);

    General *wind054 = new General(this, "wind054", "kaze", 3, false);
    wind054->addSkill(new IkYuzhi);
    wind054->addSkill(new IkLinglong);
    wind054->addSkill(new IkLinglongMax);
    wind054->addSkill(new IkLinglongTreasure);
    related_skills.insertMulti("iklinglong", "#iklinglong-horse");
    related_skills.insertMulti("iklinglong", "#iklinglong-treasure");

    General *bloom023 = new General(this, "bloom023", "hana");
    bloom023->addSkill(new IkBashou);

    General *bloom024 = new General(this, "bloom024", "hana");
    bloom024->addSkill(new IkXinban);

    General *bloom028 = new General(this, "bloom028", "hana", 3);
    bloom028->addSkill(new IkHuyin);
    bloom028->addSkill(new IkHuyinTrigger);
    bloom028->addSkill(new IkHuyinClear);
    related_skills.insertMulti("ikhuyin", "#ikhuyin");
    related_skills.insertMulti("ikhuyin", "#ikhuyin-clear");
    bloom028->addSkill(new IkHongcai);

    General *bloom033 = new General(this, "bloom033", "hana");
    bloom033->addSkill(new IkXunyuyouli);

    General *bloom040 = new General(this, "bloom040", "hana");
    bloom040->addSkill(new IkTianzuoyounai);
    bloom040->addSkill(new IkShenji);
    bloom040->addSkill(new IkKuanglu);
    bloom040->addSkill(new IkKuangluRecord);
    bloom040->addSkill(new IkKuangluMaxCards);
    related_skills.insert("ikkuanglu", "#ikkuanglu-record");
    related_skills.insert("ikkuanglu", "#ikkuanglu");

    General *bloom041 = new General(this, "bloom041", "hana");
    bloom041->addSkill(new IkBenhua);

    General *bloom043 = new General(this, "bloom043", "hana");
    bloom043->addSkill(new IkYaocheng);
    bloom043->addSkill(new IkHaobi);

    General *bloom044 = new General(this, "bloom044", "hana");
    bloom044->addSkill(new IkLingxue);
    bloom044->addSkill(new IkLingxueDraw);
    bloom044->addSkill(new IkLingxueMaxCards);
    related_skills.insertMulti("iklingxue", "#iklingxue-draw");
    related_skills.insertMulti("iklingxue", "#iklingxue-maxcard");

    General *bloom046 = new General(this, "bloom046", "hana", 5);
    bloom046->addSkill(new IkGonghu);
    bloom046->addSkill(new IkXuewu);
    bloom046->addSkill("ikbenghuai");

    General *bloom050 = new General(this, "bloom050", "hana");
    bloom050->addSkill(new IkQingwei);

    General *bloom051 = new General(this, "bloom051", "hana");
    bloom051->addSkill(new IkZhiyu);
    bloom051->addSkill(new IkZhiyuTargetMod);
    related_skills.insertMulti("ikzhiyu", "#ikzhiyu-tar");

    General *bloom058 = new General(this, "bloom058", "hana", 3, false);
    bloom058->addSkill(new IkChenqing);
    bloom058->addSkill(new IkMojing);

    General *snow022 = new General(this, "snow022", "yuki");
    snow022->addSkill(new Skill("ikxindu", Skill::NotCompulsory));
    snow022->addSkill(new IkFenxun);

    General *snow025 = new General(this, "snow025", "yuki", 3);
    snow025->addSkill(new IkHongrou);
    snow025->addSkill(new IkHuaxiao);

    General *snow032 = new General(this, "snow032", "yuki", 3, false);
    snow032->addSkill(new IkQiwu);
    snow032->addSkill(new IkShendao);

    General *snow033 = new General(this, "snow033", "yuki", 3);
    snow033->addSkill(new IkTianyan);
    snow033->addSkill(new IkCangwu);

    General *snow040 = new General(this, "snow040", "yuki");
    snow040->addSkill(new IkLingzhou);
    snow040->addSkill(new IkLingzhouClear);
    related_skills.insertMulti("iklingzhou", "#iklingzhou");
    snow040->addSkill(new IkMoqizhou);

    General *snow043 = new General(this, "snow043", "yuki");
    snow043->addSkill(new IkLingtong);
    snow043->addSkill(new IkXuexia);

    General *snow044 = new General(this, "snow044", "yuki");
    snow044->addSkill(new IkLunke);
    snow044->addSkill(new IkCangmie);

    General *snow047 = new General(this, "snow047", "yuki", 3, false);
    snow047->addSkill(new IkLinbu);
    snow047->addSkill(new IkMumu);

    General *snow050 = new General(this, "snow050", "yuki", 3);
    snow050->addSkill(new IkYanhuo);
    snow050->addSkill(new IkYaoyin);

    General *snow053 = new General(this, "snow053", "yuki", 3, false);
    snow053->addSkill(new IkCiyu);
    snow053->addSkill(new IkQingshe);
    snow053->addRelateSkill("ikbingling");

    General *snow055 = new General(this, "snow055", "yuki");
    snow055->addSkill(new IkHuangpo);
    snow055->addSkill(new IkHuangpoReturn);
    related_skills.insertMulti("ikhuangpo", "#ikhuangpo");
    snow055->addSkill(new IkYixiang);

    General *luna017 = new General(this, "luna017", "tsuki");
    luna017->addSkill(new IkChenyan);
    luna017->addSkill(new IkMoliao);
    luna017->addSkill("ikshengzun");

    General *luna019 = new General(this, "luna019", "tsuki");
    luna019->addSkill("thxiagong");
    luna019->addSkill(new IkZhange);
    luna019->addRelateSkill("ikshenti");

    General *luna020 = new General(this, "luna020", "tsuki", 3);
    luna020->addSkill(new IkZhizhai);
    luna020->addSkill(new IkLihui);

    General *luna021 = new General(this, "luna021", "tsuki");
    luna021->addSkill(new IkCunyang);
    luna021->addSkill(new IkFeishan);
    luna021->addSkill(new IkFeishanProhibit);
    luna021->addSkill(new SlashNoDistanceLimitSkill("ikfeishan"));
    related_skills.insertMulti("ikfeishan", "#ikfeishan");
    related_skills.insertMulti("ikfeishan", "#ikfeishan-slash-ndl");

    General *luna022 = new General(this, "luna022", "tsuki");
    luna022->addSkill(new IkNifa);
    luna022->addSkill(new IkGuyi);

    General *luna023 = new General(this, "luna023", "tsuki");
    luna023->addSkill("thjibu");
    luna023->addSkill(new IkShunqie);

    General *luna024 = new General(this, "luna024", "tsuki");
    luna024->addSkill("thjibu");
    luna024->addSkill(new IkLongya);
    luna024->addSkill(new IkLongyaMiss);
    related_skills.insertMulti("iklongya", "#iklongya-miss");

    General *luna025 = new General(this, "luna025", "tsuki", 3);
    luna025->addSkill(new IkHengmou);
    luna025->addSkill(new IkXincao);
    luna025->addSkill(new IkXincaoSlashNoDistanceLimit);
    related_skills.insertMulti("ikxincao", "#ikxincao-slash-ndl");

    General *luna026 = new General(this, "luna026", "tsuki", 3);
    luna026->addSkill(new IkKouzhu);
    luna026->addSkill(new IkKouzhuTrigger);
    luna026->addSkill(new IkKouzhuClear);
    related_skills.insertMulti("ikkouzhu", "#ikkouzhu");
    related_skills.insertMulti("ikkouzhu", "#ikkouzhu-clear");
    luna026->addSkill(new IkJiaojin);

    General *luna035 = new General(this, "luna035", "tsuki");
    luna035->addSkill(new IkSheqie);
    luna035->addSkill(new SlashNoDistanceLimitSkill("iksheqie"));
    related_skills.insertMulti("iksheqie", "#iksheqie-slash-ndl");
    luna035->addSkill(new IkYanzhou);

    General *luna040 = new General(this, "luna040", "tsuki", 3);
    luna040->addSkill(new IkHongta);
    luna040->addSkill(new IkNicu);
    luna040->addSkill(new IkNicuRecord);
    related_skills.insertMulti("iknicu", "#iknicu-record");

    General *luna043 = new General(this, "luna043", "tsuki", 3, false);
    luna043->addSkill(new IkZhoudu);
    luna043->addSkill(new IkKuangdi);
    luna043->addSkill(new IkBengying);

    General *luna044 = new General(this, "luna044", "tsuki");
    luna044->addSkill(new IkMingzhen);
    luna044->addSkill(new IkMingzhenInvalidity);
    related_skills.insertMulti("ikmingzhen", "#ikmingzhen-inv");
    luna044->addSkill(new IkPianxiang);

    General *luna050 = new General(this, "luna050", "tsuki");
    luna050->addSkill(new IkTingmai);
    luna050->addSkill(new IkHuangzhen);

    General *luna052 = new General(this, "luna052", "tsuki");
    luna052->addSkill(new Skill("ikhuisuo", Skill::Compulsory));
    luna052->addSkill(new IkCangliu);

    addMetaObject<IkXielunCard>();
    addMetaObject<IkJuechongCard>();
    addMetaObject<IkXinhuiCard>();
    addMetaObject<IkMoqiCard>();
    addMetaObject<IkTianbeiCard>();
    addMetaObject<IkDuanmengCard>();
    addMetaObject<IkFanzhongCard>();
    addMetaObject<IkYuzhiCard>();
    addMetaObject<IkXinbanCard>();
    addMetaObject<IkHuyinCard>();
    addMetaObject<IkXunyuyouliCard>();
    addMetaObject<IkYaochengCard>();
    addMetaObject<IkHaobiCard>();
    addMetaObject<IkZhiyuCard>();
    addMetaObject<IkZhiyuBasicCard>();
    addMetaObject<IkFenxunCard>();
    addMetaObject<IkHongrouCard>();
    addMetaObject<IkTianyanCard>();
    addMetaObject<IkCangwuCard>();
    addMetaObject<IkLingzhouCard>();
    addMetaObject<IkLingtongCard>();
    addMetaObject<IkLunkeCard>();
    addMetaObject<IkYaoyinCard>();
    addMetaObject<IkHuangpoCard>();
    addMetaObject<IkBinglingCard>();
    addMetaObject<IkZhangeCard>();
    addMetaObject<IkFeishanCard>();
    addMetaObject<IkXincaoCard>();
    addMetaObject<IkKouzhuCard>();
    addMetaObject<IkJiaojinCard>();
    addMetaObject<IkSheqieCard>();
    addMetaObject<IkCangliuCard>();
    addMetaObject<IkCangliuSlash>();

    skills << new IkAnshen << new IkAnshenRecord << new OthersNullifiedDistance
           << new IkLinbuFilter << new IkBingling << new IkBinglingMaxCards;
    related_skills.insertMulti("ikanshen", "#ikanshen-record");
    related_skills.insertMulti("ikbingling", "#ikbingling-max-cards");
}

ADD_PACKAGE(IkaiSui)
