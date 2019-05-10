#include "card.h"
#include "client.h"
#include "engine.h"
#include "lua-wrapper.h"
#include "room.h"
#include "settings.h"
#include "standard.h"
#include "structs.h"
#include <QFile>

const int Card::S_UNKNOWN_CARD_ID = -1;

const Card::Suit Card::AllSuits[4] = {Card::Spade, Card::Club, Card::Heart, Card::Diamond};

Card::Card(Suit suit, int number, bool target_fixed)
    : target_fixed(target_fixed)
    , mute(false)
    , will_throw(true)
    , has_preact(false)
    , can_recast(false)
    , m_suit(suit)
    , m_number(number)
    , m_id(-1)
{
    handling_method = will_throw ? Card::MethodDiscard : Card::MethodUse;
}

QString Card::getSuitString() const
{
    return Suit2String(getSuit());
}

QString Card::Suit2String(Suit suit)
{
    switch (suit) {
    case Spade:
        return "spade";
    case Heart:
        return "heart";
    case Club:
        return "club";
    case Diamond:
        return "diamond";
    case NoSuitBlack:
        return "no_suit_black";
    case NoSuitRed:
        return "no_suit_red";
    default:
        return "no_suit";
    }
}

Card::Color Card::Suit2Color(Suit suit)
{
    switch (suit) {
    case Spade:
    case Club:
    case NoSuitBlack:
        return Black;
    case Heart:
    case Diamond:
    case NoSuitRed:
        return Red;
    default:
        return Colorless;
    }
}

bool Card::isRed() const
{
    return getColor() == Red;
}

bool Card::isBlack() const
{
    return getColor() == Black;
}

int Card::getId() const
{
    return m_id;
}

void Card::setId(int id)
{
    this->m_id = id;
}

int Card::getEffectiveId() const
{
    if (isVirtualCard()) {
        if (subcards.isEmpty())
            return -1;
        else
            return subcards.first();
    } else
        return m_id;
}

int Card::getNumber() const
{
    if (m_number > 0)
        return m_number;
    if (isVirtualCard()) {
        if (subcardsLength() == 0)
            return 0;
        else {
            int num = 0;
            foreach (int id, subcards) {
                num += Sanguosha->getCard(id)->getNumber();
            }
            return qMin(num, 13);
        }
    } else
        return m_number;
}

void Card::setNumber(int number)
{
    this->m_number = number;
}

QString Card::getNumberString() const
{
    int number = getNumber();
    if (isVirtualCard()) {
        if (subcardsLength() == 0 || subcardsLength() >= 2)
            number = 0;
    }
    if (number == 10)
        return "10";
    else {
        static const char *number_string = "-A23456789-JQK";
        return QString(number_string[number]);
    }
}

Card::Suit Card::getSuit() const
{
    static QStringList no_suit_skills;
    if (no_suit_skills.isEmpty())
        no_suit_skills << "iklixin"
                       << "_jnhuoji"
                       << "_thmoji"
                       << "rhhuanjie";
    if (no_suit_skills.contains(m_skillName))
        return NoSuit;
    if (m_suit != NoSuit && m_suit != SuitToBeDecided)
        return m_suit;
    if (isVirtualCard()) {
        if (subcardsLength() == 0)
            return NoSuit;
        else if (subcardsLength() == 1)
            return Sanguosha->getCard(subcards.first())->getSuit();
        else {
            Color color = Colorless;
            foreach (int id, subcards) {
                Color color2 = Sanguosha->getCard(id)->getColor();
                if (color == Colorless)
                    color = color2;
                else if (color != color2)
                    return NoSuit;
            }
            return (color == Red) ? NoSuitRed : NoSuitBlack;
        }
    } else
        return m_suit;
}

void Card::setSuit(Suit suit)
{
    this->m_suit = suit;
}

bool Card::sameColorWith(const Card *other) const
{
    return getColor() == other->getColor();
}

