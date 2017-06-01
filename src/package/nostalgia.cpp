#include "nostalgia.h"
#include "client.h"
#include "engine.h"
#include "ikai-kin.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "wind.h"

// old yjcm's generals

class NosWuyan : public TriggerSkill
{
public:
    NosWuyan()
        : TriggerSkill("noswuyan")
    {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.to == effect.from)
            return false;
        if (effect.card->isNDTrick()) {
            if (effect.from && effect.from->hasSkill(objectName())) {
                LogMessage log;
                log.type = "#WuyanBaD";
                log.from = effect.from;
                log.to << effect.to;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(effect.from, objectName());
                room->broadcastSkillInvoke("noswuyan");
                return true;
            }
            if (effect.to->hasSkill(objectName()) && effect.from) {
                LogMessage log;
                log.type = "#WuyanGooD";
                log.from = effect.to;
                log.to << effect.from;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(effect.to, objectName());
                room->broadcastSkillInvoke("noswuyan");
                return true;
            }
        }
        return false;
    }
};

NosJujianCard::NosJujianCard()
{
}

void NosJujianCard::onEffect(const CardEffectStruct &effect) const
{
    int n = subcardsLength();
    effect.to->drawCards(n, "nosjujian");
    Room *room = effect.from->getRoom();

    if (effect.from->isAlive() && n == 3) {
        QSet<Card::CardType> types;
        foreach (int card_id, effect.card->getSubcards())
            types << Sanguosha->getCard(card_id)->getTypeId();

        if (types.size() == 1) {
            LogMessage log;
            log.type = "#JujianRecover";
            log.from = effect.from;
            const Card *card = Sanguosha->getCard(subcards.first());
            log.arg = card->getType();
            room->sendLog(log);
            room->recover(effect.from, RecoverStruct(effect.from));
        }
    }
}

class NosJujian : public ViewAsSkill
{
public:
    NosJujian()
        : ViewAsSkill("nosjujian")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 3 && !Self->isJilei(to_select);
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("NosJujianCard");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        NosJujianCard *card = new NosJujianCard;
        card->addSubcards(cards);
        return card;
    }
};

class NosEnyuan : public TriggerSkill
{
public:
    NosEnyuan()
        : TriggerSkill("nosenyuan")
    {
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == HpRecover) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.who && recover.who != player) {
                room->broadcastSkillInvoke("nosenyuan", qrand() % 2 + 1);
                room->sendCompulsoryTriggerLog(player, objectName());
                recover.who->drawCards(recover.recover, objectName());
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (source && source != player) {
                room->broadcastSkillInvoke("nosenyuan", qrand() % 2 + 3);
                room->sendCompulsoryTriggerLog(player, objectName());

                const Card *card = room->askForCard(source, ".|heart|.|hand", "@nosenyuan-heart", data, Card::MethodNone);
                if (card)
                    player->obtainCard(card);
                else
                    room->loseHp(source);
            }
        }

        return false;
    }
};

NosXuanhuoCard::NosXuanhuoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosXuanhuoCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->obtainCard(this);

    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "he", "nosxuanhuo");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "nosxuanhuo", "@nosxuanhuo-give:" + effect.to->objectName());
    if (target != effect.from) {
        CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), target->objectName(), "nosxuanhuo", QString());
        room->obtainCard(target, Sanguosha->getCard(card_id), reason2, false);
    }
}

class NosXuanhuo : public OneCardViewAsSkill
{
public:
    NosXuanhuo()
        : OneCardViewAsSkill("nosxuanhuo")
    {
        filter_pattern = ".|heart|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("NosXuanhuoCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        NosXuanhuoCard *xuanhuoCard = new NosXuanhuoCard;
        xuanhuoCard->addSubcard(originalCard);
        return xuanhuoCard;
    }
};

class NosShangshi : public IkJiaolian
{
public:
    NosShangshi()
        : IkJiaolian()
    {
        setObjectName("nosshangshi");
    }

