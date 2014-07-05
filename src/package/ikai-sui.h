#ifndef IKAISUI_H
#define IKAISUI_H

#include "package.h"
#include "card.h"
#include "skill.h"

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

class IkTianyanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkTianyanCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkCangwuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkCangwuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkChenyan: public TriggerSkill {
    Q_OBJECT

public:
    IkChenyan();
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *yuanshu, QVariant &data, ServerPlayer* &) const;
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *yuanshu, QVariant &data, ServerPlayer *) const;

protected:
    virtual int getKingdoms(ServerPlayer *yuanshu) const;
};

class IkZhangeCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkZhangeCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkShuangrenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkShuangrenCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXincaoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXincaoCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkKouzhuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkKouzhuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkJiaojinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJiaojinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkSheqieCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkSheqieCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // IKAISUI_H