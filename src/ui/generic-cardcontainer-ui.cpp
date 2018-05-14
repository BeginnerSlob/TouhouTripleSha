#include "generic-cardcontainer-ui.h"
#include "clientplayer.h"
#include "engine.h"
#include "roomscene.h"
#include "standard.h"
#include <QGraphicsColorizeEffect>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QParallelAnimationGroup>
#include <qlabel.h>
#include <qmenu.h>
#include <qpropertyanimation.h>
#include <qpushbutton.h>
#include <qtextdocument.h>

using namespace QSanProtocol;

QList<CardItem *> GenericCardContainer::cloneCardItems(QList<int> card_ids)
{
    return _createCards(card_ids);
}

QList<CardItem *> GenericCardContainer::_createCards(QList<int> card_ids)
{
    QList<CardItem *> result;
    foreach (int card_id, card_ids) {
        CardItem *item = _createCard(card_id);
        result.append(item);
    }
    return result;
}

CardItem *GenericCardContainer::_createCard(int card_id)
{
    const Card *card = Sanguosha->getCard(card_id);
    CardItem *item = new CardItem(card);
    item->setOpacity(0.0);
    item->setParentItem(this);
    return item;
}

void GenericCardContainer::_destroyCard()
{
    CardItem *card = (CardItem *)sender();
    card->setVisible(false);
    card->deleteLater();
}

bool GenericCardContainer::_horizontalPosLessThan(const CardItem *card1, const CardItem *card2)
{
    return (card1->x() < card2->x());
}

void GenericCardContainer::_disperseCards(QList<CardItem *> &cards, QRectF fillRegion, Qt::Alignment align, bool useHomePos,
                                          bool keepOrder)
{
    int numCards = cards.size();
    if (numCards == 0)
        return;
    if (!keepOrder)
        qSort(cards.begin(), cards.end(), GenericCardContainer::_horizontalPosLessThan);
    double maxWidth = fillRegion.width();
    int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    double step = qMin((double)cardWidth, (maxWidth - cardWidth) / (numCards - 1));
    align &= Qt::AlignHorizontal_Mask;
    for (int i = 0; i < numCards; i++) {
        CardItem *card = cards[i];
        double newX = 0;
        if (align == Qt::AlignHCenter)
            newX = fillRegion.center().x() + (step + 6) * (i - (numCards - 1) / 2.0);
        else if (align == Qt::AlignLeft)
            newX = fillRegion.left() + step * i + card->boundingRect().width() / 2.0;
        else if (align == Qt::AlignRight)
            newX = fillRegion.right() + step * (i - numCards) + card->boundingRect().width() / 2.0;
        else
            continue;
        QPointF newPos = QPointF(newX, fillRegion.center().y());
        if (useHomePos)
            card->setHomePos(newPos);
        else
            card->setPos(newPos);
        card->setZValue(_m_highestZ++);
    }
}

void GenericCardContainer::onAnimationFinished()
{
    QParallelAnimationGroup *animation = qobject_cast<QParallelAnimationGroup *>(sender());
    if (animation) {
        while (animation->animationCount() > 0)
            animation->takeAnimation(0);
        animation->deleteLater();
    }
}

void GenericCardContainer::_doUpdate()
{
    update();
}

void GenericCardContainer::_playMoveCardsAnimation(QList<CardItem *> &cards, bool destroyCards)
{
    QParallelAnimationGroup *animation = new QParallelAnimationGroup;
    foreach (CardItem *card_item, cards) {
        if (destroyCards)
            connect(card_item, &CardItem::movement_animation_finished, this, &GenericCardContainer::_destroyCard);
        animation->addAnimation(card_item->getGoBackAnimation(true));
    }

    connect(animation, &QParallelAnimationGroup::finished, this, &GenericCardContainer::_doUpdate);
    connect(animation, &QParallelAnimationGroup::finished, this, &GenericCardContainer::onAnimationFinished);
    animation->start();
}

void GenericCardContainer::addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo)
{
    foreach (CardItem *card_item, card_items) {
        card_item->setPos(mapFromScene(card_item->scenePos()));
        card_item->setParentItem(this);
    }
    bool destroy = _addCardItems(card_items, moveInfo);
    _playMoveCardsAnimation(card_items, destroy);
}

void PlayerCardContainer::_paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QString &key)
{
    _paintPixmap(item, rect, _getPixmap(key));
}

void PlayerCardContainer::_paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QString &key,
                                       QGraphicsItem *parent)
{
    _paintPixmap(item, rect, _getPixmap(key), parent);
}

void PlayerCardContainer::_paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QPixmap &pixmap)
{
    _paintPixmap(item, rect, pixmap, _m_groupMain);
}

QPixmap PlayerCardContainer::_getPixmap(const QString &key, const QString &sArg, bool cache)
{
    Q_ASSERT(key.contains("%1"));
    if (key.contains("%2")) {
        QString rKey = key.arg(getResourceKeyName()).arg(sArg);

        if (G_ROOM_SKIN.isImageKeyDefined(rKey))
            return G_ROOM_SKIN.getPixmap(rKey, QString(), cache); // first try "%1key%2 = ...", %1 = "photo", %2 = sArg

        rKey = key.arg(getResourceKeyName());
        return G_ROOM_SKIN.getPixmap(rKey, sArg, cache); // then try "%1key = ..."
    } else {
        return G_ROOM_SKIN.getPixmap(key, sArg, cache); // finally, try "key = ..."
    }
}

QPixmap PlayerCardContainer::_getPixmap(const QString &key, bool cache)
{
    if (key.contains("%1") && G_ROOM_SKIN.isImageKeyDefined(key.arg(getResourceKeyName())))
        return G_ROOM_SKIN.getPixmap(key.arg(getResourceKeyName()), QString(), cache);
    else
        return G_ROOM_SKIN.getPixmap(key, QString(), cache);
}

