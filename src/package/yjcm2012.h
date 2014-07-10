#ifndef _YJCM2012_H
#define _YJCM2012_H

#include "package.h"
#include "card.h"
#include "wind.h"

#include <QMutex>
#include <QGroupBox>
#include <QAbstractButton>

class YJCM2012Package: public Package {
    Q_OBJECT

public:
    YJCM2012Package();
};

class GongqiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE GongqiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JiefanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JiefanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif

