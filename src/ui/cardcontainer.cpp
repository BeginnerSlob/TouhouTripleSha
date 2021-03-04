#include "cardcontainer.h"

#include "button.h"
#include "carditem.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "graphicsbox.h"
#include "roomscene.h"
#include "timed-progressbar.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

using namespace QSanProtocol;

CardContainer::CardContainer()
    : confirm_button(new Button(tr("confirm"), 0.6))
    , scene_width(0)
    , itemCount(0)
    , progressBar(NULL)
{
    confirm_button->setParentItem(this);
    confirm_button->hide();
    connect(confirm_button, &Button::clicked, this, &CardContainer::clear);

    GraphicsBox::stylize(this);
}

void CardContainer::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    GraphicsBox::paintGraphicsBoxStyle(painter, tr("TouhouTripleSha"), boundingRect());

    const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int width = (card_width + cardInterval) * items.length() - cardInterval + 50;
    if (width * 1.5 > RoomSceneInstance->sceneRect().width()) {
        width = (card_width + cardInterval) * ((items.length() + 1) / 2) - cardInterval + 50;
        one_row = false;
    }

    int first_row = one_row ? items.length() : (items.length() + 1) / 2;

    for (int i = 0; i < items.length(); ++i) {
        int x, y = 0;
        if (i < first_row) {
            x = 25 + (card_width + cardInterval) * i;
            y = 45;
        } else {
            if (items.length() % 2 == 1)
                x = 25 + card_width / 2 + cardInterval / 2 + (card_width + cardInterval) * (i - first_row);
            else
                x = 25 + (card_width + cardInterval) * (i - first_row);
            y = 45 + card_height + cardInterval;
        }
        painter->drawPixmap(x, y, card_width, card_height,
                            G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_CARD_BOX_DEST_PLACE));
    }
}

QRectF CardContainer::boundingRect() const
{
    const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int width = (card_width + cardInterval) * itemCount - cardInterval + 50;
    if (width * 1.5 > (scene_width ? scene_width : 800)) {
        width = (card_width + cardInterval) * ((itemCount + 1) / 2) - cardInterval + 50;
        one_row = false;
    }
    int height = (one_row ? 1 : 2) * card_height + 90 + (one_row ? 0 : cardInterval) + 20;

    return QRectF(0, 0, width, height);
}

void CardContainer::fillCards(const QList<int> &card_ids, const QList<int> &disabled_ids)
{
    if (card_ids == ids)
        return;

    QList<CardItem *> card_items;
    if (card_ids.isEmpty() && items.isEmpty())
        return;
    else if (card_ids.isEmpty() && !items.isEmpty()) {
        card_items = items;
        items.clear();
    } else if (!items.isEmpty()) {
        retained_stack.push(retained());
        items_stack.push(items);
        foreach (CardItem *item, items)
            item->hide();
        items.clear();
    }

    scene_width = RoomSceneInstance->sceneRect().width();

    confirm_button->hide();
    if (card_items.isEmpty())
        card_items = _createCards(card_ids);

    items.append(card_items);
    itemCount = items.length();
    prepareGeometryChange();

    int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int width = (card_width + cardInterval) * itemCount - cardInterval + 50;
    if (width * 1.5 > scene_width) {
        width = (card_width + cardInterval) * ((itemCount + 1) / 2) - cardInterval + 50;
        one_row = false;
    }
    int first_row = one_row ? itemCount : (itemCount + 1) / 2;

    for (int i = 0; i < itemCount; i++) {
        QPointF pos;
        if (i < first_row) {
            pos.setX(25 + (card_width + cardInterval) * i);
            pos.setY(45);
        } else {
            if (itemCount % 2 == 1)
                pos.setX(25 + card_width / 2 + cardInterval / 2 + (card_width + cardInterval) * (i - first_row));
            else
                pos.setX(25 + (card_width + cardInterval) * (i - first_row));
            pos.setY(45 + card_height + cardInterval);
        }
        CardItem *item = items[i];
        item->resetTransform();
        item->setPos(pos);
        item->setHomePos(pos);
        item->setOpacity(1.0);
        item->setHomeOpacity(1.0);
        item->setFlag(QGraphicsItem::ItemIsFocusable);
        if (disabled_ids.contains(item->getCard()->getEffectiveId()))
            item->setEnabled(false);
        item->setOuterGlowEffectEnabled(true);
        item->show();
        ids << item->getId();
    }
    confirm_button->setPos(boundingRect().center().x() - confirm_button->boundingRect().width() / 2,
                           boundingRect().height() - 60);
}

