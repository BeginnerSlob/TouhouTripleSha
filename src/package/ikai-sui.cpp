#include "ikai-sui.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"

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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (!TriggerSkill::triggerable(player)) return skill;
        RecoverStruct recover = data.value<RecoverStruct>();
        for (int i = 0; i < recover.recover; i++)
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@ikshushen", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkShushenTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = player->tag["IkShushenTarget"].value<ServerPlayer *>();
        player->tag.remove("IkShushenTarget");
        if (target) {
            target->drawCards(1, objectName());
            target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
            target->drawCards(1, objectName());
        }
        return false;
    }
};

class IkQiaoxia: public PhaseChangeSkill {
public:
    IkQiaoxia(): PhaseChangeSkill("ikqiaoxia") {
    }

    virtual bool triggerable(const ServerPlayer *ganfuren) const{
        foreach (const Card *card, ganfuren->getHandcards())
            if (ganfuren->isJilei(card))
                return false;
        return PhaseChangeSkill::triggerable(ganfuren)
            && ganfuren->getPhase() == Player::Start
            && !ganfuren->isKongcheng();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *ganfuren) const{
        Room *room = ganfuren->getRoom();
        int handcard_num = ganfuren->getHandcardNum();
        ganfuren->throwAllHandCards();
        if (handcard_num >= ganfuren->getHp())
            room->recover(ganfuren, RecoverStruct(ganfuren));
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

    return Self->inMyAttackRange(to_select, range_fix);
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
        frequency = NotFrequent;
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
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->hasFlag("ikcanyue_disabled"))
                        room->setPlayerFlag(p, "-ikcanyue_disabled");
            }
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->addPlayerMark(player, "ikcanyue");
        return false;
    }
};

class IkCanyueProhibit: public ProhibitSkill {
public:
    IkCanyueProhibit(): ProhibitSkill("#ikcanyue-prohibit") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasFlag("ikcanyue_disabled") && card->isKindOf("Slash");
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
                room->handleAcquireDetachSkills(player, detachList);
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
            room->handleAcquireDetachSkills(player, acquired_skills + detached_skills);
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
    button->setToolTip(skill->getDescription());

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

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play) {
                foreach (ServerPlayer *xiahou, room->findPlayersBySkillName(objectName())) {
                    if (!xiahou->canDiscard(xiahou, "he")) continue;
                    skill_list.insert(xiahou, QStringList(objectName()));
                }
            }
        } else if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)) {
            ServerPlayer *current = room->getCurrent();
            if (!current || current->getPhase() != Player::Play) return skill_list;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                QList<int> ids, disabled;
                QList<int> all_ids = move.card_ids;
                foreach (int id, move.card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (card->isKindOf("TrickCard") || card->isKindOf("Peach")) continue;
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

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *xiahou) const{
        if (triggerEvent == EventPhaseStart) {
            const Card *card = room->askForCard(xiahou, "..", "@ikyanyu-discard", QVariant(), objectName());
            if (card) {
                room->broadcastSkillInvoke(objectName());
                xiahou->addMark("IkYanyuDiscard" + QString::number(card->getTypeId()), 3);
            }
        } else if (triggerEvent == BeforeCardsMove)
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const{
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getPhase() != Player::Play) return false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        QList<int> ids, disabled;
        QList<int> all_ids = move.card_ids;
        foreach (int id, move.card_ids) {
            const Card *card = Sanguosha->getCard(id);
            if (card->isKindOf("TrickCard") || card->isKindOf("Peach")) {
                disabled << id;
                continue;
            }
            if (player->getMark("IkYanyuDiscard" + QString::number(card->getTypeId())) > 0)
                ids << id;
            else
                disabled << id;
        }
        if (ids.isEmpty()) return false;
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
                player->removeMark("YanyuDiscard" + QString::number(card->getTypeId()));
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
                target->obtainCard(card);
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() == Player::NotActive && move.from && move.from->isAlive()
            && move.from->objectName() != player->objectName()
            && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
            foreach (int id, move.card_ids)
                if (Sanguosha->getCard(id)->getTypeId() == Card::TypeBasic)
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

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(1, "ikwuyue");
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

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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

class IkXinhui: public TriggerSkill {
public:
    IkXinhui(): TriggerSkill("ikxinhui") {
        events << PreDamageDone << EventPhaseEnd;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
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
        if (player->askForSkillInvoke(objectName())) {
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

class IkYongji: public TriggerSkill {
public:
    IkYongji(): TriggerSkill("ikyongji") {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
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

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
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
    room->handleAcquireDetachSkills(effect.from, "-ikmoqi|-iktianbei");
    if (effect.from->isWounded())
        room->recover(effect.from, RecoverStruct(effect.from));
    room->acquireSkill(effect.to, "ikanshen");
    if (effect.to != effect.from)
        effect.to->drawCards(2, "iktianbei");
}

class IkTianbei: public ZeroCardViewAsSkill {
public:
    IkTianbei(): ZeroCardViewAsSkill("iktianbei") {
        frequency = Limited;
    }

    virtual const Card *viewAs() const{
        return new IkTianbeiCard;
    }
};

class IkAnshen: public TriggerSkill {
public:
    IkAnshen(): TriggerSkill("ikanshen") {
        events << BeforeCardsMove;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *anshen_user = room->getTag("ikanshen_user").value<ServerPlayer *>();
        if (anshen_user) {
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(anshen_user))) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
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
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
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

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
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
                    if (player->getMark(markname) > 0)
                        room->setPlayerMark(player, markname, 0);
                }
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const{
        player->drawCards(2, objectName());
        return false;
    }
};

class IkXiaorui: public TriggerSkill {
public:
    IkXiaorui(): TriggerSkill("ikxiaorui") {
        events << EventPhaseStart;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        QMap<ServerPlayer *, QStringList> skill;
        if (player->getPhase() != Player::Finish)
            return skill;
        foreach (ServerPlayer *yuejin, room->findPlayersBySkillName(objectName())) {
            if (yuejin == player) continue;
            if (yuejin->canDiscard(yuejin, "h"))
                skill.insert(yuejin, QStringList(objectName()));
        }
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *yuejin) const{
        if (room->askForCard(yuejin, ".Basic", "@ikxiaorui", QVariant(), objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *yuejin) const{
        if (!room->askForCard(player, "^BasicCard", "@ikxiaorui-discard"))
            room->damage(DamageStruct(objectName(), yuejin, player));
        return false;
    }
};

IkXinbanCard::IkXinbanCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool IkXinbanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
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
        return !player->hasUsed("IkXinbanCard") && !player->isNude();
    }
};

class IkHuyin: public TriggerSkill {
public:
    IkHuyin(): TriggerSkill("ikhuyin") {
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *yangxiu, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(yangxiu)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getPhase() == Player::NotActive || current->isDead() || !damage.from)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *yangxiu, QVariant &data, ServerPlayer *) const{
        if (yangxiu->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *yangxiu, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        QString choice = room->askForChoice(yangxiu, objectName(), "BasicCard+EquipCard+TrickCard");

        LogMessage log;
        log.type = "#IkHuyin";
        log.from = damage.from;
        log.arg = choice;
        room->sendLog(log);

        QStringList huyin_list = damage.from->tag[objectName()].toStringList();
        if (huyin_list.contains(choice)) return false;
        huyin_list.append(choice);
        damage.from->tag[objectName()] = QVariant::fromValue(huyin_list);
        QString _type = choice + "|.|.|hand"; // Handcards only
        room->setPlayerCardLimitation(damage.from, "use,response,discard", _type, true);

        QString type_name = choice.replace("Card", "").toLower();
        if (damage.from->getMark("@huyin_" + type_name) == 0)
            room->addPlayerMark(damage.from, "@huyin_" + type_name);

        return false;
    }
};

class IkHuyinClear: public TriggerSkill {
public:
    IkHuyinClear(): TriggerSkill("#ikhuyin-clear") {
        events << EventPhaseChanging << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
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
            QStringList huyin_list = player->tag["ikhuyin"].toStringList();
            if (!huyin_list.isEmpty()) {
                LogMessage log;
                log.type = "#IkHuyinClear";
                log.from = player;
                room->sendLog(log);

                foreach (QString huyin_type, huyin_list) {
                    room->removePlayerCardLimitation(player, "use,response,discard", huyin_type + "|.|.|hand$1");
                    QString type_name = huyin_type.replace("Card", "").toLower();
                    room->setPlayerMark(player, "@huyin_" + type_name, 0);
                }
                player->tag.remove("ikhuyin");
            }
        }

        return QStringList();
    }
};

class IkHongce: public TriggerSkill {
public:
    IkHongce(): TriggerSkill("ikhongce") {
        events << TargetSpecified;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard")) {
            foreach (ServerPlayer *yangxiu, room->findPlayersBySkillName(objectName()))
                if (use.to.contains(yangxiu))
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

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const{
        player->drawCards(1, objectName());
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() > 1) {
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

IkShenyuCard::IkShenyuCard() {
}

bool IkShenyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total = Self->getAliveSiblings().length() + 1;
    return targets.length() < total / 2 - 1 && to_select != Self;
}

void IkShenyuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->setPlayerProperty(effect.to, "ikshenyu_from", QVariant::fromValue(effect.from->objectName()));
    effect.to->gainMark("@yushou");
}

class IkShenyuViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkShenyuViewAsSkill(): ZeroCardViewAsSkill("ikshenyu") {
        response_pattern = "@@ikshenyu";
    }

    virtual const Card *viewAs() const{
        return new IkShenyuCard;
    }
};

class IkShenyu: public TriggerSkill {
public:
    IkShenyu(): TriggerSkill("ikshenyu") {
        events << EventPhaseChanging << Death;
        view_as_skill = new IkShenyuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(objectName(), true))
                return QStringList();
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->tag["ikshenyu_from"].toString() == player->objectName()) {
                    room->setPlayerProperty(p, "ikshenyu_from", QVariant());
                    room->setPlayerMark(p, "@yushou", 0);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->tag["ikshenyu_from"].toString() == player->objectName()) {
                    room->setPlayerProperty(p, "ikshenyu_from", QVariant());
                    room->setPlayerMark(p, "@yushou", 0);
                }
            }
            if (!TriggerSkill::triggerable(player) || player->aliveCount() <= 3)
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->askForUseCard(player, "@@ikshenyu", "@ikshenyu");
        return false;
    }
};

class IkShenyuDistance: public DistanceSkill {
public:
    IkShenyuDistance(): DistanceSkill("#ikshenyu") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if (to->getMark("@yushou") > 0 && from->getMark("@yushou") == 0
            && from->objectName() != to->property("ikshenyu_from").toString())
            return 1;
        return 0;
    }
};

class IkKuangzhan: public TriggerSkill {
public:
    IkKuangzhan(): TriggerSkill("ikkuangzhan") {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
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
    wind023->addSkill(new IkCanyueProhibit);
    wind023->addSkill(new IkCanyueClear);
    related_skills.insertMulti("ikcanyue", "#ikcanyue-count");
    related_skills.insertMulti("ikcanyue", "#ikcanyue-prohibit");
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

    General *bloom023 = new General(this, "bloom023", "hana");
    bloom023->addSkill(new IkXiaorui);

    General *bloom024 = new General(this, "bloom024", "hana");
    bloom024->addSkill(new IkXinban);

    General *bloom028 = new General(this, "bloom028", "hana", 3);
    bloom028->addSkill(new IkHuyin);
    bloom028->addSkill(new IkHuyinClear);
    bloom028->addSkill(new IkHongce);
    related_skills.insertMulti("ikhuyin", "#ikhuyin-clear");

    General *bloom033 = new General(this, "bloom033", "hana");
    bloom033->addSkill(new IkShenyu);
    bloom033->addSkill(new IkShenyuDistance);
    related_skills.insert("ikshenyu", "#ikshenyu");

    General *bloom040 = new General(this, "bloom040", "hana");
    bloom040->addSkill(new IkKuangzhan);
    bloom040->addSkill(new IkShenji);

    addMetaObject<IkXielunCard>();
    addMetaObject<IkJuechongCard>();
    addMetaObject<IkMoqiCard>();
    addMetaObject<IkTianbeiCard>();
    addMetaObject<IkDuanmengCard>();
    addMetaObject<IkXinbanCard>();
    addMetaObject<IkShenyuCard>();

    skills << new IkAnshen << new IkAnshenRecord;
    related_skills.insertMulti("ikanshen", "#ikanshen-record");
}

ADD_PACKAGE(IkaiSui)