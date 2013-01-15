#include "touhou.h"

TouhouPackage::TouhouPackage():Package("touhou")
{
    addKazeGenerals();
    addHanaGenerals();
    addYukiGenerals();
    addTsukiGenerals();
}

ADD_PACKAGE(Touhou)