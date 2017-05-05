#include "touhou-hana.h"

#include "general.h"
#include "skill.h"
#include "room.h"
#include "carditem.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "card.h"

class ThHuaji: public TriggerSkill
{
public:
    ThHuaji(): TriggerSkill("thhuaji")
    {
        events << CardUsed << CardResponded << NullificationEffect;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            ServerPlayer *current = room->getCurrent();
            if (TriggerSkill::triggerable(current) && current->getPhase() != Player::NotActive && player != current
                && !use.card->isKindOf("Nullification") && (use.card->isNDTrick() || use.card->isKindOf("BasicCard"))) {
                ask_who = current;
                return QStringList(objectName());
            }
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            ServerPlayer *current = room->getCurrent();
            if (TriggerSkill::triggerable(current) && current->getPhase() != Player::NotActive && player != current
                && resp.m_isUse && (resp.m_card->isNDTrick() || resp.m_card->isKindOf("BasicCard"))) {
                ask_who = current;
                return QStringList(objectName());
            }
        } else if (triggerEvent == NullificationEffect) {
            ServerPlayer *current = room->getCurrent();
            if (TriggerSkill::triggerable(current) && current->getPhase() != Player::NotActive && player != current) {
                ask_who = current;
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (room->askForCard(ask_who, ".|black", "@thhuajiuse:" + player->objectName(), data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString name = use.card->getClassName();
            if (name.endsWith("Slash"))
                name = "Slash";
            QString str = use.card->objectName();
            if (str.endsWith("_slash"))
                str = "slash";
            if (room->askForCard(player, name, "@thhuaji:::" + str)) return false;
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue(use);
        } else if (triggerEvent == CardResponded) {
            QString name, str;
            if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                name = resp.m_card->getClassName();
                str = resp.m_card->objectName();
            }
            if (name.endsWith("Slash"))
                name = "Slash";
            if (str.endsWith("_slash"))
                str = "slash";
            if (room->askForCard(player, name, "@thhuaji:::" + str)) return false;
            room->setPlayerFlag(player, "thhuaji_cancel");
        } else if (triggerEvent == NullificationEffect) {
            if (room->askForCard(player, "Nullification", "@thhuaji:::nullification"))
                return false;
            return true;
        }
        return false;
    }
};

class ThFeizhan: public MaxCardsSkill {
public:
    ThFeizhan(): MaxCardsSkill("thfeizhan$") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasLordSkill(objectName())) {
            int n = 0;
            foreach (const Player *p, target->getAliveSiblings())
                if (p->getKingdom() == "hana")
                    ++n;
            return n;
        } else {
            return 0;
        }
    }
};

ThJiewuCard::ThJiewuCard()
{
}

bool ThJiewuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isNude() && Self->inMyAttackRange(to_select);
}

void ThJiewuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    int card_id = room->askForCardChosen(source, target, "he", "thjiewu");
    room->obtainCard(source, card_id, false);
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_thjiewu");
    if (target->canSlash(source, slash, false)) {
        CardUseStruct use;
        use.card = slash;
        use.from = target;
        use.to << source;
        room->useCard(use, false);
    } else
        delete slash;
}

class ThJiewu : public ZeroCardViewAsSkill
{
public:
    ThJiewu() : ZeroCardViewAsSkill("thjiewu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThJiewuCard");
    }

    virtual const Card *viewAs() const
    {
        return new ThJiewuCard;
    }
};

class ThJiewuTrigger : public TriggerSkill
{
public:
    ThJiewuTrigger() : TriggerSkill("#thjiewu")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (player && player->isAlive() && use.card->isKindOf("Slash") && use.card->getSkillName(false) == "_thjiewu")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->sendCompulsoryTriggerLog(player, "thjiewu");
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to.toSet())
            p->addQinggangTag(use.card);
        return false;
    }
};


class ThGenxing: public TriggerSkill {
public:
    ThGenxing(): TriggerSkill("thgenxing") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getHp() == 1
            && player->getPhase() == Player::Start
            && player->getMark("@genxing") <= 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        LogMessage log;
        log.type = "#ThGenxing";
        log.from = player;
        log.arg = objectName();
        log.arg2 = QString::number(player->getHp());
        room->sendLog(log);

        room->setPlayerMark(player, "@genxing", 1);
        QStringList choices;
        if (player->isWounded())
            choices << "recover";
        choices << "draw";

        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "recover") {
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        } else
            player->drawCards(2);

        room->changeMaxHpForAwakenSkill(player);
        room->acquireSkill(player, "thmopao");
        return false;
    }
};

class ThMopao: public TriggerSkill {
public:
    ThMopao(): TriggerSkill("thmopao") {
        events << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardResponseStruct resp = data.value<CardResponseStruct>();
        if (resp.m_card->isKindOf("Jink"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thmopao", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThMopaoTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThMopaoTarget"].value<ServerPlayer *>();
        player->tag.remove("ThMopaoTarget");
        if (target) {
            target->drawCards(1);
            room->damage(DamageStruct(objectName(), player, target, 1, DamageStruct::Fire));
        }

        return false;
    }
};

class ThBian: public TriggerSkill {
public:
    ThBian(): TriggerSkill("thbian") {
        events << TargetConfirming;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        TriggerList skill_list;
        if (player->hasFlag("Global_Dying")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Peach")) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->canDiscard(p, "he"))
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        }

        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        if (room->askForCard(ask_who, "TrickCard", "@thbian:" + player->objectName(), data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << player->objectName();
        data = QVariant::fromValue(use);

        return false;
    }
};

class ThGuihang : public TriggerSkill
{
public:
    ThGuihang() : TriggerSkill("thguihang")
    {
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who->getHp() > 0 || dying.who->isDead())
            return QStringList();
        if (dying.who->isKongcheng())
            return QStringList();

        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        int card_id;
        if (player == dying.who)
            card_id = room->askForCardShow(player, player, "thguihang")->getId();
        else
            card_id = room->askForCardChosen(player, dying.who, "h", objectName());

        room->showCard(dying.who, card_id);
        const Card *card = Sanguosha->getCard(card_id);
        if (card->isRed()) {
            room->throwCard(card_id, dying.who);
            RecoverStruct recover;
            recover.who = player;
            room->recover(dying.who, recover);
        }

        return false;
    }
};

ThWujianCard::ThWujianCard() {
}

bool ThWujianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    int rangefix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);
    }

    if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getId() == subcards.first())
        rangefix += 1;

    return Self->inMyAttackRange(to_select, rangefix);
}

void ThWujianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->addPlayerMark(targets.first(), "@gap");
    source->tag["ThWujianUsed"] = true;
}

class ThWujian: public OneCardViewAsSkill {
public:
    ThWujian(): OneCardViewAsSkill("thwujian") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThWujianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThWujianCard *card = new ThWujianCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThWujianClear: public TriggerSkill {
public:
    ThWujianClear(): TriggerSkill("#thwujian") {
        events << EventPhaseStart << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == EventPhaseStart || triggerEvent == Death) {
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return QStringList();
            }
            if (!player->tag.value("ThWujianUsed", false).toBool())
                return QStringList();
            bool invoke = false;
            if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) || triggerEvent == Death)
                invoke = true;
            if (!invoke)
                return QStringList();
            QList<ServerPlayer *> players = room->getAllPlayers();
            foreach (ServerPlayer *player, players)
                room->setPlayerMark(player, "@gap", 0);
            player->tag.remove("ThWujianUsed");
        }

        return QStringList();
    }
};

