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
                    if (TriggerSkill::triggerable(p)) {
                        ask_who = p;
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const{
        room->doStory("$TouXinDao", 4000);

        LogMessage log;
        log.type = "#CxQihuang";
        log.from = ask_who;
        log.arg = "TouXinDao";
        room->sendLog(log);

        room->changeMaxHpForAwakenSkill(ask_who, 0);

        ask_who->drawCards(1, objectName());
        room->setPlayerProperty(ask_who, "role", "rebel");
        return false;
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

                    ServerPlayer *youmeng = room->findPlayer("yuki003");
                    room->installEquip(youmeng, "qinggang_sword");

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
           << new CxQihuang;
}

void ChunxueScenario::onTagSet(Room *room, const QString &key) const{
    if (key == "MingYuFei") {
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
    } else if (key == "MoRanYing") {
        ServerPlayer *lord = room->getLord();
        if (lord) {
            room->detachSkillFromPlayer(lord, "cxlinli_lord", true);
            room->setPlayerMark(lord, "ChunXueWu", 0);
        }
    }
}
