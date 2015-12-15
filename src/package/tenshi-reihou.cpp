#include "tenshi-reihou.h"
#include "general.h"
#include "skill.h"
#include "standard.h"

class RhFangcun : public OneCardViewAsSkill
{
public:
    RhFangcun() : OneCardViewAsSkill("rhfangcun")
    {
        filter_pattern = "^TrickCard|black";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Dismantlement *dis = new Dismantlement(originalCard->getSuit(), originalCard->getNumber());
        dis->addSubcard(originalCard);
        dis->setSkillName(objectName());
        return dis;
    }
};

class RhHuifu : public ProhibitSkill
{
public:
    RhHuifu() : ProhibitSkill("rhhuifu")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(objectName()) && (card->isKindOf("Dismantlement") || card->isKindOf("Snatch"));
    }
};

TenshiReihouPackage::TenshiReihouPackage()
    :Package("tenshi-reihou")
{
    General *reihou005 = new General(this, "reihou005", "rei", 4, true, true);
    reihou005->addSkill(new RhFangcun);
    reihou005->addSkill(new RhHuifu);
}

ADD_PACKAGE(TenshiReihou)
