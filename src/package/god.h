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

#endif

