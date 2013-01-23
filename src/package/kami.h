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

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class GongxinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE GongxinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
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

#endif // KAMI_H