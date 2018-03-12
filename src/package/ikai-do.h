#ifndef IKAIDO_H
#define IKAIDO_H

#include "card.h"
#include "package.h"
#include "skill.h"

class IkaiDoPackage : public Package
{
    Q_OBJECT

public:
    IkaiDoPackage();
};

class IkShenaiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkShenaiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXinqiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXinqiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkXinqiViewAsSkill : public ZeroCardViewAsSkill
{
    Q_OBJECT

public:
    IkXinqiViewAsSkill();

    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const;
    virtual const Card *viewAs() const;

private:
    static bool hasKazeGenerals(const Player *player);
};

#include <QButtonGroup>
#include <QCommandLinkButton>
#include <QVBoxLayout>
class IkChilianDialog : public SkillDialog
{
    Q_OBJECT

public:
    static IkChilianDialog *getInstance();

public slots:
    virtual void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit IkChilianDialog();

    QButtonGroup *group;
    QVBoxLayout *button_layout;

    QHash<QString, const Card *> map;
};

class IkXingyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXingyuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkChibaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkChibaoCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYumeng : public MasochismSkill
{
    Q_OBJECT

public:
    IkYumeng();
    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const;

protected:
    int n;
};

class IkZhihengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkZhihengCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkGuisiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkGuisiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkQinghuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkQinghuaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkKurouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkKurouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkGuidengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkGuidengCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkWanmeiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkWanmeiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXuanhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkXuanhuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYuanheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYuanheCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkHuanluCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkHuanluCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkZiqiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkZiqiangCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkWudiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkWudiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkQingguoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkQingguoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkQingnangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkQingnangCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYaogeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IkYaogeCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // IKAIDO_H
