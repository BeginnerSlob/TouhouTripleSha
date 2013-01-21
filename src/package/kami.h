#ifndef KAMI_H
#define KAMI_H

#include "package.h"
#include "card.h"

class KamiPackage : public Package{
    Q_OBJECT

public:
    KamiPackage();
};

class ThYouyaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYouyaCard();
	
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // KAMI_H