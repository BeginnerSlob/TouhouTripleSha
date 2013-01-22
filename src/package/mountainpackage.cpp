#include "mountainpackage.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "carditem.h"
#include "generaloverview.h"
#include "clientplayer.h"
#include "client.h"
#include "ai.h"
#include "jsonutils.h"

#include <QCommandLinkButton>

class Tuntian: public DistanceSkill{
public:
    Tuntian():DistanceSkill("tuntian"){
        frequency = NotFrequent;
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if(from->hasSkill(objectName()))
            return -from->getPile("field").length();
        else
            return 0;
    }
};

class TuntianGet: public TriggerSkill{
public:
    TuntianGet():TriggerSkill("#tuntian-get"){
        events << CardsMoveOneTime << FinishJudge << EventLoseSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(TriggerSkill::triggerable(player) && player->getPhase() == Player::NotActive)
        {
            if(triggerEvent == CardsMoveOneTime){
                CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
                if(move->from == player &&
                        (move->from_places.contains(Player::PlaceHand)
                         || move->from_places.contains(Player::PlaceEquip))
                        && player->askForSkillInvoke("tuntian", data)){
                    room->broadcastSkillInvoke("tuntian");
                    JudgeStruct judge;
                    judge.pattern = QRegExp("(.*):(heart):(.*)");
                    judge.good = false;
                    judge.reason = "tuntian";
                    judge.who = player;
                    judge.play_animation = true;
                    room->judge(judge);
                }
            }else if(triggerEvent == FinishJudge){
                JudgeStar judge = data.value<JudgeStar>();
                if(judge->reason == "tuntian" && judge->isGood()){
                    player->addToPile("field", judge->card->getEffectiveId());
                    return true;
                }
            }
        }
        else if (triggerEvent == EventLoseSkill && data.toString() == "tuntian")
            player->clearOnePrivatePile("field");

        return false;
    }
};

class Zaoxian: public PhaseChangeSkill{
public:
    Zaoxian():PhaseChangeSkill("zaoxian"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && PhaseChangeSkill::triggerable(target)
                && target->getPhase() == Player::Start
                && target->getMark("zaoxian") == 0
                && target->getPile("field").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *dengai) const{
        Room *room = dengai->getRoom();

        room->setPlayerMark(dengai, "zaoxian", 1);
        dengai->gainMark("@waked");
        room->loseMaxHp(dengai);

        LogMessage log;
        log.type = "#ZaoxianWake";
        log.from = dengai;
        log.arg = QString::number(dengai->getPile("field").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke("zaoxian");
        room->broadcastInvoke("animate", "lightbox:$zaoxian:4000");
        room->getThread()->delay(4000);

        room->acquireSkill(dengai, "jixi");

        return false;
    }
};

JixiCard::JixiCard(){
    target_fixed = true;
}

void JixiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *dengai = card_use.from;

    QList<int> fields = dengai->getPile("field");
    if(fields.isEmpty())
        return ;

    int card_id;
    if(fields.length() == 1)
        card_id = fields.first();
    else{
        room->fillAG(fields, dengai);
        card_id = room->askForAG(dengai, fields, true, "jixi");
        dengai->invoke("clearAG");

        if(card_id == -1)
            return;
    }

    const Card *card = Sanguosha->getCard(card_id);
    Snatch *snatch = new Snatch(card->getSuit(), card->getNumber());
    snatch->setSkillName("jixi");
    snatch->addSubcard(card_id);

    QList<ServerPlayer *> targets;
    QList<const Player *> empty_list;
    foreach(ServerPlayer *p, room->getAlivePlayers()){
        if(!snatch->targetFilter(empty_list, p, dengai))
            continue;
        if(dengai->distanceTo(p,1) > 1)
            continue;
        if(dengai->isProhibited(p, snatch))
            continue;

        targets << p;
    }

    if(targets.isEmpty())
        return;

    ServerPlayer *target = room->askForPlayerChosen(dengai, targets, "jixi");

