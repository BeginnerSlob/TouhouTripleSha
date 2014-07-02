#ifndef IKAIKI_H
#define IKAIKI_H

#include "package.h"
#include "card.h"
#include "skill.h"

class IkaiKiPackage : public Package{
    Q_OBJECT

public:
    IkaiKiPackage();
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

class IkXunyuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXunyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkMancaiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkMancaiCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkQiangxiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkQiangxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYushenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkYushenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkYihuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE IkYihuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkJilveCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJilveCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

#endif // IKAIKI_H