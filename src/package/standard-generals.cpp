#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "ai.h"
#include "settings.h"
#include "maneuvering.h"

class IkJiaoman: public MasochismSkill {
public:
    IkJiaoman(): MasochismSkill("ikjiaoman") {
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        DamageStruct damage = data.value<DamageStruct>();
        const Card *card = damage.card;
        if (card) {
            QList<int> ids;
            if (card->isVirtualCard())
                ids = card->getSubcards();
            else
                ids << card->getEffectiveId();
            if (ids.length() > 0) {
                bool all_place_table = true;
                foreach (int id, ids) {
                    if (room->getCardPlace(id) != Player::PlaceTable) {
                        all_place_table = false;
                        break;
                    }
                }
                if (all_place_table) return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        QList<ServerPlayer *> targets = damage.from ? room->getOtherPlayers(damage.from) : room->getAlivePlayers();
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@ikjiaoman", true, true);
        if (target) {
            room->broadcastSkillInvoke(objectName());
            player->tag["IkJiaomanTarget"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const{
        ServerPlayer *target = caocao->tag["IkJiaomanTarget"].value<PlayerStar>();
        caocao->tag.remove("IkJiaomanTarget");
        if (target)
            target->obtainCard(damage.card);
    }
};

class IkHuanwei: public TriggerSkill {
public:
    IkHuanwei(): TriggerSkill("ikhuanwei$") {
        events << CardAsked;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player && player->isAlive() && player->hasLordSkill(objectName())) {
            QString pattern = data.toStringList().first();
            QString prompt = data.toStringList().at(1);
            if (pattern != "jink" || prompt.startsWith("@ikhuanwei-jink"))
                return QStringList();

            QList<ServerPlayer *> lieges = room->getLieges("hana", player);
            if (lieges.isEmpty())
                return QStringList();

            return QStringList(objectName());
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        QVariant tohelp = QVariant::fromValue((PlayerStar)player);
        foreach (ServerPlayer *liege, room->getLieges("hana", player)) {
            const Card *jink = room->askForCard(liege, "jink", "@ikhuanwei-jink:" + player->objectName(),
                                                tohelp, Card::MethodResponse, player, false, QString(), true);
            if (jink) {
                room->provide(jink);
                return true;
            }
        }

        return false;
    }
};

class IkTiandu: public TriggerSkill {
public:
    IkTiandu(): TriggerSkill("iktiandu") {
        frequency = Frequent;
        events << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *guojia, QVariant &data, ServerPlayer* &) const {
        JudgeStar judge = data.value<JudgeStar>();
        if (TriggerSkill::triggerable(guojia) && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *guojia, QVariant &data, ServerPlayer *) const {
        if (guojia->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *guojia, QVariant &data, ServerPlayer *) const {
        JudgeStar judge = data.value<JudgeStar>();
        guojia->obtainCard(judge->card);

        return false;
    }
};

class YijiViewAsSkill: public ZeroCardViewAsSkill {
public:
    YijiViewAsSkill(): ZeroCardViewAsSkill("yiji") {
        response_pattern = "@@yiji";
    }

    virtual const Card *viewAs() const{
        return new YijiCard;
    }
};

class Yiji: public MasochismSkill {
public:
    Yiji(): MasochismSkill("yiji") {
        view_as_skill = new YijiViewAsSkill;
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        for (int i = 0; i < damage.damage; i++) {
            if (target->isAlive() && room->askForSkillInvoke(target, objectName(), QVariant::fromValue(damage))) {
                room->broadcastSkillInvoke(objectName());
                target->drawCards(2, objectName());
                room->askForUseCard(target, "@@yiji", "@yiji");
            } else {
                break;
            }
        }
    }
};

class YijiObtain: public PhaseChangeSkill {
public:
    YijiObtain(): PhaseChangeSkill("#yiji") {
    }

    virtual int getPriority(TriggerEvent) const{
        return 4;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (target->getPhase() == Player::Draw && !target->getPile("yiji").isEmpty()) {
            DummyCard *dummy = new DummyCard(target->getPile("yiji"));
            CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, target->objectName(), "yiji", QString());
            room->obtainCard(target, dummy, reason, false);
            delete dummy;
        }
        return false;
    }
};

class IkTiansuo: public TriggerSkill {
public:
    IkTiansuo(): TriggerSkill("iktiansuo") {
        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return TriggerSkill::triggerable(player)
            && !player->isKongcheng();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStar judge = data.value<JudgeStar>();

        QStringList prompt_list;
        prompt_list << "@iktiansuo-card" << judge->who->objectName()
                    << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        bool forced = false;
        if (player->getMark("JilveEvent") == int(AskForRetrial))
            forced = true;
        const Card *card = room->askForCard(player, forced ? ".!" : "." , prompt, data, Card::MethodResponse, judge->who, true);
        if (forced && card == NULL)
            card = player->getRandomHandCard();
        if (card) {
            player->tag["IkTiansuoCard"] = QVariant::fromValue(card);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardStar card = player->tag["IkTiansuoCard"].value<CardStar>();
        player->tag.remove("IkTiansuoCard");
        if (card) {
            if (player->hasInnateSkill("iktiansuo") || !player->hasSkill("jilve"))
                room->broadcastSkillInvoke(objectName());
            else
                room->broadcastSkillInvoke("jilve", 1);
            room->retrial(card, player, data.value<JudgeStar>(), objectName());
        }

        return false;
    }
};

class IkHuanji: public MasochismSkill {
public:
    IkHuanji(): MasochismSkill("ikhuanji") {
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (!TriggerSkill::triggerable(player)) return skill;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && !damage.from->isAllNude())
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.from))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const{
        int card_id = simayi->getRoom()->askForCardChosen(simayi, damage.from, "hej", "ikhuanji");
        simayi->obtainCard(Sanguosha->getCard(card_id), false);
    }
};

class IkAoli: public TriggerSkill {
public:
    IkAoli(): TriggerSkill("ikaoli") {
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *xiahou, QVariant &data, ServerPlayer* &) const{
        QStringList skill;
        if (!TriggerSkill::triggerable(xiahou)) return skill;
        DamageStruct damage = data.value<DamageStruct>();
        for (int i = 0; i < damage.damage; i++)
            skill << objectName();
        return skill;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *xiahou, QVariant &data, ServerPlayer *) const{
        if (xiahou->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *xiahou, QVariant &data, ServerPlayer *) const{
        JudgeStruct judge;
        judge.pattern = ".";
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = xiahou;

        room->judge(judge);
        DamageStruct damage = data.value<DamageStruct>();
        
        if (!damage.from || damage.from->isDead()) return false;
        Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
        switch (suit) {
        case Card::Heart:
        case Card::Diamond: {
                room->damage(DamageStruct(objectName(), xiahou, damage.from));
                break;
            }
        case Card::Club:
        case Card::Spade: {
                if (xiahou->canDiscard(damage.from, "he")) {
                    int id = room->askForCardChosen(xiahou, damage.from, "he", objectName(), false, Card::MethodDiscard);
                    room->throwCard(id, damage.from, xiahou);
                }
                break;
            }
        default:
                break;
        }

        return false;
    }
};

class IkAoliRecord: public TriggerSkill {
public:
    IkAoliRecord(): TriggerSkill("#ikaoli") {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *xiahou, QVariant &data, ServerPlayer* &) const{
        JudgeStar judge = data.value<JudgeStar>();
        if (judge->reason != "ikaoli") return QStringList();
        judge->pattern = QString::number(int(judge->card->getSuit()));
        return QStringList();
    }
};

class IkQingjian: public TriggerSkill {
public:
    IkQingjian(): TriggerSkill("ikqingjian") {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw
            && move.to == player && move.to_place == Player::PlaceHand) {
            QList<int> ids;
            foreach (int id, move.card_ids)
                if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                    return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw
            && move.to == player && move.to_place == Player::PlaceHand) {
            QList<int> ids;
            foreach (int id, move.card_ids) {
                if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                    ids << id;
            }
            if (room->askForYiji(player, ids, objectName(), false, true, true, -1,
                                 QList<ServerPlayer *>(), CardMoveReason(), "@ikqingjian-distribute", true))
                return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                ids << id;
        }
        if (ids.isEmpty())
            return false;
        while (room->askForYiji(player, ids, objectName(), false, false, true, -1,
                                QList<ServerPlayer *>(), CardMoveReason(), "@ikqingjian-distribute", false)) {
            room->notifySkillInvoked(player, objectName());
            if (player->isDead()) return false;
        }

        return false;
    }
};

class IkLianbaoViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkLianbaoViewAsSkill(): ZeroCardViewAsSkill("iklianbao") {
        response_pattern = "@@iklianbao";
    }

    virtual const Card *viewAs() const{
        return new IkLianbaoCard;
    }
};

class IkLianbao: public DrawCardsSkill {
public:
    IkLianbao(): DrawCardsSkill("iklianbao") {
        view_as_skill = new IkLianbaoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(zhangliao)) return QStringList();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->getHandcardNum() >= zhangliao->getHandcardNum())
                targets << p;
        int num = qMin(targets.length(), data.toInt());
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            p->setFlags("-IkLianbaoTarget");
        if (num > 0)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &data, ServerPlayer *) const{
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->getHandcardNum() >= zhangliao->getHandcardNum())
                targets << p;
        int num = qMin(targets.length(), data.toInt());
        room->setPlayerMark(zhangliao, "iklianbao", num);
        if (room->askForUseCard(zhangliao, "@@iklianbao", "@iklianbao-card:::" + QString::number(num))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        } else
            room->setPlayerMark(zhangliao, "iklianbao", 0);
        return false;
    }

    virtual int getDrawNum(ServerPlayer *zhangliao, int n) const{
        Room *room = zhangliao->getRoom();
        int count = 0;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->hasFlag("IkLianbaoTarget")) count++;
        
        return n - count;
    }
};

