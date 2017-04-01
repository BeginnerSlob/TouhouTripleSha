#ifndef TENSHIREIHOU_H
#define TENSHIREIHOU_H

#include "package.h"
#include "card.h"

class RhDuanlongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhDuanlongCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class RhRuyiCard: public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhRuyiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class TenshiReihouPackage : public Package
{
    Q_OBJECT

public:
    TenshiReihouPackage();
};


#endif // TENSHIREIHOU_H

