#ifndef _JIANNIANG_SCENARIO_H
#define _JIANNIANG_SCENARIO_H

#include "scenario.h"

class ServerPlayer;

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