Card::Color Card::getColor() const
{
    return Suit2Color(getSuit());
}

bool Card::isEquipped() const
{
    return Self->hasEquip(this);
}

bool Card::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    foreach (QString ptn, patterns)
        if (objectName() == ptn || getType() == ptn || getSubtype() == ptn)
            return true;
    return false;
}

bool Card::CompareByNumber(const Card *a, const Card *b)
{
    static Suit new_suits[] = {Spade, Heart, Club, Diamond, NoSuitBlack, NoSuitRed, NoSuit};
    Suit suit1 = new_suits[a->getSuit()];
    Suit suit2 = new_suits[b->getSuit()];

    if (a->m_number != b->m_number)
        return a->m_number < b->m_number;
    else
        return suit1 < suit2;
}

bool Card::CompareBySuit(const Card *a, const Card *b)
{
    static Suit new_suits[] = {Spade, Heart, Club, Diamond, NoSuitBlack, NoSuitRed, NoSuit};
    Suit suit1 = new_suits[a->getSuit()];
    Suit suit2 = new_suits[b->getSuit()];

    if (suit1 != suit2)
        return suit1 < suit2;
    else
        return a->m_number < b->m_number;
}

bool Card::CompareByType(const Card *a, const Card *b)
{
    int order1 = a->getTypeId();
    int order2 = b->getTypeId();
    if (order1 != order2)
        return order1 < order2;
    else {
        static QStringList basic;
        if (basic.isEmpty())
            basic << "slash"
                  << "thunder_slash"
                  << "fire_slash"
                  << "jink"
                  << "peach"
                  << "analeptic";
        switch (a->getTypeId()) {
        case TypeBasic: {
            foreach (QString object_name, basic) {
                if (a->objectName() == object_name) {
                    if (b->objectName() == object_name)
                        return CompareBySuit(a, b);
                    else
                        return true;
                }
                if (b->objectName() == object_name)
                    return false;
            }
            return CompareBySuit(a, b);
            break;
        }
        case TypeTrick: {
            if (a->objectName() == b->objectName())
                return CompareBySuit(a, b);
            else
                return a->objectName() < b->objectName();
            break;
        }
        case TypeEquip: {
            const EquipCard *eq_a = qobject_cast<const EquipCard *>(a->getRealCard());
            const EquipCard *eq_b = qobject_cast<const EquipCard *>(b->getRealCard());
            if (eq_a->location() == eq_b->location()) {
                if (eq_a->isKindOf("Weapon")) {
                    const Weapon *wep_a = qobject_cast<const Weapon *>(a->getRealCard());
                    const Weapon *wep_b = qobject_cast<const Weapon *>(b->getRealCard());
                    if (wep_a->getRange() == wep_b->getRange())
                        return CompareBySuit(a, b);
                    else
                        return wep_a->getRange() < wep_b->getRange();
                } else {
                    if (a->objectName() == b->objectName())
                        return CompareBySuit(a, b);
                    else
                        return a->objectName() < b->objectName();
                }
            } else {
                return eq_a->location() < eq_b->location();
            }
            break;
        }
        default:
            return CompareBySuit(a, b);
        }
    }
}

bool Card::isNDTrick() const
{
    return getTypeId() == TypeTrick && !isKindOf("DelayedTrick");
}

QString Card::getPackage() const
{
    if (parent())
        return parent()->objectName();
    else
        return QString();
}

QString Card::getFullName(bool include_suit) const
{
    QString name = getName();
    if (include_suit) {
        QString suit_name = Sanguosha->translate(getSuitString());
        return QString("%1%2 %3").arg(suit_name).arg(getNumberString()).arg(name);
    } else
        return QString("%1 %2").arg(getNumberString()).arg(name);
}

