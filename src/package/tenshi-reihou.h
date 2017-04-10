#ifndef TENSHIREIHOU_H
#define TENSHIREIHOU_H

#include "package.h"
#include "card.h"

class RhDuanlongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhDuanlongCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class RhRuyiCard: public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhRuyiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class RhHuanjingCard: public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhHuanjingCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class RhPujiuCard: public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhPujiuCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class RhGaimingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhGaimingCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class RhXuanrenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhXuanrenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#include <QDialog>
#include <QButtonGroup>
#include <QVBoxLayout>
class RhHaoqiangDialog: public QDialog
{
    Q_OBJECT

public:
    static RhHaoqiangDialog *getInstance();

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit RhHaoqiangDialog();

    QButtonGroup *group;
    QVBoxLayout *button_layout;

    QHash<QString, const Card *> map;

signals:
    void onButtonClick();
};

class RhHaoqiangCard: public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhHaoqiangCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class RhYarenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhYarenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class RhYinrenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RhYinrenCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class TenshiReihouPackage : public Package
{
    Q_OBJECT

public:
    TenshiReihouPackage();
};


#endif // TENSHIREIHOU_H

