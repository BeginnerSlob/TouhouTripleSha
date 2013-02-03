#ifndef STANDARDSKILLCARDS_H
#define STANDARDSKILLCARDS_H

#include "skill.h"
#include "card.h"

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

class TiansuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TiansuoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LuoyiCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LuoyiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TuxiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TuxiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
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

class BisuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE BisuoCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Nvelian: public TriggerSkill{
    Q_OBJECT

public:
    Nvelian(const QString &name, int n);
    virtual bool trigger(TriggerEvent event,  Room* room, ServerPlayer *player, QVariant &data) const;
    virtual QString getEffectName() const;

private:
    int n;
};

class GuidengCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE GuidengCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class XuanhuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE XuanhuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
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

class BianshengCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE BianshengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
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

class JibanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JibanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
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

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
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
/*class KurouCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE KurouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};*/

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