void PlayerCardContainer::_paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QPixmap &pixmap,
                                       QGraphicsItem *parent)
{
    if (item == NULL) {
        item = new QGraphicsPixmapItem(parent);
        item->setTransformationMode(Qt::SmoothTransformation);
    }
    item->setPos(rect.x(), rect.y());
    if (pixmap.size() == rect.size())
        item->setPixmap(pixmap);
    else
        item->setPixmap(pixmap.scaled(rect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    item->setParentItem(parent);
}

void PlayerCardContainer::_clearPixmap(QGraphicsPixmapItem *pixmap)
{
    QPixmap dummy;
    if (pixmap == NULL)
        return;
    pixmap->setPixmap(dummy);
    pixmap->hide();
}

void PlayerCardContainer::hideProgressBar()
{
    _m_progressBar->hide();
}

void PlayerCardContainer::showProgressBar(Countdown countdown)
{
    _m_progressBar->setCountdown(countdown);
    _m_progressBar->show();
}

QPixmap PlayerCardContainer::_getAvatarIcon(QString heroName)
{
    int avatarSize = m_player->getGeneral2() ? _m_layout->m_primaryAvatarSize : _m_layout->m_avatarSize;
    return G_ROOM_SKIN.getGeneralPixmap(heroName, (QSanRoomSkin::GeneralIconSize)avatarSize);
}

void PlayerCardContainer::updateAvatar()
{
    const General *general = NULL;
    if (m_player) {
        general = m_player->getAvatarGeneral();
        QString show_string = m_player->screenName();
        if (m_player->userId() != -1)
            show_string = QString("Lv.%1 %2").arg(m_player->getLevel()).arg(m_player->screenName());
        _m_layout->m_screenNameFont.paintText(_m_screenNameItem, _m_layout->m_screenNameArea, Qt::AlignCenter, show_string);
    } else {
        _m_layout->m_screenNameFont.paintText(_m_screenNameItem, _m_layout->m_screenNameArea, Qt::AlignCenter, QString());
    }
    if (general != NULL) {
        _m_avatarArea->setToolTip(m_player->getSkillDescription());
        QPixmap avatarIcon = _getAvatarIcon(general->objectName());
        _paintPixmap(_m_avatarIcon, _m_layout->m_avatarArea, avatarIcon, _getAvatarParent());
        // this is just avatar general, perhaps game has not started yet.
        const General *general = m_player->getGeneral();
        if (general != NULL) {
            QString kingdom = m_player->getKingdom();
            QString gender = general->isMale() ? "male" : "female";
            _paintPixmap(_m_kingdomIcon, _m_layout->m_kingdomIconArea,
                         G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_KINGDOM_ICON, kingdom), _getAvatarParent());
            _paintPixmap(_m_kingdomColorMaskIcon, _m_layout->m_kingdomMaskArea,
                         G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_KINGDOM_COLOR_MASK, kingdom), _getAvatarParent());
            _paintPixmap(_m_handCardBg, _m_layout->m_handCardArea, _getPixmap(QSanRoomSkin::S_SKIN_KEY_HANDCARDNUM, kingdom),
                         _getAvatarParent());
            _paintPixmap(_m_genderBg, _m_layout->m_genderArea, _getPixmap(QSanRoomSkin::S_SKIN_KEY_GENDER, gender),
                         _getAvatarParent());
            _paintPixmap(_m_avatarNameItem, _m_layout->m_avatarNameArea,
                         _getPixmap(QSanRoomSkin::S_SKIN_KEY_AVATARNAME, general->objectName()), _getAvatarParent());
        } else {
            _paintPixmap(_m_handCardBg, _m_layout->m_handCardArea,
                         _getPixmap(QSanRoomSkin::S_SKIN_KEY_HANDCARDNUM, QString(QSanRoomSkin::S_SKIN_KEY_DEFAULT_SECOND)),
                         _getAvatarParent());
            _paintPixmap(_m_genderBg, _m_layout->m_genderArea,
                         _getPixmap(QSanRoomSkin::S_SKIN_KEY_GENDER, QString(QSanRoomSkin::S_SKIN_KEY_DEFAULT_SECOND)),
                         _getAvatarParent());
        }
    } else if (m_player && m_player->getState() == "robot") {
        _m_avatarArea->setToolTip(m_player->getSkillDescription());
        QPixmap avatarIcon = _getAvatarIcon("robot");
        _paintPixmap(_m_avatarIcon, _m_layout->m_avatarArea, avatarIcon, _getAvatarParent());
        _paintPixmap(_m_handCardBg, _m_layout->m_handCardArea,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_HANDCARDNUM, QString(QSanRoomSkin::S_SKIN_KEY_DEFAULT_SECOND)),
                     _getAvatarParent());
        _paintPixmap(_m_genderBg, _m_layout->m_genderArea,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_GENDER, QString(QSanRoomSkin::S_SKIN_KEY_DEFAULT_SECOND)),
                     _getAvatarParent());
    } else if (m_player) {
        _m_avatarArea->setToolTip(m_player->getSkillDescription());
        QPixmap avatarIcon = _getAvatarIcon(Sanguosha->getRandomGeneralName());
        _paintPixmap(_m_avatarIcon, _m_layout->m_avatarArea, avatarIcon, _getAvatarParent());
        _paintPixmap(_m_handCardBg, _m_layout->m_handCardArea,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_HANDCARDNUM, QString(QSanRoomSkin::S_SKIN_KEY_DEFAULT_SECOND)),
                     _getAvatarParent());
        _paintPixmap(_m_genderBg, _m_layout->m_genderArea,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_GENDER, QString(QSanRoomSkin::S_SKIN_KEY_DEFAULT_SECOND)),
                     _getAvatarParent());
    } else {
        _paintPixmap(_m_avatarIcon, _m_layout->m_avatarArea, QSanRoomSkin::S_SKIN_KEY_BLANK_GENERAL, _getAvatarParent());
        _clearPixmap(_m_kingdomColorMaskIcon);
        _clearPixmap(_m_kingdomIcon);
        _paintPixmap(_m_handCardBg, _m_layout->m_handCardArea,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_HANDCARDNUM, QString(QSanRoomSkin::S_SKIN_KEY_DEFAULT_SECOND)),
                     _getAvatarParent());
        _paintPixmap(_m_genderBg, _m_layout->m_genderArea,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_GENDER, QString(QSanRoomSkin::S_SKIN_KEY_DEFAULT_SECOND)),
                     _getAvatarParent());
        _m_avatarArea->setToolTip(QString());
    }
    _m_avatarIcon->show();
    _adjustComponentZValues();
    _initializeRemovedEffect();
    refresh();
}

QPixmap PlayerCardContainer::paintByMask(QPixmap &source)
{
    QPixmap tmp = G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_GENERAL_CIRCLE_MASK,
                                        QString::number(_m_layout->m_circleImageSize), true);
    if (tmp.height() <= 1 && tmp.width() <= 1)
        return source;
    QPainter p(&tmp);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.drawPixmap(0, 0, _m_layout->m_smallAvatarArea.width(), _m_layout->m_smallAvatarArea.height(), source);
    return tmp;
}

void PlayerCardContainer::updateSmallAvatar()
{
    updateAvatar();
    const General *general = NULL;
    if (m_player)
        general = m_player->getGeneral2();
    if (general != NULL) {
        QPixmap smallAvatarIcon
            = G_ROOM_SKIN.getGeneralPixmap(general->objectName(), QSanRoomSkin::GeneralIconSize(_m_layout->m_smallAvatarSize));
        smallAvatarIcon = paintByMask(smallAvatarIcon);
        _paintPixmap(_m_smallAvatarIcon, _m_layout->m_smallAvatarArea, smallAvatarIcon, _getAvatarParent());
        _paintPixmap(_m_circleItem, _m_layout->m_circleArea,
                     QString(QSanRoomSkin::S_SKIN_KEY_GENERAL_CIRCLE_IMAGE).arg(_m_layout->m_circleImageSize),
                     _getAvatarParent());
        _m_smallAvatarArea->setToolTip(m_player->getSkillDescription());
        QString name = general->getTranslatedName();
        _m_layout->m_smallAvatarNameFont.paintText(_m_smallAvatarNameItem, _m_layout->m_smallAvatarNameArea,
                                                   Qt::AlignLeft | Qt::AlignJustify, name);
        _m_smallAvatarIcon->show();
    } else {
        _clearPixmap(_m_smallAvatarIcon);
        _clearPixmap(_m_circleItem);
        _m_layout->m_smallAvatarNameFont.paintText(_m_smallAvatarNameItem, _m_layout->m_smallAvatarNameArea,
                                                   Qt::AlignLeft | Qt::AlignJustify, QString());
        _m_smallAvatarArea->setToolTip(QString());
    }
    _adjustComponentZValues();
}