    CardUseStruct use;
    use.card = snatch;
    use.from = dengai;
    use.to << target;

    room->useCard(use);
}

class Jixi:public ZeroCardViewAsSkill{
public:
    Jixi():ZeroCardViewAsSkill("jixi"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("field").isEmpty();
    }

    virtual const Card *viewAs() const{
        return new JixiCard;
    }

    virtual Location getLocation() const{
        return Right;
    }
};

TiaoxinCard::TiaoxinCard(){
    once = true;
    mute = true;
}

bool TiaoxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->inMyAttackRange(Self) && to_select != Self;
}

void TiaoxinCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();

    if(effect.from->hasArmorEffect("EightDiagram") || effect.from->hasSkill("shengtang"))
        room->broadcastSkillInvoke("tiaoxin", 3);
    else
        room->broadcastSkillInvoke("tiaoxin", qrand() % 2 + 1);

    if(!room->askForUseSlashTo(effect.to, effect.from, "@tiaoxin-slash:" + effect.from->objectName()) && !effect.to->isNude())
        room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "tiaoxin"), effect.to, effect.from);
}

class Tiaoxin: public ZeroCardViewAsSkill{
public:
    Tiaoxin():ZeroCardViewAsSkill("tiaoxin"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("TiaoxinCard");
    }

    virtual const Card *viewAs() const{
        return new TiaoxinCard;
    }
};

class Zhiji: public PhaseChangeSkill{
public:
    Zhiji():PhaseChangeSkill("zhiji"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && PhaseChangeSkill::triggerable(target)
                && target->getMark("zhiji") == 0
                && target->getPhase() == Player::Start
                && target->isKongcheng();
    }

    virtual bool onPhaseChange(ServerPlayer *jiangwei) const{
        Room *room = jiangwei->getRoom();

        LogMessage log;
        log.type = "#ZhijiWake";
        log.from = jiangwei;
        log.arg = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke("zhiji");
        room->broadcastInvoke("animate", "lightbox:$Zhiji:5000");
        room->getThread()->delay(5000);
        QStringList choicelist;
        choicelist << "draw";
        if (jiangwei->getLostHp() != 0)
            choicelist << "recover";
        QString choice;
        if (choicelist.length() >=2)
            choice = room->askForChoice(jiangwei, objectName(), choicelist.join("+"));
        else
            choice = "draw";
        if(choice == "recover"){
            RecoverStruct recover;
            recover.who = jiangwei;
            room->recover(jiangwei, recover);
        }else
            room->drawCards(jiangwei, 2);

        room->setPlayerMark(jiangwei, "zhiji", 1);
        jiangwei->gainMark("@waked");
        room->acquireSkill(jiangwei, "yuxi");

        room->loseMaxHp(jiangwei);

        return false;
    }
};

class Xiangle: public TriggerSkill{
public:
    Xiangle():TriggerSkill("xiangle"){
        events << SlashEffected << TargetConfirming;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *liushan, QVariant &data) const{

        if(triggerEvent == TargetConfirming){

            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card && use.card->isKindOf("Slash")){

                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.type = "#Xiangle";
                log.from = use.from;
                log.to << liushan;
                log.arg = objectName();
                room->sendLog(log);
                QVariant dataforai = QVariant::fromValue(liushan);
                if(!room->askForCard(use.from, ".Basic", "@xiangle-discard", dataforai))
                    liushan->addMark("xiangle");
            }
        }
        else {
            if(liushan->getMark("xiangle") > 0){
                liushan->setMark("xiangle", liushan->getMark("xiangle") - 1);
                return true;
            }
        }

        return false;
    }
};

