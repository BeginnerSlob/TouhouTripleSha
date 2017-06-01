#ifndef _JIANNIANG_SCENARIO_H
#define _JIANNIANG_SCENARIO_H

#include "scenario.h"

class ServerPlayer;

class IkTanyanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkTanyanCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkXiashanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXiashanCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class JnMingshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JnMingshiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    void slash(Room *room, ServerPlayer *from, ServerPlayer *to) const;
};

class JnChunsuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JnChunsuCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JnAngongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JnAngongCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JnHuojiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JnHuojiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class JnTaoxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JnTaoxiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JianniangScenario : public Scenario
{
    Q_OBJECT

public:
    explicit JianniangScenario();

    virtual void assign(QStringList &generals, QStringList &roles) const;
    virtual int getPlayerCount() const;
    virtual QString getRoles() const;
    virtual void onTagSet(Room *room, const QString &key) const;
};

#endif
