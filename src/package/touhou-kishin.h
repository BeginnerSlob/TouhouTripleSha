#ifndef TOUHOUKISHIN_H
#define TOUHOUKISHIN_H

#include "package.h"
#include "card.h"

class TouhouKishinPackage : public Package{
    Q_OBJECT

public:
    TouhouKishinPackage();
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

#endif // TOUHOUKISHIN_H