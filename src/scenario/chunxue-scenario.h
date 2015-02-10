#ifndef _CHUNXUE_SCENARIO_H
#define _CHUNXUE_SCENARIO_H

#include "scenario.h"

class ChunxueScenario : public Scenario {
    Q_OBJECT

public:
    ChunxueScenario();

    virtual void onTagSet(Room *room, const QString &key) const;
};

#endif