void PlayerCardContainer::updatePhase()
{
    if (!m_player || !m_player->isAlive())
        _clearPixmap(_m_phaseIcon);
    else if (m_player->getPhase() != Player::NotActive) {
        if (m_player->getPhase() == Player::PhaseNone)
            return;
        int index = static_cast<int>(m_player->getPhase());
        QRect phaseArea = _m_layout->m_phaseArea.getTranslatedRect(_getPhaseParent()->boundingRect().toRect());
        _paintPixmap(_m_phaseIcon, phaseArea, _getPixmap(QSanRoomSkin::S_SKIN_KEY_PHASE, QString::number(index), true),
                     _getPhaseParent());
        _m_phaseIcon->show();
    } else {
        if (_m_progressBar)
            _m_progressBar->hide();
        if (_m_phaseIcon)
            _m_phaseIcon->hide();
    }
}

void PlayerCardContainer::updateHp()
{
    Q_ASSERT(_m_hpBox && _m_saveMeIcon && m_player);
    _m_hpBox->setHp(m_player->getHp());
    _m_hpBox->setMaxHp(m_player->getMaxHp());
    _m_hpBox->update();
    if (m_player->getHp() > 0 || m_player->getMaxHp() == 0)
        _m_saveMeIcon->setVisible(false);
}

static bool CompareByNumber(const Card *card1, const Card *card2)
{
    return card1->getNumber() < card2->getNumber();
}

void PlayerCardContainer::updatePile(const QString &pile_name)
{
    ClientPlayer *player = (ClientPlayer *)sender();
    if (!player)
        player = m_player;
    if (!player)
        return;

    QString treasure_name;
    if (player->getTreasure())
        treasure_name = player->getTreasure()->objectName();

    const QList<int> &pile = player->getPile(pile_name);
    if (pile.size() == 0) {
        if (_m_privatePiles.contains(pile_name)) {
            delete _m_privatePiles[pile_name];
            _m_privatePiles[pile_name] = NULL;
            _m_privatePiles.remove(pile_name);
        }
    } else {
        // retrieve menu and create a new pile if necessary
        QMenu *menu;
        QPushButton *button;
        if (!_m_privatePiles.contains(pile_name)) {
            button = new QPushButton;
            button->setObjectName(pile_name);
            if (treasure_name == pile_name)
                button->setProperty("treasure", "true");
            else
                button->setProperty("private_pile", "true");
            QGraphicsProxyWidget *button_widget = new QGraphicsProxyWidget(_getPileParent());
            button_widget->setObjectName(pile_name);
            button_widget->setWidget(button);
            _m_privatePiles[pile_name] = button_widget;
        } else {
            button = (QPushButton *)(_m_privatePiles[pile_name]->widget());
            menu = button->menu();
        }

        QString text = Sanguosha->translate(pile_name);
        if (pile.length() > 0)
            text.append(QString("(%1)").arg(pile.length()));
        button->setText(text);
        menu = new QMenu(button);
        if (treasure_name == pile_name)
            menu->setProperty("treasure", "true");
        else
            menu->setProperty("private_pile", "true");

        //Sort the cards in pile by number can let players know what is in this pile more clear.
        //If someone has "buqu", we can got which card he need or which he hate easier.
        QList<const Card *> cards;
        foreach (int card_id, pile) {
            const Card *card = Sanguosha->getCard(card_id);
            if (card != NULL)
                cards << card;
        }
        qSort(cards.begin(), cards.end(), CompareByNumber);

        foreach (const Card *card, cards)
            menu->addAction(G_ROOM_SKIN.getCardSuitPixmap(card->getSuit()), card->getFullName());

        int length = cards.count();
        if (length > 0)
            button->setMenu(menu);
        else {
            delete menu;
            button->setMenu(NULL);
        }
    }

    QPoint start = _m_layout->m_privatePileStartPos;
    QPoint step = _m_layout->m_privatePileStep;
    QSize size = _m_layout->m_privatePileButtonSize;
    QList<QGraphicsProxyWidget *> widgets_t, widgets_p, widgets = _m_privatePiles.values();
    foreach (QGraphicsProxyWidget *widget, widgets) {
        if (widget->objectName() == treasure_name)
            widgets_t << widget;
        else
            widgets_p << widget;
    }
    widgets = widgets_t + widgets_p;
    for (int i = 0; i < widgets.length(); i++) {
        QGraphicsProxyWidget *widget = widgets[i];
        widget->setPos(start + i * step);
        widget->resize(size);
    }
}

void PlayerCardContainer::updateDrankState()
{
    if (m_player->getMark("drank") > 0)
        _m_avatarArea->setBrush(G_PHOTO_LAYOUT.m_drankMaskColor);
    else
        _m_avatarArea->setBrush(Qt::NoBrush);
}

void PlayerCardContainer::updateIkQihuang()
{
    return;
}

void PlayerCardContainer::updateHandcardNum()
{
    int num = 0;
    if (m_player && m_player->getGeneral())
        num = m_player->getHandcardNum();
    Q_ASSERT(num >= 0);
    _m_layout->m_handCardFont.paintText(_m_handCardNumText, _m_layout->m_handCardArea, Qt::AlignCenter, QString::number(num));
    _m_handCardNumText->setVisible(true);
}

void PlayerCardContainer::updateMarks()
{
    if (!_m_markItem)
        return;
    QRect parentRect = _getMarkParent()->boundingRect().toRect();
    QSize markSize = _m_markItem->boundingRect().size().toSize();
    QRect newRect = _m_layout->m_markTextArea.getTranslatedRect(parentRect, markSize);
    if (_m_layout == &G_PHOTO_LAYOUT)
        _m_markItem->setPos(newRect.topLeft());
    else
        _m_markItem->setPos(newRect.left(), newRect.top() + newRect.height() / 2);
}

void PlayerCardContainer::_updateEquips()
{
    for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
        CardItem *equip = _m_equipCards[i];
        if (equip == NULL)
            continue;
        const EquipCard *equip_card = qobject_cast<const EquipCard *>(equip->getCard()->getRealCard());
        QPixmap pixmap = _getEquipPixmap(equip_card);
        _m_equipLabel[i]->setPixmap(pixmap);
        _m_equipRegions[i]->setPos(_m_layout->m_equipAreas[i].topLeft());
    }
}

