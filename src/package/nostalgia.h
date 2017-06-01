#ifndef _NOSTALGIA_H
#define _NOSTALGIA_H

#include "card.h"
#include "package.h"
#include "standard-skillcards.h"
#include "standard.h"

class NostalGeneralPackage : public Package
{
    Q_OBJECT

public:
    NostalGeneralPackage();
};

class NostalStandardPackage : public Package
{
    Q_OBJECT

public:
    NostalStandardPackage();
};

class NostalWindPackage : public Package
{
    Q_OBJECT

public:
    NostalWindPackage();
};

class NostalYJCMPackage : public Package
{
    Q_OBJECT

public:
    NostalYJCMPackage();
};

class NostalYJCM2013Package : public Package
{
    Q_OBJECT

public:
    NostalYJCM2013Package();
};

class NosJujianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosJujianCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosXuanhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosXuanhuoCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosRenxinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosRenxinCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NosYexinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosYexinCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class NosTuxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosTuxiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosRendeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosRendeCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NosFanjianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosFanjianCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif
