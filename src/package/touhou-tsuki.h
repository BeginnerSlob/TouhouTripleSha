#ifndef TOUHOUTSUKI_H
#define TOUHOUTSUKI_H

#include "card.h"
#include "package.h"
#include "skill.h"

class TouhouTsukiPackage : public Package
{
    Q_OBJECT

public:
    TouhouTsukiPackage();
};

class ThYejunCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThYejunCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class ThJinguoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThJinguoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThKuangqiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThKuangqiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThXushiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThXushiCard();

    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class ThLianhuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThLianhuaCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThShennaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThShennaoCard();

    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ThHeiguanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThHeiguanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThKanyaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThKanyaoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThExiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThExiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThGuixuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThGuixuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThShenbaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThShenbaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif
