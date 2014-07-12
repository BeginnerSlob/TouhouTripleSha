#ifndef IKAIKA_H
#define IKAIKA_H

#include "package.h"
#include "card.h"

class IkaiKaPackage : public Package{
    Q_OBJECT

public:
    IkaiKaPackage();
};

class IkZhijuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkZhijuCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // IKAIKA_H