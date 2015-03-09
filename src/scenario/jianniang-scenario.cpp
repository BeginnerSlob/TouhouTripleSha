#include "jianniang-scenario.h"

#include "skill.h"
#include "engine.h"

#define DAHE "luna019"
#define ZHENMING "snow039"
#define XIANG "wind010"
#define XILI "wind051"
#define RUIFENG "wind003"
#define JIAHE "wind047"
#define BEISHANG "bloom036"
#define DAOFENG "luna037"
#define AIDANG "bloom052"
#define MISHENG "snow052"
#define CHICHENG "luna052"
#define SHU "wind052"

class JianniangScenarioRule: public ScenarioRule {
public:
    JianniangScenarioRule(Scenario *scenario)
        : ScenarioRule(scenario)
    {
        events << GameStart << GameOverJudge << BuryVictim;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        switch (triggerEvent) {
        case GameStart: {
            if (player != NULL) {
                
                return false;
            }
            break;
                        }
        case GameOverJudge: {
            QList<ServerPlayer *> players = room->getAlivePlayers();
            if (players.length() == 1)
                room->gameOver(players.first()->objectName());
            else {
                QString first_role = players.first()->getRole();
                if (first_role != "renegade") {
                    bool all_same = true;
                    foreach (ServerPlayer *p, players) {
                        if (p->getRole() == "renegade" || p->getRole() != first_role) {
                            all_same = false;
                            break;
                        }
                    }
                    if (all_same)
                        room->gameOver(first_role);
                }
            }
            return true;
                            }
        case BuryVictim: {
            DeathStruct death = data.value<DeathStruct>();
            player->bury();
            // reward and punishment
            if (death.damage && death.damage->from) {
                ServerPlayer *killer = death.damage->from;
                if (killer == player || killer->hasSkill("iktianzuo"))
                    return false;
                if (killer->getRole() != "renegade" && killer->getRole() == death.who->getRole())
                    killer->throwAllHandCardsAndEquips();
                else
                    killer->drawCards(3, "kill");
            }

            break;
                         }
        default:
            break;
        }

        return false;
    }
};

JianniangScenario::JianniangScenario()
    : Scenario("jianniang")
{
    rule = new JianniangScenarioRule(this);
}

void JianniangScenario::assign(QStringList &generals, QStringList &roles) const{
    generals << DAHE
             << ZHENMING
             << XIANG
             << XILI
             << RUIFENG
             << JIAHE
             << BEISHANG
             << DAOFENG
             << AIDANG
             << MISHENG
             << CHICHENG
             << SHU;
    qShuffle(generals);
    generals.mid(0, 8);

    // roles
    for (int i = 0; i < 8; i++)
        roles << "renegade";
}

int JianniangScenario::getPlayerCount() const{
    return 8;
}

QString JianniangScenario::getRoles() const{
    return "NNNNNNNN";
}

void JianniangScenario::onTagSet(Room *, const QString &) const{
}
