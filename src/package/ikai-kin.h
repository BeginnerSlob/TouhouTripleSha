#ifndef IKAIKIN_H
#define IKAIKIN_H

#include "package.h"
#include "card.h"

class IkaiKinPackage : public Package{
    Q_OBJECT

public:
    IkaiKinPackage();
};

class IkXinchaoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXinchaoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // IKAIKIN_H