class ThWujianDistanceSkill: public DistanceSkill{
public:
    ThWujianDistanceSkill(): DistanceSkill("#thwujian-distance") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        return from->getMark("@gap");
    }
};

class ThXuelan: public TriggerSkill {
public:
    ThXuelan(): TriggerSkill("thxuelan") {
        events << CardEffected;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const {
        TriggerList skill_list;
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isKindOf("Peach") && !effect.nullified) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner->canDiscard(owner, "he"))
                    skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        if (room->askForCard(ask_who, ".|red", "@thxuelan:" + player->objectName(), data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }

        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (player->getMaxHp() <= player->getGeneralMaxHp()) {
            room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);

            LogMessage log;
            log.type = "#GainMaxHp";
            log.from = player;
            log.arg = QString::number(1);
            room->sendLog(log);

            LogMessage log2;
            log2.type = "#GetHp";
            log2.from = player;
            log2.arg = QString::number(player->getHp());
            log2.arg2 = QString::number(player->getMaxHp());
            room->sendLog(log2);
        }

        effect.nullified = true;

        data = QVariant::fromValue(effect);

        return false;
    }
};

class ThXinwang: public TriggerSkill {
public:
    ThXinwang(): TriggerSkill("thxinwang") {
        events << CardsMoveOneTime << CardUsed << CardResponded;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player) || player == room->getCurrent())
            return QStringList();
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player) return QStringList();;

            if (triggerEvent == CardsMoveOneTime) {
                CardMoveReason reason = move.reason;
                if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    const Card *card;
                    QStringList list;
                    int i = 0;
                    foreach (int card_id, move.card_ids) {
                        card = Sanguosha->getCard(card_id);
                        if (card->getSuit() == Card::Heart && (move.from_places[i] == Player::PlaceHand
                                                               || move.from_places[i] == Player::PlaceEquip))
                            list << objectName();
                        i ++;
                    }
                    return list;
                }
            }
        } else {
            const Card *card = NULL;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                card = use.card;
            } else if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                card = resp.m_card;
            }
            if (card && card->getSuit() == Card::Heart) {
                return QStringList(objectName());
            }
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
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (p->isWounded())
                targets << p;
        ServerPlayer *target = NULL;
        if (!targets.isEmpty())
            target = room->askForPlayerChosen(player, targets, objectName(), "@thxinwang", true);
        if (target)
            room->recover(target, RecoverStruct(player));
        else
            player->drawCards(1);
        return false;
    }
};

class ThJuedu: public TriggerSkill {
public:
    ThJuedu():TriggerSkill("thjuedu") {
        events << Death << GameStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == GameStart) {
            if (player == NULL) {
                const TriggerSkill *benghuai = Sanguosha->getTriggerSkill("ikbenghuai");
                if (benghuai)
                    room->getThread()->addTriggerSkill(benghuai);
            }
            return QStringList();
        }
        if (player && player->hasSkill(objectName())) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();

            ServerPlayer *killer = death.damage ? death.damage->from : NULL;

            if (killer)
                if (killer != player)
                    return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const {
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *killer = death.damage->from;
        room->addPlayerMark(killer, "@juedu");
        room->acquireSkill(killer, "ikbenghuai");

        return false;
    }
};

class ThTingwu: public TriggerSkill
{
public:
    ThTingwu(): TriggerSkill("thtingwu")
    {
        events << DamageComplete << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == EventPhaseChanging)
            player->setMark(objectName(), 0);
        else if (TriggerSkill::triggerable(damage.from)
                 && damage.from->getPhase() == Player::Play
                 && !player->isChained()
                 && damage.nature == DamageStruct::Thunder
                 && damage.from->getMark(objectName()) < 2) {
            ask_who = damage.from;
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        if (ask_who->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        ask_who->addMark(objectName());
        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = objectName();
        judge.who = ask_who;
        room->judge(judge);

        if (judge.isGood() && player->isAlive() && !player->isRemoved()) {
            QList<ServerPlayer *> targets;
            targets << room->findPlayer(player->getLastAlive(1, false)->objectName())
                    << room->findPlayer(player->getNextAlive(1, false)->objectName());
            if (!targets.isEmpty()) {
                ServerPlayer *target = room->askForPlayerChosen(ask_who, targets, objectName());
                room->damage(DamageStruct(objectName(), ask_who, target, 1, DamageStruct::Thunder));
            }
        }

        return false;
    }
};

class ThYuchang: public FilterSkill {
public:
    ThYuchang(): FilterSkill("thyuchang") {
    }

    virtual bool viewFilter(const Card* to_select) const {
        return to_select->isKindOf("Slash") && to_select->getSuit() == Card::Club;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        ThunderSlash *slash = new ThunderSlash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class ThYuchangTargetMod: public TargetModSkill {
public:
    ThYuchangTargetMod(): TargetModSkill("#thyuchang-target") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const {
        if (from->hasSkill("thyuchang") && card->isKindOf("ThunderSlash"))
            return 1000;
        else
            return 0;
    }
};

ThXihuaCard::ThXihuaCard() {
    will_throw = false;
    handling_method = MethodNone;
}

bool ThXihuaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;
    Duel *duel = new Duel(NoSuit, 0);
    duel->setSkillName("_thxihua");
    duel->deleteLater();
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_thxihua");
    slash->deleteLater();
    if (Self->isProhibited(to_select, duel) && !Self->canSlash(to_select, slash, false))
        return false;
    if (Self->isCardLimited(duel, MethodUse) && Self->isCardLimited(slash, MethodUse))
        return false;
    return true;
}

void ThXihuaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    source->addToPile("joke", this, false);
    ServerPlayer *target = targets[0];
    QStringList choices;
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("_thxihua");
    slash->deleteLater();
    Duel *duel = new Duel(NoSuit, 0);
    duel->setSkillName("_thxihua");
    duel->deleteLater();
    if (slash->isAvailable(source) && source->canSlash(target, slash, false))
        choices << "slash";

    if (duel->isAvailable(source) && !source->isProhibited(target, duel))
        choices << "duel";

    if (!choices.isEmpty()) {
        QString choice = room->askForChoice(source, "thxihua", choices.join("+"), QVariant::fromValue(target));
        if (choice == "slash") {
            CardUseStruct use;
            use.from = source;
            use.to << target;
            use.card = slash;
            room->useCard(use, false);
        } else if (choice == "duel") {
            CardUseStruct use;
            use.from = source;
            use.to << target;
            use.card = duel;
            room->useCard(use, false);
        }
    }

    source->clearOnePrivatePile("joke");
}

class ThXihua: public OneCardViewAsSkill
{
public:
    ThXihua(): OneCardViewAsSkill("thxihua")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ThXihuaCard *card = new ThXihuaCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ThXihuaCard");
    }
};

