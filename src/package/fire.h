#ifndef _FIRE_H
#define _FIRE_H

#include "package.h"
#include "card.h"

class FirePackage: public Package {
    Q_OBJECT

public:
    FirePackage();
};

class IkYushenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkYushenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkQiangxiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkQiangxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkJianmieCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJianmieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif

