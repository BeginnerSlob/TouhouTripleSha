#include "skill.h"
#include "settings.h"
#include "engine.h"
#include "player.h"
#include "room.h"
#include "client.h"
#include "standard.h"
#include "scenario.h"

#include <QFile>

Skill::Skill(const QString &name, Frequency frequency)
    : frequency(frequency), limit_mark(QString()), owner_only_skill(false)
{
    static QChar lord_symbol('$');
    static QChar attached_lord_symbol('&');
    lord_skill = false;
    attached_lord_skill = false;
    if (name.endsWith(lord_symbol)) {
        QString copy = name;
        copy.remove(lord_symbol);
        setObjectName(copy);
        lord_skill = true;
    } else if (name.endsWith(attached_lord_symbol)) {
        QString copy = name;
        copy.remove(attached_lord_symbol);
        setObjectName(copy);
        attached_lord_skill = true;
    } else {
        setObjectName(name);
    }
}

bool Skill::isLordSkill() const{
    return lord_skill;
}

bool Skill::isAttachedLordSkill() const{
    return attached_lord_skill;
}

bool Skill::shouldBeVisible(const Player *Self) const
{
    return Self != NULL && Self->hasSkill(objectName());
}

bool Skill::isOwnerOnlySkill() const
{
    return owner_only_skill;
}

QString Skill::getDescription(int step) const
{
    bool normal_game = ServerInfo.DuringGame && isNormalGameMode(ServerInfo.GameMode);
    QString name = QString("%1%2%3").arg(objectName()).arg(step > 0 ? QString::number(step) : "").arg(normal_game ? "_p" : "");
    QString des_src = Sanguosha->translate(":" + name);
    if (normal_game && des_src.startsWith(":"))
        des_src = Sanguosha->translate(":" + QString("%1%2").arg(objectName()).arg(step > 0 ? QString::number(step) : ""));
    if (des_src.startsWith(":"))
        return QString();
    if (des_src.startsWith("[NoAutoRep]"))
        return des_src.mid(11);

    if (Config.value("AutoSkillTypeColorReplacement", true).toBool()) {
        QMap<QString, QColor> skilltype_color_map = Sanguosha->getSkillTypeColorMap();
        foreach (QString skill_type, skilltype_color_map.keys()) {
            QString type_name = Sanguosha->translate(skill_type);
            QString color_name = skilltype_color_map[skill_type].name();
            des_src.replace(type_name, QString("<font color=%1><b>%2</b></font>").arg(color_name).arg(type_name));
        }
    }
    if (Config.value("AutoSuitReplacement", true).toBool()) {
        for (int i = 0; i <= 3; i++) {
            Card::Suit suit = (Card::Suit)i;
            QString suit_name = Sanguosha->translate(Card::Suit2String(suit));
            QString suit_char = Sanguosha->translate(Card::Suit2String(suit) + "_char");
            QString colored_suit_char;
            if (i < 2)
                colored_suit_char = suit_char;
            else
                colored_suit_char = QString("<font color=#FF0000>%1</font>").arg(suit_char);
            des_src.replace(suit_char, colored_suit_char);
            des_src.replace(suit_name, colored_suit_char);
        }
    }
    return des_src;
}

QString Skill::getNotice(int index) const{
    if (index == -1)
        return Sanguosha->translate("~" + objectName());

    return Sanguosha->translate(QString("~%1%2").arg(objectName()).arg(index));
}

bool Skill::isVisible() const{
    return !objectName().startsWith("#") && !inherits("SPConvertSkill");
}

int Skill::getEffectIndex(const ServerPlayer *, const Card *) const{
    return -1;
}

void Skill::initMediaSource() {
    sources.clear();
    for (int i = 1; ;i++) {
        QString file_name = QString("%1%2").arg(objectName()).arg(QString::number(i));
        QString new_name = Sanguosha->translate(file_name);
        if (new_name != file_name && QFile::exists(QString("audio/skill/%1.ogg").arg(new_name)))
            sources << QString("audio/skill/%1.ogg").arg(new_name);
        else {
            QString effect_file = QString("audio/skill/%1.ogg").arg(file_name);
            if (QFile::exists(effect_file))
                sources << effect_file;
            else
                break;
        }
    }

    if (sources.isEmpty()) {
        QString effect_file = QString("audio/skill/%1.ogg").arg(objectName());
        if (QFile::exists(effect_file))
            sources << effect_file;
    }
}

