#ifndef TOUHOUKAZE_H
#define TOUHOUKAZE_H

#include "card.h"
#include "package.h"
#include "skill.h"

class TouhouKazePackage : public Package
{
    Q_OBJECT

public:
    TouhouKazePackage();
};

class ThJiyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThJiyiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThNiankeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThNiankeCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThEnanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThEnanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThMicaiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThMicaiCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThQiaogongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThQiaogongCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThQianyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThQianyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThHuosuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThHuosuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThKunyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThKunyiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThCannveCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThCannveCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThGelongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThGelongCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThDasuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThDasuiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThSuilunCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThSuilunCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThRansangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThRansangCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThYanxingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThYanxingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThMaihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThMaihuoCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ThSangzhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThSangzhiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ThXinhuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ThXinhuaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif
