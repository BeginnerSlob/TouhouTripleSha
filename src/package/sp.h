#ifndef _SP_H
#define _SP_H

#include "card.h"
#include "package.h"
#include "standard.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QVBoxLayout>

class SPPackage : public Package
{
    Q_OBJECT

public:
    SPPackage();
};

class OLPackage : public Package
{
    Q_OBJECT

public:
    OLPackage();
};

class TaiwanSPPackage : public Package
{
    Q_OBJECT

public:
    TaiwanSPPackage();
};

class MiscellaneousPackage : public Package
{
    Q_OBJECT

public:
    MiscellaneousPackage();
};

class QingyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhoufuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhoufuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ShefuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShefuCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#include "touhou-hana.h"
class ShefuDialog : public ThMimengDialog
{
    Q_OBJECT

public:
    static ShefuDialog *getInstance(const QString &object);

protected:
    explicit ShefuDialog(const QString &object);
    virtual bool isButtonEnabled(const QString &button_name) const;
};

class BenyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BenyuCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HegemonySPPackage : public Package
{
    Q_OBJECT

public:
    HegemonySPPackage();
};

#endif
