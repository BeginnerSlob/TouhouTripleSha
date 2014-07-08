#include "yjcm2013.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"

JunxingCard::JunxingCard() {
}

void JunxingCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    if (!target->isAlive()) return;

    QString type_name[4] = { QString(), "BasicCard", "TrickCard", "EquipCard" };
    QStringList types;
    types << "BasicCard" << "TrickCard" << "EquipCard";
    foreach (int id, subcards) {
        const Card *c = Sanguosha->getCard(id);
        types.removeOne(type_name[c->getTypeId()]);
        if (types.isEmpty()) break;
    }
    if (!target->canDiscard(target, "h") || types.isEmpty()
        || !room->askForCard(target, types.join(",") + "|.|.|hand", "@junxing-discard")) {
        target->turnOver();
        target->drawCards(subcards.length(), "junxing");
    }
}

class Junxing: public ViewAsSkill {
public:
    Junxing(): ViewAsSkill("junxing") {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        JunxingCard *card = new JunxingCard;
        card->addSubcards(cards);
        card->setSkillName(objectName());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "h") && !player->hasUsed("JunxingCard");
    }
};

class Yuce: public MasochismSkill {
public:
    Yuce(): MasochismSkill("yuce") {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        if (target->isKongcheng()) return;

        Room *room = target->getRoom();
        QVariant data = QVariant::fromValue(damage);
        const Card *card = room->askForCard(target, ".", "@yuce-show", data, Card::MethodNone);
        if (card) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(target, objectName());
            LogMessage log;
            log.from = target;
            log.type = "#InvokeSkill";
            log.arg = objectName();
            room->sendLog(log);

            room->showCard(target, card->getEffectiveId());
            if (!damage.from || damage.from->isDead()) return;

            QString type_name[4] = { QString(), "BasicCard", "TrickCard", "EquipCard" };
            QStringList types;
            types << "BasicCard" << "TrickCard" << "EquipCard";
            types.removeOne(type_name[card->getTypeId()]);
            if (!damage.from->canDiscard(damage.from, "h")
                || !room->askForCard(damage.from, types.join(",") + "|.|.|hand",
                                     QString("@yuce-discard:%1::%2:%3")
                                             .arg(target->objectName())
                                             .arg(types.first()).arg(types.last()),
                                     data)) {
                room->recover(target, RecoverStruct(target));
            }
        }
    }
};

class Duodao: public MasochismSkill {
public:
    Duodao(): MasochismSkill("duodao") {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        if (!damage.card || !damage.card->isKindOf("Slash") || !target->canDiscard(target, "he"))
            return;
        QVariant data = QVariant::fromValue(damage);
        Room *room = target->getRoom();
        if (room->askForCard(target, "..", "@duodao-get", data, objectName())) {
            if (damage.from && damage.from->getWeapon()) {
                room->broadcastSkillInvoke(objectName());
                target->obtainCard(damage.from->getWeapon());
            }
        }
    }
};

class Anjian: public TriggerSkill {
public:
    Anjian(): TriggerSkill("anjian") {
        events << DamageCaused;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user) return false;
        if (damage.from && !damage.to->inMyAttackRange(damage.from)
            && damage.card && damage.card->isKindOf("Slash")) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(damage.from, objectName());

            LogMessage log;
            log.type = "#AnjianBuff";
            log.from = damage.from;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

ZongxuanCard::ZongxuanCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
    target_fixed = true;
}

void ZongxuanCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    QVariantList subcardsList;
    foreach (int id, subcards)
        subcardsList << id;
    source->tag["zongxuan"] = QVariant::fromValue(subcardsList);
}

class ZongxuanViewAsSkill: public ViewAsSkill {
public:
    ZongxuanViewAsSkill(): ViewAsSkill("zongxuan") {
        response_pattern = "@@zongxuan";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        QStringList zongxuan = Self->property("zongxuan").toString().split("+");
        foreach (QString id, zongxuan) {
            bool ok;
            if (id.toInt(&ok) == to_select->getEffectiveId() && ok)
                return true;
        }
        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty()) return NULL;

        ZongxuanCard *card = new ZongxuanCard;
        card->addSubcards(cards);
        return card;
    }
};

class Zongxuan: public TriggerSkill {
public:
    Zongxuan(): TriggerSkill("zongxuan") {
        events << BeforeCardsMove;
        view_as_skill = new ZongxuanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player)
            return false;
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)) {

            int i = 0;
            QList<int> zongxuan_card;
            foreach (int card_id, move.card_ids) {
                if (room->getCardOwner(card_id) == move.from
                    && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)) {
                        zongxuan_card << card_id;
                }
                i++;
            }
            if (zongxuan_card.isEmpty())
                return false;

            room->setPlayerProperty(player, "zongxuan", IntList2StringList(zongxuan_card).join("+"));
            do {
                if (!room->askForUseCard(player, "@@zongxuan", "@zongxuan-put")) break;

                QList<int> subcards;
                QVariantList subcards_variant = player->tag["zongxuan"].toList();
                if (!subcards_variant.isEmpty()) {
                    subcards = VariantList2IntList(subcards_variant);
                    QStringList zongxuan = player->property("zongxuan").toString().split("+");
                    foreach (int id, subcards) {
                        zongxuan_card.removeOne(id);
                        zongxuan.removeOne(QString::number(id));
                        room->setPlayerProperty(player, "zongxuan", zongxuan.join("+"));
                        QList<int> _id;
                        _id << id;
                        move.removeCardIds(_id);
                        data = QVariant::fromValue(move);
                        room->setPlayerProperty(player, "zongxuan_move", QString::number(id)); // For UI to translate the move reason
                        room->moveCardTo(Sanguosha->getCard(id), player, NULL, Player::DrawPile, move.reason, true);
                        if (!player->isAlive())
                            break;
                    }
                }
                player->tag.remove("zongxuan");
            } while (!zongxuan_card.isEmpty());
        }
        return false;
    }
};

class Zhiyan: public PhaseChangeSkill {
public:
    Zhiyan(): PhaseChangeSkill("zhiyan") {
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Finish)
            return false;

        Room *room = target->getRoom();
        ServerPlayer *to = room->askForPlayerChosen(target, room->getAlivePlayers(), objectName(), "zhiyan-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            QList<int> ids = room->getNCards(1, false);
            const Card *card = Sanguosha->getCard(ids.first());
            room->obtainCard(to, card, false);
            if (!to->isAlive())
                return false;
            room->showCard(to, ids.first());

            if (card->isKindOf("EquipCard")) {
                room->recover(to, RecoverStruct(target));
                if (to->isAlive() && !to->isCardLimited(card, Card::MethodUse))
                    room->useCard(CardUseStruct(card, to, to));
            }
        }
        return false;
    }
};

DanshouCard::DanshouCard() {
}

bool DanshouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty())
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

void DanshouCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    int len = subcardsLength();
    switch (len) {
    case 0:
            Q_ASSERT(false);
            break;
    case 1:
            if (effect.from->canDiscard(effect.to, "he")) {
                int id = room->askForCardChosen(effect.from, effect.to, "he", "danshou", false, Card::MethodDiscard);
                room->throwCard(id, effect.to, effect.from);
            }
            break;
    case 2:
            if (!effect.to->isNude()) {
                int id = room->askForCardChosen(effect.from, effect.to, "he", "danshou");
                room->obtainCard(effect.from, id, false);
            }
            break;
    case 3:
            room->damage(DamageStruct("danshou", effect.from, effect.to));
            break;
    default:
            room->drawCards(effect.from, 2, "danshou");
            room->drawCards(effect.to, 2, "danshou");
            break;
    }
}

class DanshouViewAsSkill: public ViewAsSkill {
public:
    DanshouViewAsSkill(): ViewAsSkill("danshou") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return !Self->isJilei(to_select) && selected.length() <= Self->getMark("danshou");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != Self->getMark("danshou") + 1) return NULL;
        DanshouCard *danshou = new DanshouCard;
        danshou->addSubcards(cards);
        return danshou;
    }
};

class Danshou: public TriggerSkill {
public:
    Danshou(): TriggerSkill("danshou") {
        events << EventPhaseStart << PreCardUsed;
        view_as_skill = new DanshouViewAsSkill;
    }

    virtual int getPriority(TriggerEvent) const{
        return 6;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            room->setPlayerMark(player, "danshou", 0);
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("DanshouCard"))
                room->addPlayerMark(use.from, "danshou");
        }
        return false;
    }
};

