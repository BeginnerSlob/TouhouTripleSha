#ifndef TOUHOUBANGAI_H
#define TOUHOUBANGAI_H

#include "package.h"
#include "card.h"

class TouhouBangaiPackage : public Package{
    Q_OBJECT

public:
    TouhouBangaiPackage();
};

class ThShoujuanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThShoujuanCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThZushaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThZushaCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThYaomeiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYaomeiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThXingxieCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThXingxieCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYuboCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThYuboCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThGuijuanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThGuijuanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThWangdaoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThWangdaoCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThSixiangCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThSixiangCard();
    
    virtual bool targetFixed() const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // TOUHOUBANGAI_H