QString Card::getLogName() const
{
    QString suit_char;
    QString number_string;

    switch (getSuit()) {
    case Spade:
    case Heart:
    case Club:
    case Diamond: {
        suit_char = QString("<img src='image/system/log/%1.png' height = 12/>").arg(getSuitString());
        break;
    }
    case NoSuitRed: {
        suit_char = tr("NoSuitRed");
        break;
    }
    case NoSuitBlack: {
        suit_char = tr("NoSuitBlack");
        break;
    }
    case NoSuit: {
        suit_char = tr("NoSuit");
        break;
    }
    default:
        break;
    }

    if (m_number > 0 && m_number <= 13)
        number_string = getNumberString();

    return QString("%1[%2%3]").arg(getName()).arg(suit_char).arg(number_string);
}

QString Card::getCommonEffectName() const
{
    return QString();
}

QString Card::getName() const
{
    return Sanguosha->translate(objectName());
}

QString Card::getSkillName(bool removePrefix) const
{
    if (m_skillName.startsWith("_") && removePrefix)
        return m_skillName.mid(1);
    else
        return m_skillName;
}

void Card::setSkillName(const QString &name)
{
    this->m_skillName = name;
}

QString Card::getDescription() const
{
    QString desc = Sanguosha->translate(":" + objectName());
    if (desc.startsWith(":"))
        desc = QString();
    else if (desc.startsWith("[NoAutoRep]"))
        desc = desc.mid(11);
    else {
        if (Config.value("AutoSkillTypeColorReplacement", true).toBool()) {
            QMap<QString, QColor> skilltype_color_map = Sanguosha->getSkillTypeColorMap();
            foreach (QString skill_type, skilltype_color_map.keys()) {
                QString type_name = Sanguosha->translate(skill_type);
                QString color_name = skilltype_color_map[skill_type].name();
                desc.replace(type_name, QString("<font color=%1><b>%2</b></font>").arg(color_name).arg(type_name));
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
                desc.replace(suit_char, colored_suit_char);
                desc.replace(suit_name, colored_suit_char);
            }
        }
    }
    desc.replace("\n", "<br/>");
    return tr("<b>[%1]</b> %2").arg(getName()).arg(desc);
}

QString Card::toString(bool hidden) const
{
    Q_UNUSED(hidden);
    if (!isVirtualCard())
        return QString::number(m_id);
    else
        return QString("%1:%2[%3:%4]=%5")
            .arg(objectName())
            .arg(m_skillName)
            .arg(getSuitString())
            .arg(getNumberString())
            .arg(subcardString());
}

QString Card::subcardString() const
{
    if (subcards.isEmpty())
        return ".";

    QStringList str;
    foreach (int subcard, subcards)
        str << QString::number(subcard);

    return str.join("+");
}

void Card::addSubcards(const QList<const Card *> &cards)
{
    foreach (const Card *card, cards)
        subcards.append(card->getId());
}

void Card::addSubcards(const QList<int> &subcards_list)
{
    subcards.append(subcards_list);
}

int Card::subcardsLength() const
{
    return subcards.length();
}

bool Card::isVirtualCard() const
{
    return m_id < 0;
}

