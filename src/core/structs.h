#ifndef _STRUCTS_H
#define _STRUCTS_H

class Room;
class TriggerSkill;
class Card;
class Slash;

#include "serverplayer.h"

#include "json.h"
#include <QVariant>

struct DamageStruct
{
    enum Nature
    {
        Normal, // normal slash, duel and most damage caused by skill
        Fire, // fire slash, fire attack and few damage skill (IkYeyan, etc)
        Thunder // lightning, thunder slash, and few damage skill (Leiji, etc)
    };

    DamageStruct();
    DamageStruct(const Card *card, ServerPlayer *from, ServerPlayer *to, int damage = 1, Nature nature = Normal);
    DamageStruct(const QString &reason, ServerPlayer *from, ServerPlayer *to, int damage = 1, Nature nature = Normal);

    ServerPlayer *from;
    ServerPlayer *to;
    const Card *card;
    int damage;
    Nature nature;
    bool chain;
    bool transfer;
    bool by_user;
    QString reason;
    QString transfer_reason;
    bool prevented;
    QStringList invoke_skills;

    QString getReason() const;
};

struct CardEffectStruct
{
    CardEffectStruct();

    const Card *card;

    ServerPlayer *from;
    ServerPlayer *to;

    bool multiple; // helper to judge whether the card has multiple targets
    // does not make sense if the card inherits SkillCard
    bool nullified;
};

struct SlashEffectStruct
{
    SlashEffectStruct();

    int jink_num;

    const Card *slash;
    const Card *jink;

    ServerPlayer *from;
    ServerPlayer *to;

    int drank;

    DamageStruct::Nature nature;

    bool nullified;
};

struct CardUseStruct
{
    enum CardUseReason
    {
        CARD_USE_REASON_UNKNOWN = 0x00,
        CARD_USE_REASON_PLAY = 0x01,
        CARD_USE_REASON_RESPONSE = 0x02,
        CARD_USE_REASON_RESPONSE_USE = 0x12
    };

    CardUseStruct();
    CardUseStruct(const Card *card, ServerPlayer *from, QList<ServerPlayer *> to, bool isOwnerUse = true);
    CardUseStruct(const Card *card, ServerPlayer *from, ServerPlayer *target, bool isOwnerUse = true);
    bool isValid(const QString &pattern) const;
    void parse(const QString &str, Room *room);
    bool tryParse(const QVariant &, Room *room);

    const Card *card;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
    bool m_isOwnerUse;
    bool m_addHistory;
    bool m_isHandcard;
    QStringList nullified_list;
};

class CardMoveReason
{
public:
    int m_reason;
    QString m_playerId;
    // the cause (not the source) of the movement, such as "lusu" when "ikyuanjie", or "zhanghe" when "ikmancai"
    QString m_targetId; // To keep this structure lightweight, currently this is only used for UI purpose.
    // It will be set to empty if multiple targets are involved. NEVER use it for trigger condition
    // judgement!!! It will not accurately reflect the real reason.
    QString m_skillName; // skill that triggers movement of the cards, such as "ikhuahuan", "ikyuanjie"
    QString m_eventName; // additional arg such as "lebusishu" on top of "S_REASON_JUDGE"
    QVariant m_extraData; // additional data and will not be parsed to clients
    inline CardMoveReason()
    {
        m_reason = S_REASON_UNKNOWN;
    }
    inline CardMoveReason(int moveReason, QString playerId)
    {
        m_reason = moveReason;
        m_playerId = playerId;
    }

    inline CardMoveReason(int moveReason, QString playerId, QString skillName, QString eventName)
    {
        m_reason = moveReason;
        m_playerId = playerId;
        m_skillName = skillName;
        m_eventName = eventName;
    }

    inline CardMoveReason(int moveReason, QString playerId, QString targetId, QString skillName, QString eventName)
    {
        m_reason = moveReason;
        m_playerId = playerId;
        m_targetId = targetId;
        m_skillName = skillName;
        m_eventName = eventName;
    }

    bool tryParse(const QVariant &);
    QVariant toVariant() const;

    inline bool operator==(const CardMoveReason &other) const
    {
        return m_reason == other.m_reason && m_playerId == other.m_playerId && m_targetId == other.m_targetId
            && m_skillName == other.m_skillName && m_eventName == other.m_eventName;
    }

    static const int S_REASON_UNKNOWN = 0x00;
    static const int S_REASON_USE = 0x01;
    static const int S_REASON_RESPONSE = 0x02;
    static const int S_REASON_DISCARD = 0x03;
    static const int S_REASON_RECAST = 0x04; // ironchain etc.
    static const int S_REASON_PINDIAN = 0x05;
    static const int S_REASON_DRAW = 0x06;
    static const int S_REASON_GOTCARD = 0x07;
    static const int S_REASON_SHOW = 0x08;
    static const int S_REASON_TRANSFER = 0x09;
    static const int S_REASON_PUT = 0x0A;

    //subcategory of use
    static const int S_REASON_LETUSE = 0x11; // use a card when self is not current

    //subcategory of response
    static const int S_REASON_RETRIAL = 0x12;

    //subcategory of discard
    static const int S_REASON_RULEDISCARD = 0x13; //  discard at one's Player::Discard for gamerule
    static const int S_REASON_THROW = 0x23; /*  gamerule(dying or punish)
                                                            as the cost of some skills   */
    static const int S_REASON_DISMANTLE = 0x33; //  one throw card of another
    static const int S_REASON_CHANGE_EQUIP = 0x43; //  replace existed equip // @todo_Slob: TripleSha Only

    //subcategory of gotcard
    static const int S_REASON_GIVE = 0x17; // from one hand to another hand
    static const int S_REASON_EXTRACTION = 0x27; // from another's place to one's hand
    static const int S_REASON_GOTBACK = 0x37; // from placetable to hand
    static const int S_REASON_RECYCLE = 0x47; // from discardpile to hand
    static const int S_REASON_ROB = 0x57; // got a definite card from other's hand
    static const int S_REASON_PREVIEWGIVE = 0x67; // give cards after previewing, i.e. Yiji & Nosmiji

    //subcategory of show
    static const int S_REASON_TURNOVER = 0x18; // show n cards from drawpile
    static const int S_REASON_JUDGE = 0x28; // show a card from drawpile for judge
    static const int S_REASON_PREVIEW = 0x38; // Not done yet, plan for view some cards for self only(guanxing yiji nosmiji)
    static const int S_REASON_DEMONSTRATE = 0x48; // show a card which copy one to move to table

    //subcategory of transfer
    static const int S_REASON_SWAP = 0x19; // exchange card for two players
    static const int S_REASON_MOVE_EQUIP = 0x29; // transfer card from placeequip to placeequip
    static const int S_REASON_OVERRIDE = 0x39; // exchange cards from cards in game
    static const int S_REASON_EXCHANGE_FROM_PILE = 0x49; // exchange cards from cards moved out of game (for ikqiyao only)

    //subcategory of put
    static const int S_REASON_NATURAL_ENTER = 0x1A; //  a card with no-owner move into discardpile
    //  e.g. delayed trick enters discardpile
    static const int S_REASON_REMOVE_FROM_PILE = 0x2A; //  cards moved out of game go back into discardpile
    static const int S_REASON_JUDGEDONE = 0x3A; //  judge card move into discardpile

    static const int S_MASK_BASIC_REASON = 0x0F;
};

struct CardsMoveOneTimeStruct
{
    QList<int> card_ids;
    QList<Player::Place> from_places;
    Player::Place to_place;
    CardMoveReason reason;
    Player *from, *to;
    QStringList from_pile_names;
    QString to_pile_name;

    QList<bool> open; // helper to prevent sending card_id to unrelevant clients
    bool is_last_handcard;
    bool is_last_equip;

    inline void removeCardIds(const QList<int> &to_remove)
    {
        foreach (int id, to_remove) {
            int index = card_ids.indexOf(id);
            if (index != -1) {
                card_ids.removeAt(index);
                from_places.removeAt(index);
                from_pile_names.removeAt(index);
                open.removeAt(index);
            }
        }
    }
};

struct CardsMoveStruct
{
    inline CardsMoveStruct()
    {
        from_place = Player::PlaceUnknown;
        to_place = Player::PlaceUnknown;
        from = NULL;
        to = NULL;
        is_last_handcard = false;
        is_last_equip = false;
    }

    inline CardsMoveStruct(const QList<int> &ids, Player *from, Player *to, Player::Place from_place, Player::Place to_place,
                           CardMoveReason reason)
    {
        this->card_ids = ids;
        this->from_place = from_place;
        this->to_place = to_place;
        this->from = from;
        this->to = to;
        this->reason = reason;
        this->is_last_handcard = false;
        this->is_last_equip = false;
        if (from)
            this->from_player_name = from->objectName();
        if (to)
            this->to_player_name = to->objectName();
    }