    virtual int getMaxLostHp(ServerPlayer *zhangchunhua) const
    {
        return zhangchunhua->getLostHp();
    }
};

class NosChengxiang : public IkXingshi
{
public:
    NosChengxiang()
        : IkXingshi()
    {
        setObjectName("noschengxiang");
        total_point = 12;
    }
};

NosRenxinCard::NosRenxinCard()
{
    target_fixed = true;
    mute = true;
}

void NosRenxinCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const
{
    if (player->isKongcheng())
        return;
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who)
        return;

    room->broadcastSkillInvoke("renxin");
    DummyCard *handcards = player->wholeHandCards();
    player->turnOver();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), who->objectName(), "nosrenxin", QString());
    room->obtainCard(who, handcards, reason, false);
    delete handcards;
    room->recover(who, RecoverStruct(player));
}

class NosRenxin : public ZeroCardViewAsSkill
{
public:
    NosRenxin()
        : ZeroCardViewAsSkill("nosrenxin")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "peach" && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new NosRenxinCard;
    }
};

class NosZhenggong : public MasochismSkill
{
public:
    NosZhenggong()
        : MasochismSkill("noszhenggong")
    {
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("nosbaijiang") == 0;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const
    {
        if (damage.from && damage.from->hasEquip()) {
            QVariant data = QVariant::fromValue(damage.from);
            if (!zhonghui->askForSkillInvoke(objectName(), data))
                return;

            Room *room = zhonghui->getRoom();
            room->broadcastSkillInvoke(objectName());
            int equip = room->askForCardChosen(zhonghui, damage.from, "e", objectName());
            const Card *card = Sanguosha->getCard(equip);

            int equip_index = -1;
            const EquipCard *equipcard = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equipcard->location());

            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct move1(equip, zhonghui, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_ROB, zhonghui->objectName()));
            exchangeMove.push_back(move1);
            if (zhonghui->getEquip(equip_index) != NULL) {
                CardsMoveStruct move2(zhonghui->getEquip(equip_index)->getId(), NULL, Player::DiscardPile,
                                      CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, zhonghui->objectName()));
                exchangeMove.push_back(move2);
            }
            room->moveCardsAtomic(exchangeMove, true);
        }
    }
};

class NosQuanji : public TriggerSkill
{
public:
    NosQuanji()
        : TriggerSkill("nosquanji")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        bool skip = false;
        foreach (ServerPlayer *zhonghui, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(zhonghui) || zhonghui == player || zhonghui->isKongcheng() || zhonghui->getMark("nosbaijiang") > 0 || player->isKongcheng())
                continue;

            if (room->askForSkillInvoke(zhonghui, "nosquanji")) {
                room->broadcastSkillInvoke(objectName(), 1);
                if (zhonghui->pindian(player, objectName(), NULL)) {
                    if (!skip) {
                        room->broadcastSkillInvoke(objectName(), 2);
                        player->skip(Player::Start);
                        player->skip(Player::Judge);
                        skip = true;
                    } else {
                        room->broadcastSkillInvoke(objectName(), 3);
                    }
                }
            }
        }
        return skip;
    }
};

class NosBaijiang : public PhaseChangeSkill
{
public:
    NosBaijiang()
        : PhaseChangeSkill("nosbaijiang")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getMark("nosbaijiang") == 0 && target->getPhase() == Player::Start && target->getEquips().length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const
    {
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#NosBaijiangWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getEquips().length());
        log.arg2 = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$NosBaijiangAnimate", 5000);

        room->setPlayerMark(zhonghui, "nosbaijiang", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui, 1)) {
            room->recover(zhonghui, RecoverStruct(zhonghui));
            if (zhonghui->getMark("nosbaijiang") == 1)
                room->handleAcquireDetachSkills(zhonghui, "-noszhenggong|-nosquanji|nosyexin");
        }

        return false;
    }
};

NosYexinCard::NosYexinCard()
{
    target_fixed = true;
}