void Skill::playAudioEffect(int index, bool superpose) const{
    if (!sources.isEmpty()) {
        if (index == -1)
            index = qrand() % sources.length();
        else
            index--;

        // check length
        QString filename;
        if (index >= 0 && index < sources.length())
            filename = sources.at(index);
        else if (index >= sources.length()) {
            while (index >= sources.length())
                index -= sources.length();
            filename = sources.at(index);
        } else
            filename = sources.first();

        Sanguosha->playAudioEffect(filename, superpose);
    }
}

Skill::Frequency Skill::getFrequency(const Player *) const
{
    return frequency;
}

QString Skill::getLimitMark() const
{
    return limit_mark;
}

QStringList Skill::getSources() const{
    return sources;
}

QDialog *Skill::getDialog() const{
    return NULL;
}

ViewAsSkill::ViewAsSkill(const QString &name)
    : Skill(name), response_pattern(QString()), response_or_use(false), expand_pile(QString())
{
}

bool ViewAsSkill::isAvailable(const Player *invoker,
                              CardUseStruct::CardUseReason reason,
                              const QString &pattern) const{
    if (!invoker->hasSkill(objectName()) && !invoker->hasLordSkill(objectName())
        && !invoker->hasFlag(objectName())) // For IkQiyu
        return false;
    switch (reason) {
    case CardUseStruct::CARD_USE_REASON_PLAY: return isEnabledAtPlay(invoker);
    case CardUseStruct::CARD_USE_REASON_RESPONSE:
    case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: return isEnabledAtResponse(invoker, pattern);
    default:
            return false;
    }
}

bool ViewAsSkill::isEnabledAtPlay(const Player *) const{
    return response_pattern.isEmpty();
}

bool ViewAsSkill::isEnabledAtResponse(const Player *, const QString &pattern) const{
    if (!response_pattern.isEmpty())
        return pattern == response_pattern;
    return false;
}

bool ViewAsSkill::isEnabledAtNullification(const ServerPlayer *) const{
    return false;
}

const ViewAsSkill *ViewAsSkill::parseViewAsSkill(const Skill *skill) {
    if (skill == NULL) return NULL;
    if (skill->inherits("ViewAsSkill")) {
        const ViewAsSkill *view_as_skill = qobject_cast<const ViewAsSkill *>(skill);
        return view_as_skill;
    }
    if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        Q_ASSERT(trigger_skill != NULL);
        const ViewAsSkill *view_as_skill = trigger_skill->getViewAsSkill();
        if (view_as_skill != NULL) return view_as_skill;
    }
    return NULL;
}

ZeroCardViewAsSkill::ZeroCardViewAsSkill(const QString &name)
    : ViewAsSkill(name)
{
}

const Card *ZeroCardViewAsSkill::viewAs(const QList<const Card *> &cards) const{
    if (cards.isEmpty())
        return viewAs();
    else
        return NULL;
}

bool ZeroCardViewAsSkill::viewFilter(const QList<const Card *> &, const Card *) const{
    return false;
}

OneCardViewAsSkill::OneCardViewAsSkill(const QString &name)
    : ViewAsSkill(name), filter_pattern(QString())
{
}

bool OneCardViewAsSkill::viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
    return selected.isEmpty() && !to_select->hasFlag("using") && viewFilter(to_select);
}

bool OneCardViewAsSkill::viewFilter(const Card *to_select) const{
    if (!inherits("FilterSkill") && !filter_pattern.isEmpty()) {
        QString pat = filter_pattern;
        if (pat.endsWith("!")) {
            if (Self->isJilei(to_select)) return false;
            pat.chop(1);
        } else if (response_or_use && pat.contains("hand")) {
            pat.replace("hand", "hand,wooden_ox,pokemon,currency");
        }
        ExpPattern pattern(pat);
        return pattern.match(Self, to_select);
    }
    return false;
}

const Card *OneCardViewAsSkill::viewAs(const QList<const Card *> &cards) const{
    if (cards.length() != 1)
        return NULL;
    else
        return viewAs(cards.first());
}

FilterSkill::FilterSkill(const QString &name)
    : OneCardViewAsSkill(name)
{
    frequency = Compulsory;
}

TriggerSkill::TriggerSkill(const QString &name)
    : Skill(name), view_as_skill(NULL), global(false), dynamic_priority(0.0)
{
}

const ViewAsSkill *TriggerSkill::getViewAsSkill() const{
    return view_as_skill;
}

QList<TriggerEvent> TriggerSkill::getTriggerEvents() const{
    return events;
}

int TriggerSkill::getPriority(TriggerEvent) const{
    return 3;
}

