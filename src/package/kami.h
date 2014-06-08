#ifndef KAMI_H
#define KAMI_H

#include "package.h"
#include "card.h"

class KamiPackage : public Package{
    Q_OBJECT

public:
    KamiPackage();
};

class ThShenfengCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThShenfengCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThGugaoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThGugaoCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLeshiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLeshiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYouyaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYouyaCard();
    
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThJinluCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThJinluCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThChuangxinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThChuangxinCard();
    
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThTianxinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThTianxinCard();
    
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThBaihunCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThBaihunCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThBingzhangCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThBingzhangCard();
};

class ThSiqiangCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThSiqiangCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThJiefuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThJiefuCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // KAMI_H