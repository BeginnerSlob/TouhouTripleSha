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

class IkJilunCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJilunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // IKAIKA_H