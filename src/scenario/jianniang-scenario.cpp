#include "jianniang-scenario.h"

#include "skill.h"
#include "engine.h"

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
            if (player != NULL)
                return false;
            break;
                        }
        case GameOverJudge: {
            return true;
                            }
        case BuryVictim: {
            DeathStruct death = data.value<DeathStruct>();
            player->bury();
            // reward and punishment
            if (death.damage && death.damage->from) {
                ServerPlayer *killer = death.damage->from;
                if (killer == player)
                    return false;
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
    generals << "bloom036" // beishang
             << "luna019"  // dahe
             << "luna037"  // daofeng
             << "snow039"  // zhenming
             << "wind003"  // ruifeng
             << "wind010"  // xiang
             << "wind047"  // jiahe
             << "wind051"; // xili
    qShuffle(generals);

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
