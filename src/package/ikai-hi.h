#ifndef IKAIHI_H
#define IKAIHI_H

#include "package.h"
#include "card.h"
#include "skill.h"

class IkaiHiPackage : public Package{
    Q_OBJECT

public:
    IkaiHiPackage();
};

class IkTiaoxinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkTiaoxinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkBaishenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkBaishenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkLiefengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkLiefengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkMiaowuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkMiaowuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkZhihun: public ViewAsSkill {
    Q_OBJECT

public:
    IkZhihun();
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const;
    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *card) const;
    virtual const Card *viewAs(const QList<const Card *> &cards) const;
    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const;
    virtual bool isEnabledAtNullification(const ServerPlayer *player) const;

protected:
    virtual int getEffHp(const Player *zhaoyun) const;
};

#endif // IKAIHI_H