void NosYexinCard::onUse(Room *, const CardUseStruct &card_use) const
{
    ServerPlayer *zhonghui = card_use.from;

    QList<int> powers = zhonghui->getPile("nospower");
    if (powers.isEmpty())
        return;
    zhonghui->exchangeFreelyFromPrivatePile("nosyexin", "nospower");
}

class NosYexinViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NosYexinViewAsSkill()
        : ZeroCardViewAsSkill("nosyexin")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("nospower").isEmpty() && !player->hasUsed("NosYexinCard");
    }

    virtual const Card *viewAs() const
    {
        return new NosYexinCard;
    }
};

class NosYexin : public TriggerSkill
{
public:
    NosYexin()
        : TriggerSkill("nosyexin")
    {
        events << Damage << Damaged;
        view_as_skill = new NosYexinViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *zhonghui, QVariant &) const
    {
        if (!zhonghui->askForSkillInvoke(objectName()))
            return false;
        room->broadcastSkillInvoke(objectName(), 1);
        zhonghui->addToPile("nospower", room->drawCard());

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 2;
    }
};

class NosPaiyi : public PhaseChangeSkill
{
public:
    NosPaiyi()
        : PhaseChangeSkill("nospaiyi")
    {
        _m_place["Judging"] = Player::PlaceDelayedTrick;
        _m_place["Equip"] = Player::PlaceEquip;
        _m_place["Hand"] = Player::PlaceHand;
    }

    QString getPlace(Room *room, ServerPlayer *player, QStringList places) const
    {
        if (places.length() > 0) {
            QString place = room->askForChoice(player, "nospaiyi", places.join("+"));
            return place;
        }
        return QString();
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const
    {
        if (zhonghui->getPhase() != Player::Finish || zhonghui->getPile("nospower").isEmpty())
            return false;

        Room *room = zhonghui->getRoom();
        QList<int> powers = zhonghui->getPile("nospower");
        if (powers.isEmpty() || !room->askForSkillInvoke(zhonghui, objectName()))
            return false;
        QStringList places;
        places << "Hand";

        room->fillAG(powers, zhonghui);
        int power = room->askForAG(zhonghui, powers, false, "nospaiyi");
        room->clearAG(zhonghui);

        if (power == -1)
            power = powers.first();

        const Card *card = Sanguosha->getCard(power);

        ServerPlayer *target = room->askForPlayerChosen(zhonghui, room->getAlivePlayers(), "nospaiyi", "@nospaiyi-to:::" + card->objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, zhonghui->objectName(), "nospaiyi", QString());

        if (card->isKindOf("DelayedTrick")) {
            if (!zhonghui->isProhibited(target, card) && !target->containsTrick(card->objectName()))
                places << "Judging";
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        } else if (card->isKindOf("EquipCard")) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            if (!target->getEquip(equip->location()))
                places << "Equip";
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        } else
            room->moveCardTo(card, zhonghui, target, _m_place[getPlace(room, zhonghui, places)], reason, true);

        int index = 1;
        if (target != zhonghui) {
            index++;
            room->drawCards(zhonghui, 1, objectName());
        }
        room->broadcastSkillInvoke(objectName(), index);

        return false;
    }

private:
    QMap<QString, Player::Place> _m_place;
};

class NosZili : public PhaseChangeSkill
{
public:
    NosZili()
        : PhaseChangeSkill("noszili")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getMark("noszili") == 0 && target->getPhase() == Player::Start && target->getPile("nospower").length() >= 4;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const
    {
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#NosZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("nospower").length());
        log.arg2 = objectName();
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$NosZiliAnimate", 5000);

        room->setPlayerMark(zhonghui, "noszili", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui) && zhonghui->getMark("noszili") == 1)
            room->acquireSkill(zhonghui, "nospaiyi");

        return false;
    }
};

