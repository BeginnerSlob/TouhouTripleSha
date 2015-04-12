#ifndef TOUHOUSP_H
#define TOUHOUSP_H

#include "package.h"
#include "card.h"
#include "standard.h"

class TouhouSPPackage: public Package{
    Q_OBJECT

public:
    TouhouSPPackage();
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

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class ThFenglingCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThFenglingCard();
};

class ThYingshiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYingshiCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class ThXuyouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThXuyouCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class ThJingyuanspCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThJingyuanspCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // TOUHOUSP_H