void PlayerCardContainer::refresh(bool)
{
    if (!m_player || !m_player->getGeneral() || !m_player->isAlive()) {
        if (_m_faceTurnedIcon)
            _m_faceTurnedIcon->setVisible(false);
        if (_m_chainIcon)
            _m_chainIcon->setVisible(false);
        if (_m_actionIcon)
            _m_actionIcon->setVisible(false);
        if (_m_saveMeIcon)
            _m_saveMeIcon->setVisible(false);
    } else if (m_player) {
        if (_m_faceTurnedIcon)
            _m_faceTurnedIcon->setVisible(!m_player->faceUp());
        if (_m_chainIcon)
            _m_chainIcon->setVisible(m_player->isChained());
        if (_m_actionIcon)
            _m_actionIcon->setVisible(m_player->hasFlag("actioned"));
        if (_m_deathIcon && !(ServerInfo.GameMode == "04_1v3" && m_player->getGeneralName() != "shenlvbu2"))
            _m_deathIcon->setVisible(m_player->isDead());
    }
    updateHandcardNum();
    _adjustComponentZValues();
}

void PlayerCardContainer::repaintAll()
{
    if (!_m_avatarIcon) {
        _m_avatarIcon = new QGraphicsPixmapItem(_getAvatarParent());
        _m_avatarIcon->setTransformationMode(Qt::SmoothTransformation);
    }
    if (!_m_smallAvatarIcon) {
        _m_smallAvatarIcon = new QGraphicsPixmapItem(_getAvatarParent());
        _m_smallAvatarIcon->setTransformationMode(Qt::SmoothTransformation);
    }

    _m_avatarArea->setRect(_m_layout->m_avatarArea);
    _m_smallAvatarArea->setRect(_m_layout->m_smallAvatarArea);

    updateAvatar();
    updateSmallAvatar();
    updatePhase();
    updateMarks();
    _updateProgressBar();
    _updateDeathIcon();
    _updateEquips();
    updateDelayedTricks();

    if (_m_huashenAnimation != NULL)
        startHuaShen(_m_huashenGeneralName, _m_huashenSkillName, _m_huashenChangeName);

    _paintPixmap(_m_faceTurnedIcon, _m_layout->m_avatarArea, QSanRoomSkin::S_SKIN_KEY_FACETURNEDMASK, _getAvatarParent());
    _paintPixmap(_m_chainIcon, _m_layout->m_chainedIconRegion, QSanRoomSkin::S_SKIN_KEY_CHAIN, _getAvatarParent());
    _paintPixmap(_m_saveMeIcon, _m_layout->m_saveMeIconRegion, QSanRoomSkin::S_SKIN_KEY_SAVE_ME_ICON, _getAvatarParent());
    _paintPixmap(_m_actionIcon, _m_layout->m_actionedIconRegion, QSanRoomSkin::S_SKIN_KEY_ACTIONED_ICON, _getAvatarParent());
    if (_m_roleComboBox != NULL)
        _m_roleComboBox->setPos(_m_layout->m_roleComboBoxPos);

    _m_hpBox->setIconSize(_m_layout->m_magatamaSize);
    _m_hpBox->setOrientation(_m_layout->m_magatamasHorizontal ? Qt::Horizontal : Qt::Vertical);
    _m_hpBox->setBackgroundVisible(_m_layout->m_magatamasBgVisible);
    _m_hpBox->setAnchorEnable(true);
    _m_hpBox->setAnchor(_m_layout->m_magatamasAnchor, _m_layout->m_magatamasAlign);
    _m_hpBox->setImageArea(_m_layout->m_magatamaImageArea);
    _m_hpBox->update();

    _adjustComponentZValues();
    refresh();
}

QPropertyAnimation *PlayerCardContainer::initializeBlurEffect(QGraphicsPixmapItem *icon)
{
    QGraphicsBlurEffect *effect = new QGraphicsBlurEffect(this);
    effect->setBlurHints(QGraphicsBlurEffect::AnimationHint);
    effect->setBlurRadius(0);
    icon->setGraphicsEffect(effect);

    QPropertyAnimation *animation = new QPropertyAnimation(effect, "blurRadius", this);
    animation->setEasingCurve(QEasingCurve::OutInBounce);
    animation->setDuration(2000);
    animation->setStartValue(0);
    animation->setEndValue(5);
    return animation;
}

void PlayerCardContainer::_initializeRemovedEffect()
{
    _blurEffect = new QParallelAnimationGroup(this);
    _blurEffect->addAnimation(initializeBlurEffect(_m_avatarIcon));
    _blurEffect->addAnimation(initializeBlurEffect(_m_smallAvatarIcon));
}

void PlayerCardContainer::_createRoleComboBox()
{
    _m_roleComboBox = new RoleComboBox(_getRoleComboBoxParent());
}

void PlayerCardContainer::setPlayer(ClientPlayer *player)
{
    this->m_player = player;
    if (player) {
        connect(player, &ClientPlayer::general_changed, this, &PlayerCardContainer::updateAvatar);
        connect(player, &ClientPlayer::general2_changed, this, &PlayerCardContainer::updateSmallAvatar);
        connect(player, &ClientPlayer::level_changed, this, &PlayerCardContainer::updateAvatar);
        connect(player, &ClientPlayer::kingdom_changed, this, &PlayerCardContainer::updateAvatar);
        connect(player, &ClientPlayer::state_changed, this, (void (PlayerCardContainer::*)())(&PlayerCardContainer::refresh));
        connect(player, &ClientPlayer::phase_changed, this, &PlayerCardContainer::updatePhase);
        connect(player, &ClientPlayer::drank_changed, this, &PlayerCardContainer::updateDrankState);
        connect(player, &ClientPlayer::action_taken, this, (void (PlayerCardContainer::*)())(&PlayerCardContainer::refresh));
        connect(player, &ClientPlayer::ikqihuang_invoked, this, &PlayerCardContainer::updateIkQihuang);
        connect(player, &ClientPlayer::pile_changed, this, &PlayerCardContainer::updatePile);
        connect(player, &ClientPlayer::role_changed, _m_roleComboBox, &RoleComboBox::fix);
        connect(player, &ClientPlayer::hp_changed, this, &PlayerCardContainer::updateHp);
        connect(player, &ClientPlayer::removedChanged, this, &PlayerCardContainer::onRemovedChanged);

        QTextDocument *textDoc = m_player->getMarkDoc();
        Q_ASSERT(_m_markItem);
        _m_markItem->setDocument(textDoc);
        connect(textDoc, &QTextDocument::contentsChanged, this, &PlayerCardContainer::updateMarks);
    }
    updateAvatar();
    refresh();
}

QList<CardItem *> PlayerCardContainer::removeDelayedTricks(const QList<int> &cardIds)
{
    QList<CardItem *> result;
    foreach (int card_id, cardIds) {
        CardItem *item = CardItem::FindItem(_m_judgeCards, card_id);
        Q_ASSERT(item != NULL);
        int index = _m_judgeCards.indexOf(item);
        QRect start = _m_layout->m_delayedTrickFirstRegion;
        QPoint step = _m_layout->m_delayedTrickStep;
        start.translate(step * index);
        item->setOpacity(0.0);
        item->setPos(start.center());
        _m_judgeCards.removeAt(index);
        delete _m_judgeIcons.takeAt(index);
        result.append(item);
    }
    updateDelayedTricks();
    return result;
}

