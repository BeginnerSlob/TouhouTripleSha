#include "chunxue-scenario.h"

#include "skill.h"

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
                /*player = room->getLord();
                room->installEquip(player, "renwang_shield");
                room->installEquip(player, "hualiu");

                ServerPlayer *caocao = room->findPlayer("caocao");
                room->installEquip(caocao, "qinggang_sword");
                room->installEquip(caocao, "zhuahuangfeidian");

                ServerPlayer *liubei = room->findPlayer("liubei");
                room->installEquip(liubei, "double_sword");

                ServerPlayer *guanyu = room->findPlayer("guanyu");
                room->installEquip(guanyu, "blade");
                room->installEquip(guanyu, "chitu");
                room->acquireSkill(guanyu, "zhanshuangxiong");


                ServerPlayer *zhangliao = room->findPlayer("nos_zhangliao");
                room->handleAcquireDetachSkills(zhangliao, "-nostuxi|smalltuxi");*/

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

    skills;
}

void ChunxueScenario::onTagSet(Room *room, const QString &key) const{
}
