#ifndef _GOD_H
#define _GOD_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"

class GodPackage: public Package {
    Q_OBJECT

public:
    GodPackage();
};

class IkLingshiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkLingshiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYeyanCard: public SkillCard {
    Q_OBJECT

public:
    void damage(ServerPlayer *shenzhouyu, ServerPlayer *target, int point) const;
};

class GreatIkYeyanCard: public IkYeyanCard {
    Q_OBJECT

public:
    Q_INVOKABLE GreatIkYeyanCard();

    virtual bool targetFilter(const QList<const Player *> &targets,
                              const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select,
                              const Player *Self, int &maxVotes) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SmallIkYeyanCard: public IkYeyanCard {
    Q_OBJECT

public:
    Q_INVOKABLE SmallIkYeyanCard();
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkTianwubakaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkTianwubakaCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkSuikongCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkSuikongCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYihuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE IkYihuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkJilveCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJilveCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

#endif