class NosGuixin : public PhaseChangeSkill
{
public:
    NosGuixin()
        : PhaseChangeSkill("nosguixin")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *weiwudi) const
    {
        if (weiwudi->getPhase() != Player::Finish)
            return false;

        Room *room = weiwudi->getRoom();
        if (!room->askForSkillInvoke(weiwudi, objectName()))
            return false;

        QString choice = room->askForChoice(weiwudi, objectName(), "modify+obtain");

        int index = qrand() % 2 + 1;

        if (choice == "modify") {
            ServerPlayer *to_modify = room->askForPlayerChosen(weiwudi, room->getOtherPlayers(weiwudi), objectName());
            QStringList kingdomList = Sanguosha->getKingdoms();
            kingdomList.removeOne("kami");
            QString old_kingdom = to_modify->getKingdom();
            kingdomList.removeOne(old_kingdom);
            if (kingdomList.isEmpty())
                return false;
            QString kingdom = room->askForChoice(weiwudi, "nosguixin_kingdom", kingdomList.join("+"), QVariant::fromValue(to_modify));
            room->setPlayerProperty(to_modify, "kingdom", kingdom);

            room->broadcastSkillInvoke(objectName(), index);

            LogMessage log;
            log.type = "#ChangeKingdom";
            log.from = weiwudi;
            log.to << to_modify;
            log.arg = old_kingdom;
            log.arg2 = kingdom;
            room->sendLog(log);
        } else if (choice == "obtain") {
            room->broadcastSkillInvoke(objectName(), index + 2);
            QStringList lords = Sanguosha->getLords();
            foreach (ServerPlayer *player, room->getAlivePlayers()) {
                QString name = player->getGeneralName();
                if (Sanguosha->isGeneralHidden(name)) {
                    QString fname = Sanguosha->findConvertFrom(name);
                    if (!fname.isEmpty())
                        name = fname;
                }
                lords.removeOne(name);

                if (!player->getGeneral2())
                    continue;

                name = player->getGeneral2Name();
                if (Sanguosha->isGeneralHidden(name)) {
                    QString fname = Sanguosha->findConvertFrom(name);
                    if (!fname.isEmpty())
                        name = fname;
                }
                lords.removeOne(name);
            }

            QStringList lord_skills;
            foreach (QString lord, lords) {
                const General *general = Sanguosha->getGeneral(lord);
                QList<const Skill *> skills = general->findChildren<const Skill *>();
                foreach (const Skill *skill, skills) {
                    if (skill->isLordSkill() && !weiwudi->hasSkill(skill->objectName()))
                        lord_skills << skill->objectName();
                }
            }

            if (!lord_skills.isEmpty()) {
                QString skill_name = room->askForChoice(weiwudi, "nosguixin_lordskills", lord_skills.join("+"));

                const Skill *skill = Sanguosha->getSkill(skill_name);
                room->acquireSkill(weiwudi, skill);
            }
        }
        return false;
    }
};

// old stantard generals

class NosJianxiong : public MasochismSkill
{
public:
    NosJianxiong()
        : MasochismSkill("nosjianxiong")
    {
    }

    virtual void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const
    {
        Room *room = caocao->getRoom();
        const Card *card = damage.card;
        if (!card)
            return;

        QList<int> ids;
        if (card->isVirtualCard())
            ids = card->getSubcards();
        else
            ids << card->getEffectiveId();

        if (ids.isEmpty())
            return;
        foreach (int id, ids) {
            if (room->getCardPlace(id) != Player::PlaceTable)
                return;
        }
        QVariant data = QVariant::fromValue(damage);
        if (room->askForSkillInvoke(caocao, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            caocao->obtainCard(card);
        }
    }
};

class NosFankui : public MasochismSkill
{
public:
    NosFankui()
        : MasochismSkill("nosfankui")
    {
    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const
    {
        ServerPlayer *from = damage.from;
        Room *room = simayi->getRoom();
        QVariant data = QVariant::fromValue(from);
        if (from && !from->isNude() && room->askForSkillInvoke(simayi, "nosfankui", data)) {
            room->broadcastSkillInvoke(objectName());
            int card_id = room->askForCardChosen(simayi, from, "he", "nosfankui");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, simayi->objectName());
            room->obtainCard(simayi, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
        }
    }
};

class NosGuicai : public TriggerSkill
{
public:
    NosGuicai()
        : TriggerSkill("nosguicai")
    {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->isKongcheng())
            return false;

        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@nosguicai-card" << judge->who->objectName() << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, ".", prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
            room->broadcastSkillInvoke(objectName());
            room->retrial(card, player, judge, objectName());
        }

