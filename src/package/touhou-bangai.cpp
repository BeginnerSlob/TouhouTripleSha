#include "touhou-bangai.h"
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.from != damage.to)
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (!player->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName());
        return true;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        int n = qMax(1, player->getLostHp());

        for (int i = 0; i < n; ++i) {
            JudgeStruct judge;
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card->getSuit() > Card::Diamond)
                judge.good = false;
            else {
                judge.pattern = ".|" + damage.card->getSuitString();
                judge.good = true;
            }
            judge.reason = objectName();
            judge.who = player;
            room->judge(judge);

            if (judge.isGood()) {
                ServerPlayer *target = triggerEvent == Damage ? damage.to : damage.from;
                if (target && target->isAlive() && !target->isNude()) {
                    int card_id = room->askForCardChosen(player, target, "he", objectName());
                    room->obtainCard(player, card_id, false);
                }
            }
        }

        return false;
    }
};

class ThShoujuan: public TriggerSkill {
public:
    ThShoujuan(): TriggerSkill("thshoujuan") {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setMark(objectName(), 0);
            }
            return QStringList();
        }
        if (TriggerSkill::triggerable(player)
            && player == room->getCurrent() && player->getPhase() != Player::NotActive
            && player->getMark(objectName()) < 3) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from != player
                && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip)))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue((ServerPlayer *)move.from))) {
            room->broadcastSkillInvoke(objectName());
            player->addMark(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ((ServerPlayer *)move.from)->drawCards(2, objectName());
        return false;
    }
};

ThMiqiCard::ThMiqiCard() {
}

bool ThMiqiCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const{
    Q_ASSERT(false);
    return false;
}

bool ThMiqiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select,
                                  const Player *Self, int &maxVotes) const{
    if (!Self->canDiscard(to_select, "ej"))
        return false;
    int i = 0;
    foreach (const Player *player, targets) {
        if (player == to_select)
            i++;
    }
    if (i == to_select->getEquips().length() + to_select->getJudgingArea().length())
        return false;
    int max = Self->getHandcardNum();
    maxVotes = qMax(max - targets.size(), 0) + i;
    return maxVotes > 0;
}

void ThMiqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    QMap<ServerPlayer *, int> map;

    foreach (ServerPlayer *sp, targets)
        map[sp]++;

    QList<ServerPlayer *> victims = map.keys();
    room->sortByActionOrder(victims);
    foreach (ServerPlayer *sp, victims)
        discard(source, sp, map[sp]);
}

void ThMiqiCard::discard(ServerPlayer *source, ServerPlayer *target, int num) const {
    Room *room = source->getRoom();
    room->setPlayerFlag(target, "thmiqi_InTempMoving");
    DummyCard *dummy = new DummyCard;
    QList<int> card_ids;
    QList<Player::Place> original_places;
    for (int i = 0; i < num; i++) {
        if (!source->canDiscard(target, "ej"))
            break;
        card_ids << room->askForCardChosen(source, target, "ej", objectName(), false, Card::MethodDiscard);
        original_places << room->getCardPlace(card_ids[i]);
        dummy->addSubcard(card_ids[i]);
        source->addToPile("#thmiqi", card_ids[i], false);
    }
    for (int i = 0; i < dummy->subcardsLength(); i++)
        room->moveCardTo(Sanguosha->getCard(card_ids[i]), target, original_places[i], false);
    room->setPlayerFlag(target, "-thmiqi_InTempMoving");
    if (dummy->subcardsLength() > 0)
        room->throwCard(dummy, target, source);
    delete dummy;
}

class ThMiqiVS: public ZeroCardViewAsSkill {
public:
    ThMiqiVS(): ZeroCardViewAsSkill("thmiqi") {
        response_pattern = "@@thmiqi";
    }

    virtual const Card *viewAs() const{
        return new ThMiqiCard;
    }
};

