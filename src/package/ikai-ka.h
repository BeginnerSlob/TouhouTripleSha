#ifndef IKAIKA_H
#define IKAIKA_H

#include "package.h"
#include "card.h"

class IkaiKaPackage : public Package{
    Q_OBJECT

public:
    IkaiKaPackage();
};

class IkZhijuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkZhijuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkJilunCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkJilunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkKangjinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkKangjinCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#include <QDialog>
#include <QVBoxLayout>
#include <QCommandLinkButton>
#include <QButtonGroup>
class SelectSuitDialog: public QDialog {
    Q_OBJECT

public:
    static SelectSuitDialog *getInstance();

public slots:
    void popup();
    void selectSuit(QAbstractButton *button);

private:
    explicit SelectSuitDialog();

    QButtonGroup *group;
    QVBoxLayout *button_layout;

signals:
    void onButtonClick();
};

class IkHunkaoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkHunkaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkHuangshiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkHuangshiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkPaomuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkPaomuCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkDengpoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkDengpoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkQihunCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkQihunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkShidaoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkShidaoCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class IkLinghuiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkLinghuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkDianyanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkDianyanCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkDianyanPutCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkDianyanPutCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkQisiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE IkQisiCard();

    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class IkManwuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE IkManwuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXianlvCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXianlvCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkLianwuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkLianwuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkLianwuDrawCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkLianwuDrawCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXiekeCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXiekeCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

#endif // IKAIKA_H