void CardContainer::fillGeneralCards(const QList<CardItem *> &card_item, const QList<CardItem *> &disabled_item)
{
    if (card_item == items)
        return;

    QList<CardItem *> card_items = card_item;
    if (card_items.isEmpty() && items.isEmpty())
        return;
    else if (card_item.isEmpty() && !items.isEmpty()) {
        card_items = items;
        items.clear();
    } else if (!items.isEmpty()) {
        retained_stack.push(retained());
        items_stack.push(items);
        foreach (CardItem *item, items)
            item->hide();
        items.clear();
    }

    scene_width = RoomSceneInstance->sceneRect().width();
    confirm_button->hide();

    items.append(card_items);
    itemCount = items.length();
    prepareGeometryChange();

    int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int width = (card_width + cardInterval) * itemCount - cardInterval + 50;
    if (width * 1.5 > scene_width) {
        width = (card_width + cardInterval) * ((itemCount + 1) / 2) - cardInterval + 50;
        one_row = false;
    }
    int first_row = one_row ? itemCount : (itemCount + 1) / 2;

    for (int i = 0; i < itemCount; i++) {
        QPointF pos;
        if (i < first_row) {
            pos.setX(25 + (card_width + cardInterval) * i);
            pos.setY(45);
        } else {
            if (itemCount % 2 == 1)
                pos.setX(25 + card_width / 2 + cardInterval / 2 + (card_width + cardInterval) * (i - first_row));
            else
                pos.setX(25 + (card_width + cardInterval) * (i - first_row));
            pos.setY(45 + card_height + cardInterval);
        }
        CardItem *item = items[i];
        item->resetTransform();
        item->setPos(pos);
        item->setHomePos(pos);
        item->setOpacity(1.0);
        item->setHomeOpacity(1.0);
        item->setFlag(QGraphicsItem::ItemIsFocusable);
        if (disabled_item.contains(item))
            item->setEnabled(false);
        item->setOuterGlowEffectEnabled(true);
        item->show();
    }
    confirm_button->setPos(boundingRect().center().x() - confirm_button->boundingRect().width() / 2,
                           boundingRect().height() - 40);
}

bool CardContainer::_addCardItems(QList<CardItem *> &, const CardsMoveStruct &)
{
    return true;
}

bool CardContainer::retained()
{
    return confirm_button != NULL && confirm_button->isVisible();
}

void CardContainer::clear()
{
    if (progressBar != NULL) {
        progressBar->hide();
        progressBar->deleteLater();
        progressBar = NULL;
    }

    foreach (CardItem *item, items) {
        item->hide();
        item->deleteLater();
        item = NULL;
    }

    items.clear();
    if (!items_stack.isEmpty() && Sanguosha->currentRoomObject() != NULL) {
        items = items_stack.pop();
        bool retained = retained_stack.pop();
        fillCards();
        if (retained && confirm_button)
            addConfirmButton();
    } else {
        ids.clear();
        confirm_button->hide();
        prepareGeometryChange();
        hide();
    }
}

void CardContainer::freezeCards(bool is_frozen)
{
    foreach (CardItem *item, items)
        item->setFrozen(is_frozen);
}

QList<CardItem *> CardContainer::removeCardItems(const QList<int> &card_ids, Player::Place)
{
    QList<CardItem *> result;
    foreach (int card_id, card_ids) {
        CardItem *to_take = NULL;
        foreach (CardItem *item, items) {
            if (item->getCard()->getId() == card_id) {
                to_take = item;
                break;
            }
        }
        if (to_take == NULL)
            continue;

        to_take->setEnabled(false);

        CardItem *copy = new CardItem(to_take->getCard());
        copy->setPos(mapToScene(to_take->pos()));
        copy->setEnabled(false);
        result.append(copy);

        if (m_currentPlayer)
            to_take->showAvatar(m_currentPlayer->getGeneral());
    }
    return result;
}

int CardContainer::getFirstEnabled() const
{
    foreach (CardItem *card, items) {
        if (card->isEnabled())
            return card->getCard()->getId();
    }
    return -1;
}

void CardContainer::startChoose()
{
    confirm_button->hide();
    foreach (CardItem *item, items) {
        connect(item, &CardItem::leave_hover, this, &CardContainer::grabItem);
        connect(item, &CardItem::clicked, this, &CardContainer::chooseItem);
    }
}

void CardContainer::startGongxin(const QList<int> &enabled_ids)
{
    if (enabled_ids.isEmpty())
        return;
    foreach (CardItem *item, items) {
        const Card *card = item->getCard();
        if (card && enabled_ids.contains(card->getEffectiveId()))
            connect(item, &CardItem::clicked, this, &CardContainer::gongxinItem);
        else
            item->setEnabled(false);
    }
}

void CardContainer::addConfirmButton()
{
    foreach (CardItem *card, items)
        card->setFlag(ItemIsMovable, false);

    confirm_button->show();
    if (!progressBar) {
        progressBar = new QSanCommandProgressBar();
        progressBarItem = new QGraphicsProxyWidget(this);
    }
    progressBar->setMaximumWidth(boundingRect().width() - 10);
    progressBar->setMaximumHeight(10);
    progressBar->setTimerEnabled(true);
    progressBarItem->setWidget(progressBar);
    progressBarItem->setPos(boundingRect().center().x() - progressBarItem->boundingRect().width() / 2,
                            boundingRect().height() - 20);
    connect(progressBar, &QSanCommandProgressBar::timedOut, this, &CardContainer::clear);

    Countdown countdown;
    countdown.max = 10000;
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    progressBar->setCountdown(countdown);
    progressBar->show();
}

void CardContainer::grabItem()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item && !collidesWithItem(card_item)) {
        card_item->disconnect(this);
        emit item_chosen(card_item->getCard()->getId());
    }
}

void CardContainer::chooseItem()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item) {
        card_item->disconnect(this);
        emit item_chosen(card_item->getCard()->getId());
    }
}

void CardContainer::gongxinItem()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item) {
        emit item_gongxined(card_item->getCard()->getId());
        clear();
    }
}

void CardContainer::view(const ClientPlayer *player)
{
    QList<int> card_ids;
    QList<const Card *> cards = player->getHandcards();
    foreach (const Card *card, cards)
        card_ids << card->getEffectiveId();

    fillCards(card_ids);
}
