#ifndef TOUHOUSTORY_H
#define TOUHOUSTORY_H

#include "package.h"
#include "card.h"

class TouhouStoryHulaopassPackage : public Package{
    Q_OBJECT

public:
    TouhouStoryHulaopassPackage();
};

/*class ThChayinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThChayinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
}*/

#endif // TOUHOUSTORY_H
