#ifndef IKAISUI_H
#define IKAISUI_H

#include "package.h"
#include "card.h"

class IkaiSuiPackage : public Package{
    Q_OBJECT

public:
    IkaiSuiPackage();
};

class IkXielunCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXielunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#include <QGroupBox>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QVBoxLayout>

class IkShengzunDialog: public QDialog {
    Q_OBJECT

public:
    static IkShengzunDialog *getInstance();

public slots:
    void popup();
    void selectSkill(QAbstractButton *button);

private:
    explicit IkShengzunDialog();

    QAbstractButton *createSkillButton(const QString &skill_name);
    QButtonGroup *group;
    QVBoxLayout *button_layout;

signals:
    void onButtonClick();
};

class IkJuechongCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJuechongCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkMoqiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkMoqiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkTianbeiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkTianbeiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkDuanmengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkDuanmengCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkXinbanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXinbanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkShenyuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkShenyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkFenxunCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkFenxunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkHongrouCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkHongrouCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // IKAISUI_H