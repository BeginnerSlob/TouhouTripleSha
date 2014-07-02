#include "fire.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "engine.h"
#include "maneuvering.h"

class IkXinghuang: public ViewAsSkill {
public:
    IkXinghuang(): ViewAsSkill("ikxinghuang") {
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

class IkShuangniangViewAsSkill: public OneCardViewAsSkill {
public:
    IkShuangniangViewAsSkill(): OneCardViewAsSkill("ikshuangniang") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("ikshuangniang") != 0 && !player->isKongcheng();
    }

    virtual bool viewFilter(const Card *card) const{
        if (card->isEquipped())
            return false;

        int value = Self->getMark("ikshuangniang");
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

class IkShuangniang: public TriggerSkill {
public:
    IkShuangniang(): TriggerSkill("ikshuangniang") {
        events << EventPhaseStart << FinishJudge << EventPhaseChanging;
        view_as_skill = new IkShuangniangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Start) {
                room->setPlayerMark(player, "ikshuangniang", 0);
            } else if (player->getPhase() == Player::Draw && TriggerSkill::triggerable(player)) {
                if (player->askForSkillInvoke(objectName())) {
                    room->setPlayerFlag(player, "ikshuangniang");

                    room->broadcastSkillInvoke("ikshuangniang", 1);
                    JudgeStruct judge;
                    judge.good = true;
                    judge.play_animation = false;
                    judge.reason = objectName();
                    judge.who = player;

                    room->judge(judge);
                    room->setPlayerMark(player, "ikshuangniang", judge.card->isRed() ? 1 : 2);

                    return true;
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "ikshuangniang" && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                player->obtainCard(judge->card);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->hasFlag("ikshuangniang"))
                room->setPlayerFlag(player, "-ikshuangniang");
        }

        return false;
    }
};

class IkMengjin: public TriggerSkill {
public:
    IkMengjin():TriggerSkill("ikmengjin") {
        events << SlashMissed << Damage;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.to->isAlive() && player->canDiscard(effect.to, "he"))
                return QStringList(objectName());
        } else if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.transfer && !damage.chain
                && damage.card && damage.card->isKindOf("Slash") && damage.card->isRed())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *pangde, QVariant &, ServerPlayer *) const{
        if (pangde->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *pangde, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            int to_throw = room->askForCardChosen(pangde, effect.to, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(Sanguosha->getCard(to_throw), effect.to, pangde);
        } else
            pangde->drawCards(1);

        return false;
    }
};

FirePackage::FirePackage()
    : Package("fire")
{
    General *luna004 = new General(this, "luna004", "tsuki");
    luna004->addSkill(new IkXinghuang);

    General *luna005 = new General(this, "luna005", "tsuki");
    luna005->addSkill(new IkShuangniang);

    General *luna008 = new General(this, "luna008", "tsuki");
    luna008->addSkill("thjibu");
    luna008->addSkill(new IkMengjin);

}

ADD_PACKAGE(Fire)

