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
