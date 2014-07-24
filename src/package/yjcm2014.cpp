#include "yjcm2014.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"

class Youdi: public PhaseChangeSkill {
public:
    Youdi(): PhaseChangeSkill("youdi") {
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Finish || target->isNude()) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> players;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->canDiscard(target, "he")) players << p;
        }
        if (players.isEmpty()) return false;
        ServerPlayer *player = room->askForPlayerChosen(target, players, objectName(), "youdi-invoke", true, true);
        if (player) {
            int id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, target, player);
            if (!Sanguosha->getCard(id)->isKindOf("Slash") && player->isAlive() && !player->isNude()) {
                int id2= room->askForCardChosen(target, player, "he", "youdi_obtain");
                room->obtainCard(target, id2);
            }
        }
        return false;
    }
};

YJCM2014Package::YJCM2014Package()
    : Package("YJCM2014")
{
    General *zhuhuan = new General(this, "zhuhuan", "wu"); // YJ 311
    zhuhuan->addSkill(new Youdi);
}

ADD_PACKAGE(YJCM2014)