void PlayerCardContainer::updateDelayedTricks()
{
    for (int i = 0; i < _m_judgeIcons.size(); i++) {
        QGraphicsPixmapItem *item = _m_judgeIcons[i];
        QRect start = _m_layout->m_delayedTrickFirstRegion;
        QPoint step = _m_layout->m_delayedTrickStep;
        start.translate(step * i);
        item->setPos(start.topLeft());
    }
}

void PlayerCardContainer::addDelayedTricks(QList<CardItem *> &tricks)
{
    foreach (CardItem *trick, tricks) {
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(_getDelayedTrickParent());
        QRect start = _m_layout->m_delayedTrickFirstRegion;
        QPoint step = _m_layout->m_delayedTrickStep;
        start.translate(step * _m_judgeCards.size());
        _paintPixmap(item, start, G_ROOM_SKIN.getCardJudgeIconPixmap(trick->getCard()->objectName()));
        trick->setHomeOpacity(0.0);
        trick->setHomePos(start.center());
        const Card *_trick = trick->getCard();
        const Card *card = Sanguosha->getEngineCard(_trick->getEffectiveId());
        QString toolTip = QString("<b>%1 [%2</b><img src='image/system/log/%3.png' height = 12/><b>%4]</b>")
                              .arg(Sanguosha->translate(_trick->objectName()))
                              .arg(card->objectName() != _trick->objectName() ? Sanguosha->translate(card->objectName()) : "")
                              .arg(_trick->getSuitString())
                              .arg(_trick->getNumberString());
        toolTip.append("<br>");
        toolTip.append(Sanguosha->translate(":" + _trick->objectName()));
        if (Config.value("AutoSkillTypeColorReplacement", true).toBool()) {
            QMap<QString, QColor> skilltype_color_map = Sanguosha->getSkillTypeColorMap();
            foreach (QString skill_type, skilltype_color_map.keys()) {
                QString type_name = Sanguosha->translate(skill_type);
                QString color_name = skilltype_color_map[skill_type].name();
                toolTip.replace(type_name, QString("<font color=%1><b>%2</b></font>").arg(color_name).arg(type_name));
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
                toolTip.replace(suit_char, colored_suit_char);
                toolTip.replace(suit_name, colored_suit_char);
            }
        }
        item->setToolTip(toolTip);
        _m_judgeCards.append(trick);
        _m_judgeIcons.append(item);
    }
}

QPixmap PlayerCardContainer::_getEquipPixmap(const EquipCard *equip)
{
    const Card *realCard = Sanguosha->getEngineCard(equip->getEffectiveId());
    QPixmap equipIcon(_m_layout->m_equipAreas[0].size());
    equipIcon.fill(Qt::transparent);
    QPainter painter(&equipIcon);
    // icon / background
    QRect imageArea = _m_layout->m_equipImageArea;
    if (equip->isKindOf("Horse"))
        imageArea = _m_layout->m_horseImageArea;
    painter.drawPixmap(imageArea, _getPixmap(QSanRoomSkin::S_SKIN_KEY_EQUIP_ICON, equip->objectName()));
    // equip suit
    QRect suitArea = _m_layout->m_equipSuitArea;
    if (equip->isKindOf("Horse"))
        suitArea = _m_layout->m_horseSuitArea;
    painter.drawPixmap(suitArea, G_ROOM_SKIN.getCardSuitPixmap(realCard->getSuit()));
    // equip point
    QRect pointArea = _m_layout->m_equipPointArea;
    if (equip->isKindOf("Horse"))
        pointArea = _m_layout->m_horsePointArea;
    QString numberString = realCard->getNumberString();
    if (numberString == "10")
        numberString = "X";
    if (realCard->isRed()) {
        _m_layout->m_equipPointFontRed.paintText(&painter, pointArea, Qt::AlignLeft | Qt::AlignVCenter, numberString);
    } else {
        _m_layout->m_equipPointFontBlack.paintText(&painter, pointArea, Qt::AlignLeft | Qt::AlignVCenter, numberString);
    }
    return equipIcon;
}

void PlayerCardContainer::setFloatingArea(QRect rect)
{
    _m_floatingAreaRect = rect;
    QPixmap dummy(rect.size());
    dummy.fill(Qt::transparent);
    _m_floatingArea->setPixmap(dummy);
    _m_floatingArea->setPos(rect.topLeft());
    if (_getPhaseParent() == _m_floatingArea)
        updatePhase();
    if (_getMarkParent() == _m_floatingArea)
        updateMarks();
    if (_getProgressBarParent() == _m_floatingArea)
        _updateProgressBar();
}

void PlayerCardContainer::addEquips(QList<CardItem *> &equips)
{
    foreach (CardItem *equip, equips) {
        const EquipCard *equip_card = qobject_cast<const EquipCard *>(equip->getCard()->getRealCard());
        int index = (int)(equip_card->location());
        Q_ASSERT(_m_equipCards[index] == NULL);
        _m_equipCards[index] = equip;
        connect(equip, &CardItem::mark_changed, this, &PlayerCardContainer::_onEquipSelectChanged);
        equip->setHomeOpacity(0.0);
        equip->setHomePos(_m_layout->m_equipAreas[index].center());
        _m_equipRegions[index]->setToolTip(equip_card->getDescription());
        QPixmap pixmap = _getEquipPixmap(equip_card);
        _m_equipLabel[index]->setPixmap(pixmap);

        _mutexEquipAnim.lock();
        _m_equipRegions[index]->setPos(_m_layout->m_equipAreas[index].topLeft()
                                       + QPoint(_m_layout->m_equipAreas[index].width() / 2, 0));
        _m_equipRegions[index]->setOpacity(0);
        _m_equipRegions[index]->show();
        _m_equipAnim[index]->stop();
        _m_equipAnim[index]->clear();
        QPropertyAnimation *anim = new QPropertyAnimation(_m_equipRegions[index], "pos");
        anim->setEndValue(_m_layout->m_equipAreas[index].topLeft());
        anim->setDuration(200);
        _m_equipAnim[index]->addAnimation(anim);
        connect(anim, &QPropertyAnimation::finished, anim, &QPropertyAnimation::deleteLater);
        anim = new QPropertyAnimation(_m_equipRegions[index], "opacity");
        anim->setEndValue(255);
        anim->setDuration(200);
        _m_equipAnim[index]->addAnimation(anim);
        connect(anim, &QPropertyAnimation::finished, anim, &QPropertyAnimation::deleteLater);
        _m_equipAnim[index]->start();
        _mutexEquipAnim.unlock();

        const Skill *skill = Sanguosha->getSkill(equip_card->objectName());
        if (skill == NULL)
            continue;
        emit add_equip_skill(skill, true);
    }
}

QList<CardItem *> PlayerCardContainer::removeEquips(const QList<int> &cardIds)
{
    QList<CardItem *> result;
    foreach (int card_id, cardIds) {
        WrappedCard *wrapped = Sanguosha->getWrappedCard(card_id);
        const EquipCard *equip_card = qobject_cast<const EquipCard *>(wrapped->getRealCard());
        if (!equip_card)
            equip_card = qobject_cast<const EquipCard *>(Sanguosha->getEngineCard(card_id));
        int index = (int)(equip_card->location());
        Q_ASSERT(_m_equipCards[index] != NULL);
        CardItem *equip = _m_equipCards[index];
        equip->setHomeOpacity(0.0);
        equip->setPos(_m_layout->m_equipAreas[index].center());
        result.append(equip);
        _m_equipCards[index] = NULL;
        _mutexEquipAnim.lock();
        _m_equipAnim[index]->stop();
        _m_equipAnim[index]->clear();
        QPropertyAnimation *anim = new QPropertyAnimation(_m_equipRegions[index], "pos");
        anim->setEndValue(_m_layout->m_equipAreas[index].topLeft() + QPoint(_m_layout->m_equipAreas[index].width() / 2, 0));
        anim->setDuration(200);
        _m_equipAnim[index]->addAnimation(anim);
        connect(anim, &QPropertyAnimation::finished, anim, &QPropertyAnimation::deleteLater);
        anim = new QPropertyAnimation(_m_equipRegions[index], "opacity");
        anim->setEndValue(0);
        anim->setDuration(200);
        _m_equipAnim[index]->addAnimation(anim);
        connect(anim, &QPropertyAnimation::finished, anim, &QPropertyAnimation::deleteLater);
        _m_equipAnim[index]->start();
        _mutexEquipAnim.unlock();

        const Skill *skill = Sanguosha->getSkill(equip_card->objectName());
        if (skill != NULL)
            emit remove_equip_skill(skill->objectName());
    }
    return result;
}