const Card *Card::Parse(const QString &str)
{
    static QMap<QString, Card::Suit> suit_map;
    if (suit_map.isEmpty()) {
        suit_map.insert("spade", Card::Spade);
        suit_map.insert("club", Card::Club);
        suit_map.insert("heart", Card::Heart);
        suit_map.insert("diamond", Card::Diamond);
        suit_map.insert("no_suit_red", Card::NoSuitRed);
        suit_map.insert("no_suit_black", Card::NoSuitBlack);
        suit_map.insert("no_suit", Card::NoSuit);
    }

    if (str.startsWith(QChar('@'))) {
        // skill card
        QRegExp pattern("@(\\w+)=([^:]+)(:.+)?");
        QRegExp ex_pattern("@(\\w*)\\[(\\w+):(.+)\\]=([^:]+)(:.+)?");

        QStringList texts;
        QString card_name, card_suit, card_number;
        QStringList subcard_ids;
        QString subcard_str;
        QString user_string;

        if (pattern.exactMatch(str)) {
            texts = pattern.capturedTexts();
            card_name = texts.at(1);
            subcard_str = texts.at(2);
            user_string = texts.at(3);
        } else if (ex_pattern.exactMatch(str)) {
            texts = ex_pattern.capturedTexts();
            card_name = texts.at(1);
            card_suit = texts.at(2);
            card_number = texts.at(3);
            subcard_str = texts.at(4);
            user_string = texts.at(5);
        } else
            return NULL;

        if (subcard_str != ".")
            subcard_ids = subcard_str.split("+");

        SkillCard *card = Sanguosha->cloneSkillCard(card_name);

        if (card == NULL)
            return NULL;

        card->addSubcards(StringList2IntList(subcard_ids));

        // skill name
        // @todo: This is extremely dirty and would cause endless troubles.
        QString skillName = card->getSkillName();
        if (skillName.isNull()) {
            skillName = card_name.remove("Card").toLower();
            card->setSkillName(skillName);
        }
        if (!card_suit.isEmpty())
            card->setSuit(suit_map.value(card_suit, Card::NoSuit));

        if (!card_number.isEmpty()) {
            int number = 0;
            if (card_number == "A")
                number = 1;
            else if (card_number == "J")
                number = 11;
            else if (card_number == "Q")
                number = 12;
            else if (card_number == "K")
                number = 13;
            else
                number = card_number.toInt();

            card->setNumber(number);
        }

        if (!user_string.isEmpty()) {
            user_string.remove(0, 1);
            card->setUserString(user_string);
        }
        card->deleteLater();
        return card;
    } else if (str.startsWith(QChar('$'))) {
        QString copy = str;
        copy.remove(QChar('$'));
        QStringList card_strs = copy.split("+");
        DummyCard *dummy = new DummyCard(StringList2IntList(card_strs));
        dummy->deleteLater();
        return dummy;
    } else if (str.startsWith(QChar('#'))) {
        LuaSkillCard *new_card = LuaSkillCard::Parse(str);
        new_card->deleteLater();
        return new_card;
    } else if (str.contains(QChar('='))) {
        QRegExp pattern("(\\w+):(\\w*)\\[(\\w+):(.+)\\]=(.+)");
        if (!pattern.exactMatch(str))
            return NULL;

        QStringList texts = pattern.capturedTexts();
        QString card_name = texts.at(1);
        QString m_skillName = texts.at(2);
        QString suit_string = texts.at(3);
        QString number_string = texts.at(4);
        QString subcard_str = texts.at(5);
        QStringList subcard_ids;
        if (subcard_str != ".")
            subcard_ids = subcard_str.split("+");

        Suit suit = Card::NoSuit;
        DummyCard *dummy = new DummyCard(StringList2IntList(subcard_ids));
        if (suit_string == "to_be_decided")
            suit = dummy->getSuit();
        else
            suit = suit_map.value(suit_string, Card::NoSuit);
        delete dummy;

        int number = 0;
        if (number_string == "A")
            number = 1;
        else if (number_string == "J")
            number = 11;
        else if (number_string == "Q")
            number = 12;
        else if (number_string == "K")
            number = 13;
        else
            number = number_string.toInt();

        Card *card = Sanguosha->cloneCard(card_name, suit, number);
        if (card == NULL)
            return NULL;

        card->addSubcards(StringList2IntList(subcard_ids));
        card->setSkillName(m_skillName);
        card->deleteLater();
        return card;
    } else {
        bool ok;
        int card_id = str.toInt(&ok);
        if (ok)
            return Sanguosha->getCard(card_id)->getRealCard();
        else
            return NULL;
    }
}