class IkLianbaoAct: public TriggerSkill {
public:
    IkLianbaoAct(): TriggerSkill("#iklianbao") {
        events << AfterDrawNCards;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &, ServerPlayer* &) const{
        if (zhangliao->getMark("iklianbao") == 0) return QStringList();
        return QStringList(objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhangliao, QVariant &, ServerPlayer *) const{
        room->setPlayerMark(zhangliao, "iklianbao", 0);

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(zhangliao)) {
            if (p->hasFlag("IkLianbaoTarget")) {
                p->setFlags("-IkLianbaoTarget");
                targets << p;
            }
        }
        foreach (ServerPlayer *p, targets) {
            if (!zhangliao->isAlive())
                break;
            if (p->isAlive() && !p->isKongcheng()) {
                int card_id = room->askForCardChosen(zhangliao, p, "h", "iklianbao");

                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, zhangliao->objectName());
                room->obtainCard(zhangliao, Sanguosha->getCard(card_id), reason, false);
            }
        }
        return false;
    }
};

class IkLuoyi: public TriggerSkill {
public:
    IkLuoyi(): TriggerSkill("ikluoyi") {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (TriggerSkill::triggerable(player) && change.to == Player::Draw && !player->isSkipped(Player::Draw))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        player->skip(Player::Draw, true);
        room->setPlayerMark(player, "ikluoyi", 1);

        QList<int> ids = room->getNCards(3, false);
        CardsMoveStruct move(ids, player, Player::PlaceTable,
                             CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "ikluoyi", QString()));
        room->moveCardsAtomic(move, true);

        room->getThread()->delay();
        room->getThread()->delay();

        QList<int> card_to_throw;
        QList<int> card_to_gotback;
        for (int i = 0; i < 3; i++) {
            const Card *card = Sanguosha->getCard(ids[i]);
            if (card->getTypeId() == Card::TypeBasic || card->isKindOf("Weapon") || card->isKindOf("Duel"))
                card_to_gotback << ids[i];
            else
                card_to_throw << ids[i];
        }
        if (!card_to_throw.isEmpty()) {
            DummyCard *dummy = new DummyCard(card_to_throw);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "ikluoyi", QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
        }
        if (!card_to_gotback.isEmpty()) {
            DummyCard *dummy = new DummyCard(card_to_gotback);
            room->obtainCard(player, dummy);
            delete dummy;
        }
        return false;
    }
};

class IkLuoyiBuff: public TriggerSkill {
public:
    IkLuoyiBuff(): TriggerSkill("#ikluoyi") {
        events << DamageCaused;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data, ServerPlayer* &) const{
        if (!xuchu || xuchu->getMark("ikluoyi") == 0 || xuchu->isDead()) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer) return QStringList();
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel")))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#IkLuoyiBuff";
        log.from = xuchu;
        log.to << damage.to;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(++damage.damage);
        room->sendLog(log);

        data = QVariant::fromValue(damage);

        return false;
    }
};

class IkLuoyiClear: public TriggerSkill {
public:
    IkLuoyiClear(): TriggerSkill("#ikluoyi-clear") {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL && player->isAlive() && player->getPhase() == Player::RoundStart && player->getMark("ikluoyi") > 0)
            room->setPlayerMark(player, "ikluoyi", 0);
        return QStringList();
    }
};

class IkMengyang: public TriggerSkill {
public:
    IkMengyang(): TriggerSkill("ikmengyang") {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const{
        if (player->getPhase() == Player::Start){
            if (TriggerSkill::triggerable(player))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        if (player->askForSkillInvoke(objectName())){
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const{
        JudgeStruct judge;
        forever {
            judge.pattern = ".|black";
            judge.good = true;
            judge.reason = objectName();
            judge.play_animation = false;
            judge.who = player;
            judge.time_consuming = true;

            room->judge(judge);
            if ((judge.isGood() && !player->askForSkillInvoke(objectName())) || judge.isBad())
                break;
        }

        return false;
    }
};

class IkMengyangMove: public TriggerSkill {
public:
    IkMengyangMove(): TriggerSkill("#ikmengyang-move") {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player != NULL) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == "ikmengyang") {
                if (judge->isGood()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        JudgeStar judge = data.value<JudgeStar>();
        room->moveCardTo(judge->card, judge->who, Player::PlaceHand, true);

        return false;
    }
};

class IkZhongyan: public OneCardViewAsSkill {
public:
    IkZhongyan(): OneCardViewAsSkill("ikzhongyan") {
        filter_pattern = ".|black|.|hand";
        response_pattern = "jink";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(originalCard->getId());
        return jink;
    }
};

class IkShenai: public ViewAsSkill {
public:
    IkShenai(): ViewAsSkill("ikshenai") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() >= 3)
           return false;
        else
            return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkShenaiCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        IkShenaiCard *ikshenai_card = new IkShenaiCard;
        ikshenai_card->addSubcards(cards);
        return ikshenai_card;
    }
};

IkXinqiViewAsSkill::IkXinqiViewAsSkill(): ZeroCardViewAsSkill("ikxinqi$") {
}

bool IkXinqiViewAsSkill::isEnabledAtPlay(const Player *player) const{
    return hasKazeGenerals(player) && !player->hasFlag("Global_IkXinqiFailed") && Slash::IsAvailable(player);
}

bool IkXinqiViewAsSkill::isEnabledAtResponse(const Player *player, const QString &pattern) const{
    return hasKazeGenerals(player)
           && (pattern == "slash" || pattern == "@ikxinqi")
           && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
           && !player->hasFlag("Global_IkXinqiFailed");
}

const Card *IkXinqiViewAsSkill::viewAs() const{
    return new IkXinqiCard;
}

bool IkXinqiViewAsSkill::hasKazeGenerals(const Player *player) {
    foreach (const Player *p, player->getAliveSiblings())
        if (p->getKingdom() == "kaze")
            return true;
    return false;
}

class IkXinqi: public TriggerSkill {
public:
    IkXinqi(): TriggerSkill("ikxinqi$") {
        events << CardAsked;
        view_as_skill = new IkXinqiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (player && player->isAlive() && player->hasLordSkill("ikxinqi")) {
            QString pattern = data.toStringList().first();
            QString prompt = data.toStringList().at(1);
            if (pattern != "slash" || prompt.startsWith("@ikxinqi-slash"))
                return QStringList();
            QList<ServerPlayer *> lieges = room->getLieges("kaze", player);
            if (lieges.isEmpty())
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *liubei, QVariant &data, ServerPlayer *) const{
        if (liubei->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName(), getEffectIndex(liubei, NULL));
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liubei, QVariant &data, ServerPlayer *) const{
        QList<ServerPlayer *> lieges = room->getLieges("kaze", liubei);
        foreach (ServerPlayer *liege, lieges) {
            const Card *slash = room->askForCard(liege, "slash", "@ikxinqi-slash:" + liubei->objectName(),
                                                 QVariant(), Card::MethodResponse, liubei, false, QString(), true);
            if (slash) {
                room->provide(slash);
                return true;
            }
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int r = 1 + qrand() % 2;
        if (!player->hasInnateSkill("ikxinqi") && player->hasSkill("ruoyu"))
            r += 2;
        return r;
    }
};

class IkChilian: public OneCardViewAsSkill {
public:
    IkChilian(): OneCardViewAsSkill("ikchilian") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const Card *card) const{
        if (!card->isRed())
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(card->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard->getId());
        slash->setSkillName(objectName());
        return slash;
    }
};

class IkZhenhong: public TriggerSkill {
public:
    IkZhenhong(): TriggerSkill("ikzhenhong") {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && use.card->isKindOf("Slash") && use.card->getSuit() == Card::Diamond)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());

        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to.toSet())
            p->addQinggangTag(use.card);
        return false;
    }
};

class IkZhenhongTargetMod: public TargetModSkill {
public:
    IkZhenhongTargetMod(): TargetModSkill("#ikzhenhong-target") {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("ikzhenhong") && card->getSuit() == Card::Heart)
            return 1000;
        else
            return 0;
    }
};

class IkLipao: public TargetModSkill {
public:
    IkLipao(): TargetModSkill("iklipao") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class IkJiukuang: public OneCardViewAsSkill {
public:
    IkJiukuang(): OneCardViewAsSkill("ikjiukuang") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player) && player->getPhase() == Player::Play;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.contains("analeptic") && player->getPhase() == Player::Play;
    }

    virtual bool viewFilter(const Card *card) const{
        if (!(card->isNDTrick() && card->isBlack()) && !card->isKindOf("Weapon"))
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Analeptic *anal = new Analeptic(Card::SuitToBeDecided, -1);
            anal->addSubcard(card->getEffectiveId());
            anal->deleteLater();
            return anal->isAvailable(Self);
        }
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Analeptic *anal = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        anal->addSubcard(originalCard->getId());
        anal->setSkillName(objectName());
        return anal;
    }
};

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

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int index = qrand() % 2 + 1;
        if (Player::isNostalGeneral(player, "zhaoyun"))
            index += 2;
        return index;
    }
};

class Yajiao: public TriggerSkill {
public:
    Yajiao(): TriggerSkill("yajiao") {
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive) return false;
        CardStar cardstar = NULL;
        bool isHandcard = false;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            cardstar = use.card;
            isHandcard = use.m_isHandcard;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            cardstar = resp.m_card;
            isHandcard = resp.m_isHandcard;
        }
        if (isHandcard && room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, player, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "yajiao", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            room->fillAG(ids, player);
            bool dealt = false;
            if (card->getTypeId() == cardstar->getTypeId()) {
                player->setMark("yajiao", id); // For AI
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                                                                QString("@yajiao-give:::%1:%2\\%3").arg(card->objectName())
                                                                                                   .arg(card->getSuitString() + "_char")
                                                                                                   .arg(card->getNumberString()),
                                                                true);
                if (target) {
                    room->clearAG(player);
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_DRAW, target->objectName(), "yajiao", QString());
                    room->obtainCard(target, card, reason);
                }
            } else {
                QVariant carddata = QVariant::fromValue((CardStar)card);
                if (room->askForChoice(player, objectName(), "throw+cancel", carddata) == "throw") {
                    room->clearAG(player);
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "yajiao", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
            if (!dealt) {
                room->clearAG(player);
                room->returnToTopDrawPile(ids);
            }
        }
        return false;
    }
};

class NonCompulsoryInvalidity: public InvaliditySkill {
public:
    NonCompulsoryInvalidity(): InvaliditySkill("#non-compulsory-invalidity") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return player->getMark("@skill_invalidity") == 0 || skill->getFrequency() == Skill::Compulsory;
    }
};

class IkYufeng: public TriggerSkill {
public:
    IkYufeng(): TriggerSkill("ikyufeng") {
        events << TargetSpecified;
    }
    
    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        QList<ServerPlayer *> tos;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive()) break;
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());
                if (!tos.contains(p)) {
                    p->addMark("ikyufeng");
                    room->addPlayerMark(p, "@skill_invalidity");
                    tos << p;

                    foreach (ServerPlayer *pl, room->getAllPlayers())
                        room->filterCards(pl, pl->getCards("he"), true);
                    Json::Value args;
                    args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                }

                JudgeStruct judge;
                judge.pattern = ".";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;
                judge.play_animation = false;

                room->judge(judge);

                if (p->isAlive() && !p->canDiscard(p, "he")
                    || !room->askForCard(p, ".|" + judge.pattern, "@ikyufeng-discard:::" + judge.pattern, data, Card::MethodDiscard)) {
                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);
                    jink_list.replace(index, QVariant(0));
                }
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class IkYufengClear: public TriggerSkill {
public:
    IkYufengClear(): TriggerSkill("#ikyufeng-clear") {
        events << EventPhaseChanging << Death << FinishJudge;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == FinishJudge) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == "ikyufeng")
                judge->pattern = judge->card->getSuitString();
            return QStringList();
        } else if (triggerEvent == EventPhaseChanging) {
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
            if (player->getMark("ikyufeng") == 0) continue;
            room->removePlayerMark(player, "@skill_invalidity", player->getMark("ikyufeng"));
            player->setMark("ikyufeng", 0);

            foreach (ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), false);
            Json::Value args;
            args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
        return QStringList();
    }
};

class IkYuxi: public PhaseChangeSkill {
public:
    IkYuxi(): PhaseChangeSkill("ikyuxi") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            int index = qrand() % 2 + 1;
            if (objectName() == "ikyuxi" && !player->hasInnateSkill(objectName()) && player->hasSkill("zhiji"))
                index += 2;
            room->broadcastSkillInvoke(objectName(), index);
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *zhuge) const {
        Room *room = zhuge->getRoom();
        QList<int> guanxing = room->getNCards(getIkYuxiNum(room));

        LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = zhuge;
        log.card_str = IntList2StringList(guanxing).join("+");
        room->sendLog(log, zhuge);

        room->askForGuanxing(zhuge, guanxing);

        return false;
    }

    virtual int getIkYuxiNum(Room *room) const{
        return qMin(5, room->alivePlayerCount());
    }
};

class IkJingyou: public ProhibitSkill {
public:
    IkJingyou(): ProhibitSkill("ikjingyou") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (card->isKindOf("Slash") || card->isKindOf("Duel")) && to->isKongcheng();
    }
};

class Jizhi: public TriggerSkill {
public:
    Jizhi(): TriggerSkill("jizhi") {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->getTypeId() == Card::TypeTrick
            && (yueying->getMark("JilveEvent") > 0 || room->askForSkillInvoke(yueying, objectName()))) {
            if (yueying->getMark("JilveEvent") > 0)
                room->broadcastSkillInvoke("jilve", 5);
            else
                room->broadcastSkillInvoke(objectName());

            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, yueying, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, yueying->objectName(), "jizhi", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            if (!card->isKindOf("BasicCard")) {
                CardMoveReason reason(CardMoveReason::S_REASON_DRAW, yueying->objectName(), "jizhi", QString());
                room->obtainCard(yueying, card, reason);
            } else {
                const Card *card_ex = NULL;
                if (!yueying->isKongcheng())
                    card_ex = room->askForCard(yueying, ".", "@jizhi-exchange:::" + card->objectName(),
                                               QVariant::fromValue((CardStar)card), Card::MethodNone);
                if (card_ex) {
                    CardMoveReason reason1(CardMoveReason::S_REASON_PUT, yueying->objectName(), "jizhi", QString());
                    CardMoveReason reason2(CardMoveReason::S_REASON_DRAW, yueying->objectName(), "jizhi", QString());
                    CardsMoveStruct move1(card_ex->getEffectiveId(), yueying, NULL, Player::PlaceUnknown, Player::DrawPile, reason1);
                    CardsMoveStruct move2(ids, yueying, yueying, Player::PlaceUnknown, Player::PlaceHand, reason2);

                    QList<CardsMoveStruct> moves;
                    moves.append(move1);
                    moves.append(move2);
                    room->moveCardsAtomic(moves, false);
                } else {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, yueying->objectName(), "jizhi", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
        }

        return false;
    }
};

class Qicai: public TargetModSkill {
public:
    Qicai(): TargetModSkill("qicai") {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class Zhuhai: public TriggerSkill {
public:
    Zhuhai(): TriggerSkill("zhuhai") {
        events << EventPhaseStart << ChoiceMade;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            ServerPlayer *xushu = room->findPlayerBySkillName(objectName());
            if (xushu && xushu != player && xushu->canSlash(player, false)
                && player->hasFlag("ZhuhaiDamage")) {
                xushu->setFlags("ZhuhaiSlash");
                QString prompt = QString("@zhuhai-slash:%1:%2").arg(xushu->objectName()).arg(player->objectName());
                if (!room->askForUseSlashTo(xushu, player, prompt, false))
                    xushu->setFlags("-ZhuhaiSlash");
            }
        } else if (triggerEvent == ChoiceMade && player->hasFlag("ZhuhaiSlash") && data.canConvert<CardUseStruct>()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            player->setFlags("-ZhuhaiSlash");
        }
        return false;
    }
};

class ZhuhaiRecord: public TriggerSkill {
public:
    ZhuhaiRecord(): TriggerSkill("#zhuhai-record") {
        events << PreDamageDone;
        //global = true;
    }

    virtual int getPriority(TriggerEvent) const{
        return 4;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->getPhase() != Player::NotActive && !damage.from->hasFlag("ZhuhaiDamage"))
            damage.from->setFlags("ZhuhaiDamage");
        return false;
    }
};

class Qianxin: public TriggerSkill {
public:
    Qianxin(): TriggerSkill("qianxin") {
        events << Damage;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && TriggerSkill::triggerable(target)
               && target->getMark("qianxin") == 0
               && target->isWounded();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        room->doLightbox("$QianxinAnimate");

        LogMessage log;
        log.type = "#QianxinWake";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);

        room->setPlayerMark(player, "qianxin", 1);
        if (room->changeMaxHpForAwakenSkill(player) && player->getMark("qianxin") == 1)
            room->acquireSkill(player, "jianyan");

        return false;
    }
};

class Jianyan: public ZeroCardViewAsSkill {
public:
    Jianyan(): ZeroCardViewAsSkill("jianyan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("JianyanCard");
    }

    virtual const Card *viewAs() const{
        return new JianyanCard;
    }
};

class IkZhiheng: public ViewAsSkill {
public:
    IkZhiheng(): ViewAsSkill("ikzhiheng") {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() >= Self->getMaxHp()) return false;
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
            return NULL;

        IkZhihengCard *ikzhiheng_card = new IkZhihengCard;
        ikzhiheng_card->addSubcards(cards);
        ikzhiheng_card->setSkillName(objectName());
        return ikzhiheng_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("IkZhihengCard");
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@ikzhiheng";
    }
};

class IkJiyuan: public TriggerSkill {
public:
    IkJiyuan(): TriggerSkill("ikjiyuan$") {
        events << TargetSpecified << PreHpRecover;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Peach") && player->getKingdom() == "yuki") {
                foreach (ServerPlayer *p, use.to)
                    if (p->hasLordSkill("ikjiyuan"))
                        room->setCardFlag(use.card, "ikjiyuan");
            }
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct rec = data.value<RecoverStruct>();
            if (rec.card && rec.card->hasFlag("ikjiyuan"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *sunquan, QVariant &data, ServerPlayer *) const{
        RecoverStruct rec = data.value<RecoverStruct>();

        room->notifySkillInvoked(sunquan, "ikjiyuan");
        room->broadcastSkillInvoke("ikjiyuan");

        LogMessage log;
        log.type = "#IkJiyuanExtraRecover";
        log.from = sunquan;
        log.to << rec.who;
        log.arg = objectName();
        room->sendLog(log);

        rec.recover++;
        data = QVariant::fromValue(rec);

        return false;
    }
};

class IkChenhong: public DrawCardsSkill {
public:
    IkChenhong(): DrawCardsSkill("ikchenhong") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *zhouyu, int n) const{
        Room *room = zhouyu->getRoom();
        room->broadcastSkillInvoke(objectName());

        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = zhouyu;
        log.arg = objectName();
        room->sendLog(log);

        return n + 1;
    }
};

class IkChenhongMaxCards: public MaxCardsSkill {
public:
    IkChenhongMaxCards(): MaxCardsSkill("#ikchenhong") {
    }

    virtual int getFixed(const Player *target) const{
        if (target->hasSkill("ikchenhong"))
            return target->getMaxHp();
        else
            return -1;
    }
};

class IkGuideng: public OneCardViewAsSkill {
public:
    IkGuideng(): OneCardViewAsSkill("ikguideng") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("IkGuidengCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkGuidengCard *card = new IkGuidengCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class IkKuipo: public OneCardViewAsSkill {
public:
    IkKuipo(): OneCardViewAsSkill("ikkuipo") {
        filter_pattern = ".|black";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Dismantlement *dismantlement = new Dismantlement(originalCard->getSuit(), originalCard->getNumber());
        dismantlement->addSubcard(originalCard->getId());
        dismantlement->setSkillName(objectName());
        return dismantlement;
    }
};

class IkGuisiViewAsSkill: public ZeroCardViewAsSkill {
public:
    IkGuisiViewAsSkill():ZeroCardViewAsSkill("ikguisi") {
        response_pattern = "@@ikguisi";
    }

    virtual const Card *viewAs() const{
        return new IkGuisiCard;
    }
};

class IkGuisi: public TriggerSkill {
public:
    IkGuisi(): TriggerSkill("ikguisi") {
        events << TargetSpecifying;
        view_as_skill = new IkGuisiViewAsSkill;
        frequency = Limited;
        limit_mark = "@guisi";
    }

    virtual QMap<ServerPlayer *,QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        QMap<ServerPlayer *,QStringList> skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() <= 1 || !use.card->isNDTrick())
            return skill_list;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            if (p->getMark("@guisi") > 0)
                skill_list.insert(p, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ganning) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList target_list;
        foreach (ServerPlayer *p, use.to)
            target_list << p->objectName();
        room->setPlayerProperty(ganning, "ikguisi_targets", target_list.join("+"));
        ganning->tag["ikguisi"] = data;
        room->askForUseCard(ganning, "@@ikguisi", "@ikguisi-card");
        data = ganning->tag["ikguisi"];

        return false;
    }
};

