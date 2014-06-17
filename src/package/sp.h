#ifndef SPPACKAGE_H
#define SPPACKAGE_H

#include "package.h"
#include "card.h"
#include "standard.h"

class SPPackage: public Package{
    Q_OBJECT

public:
    SPPackage();
};

class ThChiyingCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThChiyingCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class ThXuezhongCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThXuezhongCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLunminCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLunminCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThShushuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThShushuCard();
};

class ThYingshiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYingshiCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
};

#endif // SPPACKAGE_H