Card *Card::Clone(const Card *card)
{
    Card::Suit suit = card->getSuit();
    int number = card->getNumber();

    QObject *card_obj = NULL;
    if (card->isKindOf("LuaBasicCard")) {
        const LuaBasicCard *lcard = qobject_cast<const LuaBasicCard *>(card);
        Q_ASSERT(lcard != NULL);
        card_obj = lcard->clone();
    } else if (card->isKindOf("LuaTrickCard")) {
        const LuaTrickCard *lcard = qobject_cast<const LuaTrickCard *>(card);
        Q_ASSERT(lcard != NULL);
        card_obj = lcard->clone();
    } else if (card->isKindOf("LuaWeapon")) {
        const LuaWeapon *lcard = qobject_cast<const LuaWeapon *>(card);
        Q_ASSERT(lcard != NULL);
        card_obj = lcard->clone();
    } else if (card->isKindOf("LuaArmor")) {
        const LuaArmor *lcard = qobject_cast<const LuaArmor *>(card);
        Q_ASSERT(lcard != NULL);
        card_obj = lcard->clone();
    } else {
        const QMetaObject *meta = card->metaObject();
        card_obj = meta->newInstance(Q_ARG(Card::Suit, suit), Q_ARG(int, number));
    }
    if (card_obj) {
        Card *new_card = qobject_cast<Card *>(card_obj);
        new_card->setId(card->getId());
        new_card->setObjectName(card->objectName());
        new_card->addSubcard(card->getId());
        return new_card;
    } else
        return NULL;
}

bool Card::targetFixed() const
{
    // for JnXianmao ======
    if (Self && Self->hasFlag("JnXianmaoUsed"))
        return false;
    // ====================
    // for RhZhangchi =====
    if (Self && Self->hasFlag("RhZhangchiUsed"))
        return false;
    // ====================
    if (Self && Self->hasFlag("ThChouceUse") && !isKindOf("SkillCard"))
        return false;
    return target_fixed;
}

bool Card::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse") && !isKindOf("SkillCard"))
        return targets.length() == 1;
    if (targetFixed())
        return true;
    else
        return !targets.isEmpty();
}

bool Card::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasFlag("ThChouceUse") && !isKindOf("SkillCard"))
        return targets.isEmpty();
    return targets.isEmpty() && to_select != Self;
}

bool Card::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const
{
    // for JnXianmao ======
    if (Self->hasFlag("JnXianmaoUsed")) {
        bool canSelect = targets.isEmpty() && to_select->hasFlag("JnXianmaoTarget");
        maxVotes = canSelect ? 1 : 0;
        return canSelect;
    }
    // ====================
    // for RhZhangchi =====
    if (Self->hasFlag("RhZhangchiUsed")) {
        bool canSelect = targets.isEmpty() && to_select->isAdjacentTo(Self);
        maxVotes = canSelect ? 1 : 0;
        return canSelect;
    }
    // ====================
    bool canSelect = targetFilter(targets, to_select, Self);
    maxVotes = canSelect ? 1 : 0;
    return canSelect;
}

void Card::doPreAction(Room *, const CardUseStruct &) const
{
}

void Card::onUse(Room *room, const CardUseStruct &use) const
{
    CardUseStruct card_use = use;

    if (use.from->hasFlag("ThChouceUse") && !isKindOf("SkillCard")) {
        room->setPlayerFlag(use.from, "-ThChouceUse");
        int n = use.from->getMark("ThChouce");
        if (n > 0)
            room->removePlayerCardLimitation(use.from, "use", QString("^SkillCard|.|1~%1$0").arg(n));

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = use.from;
        log.arg = "thchouce";
        room->sendLog(log);

        use.card->setFlags("thchouce_use");
    }

    room->sortByActionOrder(card_use.to);

    QList<ServerPlayer *> targets = card_use.to;
    if (room->getMode() == "06_3v3" && (isKindOf("AOE") || isKindOf("GlobalEffect")))
        room->reverseFor3v3(this, card_use.from, targets);
    card_use.to = targets;

    bool hidden = (card_use.card->getTypeId() == TypeSkill && !card_use.card->willThrow());

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();
    thread->trigger(PreCardUsed, room, card_use.from, data);
    card_use = data.value<CardUseStruct>();

    LogMessage log;
    log.from = card_use.from;
    if (!card_use.card->targetFixed() || card_use.to.length() > 1 || !card_use.to.contains(card_use.from))
        log.to = card_use.to;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString(hidden);
    room->sendLog(log);

    if (card_use.card->isKindOf("Collateral")) { // put it here for I don't wanna repeat these codes in Card::onUse
        ServerPlayer *victim = card_use.to.first()->tag["collateralVictim"].value<ServerPlayer *>();
        if (victim) {
            LogMessage log;
            log.type = "#CollateralSlash";
            log.from = card_use.from;
            log.to << victim;
            room->sendLog(log);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, card_use.to.first()->objectName(), victim->objectName());
        } else {
            LogMessage log;
            log.type = "#CollateralNoSlash";
            log.from = card_use.to.first();
            room->sendLog(log);
        }
    } else if (card_use.card->isKindOf("FeintAttack")) { // put it here for I don't wanna repeat these codes in Card::onUse
        ServerPlayer *victim = card_use.to.first()->tag["feintTarget"].value<ServerPlayer *>();
        if (victim) {
            LogMessage log;
            log.type = "#FeintAttackWest";
            log.from = card_use.from;
            log.to << victim;
            room->sendLog(log);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, card_use.to.first()->objectName(), victim->objectName());
        } else {
            LogMessage log;
            log.type = "#FeintAttackNoWest";
            log.from = card_use.to.first();
            room->sendLog(log);
        }
    }

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    if (card_use.card->isVirtualCard())
        used_cards.append(card_use.card->getSubcards());
    else
        used_cards << card_use.card->getEffectiveId();

    if (card_use.card->getTypeId() != TypeSkill) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, card_use.from->objectName(), QString(),
                              card_use.card->getSkillName(), QString());
        if (card_use.to.size() == 1)
            reason.m_targetId = card_use.to.first()->objectName();
        reason.m_extraData = QVariant::fromValue(card_use.card);
        foreach (int id, used_cards) {
            CardsMoveStruct move(id, NULL, Player::PlaceTable, reason);
            moves.append(move);
        }
        room->moveCardsAtomic(moves, true);
        card_use.card = reason.m_extraData.value<const Card *>();
    } else if (card_use.card->willThrow()) {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(),
                              card_use.card->getSkillName(), QString());
        room->moveCardTo(this, card_use.from, NULL, Player::DiscardPile, reason, true);
    }

    data = QVariant::fromValue(card_use);
    thread->trigger(CardUsed, room, card_use.from, data);
    card_use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, card_use.from, data);
}

void Card::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    foreach (ServerPlayer *target, targets)
        room->addPlayerMark(target, "cardEffect_" + toString());
    foreach (ServerPlayer *target, targets) {
        room->removePlayerMark(target, "cardEffect_" + toString());
        bool null_heg = target->getMark("cardNullifyHeg_" + toString()) > 0;
        if (null_heg)
            room->removePlayerMark(target, "cardNullifyHeg_" + toString());

        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        effect.nullified = null_heg || (all_nullified || nullified_list.contains(target->objectName()));

        room->cardEffect(effect);
    }
    foreach (ServerPlayer *p, room->getAlivePlayers())
        room->setPlayerMark(p, "cardEffect_" + toString(), 0);

    if (room->getCardPlace(getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        if (targets.size() == 1)
            reason.m_targetId = targets.first()->objectName();
        reason.m_extraData = QVariant::fromValue(this);
        room->moveCardTo(this, source, NULL, Player::DiscardPile, reason, true);
    }
}

