#include "bangai.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"
#include "standard-equips.h"

class ThBianfang: public TriggerSkill {
public:
    ThBianfang(): TriggerSkill("thbianfang") {
        events << Damage << Damaged;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        DamageStruct damage = data.value<DamageStruct>();
        if (!TriggerSkill::triggerable(player) || !player->isWounded() || !damage.card || !damage.card->isKindOf("Slash"))
            return QStringList();
        if (triggerEvent == Damage && (damage.chain || damage.transfer))
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (!player->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName());
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        QStringList chosen;
        int n = qMin(3, player->getLostHp());
        QStringList choices;
        choices << "spade" << "heart" << "club" << "diamond";
        for (int i = 0; i < n; i++) {
            QString choice = room->askForChoice(player, objectName(), choices.join("+"));
            LogMessage log;
            log.type = "#ChooseSuit";
            log.from = player;
            log.arg  = choice;
            room->sendLog(log);
            choices.removeOne(choice);
            chosen << choice;
        }
        
        JudgeStruct judge;
        judge.pattern = ".|" + chosen.join(",");
        judge.good = true;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        if (judge.isGood()) {
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if (player->canDiscard(p, "he"))
                    targets << p;
            
            ServerPlayer *target = NULL;
            if (!targets.isEmpty())
                target = room->askForPlayerChosen(player, targets, objectName(), "@thbianfang", true);

            if (target) {
                int card_id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, target, player);
            } else {
                room->recover(player, RecoverStruct(player));
            }
        }
        return false;
    }
};

class ThChuandao: public TriggerSkill {
public:
    ThChuandao(): TriggerSkill("thchuandao") {
        events << DrawNCards << Death;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == DrawNCards && player->getMark("@yaoshu") > 0)
            return QStringList(objectName());
        else if (triggerEvent == Death && player->hasSkill(objectName())) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
            if (player->getMark("@yaoshu")) return QStringList(objectName());
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->getMark("@yaoshu") > 0)
                    return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == DrawNCards && player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        } else if (triggerEvent == Death) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName(), "@thchuandao", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThChuandaoTarget"] = QVariant::fromValue(target);
                return true;
            }
        }
        return false;
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == DrawNCards)
            data = data.toInt() + 1;
        else if (triggerEvent == Death) {
            ServerPlayer *target = player->tag["ThChuandaoTarget"].value<PlayerStar>();
            player->tag.remove("ThChuandaoTarget");
            if (target) {
                if (target->getMark("@yaoshu") < 1)
                    target->gainMark("@yaoshu");
                foreach (ServerPlayer *p, room->getOtherPlayers(target))
                    room->setPlayerMark(p, "@yaoshu", 0);

                target->drawCards(2);
                room->acquireSkill(target, objectName());
            }
        }

        return false;
    }
};

ThShoujuanCard::ThShoujuanCard() {
}

void ThShoujuanCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->removePlayerMark(effect.from, "@yaoshu");
    effect.to->gainMark("@yaoshu");
}

class ThShoujuanViewAsSkill: public ZeroCardViewAsSkill {
public:
    ThShoujuanViewAsSkill(): ZeroCardViewAsSkill("thshoujuan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@yaoshu") > 0;
    }

    virtual const Card *viewAs() const{
        return new ThShoujuanCard;
    }
};

class ThShoujuan: public TriggerSkill {
public:
    ThShoujuan(): TriggerSkill("thshoujuan") {
        events << Dying << Damage;
        view_as_skill = new ThShoujuanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.from || damage.from->getMark("@yaoshu") <= 0 || !damage.to->hasSkill(objectName()) || damage.from == damage.to)
                return QStringList();
            ask_who = damage.to;
            return QStringList(objectName());
        } else if (triggerEvent == Dying && TriggerSkill::triggerable(player)) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who == player || dying.who->getMark("@yaoshu") <= 0)
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        if (triggerEvent == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            player = dying.who;
        }
        ask_who->turnOver();
        if (!player->isKongcheng()) {
            DummyCard *handcards = player->wholeHandCards();
            ask_who->obtainCard(handcards, false);
            delete handcards;
        }
        room->removePlayerMark(player, "@yaoshu");
        ask_who->gainMark("@yaoshu");
        return false;
    }
};

class ThShuling: public TriggerSkill{
public:
    ThShuling(): TriggerSkill("thshuling"){
        events << GameStart;
        frequency = Compulsory;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        player->gainMark("@yaoshu");
        return false;
    }
};