class IkBiju: public TriggerSkill {
public:
    IkBiju(): TriggerSkill("ikbiju") {
        events << EventPhaseChanging;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &data, ServerPlayer* &) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (TriggerSkill::triggerable(lvmeng) && change.to == Player::Discard && !lvmeng->hasFlag("IkBijuSlashInPlayPhase"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &, ServerPlayer *) const{
        if (lvmeng->askForSkillInvoke(objectName())) {
            if (lvmeng->getHandcardNum() > lvmeng->getMaxCards())
                room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *lvmeng, QVariant &, ServerPlayer *) const{
        lvmeng->skip(Player::Discard);
        return false;
    }
};

class IkBijuRecord: public TriggerSkill {
public:
    IkBijuRecord(): TriggerSkill("#ikbiju-record") {
        events << PreCardUsed << CardResponded;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &data, ServerPlayer* &) const{
        if (lvmeng->getPhase() == Player::Play) {
            CardStar card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;
            if (card->isKindOf("Slash"))
                lvmeng->setFlags("IkBijuSlashInPlayPhase");
        }
        return QStringList();
    }
};

class IkPojian: public TriggerSkill {
public:
    IkPojian(): TriggerSkill("ikpojian") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && target->getMark("ikpojian") >= 5
            && target->getMark("@pojian") == 0;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#IkPojianWake";
        log.from = player;
        log.arg = QString::number(player->getMark("ikpojian"));
        log.arg2 = objectName();
        room->sendLog(log);
        room->addPlayerMark(player, "@pojian");

        room->recover(player, RecoverStruct(player));
        room->changeMaxHpForAwakenSkill(player);

        room->acquireSkill(player, "ikqinghua");
        return false;
    }
};

class IkPojianRecord: public TriggerSkill {
public:
    IkPojianRecord(): TriggerSkill("#ikpojian-record") {
        events << PreCardUsed << CardResponded << EventPhaseStart;
        frequency = Compulsory;
        global = true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *lvmeng, QVariant &data, ServerPlayer* &) const{
        if (triggerEvent == EventPhaseStart) {
            if (lvmeng->getPhase() == Player::RoundStart)
                lvmeng->setMark("ikpojian", 0);
        } else {
            CardStar card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (card && !card->isKindOf("EquipCard"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *, ServerPlayer *lvmeng, QVariant &data, ServerPlayer *) const{
        lvmeng->addMark("ikpojian");
        return false;
    }
};

class IkQinghua: public ZeroCardViewAsSkill {
public:
    IkQinghua(): ZeroCardViewAsSkill("ikqinghua") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkQinghuaCard");
    }

    virtual const Card *viewAs() const{
        return new IkQinghuaCard;
    }
};

class Kurou: public OneCardViewAsSkill {
public:
    Kurou(): OneCardViewAsSkill("kurou") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("KurouCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        KurouCard *card = new KurouCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Zhaxiang: public TriggerSkill {
public:
    Zhaxiang(): TriggerSkill("zhaxiang") {
        events << HpLost << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == HpLost && TriggerSkill::triggerable(player)) {
            int lose = data.toInt();

            room->notifySkillInvoked(player, objectName());
            room->broadcastSkillInvoke(objectName());
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            for (int i = 0; i < lose; i++) {
                player->drawCards(3, objectName());
                room->addPlayerMark(player, objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, objectName(), 0);
        }
        return false;
    }
};

class ZhaxiangRedSlash: public TriggerSkill {
public:
    ZhaxiangRedSlash(): TriggerSkill("#zhaxiang") {
        events << TargetSpecified;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->isAlive() && target->getMark("zhaxiang") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || !use.card->isRed())
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            LogMessage log;
            log.type = "#NoJink";
            log.from = p;
            room->sendLog(log);
            jink_list.replace(index, QVariant(0));
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class ZhaxiangTargetMod: public TargetModSkill {
public:
    ZhaxiangTargetMod(): TargetModSkill("#zhaxiang-target") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        return from->getMark("zhaxiang");
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (card->isRed() && from->getMark("zhaxiang") > 0)
            return 1000;
        else
            return 0;
    }
};

class IkWanmeiViewAsSkill: public OneCardViewAsSkill {
public:
    IkWanmeiViewAsSkill(): OneCardViewAsSkill("ikwanmei") {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkWanmeiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkWanmeiCard *card = new IkWanmeiCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class IkWanmei: public TriggerSkill {
public:
    IkWanmei(): TriggerSkill("ikwanmei") {
        events << CardFinished;
        view_as_skill = new IkWanmeiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Indulgence") && use.card->getSkillName() == objectName())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        player->drawCards(1, objectName());
        return false;
    }
};

class IkXuanhuoViewAsSkill: public OneCardViewAsSkill {
public:
    IkXuanhuoViewAsSkill(): OneCardViewAsSkill("ikxuanhuo") {
        filter_pattern = ".!";
        response_pattern = "@@ikxuanhuo";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IkXuanhuoCard *ikxuanhuo_card = new IkXuanhuoCard;
        ikxuanhuo_card->addSubcard(originalCard);
        return ikxuanhuo_card;
    }
};

class IkXuanhuo: public TriggerSkill {
public:
    IkXuanhuo(): TriggerSkill("ikxuanhuo") {
        events << TargetConfirming;
        view_as_skill = new IkXuanhuoViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(daqiao)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isKindOf("Slash") && use.to.contains(daqiao) && daqiao->canDiscard(daqiao, "he")) {
            QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
            players.removeOne(use.from);

            foreach (ServerPlayer *p, players) {
                if (use.from->canSlash(p, use.card, false) && daqiao->inMyAttackRange(p))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "@ikxuanhuo:" + use.from->objectName();
        room->setPlayerFlag(use.from, "IkXuanhuoSlashSource");
        // a temp nasty trick
        daqiao->tag["ikxuanhuo-card"] = QVariant::fromValue((CardStar)use.card); // for the server (AI)
        room->setPlayerProperty(daqiao, "ikxuanhuo", use.card->toString()); // for the client (UI)
        if (room->askForUseCard(daqiao, "@@ikxuanhuo", prompt, -1, Card::MethodDiscard))
            return true;
        else {
            daqiao->tag.remove("ikxuanhuo-card");
            room->setPlayerProperty(daqiao, "ikxuanhuo", QString());
            room->setPlayerFlag(use.from, "-IkXuanhuoSlashSource");
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *daqiao, QVariant &data, ServerPlayer *) const{
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
        players.removeOne(use.from);

        daqiao->tag.remove("ikxuanhuo-card");
        room->setPlayerProperty(daqiao, "ikxuanhuo", QString());
        room->setPlayerFlag(use.from, "-IkXuanhuoSlashSource");
        foreach (ServerPlayer *p, players) {
            if (p->hasFlag("IkXuanhuoTarget")) {
                p->setFlags("-IkXuanhuoTarget");
                if (!use.from->canSlash(p, false))
                    return false;
                use.to.removeOne(daqiao);
                use.to.append(p);
                room->sortByActionOrder(use.to);
                data = QVariant::fromValue(use);
                room->getThread()->trigger(TargetConfirming, room, p, data);
                return false;
            }
        }

        return false;
    }
};

class Qianxun: public TriggerSkill {
public:
    Qianxun(): TriggerSkill("qianxun") {
        events << TrickEffect << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TrickEffect && TriggerSkill::triggerable(player)) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.multiple && effect.card->getTypeId() == Card::TypeTrick
                && (effect.card->isKindOf("DelayedTrick") || effect.from != player)
                && room->askForSkillInvoke(player, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                player->tag["QianxunEffectData"] = data;

                CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, player->objectName(), objectName(), QString());
                QList<int> handcards = player->handCards();
                QList<ServerPlayer *> open;
                open << player;
                player->addToPile("qianxun", handcards, false, open, reason);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getPile("qianxun").length() > 0) {
                        DummyCard *dummy = new DummyCard(p->getPile("qianxun"));
                        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "qianxun", QString());
                        room->obtainCard(p, dummy, reason, false);
                        delete dummy;
                    }
                }
            }
        }
        return false;
    }
};

class LianyingViewAsSkill: public ZeroCardViewAsSkill {
public:
    LianyingViewAsSkill(): ZeroCardViewAsSkill("lianying") {
        response_pattern = "@@lianying";
    }

    virtual const Card *viewAs() const{
        return new LianyingCard;
    }
};

class Lianying: public TriggerSkill {
public:
    Lianying(): TriggerSkill("lianying") {
        events << CardsMoveOneTime;
        view_as_skill = new LianyingViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *luxun, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == luxun && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            luxun->tag["LianyingMoveData"] = data;
            int count = 0;
            for (int i = 0; i < move.from_places.length(); i++) {
                if (move.from_places[i] == Player::PlaceHand) count++;
            }
            room->setPlayerMark(luxun, "lianying", count);
            room->askForUseCard(luxun, "@@lianying", "@lianying-card:::" + QString::number(count));
        }
        return false;
    }
};

class IkYulu: public ViewAsSkill {
public:
    IkYulu(): ViewAsSkill("ikyulu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() >= 2 && !player->hasUsed("IkYuluCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() > 1 || Self->isJilei(to_select))
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 2)
            return NULL;

        IkYuluCard *ikyulu_card = new IkYuluCard();
        ikyulu_card->addSubcards(cards);
        return ikyulu_card;
    }
};

class IkCuimeng: public TriggerSkill {
public:
    IkCuimeng(): TriggerSkill("ikcuimeng") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.from_places.contains(Player::PlaceEquip))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *sunshangxiang, QVariant &, ServerPlayer *) const{
        if (sunshangxiang->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *sunshangxiang, QVariant &, ServerPlayer *) const{
        sunshangxiang->drawCards(2, objectName());
        return false;
    }
};

class IkWushuang: public TriggerSkill {
public:
    IkWushuang(): TriggerSkill("ikwushuang") {
        events << TargetSpecified << CardFinished;
        frequency = Compulsory;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Duel")) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->tag.remove("IkWushuang_" + use.card->toString());
            }
        } else if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && TriggerSkill::triggerable(player))
                skill_list.insert(player, QStringList(objectName()));
            else if (use.card->isKindOf("Duel")) {
                if (TriggerSkill::triggerable(player))
                    skill_list.insert(player, QStringList(objectName()));
                foreach (ServerPlayer *p, use.to.toSet())
                    if (TriggerSkill::triggerable(p))
                        skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = ask_who;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(ask_who, objectName());
            room->broadcastSkillInvoke(objectName());

            QVariantList jink_list = ask_who->tag["Jink_" + use.card->toString()].toList();
            for (int i = 0; i < use.to.length(); i++) {
                if (jink_list.at(i).toInt() == 1)
                    jink_list.replace(i, QVariant(2));
            }
            ask_who->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        } else if (use.card->isKindOf("Duel")) {
            if (use.from == ask_who) {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = ask_who;
                log.arg = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(ask_who, objectName());
                room->broadcastSkillInvoke(objectName());

                QStringList ikwushuang_tag;
                foreach (ServerPlayer *to, use.to)
                    ikwushuang_tag << to->objectName();
                ask_who->tag["IkWushuang_" + use.card->toString()] = ikwushuang_tag;
            } else {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = use.from;
                log.arg = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(use.from, objectName());
                room->broadcastSkillInvoke(objectName());

                ask_who->tag["IkWushuang_" + use.card->toString()] = QStringList(use.from->objectName());
            }
        }

        return false;
    }
};

class IkWudi: public ViewAsSkill {
public:
    IkWudi(): ViewAsSkill("ikwudi") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("IkWudiCard");
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
            Duel *duel = new Duel(Card::SuitToBeDecided, 0);
            duel->addSubcards(cards);
            duel->setSkillName(objectName());
            duel->deleteLater();
            if (duel->isAvailable(Self)) {
                IkWudiCard *card = new IkWudiCard;
                card->addSubcards(cards);
                return card;
            }
        }
        return NULL;
    }
};

class Lijian: public OneCardViewAsSkill {
public:
    Lijian(): OneCardViewAsSkill("lijian") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getAliveSiblings().length() > 1
               && player->canDiscard(player, "he") && !player->hasUsed("LijianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LijianCard *lijian_card = new LijianCard;
        lijian_card->addSubcard(originalCard->getId());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

class IkZhuoyue: public PhaseChangeSkill {
public:
    IkZhuoyue(): PhaseChangeSkill("ikzhuoyue") {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Finish;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *diaochan) const{
        diaochan->drawCards(1, objectName());
        return false;
    }
};

class Chuli: public OneCardViewAsSkill {
public:
    Chuli(): OneCardViewAsSkill("chuli") {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("ChuliCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ChuliCard *chuli_card = new ChuliCard;
        chuli_card->addSubcard(originalCard->getId());
        return chuli_card;
    }
};

class IkHuichun: public OneCardViewAsSkill {
public:
    IkHuichun(): OneCardViewAsSkill("ikhuichun") {
        filter_pattern = ".|red";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.contains("peach") && !player->hasFlag("Global_PreventPeach")
                && player->getPhase() == Player::NotActive && player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
        peach->addSubcard(originalCard->getId());
        peach->setSkillName(objectName());
        return peach;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int index = qrand() % 2 + 1;
        if (Player::isNostalGeneral(player, "huatuo"))
            index += 2;
        return index;
    }
};

class Mashu: public DistanceSkill {
public:
    Mashu(): DistanceSkill("mashu") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName()))
            return -1;
        else
            return 0;
    }
};

class Xunxun: public PhaseChangeSkill {
public:
    Xunxun(): PhaseChangeSkill("xunxun") {
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *lidian) const{
        if (lidian->getPhase() == Player::Draw) {
            Room *room = lidian->getRoom();
            if (room->askForSkillInvoke(lidian, objectName())) {
                room->broadcastSkillInvoke(objectName());
                QList<ServerPlayer *> p_list;
                p_list << lidian;
                QList<int> card_ids = room->getNCards(4);
                QList<int> obtained;
                room->fillAG(card_ids, lidian);
                int id1 = room->askForAG(lidian, card_ids, false, objectName());
                card_ids.removeOne(id1);
                obtained << id1;
                room->takeAG(lidian, id1, false, p_list);
                int id2 = room->askForAG(lidian, card_ids, false, objectName());
                card_ids.removeOne(id2);
                obtained << id2;
                room->clearAG(lidian);

                room->askForGuanxing(lidian, card_ids, Room::GuanxingDownOnly);
                DummyCard *dummy = new DummyCard(obtained);
                lidian->obtainCard(dummy, false);
                delete dummy;

                return true;
            }
        }

        return false;
    }
};

class Wangxi: public TriggerSkill {
public:
    Wangxi(): TriggerSkill("wangxi") {
        events << Damage << Damaged;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = NULL;
        if (triggerEvent == Damage && !damage.to->hasFlag("Global_DebutFlag"))
            target = damage.to;
        else if (triggerEvent == Damaged)
            target = damage.from;
        if (!target || target == player) return false;
        QList<ServerPlayer *> players;
        players << player << target;
        room->sortByActionOrder(players);

        for (int i = 1; i <= damage.damage; i++) {
            if (!target->isAlive() || !player->isAlive())
                return false;
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue((PlayerStar)target))) {
                room->broadcastSkillInvoke(objectName(), (triggerEvent == Damaged) ? 1 : 2);
                room->drawCards(players, 1, objectName());
            } else {
                break;
            }
        }
        return false;
    }
};

class IkGuijiao: public TriggerSkill {
public:
    IkGuijiao(): TriggerSkill("ikguijiao") {
        events << EventPhaseStart;
    }

    virtual QMap<ServerPlayer *, QStringList> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QMap<ServerPlayer *, QStringList> skill_list;
        if (player->getPhase() == Player::Start) {
            if (player->getMark("@ejiao") > 0) {
                foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                    skill_list.insert(owner, QStringList(objectName()));
            } else {
                bool can_invoke = true;
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (p->getMark("@ejiao") > 0) {
                        can_invoke = false;
                        break;
                    }
                if (can_invoke && TriggerSkill::triggerable(player))
                    skill_list.insert(player, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (player->getMark("@ejiao") > 0) {
            if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player))) {
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        } else {
            ServerPlayer *target = room->askForPlayerChosen(ask_who, room->getOtherPlayers(ask_who), objectName(), "@ikguijiao", true, true);
            if (target) {
                ask_who->tag["ThGuijiaoTarget"] = QVariant::fromValue(target);
                room->broadcastSkillInvoke(objectName());
                return true;
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const{
        if (player->getMark("@ejiao") > 0) {
            ask_who->drawCards(1, objectName());
            room->setPlayerFlag(player, "IkGuijiaoDecMaxCards");
        } else {
            ServerPlayer *target = ask_who->tag["ThGuijiaoTarget"].value<PlayerStar>();
            ask_who->tag.remove("ThGuijiaoTarget");
            if (target)
                target->gainMark("@ejiao");
        }
        return false;
    }
};

class IkGuijiaoMaxCards: public MaxCardsSkill {
public:
    IkGuijiaoMaxCards(): MaxCardsSkill("#ikguijiao-maxcard") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasFlag("IkGuijiaoDecMaxCards"))
            return -1;
        else
            return 0;
    }
};

class IkJinlian: public ProhibitSkill {
public:
    IkJinlian(): ProhibitSkill("ikjinlian") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        if (card->isKindOf("Slash")) {
            // get rangefix
            int rangefix = 0;
            if (card->isVirtualCard()) {
                QList<int> subcards = card->getSubcards();
                if (from->getWeapon() && subcards.contains(from->getWeapon()->getId())) {
                    const Weapon *weapon = qobject_cast<const Weapon *>(from->getWeapon()->getRealCard());
                    rangefix += weapon->getRange() - from->getAttackRange(false);
                }

                if (from->getOffensiveHorse() && subcards.contains(from->getOffensiveHorse()->getId()))
                    rangefix += 1;
            }
            // find yuanshu
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->hasSkill(objectName()) && p != to && p->getHandcardNum() > p->getHp()
                    && from->inMyAttackRange(p, rangefix)) {
                    return true;
                }
            }
        }
        return false;
    }
};