TriggerList TriggerSkill::triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
    TriggerList skill_lists;
    if (objectName() == "game_rule") return skill_lists;
    ServerPlayer *ask_who = player;
    QStringList skill_list = triggerable(triggerEvent, room, player, data, ask_who);
    if (!skill_list.isEmpty())
        skill_lists.insert(ask_who, skill_list);
    return skill_lists;
}

bool TriggerSkill::triggerable(const ServerPlayer *target, Room *) const{
    return triggerable(target);
}

bool TriggerSkill::triggerable(const ServerPlayer *target) const{
    return target != NULL && (global || (target->isAlive() && target->hasSkill(objectName())));
}

QStringList TriggerSkill::triggerable(TriggerEvent , Room *room, ServerPlayer *target, QVariant &, ServerPlayer* &) const{
    if (triggerable(target, room))
        return QStringList(objectName());
    return QStringList();
}

bool TriggerSkill::cost(TriggerEvent , Room *, ServerPlayer *, QVariant &, ServerPlayer *) const{
    return true;
}

bool TriggerSkill::effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
    return trigger(triggerEvent, room, player, data);
}

bool TriggerSkill::trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &) const{
    return false;
}

TriggerSkill::~TriggerSkill() {
    if (view_as_skill && !view_as_skill->inherits("LuaViewAsSkill"))
        delete view_as_skill;
    events.clear();
}

ScenarioRule::ScenarioRule(Scenario *scenario)
    :TriggerSkill(scenario->objectName())
{
    setParent(scenario);
}

int ScenarioRule::getPriority(TriggerEvent) const{
    return 1;
}

QStringList ScenarioRule::triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer* &ask_who) const{
    ask_who = NULL;
    return QStringList(objectName());
}

MasochismSkill::MasochismSkill(const QString &name)
    : TriggerSkill(name)
{
    events << Damaged;
}

bool MasochismSkill::trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
    DamageStruct damage = data.value<DamageStruct>();
    onDamaged(player, damage);

    return false;
}

PhaseChangeSkill::PhaseChangeSkill(const QString &name)
    : TriggerSkill(name)
{
    events << EventPhaseStart;
}

bool PhaseChangeSkill::trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const{
    return onPhaseChange(player);
}

DrawCardsSkill::DrawCardsSkill(const QString &name, bool is_initial)
    : TriggerSkill(name), is_initial(is_initial)
{
    if (is_initial)
        events << DrawInitialCards;
    else
        events << DrawNCards;
}

bool DrawCardsSkill::trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
    int n = data.toInt();
    data = getDrawNum(player, n);
    return false;
}

GameStartSkill::GameStartSkill(const QString &name)
    : TriggerSkill(name)
{
    events << GameStart;
}

bool GameStartSkill::trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const{
    onGameStart(player);
    return false;
}

SPConvertSkill::SPConvertSkill(const QString &from, const QString &to)
    : GameStartSkill(QString("cv_%1").arg(from)), from(from), to(to)
{
    to_list = to.split("+");
}

bool SPConvertSkill::triggerable(const ServerPlayer *target) const{
    if (target == NULL) return false;
    if (!Config.value("EnableSPConvert", true).toBool()) return false;
    if (Config.EnableHegemony) return false;
    if (!isNormalGameMode(Config.GameMode)) return false;
    bool available = false;
    foreach (QString to_gen, to_list) {
        const General *gen = Sanguosha->getGeneral(to_gen);
        if (gen && !Config.value("Banlist/Roles", "").toStringList().contains(to_gen)
            && !Sanguosha->getBanPackages().contains(gen->getPackage())) {
            available = true;
            break;
        }
    }
    return GameStartSkill::triggerable(target)
           && (target->getGeneralName() == from || target->getGeneral2Name() == from) && available;
}

void SPConvertSkill::onGameStart(ServerPlayer *player) const{
    Room *room = player->getRoom();
    QStringList choicelist;
    foreach (QString to_gen, to_list) {
        const General *gen = Sanguosha->getGeneral(to_gen);
        if (gen && !Config.value("Banlist/Roles", "").toStringList().contains(to_gen)
            && !Sanguosha->getBanPackages().contains(gen->getPackage()))
            choicelist << to_gen;
    }
    QString data = choicelist.join("\\,\\");
    if (choicelist.length() >= 2)
        data.replace("\\,\\" + choicelist.last(), "\\or\\" + choicelist.last());
    if (player->askForSkillInvoke(objectName(), data)) {
        QString to_cv;
        AI *ai = player->getAI();
        if (ai)
            to_cv = room->askForChoice(player, objectName(), choicelist.join("+"));
        else
            to_cv = choicelist.length() == 1 ? choicelist.first() : room->askForGeneral(player, choicelist.join("+"));
        bool isSecondaryHero = (player->getGeneralName() != from && player->getGeneral2Name() == from);

        room->changeHero(player, to_cv, true, false, isSecondaryHero);

        const General *general = Sanguosha->getGeneral(to_cv);
        const QString kingdom = general->getKingdom();
        if (!isSecondaryHero && kingdom != "kami" && kingdom != player->getKingdom())
            room->setPlayerProperty(player, "kingdom", kingdom);
    }
}

