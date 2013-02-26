#ifndef STANDARDSKILLCARDS_H
#define STANDARDSKILLCARDS_H

#include "skill.h"
#include "card.h"

class TiaoxinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE TiaoxinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class XuanhuiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE XuanhuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class XinchaoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE XinchaoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Sishi2Card: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Sishi2Card();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class XielunCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE XielunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LiefengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE LiefengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class MiaowuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE MiaowuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TiansuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TiansuoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LianbaoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LianbaoCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LuoyiCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LuoyiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XunyuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE XunyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class MancaiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE MancaiCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QiangxiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE QiangxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YushenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE YushenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JiemingCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JiemingCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class BisuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE BisuoCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class HuanwuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE HuanwuCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class MiceCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE MiceCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(const CardUseStruct *card_use) const;
};

class XinbanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE XinbanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YihuoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE YihuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JilveCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JilveCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class QinghuaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE QinghuaCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class KurouCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE KurouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class GuidengCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE GuidengCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YuanheCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE YuanheCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YuluCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE YuluCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class LiangbanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LiangbanCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JilianCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JilianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QixiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE QixiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhihuiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ZhihuiCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JianmieCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JianmieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class BianshengCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE BianshengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JibanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JibanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class GuanjuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE GuanjuCard();
    void swapEquip(ServerPlayer *first, ServerPlayer *second) const;

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XiaozuiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE XiaozuiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class AnxuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE AnxuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class FenxunCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE FenxunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class MengjingCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE MengjingCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhizhanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ZhizhanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class QingnangCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE QingnangCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class MoyuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE MoyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class WenleCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE WenleCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#include "generaloverview.h"

class HuanshenDialog: public GeneralOverview {
    Q_OBJECT

public:
    HuanshenDialog();

public slots:
    void popup();
};

#include <QGroupBox>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>

class GuhuoDialog: public QDialog {
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

class LvdongCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LvdongCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class LvdongSlashCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LvdongSlashCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class MingceCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE MingceCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class LeijiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LeijiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class TianshiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TianshiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YujiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE YujiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class FunuanCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE FunuanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhihengCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ZhihengCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LingshiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LingshiCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YeyanCard: public SkillCard {
    Q_OBJECT

public:
    void damage(ServerPlayer *player, ServerPlayer *target, int point) const;
};

class GreatYeyanCard: public YeyanCard {
    Q_OBJECT

public:
    Q_INVOKABLE GreatYeyanCard();

    virtual bool targetFilter(const QList<const Player *> &targets,
                              const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select,
                              const Player *Self, int &maxVotes) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SmallYeyanCard: public YeyanCard {
    Q_OBJECT

public:
    Q_INVOKABLE SmallYeyanCard();
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class LiqiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LiqiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LiqiViewAsSkill: public ZeroCardViewAsSkill {
    Q_OBJECT

public:
    LiqiViewAsSkill();

    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const;
    virtual const Card *viewAs() const;

private:
    static bool hasShuGenerals(const Player *player);
};

class JimianCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JimianCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class KouzhuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE KouzhuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JiaojinCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE JiaojinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SuikongCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SuikongCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class TianwuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE TianwuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // STANDARDSKILLCARDS_H
