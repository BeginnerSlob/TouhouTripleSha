#ifndef TOUHOUSHIN_H
#define TOUHOUSHIN_H

#include "card.h"
#include "package.h"
#include "skill.h"

class QAbstractButton;
class QButtonGroup;
class QVBoxLayout;

class TouhouShinPackage : public Package
{
    Q_OBJECT

public:
    TouhouShinPackage();
};

class ThLuanshenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThLuanshenCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLianyingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThLianyingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThMumiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThMumiCard();
};

class ThShenmiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThShenmiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ThMuyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThMuyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThNihuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThNihuiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThKuangwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThKuangwuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThDieyingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThDieyingCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThBiyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThBiyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThTunaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThTunaCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThNingguCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThNingguCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThAiminCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThAiminCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThRenmoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThRenmoCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class ThRuizhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThRuizhiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLingweiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThLingweiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class ThMinwangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThMinwangCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class ThYuguangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThYuguangCard();

    static bool CompareBySuit(int card1, int card2);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThCanfeiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThCanfeiCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class ThZuoyongDialog : public SkillDialog
{
    Q_OBJECT

public:
    enum ThChuanyuName
    {
        Jink = 0x1,
        Analeptic = 0x2,
        Nullification = 0x4,
        FireAttack = 0x8,
    };

    static ThZuoyongDialog *getInstance();

    static bool hasNamed(QString name, const Player *player);
    static bool hasAllNamed(const Player *player);

    static const QString &chuanyuNames;
    static QMap<QString, ThChuanyuName> chuanyuMap;

public slots:
    virtual void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit ThZuoyongDialog();

    QButtonGroup *group;
    QVBoxLayout *button_layout;

    QHash<QString, const Card *> map;
};

class ThGuiyuniuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThGuiyuniuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThHaixingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThHaixingCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // TOUHOUSHIN_H
