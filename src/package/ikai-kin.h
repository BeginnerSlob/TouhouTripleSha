#ifndef IKAIKIN_H
#define IKAIKIN_H

#include "card.h"
#include "package.h"
#include "skill.h"
#include "touhou-hana.h"

class IkaiKinPackage : public Package
{
    Q_OBJECT

public:
    IkaiKinPackage();
};

class IkXinchaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXinchaoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkSishiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkSishiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ExtraCollateralCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ExtraCollateralCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class ExtraFeintAttackCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ExtraFeintAttackCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class IkQizhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkQizhiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkXianyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXianyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXianyuSlashCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXianyuSlashCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkZangyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkZangyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkHuitaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkHuitaoCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class IkShitieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkShitieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkJiushiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkJiushiCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class IkJiaolian : public TriggerSkill
{
    Q_OBJECT

public:
    IkJiaolian();
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data,
                                    ServerPlayer *&) const;
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &, ServerPlayer *) const;
    virtual bool effect(TriggerEvent, Room *, ServerPlayer *zhangchunhua, QVariant &, ServerPlayer *) const;

protected:
    virtual int getMaxLostHp(ServerPlayer *zhangchunhua) const;
};

class IkZhuyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkZhuyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkMiceCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkMiceCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class IkXingshi : public MasochismSkill
{
    Q_OBJECT

public:
    IkXingshi();
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const;
    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const;

protected:
    int total_point;
};

class IkBingyanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkBingyanCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkDingpinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkDingpinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkQimoDialog : public ThMimengDialog
{
    Q_OBJECT

public:
    static IkQimoDialog *getInstance();

protected:
    explicit IkQimoDialog();
    virtual bool isButtonEnabled(const QString &button_name) const;
};

class IkQimoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkQimoCard();
    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class IkGuanjuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkGuanjuCard();
    void swapEquip(ServerPlayer *first, ServerPlayer *second) const;

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkXiaozuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXiaozuiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkXiaozuiPeachCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXiaozuiPeachCard();

    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class IkAnxuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkAnxuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkMengjingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkMengjingCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkZhizhanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkZhizhanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkZongxuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkZongxuanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkShenxingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkShenxingCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkXiangzhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXiangzhaoCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkYoudanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYoudanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYishenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYishenCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
    static void askForExchangeHand(ServerPlayer *quancong);
};

class IkFansuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkFansuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkShenchiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkShenchiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkJiebiPutCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkJiebiPutCard();
};

class IkJiebiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkJiebiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkLvdongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkLvdongCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkMingceCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkMingceCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkFenshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkFenshiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkSulingDamageCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkSulingDamageCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkSulingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkSulingCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYusuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYusuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkBengyanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkBengyanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // IKAIKIN_H
