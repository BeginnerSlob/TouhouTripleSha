#ifndef IKAITSUCHI_H
#define IKAITSUCHI_H

#include "package.h"
#include "card.h"
#include "skill.h"

class IkaiTsuchiPackage : public Package{
    Q_OBJECT

public:
    IkaiTsuchiPackage();
};

class IkShenaiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkShenaiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkXinqiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXinqiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkXinqiViewAsSkill: public ZeroCardViewAsSkill {
    Q_OBJECT

public:
    IkXinqiViewAsSkill();

    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const;
    virtual const Card *viewAs() const;

private:
    static bool hasKazeGenerals(const Player *player);
};

class IkLianbaoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkLianbaoCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYumeng: public MasochismSkill {
    Q_OBJECT

public:
    IkYumeng();
    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const;

protected:
    int n;
};

class IkZhihengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkZhihengCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkGuisiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkGuisiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkQinghuaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkQinghuaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkKurouCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkKurouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkGuidengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkGuidengCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkWanmeiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkWanmeiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkXuanhuoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkXuanhuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkYuanheCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkYuanheCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkYuluCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkYuluCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class IkWudiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkWudiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class IkMoyuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkMoyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class IkQingnangCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE IkQingnangCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // IKAITSUCHI_H