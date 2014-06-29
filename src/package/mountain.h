#ifndef _MOUNTAIN_H
#define _MOUNTAIN_H

#include "package.h"
#include "card.h"
#include "generaloverview.h"

class IkMancaiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkMancaiCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkTiaoxinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkTiaoxinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkJibanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJibanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkBianshengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkBianshengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkBaishenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkBaishenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkHuanshenDialog: public GeneralOverview {
    Q_OBJECT

public:
    IkHuanshenDialog();

public slots:
    void popup();
};

class MountainPackage: public Package {
    Q_OBJECT

public:
    MountainPackage();
};

#endif

