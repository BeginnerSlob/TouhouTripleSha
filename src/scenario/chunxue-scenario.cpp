#include "chunxue-scenario.h"

#include "skill.h"

class CxLinli: public DrawCardsSkill {
public:
    CxLinli(): DrawCardsSkill("cxlinli") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#CxLinli";
        log.from = player;
        log.arg = "1";
        log.arg2 = "ChunXueWu";
        room->sendLog(log);
        return n - 1;
    }
};

class CxLinliLord: public InvaliditySkill {
public:
    CxLinliLord(): InvaliditySkill("cxlinli_lord") {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const{
        return player->getMark("ChunXueWu") == 0 || !(skill->objectName() == "thyoushang" || skill->objectName() == "thmanxiao");
    }
};

class CxQihuang: public TriggerSkill {
public:
    CxQihuang(): TriggerSkill("cxqihuang") {
        events << CardUsed;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (player->getGeneralName() == "hana002") {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && (use.card->isKindOf("Snatch") || use.card->isKindOf("ThJiewuCard"))) {
                foreach (ServerPlayer *p, use.to) {
                    if (TriggerSkill::triggerable(p) && p->getMark("touxindao") == 0) {
                        ask_who = p;
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const{
        room->addPlayerMark(ask_who, "touxindao");
        room->doStory("$TouXinDao", 4000);

        LogMessage log;
        log.type = "#CxQihuang";
        log.from = ask_who;
        log.arg = "TouXinDao";
        room->sendLog(log);

        ask_who->drawCards(1, objectName());
        room->setPlayerProperty(ask_who, "role", "rebel");
        room->setTag("TouXinDao", true);
        return false;
    }
};

class CxXuqu: public TriggerSkill {
public:
    CxXuqu(): TriggerSkill("cxxuqu") {
        events << TargetSpecified;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &ask_who) const{
        if (player->getRole() == "rebel" || player->getGeneralName() == "yuki004") {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->getTypeId() != Card::TypeSkill) {
                foreach (ServerPlayer *to, use.to) {
                    if (to->hasSkill(objectName(), true)) {
                        QStringList froms = to->tag["CxXuquRecord"].toStringList();
                        if (!froms.contains(player->objectName())) {
                            froms << player->objectName();
                            to->tag["CxXuquRecord"] = QVariant::fromValue(froms);
                        }
                        if (froms.length() == 4 && to->getMark("mingyufei") == 0) {
                            ask_who = to;
                            return QStringList(objectName());
                        }
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const{
        room->addPlayerMark(ask_who, "mingyufei");
        room->doStory("$MingYuFei", 4000);

        LogMessage log;
        log.type = "#CxXuqu";
        log.from = ask_who;
        log.arg = "MingYuFei";
        room->sendLog(log);

        QList<ServerPlayer *> targets = room->getAllPlayers();
        targets.removeOne(room->getLord());
        targets.removeOne(room->findPlayer("yuki003", true));
        room->drawCards(targets, 1, objectName());

        room->setTag("MingYuFei", true);
        return false;
    }
};

CxQiuwenCard::CxQiuwenCard() {
    target_fixed = true;
}

void CxQiuwenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->doStory("$YouMingFa", 4000);
    room->setPlayerFlag(source, "CxQiuwenUsed");
    room->setTag("YouMingFa", true);
}

class CxQiuwen: public ZeroCardViewAsSkill {
public:
    CxQiuwen(): ZeroCardViewAsSkill("cxqiuwen") {
        frequency = Limited;
    }

    virtual const Card *viewAs() const {
        return new CxQiuwenCard;
    }
};

class CxQiuwenTrigger: public TriggerSkill {
public:
    CxQiuwenTrigger(): TriggerSkill("#cxqiuwen") {
        events << PreCardUsed << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified) {
            if (use.card->isKindOf("Slash") && player->hasFlag("CxQiuwenUsed"))
                return QStringList(objectName());
        } else if (triggerEvent == PreCardUsed) {
            if (use.card->isKindOf("Slash") && player->hasFlag("CxQiuwenUsed")) {
                if (use.m_addHistory)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to.toSet())
                p->addQinggangTag(use.card);
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            room->addPlayerHistory(player, use.card->getClassName(), -1);
            use.m_addHistory = false;
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class CxQiuwenTargetMod: public TargetModSkill {
public:
    CxQiuwenTargetMod(): TargetModSkill("#cxqiuwen-tar") {
        frequency = Limited;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasFlag("CxQiuwenUsed"))
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasFlag("CxQiuwenUsed"))
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasFlag("CxQiuwenUsed"))
            return 1;
        else
            return 0;
    }
};

class CxLiujing: public TriggerSkill {
public:
    CxLiujing(): TriggerSkill("cxliujing") {
        events << DamageInflicted;
        frequency = Limited;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const {
        if (!TriggerSkill::triggerable(player))
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->askForSkillInvoke(objectName())) {
            room->doStory("$KongGuanJian", 4000);
            room->broadcastSkillInvoke(objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const {
        if (player->getHandcardNum() < 4)
            player->drawCards(4 - player->getHandcardNum(), objectName());

        DamageStruct damage = data.value<DamageStruct>();
        damage.to = damage.from;
        damage.transfer = true;
        damage.transfer_reason = objectName();
        player->tag["TransferDamage"] = QVariant::fromValue(damage);

        if (damage.from->isAlive())
            damage.from->turnOver();

        room->setTag("KongGuanJian", true);

        return true;
    }
};

class ChunxueRule: public ScenarioRule {
public:
    ChunxueRule(Scenario *scenario)
        : ScenarioRule(scenario)
    {
        events << GameStart << DrawNCards << Damaged << GameOverJudge;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        switch (triggerEvent) {
        case GameStart: {
                if (player == NULL) {
                    room->doStory("$ChunXueWu", 4000);

                    ServerPlayer *lord = room->getLord();
                    room->installEquip(lord, "moon_spear");
                    room->acquireSkill(lord, "cxlinli_lord");
                    room->setPlayerMark(lord, "ChunXueWu", 1);

                    ServerPlayer *yaomeng = room->findPlayer("yuki003");
                    room->installEquip(yaomeng, "qinggang_sword");

                    ServerPlayer *chen = room->findPlayer("yuki006");
                    room->acquireSkill(chen, "cxxuqu");

                    ServerPlayer *lingmeng = room->findPlayer("yuki001");
                    room->installEquip(lingmeng, "chitu");
                    room->acquireSkill(lingmeng, "cxlinli");

                    ServerPlayer *molisha = room->findPlayer("hana002");
                    room->installEquip(molisha, "fan");
                    room->acquireSkill(molisha, "cxlinli");

                    ServerPlayer *xiaoye = room->findPlayer("tsuki008");
                    room->acquireSkill(xiaoye, "cxlinli");

                    ServerPlayer *ailisi = room->findPlayer("yuki004");
                    room->installEquip(ailisi, "vine");
                    room->acquireSkill(ailisi, "cxlinli");
                    room->acquireSkill(ailisi, "cxqihuang");
                }

                break;
            }
        case DrawNCards: {
                /*if (player->getPhase() == Player::Draw) {
                    bool burned = room->getTag("BurnWuchao").toBool();
                    if (!burned) {
                        QString name = player->getGeneralName();
                        if (name == "caocao" || name == "nos_guojia" || name == "guanyu")
                            data = data.toInt() - 1;
                    }
                }*/
                break;
            }
        case Damaged: {
                /*bool burned = room->getTag("BurnWuchao").toBool();
                if (burned) return false;

                DamageStruct damage = data.value<DamageStruct>();
                if (player->getGeneralName() == "yuanshao" && damage.nature == DamageStruct::Fire
                    && damage.from->getRoleEnum() == Player::Rebel) {
                    room->setTag("BurnWuchao", true);

                    QStringList tos;
                    tos << "yuanshao" << "yanliangwenchou" << "zhenji" << "liubei";

                    foreach (QString name, tos) {
                        ServerPlayer *to = room->findPlayer(name);
                        if (to == NULL || to->containsTrick("supply_shortage"))
                            continue;

                        int card_id = room->getCardFromPile("@ikkujie");
                        if (card_id == -1)
                            break;

                        const Card *originalCard = Sanguosha->getCard(card_id);

                        LogMessage log;
                        log.type = "#BurnWuchao";
                        log.from = to;
                        log.card_str = originalCard->toString();
                        room->sendLog(log);

                        SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
                        shortage->setSkillName("ikkujie");
                        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
                        card->takeOver(shortage);
                        room->broadcastUpdateCard(room->getPlayers(), card->getId(), card);
                        room->moveCardTo(card, to, Player::PlaceDelayedTrick, true);
                        shortage->deleteLater();
                    }
                }*/
                break;
            }
        case GameOverJudge: {
                /*if (player->isLord()) {
                    QStringList roles = room->aliveRoles(player);
                    if (roles.length() == 2) {
                        QString first = roles.at(0);
                        QString second = roles.at(1);
                        if (first == "renegade" && second == "renegade") {
                            player->bury();
                            room->gameOver("renegade");
                            return true;
                        }
                    }
                }*/
                break;
            }
        default:
            break;
        }

        return false;
    }
};

ChunxueScenario::ChunxueScenario()
    : Scenario("chunxue")
{
    lord = "kami007";
    loyalists << "yuki003" << "yuki006";
    rebels << "yuki001" << "hana002" << "tsuki008";
    renegades << "yuki004" << "yuki010";

    rule = new ChunxueRule(this);

    skills << new CxLinli
           << new CxLinliLord
           << new CxQihuang
           << new CxXuqu
           << new CxQiuwen
           << new CxQiuwenTrigger
           << new CxQiuwenTargetMod
           << new CxLiujing;
    related_skills.insertMulti("cxqiuwen", "#cxqiuwen");
    related_skills.insertMulti("cxqiuwen", "#cxqiuwen-tar");

    addMetaObject<CxQiuwenCard>();
}

void ChunxueScenario::onTagSet(Room *room, const QString &key) const{
    if (key == "MingYuFei") {
        ServerPlayer *yaomeng = room->findPlayer("yuki003");
        if (yaomeng) {
            room->acquireSkill(yaomeng, "cxqiuwen");
            room->acquireSkill(yaomeng, "cxliujing");
        }

        ServerPlayer *chen = room->findPlayer("yuki006");
        if (chen)
            room->detachSkillFromPlayer(chen, "cxxuqu", true);

        ServerPlayer *lingmeng = room->findPlayer("yuki001");
        if (lingmeng)
            room->detachSkillFromPlayer(lingmeng, "cxlinli", true);

        ServerPlayer *molisha = room->findPlayer("hana002");
        if (molisha)
            room->detachSkillFromPlayer(molisha, "cxlinli", true);

        ServerPlayer *xiaoye = room->findPlayer("tsuki008");
        if (xiaoye)
            room->detachSkillFromPlayer(xiaoye, "cxlinli", true);

        ServerPlayer *ailisi = room->findPlayer("yuki004");
        if (ailisi)
            room->detachSkillFromPlayer(ailisi, "cxlinli", true);
    } else if (key == "YouMingFa") {
        ServerPlayer *yaomeng = room->findPlayer("yuki003");
        if (yaomeng)
            room->detachSkillFromPlayer(yaomeng, "cxqiuwen", true);
    } else if (key == "KongGuanJian") {
        ServerPlayer *yaomeng = room->findPlayer("yuki003");
        if (yaomeng)
            room->detachSkillFromPlayer(yaomeng, "cxliujing", true);
    } else if (key == "MoRanYing") {
        ServerPlayer *lord = room->getLord();
        if (lord) {
            room->detachSkillFromPlayer(lord, "cxlinli_lord", true);
            room->setPlayerMark(lord, "ChunXueWu", 0);
        }
    }
}
