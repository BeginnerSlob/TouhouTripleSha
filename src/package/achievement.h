#ifndef ACHIEVEMENT_H
#define ACHIEVEMENT_H

#include "package.h"
#include "skill.h"

class AchieveSkill : public TriggerSkill
{
    Q_OBJECT

public:
    Q_INVOKABLE AchieveSkill(QString objectName);

    virtual int getPriority(TriggerEvent) const;
    virtual void onGameOver(Room *, ServerPlayer *, QVariant &) const;
    void gainAchievement(ServerPlayer *player, Room *room) const;
    QStringList getAchievementTranslations(QString _key = QString()) const;

    QString key;
};

class AchievementPackage : public Package
{
    Q_OBJECT

public:
    AchievementPackage();
};

#endif // ACHIEVEMENT_H
