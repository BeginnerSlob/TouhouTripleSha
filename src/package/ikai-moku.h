#ifndef IKAIMOKU_H
#define IKAIMOKU_H

#include "card.h"
#include "package.h"
#include "skill.h"

class IkaiMokuPackage : public Package
{
    Q_OBJECT

public:
    IkaiMokuPackage();
};

class IkHuanghunCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkHuanghunCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class IkSuinieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkSuinieCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkTiaoxinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkTiaoxinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYoujiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYoujiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkLiefengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkLiefengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkMiaowuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkMiaowuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkZhihun : public ViewAsSkill
{
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

class IkXunyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXunyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkMancaiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkMancaiCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkQiangxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkQiangxiCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYushenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYushenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkYihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYihuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkJilveCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkJilveCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class IkZhuoshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkZhuoshiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkYuanjieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYuanjieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkZhihuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkZhihuiCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkJianlveCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkJianlveCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkBianshengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkBianshengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkJibanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkJibanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkLingshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkLingshiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYeyanCard : public SkillCard
{
    Q_OBJECT

public:
    void damage(ServerPlayer *shenzhouyu, ServerPlayer *target, int point) const;
};

class GreatIkYeyanCard : public IkYeyanCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GreatIkYeyanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self,
                              int &maxVotes) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SmallIkYeyanCard : public IkYeyanCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SmallIkYeyanCard();
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXuzhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXuzhaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkWenleCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkWenleCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#include "generaloverview.h"
class IkHuanshenDialog : public GeneralOverview
{
    Q_OBJECT

public:
    IkHuanshenDialog();

public slots:
    virtual void popup();
};

class IkGuihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkGuihuoCard();
    bool ikguihuo(ServerPlayer *yuji) const;

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class IkYujiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYujiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class IkSuikongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkSuikongCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkTianwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkTianwuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // IKAIMOKU_H
