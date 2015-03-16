#ifndef _JIANNIANG_SCENARIO_H
#define _JIANNIANG_SCENARIO_H

#include "scenario.h"

class ServerPlayer;

class IkTanyanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkTanyanCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class JianniangScenario: public Scenario {
    Q_OBJECT

public:
    explicit JianniangScenario();

    virtual void assign(QStringList &generals, QStringList &roles) const;
    virtual int getPlayerCount() const;
    virtual QString getRoles() const;
    virtual void onTagSet(Room *room, const QString &key) const;
};

#endif
