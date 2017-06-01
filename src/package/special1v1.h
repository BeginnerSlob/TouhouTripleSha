#ifndef _SPECIAL1V1_H
#define _SPECIAL1V1_H

#include "card.h"
#include "package.h"
#include "skill.h"
#include "standard.h"

class XiechanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiechanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class CangjiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE CangjiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class PujiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PujiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Special1v1Package : public Package
{
    Q_OBJECT

public:
    Special1v1Package();
};

class New1v1CardPackage : public Package
{
    Q_OBJECT

public:
    New1v1CardPackage();
};

#endif
