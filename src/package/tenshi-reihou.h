#ifndef TENSHIREIHOU_H
#define TENSHIREIHOU_H

#include "package.h"
#include "card.h"

class RhDuanlongCard : public SkillCard
{
    Q_OBJECT

public:
    RhDuanlongCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TenshiReihouPackage : public Package
{
    Q_OBJECT

public:
    TenshiReihouPackage();
};


#endif // TENSHIREIHOU_H

