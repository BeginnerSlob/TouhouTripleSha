#ifndef TOUHOUKAMI_H
#define TOUHOUKAMI_H

#include "card.h"
#include "package.h"
#include "skill.h"
#include "standard-equips.h"

class ThShenfengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThShenfengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThGugaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThGugaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLeshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThLeshiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThJingwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThJingwuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYouyaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThYouyaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThJinluCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThJinluCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThChuangxinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThChuangxinCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThTianxinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThTianxinCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThBaihunCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThBaihunCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

#include <QButtonGroup>
#include <QCommandLinkButton>
#include <QDialog>
#include <QVBoxLayout>
class ThLingyunDialog : public SkillDialog
{
    Q_OBJECT

public:
    static ThLingyunDialog *getInstance();

public slots:
    virtual void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit ThLingyunDialog();

    QButtonGroup *group;
    QVBoxLayout *button_layout;

    QHash<QString, const Card *> map;
};

class ThLingyunCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThLingyunCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class ThBingzhangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThBingzhangCard();
};

class ThSiqiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThSiqiangCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThJiefuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThJiefuCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThShuangfengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThShuangfengCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThXianhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThXianhuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThJieshaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThJieshaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TouhouKamiPackage : public Package
{
    Q_OBJECT

public:
    TouhouKamiPackage();
};

class Kuukanken : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Kuukanken(Card::Suit suit, int number);
};

class TouhouKamiCardPackage : public Package
{
    Q_OBJECT

public:
    TouhouKamiCardPackage();
};

#endif // TOUHOUKAMI_H