class Fangquan: public TriggerSkill{
public:
    Fangquan():TriggerSkill("fangquan"){
        events << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *liushan, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        switch(change.to){
        case Player::Play: {
            bool invoked = false;

            if(liushan->isSkipped(Player::Play))
                return false;
            invoked = liushan->askForSkillInvoke(objectName());
            if(invoked){
                liushan->setFlags("fangquan");
                liushan->skip(Player::Play);
            }
            return false;
        }
        case Player::NotActive: {
            if(liushan->hasFlag("fangquan")){
                if(liushan->isKongcheng() || !room->askForDiscard(liushan, "fangquan", 1, 1, true))
                    return false;

                ServerPlayer *player = room->askForPlayerChosen(liushan, room->getOtherPlayers(liushan), objectName());

                QString name = player->getGeneralName();
                if(name == "zhugeliang" || name == "shenzhugeliang" || name == "wolong")
                    room->broadcastSkillInvoke("fangquan", 1);
                else
                    room->broadcastSkillInvoke("fangquan", 2);

                LogMessage log;
                log.type = "#Fangquan";
                log.from = liushan;
                log.to << player;
                room->sendLog(log);

                PlayerStar p = player;
                room->setTag("FangquanTarget", QVariant::fromValue(p));
            }
            break;
        }
        default:
            break;
        }
        return false;
    }
};

class FangquanGive: public PhaseChangeSkill{
public:
    FangquanGive():PhaseChangeSkill("#fangquan-give"){

    }

    virtual int getPriority() const{
        return -4;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPhase() == Player::NotActive;
    }

    virtual bool onPhaseChange(ServerPlayer *liushan) const{
        Room *room = liushan->getRoom();
        if(!room->getTag("FangquanTarget").isNull())
        {
            PlayerStar target = room->getTag("FangquanTarget").value<PlayerStar>();
            room->removeTag("FangquanTarget");
            if(target->isAlive())
                target->gainAnExtraTurn();
        }
        return false;
    }
};

class Ruoyu: public PhaseChangeSkill{
public:
    Ruoyu():PhaseChangeSkill("ruoyu$"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPhase() == Player::Start
                && target->hasLordSkill("ruoyu")
                && target->isAlive()
                && target->getMark("ruoyu") == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *liushan) const{
        Room *room = liushan->getRoom();

        bool can_invoke = true;
        foreach(ServerPlayer *p, room->getAllPlayers()){
            if(liushan->getHp() > p->getHp()){
                can_invoke = false;
                break;
            }
        }

        if(can_invoke){
            room->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#RuoyuWake";
            log.from = liushan;
            log.arg = QString::number(liushan->getHp());
            log.arg2 = objectName();
            room->sendLog(log);

            room->setPlayerMark(liushan, "ruoyu", 1);
            liushan->gainMark("@waked");
            room->setPlayerProperty(liushan, "maxhp", liushan->getMaxHp() + 1);

            RecoverStruct recover;
            recover.who = liushan;
            room->recover(liushan, recover);

            room->acquireSkill(liushan, "jijiang");
        }

        return false;
    }
};

class Huashen: public GameStartSkill{
public:
    Huashen():GameStartSkill("huashen"){
    }

    static void playAudioEffect(ServerPlayer *zuoci, const QString &skill_name){
        zuoci->getRoom()->broadcastSkillInvoke(skill_name,  zuoci->isMale(), -1);
    }

    static void AcquireGenerals(ServerPlayer *zuoci, int n){
        QStringList list = GetAvailableGenerals(zuoci);
        qShuffle(list);

        QStringList acquired = list.mid(0, n);
        QVariantList huashens = zuoci->tag["Huashens"].toList();
        foreach(QString huashen, acquired){
            huashens << huashen;
            const General *general = Sanguosha->getGeneral(huashen);
            foreach(const TriggerSkill *skill, general->getTriggerSkills()){
                zuoci->getRoom()->getThread()->addTriggerSkill(skill);
            }
        }

        zuoci->tag["Huashens"] = huashens;

        zuoci->invoke("animate", "huashen:" + acquired.join(":"));

        LogMessage log;
        log.type = "#GetHuashen";
        log.from = zuoci;
        log.arg = QString::number(n);
        log.arg2 = QString::number(huashens.length());
        zuoci->getRoom()->sendLog(log);
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci){
        QSet<QString> all = Sanguosha->getLimitedGeneralNames().toSet();
        QSet<QString> huashen_set, room_set;
        QVariantList huashens = zuoci->tag["Huashens"].toList();
        foreach(QVariant huashen, huashens)
            huashen_set << huashen.toString();
        Room *room = zuoci->getRoom();
        QList<const ServerPlayer *> players = room->findChildren<const ServerPlayer *>();
        foreach(const ServerPlayer *player, players){
            room_set << player->getGeneralName();
            if(player->getGeneral2())
                room_set << player->getGeneral2Name();
        }

        static QSet<QString> banned;
        if(banned.isEmpty()){
            banned << "zuoci" << "zuocif" << "guzhielai" << "dengshizai" << "caochong"
                   << "jiangboyue" << "shenzhugeliang" << "shenlvbu" << "huaxiong" << "zhugejin";
        }

        return (all - banned - huashen_set - room_set).toList();
    }

    static QString SelectSkill(ServerPlayer *zuoci){
        Room *room = zuoci->getRoom();
        playAudioEffect(zuoci, "huashen");

        QString huashen_skill = zuoci->tag["HuashenSkill"].toString();
        if(!huashen_skill.isEmpty())
            room->detachSkillFromPlayer(zuoci, huashen_skill);

        QVariantList huashens = zuoci->tag["Huashens"].toList();
        if(huashens.isEmpty())
            return QString();

        QStringList huashen_generals;
        foreach(QVariant huashen, huashens)
            huashen_generals << huashen.toString();

        QStringList skill_names;
        QString skill_name;
        const General* general = NULL;
        AI* ai = zuoci->getAI();
        if(ai){
            QHash<QString, const General*> hash;
            foreach(QString general_name, huashen_generals){
                const General* general = Sanguosha->getGeneral(general_name);
                foreach(const Skill *skill, general->getVisibleSkillList()){
                    if(skill->isLordSkill() || skill->getFrequency() == Skill::Limited
                            || skill->getFrequency() == Skill::Wake)
                        continue;

                    if(!skill_names.contains(skill->objectName())){
                        hash[skill->objectName()] = general;
                        skill_names << skill->objectName();
                    }
                }
            }
            Q_ASSERT(skill_names.length() > 0);
            skill_name = ai->askForChoice("huashen", skill_names.join("+"), QVariant());
            general = hash[skill_name];
            Q_ASSERT(general != NULL);
            
        }
        else{
            QString general_name = room->askForGeneral(zuoci, huashen_generals);
            general = Sanguosha->getGeneral(general_name);

            foreach(const Skill *skill, general->getVisibleSkillList()){
                if(skill->isLordSkill() || skill->getFrequency() == Skill::Limited
                   || skill->getFrequency() == Skill::Wake)
                    continue;

                skill_names << skill->objectName();
            }

            if(skill_names.isEmpty())
                return QString();

            if(skill_names.length() == 1)
                skill_name = skill_names.first();
            else
                skill_name = room->askForChoice(zuoci, "huashen", skill_names.join("+"));
        }

        QString kingdom = general->getKingdom();
        
        if(zuoci->getKingdom() != kingdom){
            if(kingdom == "god")
                kingdom = room->askForKingdom(zuoci);
            room->setPlayerProperty(zuoci, "kingdom", kingdom);
        }

        if(zuoci->getGender() != general->getGender())
            zuoci->setGender(general->getGender());

        Q_ASSERT(!skill_name.isNull() && !skill_name.isEmpty());

        Json::Value arg(Json::arrayValue);
        arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        arg[1] = QSanProtocol::Utils::toJsonString(zuoci->objectName());
        arg[2] = QSanProtocol::Utils::toJsonString(general->objectName());
        arg[3] = QSanProtocol::Utils::toJsonString(skill_name);
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        zuoci->tag["HuashenSkill"] = skill_name;
        
        room->acquireSkill(zuoci, skill_name);

        return skill_name;
    }

    virtual void onGameStart(ServerPlayer *zuoci) const{
        AcquireGenerals(zuoci, 2);
        SelectSkill(zuoci);
    }

    virtual QDialog *getDialog() const{
        static HuashenDialog *dialog;

        if(dialog == NULL)
            dialog = new HuashenDialog;

        return dialog;
    }
};

HuashenDialog::HuashenDialog()
{
    setWindowTitle(Sanguosha->translate("huashen"));
}

void HuashenDialog::popup(){
    QVariantList huashen_list = Self->tag["Huashens"].toList();
    QList<const General *> huashens;
    foreach(QVariant huashen, huashen_list)
        huashens << Sanguosha->getGeneral(huashen.toString());

    fillGenerals(huashens);

    show();
}

class HuashenBegin: public PhaseChangeSkill{
public:
    HuashenBegin():PhaseChangeSkill("#huashen-begin"){
    }

    int getPriority() const{
        return 3;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::RoundStart;
    }

    virtual bool onPhaseChange(ServerPlayer *zuoci) const{
        if(zuoci->askForSkillInvoke("huashen")){
            QString skill_name = Huashen::SelectSkill(zuoci);
        }
        return false;
    }
};

class HuashenEnd: public TriggerSkill{
public:
    HuashenEnd():TriggerSkill("#huashen-end"){
        events << EventPhaseStart;
    }

    virtual int getPriority() const{
        return -2;
    }

    virtual bool trigger(TriggerEvent , Room* , ServerPlayer *zuoci, QVariant &data) const{
        if(zuoci->getPhase() == Player::NotActive && zuoci->askForSkillInvoke("huashen"))
            Huashen::SelectSkill(zuoci);
        return false;
    }
};

class Xinsheng: public MasochismSkill{
public:
    Xinsheng():MasochismSkill("xinsheng"){
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *zuoci, const DamageStruct &damage) const{
        int n = damage.damage;
        if(n == 0)
            return;

        if(zuoci->askForSkillInvoke(objectName())){
            Huashen::playAudioEffect(zuoci, objectName());
            Huashen::AcquireGenerals(zuoci, n);
        }
    }
};

MountainPackage::MountainPackage()
    :Package("mountain")
{
    /*General *zhanghe = new General(this, "zhanghe", "wei");
    zhanghe->addSkill(new Qiaobian);

    General *dengai = new General(this, "dengai", "wei", 4);
    dengai->addSkill(new Tuntian);
    dengai->addSkill(new TuntianGet);
    dengai->addSkill(new Zaoxian);
    dengai->addRelateSkill("jixi");
    related_skills.insertMulti("tuntian", "#tuntian-get");

    General *jiangwei = new General(this, "jiangwei", "shu");
    jiangwei->addSkill(new Tiaoxin);
    jiangwei->addSkill(new Zhiji);
    related_skills.insertMulti("zhiji", "yuxi");

    General *liushan = new General(this, "liushan$", "shu", 3);
    liushan->addSkill(new Xiangle);
    liushan->addSkill(new Fangquan);
    liushan->addSkill(new FangquanGive);
    liushan->addSkill(new Ruoyu);
    related_skills.insertMulti("fangquan", "#fangquan-give");

    General *zuoci = new General(this, "zuoci", "qun", 3);
    zuoci->addSkill(new Huashen);
    zuoci->addSkill(new HuashenBegin);
    zuoci->addSkill(new HuashenEnd);
    zuoci->addSkill(new Xinsheng);

    related_skills.insertMulti("huashen", "#huashen-begin");
    related_skills.insertMulti("huashen", "#huashen-end");

    addMetaObject<TiaoxinCard>();
    addMetaObject<JixiCard>();

    skills << new Jixi;*/
}

ADD_PACKAGE(Mountain)