class Yaowu: public TriggerSkill {
public:
    Yaowu(): TriggerSkill("yaowu") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.card->isRed()
            && damage.from && damage.from->isAlive()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(damage.to, objectName());

            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = damage.to;
            log.arg = objectName();
            room->sendLog(log);

            if (damage.from->isWounded() && room->askForChoice(damage.from, objectName(), "recover+draw", data) == "recover")
                room->recover(damage.from, RecoverStruct(damage.to));
            else
                damage.from->drawCards(1, objectName());
        }
        return false;
    }
};

class IkBenyin: public TriggerSkill {
public:
    IkBenyin(): TriggerSkill("ikbenyin") {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && damage.card && damage.card->isKindOf("Slash") && damage.card->isBlack()
            && (player->canDiscard(damage.to, "e") || damage.to->getEquip(2) || damage.to->getEquip(3)))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to))) {
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        DamageStruct damage = data.value<DamageStruct>();
        QList<int> disabled_ids;
        for (int i = 0; i < 5; i++) {
            if (i == 3 || i == 4) continue;
            const Card *card = damage.to->getEquip(i);
            int id = card->getEffectiveId();
            if (!player->canDiscard(damage.to, id))
                disabled_ids << id;
        }
        int card_id = room->askForCardChosen(player, damage.to, "e", objectName(), false, Card::MethodNone, disabled_ids);
        if (card_id == damage.to->getEquip(2)->getEffectiveId() || card_id == damage.to->getEquip(3)->getEffectiveId())
            room->obtainCard(player, card_id);
        else
            room->throwCard(card_id, damage.to, player);
        return false;
    }
};

class Xiaoxi: public TriggerSkill {
public:
    Xiaoxi(): TriggerSkill("xiaoxi") {
        events << Debut;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        ServerPlayer *opponent = player->getNext();
        if (!opponent->isAlive())
            return false;
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_xiaoxi");
        if (player->isLocked(slash) || !player->canSlash(opponent, slash, false)) {
            delete slash;
            return false;
        }
        if (room->askForSkillInvoke(player, objectName()))
            room->useCard(CardUseStruct(slash, player, opponent));
        return false;
    }
};

void StandardPackage::addGenerals() {
    // Wei
    General *bloom001 = new General(this, "bloom001$", "hana");
    bloom001->addSkill(new IkJiaoman);
    bloom001->addSkill(new IkHuanwei);

    General *bloom002 = new General(this, "bloom002", "hana", 3);
    bloom002->addSkill(new IkTiansuo);
    bloom002->addSkill(new IkHuanji);

    General *bloom003 = new General(this, "bloom003", "hana");
    bloom003->addSkill(new IkAoli);
    bloom003->addSkill(new IkAoliRecord);
    related_skills.insertMulti("ikaoli", "#ikaoli");
    bloom003->addSkill(new IkQingjian);

    General *bloom004 = new General(this, "bloom004", "hana");
    bloom004->addSkill(new IkLianbao);
    bloom004->addSkill(new IkLianbaoAct);
    related_skills.insertMulti("iklianbao", "#iklianbao");

    General *bloom005 = new General(this, "bloom005", "hana");
    bloom005->addSkill(new IkLuoyi);
    bloom005->addSkill(new IkLuoyiBuff);
    bloom005->addSkill(new IkLuoyiClear);
    related_skills.insertMulti("ikluoyi", "#ikluoyi");
    related_skills.insertMulti("ikluoyi", "#ikluoyi-clear");

    General *guojia = new General(this, "guojia", "wei", 3); // WEI 006
    guojia->addSkill(new IkTiandu);
    guojia->addSkill(new Yiji);
    guojia->addSkill(new YijiObtain);
    related_skills.insertMulti("yiji", "#yiji");

    General *bloom007 = new General(this, "bloom007", "hana", 3, false);
    bloom007->addSkill(new IkMengyang);
    bloom007->addSkill(new IkMengyangMove);
    bloom007->addSkill(new IkZhongyan);
    related_skills.insertMulti("ikmengyang", "#ikmengyang-move");

    General *lidian = new General(this, "lidian", "wei", 3); // WEI 017
    lidian->addSkill(new Xunxun);
    lidian->addSkill(new Wangxi);

    // Shu
    General *wind001 = new General(this, "wind001$", "kaze");
    wind001->addSkill(new IkShenai);
    wind001->addSkill(new IkXinqi);

    General *wind002 = new General(this, "wind002", "kaze");
    wind002->addSkill(new IkChilian);
    wind002->addSkill(new IkZhenhong);
    wind002->addSkill(new IkZhenhongTargetMod);
    related_skills.insertMulti("ikzhenhong", "#ikzhenhong-target");

    General *wind003 = new General(this, "wind003", "kaze");
    wind003->addSkill(new IkLipao);
    wind003->addSkill(new IkJiukuang);

    General *wind004 = new General(this, "wind004", "kaze", 3);
    wind004->addSkill(new IkYuxi);
    wind004->addSkill(new IkJingyou);

    General *zhaoyun = new General(this, "zhaoyun", "shu"); // SHU 005
    zhaoyun->addSkill(new IkHuahuan);
    zhaoyun->addSkill(new Yajiao);

    General *wind006 = new General(this, "wind006", "kaze");
    wind006->addSkill("thjibu");
    wind006->addSkill(new IkYufeng);
    wind006->addSkill(new IkYufengClear);
    related_skills.insertMulti("ikyufeng", "#ikyufeng-clear");

    General *huangyueying = new General(this, "huangyueying", "shu", 3, false); // SHU 007
    huangyueying->addSkill(new Jizhi);
    huangyueying->addSkill(new Qicai);

    General *st_xushu = new General(this, "st_xushu", "shu"); // SHU 017
    st_xushu->addSkill(new Zhuhai);
    st_xushu->addSkill(new ZhuhaiRecord);
    st_xushu->addSkill(new Qianxin);
    st_xushu->addRelateSkill("jianyan");
    related_skills.insertMulti("zhuhai", "#zhuhai-record");

    // Wu
    General *snow001 = new General(this, "snow001$", "yuki");
    snow001->addSkill(new IkZhiheng);
    snow001->addSkill(new IkJiyuan);

    General *snow002 = new General(this, "snow002", "yuki");
    snow002->addSkill(new IkKuipo);
    snow002->addSkill(new IkGuisi);

    General *snow003 = new General(this, "snow003", "yuki");
    snow003->addSkill(new IkBiju);
    snow003->addSkill(new IkBijuRecord);
    snow003->addSkill(new IkPojian);
    snow003->addSkill(new IkPojianRecord);
    related_skills.insertMulti("ikbiju", "#ikbiju-record");
    related_skills.insertMulti("ikpojian", "#ikpojian-record");
    snow003->addRelateSkill("ikqinghua");

    General *huanggai = new General(this, "huanggai", "wu"); // WU 004
    huanggai->addSkill(new Kurou);
    huanggai->addSkill(new Zhaxiang);
    huanggai->addSkill(new ZhaxiangRedSlash);
    huanggai->addSkill(new ZhaxiangTargetMod);
    related_skills.insertMulti("zhaxiang", "#zhaxiang");
    related_skills.insertMulti("zhaxiang", "#zhaxiang-target");

    General *snow005 = new General(this, "snow005", "yuki", 3);
    snow005->addSkill(new IkGuideng);
    snow005->addSkill(new IkChenhong);
    snow005->addSkill(new IkChenhongMaxCards);
    related_skills.insertMulti("ikchenhong", "#ikchenhong");

    General *snow006 = new General(this, "snow006", "yuki", 3, false);
    snow006->addSkill(new IkWanmei);
    snow006->addSkill(new IkXuanhuo);

    General *luxun = new General(this, "luxun", "wu", 3); // WU 007
    luxun->addSkill(new Qianxun);
    luxun->addSkill(new Lianying);

    General *snow008 = new General(this, "snow008", "yuki", 3, false);
    snow008->addSkill(new IkYulu);
    snow008->addSkill(new IkCuimeng);

    // Qun
    General *huatuo = new General(this, "huatuo", "qun", 3); // QUN 001
    huatuo->addSkill(new Chuli);
    huatuo->addSkill(new IkHuichun);

    General *luna002 = new General(this, "luna002", "tsuki");
    luna002->addSkill(new IkWushuang);
    luna002->addSkill(new IkWudi);

    General *diaochan = new General(this, "diaochan", "qun", 3, false); // QUN 003
    diaochan->addSkill(new Lijian);
    diaochan->addSkill(new IkZhuoyue);

    General *st_huaxiong = new General(this, "st_huaxiong", "qun", 6); // QUN 019
    st_huaxiong->addSkill(new Yaowu);

    General *luna034 = new General(this, "luna034", "tsuki");
    luna034->addSkill(new IkGuijiao);
    luna034->addSkill(new IkGuijiaoMaxCards);
    luna034->addSkill(new IkJinlian);
    related_skills.insertMulti("ikguijiao", "#ikguijiao-maxcard");

    General *luna018 = new General(this, "luna018", "tsuki");
    luna018->addSkill("ikzhuji");
    luna018->addSkill(new IkBenyin);

    // for skill cards
    addMetaObject<IkZhihengCard>();
    addMetaObject<IkShenaiCard>();
    addMetaObject<IkLianbaoCard>();
    addMetaObject<IkYuluCard>();
    addMetaObject<KurouCard>();
    addMetaObject<LijianCard>();
    addMetaObject<IkGuidengCard>();
    addMetaObject<ChuliCard>();
    addMetaObject<IkXuanhuoCard>();
    addMetaObject<LianyingCard>();
    addMetaObject<IkXinqiCard>();
    addMetaObject<YijiCard>();
    addMetaObject<IkGuisiCard>();
    addMetaObject<IkQinghuaCard>();
    addMetaObject<JianyanCard>();
    addMetaObject<IkWanmeiCard>();
    addMetaObject<IkWudiCard>();

    skills << new Xiaoxi << new NonCompulsoryInvalidity << new Jianyan << new IkQinghua;
}

