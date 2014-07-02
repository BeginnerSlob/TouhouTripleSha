#ifndef _THICKET_H
#define _THICKET_H

#include "package.h"
#include "card.h"

class ThicketPackage: public Package {
    Q_OBJECT

public:
    ThicketPackage();
};

class IkWenleCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkWenleCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif

