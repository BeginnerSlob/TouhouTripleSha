#ifndef _SERVER_PLAYER_H
#define _SERVER_PLAYER_H

class Room;
class AI;
class Recorder;

class CardMoveReason;
struct PhaseStruct;

#include "player.h"
#include "protocol.h"
#include "socket.h"

#include <QDateTime>
#include <QSemaphore>

class ServerPlayer : public Player
{
    Q_OBJECT
    Q_PROPERTY(QString ip READ getIp)

public:
    explicit ServerPlayer(Room *room);
    ~ServerPlayer();

    void setSocket(ClientSocket *socket);
    void invoke(const QSanProtocol::AbstractPacket *packet);
    QString reportHeader() const;
    void unicast(const QByteArray &message);
    void drawCard(const Card *card);
    Room *getRoom() const;
    void broadcastSkillInvoke(const Card *card) const;
    void broadcastSkillInvoke(const QString &card_name) const;
    int getRandomHandCardId() const;
    const Card *getRandomHandCard() const;
    void obtainCard(const Card *card, bool unhide = true);
    void throwAllEquips();
    void throwAllHandCards();
    void throwAllHandCardsAndEquips();
    void throwAllCards();
    void bury();
    void throwAllMarks(bool visible_only = true);
    void clearOnePrivatePile(QString pile_name);
    void clearPrivatePiles();
    void drawCards(int n, const QString &reason = QString());
    bool askForSkillInvoke(const QString &skill_name, const QVariant &data = QVariant());
    QList<int> forceToDiscard(const QString &reason, int discard_num, bool include_equip, bool is_discard = true,
                              const QString &pattern = ".");
    QList<int> handCards() const;
    virtual QList<const Card *> getHandcards() const;
    QList<const Card *> getCards(const QString &flags) const;
    DummyCard *wholeHandCards() const;
    bool hasNullification() const;
    bool pindian(ServerPlayer *target, const QString &reason);
    QList<bool> pindian(QList<ServerPlayer *> targets, const QString &reason);
    void turnOver();
    void play(QList<Player::Phase> set_phases = QList<Player::Phase>());
    bool changePhase(Player::Phase from, Player::Phase to);

    QList<Player::Phase> &getPhases();
    void skip(Player::Phase phase, bool isCost = false);
    void insertPhase(Player::Phase phase);
    bool isSkipped(Player::Phase phase);

    void gainMark(const QString &mark, int n = 1);
    void loseMark(const QString &mark, int n = 1);
    void loseAllMarks(const QString &mark_name);

    virtual void addSkill(const QString &skill_name);
    virtual void loseSkill(const QString &skill_name);
    virtual void setGender(int gender);

    void setAI(AI *ai);
    AI *getAI() const;
    AI *getSmartAI() const;

    bool isOnline() const;
    inline bool isOffline() const
    {
        return getState() == "robot" || getState() == "offline";
    }

    virtual int aliveCount(bool includeRemoved = true) const;
    virtual int getHandcardNum() const;
    virtual void removeCard(const Card *card, Place place);
    virtual void addCard(const Card *card, Place place);
    virtual bool isLastHandCard(const Card *card, bool contain = false) const;

    void addVictim(ServerPlayer *victim);
    QList<ServerPlayer *> getVictims() const;

    void startRecord();
    void saveRecord(const QString &filename);

    // 3v3 methods
    void addToSelected(const QString &general);
    QStringList getSelected() const;
    QString findReasonable(const QStringList &generals, bool no_unreasonable = false);
    void clearSelected();

    int getGeneralMaxHp() const;
    int getGeneralStartHp() const;
    virtual QString getGameMode() const;

    QString getIp() const;
    void introduceTo(ServerPlayer *player);
    void marshal(ServerPlayer *player) const;

    void addToPile(const QString &pile_name, const Card *card, bool open = true,
                   QList<ServerPlayer *> open_players = QList<ServerPlayer *>());
    void addToPile(const QString &pile_name, int card_id, bool open = true,
                   QList<ServerPlayer *> open_players = QList<ServerPlayer *>());
    void addToPile(const QString &pile_name, QList<int> card_ids, bool open = true,
                   QList<ServerPlayer *> open_players = QList<ServerPlayer *>());
    void addToPile(const QString &pile_name, QList<int> card_ids, bool open, QList<ServerPlayer *> open_players,
                   CardMoveReason reason);
    void exchangeFreelyFromPrivatePile(const QString &skill_name, const QString &pile_name, int upperlimit = 1000,
                                       bool include_equip = false);
    void updatePile(const QString &pile_name, QList<int> remove, QList<int> append);
    void gainAnExtraTurn();

    void copyFrom(ServerPlayer *sp);

    void startNetworkDelayTest();
    qint64 endNetworkDelayTest();

    //Synchronization helpers
    enum SemaphoreType
    {
        SEMA_MUTEX, // used to protect mutex access to member variables
        SEMA_COMMAND_INTERACTIVE // used to wait for response from client
    };
    inline QSemaphore *getSemaphore(SemaphoreType type)
    {
        return semas[type];
    }
    inline void acquireLock(SemaphoreType type)
    {
        semas[type]->acquire();
    }
    inline bool tryAcquireLock(SemaphoreType type, int timeout = 0)
    {
        return semas[type]->tryAcquire(1, timeout);
    }
    inline void releaseLock(SemaphoreType type)
    {
        semas[type]->release();
    }
    inline void drainLock(SemaphoreType type)
    {
        while (semas[type]->tryAcquire()) {
        }
    }
    inline void drainAllLocks()
    {
        for (int i = 0; i < S_NUM_SEMAPHORES; i++) {
            drainLock((SemaphoreType)i);
        }
    }
    inline QString getClientReplyString()
    {
        return m_clientResponseString;
    }
    inline void setClientReplyString(const QString &val)
    {
        m_clientResponseString = val;
    }
    inline QVariant getClientReply()
    {
        return _m_clientResponse;
    }
    inline void setClientReply(const QVariant &val)
    {
        _m_clientResponse = val;
    }
    unsigned int m_expectedReplySerial; // Suggest the acceptable serial number of an expected response.
    bool m_isClientResponseReady; //Suggest whether a valid player's reponse has been received.
    bool m_isWaitingReply; // Suggest if the server player is waiting for client's response.
    QVariant m_cheatArgs; // Store the cheat code received from client.
    QSanProtocol::CommandType m_expectedReplyCommand; // Store the command to be sent to the client.
    QVariant m_commandArgs; // Store the command args to be sent to the client.

    // static function
    static bool CompareByActionOrder(ServerPlayer *a, ServerPlayer *b);

    // remove QinggangTag and BladeDisableShow
    void slashSettlementFinished(const Card *slash);

protected:
    //Synchronization helpers
    QSemaphore **semas;
    static const int S_NUM_SEMAPHORES;

private:
    ClientSocket *socket;
    QList<const Card *> handcards;
    Room *room;
    AI *ai;
    AI *trust_ai;
    QList<ServerPlayer *> victims;
    Recorder *recorder;
    QList<Phase> phases;
    int _m_phases_index;
    QList<PhaseStruct> _m_phases_state;
    QStringList selected; // 3v3 mode use only
    QDateTime test_time;
    QString m_clientResponseString;
    QVariant _m_clientResponse;

private slots:
    void getMessage(QByteArray request);
    void sendMessage(const QByteArray &message);

signals:
    void disconnected();
    void request_got(const QByteArray &request);
    void message_ready(const QByteArray &msg);
};

#endif
