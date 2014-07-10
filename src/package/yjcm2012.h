#ifndef _YJCM2012_H
#define _YJCM2012_H

#include "package.h"
#include "card.h"
#include "wind.h"

#include <QMutex>
#include <QGroupBox>
#include <QAbstractButton>

class YJCM2012Package: public Package {
    Q_OBJECT

public:
    YJCM2012Package();
};

#endif

