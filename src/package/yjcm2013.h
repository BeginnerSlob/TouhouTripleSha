#ifndef _YJCM2013_H
#define _YJCM2013_H

#include "package.h"
#include "card.h"

class YJCM2013Package: public Package {
    Q_OBJECT

public:
    YJCM2013Package();
};

class MiejiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE MiejiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class FenchengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE FenchengCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DanshouCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE DanshouCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif
