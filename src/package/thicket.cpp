#include "thicket.h"
#include "general.h"
#include "skill.h"
#include "room.h"
#include "carditem.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"

HaoshiCard::HaoshiCard(){
    will_throw = false;
    mute = true;
}

bool HaoshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select == Self)
        return false;

    return to_select->getHandcardNum() == Self->getMark("haoshi");
}

void HaoshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *beggar = targets.first();

    room->moveCardTo(this, beggar, Player::PlaceHand, false);
    room->setEmotion(beggar, "draw-card");
}

class HaoshiViewAsSkill: public ViewAsSkill{
public:
    HaoshiViewAsSkill():ViewAsSkill("haoshi"){

    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(to_select->isEquipped())
            return false;

        int length = Self->getHandcardNum() / 2;
        return selected.length() < length;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.length() != Self->getHandcardNum() / 2)
            return NULL;

        HaoshiCard *card = new HaoshiCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@haoshi!";
    }
};

class HaoshiGive: public PhaseChangeSkill{
public:
    HaoshiGive():PhaseChangeSkill("#haoshi-give"){

    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool onPhaseChange(ServerPlayer *lusu) const{
        if(lusu->getPhase() == Player::Draw && lusu->hasFlag("haoshi")){
            lusu->setFlags("-haoshi");

            Room *room = lusu->getRoom();
            if(lusu->getHandcardNum() <= 5)
                return false;            

            QList<ServerPlayer *> other_players = room->getOtherPlayers(lusu);
            int least = 1000;
            foreach(ServerPlayer *player, other_players)
                least = qMin(player->getHandcardNum(), least);
            room->setPlayerMark(lusu, "haoshi", least);
            bool used = room->askForUseCard(lusu, "@@haoshi!", "@haoshi");

            if(!used){
                // force lusu to give his half cards
                ServerPlayer *beggar = NULL;
                foreach(ServerPlayer *player, other_players){
                    if(player->getHandcardNum() == least){
                        beggar = player;
                        break;
                    }
                }

                int n = lusu->getHandcardNum()/2;
                QList<int> to_give = lusu->handCards().mid(0, n);
                HaoshiCard *haoshi_card = new HaoshiCard;
                foreach(int card_id, to_give)
                    haoshi_card->addSubcard(card_id);
                QList<ServerPlayer *> targets;
                targets << beggar;
                haoshi_card->use(room, lusu, targets);
                delete haoshi_card;
            }
        }

        return false;
    }
};

class Haoshi: public DrawCardsSkill{
public:
    Haoshi():DrawCardsSkill("#haoshi"){

    }

    virtual int getDrawNum(ServerPlayer *lusu, int n) const{
        Room *room = lusu->getRoom();
        if(room->askForSkillInvoke(lusu, "haoshi")){
            room->broadcastSkillInvoke("haoshi");
            lusu->setFlags("haoshi");
            return n + 2;
        }else
            return n;
    }
};

DimengCard::DimengCard(){
    once = true;
}

bool DimengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select == Self)
        return false;

    if(targets.isEmpty())
        return true;

    if(targets.length() == 1){
        int max_diff = Self->getCardCount(true);
        return max_diff >= qAbs(to_select->getHandcardNum() - targets.first()->getHandcardNum());
    }

    return false;
}

bool DimengCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void DimengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *a = targets.at(0);
    ServerPlayer *b = targets.at(1);

    int n1 = a->getHandcardNum();
    int n2 = b->getHandcardNum();

    int diff = qAbs(n1 - n2);
    if(diff != 0){
        room->askForDiscard(source, "dimeng", diff, diff, false, true);
    }

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1;
    move1.card_ids = a->handCards();
    move1.to = b;
    move1.to_place = Player::PlaceHand;
    CardsMoveStruct move2;
    move2.card_ids = b->handCards();
    move2.to = a;
    move2.to_place = Player::PlaceHand;
    exchangeMove.push_back(move1);
    exchangeMove.push_back(move2);

    room->moveCards(exchangeMove, false);

    LogMessage log;
    log.type = "#Dimeng";
    log.from = a;
    log.to << b;
    log.arg = QString::number(n1);
    log.arg2 = QString::number(n2);
    room->sendLog(log);
    room->getThread()->delay();
}

class Dimeng: public ZeroCardViewAsSkill{
public:
    Dimeng():ZeroCardViewAsSkill("dimeng"){

    }

    virtual const Card *viewAs() const{
        return new DimengCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("DimengCard");
    }
};

class Jiuchi: public OneCardViewAsSkill{
public:
    Jiuchi():OneCardViewAsSkill("jiuchi"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.contains("analeptic");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped() && to_select->getSuit() == Card::Spade;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());

        return analeptic;
    }
};

class Roulin: public TriggerSkill{
public:
    Roulin():TriggerSkill("roulin"){
        events << SlashProceed;

        frequency = Compulsory;
    }

