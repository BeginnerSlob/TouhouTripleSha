#include "standard-skillcards.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "room.h"
#include "standard.h"

LianyingCard::LianyingCard()
{
}

bool LianyingCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    return targets.length() < Self->getMark("lianying");
}

void LianyingCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->drawCards(1, "lianying");
}

YijiCard::YijiCard()
{
    mute = true;
}

bool YijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self)
        return false;
    if (Self->getHandcardNum() == 1)
        return targets.isEmpty();
    else
        return targets.length() < 2;
}

void YijiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *target, targets) {
        if (!source->isAlive() || source->isKongcheng())
            break;
        if (!target->isAlive())
            continue;
        int max = qMin(2, source->getHandcardNum());
        if (source->getHandcardNum() == 2 && targets.length() == 2 && targets.last()->isAlive() && target == targets.first())
            max = 1;
        const Card *dummy = room->askForExchange(source, "yiji", max, 1, false, "YijiGive::" + target->objectName());
        target->addToPile("yiji", dummy, false);
        delete dummy;
    }
}
