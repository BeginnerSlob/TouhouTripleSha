#include "firepackage.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

QuhuCard::QuhuCard(){
    once = true;
    mute = true;
    will_throw = false;
    handling_method = Card::MethodPindian;
}

bool QuhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select->getHp() <= Self->getHp())
        return false;

    if(to_select->isKongcheng())
        return false;

    return true;
}

void QuhuCard::use(Room *room, ServerPlayer *xunyu, QList<ServerPlayer *> &targets) const{
    ServerPlayer *tiger = targets.first();

    room->broadcastSkillInvoke("quhu");

    bool success = xunyu->pindian(tiger, "quhu", this);
    if(success){
        room->broadcastSkillInvoke("quhu");

        QList<ServerPlayer *> players = room->getOtherPlayers(tiger), wolves;
        foreach(ServerPlayer *player, players){
            if(tiger->inMyAttackRange(player))
                wolves << player;
        }

        if(wolves.isEmpty()){
            LogMessage log;
            log.type = "#QuhuNoWolf";
            log.from = xunyu;
            log.to << tiger;
            room->sendLog(log);

            return;
        }

        room->broadcastSkillInvoke("#tunlang");
        ServerPlayer *wolf = room->askForPlayerChosen(xunyu, wolves, "quhu");

        DamageStruct damage;
        damage.from = tiger;
        damage.to = wolf;

        room->damage(damage);

    }else{
        DamageStruct damage;
        damage.card = NULL;
        damage.from = tiger;
        damage.to = xunyu;

        room->damage(damage);
    }
}

JiemingCard::JiemingCard(){

}

bool JiemingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void JiemingCard::onEffect(const CardEffectStruct &effect) const{
    int upper = qMin(5, effect.to->getMaxHp());
    int x = upper - effect.to->getHandcardNum();
    if(x <= 0)
        return;

    effect.to->drawCards(x);
}

class JiemingViewAsSkill: public ZeroCardViewAsSkill{
public:
    JiemingViewAsSkill():ZeroCardViewAsSkill("jieming"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@jieming";
    }

    virtual const Card *viewAs() const{
        return new JiemingCard;
    }
};

class Jieming: public MasochismSkill{
public:
    Jieming():MasochismSkill("jieming"){
        view_as_skill = new JiemingViewAsSkill;
    }

    virtual void onDamaged(ServerPlayer *xunyu, const DamageStruct &damage) const{
        Room *room = xunyu->getRoom();
        int x = damage.damage, i;
        for(i=0; i<x; i++){
            if(!room->askForUseCard(xunyu, "@@jieming", "@jieming"))
                break;
        }
    }
};

class Quhu: public OneCardViewAsSkill{
public:
    Quhu():OneCardViewAsSkill("quhu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("QuhuCard") && !player->isKongcheng();
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        QuhuCard *card = new QuhuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

FirePackage::FirePackage()
    :Package("fire")
{
    /*General *dianwei, *xunyu, *pangtong, *wolong, *taishici, *yuanshao, *yanliangwenchou, *pangde;

    dianwei = new General(this, "dianwei", "wei");
    dianwei->addSkill(new Qiangxi);

    xunyu = new General(this, "xunyu", "wei", 3);
    xunyu->addSkill(new Quhu);
    xunyu->addSkill(new Jieming);

    addMetaObject<QuhuCard>();
    addMetaObject<JiemingCard>();*/
}

ADD_PACKAGE(Fire);