class ThXihuaTrigger: public TriggerSkill
{
public:
    ThXihuaTrigger() : TriggerSkill("#thxihua")
    {
        events << Predamage;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->getSkillName() == "thxihua")
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const {
        DamageStruct damage = data.value<DamageStruct>();
        int id = -1;
        ServerPlayer *owner, *victim;
        if (damage.from && !damage.from->getPile("joke").isEmpty()) {
            id = damage.from->getPile("joke").first();
            owner = damage.from;
            victim = damage.to;
        } else if (damage.to && !damage.to->getPile("joke").isEmpty()) {
            id = damage.to->getPile("joke").first();
            owner = damage.to;
            victim = damage.from;
        }

        if (id == -1)
            return true;

        room->showCard(owner, id);
        if (Sanguosha->getCard(id)->isKindOf("Slash")) {
            if (owner->canDiscard(victim, "h")) {
                int card_id = room->askForCardChosen(owner, victim, "h", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, victim, owner);
            }
        } else
            return true;

        return false;
    }
};

ThMimengDialog *ThMimengDialog::getInstance(const QString &object, bool left, bool right,
                                      bool play_only, bool slash_combined, bool delayed_tricks) {
    static ThMimengDialog *instance = NULL;
    if (instance == NULL || instance->objectName() != object)
        instance = new ThMimengDialog(object, left, right, play_only, slash_combined, delayed_tricks);

    return instance;
}

ThMimengDialog::ThMimengDialog(const QString &object, bool left, bool right, bool play_only,
                               bool slash_combined, bool delayed_tricks)
    : object_name(object), play_only(play_only),
      slash_combined(slash_combined), delayed_tricks(delayed_tricks)
{
    setObjectName(object_name);
    setWindowTitle(Sanguosha->translate(object));
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    if (left) layout->addWidget(createLeft());
    if (right) layout->addWidget(createRight());
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

bool ThMimengDialog::isButtonEnabled(const QString &button_name) const{
    const Card *card = map[button_name];
    QString allowings = Self->property("allowed_thmimeng_dialog_buttons").toString();
    QStringList ban_list;
    if (object_name == "thmimeng")
        ban_list << "ExNihilo" << "AmazingGrace" << "Snatch" << "GodSalvation" << "ArcheryAttack"
                 << "Drowning" << "BurningCamps" << "LureTiger" << "KnownBoth";
    if (object_name == "ikxieke" && Self->aliveCount() == 2)
        ban_list << "Jink" << "Analeptic" << "Peach";
    if (allowings.isEmpty())
        return !ban_list.contains(card->getClassName()) && !Self->isCardLimited(card, Card::MethodUse, true) && card->isAvailable(Self);
    else {
        if (!allowings.split("+").contains(card->objectName())) // for IkMojing
            return false;
        else
            return !ban_list.contains(card->getClassName()) && !Self->isCardLimited(card, Card::MethodUse, true) && card->isAvailable(Self);
    }
}

void ThMimengDialog::popup() {
    if (play_only && Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        Self->tag.remove(object_name);
        emit onButtonClick();
        return;
    }

    bool has_enabled_button = false;
    foreach (QAbstractButton *button, group->buttons()) {
        bool enabled = isButtonEnabled(button->objectName());
        if (enabled)
            has_enabled_button = true;
        button->setEnabled(enabled);
    }
    if (!has_enabled_button) {
        Self->tag.remove(object_name);
        emit onButtonClick();
        return;
    }

    Self->tag.remove(object_name);
    exec();
}

void ThMimengDialog::selectCard(QAbstractButton *button){
    const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card);
    if (button->objectName().contains("slash")) {
        if (objectName() == "ikguihuo")
            Self->tag["IkGuihuoSlash"] = button->objectName();
    }
    emit onButtonClick();
    accept();
}

QGroupBox *ThMimengDialog::createLeft() {
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach (const Card *card, cards) {
        if (card->getTypeId() == Card::TypeBasic && !map.contains(card->objectName())
            && !ServerInfo.Extensions.contains("!" + card->getPackage())
            && !(slash_combined && map.contains("slash") && card->objectName().contains("slash"))) {
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setParent(this);
            layout->addWidget(createButton(c));

            if (!slash_combined && objectName() == "ikguihuo" && card->objectName() == "slash"
                && !ServerInfo.Extensions.contains("!maneuvering")) {
                Card *c2 = Sanguosha->cloneCard(card->objectName());
                c2->setParent(this);
                layout->addWidget(createButton(c2));
            }
        }
    }

    layout->addStretch();
    box->setLayout(layout);
    return box;
}

QGroupBox *ThMimengDialog::createRight() {
    QGroupBox *box = new QGroupBox(Sanguosha->translate("trick"));
    QHBoxLayout *layout = new QHBoxLayout;

    QGroupBox *box1 = new QGroupBox(Sanguosha->translate("single_target_trick"));
    QVBoxLayout *layout1 = new QVBoxLayout;

    QGroupBox *box2 = new QGroupBox(Sanguosha->translate("multiple_target_trick"));
    QVBoxLayout *layout2 = new QVBoxLayout;

    QGroupBox *box3 = new QGroupBox(Sanguosha->translate("delayed_trick"));
    QVBoxLayout *layout3 = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach(const Card *card, cards){
        if (card->getTypeId() == Card::TypeTrick && (delayed_tricks || card->isNDTrick())
            && !map.contains(card->objectName())
            && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setSkillName(object_name);
            c->setParent(this);

            QVBoxLayout *layout;
            if (c->isKindOf("DelayedTrick"))
                layout = layout3;
            else if (c->isKindOf("SingleTargetTrick"))
                layout = layout1;
            else
                layout = layout2;
            layout->addWidget(createButton(c));
        }
    }

    box->setLayout(layout);
    box1->setLayout(layout1);
    box2->setLayout(layout2);
    box3->setLayout(layout3);

    layout1->addStretch();
    layout2->addStretch();
    layout3->addStretch();

    layout->addWidget(box1);
    layout->addWidget(box2);
    if (delayed_tricks)
        layout->addWidget(box3);
    return box;
}

QAbstractButton *ThMimengDialog::createButton(const Card *card){
    if (card->objectName() == "slash" && map.contains(card->objectName()) && !map.contains("normal_slash")) {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate("normal_slash"));
        button->setObjectName("normal_slash");
        button->setToolTip(card->getDescription());

        map.insert("normal_slash", card);
        group->addButton(button);

        return button;
    } else {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(card->objectName()));
        button->setObjectName(card->objectName());
        button->setToolTip(card->getDescription());

        map.insert(card->objectName(), card);
        group->addButton(button);

        return button;
    }
}

ThMimengCard::ThMimengCard() {
    mute = true;
    will_throw = false;
}

bool ThMimengCard::targetFixed() const {
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            card->addSubcard(oc);
            card->deleteLater();
            return card && card->targetFixed();
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("thmimeng").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    new_card->addSubcard(oc);
    new_card->setSkillName("thmimeng");
    new_card->setCanRecast(false);
    return new_card && new_card->targetFixed();
}

