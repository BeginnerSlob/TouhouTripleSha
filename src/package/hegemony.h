#ifndef _HEGEMONY_H
#define _HEGEMONY_H

#include "standard.h"

class HegemonyPackage : public Package
{
    Q_OBJECT

public:
    HegemonyPackage();
};

class QingchengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingchengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

#endif