class ThMiqi: public TriggerSkill {
public:
    ThMiqi(): TriggerSkill("thmiqi") {
        events << EventPhaseChanging;
        view_as_skill = new ThMiqiVS;
    }

    bool isLeastHand(const ServerPlayer *target) const{
        foreach (ServerPlayer *p, target->getRoom()->getAlivePlayers()) {
            if (p->getHandcardNum() < target->getHandcardNum())
                return false;
        }
        return true;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && isLeastHand(target);
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        room->askForUseCard(player, "@@thmiqi", "@thmiqi", -1, Card::MethodNone);
        return false;
    }
};

class ThJibu: public DistanceSkill {
public:
    ThJibu(): DistanceSkill("thjibu") {
    }

    virtual int getCorrect(const Player *from, const Player *) const {
        if (from->hasSkill(objectName()))
            return -1;
        else
            return 0;
    }
};

class ThZhiyue: public TriggerSkill{
public:
    ThZhiyue():TriggerSkill("thzhiyue"){
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
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
            room->setCardFlag(use.card, "thzhiyuered");
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
            log.type = "#ThZhiyue";
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

class ThZhiyueDiscard: public TriggerSkill{
public:
    ThZhiyueDiscard():TriggerSkill("#thzhiyue-discard") {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->hasFlag("thzhiyuered"))
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

class ThZhongjie: public TriggerSkill {
public:
    ThZhongjie(): TriggerSkill("thzhongjie") {
        events << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        foreach (ServerPlayer *p, player->getRoom()->getAlivePlayers()) {
            if (!p->isKongcheng())
                return TriggerSkill::triggerable(player);
        }
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (!p->isKongcheng())
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@thzhongjie", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["ThZhongjieTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer *target = player->tag["ThZhongjieTarget"].value<ServerPlayer *>();
        player->tag.remove("ThZhongjieTarget");
        if (target) {
            room->showAllCards(target);
            QSet<QString> types;
            foreach (const Card *c, target->getHandcards())
                types << c->getType();
            int n = 3 - types.size();
            if (n > 0)
                target->drawCards(n, objectName());
        }
        return false;
    }
};

ThXumeiCard::ThXumeiCard() {
    target_fixed = true;
}

void ThXumeiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    QList<int> card_ids = room->getNCards(3);
    CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "thxumei", QString());
    room->moveCardsAtomic(CardsMoveStruct(card_ids, NULL, Player::PlaceTable, reason), true);
    QStringList choices;
    foreach (int id, card_ids) {
        QString str = Sanguosha->getCard(id)->getType();
        if (str == "skill")
            continue;
        if (choices.contains(str))
            continue;
        choices << str;
    }
    QString type = room->askForChoice(source, "thxumei", choices.join("+"), QVariant::fromValue(IntList2VariantList(card_ids)));
    ServerPlayer *target = room->askForPlayerChosen(source, room->getAllPlayers(), "thxumei", QString(), false, true);
    CardMoveReason reason2(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "thxumei", QString());
    DummyCard *dummy = new DummyCard;
    dummy->deleteLater();
    foreach(int id, card_ids) {
        if(Sanguosha->getCard(id)->getType() == type) {
            card_ids.removeOne(id);
            dummy->addSubcard(id);
        }
    }
    room->obtainCard(target, dummy);
    dummy->clearSubcards();
    dummy->addSubcards(card_ids);
    if (dummy->subcardsLength() > 0)
        room->throwCard(dummy, reason2, NULL);

    type[0] = type[0].toUpper();
    type += "Card";
    room->setPlayerCardLimitation(target, "use,response", type, false);

    QStringList limit = target->tag["ThXumeiList"].toStringList();
    limit << type;
    target->tag["ThXumeiList"] = QVariant::fromValue(limit);
}

class ThXumeiVS: public OneCardViewAsSkill {
public:
    ThXumeiVS(): OneCardViewAsSkill("thxumei") {
        filter_pattern = "BasicCard!";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new ThXumeiCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return !player->hasUsed("ThXumeiCard");
    }
};

class ThXumei: public TriggerSkill {
public:
    ThXumei(): TriggerSkill("thxumei") {
        events << EventPhaseChanging;
        view_as_skill = new ThXumeiVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                QStringList limit = p->tag["ThXumeiList"].toStringList();
                foreach (QString pattern, limit)
                    room->removePlayerCardLimitation(p, "use,response", pattern + "$0");
                p->tag.remove("ThXumeiList");
            }
        }
        return QStringList();
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

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const {
        TriggerList skill_list;
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish && player->getHandcardNum() < 2)
            skill_list.insert(player, QStringList(objectName()));
        else if (player->getPhase() == Player::Start)
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                if (owner != player && owner->isKongcheng())
                    skill_list.insert(owner, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
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
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (TriggerSkill::triggerable(player)
            && damage.card->isKindOf("Slash")
            && !damage.chain && !damage.transfer)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
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
            JudgeStruct *judge = data.value<JudgeStruct *>();
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

    virtual bool effect(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        player->addToPile("lingzhanpile", judge->card);

        return false;
    }
};

class ThQinshao: public TriggerSkill {
public:
    ThQinshao():TriggerSkill("thqinshao") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player)
            || player->getPhase() != Player::Discard
            || player->getHandcardNum() == qMax(player->getHp(), 0))
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->getHandcardNum() > qMax(player->getHp(), 0)) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@thqinshao", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                player->tag["ThQinshaoTarget"] = QVariant::fromValue(target);
                return true;
            }
        } else if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        int x = player->getHandcardNum() - qMax(player->getHp(), 0);
        if (x > 0) {
            ServerPlayer *target = player->tag["ThQinshaoTarget"].value<ServerPlayer *>();
            player->tag.remove("ThQinshaoTarget");
            if (target)
                target->drawCards(x);
        } else if (x < 0)
            player->drawCards(-x);

        return false;
    }
};

ThXingxieCard::ThXingxieCard() {
}

bool ThXingxieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.isEmpty() && to_select->hasEquip();
}

void ThXingxieCard::onEffect(const CardEffectStruct &effect) const{
    DummyCard *dummy = new DummyCard;
    dummy->addSubcards(effect.to->getEquips());
    effect.from->addToPile("thxingxiepile", dummy);
    effect.to->setFlags("ThXingxieTarget");
    delete dummy;
}

class ThXingxieVS: public OneCardViewAsSkill {
public:
    ThXingxieVS(): OneCardViewAsSkill("thxingxie") {
        filter_pattern = ".|.|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        Card *card = new ThXingxieCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        return player->canDiscard(player, "h") && !player->hasUsed("ThXingxieCard");
    }
};

class ThXingxie: public TriggerSkill {
public:
    ThXingxie(): TriggerSkill("thxingxie") {
        events << EventPhaseChanging;
        view_as_skill = new ThXingxieVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag("ThXingxieTarget")) {
                    p->setFlags("-ThXingxieTarget");
                    if (!player->getPile("thxingxiepile").isEmpty()) {
                        QList<int> card_ids = player->getPile("thxingxiepile");
                        foreach (int id, card_ids) {
                            room->obtainCard(p, id);
                            if (room->getCardOwner(id) != p)
                                continue;
                            if (room->getCardPlace(id) != Player::PlaceHand)
                                continue;
                            const Card *card = Sanguosha->getCard(id);
                            if (card->getTypeId() != Card::TypeEquip)
                                continue;
                            room->useCard(CardUseStruct(card, p, QList<ServerPlayer *>()));
                        }
                    }
                    break;
                }
            }
        }
        return QStringList();
    }
};

ThYuboCard::ThYuboCard() {
}

bool ThYuboCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const {
    return targets.length() < 2 && !to_select->isChained();
}