class ThBanyue: public TriggerSkill{
public:
    ThBanyue():TriggerSkill("thbanyue"){
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!TriggerSkill::triggerable(player) || !use.card->isKindOf("Slash"))
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardUseStruct use = data.value<CardUseStruct>();
        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = player;
        room->judge(judge);

        if (judge.card->isRed())
            room->setCardFlag(use.card, "thbanyuered");
        else if (judge.card->isBlack()) {
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if (player->canSlash(p, use.card) && !use.to.contains(p))
                    targets << p;
            if (targets.isEmpty())
                return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            use.to << target;
            
            LogMessage log;
            log.type = "#ThBanyue";
            log.from = player;
            log.to << target;
            log.arg = objectName();
            log.arg2 = use.card->objectName();
            room->sendLog(log);
            
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class ThBanyueDiscard: public TriggerSkill{
public:
    ThBanyueDiscard():TriggerSkill("#thbanyue-discard") {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->hasFlag("thbanyuered"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to)
            if (player->canDiscard(p, "he")) {
                int card_id = room->askForCardChosen(player, p, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, p, player);
            }
        return false;
    }
};

ThZushaCard::ThZushaCard() {
}

void ThZushaCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->gainMark("@zuzhou");
}

class ThZusha: public OneCardViewAsSkill {
public:
    ThZusha(): OneCardViewAsSkill("thzusha") {
        filter_pattern = ".|spade|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThZushaCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        ThZushaCard *card = new ThZushaCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThZushaJudge: public TriggerSkill {
public:
    ThZushaJudge(): TriggerSkill("#thzusha-judge") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && target->getMark("@zuzhou") > 0
            && target->getPhase() == Player::Judge;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        LogMessage log;
        log.type = "#ThZusha";
        log.from = player;
        log.arg = "thzusha";
        room->sendLog(log);

        JudgeStruct judge;
        judge.pattern = ".|red";
        judge.good = true;
        judge.negative = true;
        judge.who = player;
        judge.reason = "thzusha";
        room->judge(judge);

        if (judge.isGood())
            player->loseMark("@zuzhou");
        else
            room->loseHp(player);

        return false;
    }
};

ThYaomeiCard::ThYaomeiCard() {
    mute = true;
}

bool ThYaomeiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    if (targets.isEmpty())
        return to_select->isWounded();
    else if (targets.size() == 1)
        return to_select->getLostHp() >= targets.first()->getLostHp();
    else
        return false;
}

bool ThYaomeiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const {
    return targets.length() == 2;
}

void ThYaomeiCard::onUse(Room *room, const CardUseStruct &card_use) const {
    CardUseStruct use = card_use;
    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, card_use.from, data);
    use = data.value<CardUseStruct>();

    room->broadcastSkillInvoke("thyaomei");

    LogMessage log;
    log.from = card_use.from;
    log.to << card_use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), "thyaomei", QString());
    room->moveCardTo(this, card_use.from, NULL, Player::DiscardPile, reason, true);

    thread->trigger(CardUsed, room, card_use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, card_use.from, data);
}

void ThYaomeiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    room->removePlayerMark(source, "@yaomei");
    room->addPlayerMark(source, "@yaomeiused");
    room->loseHp(targets.first());
    room->recover(targets.last(), RecoverStruct(source));
}

class ThYaomei: public OneCardViewAsSkill {
public:
    ThYaomei(): OneCardViewAsSkill("thyaomei") {
        frequency = Limited;
        limit_mark = "@yaomei";
        filter_pattern = ".|red|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@yaomei") > 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThYaomeiCard *card = new ThYaomeiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThZhongjie: public TriggerSkill {
public:
    ThZhongjie(): TriggerSkill("thzhongjie") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player) || player->getArmor())
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage > 1)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        damage.damage = 1;

        data = QVariant::fromValue(damage);
        return false;
    }
};

