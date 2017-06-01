#ifndef _CHUNXUE_SCENARIO_H
#define _CHUNXUE_SCENARIO_H

#include "card.h"
#include "scenario.h"

class CxQiuwenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE CxQiuwenCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ChunxueScenario : public Scenario
{
    Q_OBJECT

public:
    ChunxueScenario();

    virtual void onTagSet(Room *room, const QString &key) const;
};

#endif