void ThYuboCard::onEffect(const CardEffectStruct &effect) const {
    effect.to->setChained(true);
    Room *room = effect.from->getRoom();
    room->broadcastProperty(effect.to, "chained");
    room->setEmotion(effect.to, "effects/iron_chain");
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

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        TriggerList skill_list;
        if (!player->isChained() || player->getPhase() != Player::Finish)
            return skill_list;
        foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
            skill_list.insert(owner, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
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
        room->setEmotion(player, "effects/iron_chain");
        room->getThread()->trigger(ChainStateChanged, room, player);

        return false;
    }
};

class ThWeide: public TriggerSkill {
public:
    ThWeide(): TriggerSkill("thweide") {
        events << DrawNCards;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (TriggerSkill::triggerable(player))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        QList<int> card_ids = room->getNCards(2, false);
        room->fillAG(card_ids, player);
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName());
        room->clearAG(player);
        room->moveCardsAtomic(CardsMoveStruct(card_ids,
                                              target,
                                              Player::PlaceHand,
                                              CardMoveReason(CardMoveReason::S_REASON_PREVIEWGIVE,
                                                             player->objectName(),
                                                             target->objectName(),
                                                             objectName(),
                                                             QString())),
                              false);
        if (target != player && player->isWounded()) {
            QList<ServerPlayer *> victims;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isKongcheng())
                    victims << p;
            }
            if (!victims.isEmpty()) {
                ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName(), "@thweide", true);
                if (victim) {
                    int card_id = room->askForCardChosen(player, victim, "h", objectName());
                    room->obtainCard(player, card_id, false);
                }
            }
        }

        data = 0;
        return true;
    }
};

ThGuijuanCard::ThGuijuanCard() {
    target_fixed = true;
}

void ThGuijuanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const {
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &ask_who) const {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to && effect.to->isAlive() && effect.to->hasSkill(objectName())) {
            ask_who = effect.to;
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const {
        if (ask_who->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const {
        ask_who->drawCards(1);
        if (!room->askForUseSlashTo(player, ask_who, "@thzhayou:" + ask_who->objectName(), false))
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
    int id = getEffectiveId();
    const Card *original = Sanguosha->getCard(id);
    room->showCard(effect.from, id);
    const Card *slash = NULL;
    if (effect.to->canSlash(effect.from)) {
        room->setPlayerCardLimitation(effect.to, "use", "Slash|^" + original->getSuitString(), false);
        slash = room->askForUseSlashTo(effect.to, effect.from, "@thwangdao:" + effect.from->objectName(), false);
        room->removePlayerCardLimitation(effect.to, "use", "Slash|^black$0");
    }
    if (!slash) {
        effect.to->obtainCard(this);
        if (effect.from->canDiscard(effect.to, "he") && room->askForChoice(effect.from, "thwangdao", "discard+lose") == "discard") {
            room->setPlayerFlag(effect.to, "thwangdao_InTempMoving");
            DummyCard *dummy = new DummyCard;
            QList<int> card_ids;
            QList<Player::Place> original_places;
            for (int i = 0; i < 2; i++) {
                if (!effect.from->canDiscard(effect.to, "he"))
                    break;
                card_ids << room->askForCardChosen(effect.from, effect.to, "he", "thwangdao", false, MethodDiscard);
                original_places << room->getCardPlace(card_ids[i]);
                dummy->addSubcard(card_ids[i]);
                effect.to->addToPile("#thwangdao", card_ids[i], false);
            }
            for (int i = 0; i < dummy->subcardsLength(); i++)
                room->moveCardTo(Sanguosha->getCard(card_ids[i]), effect.to, original_places[i], false);
            room->setPlayerFlag(effect.to, "-thwangdao_InTempMoving");
            if (dummy->subcardsLength() > 0)
                room->throwCard(dummy, effect.to, effect.from);
            dummy->deleteLater();
        } else
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

bool ThSixiangCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
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
	room->addPlayerMark(source, "thsixiang");
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

class ThSixiang: public ViewAsSkill {
public:
    ThSixiang(): ViewAsSkill("thsixiang") {
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

class ThSixiangDraw: public TriggerSkill {
public:
    ThSixiangDraw(): TriggerSkill("#thsixiang-draw") {
        events << EventPhaseChanging << EventPhaseStart;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                if (player->getMark("thsixiang") > 0)
                    room->setPlayerMark(player, "thsixiang", 0);
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish && player->getMark("thsixiang") > 1)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->drawCards(player->getMark("thsixiang") - 1, "thsixiang");
        return false;
    }
};

TouhouBangaiPackage::TouhouBangaiPackage()
    :Package("touhou-bangai")
{
    General *bangai001 = new General(this, "bangai001", "kaze");
    bangai001->addSkill(new ThBianfang);

    General *bangai002 = new General(this, "bangai002", "hana", 3);
    bangai002->addSkill(new ThShoujuan);
    bangai002->addSkill(new ThMiqi);
    bangai002->addSkill(new FakeMoveSkill("thmiqi"));
    related_skills.insertMulti("thmiqi", "#thmiqi-fake-move");

    General *bangai003 = new General(this, "bangai003", "yuki");
    bangai003->addSkill(new ThJibu);
    bangai003->addSkill(new ThZhiyue);
    bangai003->addSkill(new ThZhiyueDiscard);
    related_skills.insertMulti("thzhiyue", "#thzhiyue-discard");

    General *bangai004 = new General(this, "bangai004", "tsuki", 3, false);
    bangai004->addSkill(new ThZhongjie);
    bangai004->addSkill(new ThXumei);

    General *bangai005 = new General(this, "bangai005", "kaze", 3);
    bangai005->addSkill(new ThXijing);
    bangai005->addSkill(new ThMengwei);

    General *bangai006 = new General(this, "bangai006", "hana");
    bangai006->addSkill(new ThSilian);
    bangai006->addSkill(new ThLingzhan);
    bangai006->addSkill(new ThLingzhanPut);
    related_skills.insertMulti("thlingzhan", "#thlingzhan");
    bangai006->addSkill(new Skill("thyanmeng", Skill::Compulsory));

    General *bangai007 = new General(this, "bangai007", "yuki");
    bangai007->addSkill(new ThQinshao);
    bangai007->addSkill(new ThXingxie);

    General *bangai008 = new General(this, "bangai008", "tsuki", 3);
    bangai008->addSkill(new ThYubo);
    bangai008->addSkill(new ThQiongfa);

    General *bangai009 = new General(this, "bangai009", "kaze");
    bangai009->addSkill(new ThWeide);

    General *bangai010 = new General(this, "bangai010", "hana", 3, false);
    bangai010->addSkill(new ThGuijuan);
    bangai010->addSkill(new ThZhayou);

    General *bangai011 = new General(this, "bangai011", "yuki", 3);
    bangai011->addSkill(new ThHuilun);
    bangai011->addSkill(new ThWangdao);
    bangai011->addSkill(new FakeMoveSkill("thwangdao"));
    related_skills.insertMulti("thwangdao", "#thwangdao-fake-move");

    General *bangai012 = new General(this, "bangai012", "tsuki");
    bangai012->addSkill(new ThSixiang);
    bangai012->addSkill(new ThSixiangDraw);
    related_skills.insertMulti("thsixiang", "#thsixiang-draw");

    addMetaObject<ThMiqiCard>();
    addMetaObject<ThXumeiCard>();
    addMetaObject<ThXingxieCard>();
    addMetaObject<ThYuboCard>();
    addMetaObject<ThGuijuanCard>();
    addMetaObject<ThWangdaoCard>();
    addMetaObject<ThSixiangCard>();
}

ADD_PACKAGE(TouhouBangai)