class ThXijing: public TriggerSkill {
public:
    ThXijing(): TriggerSkill("thxijing") {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)
            || (room->getCurrent() == player && player->getPhase() != Player::NotActive)
            || player->isKongcheng() || player->hasFlag("thxijing_using"))
            return QStringList();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player || move.to_place != Player::DiscardPile)
            return QStringList();
        
        for (int i = 0; i < move.card_ids.length(); i++) {
            int id = move.card_ids[i];
            if (move.from_places[i] != Player::PlaceJudge && move.from_places[i] != Player::PlaceSpecial
                && !Sanguosha->getCard(id)->isKindOf("EquipCard")
                && room->getCardPlace(id) == Player::DiscardPile) {
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        for (int i = 0; i < move.card_ids.length(); i++) {
            int id = move.card_ids[i];
            if (move.from_places[i] != Player::PlaceJudge && move.from_places[i] != Player::PlaceSpecial
                && !Sanguosha->getEngineCard(id)->isKindOf("EquipCard")
                && room->getCardPlace(id) == Player::DiscardPile) {
                const Card *c = Sanguosha->getEngineCard(id);
                QString prompt = "@thxijing:" + c->getSuitString()
                                              + ":" + QString::number(c->getNumber())
                                              + ":" + c->objectName();
                QString pattern = ".";
                if (c->isBlack())
                    pattern = ".black";
                else if (c->isRed())
                    pattern = ".red";
                const Card *card = room->askForCard(player, pattern, prompt, QVariant(), Card::MethodNone);
                
                if (card) {
                    room->setPlayerFlag(player, "thxijing_using");
                    CardMoveReason reason(CardMoveReason::S_REASON_PUT,
                                          player->objectName(),
                                          objectName(),
                                          QString());
                    room->throwCard(card, reason, player);
                    room->setPlayerFlag(player, "-thxijing_using");
                    room->obtainCard(player, Sanguosha->getCard(id));
                }
            }
        }

        return false;
    }
};

class ThMengwei: public TriggerSkill {
public:
    ThMengwei():TriggerSkill("thmengwei") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        QMap<ServerPlayer *, QStringList> skill_list;
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish && player->getHandcardNum() < 2)
            skill_list.insert(player, QStringList(objectName()));
        else if (player->getPhase() == Player::Start)
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                if (owner != player && owner->isKongcheng())
                    skill_list.insert(owner, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        if (player->getPhase() == Player::Finish)
            ask_who->drawCards(2 - ask_who->getHandcardNum());
        else
            ask_who->drawCards(1);
        return false;
    }
};

class ThSilianFilterSkill: public FilterSkill {
public:
    ThSilianFilterSkill(): FilterSkill("thsilian") {
    }

    virtual bool viewFilter(const Card* to_select) const {
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return place == Player::PlaceHand && to_select->isKindOf("Weapon");
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class ThSilian: public TriggerSkill {
public:
    ThSilian():TriggerSkill("thsilian") {
        events << GameStart << BeforeCardsMove;
        frequency = Compulsory;
        view_as_skill = new ThSilianFilterSkill;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (triggerEvent == GameStart) {
            if (player) return QStringList();
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(Sanguosha->getSkill("blade"));
            room->getThread()->addTriggerSkill(trigger_skill);
            return QStringList();
        }
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!TriggerSkill::triggerable(player) || move.to != player)
            return QStringList();
        if (move.to_place == Player::PlaceEquip) {
            for (int i = 0; i < move.card_ids.length(); i++) {
                int card_id = move.card_ids[i];
                if (Sanguosha->getEngineCard(card_id)->isKindOf("Weapon"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        DummyCard *dummy = new DummyCard;
        dummy->deleteLater();
        for (int i = 0; i < move.card_ids.length(); i++) {
            int card_id = move.card_ids[i];
            if (Sanguosha->getEngineCard(card_id)->isKindOf("Weapon"))
                dummy->addSubcard(card_id);
        }
        if (dummy->subcardsLength() > 0) {
            move.removeCardIds(dummy->getSubcards());
            room->obtainCard(player, dummy);
            data = QVariant::fromValue(move);
        }
        return false;
    }
};

class ThLingzhanViewAsSkill: public OneCardViewAsSkill {
public:
    ThLingzhanViewAsSkill(): OneCardViewAsSkill("thlingzhan") {
        expand_pile = "lingzhanpile";
        filter_pattern = ".|.|.|lingzhanpile";
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        Slash *card = new Slash(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->getPile("lingzhanpile").isEmpty() && Slash::IsAvailable(player, NULL);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        return !player->getPile("lingzhanpile").isEmpty() && pattern == "slash";
    }
};

class ThLingzhan: public TriggerSkill {
public:
    ThLingzhan():TriggerSkill("thlingzhan") {
        events << Damage;
        view_as_skill = new ThLingzhanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (TriggerSkill::triggerable(player)
            && damage.card->isKindOf("Slash")
            && !damage.chain && !damage.transfer)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.who = player;
        judge.reason = objectName();
        room->judge(judge);
        return false;
    }
};

class ThLingzhanPut: public TriggerSkill {
public:
    ThLingzhanPut():TriggerSkill("#thlingzhan") {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == "thlingzhan") {
                if (judge->isGood()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        JudgeStar judge = data.value<JudgeStar>();
        player->addToPile("lingzhanpile", judge->card);

        return false;
    }
};

class ThXiangrui: public TriggerSkill {
public:
    ThXiangrui():TriggerSkill("thxiangrui") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)
            || player->getPhase() != Player::Discard
            || player->getHandcardNum() == qMax(player->getHp(), 0))
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->getHandcardNum() > qMax(player->getHp(), 0)) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thxiangrui", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThXiangruiTarget"] = QVariant::fromValue(target);
                return true;
            }
        } else if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        int x = player->getHandcardNum() - qMax(player->getHp(), 0);
        if (x > 0) {
            ServerPlayer *target = player->tag["ThXiangruiTarget"].value<PlayerStar>();
            player->tag.remove("ThXiangruiTarget");
            if (target)
                target->drawCards(x);
        } else if (x < 0)
            player->drawCards(-x);

        return false;
    }
};

