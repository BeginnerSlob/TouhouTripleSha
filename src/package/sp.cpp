#include "sp.h"
#include "client.h"
#include "general.h"
#include "skill.h"
#include "standard-skillcards.h"
#include "engine.h"
#include "maneuvering.h"

class SPMoonSpearSkill: public WeaponSkill {
public:
    SPMoonSpearSkill(): WeaponSkill("sp_moonspear") {
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive)
            return false;

        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            card = card_use.card;
        } else if (triggerEvent == CardResponded) {
            card = data.value<CardResponseStruct>().m_card;
        }

        if (card == NULL || !card->isBlack()
            || (card->getHandlingMethod() != Card::MethodUse && card->getHandlingMethod() != Card::MethodResponse))
            return false;

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *tmp, room->getAlivePlayers()) {
            if (player->inMyAttackRange(tmp))
                targets << tmp;
        }
        if (targets.isEmpty()) return false;

        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@sp_moonspear", true, true);
        if (!target) return false;
        room->setEmotion(player, "weapon/moonspear");
        if (!room->askForCard(target, "jink", "@moon-spear-jink", QVariant(), Card::MethodResponse, player))
            room->damage(DamageStruct(objectName(), player, target));
        return false;
    }
};

SPMoonSpear::SPMoonSpear(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("sp_moonspear");
}

class Danji: public PhaseChangeSkill {
public:
    Danji(): PhaseChangeSkill("danji") { // What a silly skill!
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("danji") == 0
               && target->getHandcardNum() > target->getHp();
    }

    virtual bool onPhaseChange(ServerPlayer *guanyu) const{
        Room *room = guanyu->getRoom();
        ServerPlayer *the_lord = room->getLord();
        if (the_lord && (the_lord->getGeneralName().contains("caocao") || the_lord->getGeneral2Name().contains("caocao"))) {
            room->notifySkillInvoked(guanyu, objectName());

            LogMessage log;
            log.type = "#DanjiWake";
            log.from = guanyu;
            log.arg = QString::number(guanyu->getHandcardNum());
            log.arg2 = QString::number(guanyu->getHp());
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            room->doLightbox("$DanjiAnimate", 5000);

            room->setPlayerMark(guanyu, "danji", 1);
            if (room->changeMaxHpForAwakenSkill(guanyu) && guanyu->getMark("danji") == 1)
                room->acquireSkill(guanyu, "thjibu");
        }

        return false;
    }
};

class Xiuluo: public PhaseChangeSkill {
public:
    Xiuluo(): PhaseChangeSkill("xiuluo") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->canDiscard(target, "h")
               && hasDelayedTrick(target);
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        while (hasDelayedTrick(target) && target->canDiscard(target, "h")) {
            QStringList suits;
            foreach (const Card *jcard, target->getJudgingArea()) {
                if (!suits.contains(jcard->getSuitString()))
                    suits << jcard->getSuitString();
            }

            const Card *card = room->askForCard(target, QString(".|%1|.|hand").arg(suits.join(",")),
                                                "@xiuluo", QVariant(), objectName());
            if (!card || !hasDelayedTrick(target)) break;
            room->broadcastSkillInvoke(objectName());

            QList<int> avail_list, other_list;
            foreach (const Card *jcard, target->getJudgingArea()) {
                if (jcard->isKindOf("SkillCard")) continue;
                if (jcard->getSuit() == card->getSuit())
                    avail_list << jcard->getEffectiveId();
                else
                    other_list << jcard->getEffectiveId();
            }
            room->fillAG(avail_list + other_list, NULL, other_list);
            int id = room->askForAG(target, avail_list, false, objectName());
            room->clearAG();
            room->throwCard(id, NULL);
        }

        return false;
    }

private:
    static bool hasDelayedTrick(const ServerPlayer *target) {
        foreach (const Card *card, target->getJudgingArea())
            if (!card->isKindOf("SkillCard")) return true;
        return false;
    }
};

class Shenwei: public DrawCardsSkill {
public:
    Shenwei(): DrawCardsSkill("#shenwei-draw") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        room->broadcastSkillInvoke("shenwei");
        room->notifySkillInvoked(player, "shenwei");
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = "shenwei";
        room->sendLog(log);

        return n + 2;
    }
};

class ShenweiKeep: public MaxCardsSkill {
public:
    ShenweiKeep(): MaxCardsSkill("shenwei") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return 2;
        else
            return 0;
    }
};

class Shenji: public TargetModSkill {
public:
    Shenji(): TargetModSkill("shenji") {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()) && from->getWeapon() == NULL)
            return 2;
        else
            return 0;
    }
};

ZhoufuCard::ZhoufuCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool ZhoufuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && to_select->getPile("incantation").isEmpty();
}

void ZhoufuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    target->tag["ZhoufuSource" + QString::number(getEffectiveId())] = QVariant::fromValue(source);
    room->broadcastSkillInvoke("zhoufu");
    target->addToPile("incantation", this);
}

class ZhoufuViewAsSkill: public OneCardViewAsSkill {
public:
    ZhoufuViewAsSkill(): OneCardViewAsSkill("zhoufu") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ZhoufuCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        Card *card = new ZhoufuCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Zhoufu: public TriggerSkill {
public:
    Zhoufu(): TriggerSkill("zhoufu") {
        events << StartJudge << EventPhaseChanging;
        view_as_skill = new ZhoufuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPile("incantation").length() > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == StartJudge) {
            int card_id = player->getPile("incantation").first();

            JudgeStruct *judge = data.value<JudgeStruct *>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$ZhoufuJudge";
            log.from = player;
            log.arg = objectName();
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                             CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                             judge->who->objectName(),
                             QString(), QString(), judge->reason), true);
            judge->updateResult();
            room->setTag("SkipGameRule", true);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                int id = player->getPile("incantation").first();
                ServerPlayer *zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
                if (zhangbao && zhangbao->isAlive())
                    zhangbao->obtainCard(Sanguosha->getCard(id));
            }
        }
        return false;
    }
};

class Yingbing: public TriggerSkill {
public:
    Yingbing(): TriggerSkill("yingbing") {
        events << StartJudge;
        frequency = Frequent;
    }

    virtual int getPriority(TriggerEvent) const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        int id = judge->card->getEffectiveId();
        ServerPlayer *zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
        if (zhangbao && TriggerSkill::triggerable(zhangbao)
            && zhangbao->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            zhangbao->drawCards(2, "yingbing");
        }
        return false;
    }
};

YinbingCard::YinbingCard() {
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void YinbingCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->addToPile("yinbing", this);
}

class YinbingViewAsSkill: public ViewAsSkill {
public:
    YinbingViewAsSkill(): ViewAsSkill("yinbing") {
        response_pattern = "@@yinbing";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return to_select->getTypeId() != Card::TypeBasic;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 0) return NULL;

        Card *acard = new YinbingCard;
        acard->addSubcards(cards);
        acard->setSkillName(objectName());
        return acard;
    }
};

class Yinbing: public TriggerSkill {
public:
    Yinbing(): TriggerSkill("yinbing") {
        events << EventPhaseStart << Damaged;
        view_as_skill = new YinbingViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish && !player->isNude()) {
            room->askForUseCard(player, "@@yinbing", "@yinbing", -1, Card::MethodNone);
        } else if (triggerEvent == Damaged && !player->getPile("yinbing").isEmpty()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))) {
                room->notifySkillInvoked(player, objectName());
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                QList<int> ids = player->getPile("yinbing");
                room->fillAG(ids, player);
                int id = room->askForAG(player, ids, false, objectName());
                room->clearAG(player);
                room->throwCard(id, NULL);
            }
        }

        return false;
    }
};

class Juedi: public PhaseChangeSkill {
public:
    Juedi(): PhaseChangeSkill("juedi") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start
               && !target->getPile("yinbing").isEmpty();
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (!room->askForSkillInvoke(target, objectName())) return false;
        room->broadcastSkillInvoke(objectName());

        QList<ServerPlayer *> playerlist;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->getHp() <= target->getHp())
                playerlist << p;
        }
        ServerPlayer *to_give = NULL;
        if (!playerlist.isEmpty())
            to_give = room->askForPlayerChosen(target, playerlist, objectName(), "@juedi", true);
        if (to_give) {
            room->recover(to_give, RecoverStruct(target));
            DummyCard *dummy = new DummyCard(target->getPile("yinbing"));
            room->obtainCard(to_give, dummy);
            delete dummy;
        } else {
            int len = target->getPile("yinbing").length();
            target->clearOnePrivatePile("yinbing");
            if (target->isAlive())
                room->drawCards(target, len, objectName());
        }
        return false;
    }
};

class Dujin: public DrawCardsSkill {
public:
    Dujin(): DrawCardsSkill("dujin") {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        if (player->askForSkillInvoke(objectName())) {
            player->getRoom()->broadcastSkillInvoke(objectName());
            return n + player->getEquips().length() / 2 + 1;
        } else
            return n;
    }
};

QingyiCard::QingyiCard() {
    mute = true;
}

bool QingyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("qingyi");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void QingyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
            targets.removeOne(target);
    }

    if (targets.length() > 0) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_qingyi");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}

class QingyiViewAsSkill: public ZeroCardViewAsSkill {
public:
    QingyiViewAsSkill(): ZeroCardViewAsSkill("qingyi") {
        response_pattern = "@@qingyi";
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@qingyi";
    }

    virtual const Card *viewAs() const{
        return new QingyiCard;
    }
};

class Qingyi: public TriggerSkill {
public:
    Qingyi(): TriggerSkill("qingyi") {
        events << EventPhaseChanging;
        view_as_skill = new QingyiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !player->isSkipped(Player::Judge)
            && !player->isSkipped(Player::Draw)) {
            if (Slash::IsAvailable(player) && room->askForUseCard(player, "@@qingyi", "@qingyi-slash")) {
                player->skip(Player::Judge, true);
                player->skip(Player::Draw, true);
            }
        }
        return false;
    }
};

class Shixin: public TriggerSkill {
public:
    Shixin(): TriggerSkill("shixin") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire) {
            room->notifySkillInvoked(player, objectName());
            room->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#ShixinProtect";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = "fire_nature";
            room->sendLog(log);
            return true;
        }
        return false;
    }
};

SPCardPackage::SPCardPackage()
    : Package("sp_cards")
{
    (new SPMoonSpear)->setParent(this);
    skills << new SPMoonSpearSkill;

    type = CardPack;
}

ADD_PACKAGE(SPCard)

SPPackage::SPPackage()
    : Package("sp")
{

    General *sp_diaochan = new General(this, "sp_diaochan", "qun", 3, false, true); // SP 002
    sp_diaochan->addSkill("ikmoyu");
    sp_diaochan->addSkill("ikzhuoyue");

    General *gongsunzan = new General(this, "gongsunzan", "qun"); // SP 003
    gongsunzan->addSkill("ikzhuji");

    General *sp_sunshangxiang = new General(this, "sp_sunshangxiang", "shu", 3, false, true); // SP 005
    sp_sunshangxiang->addSkill("ikyulu");
    sp_sunshangxiang->addSkill("ikcuimeng");

    General *sp_pangde = new General(this, "sp_pangde", "wei", 4, true, true); // SP 006
    sp_pangde->addSkill("thjibu");
    sp_pangde->addSkill("ikmengjin");

    General *sp_guanyu = new General(this, "sp_guanyu", "wei", 4); // SP 007
    sp_guanyu->addSkill("ikchilian");
    sp_guanyu->addSkill(new Danji);

    General *shenlvbu1 = new General(this, "shenlvbu1", "god", 8, true, true); // SP 008 (2-1)
    shenlvbu1->addSkill("thjibu");
    shenlvbu1->addSkill("ikwushuang");

    General *shenlvbu2 = new General(this, "shenlvbu2", "god", 4, true, true); // SP 008 (2-2)
    shenlvbu2->addSkill("thjibu");
    shenlvbu2->addSkill("ikwushuang");
    shenlvbu2->addSkill(new Xiuluo);
    shenlvbu2->addSkill(new ShenweiKeep);
    shenlvbu2->addSkill(new Shenwei);
    shenlvbu2->addSkill(new Shenji);
    related_skills.insertMulti("shenwei", "#shenwei-draw");

    General *sp_caiwenji = new General(this, "sp_caiwenji", "wei", 3, false, true); // SP 009
    sp_caiwenji->addSkill("ikhuiyao");
    sp_caiwenji->addSkill("ikqihuang");

    General *sp_machao = new General(this, "sp_machao", "qun", 4, true, true); // SP 011
    sp_machao->addSkill("thjibu");
    sp_machao->addSkill("nostieji");

    General *sp_jiaxu = new General(this, "sp_jiaxu", "wei", 3, true, true); // SP 012
    sp_jiaxu->addSkill("iksishideng");
    sp_jiaxu->addSkill("ikwenle");
    sp_jiaxu->addSkill("ikmoyudeng");

    General *sp_zhenji = new General(this, "sp_zhenji", "wei", 3, false, true); // SP 015
    sp_zhenji->addSkill("ikzhongyan");
    sp_zhenji->addSkill("ikmengyang");

    General *sp_shenlvbu = new General(this, "sp_shenlvbu", "god", 5, true, true); // SP 022
    sp_shenlvbu->addSkill("ikzhuohuo");
    sp_shenlvbu->addSkill("ikwumou");
    sp_shenlvbu->addSkill("iksuikong");
    sp_shenlvbu->addSkill("iktianwubaka");

    General *sp_yuejin = new General(this, "sp_yuejin", "wei", 4, true, true); // SP 024
    sp_yuejin->addSkill("ikxiaorui");

    General *zhangbao = new General(this, "zhangbao", "qun", 3); // SP 025
    zhangbao->addSkill(new Zhoufu);
    zhangbao->addSkill(new Yingbing);

    General *sp_zhugejin = new General(this, "sp_zhugejin", "wu", 3, true, true); // SP 027
    sp_zhugejin->addSkill("ikhongrou");
    sp_zhugejin->addSkill("ikhuaxiao");

    General *sp_panfeng = new General(this, "sp_panfeng", "qun", 4, true, true); // SP 029
    sp_panfeng->addSkill("ikshunqie");

    General *zumao = new General(this, "zumao", "wu"); // SP 030
    zumao->addSkill(new Yinbing);
    zumao->addSkill(new Juedi);

    General *sp_dingfeng = new General(this, "sp_dingfeng", "wu", 4, true, true); // SP 031
    sp_dingfeng->addSkill("ikxindu");
    sp_dingfeng->addSkill("ikfenxun");

    General *sp_hetaihou = new General(this, "sp_hetaihou", "qun", 3, false, true); // SP 033
    sp_hetaihou->addSkill("ikzhoudu");
    sp_hetaihou->addSkill("ikkuangdi");

    addMetaObject<ZhoufuCard>();
    addMetaObject<YinbingCard>();
}

ADD_PACKAGE(SP)

OLPackage::OLPackage()
    : Package("OL")
{

    General *lingcao = new General(this, "lingcao", "wu", 4);
    lingcao->addSkill(new Dujin);

    General *sunru = new General(this, "sunru", "wu", 3, false);
    sunru->addSkill(new Qingyi);
    sunru->addSkill(new SlashNoDistanceLimitSkill("qingyi"));
    sunru->addSkill(new Shixin);
    related_skills.insertMulti("qingyi", "#qingyi-slash-ndl");

    General *ol_fazheng = new General(this, "ol_fazheng", "shu", 3, true, true);
    ol_fazheng->addSkill("enyuan");
    ol_fazheng->addSkill("xuanhuo");

    General *ol_xushu = new General(this, "ol_xushu", "shu", 3, true, true);
    ol_xushu->addSkill("ikmitu");
    ol_xushu->addSkill("jujian");

    General *ol_guanxingzhangbao = new General(this, "ol_guanxingzhangbao", "shu", 4, true, true);
    ol_guanxingzhangbao->addSkill("fuhun");

    General *ol_madai = new General(this, "ol_madai", "shu", 4, true, true);
    ol_madai->addSkill("thjibu");
    ol_madai->addSkill("qianxi");

    General *ol_wangyi = new General(this, "ol_wangyi", "wei", 3, false, true);
    ol_wangyi->addSkill("zhenlie");
    ol_wangyi->addSkill("miji");

    addMetaObject<QingyiCard>();
}

