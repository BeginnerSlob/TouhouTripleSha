#include "achievement.h"

class ArchieveSkill : public TriggerSkill
{
public:
    ArchieveSkill(QString objectName)
        : TriggerSkill("#achievement_" + objectName)
    {
        key = objectName;
        frequency = Compulsory;
        global = true;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 0;
    }

    QString key;
};

class NDXS : public ArchieveSkill
{
public:
    NDXS()
        : ArchieveSkill("ndxs")
    {
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->addAchievementData(player, key, 1, false);
        if (room->getAchievementData(player, key, false) >= 3) {
            room->sendCompulsoryTriggerLog(player, objectName());
        }
        return false;
    }
};

AchievementPackage::AchievementPackage()
    : Package("achievement", SpecialPack)
{
    skills << new NDXS;
}

ADD_PACKAGE(Achievement)