    const Card *askForDoubleJink(ServerPlayer *player, ServerPlayer *slasher, const QString &reason) const{
        Room *room = player->getRoom();

        const Card *first_jink = NULL, *second_jink = NULL;
        first_jink = room->askForCard(player, "jink", QString("@%1-jink-1").arg(reason), QVariant(), CardUsed, slasher);
        if(first_jink)
            second_jink = room->askForCard(player, "jink", QString("@%1-jink-2").arg(reason), QVariant(), CardUsed, slasher);

        Card *jink = NULL;
        if(first_jink && second_jink){
            jink = new DummyCard;
            jink->addSubcard(first_jink);
            jink->addSubcard(second_jink);
        }

        return jink;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && (target->hasSkill(objectName()) || target->isFemale());
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if(effect.from->hasSkill(objectName()) && effect.to->isFemale()){
            // dongzhuo slash female
            ServerPlayer *female = effect.to;

            room->broadcastSkillInvoke(objectName(), 1);

            room->slashResult(effect, askForDoubleJink(female, effect.from, "roulin1"));
            return true;

        }else if(effect.from->isFemale() && effect.to->hasSkill(objectName())){
            // female slash dongzhuo
            ServerPlayer *dongzhuo = effect.to;

            int index = effect.drank ? 3 : 2;
            room->broadcastSkillInvoke(objectName(), index);
            room->slashResult(effect, askForDoubleJink(dongzhuo, effect.from, "roulin2"));

            return true;
        }

        return false;
    }
};

class Benghuai: public PhaseChangeSkill{
public:
    Benghuai():PhaseChangeSkill("benghuai"){
        frequency = Compulsory;
    }

    virtual QString getDefaultChoice(ServerPlayer *player) const{
        if(player->getMaxHp() >= player->getHp() + 2)
            return "maxhp";
        else
            return "hp";
    }

    virtual bool onPhaseChange(ServerPlayer *dongzhuo) const{
        bool trigger_this = false;
        Room *room = dongzhuo->getRoom();

        if(dongzhuo->getPhase() == Player::Finish){
            QList<ServerPlayer *> players = room->getOtherPlayers(dongzhuo);
            foreach(ServerPlayer *player, players){
                if(dongzhuo->getHp() > player->getHp()){
                    trigger_this = true;
                    break;
                }
            }
        }

        if(trigger_this){
            QString result = room->askForChoice(dongzhuo, "benghuai", "hp+maxhp");

            int index = dongzhuo->isFemale() ? 2: 1;
            room->broadcastSkillInvoke(objectName(), index);
            room->setEmotion(dongzhuo, "bad");

            LogMessage log;
            log.from = dongzhuo;
            log.arg = objectName();
            log.type = "#TriggerSkill";
            room->sendLog(log);
            if(result == "hp")
                room->loseHp(dongzhuo);
            else
                room->loseMaxHp(dongzhuo);
        }

        return false;
    }
};

class Baonue: public TriggerSkill{
public:
    Baonue():TriggerSkill("baonue$"){
        events << Damage << PreHpReduced;
        //view_as_skill = new DummyViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(triggerEvent == PreHpReduced && damage.from)
            damage.from->tag["InvokeBaonue"] = damage.from->getKingdom() == "qun";
        else if (triggerEvent == Damage && player->tag.value("InvokeBaonue", false).toBool())
        {
            QList<ServerPlayer *> dongzhuos;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if(p->hasLordSkill(objectName())){
                    dongzhuos << p;
                }
            }

            while(!dongzhuos.isEmpty()){
                if(player->askForSkillInvoke(objectName())){
                    ServerPlayer *dongzhuo = room->askForPlayerChosen(player, dongzhuos, objectName());
                    dongzhuo->setFlags("baonueused");           //for AI
                    dongzhuos.removeOne(dongzhuo);
                    JudgeStruct judge;
                    judge.pattern = QRegExp("(.*):(spade):(.*)");
                    judge.good = true;
                    judge.reason = objectName();
                    judge.who = player;

                    room->judge(judge);

                    if(judge.isGood()){
                        room->broadcastSkillInvoke(objectName());

                        RecoverStruct recover;
                        recover.who = player;
                        room->recover(dongzhuo, recover);
                    }
                }else
                    break;
            }

            foreach(ServerPlayer *dongzhuo, room->getAllPlayers()){        //for AI
                if(dongzhuo->hasFlag("baonueused"))
                    dongzhuo->setFlags("-baonueused");
            }
        }
        return false;
    }
};

ThicketPackage::ThicketPackage()
    :Package("thicket")
{
    /*General *xuhuang, *caopi, *menghuo, *zhurong, *sunjian, *lusu, *dongzhuo, *jiaxu;

    xuhuang = new General(this, "xuhuang", "wei");
    xuhuang->addSkill(new Duanliang);

    caopi = new General(this, "caopi$", "wei", 3);
    caopi->addSkill(new Xingshang);
    caopi->addSkill(new Fangzhu);
    caopi->addSkill(new Songwei);

    lusu = new General(this, "lusu", "wu", 3);
    lusu->addSkill(new Haoshi);
    lusu->addSkill(new HaoshiViewAsSkill);
    lusu->addSkill(new HaoshiGive);
    lusu->addSkill(new Dimeng);
    related_skills.insertMulti("haoshi", "#haoshi");
    related_skills.insertMulti("haoshi", "#haoshi-give");

    dongzhuo = new General(this, "dongzhuo$", "qun", 8);
    dongzhuo->addSkill(new Jiuchi);
    dongzhuo->addSkill(new Roulin);
    dongzhuo->addSkill(new Benghuai);
    dongzhuo->addSkill(new Baonue);

    addMetaObject<DimengCard>();
    addMetaObject<HaoshiCard>();*/
}

ADD_PACKAGE(Thicket)