ADD_PACKAGE(OL)

TaiwanSPPackage::TaiwanSPPackage()
    : Package("Taiwan_sp")
{
    General *tw_caocao = new General(this, "tw_caocao$", "wei", 4, true, true); // TW SP 019
    tw_caocao->addSkill("nosjianxiong");
    tw_caocao->addSkill("ikhuanwei");

    General *tw_simayi = new General(this, "tw_simayi", "wei", 3, true, true);
    tw_simayi->addSkill("nosfankui");
    tw_simayi->addSkill("nosguicai");

    General *tw_xiahoudun = new General(this, "tw_xiahoudun", "wei", 4, true, true); // TW SP 025
    tw_xiahoudun->addSkill("nosganglie");

    General *tw_zhangliao = new General(this, "tw_zhangliao", "wei", 4, true, true); // TW SP 013
    tw_zhangliao->addSkill("nostuxi");

    General *tw_xuchu = new General(this, "tw_xuchu", "wei", 4, true, true);
    tw_xuchu->addSkill("nosluoyi");

    General *tw_guojia = new General(this, "tw_guojia", "wei", 3, true, true); // TW SP 015
    tw_guojia->addSkill("iktiandu");
    tw_guojia->addSkill("ikyumeng");

    General *tw_zhenji = new General(this, "tw_zhenji", "wei", 3, false, true); // TW SP 007
    tw_zhenji->addSkill("ikzhongyan");
    tw_zhenji->addSkill("ikmengyang");

    General *tw_liubei = new General(this, "tw_liubei$", "shu", 4, true, true); // TW SP 017
    tw_liubei->addSkill("ikshenai");
    tw_liubei->addSkill("ikxinqi");

    General *tw_guanyu = new General(this, "tw_guanyu", "shu", 4, true, true); // TW SP 018
    tw_guanyu->addSkill("ikchilian");

    General *tw_zhangfei = new General(this, "tw_zhangfei", "shu", 4, true, true);
    tw_zhangfei->addSkill("iklipao");

    General *tw_zhugeliang = new General(this, "tw_zhugeliang", "shu", 3, true, true); // TW SP 012
    tw_zhugeliang->addSkill("ikyuxi");
    tw_zhugeliang->addSkill("ikjingyou");

    General *tw_zhaoyun = new General(this, "tw_zhaoyun", "shu", 4, true, true); // TW SP 006
    tw_zhaoyun->addSkill("ikhuahuan");

    General *tw_machao = new General(this, "tw_machao", "shu", 4, true, true); // TW SP 010
    tw_machao->addSkill("thjibu");
    tw_machao->addSkill("nostieji");

    General *tw_huangyueying = new General(this, "tw_huangyueying", "shu", 3, false, true); // TW SP 011
    tw_huangyueying->addSkill("ikhuiquan");
    tw_huangyueying->addSkill("thjizhi");

    General *tw_sunquan = new General(this, "tw_sunquan$", "wu", 4, true, true); // TW SP 021
    tw_sunquan->addSkill("ikzhiheng");
    tw_sunquan->addSkill("ikjiyuan");

    General *tw_ganning = new General(this, "tw_ganning", "wu", 4, true, true); // TW SP 009
    tw_ganning->addSkill("ikkuipo");

    General *tw_lvmeng = new General(this, "tw_lvmeng", "wu", 4, true, true);
    tw_lvmeng->addSkill("ikbiju");

    General *tw_huanggai = new General(this, "tw_huanggai", "wu", 4, true, true); // TW SP 014
    tw_huanggai->addSkill("ikkurou");

    General *tw_zhouyu = new General(this, "tw_zhouyu", "wu", 3, true, true);
    tw_zhouyu->addSkill("nosyingzi");
    tw_zhouyu->addSkill("nosfanjian");

    General *tw_daqiao = new General(this, "tw_daqiao", "wu", 3, false, true); // TW SP 005
    tw_daqiao->addSkill("nosguose");
    tw_daqiao->addSkill("ikxuanhuo");

    General *tw_luxun = new General(this, "tw_luxun", "wu", 3, true, true); // TW SP 016
    tw_luxun->addSkill("ikwujie");

    General *tw_sunshangxiang = new General(this, "tw_sunshangxiang", "wu", 3, false, true); // TW SP 028
    tw_sunshangxiang->addSkill("ikyulu");
    tw_sunshangxiang->addSkill("ikcuimeng");

    /*General *tw_huatuo = new General(this, "tw_huatuo", 3, true, true);
    tw_huatuo->addSkill("ikqingnang");
    tw_huatuo->addSkill("ikhuichun");*/

    General *tw_lvbu = new General(this, "tw_lvbu", "qun", 4, true, true); // TW SP 008
    tw_lvbu->addSkill("ikwushuang");

    General *tw_diaochan = new General(this, "tw_diaochan", "qun", 3, false, true); // TW SP 002
    tw_diaochan->addSkill("ikmoyu");
    tw_diaochan->addSkill("ikzhuoyue");

    General *tw_xiaoqiao = new General(this, "tw_xiaoqiao", "wu", 3, false, true);
    tw_xiaoqiao->addSkill("ikzhihui");
    tw_xiaoqiao->addSkill("ikchiqiu");

    General *tw_yuanshu = new General(this, "tw_yuanshu", "qun", 4, true, true); // TW SP 004
    tw_yuanshu->addSkill("ikchenyan");
    tw_yuanshu->addSkill("ikshengzun");
}