class Juece: public PhaseChangeSkill {
public:
    Juece(): PhaseChangeSkill("juece") {
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Finish) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> kongcheng_players;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->isKongcheng())
                kongcheng_players << p;
        }
        if (kongcheng_players.isEmpty()) return false;

        ServerPlayer *to_damage = room->askForPlayerChosen(target, kongcheng_players, objectName(),
                                                           "@juece", true, true);
        if (to_damage) {
            room->broadcastSkillInvoke(objectName());
            room->damage(DamageStruct(objectName(), target, to_damage));
        }
        return false;
    }
};

MiejiCard::MiejiCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool MiejiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void MiejiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, effect.from->objectName(), QString(), "mieji", QString());
    room->moveCardTo(this, effect.from, NULL, Player::DrawPile, reason, true);

    int trick_num = 0, nontrick_num = 0;
    foreach (const Card *c, effect.to->getCards("he")) {
        if (effect.to->canDiscard(effect.to, c->getId())) {
            if (c->isKindOf("TrickCard"))
                trick_num++;
            else
                nontrick_num++;
        }
    }
    bool discarded = room->askForDiscard(effect.to, "mieji", 1, qMin(1, trick_num), nontrick_num > 1, true, "@mieji-trick", "TrickCard");
    if (trick_num == 0 || !discarded)
        room->askForDiscard(effect.to, "mieji", 2, 2, false, true, "@mieji-nontrick", "^TrickCard");
}

class Mieji: public OneCardViewAsSkill {
public:
    Mieji(): OneCardViewAsSkill("mieji") {
        filter_pattern = "TrickCard|black";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("MiejiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        MiejiCard *card = new MiejiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Fencheng: public ZeroCardViewAsSkill {
public:
    Fencheng(): ZeroCardViewAsSkill("fencheng") {
        frequency = Limited;
        limit_mark = "@burn";
    }

    virtual const Card *viewAs() const{
        return new FenchengCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@burn") >= 1;
    }
};

class FenchengMark: public TriggerSkill {
public:
    FenchengMark(): TriggerSkill("#fencheng") {
        events << ChoiceMade;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        QStringList data_str = data.toString().split(":");
        if (data_str.length() != 3 || data_str.first() != "cardDiscard" || data_str.at(1) != "fencheng")
            return false;
        room->setTag("FenchengDiscard", data_str.last().split("+").length());
        return false;
    }
};

FenchengCard::FenchengCard() {
    mute = true;
    target_fixed = true;
}

void FenchengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->removePlayerMark(source, "@burn");
    room->broadcastSkillInvoke("fencheng");
    room->doLightbox("$FenchengAnimate", 3000);
    room->setTag("FenchengDiscard", 0);

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    source->setFlags("FenchengUsing");
    try {
        foreach (ServerPlayer *player, players) {
            if (player->isAlive())
                room->cardEffect(this, source, player);
                room->getThread()->delay();
        }
        source->setFlags("-FenchengUsing");
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            source->setFlags("-FenchengUsing");
        throw triggerEvent;
    }
}

void FenchengCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    int length = room->getTag("FenchengDiscard").toInt() + 1;
    if (!effect.to->canDiscard(effect.to, "he") || effect.to->getCardCount(true) < length
        || !room->askForDiscard(effect.to, "fencheng", 1000, length, true, true, "@fencheng:::" + QString::number(length))) {
        room->setTag("FenchengDiscard", 0);
        room->damage(DamageStruct("fencheng", effect.from, effect.to, 2, DamageStruct::Fire));
    }
}

class Zhuikong: public TriggerSkill {
public:
    Zhuikong(): TriggerSkill("zhuikong") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        foreach (ServerPlayer *fuhuanghou, room->getAllPlayers()) {
            if (TriggerSkill::triggerable(fuhuanghou)
                && player != fuhuanghou && fuhuanghou->isWounded() && !fuhuanghou->isKongcheng()
                && room->askForSkillInvoke(fuhuanghou, objectName())) {
                room->broadcastSkillInvoke("zhuikong");
                if (fuhuanghou->pindian(player, objectName(), NULL)) {
                    room->setPlayerFlag(player, "zhuikong");
                } else {
                    room->setFixedDistance(player, fuhuanghou, 1);
                    QVariantList zhuikonglist = player->tag[objectName()].toList();
                    zhuikonglist.append(QVariant::fromValue(fuhuanghou));
                    player->tag[objectName()] = QVariant::fromValue(zhuikonglist);
                }
            }
        }
        return false;
    }
};

