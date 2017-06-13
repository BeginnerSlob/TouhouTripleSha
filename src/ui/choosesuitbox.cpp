/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#include "choosesuitbox.h"
#include "client.h"
#include "clientstruct.h"
#include "engine.h"
#include "skin-bank.h"
#include "timed-progressbar.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QPropertyAnimation>

static qreal initialOpacity = 0.8;
static int optionButtonHeight = 30;

const int ChooseSuitBox::outerBlankWidth = 37;
const int ChooseSuitBox::buttonWidth = 40;
const int ChooseSuitBox::buttonHeight = 30;
const int ChooseSuitBox::interval = 15;
const int ChooseSuitBox::topBlankWidth = 42;
const int ChooseSuitBox::bottomBlankWidth = 25;

SuitOptionButton::SuitOptionButton(QGraphicsObject *parent, const QString &suit, const int width)
    : QGraphicsObject(parent)
    , m_suit(suit)
    , width(width)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setOpacity(initialOpacity);
}

void SuitOptionButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::HighQualityAntialiasing);
    painter->save();
    painter->setBrush(QBrush(QColor(0, 0, 0, 220)));
    painter->setPen(Sanguosha->getKingdomColor(Self->getGeneral()->getKingdom()));
    QRectF rect = boundingRect();
    painter->drawRoundedRect(QRect(rect.left() - 5, rect.top(), rect.width() + 10, rect.height()), 5, 5);
    painter->restore();

    QPixmap pixmap = G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_HAND_CARD_SUIT, m_suit);
    pixmap = pixmap.scaledToHeight(optionButtonHeight, Qt::SmoothTransformation);
    QRect pixmapRect(QPoint(((rect.width() - pixmap.width()) / 2), (rect.height() - pixmap.height()) / 2), pixmap.size());
    painter->setBrush(pixmap);
    painter->drawRect(pixmapRect);
}

QRectF SuitOptionButton::boundingRect() const
{
    return QRectF(0, 0, width, optionButtonHeight);
}

void SuitOptionButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void SuitOptionButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    emit clicked();
}

void SuitOptionButton::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(1.0);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    emit hovered(true);
}

void SuitOptionButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(initialOpacity);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    emit hovered(false);
}

ChooseSuitBox::ChooseSuitBox()
    : progressBar(NULL)
{
    title = tr("Please choose a suit");
}

QRectF ChooseSuitBox::boundingRect() const
{
    const int width = buttonWidth * suitNumber + outerBlankWidth * 2 + interval * (suitNumber - 1);

    int height = topBlankWidth + buttonHeight + bottomBlankWidth;

    if (ServerInfo.OperationTimeout != 0)
        height += 12;

    return QRectF(0, 0, width, height);
}

void ChooseSuitBox::chooseSuit(const QStringList &suits)
{
    suitNumber = suits.size();
    m_suits = suits;
    prepareGeometryChange();

    foreach (const QString &suit, suits) {
        SuitOptionButton *button = new SuitOptionButton(this, suit, buttonWidth);
        button->setObjectName(suit);
        buttons << button;
        button->setParentItem(this);

        connect(button, &SuitOptionButton::clicked, this, &ChooseSuitBox::reply);
    }

    moveToCenter();
    show();

    for (int i = 0; i < buttons.length(); ++i) {
        SuitOptionButton *button = buttons.at(i);

        QPointF pos;
        pos.setX(outerBlankWidth + (buttonWidth + interval) * i);
        pos.setY(topBlankWidth);

        button->setPos(pos);
    }

    if (ServerInfo.OperationTimeout != 0) {
        if (!progressBar) {
            progressBar = new QSanCommandProgressBar();
            progressBar->setMaximumWidth(boundingRect().width() - 16);
            progressBar->setMaximumHeight(12);
            progressBar->setTimerEnabled(true);
            progressBarItem = new QGraphicsProxyWidget(this);
            progressBarItem->setWidget(progressBar);
            progressBarItem->setPos(boundingRect().center().x() - progressBarItem->boundingRect().width() / 2,
                                    boundingRect().height() - 20);
            connect(progressBar, &QSanCommandProgressBar::timedOut, this, &ChooseSuitBox::reply);
        }
        progressBar->setCountdown(QSanProtocol::S_COMMAND_MULTIPLE_CHOICE);
        progressBar->show();
    }
}

void ChooseSuitBox::reply()
{
    QString suit = sender()->objectName();
    if (suit.isEmpty())
        suit = m_suits.at(qrand() % suitNumber);
    ClientInstance->onPlayerChooseSuit(suit);
}

void ChooseSuitBox::clear()
{
    if (progressBar != NULL) {
        progressBar->hide();
        progressBar->deleteLater();
        progressBar = NULL;
    }

    foreach (SuitOptionButton *button, buttons)
        button->deleteLater();

    buttons.clear();

    disappear();
}