void PlayerCardContainer::startHuaShen(QString generalName, QString skillName, bool changeName)
{
    if (m_player == NULL)
        return;
    _m_huashenGeneralName = generalName;
    _m_huashenSkillName = skillName;
    _m_huashenChangeName = changeName;
    //Q_ASSERT(m_player->hasSkill("ikhuanshen"));

    /*bool second_zuoci = m_player && m_player->getGeneralName() != "luna009" && m_player->getGeneral2Name() == "luna009";
    int avatarSize = second_zuoci ? _m_layout->m_smallAvatarSize :
                                    (m_player->getGeneral2() ? _m_layout->m_primaryAvatarSize :
                                                               _m_layout->m_avatarSize);*/
    int avatarSize = _m_layout->m_avatarSize;
    QPixmap pixmap = G_ROOM_SKIN.getGeneralPixmap(generalName, (QSanRoomSkin::GeneralIconSize)avatarSize);

    //QRect animRect = second_zuoci ? _m_layout->m_smallAvatarArea : _m_layout->m_avatarArea;
    QRect animRect = _m_layout->m_avatarArea;
    if (pixmap.size() != animRect.size())
        pixmap = pixmap.scaled(animRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    /*if (second_zuoci)
        pixmap = paintByMask(pixmap);*/

    stopHuaShen();
    if (m_player->getGeneral() && m_player->getGeneral()->objectName() != generalName) {
        _m_huashenAnimation
            = G_ROOM_SKIN.createHuaShenAnimation(pixmap, animRect.topLeft(), _getAvatarParent(), _m_huashenItem);
        _m_huashenAnimation->start();
        if (m_player->isRemoved())
            _m_huashenItem->hide();
        if (changeName) {
            QPixmap name_pixmap = _getPixmap(QSanRoomSkin::S_SKIN_KEY_AVATARNAME, generalName);
            QRect name_rect = _m_layout->m_avatarNameArea;
            if (name_pixmap.size() != name_rect.size())
                name_pixmap = name_pixmap.scaled(name_rect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            _m_huashenNameAnimation
                = G_ROOM_SKIN.createHuaShenAnimation(name_pixmap, name_rect.topLeft(), _getAvatarParent(), _m_huashenNameItem);
            _m_huashenNameAnimation->start();
            if (m_player->isRemoved())
                _m_huashenNameItem->hide();
            QPixmap oldname_pixmap = _getPixmap(QSanRoomSkin::S_SKIN_KEY_AVATARNAME, m_player->getGeneral()->objectName());
            if (oldname_pixmap.size() != name_rect.size())
                oldname_pixmap = oldname_pixmap.scaled(name_rect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            _m_huashenOldNameAnimation = G_ROOM_SKIN.createHuaShenAnimation(oldname_pixmap, name_rect.topLeft(),
                                                                            _getAvatarParent(), _m_huashenOldNameItem, true);
            _m_huashenOldNameAnimation->start();
            if (m_player->isRemoved()) {
                _m_huashenOldNameItem->hide();
                _m_avatarNameItem->show();
            } else {
                _m_huashenOldNameItem->show();
                _m_avatarNameItem->hide();
            }
        }
        _paintPixmap(_m_extraSkillBg, _m_layout->m_extraSkillArea, QSanRoomSkin::S_SKIN_KEY_EXTRA_SKILL_BG,
                     _getAvatarParent());
        if (!skillName.isEmpty())
            _m_extraSkillBg->show();
        else
            _m_extraSkillBg->hide();
        _m_layout->m_extraSkillFont.paintText(_m_extraSkillText, _m_layout->m_extraSkillTextArea, Qt::AlignCenter,
                                              Sanguosha->translate(skillName).left(2));
        if (!skillName.isEmpty()) {
            _m_extraSkillText->show();
            _m_extraSkillBg->setToolTip(Sanguosha->getSkill(skillName)->getDescription(m_player->getSkillStep(skillName)));
        } else
            _m_extraSkillText->hide();
    }
    _adjustComponentZValues();
}

void PlayerCardContainer::stopHuaShen()
{
    if (_m_huashenAnimation != NULL) {
        _m_huashenAnimation->stop();
        delete _m_huashenAnimation;
        _m_huashenAnimation = NULL;
    }
    if (_m_huashenNameAnimation != NULL && _m_huashenOldNameAnimation != NULL) {
        _m_avatarNameItem->show();
        _m_huashenNameAnimation->stop();
        delete _m_huashenNameAnimation;
        _m_huashenNameAnimation = NULL;
        _m_huashenOldNameAnimation->stop();
        delete _m_huashenOldNameAnimation;
        _m_huashenOldNameAnimation = NULL;
    }
    if (_m_huashenItem != NULL) {
        delete _m_huashenItem;
        _m_huashenItem = NULL;
    }
    if (_m_huashenNameItem != NULL) {
        delete _m_huashenNameItem;
        _m_huashenNameItem = NULL;
    }
    if (_m_huashenOldNameItem != NULL) {
        delete _m_huashenOldNameItem;
        _m_huashenOldNameItem = NULL;
    }
    _clearPixmap(_m_extraSkillBg);
    _clearPixmap(_m_extraSkillText);
}

void PlayerCardContainer::updateAvatarTooltip()
{
    if (m_player) {
        QString description = m_player->getSkillDescription();
        _m_avatarArea->setToolTip(description);
        if (m_player->getGeneral2())
            _m_smallAvatarArea->setToolTip(description);
    }
}

PlayerCardContainer::PlayerCardContainer()
{
    _m_layout = NULL;
    _m_avatarArea = _m_smallAvatarArea = NULL;
    _m_avatarNameItem = _m_smallAvatarNameItem = NULL;
    _m_avatarIcon = _m_smallAvatarIcon = _m_circleItem = NULL;
    _m_screenNameItem = NULL;
    _m_chainIcon = _m_faceTurnedIcon = NULL;
    _m_handCardBg = _m_handCardNumText = NULL;
    _m_genderBg = NULL;
    _m_kingdomColorMaskIcon = _m_deathIcon = NULL;
    _m_actionIcon = NULL;
    _m_kingdomIcon = NULL;
    _m_saveMeIcon = NULL;
    _m_phaseIcon = NULL;
    _m_markItem = NULL;
    _m_roleComboBox = NULL;
    m_player = NULL;
    _m_selectedFrame = NULL;

    for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
        _m_equipCards[i] = NULL;
        _m_equipRegions[i] = NULL;
        _m_equipAnim[i] = NULL;
        _m_equipLabel[i] = NULL;
    }
    _m_huashenItem = NULL;
    _m_huashenNameItem = NULL;
    _m_huashenOldNameItem = NULL;
    _m_huashenAnimation = NULL;
    _m_huashenNameAnimation = NULL;
    _m_huashenOldNameAnimation = NULL;
    _m_extraSkillBg = NULL;
    _m_extraSkillText = NULL;

    _m_floatingArea = NULL;
    _m_votesGot = 0;
    _m_maxVotes = 1;
    _m_votesItem = NULL;
    _m_distanceItem = NULL;
    _m_groupMain = new QGraphicsPixmapItem(this);
    _m_groupMain->setFlag(ItemHasNoContents);
    _m_groupMain->setPos(0, 0);
    _m_groupDeath = new QGraphicsPixmapItem(this);
    _m_groupDeath->setFlag(ItemHasNoContents);
    _m_groupDeath->setPos(0, 0);
    _allZAdjusted = false;
}

void PlayerCardContainer::hideAvatars()
{
    if (_m_avatarIcon)
        _m_avatarIcon->hide();
    if (_m_smallAvatarIcon)
        _m_smallAvatarIcon->hide();
}

void PlayerCardContainer::_layUnder(QGraphicsItem *item)
{
    _lastZ--;
    // Q_ASSERT((unsigned long)item != 0xcdcdcdcd);
    if (item)
        item->setZValue(_lastZ--);
    else
        _allZAdjusted = false;
}

bool PlayerCardContainer::_startLaying()
{
    if (_allZAdjusted)
        return false;
    _allZAdjusted = true;
    _lastZ = -1;
    return true;
}

void PlayerCardContainer::_layBetween(QGraphicsItem *middle, QGraphicsItem *item1, QGraphicsItem *item2)
{
    if (middle && item1 && item2)
        middle->setZValue((item1->zValue() + item2->zValue()) / 2.0);
    else
        _allZAdjusted = false;
}

void PlayerCardContainer::_adjustComponentZValues(bool killed)
{
    // all components use negative zvalues to ensure that no other generated
    // cards can be under us.

    // layout
    if (!_startLaying())
        return;
    bool second_zuoci = m_player && m_player->getGeneralName() != "zuoci" && m_player->getGeneral2Name() == "zuoci";

    _layUnder(_m_floatingArea);
    _layUnder(_m_distanceItem);
    _layUnder(_m_votesItem);
    if (!killed) {
        foreach (QGraphicsItem *pile, _m_privatePiles.values())
            _layUnder(pile);
    }
    foreach (QGraphicsItem *judge, _m_judgeIcons)
        _layUnder(judge);
    _layUnder(_m_markItem);
    _layUnder(_m_progressBarItem);
    _layUnder(_m_roleComboBox);
    _layUnder(_m_chainIcon);
    _layUnder(_m_genderBg);
    _layUnder(_m_hpBox);
    _layUnder(_m_handCardNumText);
    _layUnder(_m_handCardBg);
    _layUnder(_m_actionIcon);
    _layUnder(_m_saveMeIcon);
    _layUnder(_m_phaseIcon);
    _layUnder(_m_smallAvatarNameItem);
    if (!killed && _m_huashenNameItem != NULL && _m_huashenOldNameItem != NULL) {
        _layUnder(_m_huashenNameItem);
        _layUnder(_m_huashenOldNameItem);
    }
    _layUnder(_m_avatarNameItem);
    _layUnder(_m_kingdomIcon);
    _layUnder(_m_kingdomColorMaskIcon);
    _layUnder(_m_screenNameItem);
    for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++)
        _layUnder(_m_equipRegions[i]);
    _layUnder(_m_selectedFrame);
    _layUnder(_m_extraSkillText);
    _layUnder(_m_extraSkillBg);
    _layUnder(_m_faceTurnedIcon);
    _layUnder(_m_smallAvatarArea);
    _layUnder(_m_avatarArea);
    _layUnder(_m_circleItem);
    if (!second_zuoci)
        _layUnder(_m_smallAvatarIcon);
    if (!killed)
        _layUnder(_m_huashenItem);
    if (second_zuoci)
        _layUnder(_m_smallAvatarIcon);
    _layUnder(_m_avatarIcon);
}

void PlayerCardContainer::updateRole(const QString &role)
{
    _m_roleComboBox->fix(role);
}

void PlayerCardContainer::_updateProgressBar()
{
    QGraphicsItem *parent = _getProgressBarParent();
    if (parent == NULL)
        return;
    _m_progressBar->setOrientation(_m_layout->m_isProgressBarHorizontal ? Qt::Horizontal : Qt::Vertical);
    QRectF newRect = _m_layout->m_progressBarArea.getTranslatedRect(parent->boundingRect().toRect());
    _m_progressBar->setFixedHeight(newRect.height());
    _m_progressBar->setFixedWidth(newRect.width());
    _m_progressBarItem->setParentItem(parent);
    _m_progressBarItem->setPos(newRect.left(), newRect.top());
}

void PlayerCardContainer::_createControls()
{
    _m_floatingArea = new QGraphicsPixmapItem(_m_groupMain);

    _m_screenNameItem = new QGraphicsPixmapItem(_getAvatarParent());

    _m_avatarArea = new QGraphicsRectItem(_m_layout->m_avatarArea, _getAvatarParent());
    _m_avatarArea->setPen(Qt::NoPen);
    _m_avatarNameItem = new QGraphicsPixmapItem(_getAvatarParent());

    _m_smallAvatarArea = new QGraphicsRectItem(_m_layout->m_smallAvatarArea, _getAvatarParent());
    _m_smallAvatarArea->setPen(Qt::NoPen);
    _m_smallAvatarNameItem = new QGraphicsPixmapItem(_getAvatarParent());

    _m_extraSkillText = new QGraphicsPixmapItem(_getAvatarParent());
    _m_extraSkillText->hide();

    _m_handCardNumText = new QGraphicsPixmapItem(_getAvatarParent());

    _m_hpBox = new MagatamasBoxItem(_getAvatarParent());

    // Now set up progress bar
    _m_progressBar = new QSanCommandProgressBar;
    _m_progressBar->setAutoHide(true);
    _m_progressBar->hide();
    _m_progressBarItem = new QGraphicsProxyWidget(_getProgressBarParent());
    _m_progressBarItem->setWidget(_m_progressBar);
    _updateProgressBar();

    for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
        _m_equipLabel[i] = new QLabel;
        _m_equipLabel[i]->setStyleSheet("QLabel { background-color: transparent; }");
        _m_equipLabel[i]->setPixmap(QPixmap(_m_layout->m_equipAreas[i].size()));
        _m_equipRegions[i] = new QGraphicsProxyWidget();
        _m_equipRegions[i]->setWidget(_m_equipLabel[i]);
        _m_equipRegions[i]->setPos(_m_layout->m_equipAreas[i].topLeft());
        _m_equipRegions[i]->setParentItem(_getEquipParent());
        _m_equipRegions[i]->hide();
        _m_equipAnim[i] = new QParallelAnimationGroup(this);
        ;
    }

    _m_markItem = new QGraphicsTextItem(_getMarkParent());
    _m_markItem->setDefaultTextColor(Qt::black);

    _createRoleComboBox();
    repaintAll();
}

void PlayerCardContainer::_updateDeathIcon()
{
    if (!m_player || !m_player->isDead())
        return;
    QRect deathArea = _m_layout->m_deathIconRegion.getTranslatedRect(_getDeathIconParent()->boundingRect().toRect());
    _paintPixmap(_m_deathIcon, deathArea, QPixmap(m_player->getDeathPixmapPath()), _getDeathIconParent());
    _m_deathIcon->setZValue(30000.0);
}

void PlayerCardContainer::killPlayer()
{
    _m_roleComboBox->fix(m_player->getRole());
    _m_roleComboBox->setEnabled(false);
    _updateDeathIcon();
    _m_saveMeIcon->hide();
    if (_m_votesItem)
        _m_votesItem->hide();
    if (_m_distanceItem)
        _m_distanceItem->hide();
    QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect();
    effect->setColor(_m_layout->m_deathEffectColor);
    effect->setStrength(1.0);
    _m_groupMain->setGraphicsEffect(effect);
    refresh(true);
    if (ServerInfo.GameMode == "04_1v3" && !m_player->isLord()) {
        _m_deathIcon->hide();
        _m_votesGot = 4;
        updateVotes(false, true);
    } else
        _m_deathIcon->show();
}

void PlayerCardContainer::revivePlayer()
{
    _m_votesGot = 0;
    _m_groupMain->setGraphicsEffect(NULL);
    Q_ASSERT(_m_deathIcon);
    _m_deathIcon->hide();
    refresh();
}

void PlayerCardContainer::mousePressEvent(QGraphicsSceneMouseEvent *)
{
}

void PlayerCardContainer::updateVotes(bool need_select, bool display_1)
{
    if ((need_select && !isSelected()) || _m_votesGot < 1 || (!display_1 && _m_votesGot == 1))
        _clearPixmap(_m_votesItem);
    else {
        _paintPixmap(_m_votesItem, _m_layout->m_votesIconRegion,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_VOTES_NUMBER, QString::number(_m_votesGot)), _getAvatarParent());
        _m_votesItem->setZValue(1);
        _m_votesItem->show();
    }
}

void PlayerCardContainer::updateReformState()
{
    _m_votesGot--;
    updateVotes(false, true);
}

void PlayerCardContainer::showDistance()
{
    int dis = Self->distanceTo(m_player);
    if (dis > 0) {
        _paintPixmap(_m_distanceItem, _m_layout->m_votesIconRegion,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_VOTES_NUMBER, QString::number(dis)), _getAvatarParent());
        _m_distanceItem->setZValue(1.1);
        if (!Self->inMyAttackRange(m_player)) {
            QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect();
            effect->setColor(_m_layout->m_deathEffectColor);
            effect->setStrength(1.0);
            _m_distanceItem->setGraphicsEffect(effect);
        } else {
            _m_distanceItem->setGraphicsEffect(NULL);
        }
    } else {
        delete _m_distanceItem;
        _m_distanceItem = NULL;
    }
    if (!_m_distanceItem)
        return;
    if (!_m_distanceItem->isVisible())
        _m_distanceItem->show();
}

void PlayerCardContainer::hideDistance()
{
    if (_m_distanceItem && _m_distanceItem->isVisible())
        _m_distanceItem->hide();
}

void PlayerCardContainer::onRemovedChanged()
{
    bool is_removed = m_player->isRemoved();
    if (_m_huashenItem)
        _m_huashenItem->setVisible(!is_removed);
    if (_m_huashenNameItem && _m_huashenOldNameItem) {
        _m_huashenNameItem->setVisible(!is_removed);
        _m_huashenOldNameItem->setVisible(!is_removed);
        _m_avatarNameItem->setVisible(is_removed);
    }

    QAbstractAnimation::Direction direction = is_removed ? QAbstractAnimation::Forward : QAbstractAnimation::Backward;

    _getPlayerRemovedEffect()->setDirection(direction);
    _getPlayerRemovedEffect()->start();
}

void PlayerCardContainer::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem *item = getMouseClickReceiver();
    if (item != NULL && item->isUnderMouse() && isEnabled() && (flags() & QGraphicsItem::ItemIsSelectable)) {
        if (event->button() == Qt::RightButton)
            setSelected(false);
        else if (event->button() == Qt::LeftButton) {
            _m_votesGot++;
            setSelected(_m_votesGot <= _m_maxVotes);
            if (_m_votesGot > 1)
                emit selected_changed();
        }
        updateVotes();
    }
}

void PlayerCardContainer::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *)
{
    if (Config.EnableDoubleClick)
        RoomSceneInstance->doOkButton();
}

QVariant PlayerCardContainer::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged) {
        if (!value.toBool()) {
            _m_votesGot = 0;
            _clearPixmap(_m_selectedFrame);
            _m_selectedFrame->hide();
        } else {
            _paintPixmap(_m_selectedFrame, _m_layout->m_focusFrameArea,
                         _getPixmap(QSanRoomSkin::S_SKIN_KEY_SELECTED_FRAME, true), _getFocusFrameParent());
            _m_selectedFrame->show();
        }
        updateVotes();
        emit selected_changed();
    } else if (change == ItemEnabledHasChanged) {
        _m_votesGot = 0;
        emit enable_changed();
    }

    return QGraphicsObject::itemChange(change, value);
}

void PlayerCardContainer::_onEquipSelectChanged()
{
}

bool PlayerCardContainer::canBeSelected()
{
    QGraphicsItem *item1 = getMouseClickReceiver();
    return item1 && isEnabled() && (flags() & QGraphicsItem::ItemIsSelectable);
}