        return false;
    }
};

class NosGanglie : public MasochismSkill
{
public:
    NosGanglie()
        : MasochismSkill("nosganglie")
    {
    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage) const
    {
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();
        QVariant data = QVariant::fromValue(damage);

        if (room->askForSkillInvoke(xiahou, "nosganglie", data)) {
            room->broadcastSkillInvoke("ikaoli");

            JudgeStruct judge;
            judge.pattern = ".|heart";
            judge.good = false;
            judge.reason = objectName();
            judge.who = xiahou;

            room->judge(judge);
            if (!from || from->isDead())
                return;
            if (judge.isGood()) {
                if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
                    room->damage(DamageStruct(objectName(), xiahou, from));
            }
        }
    }
};

NosTuxiCard::NosTuxiCard()
{
}

bool NosTuxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.length() >= 2 || to_select == Self)
        return false;

    return !to_select->isKongcheng();
}

void NosTuxiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.from->isAlive() && !effect.to->isKongcheng()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "ikchibao");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
        room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
    }
}

class NosTuxiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NosTuxiViewAsSkill()
        : ZeroCardViewAsSkill("nostuxi")
    {
        response_pattern = "@@nostuxi";
    }

    virtual const Card *viewAs() const
    {
        return new NosTuxiCard;
    }
};

class NosTuxi : public PhaseChangeSkill
{
public:
    NosTuxi()
        : PhaseChangeSkill("nostuxi")
    {
        view_as_skill = new NosTuxiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *zhangliao) const
    {
        if (zhangliao->getPhase() == Player::Draw) {
            Room *room = zhangliao->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(zhangliao);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }
            zhangliao->setFlags("-NosTuxiAudioBroadcast");

            if (can_invoke && room->askForUseCard(zhangliao, "@@nostuxi", "@nostuxi-card"))
                return true;
        }

        return false;
    }
};

class NosLuoyiBuff : public TriggerSkill
{
public:
    NosLuoyiBuff()
        : TriggerSkill("#nosluoyi")
    {
        events << DamageCaused;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasFlag("nosluoyi") && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class NosLuoyi : public DrawCardsSkill
{
public:
    NosLuoyi()
        : DrawCardsSkill("nosluoyi")
    {
    }

    virtual int getDrawNum(ServerPlayer *xuchu, int n) const
    {
        Room *room = xuchu->getRoom();
        if (room->askForSkillInvoke(xuchu, objectName())) {
            room->broadcastSkillInvoke(objectName());
            xuchu->setFlags(objectName());
            return n - 1;
        } else
            return n;
    }
};

NosRendeCard::NosRendeCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosRendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();

    room->broadcastSkillInvoke("ikshenai");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nosrende", QString());
    room->obtainCard(target, this, reason, false);

    int old_value = source->getMark("nosrende");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "nosrende", new_value);

    if (old_value < 2 && new_value >= 2)
        room->recover(source, RecoverStruct(source));
}

class NosRendeViewAsSkill : public ViewAsSkill
{
public:
    NosRendeViewAsSkill()
        : ViewAsSkill("nosrende")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("nosrende") >= 2)
            return false;
        else
            return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("nosrende") >= 2)
            return false;
        return !player->isKongcheng();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        NosRendeCard *rende_card = new NosRendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class NosRende : public TriggerSkill
{
public:
    NosRende()
        : TriggerSkill("nosrende")
    {
        events << EventPhaseChanging;
        view_as_skill = new NosRendeViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("nosrende") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "nosrende", 0);
        return false;
    }
};

class NosTieji : public TriggerSkill
{
public:
    NosTieji()
        : TriggerSkill("nostieji")
    {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive())
                break;
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());

                p->setFlags("NosTiejiTarget"); // For AI

                JudgeStruct judge;
                judge.pattern = ".|red";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                try {
                    room->judge(judge);
                } catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        p->setFlags("-NosTiejiTarget");
                    throw triggerEvent;
                }

                if (judge.isGood()) {
                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);
                    jink_list.replace(index, QVariant(0));
                }

                p->setFlags("-NosTiejiTarget");
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class NosYingzi : public DrawCardsSkill
{
public:
    NosYingzi()
        : DrawCardsSkill("nosyingzi")
    {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *zhouyu, int n) const
    {
        Room *room = zhouyu->getRoom();
        if (room->askForSkillInvoke(zhouyu, objectName())) {
            room->broadcastSkillInvoke("nosyingzi");
            return n + 1;
        } else
            return n;
    }
};

NosFanjianCard::NosFanjianCard()
{
}

void NosFanjianCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    int card_id = zhouyu->getRandomHandCardId();
    const Card *card = Sanguosha->getCard(card_id);
    Card::Suit suit = room->askForSuit(target, "nosfanjian");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = target;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->getThread()->delay();
    target->obtainCard(card);
    room->showCard(target, card_id);

    if (card->getSuit() != suit)
        room->damage(DamageStruct("nosfanjian", zhouyu, target));
}

class NosFanjian : public ZeroCardViewAsSkill
{
public:
    NosFanjian()
        : ZeroCardViewAsSkill("nosfanjian")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("NosFanjianCard");
    }

    virtual const Card *viewAs() const
    {
        return new NosFanjianCard;
    }
};

