#ifndef TOUHOUHANA_H
#define TOUHOUHANA_H

#include "package.h"
#include "card.h"
#include "skill.h"

class TouhouHanaPackage: public Package {
    Q_OBJECT

public:
    TouhouHanaPackage();
};

class ThJiewuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThJiewuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThWujianCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThWujianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThXihuaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThXihuaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#include <QGroupBox>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>

class ThMimengDialog: public QDialog {
    Q_OBJECT

public:
    static ThMimengDialog *getInstance(const QString &object, bool left = true, bool right = true,
                                       bool play_only = true, bool slash_combined = false, bool delayed_tricks = false);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

protected:
    explicit ThMimengDialog(const QString &object, bool left = true, bool right = true,
                            bool play_only = true, bool slash_combined = false, bool delayed_tricks = false);
    virtual bool isButtonEnabled(const QString &button_name) const;

private:
    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;
    bool play_only; // whether the dialog will pop only during the Play phase
    bool slash_combined; // create one 'Slash' button instead of 'Slash', 'Fire Slash', 'Thunder Slash'
    bool delayed_tricks; // whether buttons of Delayed Tricks will be created

signals:
    void onButtonClick();
};

class ThMimengCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThMimengCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ThQuanshanGiveCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThQuanshanGiveCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThQuanshanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThQuanshanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThDuanzuiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThDuanzuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThZheyinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThZheyinCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYachuiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYachuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThDujiaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThDujiaCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThXianfaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThXianfaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThShengzhiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThShengzhiCard();
};

class ThLiuzhenCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLiuzhenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif