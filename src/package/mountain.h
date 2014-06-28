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

class ZhijianCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ZhijianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ZhibaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ZhibaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class FangquanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE FangquanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class HuashenDialog: public GeneralOverview {
    Q_OBJECT

public:
    HuashenDialog();

public slots:
    void popup();
};

class MountainPackage: public Package {
    Q_OBJECT

public:
    MountainPackage();
};

#endif

