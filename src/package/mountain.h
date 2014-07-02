#ifndef _MOUNTAIN_H
#define _MOUNTAIN_H

#include "package.h"
#include "card.h"
#include "generaloverview.h"

class IkHuanshenDialog: public GeneralOverview {
    Q_OBJECT

public:
    IkHuanshenDialog();

public slots:
    void popup();
};

class MountainPackage: public Package {
    Q_OBJECT

public:
    MountainPackage();
};

#endif

