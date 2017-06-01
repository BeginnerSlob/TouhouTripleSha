#ifndef _STANDARD_SKILLCARDS_H
#define _STANDARD_SKILLCARDS_H

#include "card.h"
#include "skill.h"

class LianyingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LianyingCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YijiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YijiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif
