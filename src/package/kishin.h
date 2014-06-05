#ifndef KISHIN_H
#define KISHIN_H

#include "package.h"
#include "card.h"

class KishinPackage : public Package{
    Q_OBJECT

public:
    KishinPackage();
};

class ThLuanshenCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLuanshenCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLanzouCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThLanzouCard();

    virtual bool targetFilter(const QList<const Player *> &targets,
                              const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select,
                              const Player *Self, int &maxVotes) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

    void discard(ServerPlayer *source, ServerPlayer *target, int num) const;
};

class ThLianyingCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLianyingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // KISHIN_H