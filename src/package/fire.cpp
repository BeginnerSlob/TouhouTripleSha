#include "fire.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "engine.h"
#include "maneuvering.h"

IkYushenCard::IkYushenCard() {
}

bool IkYushenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getHp() > Self->getHp() && !to_select->isKongcheng();
}

void IkYushenCard::use(Room *room, ServerPlayer *xunyu, QList<ServerPlayer *> &targets) const{
    ServerPlayer *tiger = targets.first();

    bool success = xunyu->pindian(tiger, "ikyushen", NULL);
    if (success) {
        QList<ServerPlayer *> players = room->getOtherPlayers(tiger), wolves;
        foreach (ServerPlayer *player, players) {
            if (tiger->inMyAttackRange(player))
                wolves << player;
        }

        if (wolves.isEmpty()) {
            LogMessage log;
            log.type = "#IkYushenNoWolf";
            log.from = xunyu;
            log.to << tiger;
            room->sendLog(log);

            return;
        }

        ServerPlayer *wolf = room->askForPlayerChosen(xunyu, wolves, "ikyushen", QString("@ikyushen-damage:%1").arg(tiger->objectName()));
        room->damage(DamageStruct("ikyushen", tiger, wolf));
    } else {
        room->damage(DamageStruct("ikyushen", tiger, xunyu));
    }
}

class IkJieming: public MasochismSkill {
public:
    IkJieming(): MasochismSkill("ikjieming") {
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *to = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "ikjieming-invoke", true, true);
        if (to) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkJiemingTarget"] = QVariant::fromValue(to);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *xunyu, const DamageStruct &) const{
        ServerPlayer *to = xunyu->tag["IkJiemingTarget"].value<PlayerStar>();
        xunyu->tag.remove("IkJiemingTarget");
        if (to) {
            int upper = qMin(5, to->getMaxHp());
            int x = upper - to->getHandcardNum();
            if (x <= 0) return ;
            to->drawCards(x, objectName());
        }
    }
};

class IkYushen: public ZeroCardViewAsSkill {
public:
    IkYushen(): ZeroCardViewAsSkill("ikyushen") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkYushenCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new IkYushenCard;
    }
};

IkQiangxiCard::IkQiangxiCard() {
}

bool IkQiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    int rangefix = 0;
    if (!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);;
    }

    return Self->inMyAttackRange(to_select, rangefix);
}

void IkQiangxiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    if (subcards.isEmpty())
        room->loseHp(effect.from);

    room->damage(DamageStruct("ikqiangxi", effect.from, effect.to));
}

class IkQiangxi: public ViewAsSkill {
public:
    IkQiangxi(): ViewAsSkill("ikqiangxi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkQiangxiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return new IkQiangxiCard;
        else if (cards.length() == 1) {
            IkQiangxiCard *card = new IkQiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

class Luanji: public ViewAsSkill {
public:
    Luanji(): ViewAsSkill("luanji") {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.isEmpty())
            return !to_select->isEquipped();
        else if (selected.length() == 1) {
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 2) {
            ArcheryAttack *aa = new ArcheryAttack(Card::SuitToBeDecided, 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        } else
            return NULL;
    }
};

class Xueyi: public MaxCardsSkill {
public:
    Xueyi(): MaxCardsSkill("xueyi$") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasLordSkill(objectName())) {
            int extra = 0;
            QList<const Player *> players = target->getAliveSiblings();
            foreach (const Player *player, players) {
                if (player->getKingdom() == "qun")
                    extra += 2;
            }
            return extra;
        } else
            return 0;
    }
};

class ShuangxiongViewAsSkill: public OneCardViewAsSkill {
public:
    ShuangxiongViewAsSkill(): OneCardViewAsSkill("shuangxiong") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("shuangxiong") != 0 && !player->isKongcheng();
    }

    virtual bool viewFilter(const Card *card) const{
        if (card->isEquipped())
            return false;

        int value = Self->getMark("shuangxiong");
        if (value == 1)
            return card->isBlack();
        else if (value == 2)
            return card->isRed();

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Duel *duel = new Duel(originalCard->getSuit(), originalCard->getNumber());
        duel->addSubcard(originalCard);
        duel->setSkillName(objectName());
        return duel;
    }
};

class Shuangxiong: public TriggerSkill {
public:
    Shuangxiong(): TriggerSkill("shuangxiong") {
        events << EventPhaseStart << FinishJudge << EventPhaseChanging;
        view_as_skill = new ShuangxiongViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *shuangxiong, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (shuangxiong->getPhase() == Player::Start) {
                room->setPlayerMark(shuangxiong, "shuangxiong", 0);
            } else if (shuangxiong->getPhase() == Player::Draw && TriggerSkill::triggerable(shuangxiong)) {
                if (shuangxiong->askForSkillInvoke(objectName())) {
                    room->setPlayerFlag(shuangxiong, "shuangxiong");

                    room->broadcastSkillInvoke("shuangxiong", 1);
                    JudgeStruct judge;
                    judge.good = true;
                    judge.play_animation = false;
                    judge.reason = objectName();
                    judge.who = shuangxiong;

                    room->judge(judge);
                    room->setPlayerMark(shuangxiong, "shuangxiong", judge.card->isRed() ? 1 : 2);

                    return true;
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == "shuangxiong" && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                shuangxiong->obtainCard(judge->card);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && shuangxiong->hasFlag("shuangxiong"))
                room->setPlayerFlag(shuangxiong, "-shuangxiong");
        }

        return false;
    }
};