void Card::onEffect(const CardEffectStruct &) const
{
}

bool Card::isCancelable(const CardEffectStruct &) const
{
    return false;
}

void Card::addSubcard(int card_id)
{
    if (card_id < 0)
        qWarning("%s", qPrintable(tr("Subcard must not be virtual card!")));
    else
        subcards << card_id;
}

void Card::addSubcard(const Card *card)
{
    addSubcard(card->getEffectiveId());
}

QList<int> Card::getSubcards() const
{
    return subcards;
}

void Card::clearSubcards()
{
    subcards.clear();
}

bool Card::isAvailable(const Player *player) const
{
    if (player->hasFlag("ThChouceUse") && !isKindOf("SkillCard"))
        return !player->isCardLimited(this, Card::MethodUse);
    return !player->isCardLimited(this, handling_method) || (can_recast && !player->isCardLimited(this, Card::MethodRecast));
}

const Card *Card::validate(CardUseStruct &) const
{
    return this;
}

const Card *Card::validateInResponse(ServerPlayer *) const
{
    return this;
}

bool Card::isMute() const
{
    return mute;
}

bool Card::willThrow() const
{
    return will_throw;
}

bool Card::canRecast() const
{
    return can_recast;
}

void Card::setCanRecast(bool can)
{
    can_recast = can;
}

bool Card::hasPreAction() const
{
    return has_preact;
}

Card::HandlingMethod Card::getHandlingMethod() const
{
    return handling_method;
}

void Card::setFlags(const QString &flag) const
{
    static char symbol_c = '-';

    if (flag.isEmpty())
        return;
    else if (flag == ".")
        flags.clear();
    else if (flag.startsWith(symbol_c)) {
        QString copy = flag;
        copy.remove(symbol_c);
        flags.removeOne(copy);
    } else if (!flags.contains(flag))
        flags << flag;
}

bool Card::hasFlag(const QString &flag) const
{
    return flags.contains(flag);
}

void Card::clearFlags() const
{
    flags.clear();
}

void Card::setTag(const QString &key, const QVariant &data) const
{
    tag[key] = data;
}

void Card::removeTag(const QString &key) const
{
    tag.remove(key);
}

// ---------   Skill card     ------------------

SkillCard::SkillCard()
    : Card(NoSuit, 0)
{
}

void SkillCard::setUserString(const QString &user_string)
{
    this->user_string = user_string;
}

QString SkillCard::getUserString() const
{
    return user_string;
}

QString SkillCard::getType() const
{
    return "skill_card";
}

QString SkillCard::getSubtype() const
{
    return "skill_card";
}

Card::CardType SkillCard::getTypeId() const
{
    return Card::TypeSkill;
}

QString SkillCard::toString(bool hidden) const
{
    QString str;
    if (!hidden)
        str = QString("@%1[%2:%3]=%4")
                  .arg(metaObject()->className())
                  .arg(getSuitString())
                  .arg(getNumberString())
                  .arg(subcardString());
    else
        str = QString("@%1[no_suit:-]=.").arg(metaObject()->className());

    if (!hidden && !user_string.isEmpty())
        return QString("%1:%2").arg(str).arg(user_string);
    else
        return str;
}

// ---------- Dummy card      -------------------

DummyCard::DummyCard()
    : SkillCard()
{
    target_fixed = true;
    handling_method = Card::MethodNone;
    setObjectName("dummy");
}

DummyCard::DummyCard(const QList<int> &subcards)
    : SkillCard()
{
    target_fixed = true;
    handling_method = Card::MethodNone;
    setObjectName("dummy");
    foreach (int id, subcards)
        this->subcards.append(id);
}

QString DummyCard::getType() const
{
    return "dummy_card";
}

QString DummyCard::getSubtype() const
{
    return "dummy_card";
}

QString DummyCard::toString(bool hidden) const
{
    Q_UNUSED(hidden)
    return "$" + subcardString();
}
