#ifndef TOUHOUBANGAI_H
#define TOUHOUBANGAI_H

#include "card.h"
#include "package.h"

class ThMiqiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThMiqiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

    void discard(ServerPlayer *source, ServerPlayer *target, int num) const;
};

class ThXumeiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThXumeiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThXingxieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThXingxieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThYuboCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThYuboCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThGuijuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThGuijuanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThWangdaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThWangdaoCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThKongxiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThKongxiangCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TouhouBangaiPackage : public Package
{
    Q_OBJECT

public:
    TouhouBangaiPackage();
};

#endif // TOUHOUBANGAI_H