ADD_PACKAGE(TaiwanSP)

MiscellaneousPackage::MiscellaneousPackage()
    : Package("miscellaneous")
{
    General *wz_daqiao = new General(this, "wz_daqiao", "wu", 3, false, true); // WZ 001
    wz_daqiao->addSkill("nosguose");
    wz_daqiao->addSkill("ikxuanhuo");

    General *wz_xiaoqiao = new General(this, "wz_xiaoqiao", "wu", 3, false, true); // WZ 002
    wz_xiaoqiao->addSkill("ikzhihui");
    wz_xiaoqiao->addSkill("ikchiqiu");

    General *pr_shencaocao = new General(this, "pr_shencaocao", "god", 3, true, true); // PR LE 005
    pr_shencaocao->addSkill("ikguixin");
    pr_shencaocao->addSkill("thfeiying");

    General *pr_nos_simayi = new General(this, "pr_nos_simayi", "wei", 3, true, true); // PR WEI 002
    pr_nos_simayi->addSkill("nosfankui");
    pr_nos_simayi->addSkill("nosguicai");
}

ADD_PACKAGE(Miscellaneous)

HegemonySPPackage::HegemonySPPackage()
    : Package("hegemony_sp")
{
    General *sp_heg_zhouyu = new General(this, "sp_heg_zhouyu", "wu", 3, true, true); // GSP 001
    sp_heg_zhouyu->addSkill("nosyingzi");
    sp_heg_zhouyu->addSkill("nosfanjian");

    General *sp_heg_xiaoqiao = new General(this, "sp_heg_xiaoqiao", "wu", 3, false, true); // GSP 002
    sp_heg_xiaoqiao->addSkill("ikzhihui");
    sp_heg_xiaoqiao->addSkill("ikchiqiu");
}

ADD_PACKAGE(HegemonySP)