class ZhuikongClear: public TriggerSkill {
public:
    ZhuikongClear(): TriggerSkill("#zhuikong-clear") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;

        QVariantList zhuikonglist = player->tag["zhuikong"].toList();
        if (zhuikonglist.isEmpty()) return false;
        foreach (QVariant p, zhuikonglist) {
            ServerPlayer *fuhuanghou = p.value<ServerPlayer *>();
            room->setFixedDistance(player, fuhuanghou, -1);
        }
        player->tag.remove("zhuikong");
        return false;
    }
};

class ZhuikongProhibit: public ProhibitSkill {
public:
    ZhuikongProhibit(): ProhibitSkill("#zhuikong") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        if (card->getTypeId() != Card::TypeSkill && from->hasFlag("zhuikong"))
            return to != from;
        return false;
    }
};

class Qiuyuan: public TriggerSkill {
public:
    Qiuyuan(): TriggerSkill("qiuyuan") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p != use.from)
                    targets << p;
            }
            if (targets.isEmpty()) return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "qiuyuan-invoke", true, true);
            if (target) {
                if (target->getGeneralName().contains("fuwan") || target->getGeneral2Name().contains("fuwan"))
                    room->broadcastSkillInvoke("qiuyuan", 2);
                else
                    room->broadcastSkillInvoke("qiuyuan", 1);
                const Card *card = NULL;
                if (!target->isKongcheng())
                    card = room->askForCard(target, "Jink", "@qiuyuan-give:" + player->objectName(), data, Card::MethodNone);
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), "nosqiuyuan", QString());
                if (!card) {
                    if (use.from->canSlash(target, use.card, false)) {
                        LogMessage log;
                        log.type = "#BecomeTarget";
                        log.from = target;
                        log.card_str = use.card->toString();
                        room->sendLog(log);

                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());

                        use.to.append(target);
                        room->sortByActionOrder(use.to);
                        data = QVariant::fromValue(use);
                        room->getThread()->trigger(TargetConfirming, room, target, data);
                    }
                } else {
                    room->obtainCard(player, card, reason);
                }
            }
        }
        return false;
    }
};

YJCM2013Package::YJCM2013Package()
    : Package("YJCM2013")
{

    General *fuhuanghou = new General(this, "fuhuanghou", "qun", 3, false); // YJ 202
    fuhuanghou->addSkill(new Zhuikong);
    fuhuanghou->addSkill(new ZhuikongClear);
    fuhuanghou->addSkill(new ZhuikongProhibit);
    fuhuanghou->addSkill(new Qiuyuan);
    related_skills.insertMulti("zhuikong", "#zhuikong");
    related_skills.insertMulti("zhuikong", "#zhuikong-clear");

    General *liru = new General(this, "liru", "qun", 3); // YJ 206
    liru->addSkill(new Juece);
    liru->addSkill(new Mieji);
    liru->addSkill(new Fencheng);
    liru->addSkill(new FenchengMark);
    related_skills.insertMulti("fencheng", "#fencheng");

    General *manchong = new General(this, "manchong", "wei", 3); // YJ 208
    manchong->addSkill(new Junxing);
    manchong->addSkill(new Yuce);

    General *panzhangmazhong = new General(this, "panzhangmazhong", "wu"); // YJ 209
    panzhangmazhong->addSkill(new Duodao);
    panzhangmazhong->addSkill(new Anjian);

    General *yufan = new General(this, "yufan", "wu", 3); // YJ 210
    yufan->addSkill(new Zongxuan);
    yufan->addSkill(new Zhiyan);

    General *zhuran = new General(this, "zhuran", "wu"); // YJ 211
    zhuran->addSkill(new Danshou);

    addMetaObject<JunxingCard>();
    addMetaObject<ZongxuanCard>();
    addMetaObject<MiejiCard>();
    addMetaObject<FenchengCard>();
    addMetaObject<DanshouCard>();
}

ADD_PACKAGE(YJCM2013)
