#ifndef _H_MOMENTUM_H
#define _H_MOMENTUM_H

#include "package.h"
#include "card.h"
#include "skill.h"

class DuanxieCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE DuanxieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class HMomentumPackage: public Package {
    Q_OBJECT

public:
    HMomentumPackage();
};

#endif
