#ifndef _YJCM_H
#define _YJCM_H

#include "package.h"
#include "card.h"
#include "skill.h"

class YJCMPackage: public Package {
    Q_OBJECT

public:
    YJCMPackage();
};

class MingceCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE MingceCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class XianzhenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE XianzhenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif

