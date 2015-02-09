#include "touhou-story.h"

#include "general.h"

TouhouStoryHulaopassPackage::TouhouStoryHulaopassPackage()
    :Package("touhou-story-hulaopass")
{
    General *story002 = new General(this, "story002", "kami", 8, true, true);
    Q_UNUSED(story002);

    General *story003 = new General(this, "story003", "kami", 4, true, true);
    Q_UNUSED(story003);
}

ADD_PACKAGE(TouhouStoryHulaopass)