class Mengjin: public TriggerSkill {
public:
    Mengjin():TriggerSkill("mengjin") {
        events << SlashMissed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *pangde, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isAlive() && pangde->canDiscard(effect.to, "he")) {
            if (pangde->askForSkillInvoke(objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                int to_throw = room->askForCardChosen(pangde, effect.to, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(Sanguosha->getCard(to_throw), effect.to, pangde);
            }
        }

        return false;
    }
};

class IkFuyao: public OneCardViewAsSkill {
public:
    IkFuyao(): OneCardViewAsSkill("ikfuyao") {
        filter_pattern = ".|club";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }
};

class IkNiepan: public TriggerSkill {
public:
    IkNiepan(): TriggerSkill("ikniepan") {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@niepan";
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(pangtong) || pangtong->getMark("@niepan") == 0)
            return QStringList();
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != pangtong)
            return QStringList();
        if (pangtong->isDead() || pangtong->getHp() > 0)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data, ServerPlayer *) const{
        if (pangtong->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data, ServerPlayer *) const{
        room->removePlayerMark(pangtong, "@niepan");
        room->addPlayerMark(pangtong, "@niepanused");

        QList<const Card *> tricks = pangtong->getJudgingArea();
        foreach (const Card *trick, tricks) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, pangtong->objectName());
            room->throwCard(trick, reason, NULL);
        }

        if (!pangtong->faceUp())
            pangtong->turnOver();

        if (pangtong->isChained())
            room->setPlayerProperty(pangtong, "chained", false);

        pangtong->drawCards(3, objectName());

        room->recover(pangtong, RecoverStruct(pangtong, NULL, 3 - pangtong->getHp()));

        return false;
    }
};

class Huoji: public OneCardViewAsSkill {
public:
    Huoji(): OneCardViewAsSkill("huoji") {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        return fire_attack;
    }
};

class Bazhen: public TriggerSkill {
public:
    Bazhen(): TriggerSkill("bazhen") {
        frequency = Compulsory;
        events << CardAsked;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->getArmor() && target->hasArmorEffect("eight_diagram");
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *wolong, QVariant &data) const{
        QString pattern = data.toStringList().first();

        if (pattern != "jink")
            return false;

        if (wolong->askForSkillInvoke(objectName())) {
            JudgeStruct judge;
            judge.pattern = ".|red";
            judge.good = true;
            judge.reason = objectName();
            judge.who = wolong;

            room->judge(judge);

            if (judge.isGood()) {
                room->setEmotion(wolong, "armor/eight_diagram");
                Jink *jink = new Jink(Card::NoSuit, 0);
                jink->setSkillName(objectName());
                room->provide(jink);
                return true;
            }
        }

        return false;
    }
};

class Kanpo: public OneCardViewAsSkill {
public:
    Kanpo(): OneCardViewAsSkill("kanpo") {
        filter_pattern = ".|black|.|hand";
        response_pattern = "nullification";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        foreach (const Card *card, player->getHandcards()) {
            if (card->isBlack()) return true;
        }
        return false;
    }
};

TianyiCard::TianyiCard() {
}

bool TianyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void TianyiCard::use(Room *room, ServerPlayer *taishici, QList<ServerPlayer *> &targets) const{
    bool success = taishici->pindian(targets.first(), "tianyi", NULL);
    if (success)
        room->setPlayerFlag(taishici, "TianyiSuccess");
    else
        room->setPlayerCardLimitation(taishici, "use", "Slash", true);
}

class Tianyi: public ZeroCardViewAsSkill {
public:
    Tianyi(): ZeroCardViewAsSkill("tianyi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("TianyiCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new TianyiCard;
    }
};

class TianyiTargetMod: public TargetModSkill {
public:
    TianyiTargetMod(): TargetModSkill("#tianyi-target") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("TianyiSuccess"))
            return 1;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasFlag("TianyiSuccess"))
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasFlag("TianyiSuccess"))
            return 1;
        else
            return 0;
    }
};

FirePackage::FirePackage()
    : Package("fire")
{
    General *bloom012 = new General(this, "bloom012", "hana");
    bloom012->addSkill(new IkQiangxi);

    General *bloom013 = new General(this, "bloom013", "hana", 3);
    bloom013->addSkill(new IkYushen);
    bloom013->addSkill(new IkJieming);

    General *wind010 = new General(this, "wind010", "kaze", 3);
    wind010->addSkill(new IkFuyao);
    wind010->addSkill(new IkNiepan);

    General *wolong = new General(this, "wolong", "shu", 3); // SHU 011
    wolong->addSkill(new Huoji);
    wolong->addSkill(new Kanpo);
    wolong->addSkill(new Bazhen);

    General *taishici = new General(this, "taishici", "wu"); // WU 012
    taishici->addSkill(new Tianyi);
    taishici->addSkill(new TianyiTargetMod);
    related_skills.insertMulti("tianyi", "#tianyi-target");

    General *yuanshao = new General(this, "yuanshao$", "qun"); // QUN 004
    yuanshao->addSkill(new Luanji);
    yuanshao->addSkill(new Xueyi);

    General *yanliangwenchou = new General(this, "yanliangwenchou", "qun"); // QUN 005
    yanliangwenchou->addSkill(new Shuangxiong);

    General *pangde = new General(this, "pangde", "qun"); // QUN 008
    pangde->addSkill("mashu");
    pangde->addSkill(new Mengjin);

    addMetaObject<IkYushenCard>();
    addMetaObject<IkQiangxiCard>();
    addMetaObject<TianyiCard>();
}

ADD_PACKAGE(Fire)