bool ThMimengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            card->addSubcard(oc);
            card->deleteLater();
            return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("thmimeng").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    new_card->addSubcard(oc);
    new_card->setSkillName("thmimeng");
    new_card->setCanRecast(false);
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool ThMimengCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        Card *card = NULL;
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            card->addSubcard(oc);
            card->deleteLater();
            return card && card->targetsFeasible(targets, Self);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("thmimeng").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    new_card->addSubcard(oc);
    new_card->setSkillName("thmimeng");
    new_card->setCanRecast(false);
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *ThMimengCard::validate(CardUseStruct &card_use) const {
    ServerPlayer *thmimeng_general = card_use.from;

    Room *room = thmimeng_general->getRoom();
    QString to_use = user_string;

    if (user_string == "slash"
        && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        QStringList use_list;
        use_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        to_use = room->askForChoice(thmimeng_general, "thmimeng_skill_slash", use_list.join("+"));
    }

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("thmimeng");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();

    return use_card;
}

const Card *ThMimengCard::validateInResponse(ServerPlayer *user) const{
    Room *room = user->getRoom();

    QString to_use;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        use_list << "peach";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            use_list << "analeptic";
        to_use = room->askForChoice(user, "thmimeng_skill_saveself", use_list.join("+"));
    } else if (user_string == "slash") {
        QStringList use_list;
        use_list << "slash";
        if (!ServerInfo.Extensions.contains("!maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        to_use = room->askForChoice(user, "thmimeng_skill_slash", use_list.join("+"));
    } else
        to_use = user_string;

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("thmimeng");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    return use_card;
}

class ThMimeng: public OneCardViewAsSkill {
public:
    ThMimeng(): OneCardViewAsSkill("thmimeng") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (player->getHandcardNum() != 1 || pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (pattern == "peach" && player->getMark("Global_PreventPeach") > 0) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false;
        }
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return player->getHandcardNum() == 1;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            ThMimengCard *card = new ThMimengCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        const Card *c = Self->tag.value("thmimeng").value<const Card *>();
        if (c) {
            ThMimengCard *card = new ThMimengCard;
            card->setUserString(c->objectName());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    virtual QDialog *getDialog() const {
        return ThMimengDialog::getInstance("thmimeng");
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const {
        return player->getHandcardNum() == 1;
    }
};

class ThAnyun: public TriggerSkill {
public:
    ThAnyun(): TriggerSkill("thanyun") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (player->getPhase() == Player::Draw && TriggerSkill::triggerable(player)) {
            return QStringList(objectName());
        } else if (player->getPhase() == Player::Finish && player->hasFlag("thanyun")) {
            return QStringList(objectName());
        }

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->getPhase() == Player::Draw && player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
            return true;
        } else if (player->getPhase() == Player::Finish) {
            room->broadcastSkillInvoke(objectName(), 3);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->getPhase() == Player::Draw) {
            room->setPlayerFlag(player, "thanyun");
            return true;
        } else {
            room->setPlayerFlag(player, "-thanyun");
            player->drawCards(2);
        }
        return false;
    }
};

ThQuanshanGiveCard::ThQuanshanGiveCard() {
    will_throw = false;
    handling_method = MethodNone;
}

bool ThQuanshanGiveCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && to_select != Self && !to_select->hasFlag("thquanshan");
}

void ThQuanshanGiveCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    bool can_draw = true;
    CardType typeId = Sanguosha->getCard(subcards.first())->getTypeId();
    foreach (int id, subcards) {
        const Card *card = Sanguosha->getCard(id);
        if (card->getTypeId() != typeId) {
            can_draw = false;
            break;
        }
    }

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), targets.first()->objectName(), "thquanshan", QString());
    room->obtainCard(targets.first(), this, reason);

    if (can_draw) {
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (p->hasFlag("thquanshan")) {
                p->drawCards(1, "thquanshan");
                break;
            }
    }
}

class ThQuanshanGive: public ViewAsSkill {
public:
    ThQuanshanGive(): ViewAsSkill("thquanshangive") {
        response_pattern = "@@thquanshangive!";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const {
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (cards.isEmpty())
            return NULL;

        Card *card = new ThQuanshanGiveCard;
        card->addSubcards(cards);
        return card;
    }
};

ThQuanshanCard::ThQuanshanCard() {
}

bool ThQuanshanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void ThQuanshanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    ServerPlayer *target = targets.first();
    room->setPlayerFlag(source, "thquanshan");
    bool used = room->askForUseCard(target, "@@thquanshangive!", "@thquanshan", -1, MethodNone);
    if (!used) {
        QList<ServerPlayer *>beggars = room->getOtherPlayers(target);
        beggars.removeOne(source);
        if (beggars.isEmpty()) return;

        qShuffle(beggars);

        ServerPlayer *beggar = beggars.at(0);

        QList<int> to_give = target->handCards().mid(0, 1);
        ThQuanshanGiveCard *quanshan_card = new ThQuanshanGiveCard;
        quanshan_card->addSubcards(to_give);
        QList<ServerPlayer *> targets;
        targets << beggar;
        quanshan_card->use(room, target, targets);
        delete quanshan_card;
    }
    room->setPlayerFlag(source, "-thquanshan");
}

class ThQuanshan: public ZeroCardViewAsSkill {
public:
    ThQuanshan(): ZeroCardViewAsSkill("thquanshan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThQuanshanCard") && player->aliveCount() > 2;
    }

    virtual const Card *viewAs() const{
        return new ThQuanshanCard;
    }
};

class ThXiangang: public TriggerSkill {
public:
    ThXiangang(): TriggerSkill("thxiangang") {
        events << DamageInflicted;
        frequency = Frequent;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        JudgeStruct judge;
        judge.pattern = ".|club";
        judge.good = true;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        if (judge.isGood()) {
            LogMessage log;
            log.type = "#thxiangang";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            return true;
        }

        return false;
    }
};

ThDuanzuiCard::ThDuanzuiCard() {
}

bool ThDuanzuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void ThDuanzuiCard::onEffect(const CardEffectStruct &effect) const {
    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "h", objectName());
    room->showCard(effect.to, card_id);
    const Card *card = Sanguosha->getCard(card_id);
    if (card->isKindOf("Slash")) {
        CardUseStruct use;
        use.from = effect.from;
        use.to << effect.to;
        Duel *use_card = new Duel(NoSuit, 0);
        use_card->setCancelable(false);
        use_card->setSkillName("_thduanzui");
        use_card->deleteLater();
        use.card = use_card;
        if (!effect.from->isProhibited(effect.to, use_card))
            room->useCard(use);
    } else if (card->isKindOf("Jink") || card->isKindOf("Peach")) {
        CardUseStruct use;
        use.from = effect.from;
        use.to << effect.to;
        Slash *use_card = new Slash(NoSuit, 0);
        use_card->setSkillName("_thduanzui");
        use_card->deleteLater();
        use.card = use_card;
        if (effect.from->canSlash(effect.to, use_card, false))
            room->useCard(use);
    }
};

class ThDuanzui: public ZeroCardViewAsSkill {
public:
    ThDuanzui(): ZeroCardViewAsSkill("thduanzui") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThDuanzuiCard");
    }

    virtual const Card *viewAs() const{
        return new ThDuanzuiCard;
    }
};

class ThZheyinVS: public OneCardViewAsSkill
{
public:
    ThZheyinVS(): OneCardViewAsSkill("thzheyin")
    {
        response_pattern = "@@thzheyin";
        filter_pattern = ".|.|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        DummyCard *dummy = new DummyCard;
        dummy->addSubcard(originalCard);
        dummy->setSkillName(objectName());
        return dummy;
    }
};

class ThZheyin: public TriggerSkill {
public:
    ThZheyin(): TriggerSkill("thzheyin") {
        events << CardUsed << NullificationEffect << EventPhaseChanging;
        view_as_skill = new ThZheyinVS;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        TriggerList skill_list;
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("ThZheyin")) {
                        room->setPlayerFlag(p, "-ThZheyin");
                        room->removePlayerCardLimitation(p, "use", "TrickCard$0");
                    }
                }
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Nullification") && use.card->isNDTrick()) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->canDiscard(p, "h"))
                        skill_list.insert(p, QStringList(objectName()));
                }
            }
        } else if (triggerEvent == NullificationEffect) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->canDiscard(p, "h"))
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (room->askForCard(ask_who, "@@thzheyin", "@thzheyin:" + player->objectName(), data, objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (!player->hasFlag("ThZheyin")) {
            room->setPlayerCardLimitation(player, "use", "TrickCard", false);
            room->setPlayerFlag(player, "ThZheyin");
        }

        if (!player->canDiscard(player, "he")
                || !room->askForCard(player, "..", "@thzheyin-discard:" + ask_who->objectName(), data, objectName())) {
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                use.nullified_list << "_ALL_TARGETS";
                data = QVariant::fromValue(use);

                player->obtainCard(use.card);

                player->drawCards(1, objectName());
            } else {
                const Card *card = data.value<const Card *>();
                player->obtainCard(card);
                player->drawCards(1, objectName());
                return true;
            }
        }

        return false;
    }
};

class ThYingdeng: public TriggerSkill {
public:
    ThYingdeng(): TriggerSkill("thyingdeng") {
        events << CardAsked;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (TriggerSkill::triggerable(player)) {
            QString asked = data.toStringList().first();
            if (asked == "jink")
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
        player->drawCards(1, objectName());
        room->askForDiscard(player, objectName(), 1, 1, false, true);
        return false;
    }
};

ThYachuiCard::ThYachuiCard() {
    will_throw = false;
    handling_method = MethodNone;
}

bool ThYachuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && subcardsLength() <= to_select->getLostHp();
}

void ThYachuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), targets.first()->objectName(), "thyachui", QString());
    room->obtainCard(targets.first(), this, reason);
    source->drawCards(subcardsLength());
}

class ThYachuiViewAsSkill: public ViewAsSkill {
public:
    ThYachuiViewAsSkill(): ViewAsSkill("thyachui") {
        response_pattern = "@@thyachui";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped() && to_select->isRed();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;
        ThYachuiCard *card = new ThYachuiCard;
        card->addSubcards(cards);
        return card;
    }
};

class ThYachui: public TriggerSkill {
public:
    ThYachui(): TriggerSkill("thyachui") {
        events << EventPhaseStart;
        view_as_skill = new ThYachuiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw && !player->isKongcheng())
            return true;
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (room->askForUseCard(player, "@@thyachui", "@thyachui", -1, Card::MethodNone))
            return true;
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const {
        return true;
    }
};

class ThChunhen: public TriggerSkill{
public:
    ThChunhen():TriggerSkill("thchunhen") {
        events << BeforeCardsMove;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!move.from || move.from == player)
            return QStringList();
        if (move.to_place == Player::DiscardPile
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            int i = 0;
            foreach (int card_id, move.card_ids) {
                if (Sanguosha->getCard(card_id)->getSuit() == Card::Diamond && room->getCardOwner(card_id) == move.from
                    && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))
                    return QStringList(objectName());
                i++;
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
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

class ThXiagong: public TargetModSkill {
public:
    ThXiagong(): TargetModSkill("thxiagong") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const {
        if (!from->getWeapon() && from->hasSkill(objectName()) && from->getAttackRange() < 2)
            return 2 - from->getAttackRange();
        else
            return 0;
    }
};

ThGuaitanCard::ThGuaitanCard()
{
}

bool ThGuaitanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < 2 && to_select != Self;
}

void ThGuaitanCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *player = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = player->getRoom();
    QString choice = room->askForChoice(player, objectName(), "BasicCard+EquipCard+TrickCard", QVariant::fromValue(target));

    LogMessage log;
    log.type = "#ThGuaitan";
    log.from = player;
    log.to << target;
    log.arg = choice;
    room->sendLog(log);

    room->addPlayerMark(target, "@guaitan_" + choice.left(5).toLower());

    room->setPlayerCardLimitation(target, "use,response", choice, false);
    if (target == room->getCurrent() && target->getPhase() != Player::NotActive)
        target->setFlags("guaitan_current");
}

class ThGuaitanVS : public ZeroCardViewAsSkill
{
public:
    ThGuaitanVS() : ZeroCardViewAsSkill("thguaitan")
    {
        response_pattern = "@@thguaitan";
    }

    virtual const Card *viewAs() const
    {
        return new ThGuaitanCard;
    }
};

class ThGuaitan : public TriggerSkill
{
public:
    ThGuaitan() : TriggerSkill("thguaitan")
    {
        events << PreDamageDone << CardFinished;
        view_as_skill = new ThGuaitanVS;
    }

    virtual QStringList triggerable(TriggerEvent e, Room *, ServerPlayer *sp, QVariant &d, ServerPlayer *&) const
    {
        if (e == CardFinished && TriggerSkill::triggerable(sp)) {
            CardUseStruct use = d.value<CardUseStruct>();
            if (use.card && use.card->getTypeId() != Card::TypeSkill && use.card->hasFlag("thguaitan"))
                return QStringList(objectName());
        } else if (e == PreDamageDone) {
            DamageStruct damage = d.value<DamageStruct>();
            if (damage.card)
                damage.card->setFlags("thguaitan");
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        return room->askForUseCard(player, "@@thguaitan", "@thguaitan");
    }
};

class ThGuaitanClear : public TriggerSkill
{
public:
    ThGuaitanClear() :TriggerSkill("#thguaitan")
    {
        events << Damaged << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
            if (player->hasFlag("guaitan_current"))
                return QStringList();
        }

        while (player->getMark("@guaitan_basic") > 0) {
            room->removePlayerMark(player, "@guaitan_basic");
            room->removePlayerCardLimitation(player, "use,response", "BasicCard$0");
        }
        while (player->getMark("@guaitan_equip") > 0) {
            room->removePlayerMark(player, "@guaitan_equip");
            room->removePlayerCardLimitation(player, "use,response", "EquipCard$0");
        }
        while (player->getMark("@guaitan_trick") > 0) {
            room->removePlayerMark(player, "@guaitan_trick");
            room->removePlayerCardLimitation(player, "use,response", "TrickCard$0");
        }

        return QStringList();
    }
};

class ThHouzhi: public TriggerSkill {
public:
    ThHouzhi(): TriggerSkill("thhouzhi") {
        frequency = Compulsory;
        events << EventPhaseStart << DamageInflicted;
        owner_only_skill = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish && player->getMark("@stiff") > 0)
            return QStringList(objectName());
        if (triggerEvent == DamageInflicted)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        if (triggerEvent == EventPhaseStart) {
            int n = player->getMark("@stiff");
            player->loseAllMarks("@stiff");
            room->loseHp(player, n);
            return false;
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            player->gainMark("@stiff", damage.damage);
            return true;
        }
    }
};

class ThShayu: public TriggerSkill {
public:
    ThShayu(): TriggerSkill("thshayu") {
        events << Damage;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
               && player->getPhase() == Player::Play
               && player->getMark("@stiff") > 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        player->loseMark("@stiff");
        return false;
    }
};

ThDujiaCard::ThDujiaCard() {
    target_fixed = true;
}

void ThDujiaCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->gainMark("@stiff");
    if (source->isAlive())
        source->drawCards(3, "thdujia");
}

class ThDujia: public OneCardViewAsSkill {
public:
    ThDujia(): OneCardViewAsSkill("thdujia") {
        filter_pattern = "BasicCard!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThDujiaCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        ThDujiaCard *card = new ThDujiaCard;
        card->addSubcard(originalCard);
        return card;
    }
};

ThXianfaCard::ThXianfaCard() {
}

bool ThXianfaCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void ThXianfaCard::onEffect(const CardEffectStruct &effect) const {
    Room *room = effect.from->getRoom();
    if (subcards.isEmpty())
        room->loseHp(effect.from);

    effect.from->tag["ThXianfaTarget"] = QVariant::fromValue(effect.to);
    QString choice = room->askForChoice(effect.from, "thxianfa", "start+judge+draw+discard+finish", QVariant::fromValue(effect.to));
    effect.from->tag.remove("ThXianfaTarget");
    LogMessage log;
    log.type = "#ThXianfaChoose";
    log.from = effect.from;
    log.arg = choice;
    room->sendLog(log);

    effect.from->setFlags("thxianfa");
    effect.to->tag["ThXianfa"] = QVariant::fromValue(choice);
}

class ThXianfaViewAsSkill: public ViewAsSkill {
public:
    ThXianfaViewAsSkill(): ViewAsSkill("thxianfa") {

    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThXianfaCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const {
        return selected.isEmpty();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (cards.isEmpty())
            return new ThXianfaCard;
        else if (cards.length() == 1) {
            ThXianfaCard *card = new ThXianfaCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class ThXianfa: public TriggerSkill {
public:
    ThXianfa(): TriggerSkill("thxianfa") {
        events << EventPhaseChanging;
        view_as_skill = new ThXianfaViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (player->hasFlag("thxianfa") && change.from == Player::Discard)
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (!p->tag.value("ThXianfa").isNull())
                    return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->setFlags("-thxianfa");
        ServerPlayer *target = NULL;
        foreach (ServerPlayer *p, room->getAllPlayers())
            if (!p->tag.value("ThXianfa").isNull()) {
                target = p;
                break;
            }
        if (target && target->isAlive()) {
            Player::Phase phase = Player::PhaseNone;
            QString choice = target->tag.value("ThXianfa").toString();
            target->tag.remove("ThXianfa");

            LogMessage log;
            log.type = "#ThXianfaDo";
            log.from = target;
            log.arg = objectName();
            log.arg2 = choice;
            room->sendLog(log);

            if(choice == "start")
                phase = Player::Start;
            else if(choice == "judge")
                phase = Player::Judge;
            else if(choice == "draw")
                phase = Player::Draw;
            else if(choice == "discard")
                phase = Player::Discard;
            else if(choice == "finish")
                phase = Player::Finish;
            if (phase != Player::PhaseNone) {
                Player::Phase origin_phase = target->getPhase();
                target->setPhase(phase);
                room->broadcastProperty(target, "phase");
                RoomThread *thread = room->getThread();
                if (!thread->trigger(EventPhaseStart, room, target))
                    thread->trigger(EventPhaseProceeding, room, target);
                thread->trigger(EventPhaseEnd, room, target);

                target->setPhase(origin_phase);
                room->broadcastProperty(target, "phase");
            }
        }

        return false;
    }
};

class ThWendao: public TriggerSkill {
public:
    ThWendao(): TriggerSkill("thwendao") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        if (!TriggerSkill::triggerable(player) || player->getMark("@wendao") > 0)
            return false;
        if (player->getPhase() != Player::Start || !player->isKongcheng())
            return false;
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->addPlayerMark(player, "@wendao");
        LogMessage log;
        log.type = "#ThWendao";
        log.from = player;
        log.arg  = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->getThread()->delay();

        QStringList choices;
        if (player->isWounded())
            choices << "recover";
        choices << "draw";

        QString choice = room->askForChoice(player, objectName(), choices.join("+"));

        if (choice == "recover") {
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        } else
            player->drawCards(2);

        room->changeMaxHpForAwakenSkill(player);
        room->acquireSkill(player, "ikmitu");
        return false;
    }
};

ThLeishiCard::ThLeishiCard(){
}

bool ThLeishiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty())
        return false;
    const Player *from = Self->parent()->findChild<const Player *>(Self->property("thleishi").toString());
    if (from) {
        QList<const Player *> targets;
        int min = 998;
        foreach (const Player *p, from->getAliveSiblings()) {
            int dis = from->distanceTo(p);
            if (dis == -1)
                continue;
            if (targets.isEmpty() || dis == min) {
                targets << p;
                min = dis;
            } else if (dis < min) {
                targets.clear();
                targets << p;
                min = dis;
            }
        }
        return targets.contains(to_select);
    }
    return false;
}

void ThLeishiCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->damage(DamageStruct("thleishi", effect.from, effect.to, 1, DamageStruct::Thunder));
}

