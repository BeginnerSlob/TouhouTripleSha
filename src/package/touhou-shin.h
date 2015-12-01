#ifndef TOUHOUSHIN_H
#define TOUHOUSHIN_H

#include "package.h"
#include "card.h"

class TouhouShinPackage : public Package{
    Q_OBJECT

public:
    TouhouShinPackage();
};

class ThLuanshenCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLuanshenCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLianyingCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLianyingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // TOUHOUSHIN_H