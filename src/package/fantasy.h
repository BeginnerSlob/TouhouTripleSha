#ifndef _FANTASY_H
#define _FANTASY_H

#include "standard.h"

class IbukiGourd : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE IbukiGourd(Card::Suit suit = Spade, int number = 1);
};

class IceSword : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE IceSword(Card::Suit suit = Spade, int number = 2);
};

class LureTiger : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LureTiger(Card::Suit suit, int number);

    virtual QString getSubtype() const;

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ScrollCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ScrollCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Scroll : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE Scroll(Card::Suit suit = Spade, int number = 7);
};

class KnownBoth : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE KnownBoth(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JadeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JadeCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Jade : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE Jade(Card::Suit suit = Spade, int number = 13);

    virtual void onUninstall(ServerPlayer *player) const;
};

class PurpleSong : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE PurpleSong(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void takeEffect(ServerPlayer *target) const;
};

class IronArmor : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE IronArmor(Card::Suit suit = Heart, int number = 2);
};

class MoonSpear : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE MoonSpear(Card::Suit suit = Heart, int number = 4);
};

class BurningCamps : public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE BurningCamps(Card::Suit suit = Heart, int number = 13);

    virtual bool isAvailable(const Player *player) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Breastplate : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Breastplate(Card::Suit suit = Club, int number = 1);
};

class RenwangShield : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE RenwangShield(Card::Suit suit = Club, int number = 2);
};

class Drowning : public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE Drowning(Card::Suit suit = Club, int number = 13);

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class WoodenOxCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WoodenOxCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class WoodenOx : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE WoodenOx(Card::Suit suit = Diamond, int number = 4);

    virtual void onUninstall(ServerPlayer *player) const;
};

class FantasyPackage : public Package
{
    Q_OBJECT

public:
    FantasyPackage();
};

#endif // _FANTASY_H
