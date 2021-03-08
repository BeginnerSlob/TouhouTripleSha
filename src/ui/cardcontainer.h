#ifndef _CARD_CONTAINER_H
#define _CARD_CONTAINER_H

class Button;

#include "carditem.h"
#include "generic-cardcontainer-ui.h"

#include <QStack>

class QSanCommandProgressBar;

class CardContainer : public GenericCardContainer
{
    Q_OBJECT

public:
    explicit CardContainer();
    virtual QList<CardItem *> removeCardItems(const QList<int> &card_ids, Player::Place place);
    int getFirstEnabled() const;
    void startChoose();
    void startGongxin(const QList<int> &enabled_ids);
    void addConfirmButton();
    void view(const ClientPlayer *player);
    virtual QRectF boundingRect() const;
    ClientPlayer *m_currentPlayer;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    bool retained();

public slots:
    void fillCards(const QList<int> &card_ids = QList<int>(), const QList<int> &disabled_ids = QList<int>());
    void fillGeneralCards(const QList<CardItem *> &card_items = QList<CardItem *>(), const QList<CardItem *> &disabled_item = QList<CardItem *>());
    void clear();
    void freezeCards(bool is_disable);

protected:
    virtual bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo);
    Button *confirm_button;
    int scene_width;
    int itemCount;

    static const int cardInterval = 3;
    QGraphicsProxyWidget *progressBarItem;
    QSanCommandProgressBar *progressBar;

private:
    QList<CardItem *> items;
    QStack<QList<CardItem *> > items_stack;
    QStack<bool> retained_stack;
    QList<int> ids;

    void _addCardItem(int card_id, const QPointF &pos);

private slots:
    void grabItem();
    void chooseItem();
    void gongxinItem();

signals:
    void item_chosen(int card_id);
    void item_gongxined(int card_id);
};

class CardItem;
class ClientPlayer;

#endif