class ThLeishiVS: public OneCardViewAsSkill{
public:
    ThLeishiVS(): OneCardViewAsSkill("thleishi") {
        response_pattern = "@@thleishi";
        filter_pattern = ".!";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new ThLeishiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThLeishi: public TriggerSkill {
public:
    ThLeishi(): TriggerSkill("thleishi") {
        events << DamageComplete;
        view_as_skill = new ThLeishiVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const {
        if (player && player->isAlive() && player->getHp() <= 1) {
            DamageStruct damage = data.value<DamageStruct>();
            if (TriggerSkill::triggerable(damage.from)) {
                ask_who = damage.from;
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        room->setPlayerProperty(ask_who, "thleishi", player->objectName());
        room->askForUseCard(ask_who, "@@thleishi", "@thleishi", -1, Card::MethodDiscard);
        return false;
    }
};

class ThShanling: public TriggerSkill {
public:
    ThShanling(): TriggerSkill("thshanling") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        if (TriggerSkill::triggerable(target) && target->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, target->getRoom()->getAllPlayers()) {
                if (target->getHp() > qMax(0, p->getHp()))
                    return false;
            }
            foreach (ServerPlayer *p, target->getRoom()->getAllPlayers()) {
                if (target->inMyAttackRange(p))
                    return true;
            }
        }
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (player->inMyAttackRange(p))
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thshanling", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThShanlingTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        ServerPlayer *target = player->tag["ThShanlingTarget"].value<ServerPlayer *>();
        player->tag.remove("ThShanlingTarget");
        if (target)
            room->damage(DamageStruct(objectName(), player, target, 1, DamageStruct::Thunder));
        return false;
    }
};

class ThShijieViewAsSkill: public OneCardViewAsSkill {
public:
    ThShijieViewAsSkill(): OneCardViewAsSkill("thshijie") {
        expand_pile = "utensil";
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool viewFilter(const Card *to_select) const{
        return Self->getPile("utensil").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const Card *o_card) const{
        Nullification *null = new Nullification(o_card->getSuit(), o_card->getNumber());
        null->setSkillName(objectName());
        null->addSubcard(o_card);
        return null;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "nullification" && !player->getPile("utensil").isEmpty();
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return !player->getPile("utensil").isEmpty();
    }
};

class ThShijie: public TriggerSkill {
public:
    ThShijie(): TriggerSkill("thshijie") {
        events << HpRecover;
        view_as_skill = new ThShijieViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        RecoverStruct recover = data.value<RecoverStruct>();
        QStringList skills;
        if (!TriggerSkill::triggerable(player)) return skills;
        for (int i = 0; i < recover.recover; i++)
            skills << objectName();
        return skills;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        player->addToPile("utensil", room->drawCard(), true);
        return false;
    }
};

class ThShengzhi : public TriggerSkill
{
public:
    ThShengzhi() : TriggerSkill("thshengzhi")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::RoundStart) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName())) {
                if (owner == player)
                    continue;
                skill_list.insert(owner, QStringList(objectName()));
            }
        }
        return skill_list;
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
        if (!player->canDiscard(player, "h") || !room->askForCard(player, "Nullification", "@thshengzhi")) {
            QList<Player::Phase> phases = player->getPhases();
            QStringList choices;
            foreach (Player::Phase phase, phases) {
                if (phase == Player::NotActive || phase == Player::RoundStart || phase == Player::PhaseNone)
                    continue;
                if (!player->isSkipped(phase))
                    choices << player->getPhaseString(phase);
            }
            if (choices.isEmpty()) return false;
            QString choice = room->askForChoice(ask_who, objectName(), choices.join("+"), QVariant::fromValue(player));

            QMap<QString, Player::Phase> phase_map;
            phase_map.insert("start", Player::Start);
            phase_map.insert("judge", Player::Judge);
            phase_map.insert("draw", Player::Draw);
            phase_map.insert("play", Player::Play);
            phase_map.insert("discard", Player::Discard);
            phase_map.insert("finish", Player::Finish);

            if (phase_map.value(choice, Player::PhaseNone) != Player::PhaseNone)
                player->skip(phase_map.value(choice));
            room->damage(DamageStruct(objectName(), player, ask_who));
        }
        return false;
    }
};

class ThZhaoyu: public TriggerSkill {
public:
    ThZhaoyu(): TriggerSkill("thzhaoyu") {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        TriggerList skill_list;
        if (player->getPhase() != Player::Start)
            return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
            if (!owner->isNude())
                skill_list.insert(owner, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        const Card *card = room->askForCard(ask_who, "..", "@thzhaoyu:" + player->objectName(), data, Card::MethodNone);
        if (card) {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = ask_who;
            log.arg  = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(ask_who, objectName());
            room->broadcastSkillInvoke(objectName());
            ask_who->tag["ThZhaoyuCard"] = QVariant::fromValue(card);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        const Card *card = ask_who->tag["ThZhaoyuCard"].value<const Card *>();
        ask_who->tag.remove("ThZhaoyuCard");
        if (card) {
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, ask_who->objectName(), "thzhaoyu", QString());
            room->moveCardTo(card, NULL, Player::DrawPile, reason, false);
            if ((!player->getJudgingArea().isEmpty() || player->getWeapon())
                && ask_who->askForSkillInvoke("thzhaoyu-draw", "draw")) {
                QList<int> card_ids;
                room->getThread()->trigger(FetchDrawPileCard, room, NULL);
                QList<int> &draw = room->getDrawPile();
                if (draw.isEmpty())
                    room->swapPile();
                card_ids << draw.takeLast();
                CardsMoveStruct move(card_ids,
                                     ask_who,
                                     Player::PlaceHand,
                                     CardMoveReason(CardMoveReason::S_REASON_DRAW, ask_who->objectName(), objectName(), QString()));
                room->moveCardsAtomic(move, false);
            }
        }
        return false;
    }
};

class ThWuwu: public TriggerSkill {
public:
    ThWuwu(): TriggerSkill("thwuwu") {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::Discard || player->isKongcheng())
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        room->askForDiscard(player, objectName(), 1, 1, false, false);
        return false;
    }
};

class ThRudao: public TriggerSkill {
public:
    ThRudao(): TriggerSkill("thrudao") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::Start || !player->isKongcheng())
            return false;
        if (player->getMark("@rudao") > 0)
            return false;
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        room->addPlayerMark(player, "@rudao");
        LogMessage log;
        log.type = "#ThRudao";
        log.from = player;
        log.arg  = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());

        player->drawCards(2);
        room->changeMaxHpForAwakenSkill(player, -3);
        room->detachSkillFromPlayer(player, "thwuwu");
        return false;
    }
};

ThLiuzhenCard::ThLiuzhenCard()
{
    handling_method = MethodNone;
}

bool ThLiuzhenCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *player) const
{
    const Card *slash = player->tag["thliuzhen_carduse"].value<CardUseStruct>().card;
    return !to_select->hasFlag("liuzhenold") && player->canSlash(to_select, slash, false);
}

void ThLiuzhenCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->setPlayerFlag(effect.to, "liuzhennew");
}

class ThLiuzhenViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ThLiuzhenViewAsSkill() : ZeroCardViewAsSkill("thliuzhen")
    {
        response_pattern = "@@thliuzhen";
    }

    virtual const Card *viewAs() const
    {
        return new ThLiuzhenCard;
    }
};

class ThLiuzhen : public TriggerSkill
{
public:
    ThLiuzhen() : TriggerSkill("thliuzhen")
    {
        events << TargetSpecified << BeforeCardsMove << SlashMissed;
        view_as_skill = new ThLiuzhenViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetSpecified && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && !player->hasUsed("ThLiuzhenCard")) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (player->canSlash(p, use.card, false) && !use.to.contains(p))
                        return QStringList(objectName());
                }
            }
        } else if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("liuzhenslash")) {
                return QStringList(objectName());
            }
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile
                && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
                if (player->tag["thliuzhen_user"].toBool()) {
                    const Card *liuzhen_card = move.reason.m_extraData.value<const Card *>();
                    if (liuzhen_card && liuzhen_card->hasFlag("thliuzhen"))
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *tar, use.to)
                room->setPlayerFlag(tar, "liuzhenold");
            //tag for ai
            player->tag["thliuzhen_carduse"] = data;
            if (room->askForUseCard(player, "@@thliuzhen", "@thliuzhen", -1, Card::MethodNone)) {
                room->setCardFlag(use.card, "thliuzhen");
                player->tag["thliuzhen_user"] = true;
            }
            player->tag.remove("thliuzhen_carduse");
            foreach (ServerPlayer *tar, use.to)
                room->setPlayerFlag(tar, "-liuzhenold");
            return false;
        } else if (triggerEvent == SlashMissed) {
            return true;
        } else if (triggerEvent == BeforeCardsMove) {
            player->tag["thliuzhen_user"] = false;
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            LogMessage log;
            log.type = "#ThLiuzhenMiss";
            log.from = player;
            log.to << effect.to;
            log.arg  = objectName();
            room->sendLog(log);
            if (!player->canDiscard(player, "he") || !room->askForDiscard(player, objectName(), 1, 1, true, true))
                room->loseHp(player);
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if (card) {
                card->setFlags("-thliuzhen");
                if (card->hasFlag("drank")) {
                    room->setCardFlag(card, "-drank");
                    card->setTag("drank", 0);
                }
                CardUseStruct use;
                use.card = card;
                room->setCardFlag(use.card, "liuzhenslash");
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->hasFlag("liuzhennew")) {
                        room->setPlayerFlag(p, "-liuzhennew");
                        if (player->canSlash(p, false))
                            use.to << p;
                    }
                if (use.to.isEmpty()) return false;
                room->sortByActionOrder(use.to);
                use.from = player;
                room->useCard(use, false);
                move.removeCardIds(move.card_ids);
                data = QVariant::fromValue(move);
            }
        }
        return false;
    }
};

class ThTianchanViewAsSkill: public OneCardViewAsSkill {
public:
    ThTianchanViewAsSkill(): OneCardViewAsSkill("thtianchanv") {
        attached_lord_skill = true;
        response_or_use = true;
        filter_pattern = ".|spade";
    }

    virtual bool shouldBeVisible(const Player *player) const{
        return player->getKingdom() == "hana";
    }

    virtual bool isEnabledAtPlay(const Player *) const {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (pattern != "peach" || player->getKingdom() != "hana") return false;
        QString str = player->property("currentdying").toString();
        foreach (const Player *p, player->getAliveSiblings())
            if (p->objectName() == str && p->hasLordSkill("thtianchan"))
                return true;
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        Peach *card = new Peach(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class ThTianchan: public TriggerSkill {
public:
    ThTianchan(): TriggerSkill("thtianchan$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!player) return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "thtianchan")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("thtianchanv"))
                    room->attachSkillToPlayer(p, "thtianchanv");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "thtianchan") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("thtianchanv"))
                    room->detachSkillFromPlayer(p, "thtianchanv", true);
            }
        }
        return QStringList();
    }
};

TouhouHanaPackage::TouhouHanaPackage()
    :Package("touhou-hana")
{
    General *hana001 = new General(this, "hana001$", "hana");
    hana001->addSkill(new ThHuaji);
    hana001->addSkill(new ThFeizhan);

    General *hana002 = new General(this, "hana002", "hana");
    hana002->addSkill(new ThJiewu);
    hana002->addSkill(new ThJiewuTrigger);
    hana002->addSkill(new SlashNoDistanceLimitSkill("thjiewu"));
    related_skills.insertMulti("thjiewu", "#thjiewu");
    related_skills.insertMulti("thjiewu", "#thjiewu-slash-ndl");
    hana002->addSkill(new ThGenxing);
    hana002->addRelateSkill("thmopao");

    General *hana003 = new General(this, "hana003", "hana", 3, false);
    hana003->addSkill(new ThBian);
    hana003->addSkill(new ThGuihang);
    hana003->addSkill(new ThWujian);
    hana003->addSkill(new ThWujianClear);
    related_skills.insertMulti("thwujian", "#thwujian");
    skills << new ThWujianDistanceSkill;

    General *hana004 = new General(this, "hana004", "hana", 3);
    hana004->addSkill(new ThXuelan);
    hana004->addSkill(new ThXinwang);
    hana004->addSkill(new ThJuedu);

    General *hana005 = new General(this, "hana005", "hana");
    hana005->addSkill(new ThTingwu);
    hana005->addSkill(new ThYuchang);
    hana005->addSkill(new ThYuchangTargetMod);
    related_skills.insertMulti("thyuchang", "#thyuchang-target");

    General *hana006 = new General(this, "hana006", "hana");
    hana006->addSkill(new ThXihua);
    hana006->addSkill(new ThXihuaTrigger);
    hana006->addSkill(new SlashNoDistanceLimitSkill("thxihua"));
    related_skills.insertMulti("thxihua", "#thxihua");
    related_skills.insertMulti("thxihua", "#thxihua-slash-ndl");

    General *hana007 = new General(this, "hana007", "hana", 3, false);
    hana007->addSkill(new ThMimeng);
    hana007->addSkill(new ThAnyun);

    General *hana008 = new General(this, "hana008", "hana", 3);
    hana008->addSkill(new ThQuanshan);
    hana008->addSkill(new ThXiangang);

    General *hana009 = new General(this, "hana009", "hana");
    hana009->addSkill(new ThDuanzui);
    hana009->addSkill(new SlashNoDistanceLimitSkill("thduanzui"));
    related_skills.insertMulti("thduanzui", "#thduanzui-slash-ndl");

    General *hana010 = new General(this, "hana010", "hana");
    hana010->addSkill(new ThZheyin);
    hana010->addSkill(new ThYingdeng);

    General *hana011 = new General(this, "hana011", "hana", 3);
    hana011->addSkill(new ThYachui);
    hana011->addSkill(new ThChunhen);

    General *hana012 = new General(this, "hana012", "hana");
    hana012->addSkill(new ThXiagong);
    hana012->addSkill(new ThGuaitan);
    hana012->addSkill(new ThGuaitanClear);
    related_skills.insertMulti("thguaitan", "#thguaitan");

    General *hana013 = new General(this, "hana013", "hana", 3);
    hana013->addSkill(new ThHouzhi);
    hana013->addSkill(new ThShayu);
    hana013->addSkill(new ThDujia);

    General *hana014 = new General(this, "hana014", "hana", 4, false);
    hana014->addSkill(new ThXianfa);
    hana014->addSkill(new ThWendao);

    General *hana015 = new General(this, "hana015", "hana", 3);
    hana015->addSkill(new ThLeishi);
    hana015->addSkill(new ThShanling);

    General *hana016 = new General(this, "hana016", "hana", 3);
    hana016->addSkill(new ThShijie);
    hana016->addSkill(new ThShengzhi);

    General *hana017 = new General(this, "hana017", "hana", 7);
    hana017->addSkill(new ThZhaoyu);
    hana017->addSkill(new ThWuwu);
    hana017->addSkill(new ThRudao);

    General *hana018 = new General(this, "hana018$", "hana");
    hana018->addSkill(new ThLiuzhen);
    hana018->addSkill(new ThTianchan);

    addMetaObject<ThJiewuCard>();
    addMetaObject<ThWujianCard>();
    addMetaObject<ThXihuaCard>();
    addMetaObject<ThMimengCard>();
    addMetaObject<ThQuanshanGiveCard>();
    addMetaObject<ThQuanshanCard>();
    addMetaObject<ThDuanzuiCard>();
    addMetaObject<ThYachuiCard>();
    addMetaObject<ThGuaitanCard>();
    addMetaObject<ThDujiaCard>();
    addMetaObject<ThXianfaCard>();
    addMetaObject<ThLeishiCard>();
    addMetaObject<ThLiuzhenCard>();

    skills << new ThMopao << new ThQuanshanGive << new ThTianchanViewAsSkill;
}

ADD_PACKAGE(TouhouHana)