ProhibitSkill::ProhibitSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
}

DistanceSkill::DistanceSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
}

MaxCardsSkill::MaxCardsSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
}

int MaxCardsSkill::getExtra(const Player *) const{
    return 0;
}

int MaxCardsSkill::getFixed(const Player *) const{
    return -1;
}

TargetModSkill::TargetModSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
    pattern = "Slash";
}

QString TargetModSkill::getPattern() const{
    return pattern;
}

int TargetModSkill::getResidueNum(const Player *, const Card *) const{
    return 0;
}

int TargetModSkill::getDistanceLimit(const Player *, const Card *) const{
    return 0;
}

int TargetModSkill::getExtraTargetNum(const Player *, const Card *) const{
    return 0;
}

SlashNoDistanceLimitSkill::SlashNoDistanceLimitSkill(const QString &skill_name)
    : TargetModSkill(QString("#%1-slash-ndl").arg(skill_name)), name(skill_name)
{
}

InvaliditySkill::InvaliditySkill(const QString &name)
    : Skill(name)
{
}

int SlashNoDistanceLimitSkill::getDistanceLimit(const Player *, const Card *card) const{
    if (card->getSkillName() == name)
        return 1000;
    else
        return 0;
}

FakeMoveSkill::FakeMoveSkill(const QString &name)
    : TriggerSkill(QString("#%1-fake-move").arg(name)), name(name)
{
    events << BeforeCardsMove << CardsMoveOneTime;
    frequency = Compulsory;
}

int FakeMoveSkill::getPriority(TriggerEvent) const{
    return 10;
}

QStringList FakeMoveSkill::triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer* &) const {
    QString flag = QString("%1_InTempMoving").arg(name);

    foreach (ServerPlayer *p, room->getAllPlayers())
        if (p->hasFlag(flag)) return QStringList(objectName());
    return QStringList();
}

bool FakeMoveSkill::trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &) const{
    return true;
}

DetachEffectSkill::DetachEffectSkill(const QString &skillname, const QString &pilename)
    : TriggerSkill(QString("#%1-clear").arg(skillname)), name(skillname), pile_name(pilename)
{
    events << EventLoseSkill;
    frequency = Compulsory;
}

bool DetachEffectSkill::triggerable(const ServerPlayer *target) const{
    return target != NULL;
}

bool DetachEffectSkill::trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
    if (data.toString() == name) {
        if (!pile_name.isEmpty())
            player->clearOnePrivatePile(pile_name);
        else
            onSkillDetached(room, player);
    }
    return false;
}

void DetachEffectSkill::onSkillDetached(Room *, ServerPlayer *) const{
}

WeaponSkill::WeaponSkill(const QString &name)
    : TriggerSkill(name)
{
}

int WeaponSkill::getPriority(TriggerEvent) const{
    return 2;
}

bool WeaponSkill::triggerable(const ServerPlayer *target) const{
    if (target == NULL) return false;
    if (target->getMark("Equips_Nullified_to_Yourself") > 0) return false;
    return target->hasWeapon(objectName());
}

ArmorSkill::ArmorSkill(const QString &name)
    : TriggerSkill(name)
{
}

int ArmorSkill::getPriority(TriggerEvent) const{
    return 2;
}

bool ArmorSkill::triggerable(const ServerPlayer *target) const{
    if (target == NULL)
        return false;
    return target->hasArmorEffect(objectName());
}

TreasureSkill::TreasureSkill(const QString &name)
    : TriggerSkill(name)
{
}

int TreasureSkill::getPriority(TriggerEvent) const{
    return 2;
}

bool TreasureSkill::triggerable(const ServerPlayer *target) const{
    if (target == NULL)
        return false;
    return target->hasTreasure(objectName());
}

MarkAssignSkill::MarkAssignSkill(const QString &mark, int n)
    : GameStartSkill(QString("#%1-%2").arg(mark).arg(n)), mark_name(mark), n(n)
{
    frequency = Compulsory;
}

void MarkAssignSkill::onGameStart(ServerPlayer *player) const{
    player->getRoom()->addPlayerMark(player, mark_name, n);
}

