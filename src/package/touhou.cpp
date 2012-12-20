#include "touhou.h"

TouhouPackage::TouhouPackage():Package("touhou")
{
    addKazeGenerals();
    addHanaGenerals();
    //addYukiGenerals();
    //addTsukiGenerals();
}