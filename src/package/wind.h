#ifndef _WIND_H
#define _WIND_H

#include "card.h"
#include "package.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>

class GuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GuhuoCard();
    bool guhuo(ServerPlayer *yuji) const;

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class GuhuoDialog : public QDialog
{
    Q_OBJECT

public:
    static GuhuoDialog *getInstance(const QString &object, bool left = true, bool right = true);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit GuhuoDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

#include "skill.h"
class Jushou : public PhaseChangeSkill
{
    Q_OBJECT

public:
    Jushou();
    virtual bool onPhaseChange(ServerPlayer *target) const;

protected:
    virtual int getJushouDrawNum(ServerPlayer *caoren) const;
};

class WindPackage : public Package
{
    Q_OBJECT

public:
    WindPackage();
};

#endif