class NosGuose : public OneCardViewAsSkill
{
public:
    NosGuose()
        : OneCardViewAsSkill("nosguose")
    {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Indulgence *indulgence = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
        indulgence->addSubcard(originalCard->getId());
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

// old wind generals

#include "wind.h"
class NosJushou : public Jushou
{
public:
    NosJushou()
        : Jushou()
    {
        setObjectName("nosjushou");
    }

    virtual int getJushouDrawNum(ServerPlayer *) const
    {
        return 3;
    }
};

NostalGeneralPackage::NostalGeneralPackage()
    : Package("nostal_general")
{
    General *nos_zhonghui = new General(this, "nos_zhonghui", "wei", 3);
    nos_zhonghui->addSkill(new NosZhenggong);
    nos_zhonghui->addSkill(new NosQuanji);
    nos_zhonghui->addSkill(new NosBaijiang);
    nos_zhonghui->addSkill(new NosZili);
    nos_zhonghui->addRelateSkill("nosyexin");
    nos_zhonghui->addRelateSkill("#nosyexin-fake-move");
    related_skills.insertMulti("nosyexin", "#nosyexin-fake-move");
    nos_zhonghui->addRelateSkill("nospaiyi");

    General *nos_shencaocao = new General(this, "nos_shencaocao", "god", 3);
    nos_shencaocao->addSkill(new NosGuixin);
    nos_shencaocao->addSkill("thfeiying");

    addMetaObject<NosYexinCard>();

    skills << new NosYexin << new FakeMoveSkill("nosyexin") << new NosPaiyi;
}

NostalStandardPackage::NostalStandardPackage()
    : Package("nostal_standard")
{
    General *nos_caocao = new General(this, "nos_caocao$", "wei");
    nos_caocao->addSkill(new NosJianxiong);
    nos_caocao->addSkill("ikhuanwei");

    General *nos_simayi = new General(this, "nos_simayi", "wei", 3);
    nos_simayi->addSkill(new NosFankui);
    nos_simayi->addSkill(new NosGuicai);

    General *nos_xiahoudun = new General(this, "nos_xiahoudun", "wei");
    nos_xiahoudun->addSkill(new NosGanglie);

    General *nos_zhangliao = new General(this, "nos_zhangliao", "wei");
    nos_zhangliao->addSkill(new NosTuxi);

    General *nos_xuchu = new General(this, "nos_xuchu", "wei");
    nos_xuchu->addSkill(new NosLuoyi);
    nos_xuchu->addSkill(new NosLuoyiBuff);
    related_skills.insertMulti("nosluoyi", "#nosluoyi");

    General *nos_liubei = new General(this, "nos_liubei$", "shu");
    nos_liubei->addSkill(new NosRende);
    nos_liubei->addSkill("ikxinqi");

    General *nos_guanyu = new General(this, "nos_guanyu", "shu");
    nos_guanyu->addSkill("ikchilian");

    General *nos_zhangfei = new General(this, "nos_zhangfei", "shu");
    nos_zhangfei->addSkill("ikyipao");

    General *nos_zhaoyun = new General(this, "nos_zhaoyun", "shu");
    nos_zhaoyun->addSkill("ikhuahuan");

    General *nos_machao = new General(this, "nos_machao", "shu");
    nos_machao->addSkill("thjibu");
    nos_machao->addSkill(new NosTieji);

    General *nos_ganning = new General(this, "nos_ganning", "wu");
    nos_ganning->addSkill("ikkuipo");

    General *nos_lvmeng = new General(this, "nos_lvmeng", "wu");
    nos_lvmeng->addSkill("ikbiju");

    General *nos_zhouyu = new General(this, "nos_zhouyu", "wu", 3);
    nos_zhouyu->addSkill(new NosYingzi);
    nos_zhouyu->addSkill(new NosFanjian);

    General *nos_daqiao = new General(this, "nos_daqiao", "wu", 3, false);
    nos_daqiao->addSkill(new NosGuose);
    nos_daqiao->addSkill("ikxuanhuo");

    General *nos_lvbu = new General(this, "nos_lvbu", "qun");
    nos_lvbu->addSkill("ikwushuang");

    addMetaObject<NosTuxiCard>();
    addMetaObject<NosRendeCard>();
    addMetaObject<NosFanjianCard>();
}

NostalWindPackage::NostalWindPackage()
    : Package("nostal_wind")
{
    General *nos_caoren = new General(this, "nos_caoren", "wei");
    nos_caoren->addSkill(new NosJushou);
}

NostalYJCMPackage::NostalYJCMPackage()
    : Package("nostal_yjcm")
{
    General *nos_fazheng = new General(this, "nos_fazheng", "shu", 3);
    nos_fazheng->addSkill(new NosEnyuan);
    nos_fazheng->addSkill(new NosXuanhuo);

    General *nos_xushu = new General(this, "nos_xushu", "shu", 3);
    nos_xushu->addSkill(new NosWuyan);
    nos_xushu->addSkill(new NosJujian);

    General *nos_zhangchunhua = new General(this, "nos_zhangchunhua", "wei", 3, false);
    nos_zhangchunhua->addSkill("ikxuwu");
    nos_zhangchunhua->addSkill(new NosShangshi);

    addMetaObject<NosXuanhuoCard>();
    addMetaObject<NosJujianCard>();
}

NostalYJCM2013Package::NostalYJCM2013Package()
    : Package("nostal_yjcm2013")
{
    General *nos_caochong = new General(this, "nos_caochong", "wei", 3);
    nos_caochong->addSkill(new NosChengxiang);
    nos_caochong->addSkill(new NosRenxin);

    addMetaObject<NosRenxinCard>();
}

ADD_PACKAGE(NostalGeneral)
ADD_PACKAGE(NostalWind)
ADD_PACKAGE(NostalStandard)
ADD_PACKAGE(NostalYJCM)
ADD_PACKAGE(NostalYJCM2013)