ThXingxieCard::ThXingxieCard() {
    target_fixed = true;
}

void ThXingxieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    source->throwAllHandCards();
    room->removePlayerMark(source, "@xingxie");
    room->addPlayerMark(source, "@xingxieused");
    
    DummyCard *dummy = new DummyCard;
    foreach(ServerPlayer *p, room->getOtherPlayers(source)) {
        dummy->addSubcards(p->getEquips());
        if (dummy->subcardsLength() == 0)
            continue;
        p->obtainCard(dummy);
        dummy->clearSubcards();
    }
    delete dummy;
}

class ThXingxie: public ZeroCardViewAsSkill {
public:
    ThXingxie(): ZeroCardViewAsSkill("thxingxie") {
        frequency = Limited;
        limit_mark = "@xingxie";
    }

    virtual const Card *viewAs() const {
        return new ThXingxieCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return player->canDiscard(player, "h") && player->getMark("@xingxie") > 0 && !player->isKongcheng();
    }
};

ThYuboCard::ThYuboCard() {
}

bool ThYuboCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    return targets.length() < 2 && !to_select->isChained();
}

void ThYuboCard::onEffect(const CardEffectStruct &effect) const {
    effect.to->setChained(true);
    Room *room = effect.from->getRoom();
    room->broadcastProperty(effect.to, "chained");
    room->setEmotion(effect.to, "chain");
    room->getThread()->trigger(ChainStateChanged, room, effect.to);
}

class ThYubo: public OneCardViewAsSkill {
public:
    ThYubo(): OneCardViewAsSkill("thyubo") {
        filter_pattern = ".|black|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThYuboCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThYuboCard *card = new ThYuboCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ThQiongfa: public TriggerSkill {
public:
    ThQiongfa(): TriggerSkill("thqiongfa") {
        events << EventPhaseStart;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (!player->isChained() || player->getPhase() != Player::Finish)
            return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
            skill_list.insert(owner, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (player->canDiscard(p, "he"))
                victims << p;
        if (ask_who == player) {
            ServerPlayer *victim = room->askForPlayerChosen(ask_who, victims, objectName(), "@thqiongfa", true);
            if (victim) {
                int card_id = room->askForCardChosen(player, victim, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, victim, player);
            } else
                ask_who->drawCards(1);
        } else {
            ServerPlayer *victim = NULL;
            if (!victims.isEmpty()) {
                victim = room->askForPlayerChosen(ask_who, victims, objectName());
                LogMessage log;
                log.type = "#ThQiongfa";
                log.from = ask_who;
                log.to << victim;
                room->sendLog(log);
            }

            if (victim && room->askForChoice(player, objectName(), "discard+cancel") == "discard") {
                int card_id = room->askForCardChosen(player, victim, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, victim, player);
            } else
                ask_who->drawCards(1);
        }

        player->setChained(false);
        room->broadcastProperty(player, "chained");
        room->setEmotion(player, "chain");
        room->getThread()->trigger(ChainStateChanged, room, player);

        return false;
    }
};

class ThWeide: public DrawCardsSkill {
public:
    ThWeide(): DrawCardsSkill("thweide") {
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player) || !player->isWounded())
            return QStringList();
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (p->isWounded())
                return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            if (p->isWounded())
                targets << p;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thweide", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThWeideTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        int l_hp = qMin(player->getLostHp(), 2);
        return n - l_hp;
    }
};

class ThWeideAct: public TriggerSkill {
public:
    ThWeideAct(): TriggerSkill("#thweide") {
        events << AfterDrawNCards;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player->tag["ThWeideTarget"].isNull()) return QStringList();
        return QStringList(objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        ServerPlayer *target = player->tag["ThWeideTarget"].value<PlayerStar>();
        player->tag.remove("ThWeideTarget");
        if (target) {
            target->drawCards(qMin(player->getLostHp(), 2));
            QList<ServerPlayer *> victims;
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                if (qMax(p->getHp(), 0) >= player->getHp() && !p->isKongcheng())
                    victims << p;
            if (!victims.isEmpty()) {
                ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
                QList<int> ids = victim->handCards();
                DummyCard *dummy = new DummyCard;
                for (int i = 0; i < qMin(player->getLostHp(), 2); i++)
                    if (ids.isEmpty())
                        break;
                    else {
                        int id = ids.at(qrand() % ids.length());
                        dummy->addSubcard(id);
                        ids.removeOne(id);
                    }
                player->obtainCard(dummy);
                delete dummy;
            }
        }

        return false;
    }
};

ThGuijuanCard::ThGuijuanCard() {
    target_fixed = true;
}

void ThGuijuanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const {
    source->drawCards(1);
    const Card *card = source->getHandcards().last();
    room->showCard(source, card->getId());
    const Card *use = NULL;
    if (card->isAvailable(source))
        use = room->askForUseCard(source, QString::number(card->getId()), "@thguijuan");
    if (!use)
        room->loseHp(source);
    else if (use->isKindOf("Slash") || use->isKindOf("EquipCard"))
        room->setPlayerFlag(source, "ForbidThGuijuan");
}

class ThGuijuan: public ZeroCardViewAsSkill {
public:
    ThGuijuan(): ZeroCardViewAsSkill("thguijuan") {
    }

    virtual const Card *viewAs() const{
        return new ThGuijuanCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasFlag("ForbidThGuijuan");
    }
};

class ThZhayou: public TriggerSkill {
public:
    ThZhayou(): TriggerSkill("thzhayou") {
        events << SlashMissed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to && effect.to->isAlive() && effect.to->hasSkill(objectName())) {
            ask_who = effect.to;
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const {
        ask_who->drawCards(1);
        if (!room->askForUseSlashTo(player, ask_who, "@thzhayou:" + ask_who->objectName()))
            room->damage(DamageStruct(objectName(), ask_who, player));
        return false;
    }
};

class ThHuilun: public FilterSkill {
public:
    ThHuilun(): FilterSkill("thhuilun") {
    }

    virtual bool viewFilter(const Card* to_select) const {
        return (to_select->isKindOf("Slash") && to_select->isBlack())
            || (to_select->isKindOf("Peach") && to_select->isRed());
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        if (originalCard->isBlack()) {
            Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
            peach->setSkillName(objectName());
            WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
            card->takeOver(peach);
            return card;
        } else {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->setSkillName(objectName());
            WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
            card->takeOver(slash);
            return card;
        }
    }
};

ThWangdaoCard::ThWangdaoCard() {
    will_throw = false;
    handling_method = MethodNone;
}

void ThWangdaoCard::onEffect(const CardEffectStruct &effect) const {
    Room *room = effect.from->getRoom();
    room->showCard(effect.from, getEffectiveId());
    const Card *slash = NULL;
    if (effect.to->canSlash(effect.from)) {
        room->setPlayerCardLimitation(effect.to, "use", "Slash|red", false);
        slash = room->askForUseSlashTo(effect.to, effect.from, "@thwangdao:" + effect.from->objectName());
        room->removePlayerCardLimitation(effect.to, "use", "Slash|red$0");
    }
    if (!slash) {
        effect.to->obtainCard(this);
        room->loseHp(effect.to);
    }
}

class ThWangdao: public OneCardViewAsSkill {
public:
    ThWangdao(): OneCardViewAsSkill("thwangdao") {
        filter_pattern = "Peach";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThWangdaoCard *card = new ThWangdaoCard;
        card->addSubcard(originalCard);
        return card;
    }
};

ThSixiangCard::ThSixiangCard(){
}

bool ThSixiangCard::targetFixed() const {
    return Sanguosha->getCard(getEffectiveId())->getSuit() == Card::Spade;
}
    
bool ThSixiangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if(Sanguosha->getCard(getEffectiveId())->getSuit() != Card::Spade)
        return targets.size() == 1;
    else
        return targets.isEmpty();
}

bool ThSixiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Card::Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    if (suit == Card::Spade)
        return false;
    else if(suit == Card::Diamond)
        return targets.isEmpty() && Self->canDiscard(to_select, "hej") && to_select != Self;
    else
        return targets.isEmpty() && to_select != Self;
}

void ThSixiangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    Card::Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    ServerPlayer *target = NULL;
    if (!targets.isEmpty())
        target = targets.first();

    if(suit == Card::Spade) {
        if (source->isWounded())
            room->recover(source, RecoverStruct(source, this));
    } else if(suit == Card::Heart) {
        target->drawCards(1);
        target->turnOver();
    } else if(suit == Card::Club) {
        target->drawCards(2);
        if (!target->isNude())
            room->askForDiscard(target, "thsixiang", 1, 1, false, true);
    } else if(suit == Card::Diamond) {
        int card_id = room->askForCardChosen(source, target, "hej", "thsixiang");
        room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : target, source);
    }
}

class ThSixiang:public ViewAsSkill {
public:
    ThSixiang():ViewAsSkill("thsixiang") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const {
        if (Self->isJilei(to_select)) return false;
        if (selected.isEmpty())
            return true;
        else if (selected.length() == 1)
            return to_select->getSuit() == selected.first()->getSuit();
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const {
        if (cards.length() == 2) {
            ThSixiangCard *card = new ThSixiangCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

BangaiPackage::BangaiPackage()
    :Package("bangai")
{
    General *bangai001 = new General(this, "bangai001", "kaze");
    bangai001->addSkill(new ThBianfang);

    General *bangai002 = new General(this, "bangai002", "hana", 3);
    bangai002->addSkill(new ThChuandao);
    bangai002->addSkill(new ThShoujuan);
    bangai002->addSkill(new ThShuling);

    General *bangai003 = new General(this, "bangai003", "yuki");
    bangai003->addSkill(new ThBanyue);
    bangai003->addSkill(new ThBanyueDiscard);
    related_skills.insertMulti("thbanyue", "#thbanyue-discard");

    General *bangai004 = new General(this, "bangai004", "tsuki", 3, false);
    bangai004->addSkill(new ThZusha);
    bangai004->addSkill(new ThZushaJudge);
    related_skills.insertMulti("thzusha", "#thzusha-judge");
    bangai004->addSkill(new ThYaomei);
    bangai004->addSkill(new ThZhongjie);

    General *bangai005 = new General(this, "bangai005", "kaze", 3);
    bangai005->addSkill(new ThXijing);
    bangai005->addSkill(new ThMengwei);

    General *bangai006 = new General(this, "bangai006", "hana");
    bangai006->addSkill(new ThSilian);
    bangai006->addSkill(new ThLingzhan);
    bangai006->addSkill(new ThLingzhanPut);
    related_skills.insertMulti("thlingzhan", "#thlingzhan");

    General *bangai007 = new General(this, "bangai007", "yuki");
    bangai007->addSkill(new ThXiangrui);
    bangai007->addSkill(new ThXingxie);

    General *bangai008 = new General(this, "bangai008", "tsuki", 3);
    bangai008->addSkill(new ThYubo);
    bangai008->addSkill(new ThQiongfa);

    General *bangai009 = new General(this, "bangai009", "kaze");
    bangai009->addSkill(new ThWeide);
    bangai009->addSkill(new ThWeideAct);
    related_skills.insertMulti("thweide", "#thweide");

    General *bangai010 = new General(this, "bangai010", "hana", 3, false);
    bangai010->addSkill(new ThGuijuan);
    bangai010->addSkill(new ThZhayou);

    General *bangai011 = new General(this, "bangai011", "yuki", 3);
    bangai011->addSkill(new ThHuilun);
    bangai011->addSkill(new ThWangdao);

    General *bangai012 = new General(this, "bangai012", "tsuki");
    bangai012->addSkill(new ThSixiang);
    
    addMetaObject<ThShoujuanCard>();
    addMetaObject<ThZushaCard>();
    addMetaObject<ThYaomeiCard>();
    addMetaObject<ThXingxieCard>();
    addMetaObject<ThYuboCard>();
    addMetaObject<ThGuijuanCard>();
    addMetaObject<ThWangdaoCard>();
    addMetaObject<ThSixiangCard>();
}

ADD_PACKAGE(Bangai)