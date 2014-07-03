#ifndef IKAISUI_H
#define IKAISUI_H

#include "package.h"
#include "card.h"

class IkaiSuiPackage : public Package{
    Q_OBJECT

public:
    IkaiSuiPackage();
};

class IkXielunCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXielunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // IKAISUI_H