class SuperZhiheng: public IkZhiheng {
public:
    SuperZhiheng():IkZhiheng() {
        setObjectName("super_zhiheng");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && player->usedTimes("IkZhihengCard") < (player->getLostHp() + 1);
    }
};

class SuperGuanxing: public IkYuxi {
public:
    SuperGuanxing(): IkYuxi() {
        setObjectName("super_guanxing");
    }

    virtual int getIkYuxiNum(Room *room) const{
        return 5;
    }
};

class SuperMaxCards: public MaxCardsSkill {
public:
    SuperMaxCards(): MaxCardsSkill("super_max_cards") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return target->getMark("@max_cards_test");
        return 0;
    }
};

class SuperOffensiveDistance: public DistanceSkill {
public:
    SuperOffensiveDistance(): DistanceSkill("super_offensive_distance") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName()))
            return -from->getMark("@offensive_distance_test");
        else
            return 0;
    }
};

class SuperDefensiveDistance: public DistanceSkill {
public:
    SuperDefensiveDistance(): DistanceSkill("super_defensive_distance") {
    }

    virtual int getCorrect(const Player *, const Player *to) const{
        if (to->hasSkill(objectName()))
            return to->getMark("@defensive_distance_test");
        else
            return 0;
    }
};

#include "sp-old.h"
class SuperYongsi: public Yongsi {
public:
    SuperYongsi(): Yongsi() {
        setObjectName("super_yongsi");
    }

    virtual int getKingdoms(ServerPlayer *yuanshu) const{
        return yuanshu->getMark("@yongsi_test");
    }
};

#include "wind.h"
class SuperJushou: public Jushou {
public:
    SuperJushou(): Jushou() {
        setObjectName("super_jushou");
    }

    virtual int getJushouDrawNum(ServerPlayer *caoren) const{
        return caoren->getMark("@jushou_test");
    }
};

#include "god.h"
#include "maneuvering.h"
class GdJuejing: public TriggerSkill {
public:
    GdJuejing(): TriggerSkill("gdjuejing") {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *gaodayihao, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != gaodayihao && move.to != gaodayihao)
                return false;
            if (move.to_place != Player::PlaceHand && !move.from_places.contains(Player::PlaceHand))
                return false;
        }
        if (gaodayihao->getHandcardNum() == 4)
            return false;
        int diff = abs(gaodayihao->getHandcardNum() - 4);
        if (gaodayihao->getHandcardNum() < 4) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = gaodayihao;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(gaodayihao, objectName());
            gaodayihao->drawCards(diff, objectName());
        } else if (gaodayihao->getHandcardNum() > 4) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = gaodayihao;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(gaodayihao, objectName());
            room->askForDiscard(gaodayihao, objectName(), diff, diff);
        }

        return false;
    }
};

class GdJuejingSkipDraw: public DrawCardsSkill {
public:
    GdJuejingSkipDraw(): DrawCardsSkill("#gdjuejing") {
    }

    virtual int getPriority(TriggerEvent) const{
        return 1;
    }

    virtual int getDrawNum(ServerPlayer *gaodayihao, int n) const{
        LogMessage log;
        log.type = "#GdJuejing";
        log.from = gaodayihao;
        log.arg = "gdjuejing";
        gaodayihao->getRoom()->sendLog(log);

        return 0;
    }
};

class GdLonghun: public Longhun {
public:
    GdLonghun(): Longhun() {
        setObjectName("gdlonghun");
    }

    virtual int getEffHp(const Player *) const{
        return 1;
    }
};

class GdLonghunDuojian: public TriggerSkill {
public:
    GdLonghunDuojian(): TriggerSkill("#gdlonghun-duojian") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *gaodayihao, QVariant &) const{
        if (gaodayihao->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, room->getOtherPlayers(gaodayihao)) {
               if (p->getWeapon() && p->getWeapon()->isKindOf("QinggangSword")) {
                   if (room->askForSkillInvoke(gaodayihao, "gdlonghun")) {
                       room->broadcastSkillInvoke("gdlonghun", 5);
                       gaodayihao->obtainCard(p->getWeapon());
                    }
                    break;
                }
            }
        }

        return false;
    }
};

TestPackage::TestPackage()
    : Package("test")
{
    // for test only
    General *zhiba_sunquan = new General(this, "zhiba_sunquan$", "wu", 4, true, true);
    zhiba_sunquan->addSkill(new SuperZhiheng);
    zhiba_sunquan->addSkill("ikjiyuan");

    General *wuxing_zhuge = new General(this, "wuxing_zhugeliang", "shu", 3, true, true);
    wuxing_zhuge->addSkill(new SuperGuanxing);
    wuxing_zhuge->addSkill("ikjingyou");

    General *gaodayihao = new General(this, "gaodayihao", "god", 1, true, true);
    gaodayihao->addSkill(new GdJuejing);
    gaodayihao->addSkill(new GdJuejingSkipDraw);
    related_skills.insertMulti("gdjuejing", "#gdjuejing");
    gaodayihao->addSkill(new GdLonghun);
    gaodayihao->addSkill(new GdLonghunDuojian);
    related_skills.insertMulti("gdlonghun", "#gdlonghun-duojian");

    General *super_yuanshu = new General(this, "super_yuanshu", "qun", 4, true, true);
    super_yuanshu->addSkill(new SuperYongsi);
    super_yuanshu->addSkill(new MarkAssignSkill("@yongsi_test", 4));
    related_skills.insertMulti("super_yongsi", "#@yongsi_test-4");
    super_yuanshu->addSkill("weidi");

    General *super_caoren = new General(this, "super_caoren", "wei", 4, true, true);
    super_caoren->addSkill(new SuperJushou);
    super_caoren->addSkill(new MarkAssignSkill("@jushou_test", 5));
    related_skills.insertMulti("super_jushou", "#@jushou_test-5");

    General *nobenghuai_dongzhuo = new General(this, "nobenghuai_dongzhuo$", "qun", 4, true, true);
    nobenghuai_dongzhuo->addSkill("jiuchi");
    nobenghuai_dongzhuo->addSkill("roulin");
    nobenghuai_dongzhuo->addSkill("baonue");

    new General(this, "sujiang", "god", 5, true, true);
    new General(this, "sujiangf", "god", 5, false, true);

    new General(this, "anjiang", "god", 4, true, true, true);

    skills << new SuperMaxCards << new SuperOffensiveDistance << new SuperDefensiveDistance;
}

ADD_PACKAGE(Test)

