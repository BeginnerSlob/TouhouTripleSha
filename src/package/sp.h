#ifndef _SP_H
#define _SP_H

#include "package.h"
#include "card.h"
#include "standard.h"

#include <QGroupBox>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QVBoxLayout>

class SPPackage: public Package {
    Q_OBJECT

public:
    SPPackage();
};

class OLPackage: public Package {
    Q_OBJECT

public:
    OLPackage();
};

class TaiwanSPPackage: public Package {
    Q_OBJECT

public:
    TaiwanSPPackage();
};

class MiscellaneousPackage: public Package {
    Q_OBJECT

public:
    MiscellaneousPackage();
};

class YinbingCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE YinbingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QingyiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE QingyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhoufuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ZhoufuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SPCardPackage: public Package {
    Q_OBJECT

public:
    SPCardPackage();
};

class SPMoonSpear: public Weapon {
    Q_OBJECT

public:
    Q_INVOKABLE SPMoonSpear(Card::Suit suit = Diamond, int number = 12);
};

class HegemonySPPackage: public Package {
    Q_OBJECT

public:
    HegemonySPPackage();
};

#endif

