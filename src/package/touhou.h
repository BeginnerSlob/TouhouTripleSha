#ifndef TOUHOU_H
#define TOUHOU_H

#include "package.h"
#include "skill.h"
#include "card.h"

class TouhouPackage: public Package{
    Q_OBJECT

public:
    TouhouPackage();
    void addKazeGenerals();
    void addHanaGenerals();
    void addYukiGenerals();
    void addTsukiGenerals();
};

class ThJiyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThJiyiCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThNianxieCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThNianxieCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThEnanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThEnanCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThBishaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThBishaCard();
    
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThHuosuiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThHuosuiCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThKunyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThKunyiCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThCannueCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThCannueCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThGelongCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThGelongCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThDasuiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThDasuiCard();
    
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYanlunCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYanlunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYanxingCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYanxingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThMaihuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThMaihuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThJiewuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThJiewuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThMopaoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThMopaoCard();

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

class ThMimengDialog: public QDialog{
    Q_OBJECT

public:
    static ThMimengDialog *getInstance(const QString &object);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit ThMimengDialog(const QString &object);

    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

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

    virtual const Card *validate(const CardUseStruct *card_use) const;
    virtual const Card *validateInResposing(ServerPlayer *user, bool &continuable) const;
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

class ThMengyaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThMengyaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThShijieCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThShijieCard();
    
    virtual const Card *validate(const CardUseStruct *card_use) const;
    virtual const Card *validateInResposing(ServerPlayer *user, bool &continuable) const;
};

class ThLiuzhenCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLiuzhenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThYuanqiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYuanqiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThChouceCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThChouceCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(const CardUseStruct *card_use) const;
};

class ThBingpuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThBingpuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThHuanfaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ThHuanfaCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThDongmoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThDongmoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThKujieCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThKujieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThChuanshangCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThChuanshangCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLingdieCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLingdieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThFuyueCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThFuyueCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYewangCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYewangCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class ThJinguoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThJinguoCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThKaiyunCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThKaiyunCard();
    
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLianhuaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLianhuaCard();
    
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThShennaoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThShennaoCard();
    
    virtual const Card *validate(const CardUseStruct *card_use) const;
    virtual const Card *validateInResposing(ServerPlayer *user, bool &continuable) const;
};

class ThHeiguanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThHeiguanCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThMiquCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThMiquCard();
    
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThExiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThExiCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThTianqueCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThTianqueCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThShenbaoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThShenbaoCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // TOUHOU_H