    inline CardsMoveStruct(const QList<int> &ids, Player *to, Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids = ids;
        this->from_place = Player::PlaceUnknown;
        this->to_place = to_place;
        this->from = NULL;
        this->to = to;
        this->reason = reason;
        this->is_last_handcard = false;
        this->is_last_equip = false;
        if (to)
            this->to_player_name = to->objectName();
    }

    inline CardsMoveStruct(int id, Player *from, Player *to, Player::Place from_place, Player::Place to_place,
                           CardMoveReason reason)
    {
        this->card_ids << id;
        this->from_place = from_place;
        this->to_place = to_place;
        this->from = from;
        this->to = to;
        this->reason = reason;
        this->is_last_handcard = false;
        this->is_last_equip = false;
        if (from)
            this->from_player_name = from->objectName();
        if (to)
            this->to_player_name = to->objectName();
    }

    inline CardsMoveStruct(int id, Player *to, Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids << id;
        this->from_place = Player::PlaceUnknown;
        this->to_place = to_place;
        this->from = NULL;
        this->to = to;
        this->reason = reason;
        this->is_last_handcard = false;
        this->is_last_equip = false;
        if (to)
            this->to_player_name = to->objectName();
    }

    inline bool operator==(const CardsMoveStruct &other) const
    {
        return from == other.from && from_place == other.from_place && from_pile_name == other.from_pile_name
            && from_player_name == other.from_player_name;
    }

    inline bool operator<(const CardsMoveStruct &other) const
    {
        return from < other.from || from_place < other.from_place || from_pile_name < other.from_pile_name
            || from_player_name < other.from_player_name;
    }

    QList<int> card_ids;
    Player::Place from_place, to_place;
    QString from_player_name, to_player_name;
    QString from_pile_name, to_pile_name;
    Player *from, *to;
    CardMoveReason reason;
    bool open; // helper to prevent sending card_id to unrelevant clients
    bool is_last_handcard;
    bool is_last_equip;
    bool tryParse(const QVariant &);
    QVariant toVariant() const;
    inline bool isRelevant(const Player *player)
    {
        return player != NULL && (from == player || (to == player && to_place != Player::PlaceSpecial));
    }
};

struct CardsShowStruct
{
    inline CardsShowStruct()
    {
        this->card_ids = QList<int>();
        this->from = NULL;
        this->viewer = NULL;
    }

    QList<int> card_ids;
    ServerPlayer *from;
    ServerPlayer *viewer;

    inline CardsShowStruct(ServerPlayer *viewer)
    {
        this->card_ids = QList<int>();
        this->from = NULL;
        this->viewer = viewer;
    }
};

struct DyingStruct
{
    DyingStruct();

    ServerPlayer *who; // who is ask for help
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
};

struct DeathStruct
{
    DeathStruct();

    ServerPlayer *who; // who is dead
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
};

struct RecoverStruct
{
    RecoverStruct(ServerPlayer *who = NULL, const Card *card = NULL, int recover = 1);

    int recover;
    ServerPlayer *who;
    const Card *card;
};

struct PindianStruct
{
    PindianStruct();
    bool isSuccess() const;

    ServerPlayer *from;
    ServerPlayer *to;
    const Card *from_card;
    const Card *to_card;
    int from_number;
    int to_number;
    QString reason;
    bool success;
};

struct JudgeStruct
{
    JudgeStruct();
    bool isGood() const;
    bool isBad() const;
    bool isEffected() const;
    void updateResult();

    bool isGood(const Card *card) const; // For AI

    ServerPlayer *who;
    const Card *card;
    QString pattern;
    bool good;
    QString reason;
    bool time_consuming;
    bool negative;
    bool play_animation;
    ServerPlayer *retrial_by_response; // record whether the current judge card is provided by a response retrial

private:
    enum TrialResult
    {
        TRIAL_RESULT_UNKNOWN,
        TRIAL_RESULT_GOOD,
        TRIAL_RESULT_BAD
    } _m_result;
};

struct PhaseChangeStruct
{
    PhaseChangeStruct();
    Player::Phase from;
    Player::Phase to;
};

struct PhaseStruct
{
    inline PhaseStruct()
    {
        phase = Player::PhaseNone;
        skipped = 0;
    }

    Player::Phase phase;
    int skipped; // 0 - not skipped; 1 - skipped by effect; -1 - skipped by cost
};

struct CardResponseStruct
{
    inline CardResponseStruct()
    {
        m_card = NULL;
        m_who = NULL;
        m_isUse = false;
        m_isRetrial = false;
    }

    inline CardResponseStruct(const Card *card)
    {
        m_card = card;
        m_who = NULL;
        m_isUse = false;
        m_isRetrial = false;
    }

    inline CardResponseStruct(const Card *card, ServerPlayer *who)
    {
        m_card = card;
        m_who = who;
        m_isUse = false;
        m_isRetrial = false;
    }

    inline CardResponseStruct(const Card *card, bool isUse)
    {
        m_card = card;
        m_who = NULL;
        m_isUse = isUse;
        m_isRetrial = false;
    }

    inline CardResponseStruct(const Card *card, ServerPlayer *who, bool isUse)
    {
        m_card = card;
        m_who = who;
        m_isUse = isUse;
        m_isRetrial = false;
    }

    const Card *m_card;
    ServerPlayer *m_who;
    bool m_isUse;
    bool m_isHandcard;
    bool m_isRetrial;
};

struct VersionStruct
{
    VersionStruct();
    QString versionNumber;
    QString updateDate;
    QString url;
};

enum TriggerEvent
{
    NonTrigger,

    GameStart,
    TurnStart,
    EventPhaseStart,
    EventPhaseProceeding,
    EventPhaseEnd,
    EventPhaseChanging,
    EventPhaseSkipping,
    EventPhaseSkipped,

    DrawNCards,
    AfterDrawNCards,
    DrawInitialCards,
    AfterDrawInitialCards,

    PreHpRecover,
    HpRecover,
    PreHpLost,
    HpLost,
    HpChanged,
    MaxHpChanged,

    EventLoseSkill,
    EventAcquireSkill,

    StartJudge,
    AskForRetrial,
    FinishRetrial,
    FinishJudge,

    PindianVerifying,
    Pindian,

    TurnedOver,
    ChainStateChanged,

    EventMarksGot,
    EventMarksLost,

    ConfirmDamage, // confirm the damage's count and damage's nature
    Predamage, // trigger the certain skill -- ikxuwu
    DamageForseen, // the first event in a damage -- ikliefeng ikmiaowu
    DamageCaused, // the moment for -- ikmoguang..
    DamageInflicted, // the moment for -- ikzhihui..
    PreDamageDone, // before reducing Hp
    DamageDone, // it's time to do the damage
    Damage, // the moment for -- iklieren..
    Damaged, // the moment for -- yiji..
    DamageComplete, // the moment for trigger iron chain

    EnterDying,
    Dying,
    QuitDying,
    AskForPeaches,
    AskForPeachesDone,
    Death,
    BuryVictim,
    BeforeGameOverJudge,
    GameOverJudge,
    GameFinished,

    SlashEffected,
    SlashProceed,
    SlashHit,
    SlashMissed,

    JinkEffect,
    NullificationEffect,

    CardAsked,
    PreCardResponded,
    CardResponded,
    BeforeCardsMove, // sometimes we need to record cards before the move
    CardsMoveOneTime,
    CardsShown,

    PreCardUsed, // for AI to filter events only.
    CardUsed,
    TargetSpecifying,
    TargetConfirming,
    TargetSpecified,
    TargetConfirmed,
    TargetCanceled,
    CardEffect, // for AI to filter events only
    CardEffected,
    PostCardEffected,
    CardFinished,
    TrickCardCanceling,
    TrickEffect,
    TrickMissed,

    ChoiceMade,

    StageChange, // For hulao pass only
    FetchDrawPileCard, // For miniscenarios only
    AfterSwapPile,
    ActionedReset, // For 3v3 only
    Debut, // For 1v1 only

    BeforeGameOver, // For achievement only

    TurnBroken, // For the skill 'IkYinzhai'. Do not use it to trigger events

    NumOfEvents
};

Q_DECLARE_METATYPE(DamageStruct)
Q_DECLARE_METATYPE(CardEffectStruct)
Q_DECLARE_METATYPE(SlashEffectStruct)
Q_DECLARE_METATYPE(CardUseStruct)
Q_DECLARE_METATYPE(CardsMoveStruct)
Q_DECLARE_METATYPE(CardsMoveOneTimeStruct)
Q_DECLARE_METATYPE(CardsShowStruct)
Q_DECLARE_METATYPE(DyingStruct)
Q_DECLARE_METATYPE(DeathStruct)
Q_DECLARE_METATYPE(RecoverStruct)
Q_DECLARE_METATYPE(PhaseChangeStruct)
Q_DECLARE_METATYPE(CardResponseStruct)
Q_DECLARE_METATYPE(VersionStruct)
Q_DECLARE_METATYPE(const Card *)
Q_DECLARE_METATYPE(ServerPlayer *)
Q_DECLARE_METATYPE(JudgeStruct *)
Q_DECLARE_METATYPE(PindianStruct *)
#endif
