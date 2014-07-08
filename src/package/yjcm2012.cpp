#include "yjcm2012.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"

class Zhenlie: public TriggerSkill {
public:
    Zhenlie(): TriggerSkill("zhenlie") {
        events << TargetConfirmed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.contains(player) && use.from != player) {
                if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                    if (room->askForSkillInvoke(player, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName());
                        player->setFlags("-ZhenlieTarget");
                        player->setFlags("ZhenlieTarget");
                        room->loseHp(player);
                        if (player->isAlive() && player->hasFlag("ZhenlieTarget")) {
                            player->setFlags("-ZhenlieTarget");
                            use.nullified_list << player->objectName();
                            data = QVariant::fromValue(use);
                            if (player->canDiscard(use.from, "he")) {
                                int id = room->askForCardChosen(player, use.from, "he", objectName(), false, Card::MethodDiscard);
                                room->throwCard(id, use.from, player);
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
};

class Miji: public TriggerSkill {
public:
    Miji(): TriggerSkill("miji") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const{
        if (TriggerSkill::triggerable(target) && triggerEvent == EventPhaseStart
            && target->getPhase() == Player::Finish && target->isWounded() && target->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName(), 1);
            QStringList draw_num;
            for (int i = 1; i <= target->getLostHp(); draw_num << QString::number(i++)) {}
            int num = room->askForChoice(target, "miji_draw", draw_num.join("+")).toInt();
            target->drawCards(num, objectName());
            target->setMark(objectName(), 0);
            if (!target->isKongcheng()) {
                forever {
                    int n = target->getMark(objectName());
                    if (n < num && !target->isKongcheng()) {
                        QList<int> handcards = target->handCards();
                        if (!room->askForYiji(target, handcards, objectName(), false, false, false, num - n))
                            break;
                    } else {
                        break;
                    }
                }
                // give the rest cards randomly
                if (target->getMark(objectName()) < num && !target->isKongcheng()) {
                    int rest_num = num - target->getMark(objectName());
                    forever {
                        QList<int> handcard_list = target->handCards();
                        qShuffle(handcard_list);
                        int give = qrand() % rest_num + 1;
                        rest_num -= give;
                        QList<int> to_give = handcard_list.length() < give ? handcard_list : handcard_list.mid(0, give);
                        ServerPlayer *receiver = room->getOtherPlayers(target).at(qrand() % (target->aliveCount() - 1));
                        DummyCard *dummy = new DummyCard(to_give);
                        room->obtainCard(receiver, dummy, false);
                        delete dummy;
                        if (rest_num == 0 || target->isKongcheng())
                            break;
                    }
                }
                room->broadcastSkillInvoke(objectName(), qrand() % 2 + 2);
            }
        } else if (triggerEvent == ChoiceMade) {
            QString str = data.toString();
            if (str.startsWith("Yiji:" + objectName()))
                target->addMark(objectName(), str.split(":").last().split("+").length());
        }
        return false;
    }
};

class Zishou: public DrawCardsSkill {
public:
    Zishou(): DrawCardsSkill("zishou") {
    }

    virtual int getDrawNum(ServerPlayer *liubiao, int n) const{
        Room *room = liubiao->getRoom();
        if (liubiao->isWounded() && room->askForSkillInvoke(liubiao, objectName())) {
            int losthp = liubiao->getLostHp();
            room->broadcastSkillInvoke(objectName(), qMin(3, losthp));
            liubiao->clearHistory();
            liubiao->skip(Player::Play);
            return n + losthp;
        } else
            return n;
    }
};

class Zongshi: public MaxCardsSkill {
public:
    Zongshi(): MaxCardsSkill("zongshi") {
    }

    virtual int getExtra(const Player *target) const{
        int extra = 0;
        QSet<QString> kingdom_set;
        if (target->parent()) {
            foreach (const Player *player, target->parent()->findChildren<const Player *>()) {
                if (player->isAlive())
                    kingdom_set << player->getKingdom();
            }
        }
        extra = kingdom_set.size();
        if (target->hasSkill(objectName()))
            return extra;
        else
            return 0;
    }
};

class Shiyong: public TriggerSkill {
public:
    Shiyong(): TriggerSkill("shiyong") {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash")
            && (damage.card->isRed() || damage.card->hasFlag("drank"))) {
            int index = 1;
            if (damage.from->getGeneralName().contains("guanyu"))
                index = 3;
            else if (damage.card->hasFlag("drank"))
                index = 2;
            room->broadcastSkillInvoke(objectName(), index);

            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            room->loseMaxHp(player);
        }
        return false;
    }
};

GongqiCard::GongqiCard() {
    mute = true;
    target_fixed = true;
}

void GongqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->setPlayerFlag(source, "InfinityAttackRange");
    const Card *cd = Sanguosha->getCard(subcards.first());
    if (cd->isKindOf("EquipCard")) {
        room->broadcastSkillInvoke("gongqi", 2);
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(source))
            if (source->canDiscard(p, "he")) targets << p;
        if (!targets.isEmpty()) {
            ServerPlayer *to_discard = room->askForPlayerChosen(source, targets, "gongqi", "@gongqi-discard", true);
            if (to_discard)
                room->throwCard(room->askForCardChosen(source, to_discard, "he", "gongqi", false, Card::MethodDiscard), to_discard, source);
        }
    } else {
        room->broadcastSkillInvoke("gongqi", 1);
    }
}

class Gongqi: public OneCardViewAsSkill {
public:
    Gongqi(): OneCardViewAsSkill("gongqi") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("GongqiCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        GongqiCard *card = new GongqiCard;
        card->addSubcard(originalcard->getId());
        card->setSkillName(objectName());
        return card;
    }
};

JiefanCard::JiefanCard() {
    mute = true;
}

bool JiefanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void JiefanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->removePlayerMark(source, "@rescue");
    ServerPlayer *target = targets.first();
    source->tag["JiefanTarget"] = QVariant::fromValue(target);
    room->broadcastSkillInvoke("jiefan");
    room->doLightbox("$JiefanAnimate", 2500);

    foreach (ServerPlayer *player, room->getAllPlayers()) {
        if (player->isAlive() && player->inMyAttackRange(target))
            room->cardEffect(this, source, player);
    }
    source->tag.remove("JiefanTarget");
}

void JiefanCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    ServerPlayer *target = effect.from->tag["JiefanTarget"].value<ServerPlayer *>();
    QVariant data = effect.from->tag["JiefanTarget"];
    if (target && !room->askForCard(effect.to, ".Weapon", "@jiefan-discard::" + target->objectName(), data))
        target->drawCards(1, "jiefan");
}

class Jiefan: public ZeroCardViewAsSkill {
public:
    Jiefan(): ZeroCardViewAsSkill("jiefan") {
        frequency = Limited;
        limit_mark = "@rescue";
    }

    virtual const Card *viewAs() const{
        return new JiefanCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@rescue") >= 1;
    }
};

AnxuCard::AnxuCard() {
    mute = true;
}

bool AnxuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (to_select == Self)
        return false;
    if (targets.isEmpty())
        return true;
    else if (targets.length() == 1)
        return to_select->getHandcardNum() != targets.first()->getHandcardNum();
    else
        return false;
}

bool AnxuCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void AnxuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> selecteds = targets;
    ServerPlayer *from = selecteds.first()->getHandcardNum() < selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
    ServerPlayer *to = selecteds.takeFirst();
    if (from->getGeneralName().contains("sunquan"))
        room->broadcastSkillInvoke("anxu", 2);
    else
        room->broadcastSkillInvoke("anxu", 1);
    int id = room->askForCardChosen(from, to, "h", "anxu");
    const Card *cd = Sanguosha->getCard(id);
    from->obtainCard(cd);
    room->showCard(from, id);
    if (cd->getSuit() != Card::Spade)
        source->drawCards(1, "anxu");
}

class Anxu: public ZeroCardViewAsSkill {
public:
    Anxu(): ZeroCardViewAsSkill("anxu") {
    }

    virtual const Card *viewAs() const{
        return new AnxuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("AnxuCard");
    }
};

class Zhuiyi: public TriggerSkill {
public:
    Zhuiyi(): TriggerSkill("zhuiyi") {
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        QList<ServerPlayer *> targets = (death.damage && death.damage->from) ? room->getOtherPlayers(death.damage->from) :
                                                                               room->getAlivePlayers();

        if (targets.isEmpty())
            return false;

        QString prompt = "zhuiyi-invoke";
        if (death.damage && death.damage->from && death.damage->from != player)
            prompt = QString("%1x:%2").arg(prompt).arg(death.damage->from->objectName());
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, true, true);
        if (!target) return false;

        if (target->getGeneralName().contains("sunquan"))
            room->broadcastSkillInvoke(objectName(), 2);
        else
            room->broadcastSkillInvoke(objectName(), 1);

        target->drawCards(3, objectName());
        room->recover(target, RecoverStruct(player), true);
        return false;
    }
};

class LihuoViewAsSkill: public OneCardViewAsSkill {
public:
    LihuoViewAsSkill(): OneCardViewAsSkill("lihuo") {
        filter_pattern = "%slash";
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
        Card *acard = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

class Lihuo: public TriggerSkill {
public:
    Lihuo(): TriggerSkill("lihuo") {
        events << PreDamageDone << CardFinished;
        view_as_skill = new LihuoViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()) {
                QVariantList slash_list = damage.from->tag["InvokeLihuo"].toList();
                slash_list << QVariant::fromValue(damage.card);
                damage.from->tag["InvokeLihuo"] = QVariant::fromValue(slash_list);
            }
        } else if (TriggerSkill::triggerable(player) && !player->hasFlag("Global_ProcessBroken")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;

            bool can_invoke = false;
            QVariantList slash_list = use.from->tag["InvokeLihuo"].toList();
            foreach (QVariant card, slash_list) {
                if (card.value<const Card *>() == use.card) {
                    can_invoke = true;
                    slash_list.removeOne(card);
                    use.from->tag["InvokeLihuo"] = QVariant::fromValue(slash_list);
                    break;
                }
            }
            if (!can_invoke) return false;

            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke("lihuo", 2);
            room->loseHp(player, 1);
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 1;
    }
};

class LihuoTargetMod: public TargetModSkill {
public:
    LihuoTargetMod(): TargetModSkill("#lihuo-target") {
        frequency = NotFrequent;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const{
        if (from->hasSkill("lihuo") && card->isKindOf("FireSlash"))
            return 1;
        else
            return 0;
    }
};

ChunlaoCard::ChunlaoCard() {
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ChunlaoCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->addToPile("wine", this);
}

ChunlaoWineCard::ChunlaoWineCard() {
    m_skillName = "chunlao";
    mute = true;
    target_fixed = true;
}

void ChunlaoWineCard::use(Room *room, ServerPlayer *chengpu, QList<ServerPlayer *> &) const{
    if (chengpu->getPile("wine").isEmpty()) return;
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    QList<int> cards = chengpu->getPile("wine");
    room->fillAG(cards, chengpu);
    int card_id = room->askForAG(chengpu, cards, false, "chunlao");
    room->clearAG();
    if (card_id != -1) {
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "chunlao", QString());
        room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName("_chunlao");
        room->useCard(CardUseStruct(analeptic, who, who, false));
    }
}

class ChunlaoViewAsSkill: public ViewAsSkill {
public:
    ChunlaoViewAsSkill(): ViewAsSkill("chunlao") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@chunlao"
               || (pattern.contains("peach") && !player->getPile("wine").isEmpty());
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@chunlao")
            return to_select->isKindOf("Slash");
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@chunlao") {
            if (cards.length() == 0) return NULL;

            Card *acard = new ChunlaoCard;
            acard->addSubcards(cards);
            acard->setSkillName(objectName());
            return acard;
        } else {
            if (cards.length() != 0) return NULL;
            return new ChunlaoWineCard;
        }
    }
};

class Chunlao: public TriggerSkill {
public:
    Chunlao(): TriggerSkill("chunlao") {
        events << EventPhaseStart;
        view_as_skill = new ChunlaoViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *chengpu, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && chengpu->getPhase() == Player::Finish
            && !chengpu->isKongcheng() && chengpu->getPile("wine").isEmpty()) {
            room->askForUseCard(chengpu, "@@chunlao", "@chunlao", -1, Card::MethodNone);
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const{
        if (card->isKindOf("Analeptic")) {
            if (player->getGeneralName().contains("zhouyu"))
                return 3;
            else
                return 2;
        } else
            return 1;
    }
};

YJCM2012Package::YJCM2012Package()
    : Package("YJCM2012")
{
    General *bulianshi = new General(this, "bulianshi", "wu", 3, false); // YJ 101
    bulianshi->addSkill(new Anxu);
    bulianshi->addSkill(new Zhuiyi);

    General *chengpu = new General(this, "chengpu", "wu"); // YJ 103
    chengpu->addSkill(new Lihuo);
    chengpu->addSkill(new LihuoTargetMod);
    chengpu->addSkill(new Chunlao);
    related_skills.insertMulti("lihuo", "#lihuo-target");

    General *handang = new General(this, "handang", "wu"); // YJ 105
    handang->addSkill(new Gongqi);
    handang->addSkill(new Jiefan);

    General *huaxiong = new General(this, "huaxiong", "qun", 6); // YJ 106
    huaxiong->addSkill(new Shiyong);

    General *liubiao = new General(this, "liubiao", "qun", 4); // YJ 108
    liubiao->addSkill(new Zishou);
    liubiao->addSkill(new Zongshi);

    General *wangyi = new General(this, "wangyi", "wei", 3, false); // YJ 110
    wangyi->addSkill(new Zhenlie);
    wangyi->addSkill(new Miji);

    addMetaObject<ChunlaoCard>();
    addMetaObject<ChunlaoWineCard>();
    addMetaObject<GongqiCard>();
    addMetaObject<JiefanCard>();
    addMetaObject<AnxuCard>();
}

ADD_PACKAGE(YJCM2012)

