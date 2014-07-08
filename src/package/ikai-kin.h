#ifndef IKAIKIN_H
#define IKAIKIN_H

#include "package.h"
#include "card.h"
#include "skill.h"

class IkaiKinPackage : public Package{
    Q_OBJECT

public:
    IkaiKinPackage();
};

class IkXinchaoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXinchaoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkSishiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkSishiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ExtraCollateralCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ExtraCollateralCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class IkQizhiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkQizhiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkXianyuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXianyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXianyuSlashCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXianyuSlashCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkJiushiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJiushiCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class IkNvelian: public TriggerSkill {
    Q_OBJECT

public:
    IkNvelian();
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data, ServerPlayer* &) const;
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &, ServerPlayer *) const;
    virtual bool effect(TriggerEvent, Room *, ServerPlayer *zhangchunhua, QVariant &, ServerPlayer *) const;

protected:
    virtual int getMaxLostHp(ServerPlayer *zhangchunhua) const;
};

class IkZhuyiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkZhuyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // IKAIKIN_H