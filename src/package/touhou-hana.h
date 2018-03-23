#ifndef TOUHOUHANA_H
#define TOUHOUHANA_H

#include "card.h"
#include "package.h"
#include "skill.h"

class TouhouHanaPackage : public Package
{
    Q_OBJECT

public:
    TouhouHanaPackage();
};

class ThJiewuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThJiewuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};
/*
class ThBianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThBianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &card_use) const;
};
*/
class ThXihuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThXihuaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCommandLinkButton>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

class ThMimengDialog : public SkillDialog
{
    Q_OBJECT

public:
    static ThMimengDialog *getInstance(const QString &object, bool left = true, bool right = true, bool play_only = true, bool slash_combined = false, bool delayed_tricks = false);

public slots:
    virtual void popup();
    void selectCard(QAbstractButton *button);

protected:
    explicit ThMimengDialog(const QString &object, bool left = true, bool right = true, bool play_only = true, bool slash_combined = false, bool delayed_tricks = false);
    virtual bool isButtonEnabled(const QString &button_name) const;

    QHash<QString, const Card *> map;

private:
    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;

    QString object_name;
    bool play_only; // whether the dialog will pop only during the Play phase
    bool slash_combined; // create one 'Slash' button instead of 'Slash', 'Fire Slash', 'Thunder Slash'
    bool delayed_tricks; // whether buttons of Delayed Tricks will be created
};

class ThMimengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThMimengCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ThQuanshanGiveCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThQuanshanGiveCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThQuanshanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThQuanshanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThDuanzuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThDuanzuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThYingdengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThYingdengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYachuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThYachuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThGuaitanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThGuaitanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThDujiaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThDujiaCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThXianfaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThXianfaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThLeishiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThLeishiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThLiuzhenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThLiuzhenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif
