#ifndef _ROOM_SCENE_H
#define _ROOM_SCENE_H

#include "aux-skills.h"
#include "card.h"
#include "chatwidget.h"
#include "client.h"
#include "clientlogbox.h"
#include "dashboard.h"
#include "photo.h"
#include "qsanbutton.h"
#include "skin-bank.h"
#include "sprite.h"
#include "table-pile.h"

class BubbleChatBox;
class Button;
class CardContainer;
class ChooseOptionsBox;
class ChooseSuitBox;
class ChooseTriggerOrderBox;
class GuanxingBox;
class PlayerCardBox;
class QGroupBox;
class QSanButton;
class QTimer;
class Window;
struct RoomLayout;

#include <QDialog>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsWidget>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMutex>
#include <QSpinBox>
#include <QStack>
#include <QTableWidget>
#include <QTextEdit>
#include <QThread>
#ifdef Q_OS_ANDROID
#include <QToolTip>
#endif

class ScriptExecutor : public QDialog
{
    Q_OBJECT

public:
    ScriptExecutor(QWidget *parent);

public slots:
    void doScript();
};

class DeathNoteDialog : public QDialog
{
    Q_OBJECT

public:
    DeathNoteDialog(QWidget *parent);

protected:
    virtual void accept();

private:
    QComboBox *killer, *victim;
};

class DamageMakerDialog : public QDialog
{
    Q_OBJECT

public:
    DamageMakerDialog(QWidget *parent);

protected:
    virtual void accept();

private:
    QComboBox *damage_source;
    QComboBox *damage_target;
    QComboBox *damage_nature;
    QSpinBox *damage_point;

    void fillComboBox(QComboBox *ComboBox);

private slots:
    void disableSource();
};

class KOFOrderBox : public QGraphicsPixmapItem
{
public:
    KOFOrderBox(bool self, QGraphicsScene *scene);
    void revealGeneral(const QString &name);
    void killPlayer(const QString &general_name);

private:
    QSanSelectableItem *avatars[3];
    int revealed;
};

class ReplayerControlBar : public QGraphicsObject
{
    Q_OBJECT

public:
    ReplayerControlBar(Dashboard *dashboard);
    static QString FormatTime(int secs);
    virtual QRectF boundingRect() const;

public slots:
    void setTime(int secs);
    void setSpeed(qreal speed);

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    static const int S_BUTTON_GAP = 3;
    static const int S_BUTTON_WIDTH = 25;
    static const int S_BUTTON_HEIGHT = 21;

private:
    QLabel *time_label;
    QString duration_str;
    qreal speed;
};

class RoomScene : public QGraphicsScene
{
    Q_OBJECT

public:
    enum ShefuAskState
    {
        ShefuAskAll,
        ShefuAskNecessary,
        ShefuAskNone
    };

    RoomScene(QMainWindow *main_window);
    ~RoomScene();
    void changeTextEditBackground();
    void adjustItems();
    void showIndicator(const QString &from, const QString &to);
    void showPromptBox();
    static void FillPlayerNames(QComboBox *ComboBox, bool add_none);
    void updateTable();
    void updateVolumeConfig();
    inline QMainWindow *mainWindow()
    {
        return main_window;
    }

    inline bool isCancelButtonEnabled() const
    {
        return cancel_button != NULL && cancel_button->isEnabled();
    }
    inline void setIkGuihuoLog(const QString &log)
    {
        ikguihuo_log = log;
    }

    bool isGameStarted() const
    {
        return game_started;
    }

    inline Dashboard *getDashboard()
    {
        return dashboard;
    }

    // for Android privat->public
    void chooseSkillButton();

    bool m_skillButtonSank;
    ShefuAskState m_ShefuAskState;

public slots:
    void addPlayer(ClientPlayer *player);
    void removePlayer(const QString &player_name);
    void loseCards(int moveId, QList<CardsMoveStruct> moves);
    void getCards(int moveId, QList<CardsMoveStruct> moves);
    void keepLoseCardLog(const CardsMoveStruct &move);
    void keepGetCardLog(const CardsMoveStruct &move);
    // choice dialog
    void chooseGeneral(const QStringList &generals);
    void chooseSuit(const QStringList &suits);
    void chooseCard(const ClientPlayer *playerName, const QString &flags, const QString &reason, bool handcard_visible,
                    Card::HandlingMethod method, QList<int> disabled_ids);
    void chooseKingdom(const QStringList &kingdoms);
    void chooseOption(const QString &skillName, const QStringList &options);
    void chooseOrder(QSanProtocol::Game3v3ChooseOrderCommand reason);
    void chooseRole(const QString &scheme, const QStringList &roles);
    void chooseDirection();
    void chooseTriggerOrder(const QString &reason, const QStringList &options, const bool optional);

    void bringToFront(QGraphicsItem *item);
    void arrangeSeats(const QList<const ClientPlayer *> &seats);
    void toggleDiscards();
    void enableTargets(const Card *card);
    void useSelectedCard();
    void updateStatus(Client::Status oldStatus, Client::Status newStatus);
    void killPlayer(const QString &who);
    void revivePlayer(const QString &who);
    void showServerInformation();
    void surrender();
    void saveReplayRecord();
    void makeDamage();
    void makeKilling();
    void makeReviving();
    void doScript();
    void viewGenerals(const QString &reason, const QStringList &names);
    void viewDistance();

    void handleGameEvent(const QVariant &arg);

    void doOkButton();
    void doCancelButton();
    void doDiscardButton();

    void removeLightBox();

    inline QPointF tableCenterPos()
    {
        return m_tableCenterPos;
    }

    void setChatBoxVisibleSlot();
    void pause();

    void addRobot();
    void doAddRobotAction();
    void fillRobots();

    void onAutoSpeakTimeOut();

#ifdef Q_OS_ANDROID
    void onPressHoldTimeOut();
#endif

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    //this method causes crashes
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QMutex m_roomMutex;
    QMutex m_zValueMutex;

private:
    void _getSceneSizes(QSize &minSize, QSize &maxSize);
    bool _shouldIgnoreDisplayMove(CardsMoveStruct &movement);
    bool _processCardsMove(CardsMoveStruct &move, bool isLost);
    bool _m_isInDragAndUseMode;
    bool _m_superDragStarted;
    const QSanRoomSkin::RoomLayout *_m_roomLayout;
    const QSanRoomSkin::PhotoLayout *_m_photoLayout;
    const QSanRoomSkin::CommonLayout *_m_commonLayout;
    const QSanRoomSkin *_m_roomSkin;
    QGraphicsItem *_m_last_front_item;
    double _m_last_front_ZValue;
    GenericCardContainer *_getGenericCardContainer(Player::Place place, Player *player);
    QMap<int, QList<QList<CardItem *> > > _m_cardsMoveStash;
    Button *add_robot, *start_game, *return_to_main_menu;
    QList<Photo *> photos;
    QMap<QString, Photo *> name2photo;
    Dashboard *dashboard;
    TablePile *m_tablePile;
    QMainWindow *main_window;
    QSanButton *ok_button, *cancel_button, *discard_button;
    QSanButton *trust_button;
    QMenu *miscellaneous_menu, *change_general_menu;
    Window *prompt_box;
    Window *pindian_box;
    CardItem *pindian_from_card, *pindian_to_card;
    QGraphicsItem *control_panel;
    QMap<PlayerCardContainer *, const ClientPlayer *> item2player;
    QDialog *m_choiceDialog; // Dialog for choosing generals, suits, card/equip, or kingdoms

    QGraphicsRectItem *pausing_item;
    QGraphicsSimpleTextItem *pausing_text;

    QString ikguihuo_log;

    QList<QGraphicsPixmapItem *> role_items;
    CardContainer *card_container;

    QList<QSanSkillButton *> m_skillButtons;

    ResponseSkill *response_skill;
    ShowOrPindianSkill *showorpindian_skill;
    DiscardSkill *discard_skill;
    NosYijiViewAsSkill *yiji_skill;
    ChoosePlayerSkill *choose_skill;

    QList<const Player *> selected_targets;

    GuanxingBox *m_guanxingBox;

    ChooseOptionsBox *m_chooseOptionsBox;

    ChooseTriggerOrderBox *m_chooseTriggerOrderBox;

    PlayerCardBox *m_playerCardBox;

    ChooseSuitBox *m_chooseSuitBox;

    QList<CardItem *> gongxin_items;

    ClientLogBox *log_box;
    QTextEdit *chat_box;
    QLineEdit *chat_edit;
    QGraphicsProxyWidget *chat_box_widget;
    QGraphicsProxyWidget *log_box_widget;
    QGraphicsProxyWidget *chat_edit_widget;
    QGraphicsTextItem *prompt_box_widget;
    ChatWidget *chat_widget;
    QPixmap m_rolesBoxBackground;
    QGraphicsPixmapItem *m_rolesBox;
    QGraphicsTextItem *m_pileCardNumInfoTextBox;

    QMenu *m_add_robot_menu;

    QTimer *_m_autoSpeakTimer;
    QMap<QString, BubbleChatBox *> bubbleChatBoxes;

    // for 3v3 & 1v1 mode
    QSanSelectableItem *selector_box;
    QList<CardItem *> general_items, up_generals, down_generals;
    CardItem *to_change;
    QList<QGraphicsRectItem *> arrange_rects;
    QList<CardItem *> arrange_items;
    Button *arrange_button;
    KOFOrderBox *enemy_box, *self_box;
    QPointF m_tableCenterPos;
    ReplayerControlBar *m_replayControl;

#ifdef Q_OS_ANDROID
    QTimer *_m_holdTimer;
    QGraphicsItem *_m_currentHoldItem;
    QPointF _m_lastMousePress;
#endif

    struct _MoveCardsClassifier
    {
        inline _MoveCardsClassifier(const CardsMoveStruct &move)
        {
            m_card_ids = move.card_ids;
        }
        inline bool operator==(const _MoveCardsClassifier &other) const
        {
            return m_card_ids == other.m_card_ids;
        }
        inline bool operator<(const _MoveCardsClassifier &other) const
        {
            return m_card_ids.first() < other.m_card_ids.first();
        }
        QList<int> m_card_ids;
    };

    QMap<_MoveCardsClassifier, CardsMoveStruct> m_move_cache;

    // @todo: this function shouldn't be here. But it's here anyway, before someone find a better
    // home for it.
    QString _translateMovement(const CardsMoveStruct &move);

    void useCard(const Card *card);
    void fillTable(QTableWidget *table, const QList<const ClientPlayer *> &players);

    void selectTarget(int order, bool multiple);
    void selectNextTarget(bool multiple);
    void unselectAllTargets(const QGraphicsItem *except = NULL);
    void updateTargetsEnablity(const Card *card = NULL);

    void callViewAsSkill();
    void cancelViewAsSkill();

    void freeze();
    void addRestartButton(QDialog *dialog);
    QGraphicsItem *createDashboardButtons();
    void createReplayControlBar();

    void fillGenerals1v1(const QStringList &names);
    void fillGenerals3v3(const QStringList &names);

    void showPindianBox(const QString &from_name, int from_id, const QString &to_name, int to_id, const QString &reason);
    void setChatBoxVisible(bool show);
    QRect getBubbleChatBoxShowArea(const QString &who) const;

    // animation related functions
    typedef void (RoomScene::*AnimationFunc)(const QString &, const QStringList &);
    QGraphicsObject *getAnimationObject(const QString &name) const;

    void doMovingAnimation(const QString &name, const QStringList &args);
    void doAppearingAnimation(const QString &name, const QStringList &args);
    void doLightboxAnimation(const QString &name, const QStringList &args);
    void doHuashen(const QString &name, const QStringList &args);
    void doIndicate(const QString &name, const QStringList &args);
    EffectAnimation *animations;
    bool pindian_success;

    // re-layout attempts
    bool game_started;
    void _dispersePhotos(QList<Photo *> &photos, QRectF disperseRegion, Qt::Orientation orientation, Qt::Alignment align);

    void _cancelAllFocus();
    // for miniscenes
    int _m_currentStage;

    QRectF _m_infoPlane;

    bool _m_bgEnabled;
    QString _m_bgMusicPath;

private slots:
    void fillCards(const QList<int> &card_ids, const QList<int> &disabled_ids = QList<int>());
    void updateSkillButtons(bool isPrepare = false);
    void acquireSkill(const ClientPlayer *player, const QString &skill_name);
    void updateSelectedTargets();
    void updateTrustButton();
    void onSkillActivated();
    void onSkillDeactivated();
    void doTimeout();
    void startInXs();
    void hideAvatars();
    void changeHp(const QString &who, int delta, DamageStruct::Nature nature, bool losthp);
    void changeMaxHp(const QString &who, int delta);
    void moveFocus(const QStringList &who, QSanProtocol::Countdown);
    void setEmotion(const QString &who, const QString &emotion);
    void showSkillInvocation(const QString &who, const QString &skill_name);
    void doAnimation(int name, const QStringList &args);
    void showOwnerButtons(bool owner);
    void showPlayerCards();
    void updateRolesBox();
    void updateRoles(const QString &roles);
    void addSkillButton(const Skill *skill);

    void resetPiles();

    void showCard(const QString &player_name, QList<int> card_ids);

    void speak();

    void onGameStart();
    void onGameOver();
    void onStandoff();

    void appendChatEdit(QString txt);
    void showBubbleChatBox(const QString &who, const QString &words);

    //animations
    void onEnabledChange();

    void takeAmazingGrace(ClientPlayer *taker, int card_id, bool move_cards);

    void attachSkill(const QString &skill_name);
    void detachSkill(const QString &skill_name);
    void changeSkillStep(ClientPlayer *who, const QString &skill_name);

    void doGongxin(const QList<int> &card_ids, bool enable_heart, QList<int> enabled_ids);

    void startAssign();

    void doPindianAnimation();

    // 3v3 mode & 1v1 mode
    void fillGenerals(const QStringList &names);
    void takeGeneral(const QString &who, const QString &name, const QString &rule);
    void recoverGeneral(int index, const QString &name);
    void startGeneralSelection();
    void selectGeneral();
    void startArrange(const QString &to_arrange);
    void toggleArrange();
    void finishArrange();
    void changeGeneral(const QString &general);
    void revealGeneral(bool self, const QString &general);

    void skillStateChange(const QString &skill_name);
    void trust();

signals:
    void restart();
    void return_to_start();
    void game_over_dialog_rejected();
};

extern RoomScene *RoomSceneInstance;

#endif
