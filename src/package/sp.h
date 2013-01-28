#ifndef SPPACKAGE_H
#define SPPACKAGE_H

#include "package.h"
#include "card.h"
#include "standard.h"

class SPPackage: public Package{
    Q_OBJECT

public:
    SPPackage();
};

/*class SPCardPackage: public Package{
    Q_OBJECT

public:
    SPCardPackage();
};

class SPMoonSpear:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE SPMoonSpear(Card::Suit suit = Diamond, int number = 12);
};*/

#endif // SPPACKAGE_H
