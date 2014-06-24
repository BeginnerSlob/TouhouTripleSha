#ifndef _STANDARD_SKILLCARDS_H
#define _STANDARD_SKILLCARDS_H

#include "skill.h"
#include "card.h"

class IkZhihengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkZhihengCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkShenaiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkShenaiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JieyinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JieyinCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkLianbaoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkLianbaoCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkGuidengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkGuidengCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class KurouCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE KurouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LianyingCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE LianyingCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class LijianCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE LijianCard(bool cancelable = true);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

private:
    bool duel_cancelable;
};

class ChuliCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ChuliCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LiuliCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE LiuliCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkGuisiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkGuisiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkQinghuaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkQinghuaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YijiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE YijiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JianyanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JianyanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class GuoseCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE GuoseCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXinqiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXinqiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkXinqiViewAsSkill: public ZeroCardViewAsSkill {
    Q_OBJECT

public:
    IkXinqiViewAsSkill();

    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const;
    virtual const Card *viewAs() const;

private:
    static bool hasKazeGenerals(const Player *player);
};

#endif
