#ifndef BANGAI_H
#define BANGAI_H

#include "package.h"
#include "card.h"

class BangaiPackage : public Package{
    Q_OBJECT

public:
    BangaiPackage();
};

class ThShoujuanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThShoujuanCard();
	
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThZushaCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThZushaCard();
	
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYaomeiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThYaomeiCard();
	
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLingzhanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLingzhanCard();
	
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(const CardUseStruct *card_use) const;
    virtual const Card *validateInResposing(ServerPlayer *user, bool &continuable) const;
};

class ThXingxieCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThXingxieCard();
	
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThLuanshenCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ThLuanshenCard();
	
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // BANGAI_H