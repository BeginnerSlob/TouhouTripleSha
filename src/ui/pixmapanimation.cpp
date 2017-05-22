#include "pixmapanimation.h"
#include "skin-bank.h"
#include "roomscene.h"

#include <QPainter>
#include <QPixmapCache>
#include <QDir>
#include <QTimer>

const int PixmapAnimation::S_DEFAULT_INTERVAL = 33;

PixmapAnimation::PixmapAnimation()
    : QGraphicsItem(0)
{
}

void PixmapAnimation::advance(int phase) {
    if (phase) current++;
    if (current >= frames.size()) {
        current = 0;
        emit finished();
    }
    update();
}

void PixmapAnimation::setPath(const QString &path) {
    frames.clear();
    current = 0;

    int i = 0;
    QString pic_path = QString("%1%2%3").arg(path).arg(i++).arg(".png");
    do {
        frames << G_ROOM_SKIN.getPixmapFromFileName(pic_path, true);
        pic_path = QString("%1%2%3").arg(path).arg(i++).arg(".png");
    } while(QFile::exists(pic_path));
}

void PixmapAnimation::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->drawPixmap(0, 0, frames.at(current));
}

QRectF PixmapAnimation::boundingRect() const{
    return frames.at(current).rect();
}

bool PixmapAnimation::valid() {
    return !frames.isEmpty();
}

void PixmapAnimation::timerEvent(QTimerEvent *) {
    advance(1);
}

void PixmapAnimation::start(bool permanent, int interval) {
    _m_timerId = startTimer(interval);
    if (!permanent) connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void PixmapAnimation::stop() {
    killTimer(_m_timerId);
}

void PixmapAnimation::preStart() {
    this->show();
    this->startTimer(S_DEFAULT_INTERVAL);
}

PixmapAnimation *PixmapAnimation::GetPixmapAnimation(QGraphicsItem *parent, const QString &emotion) {
    PixmapAnimation *pma = new PixmapAnimation();
    pma->setPath(QString("image/system/emotion/%1/").arg(emotion));
    if (pma->valid()) {
        if (emotion == "no-success") {
            pma->moveBy(pma->boundingRect().width() * 0.25,
                        pma->boundingRect().height() * 0.25);
            pma->setScale(0.5);
        } else if (emotion == "success") {
            pma->moveBy(pma->boundingRect().width() * 0.1,
                        pma->boundingRect().height() * 0.1);
            pma->setScale(0.8);
        } else if (emotion == "effects/dismantlement") {
            pma->moveBy(pma->boundingRect().width() * 0.4,
                        pma->boundingRect().height() * 0.3);
            pma->setScale(0.4);
        } else if (emotion == "effects/god_salvation") {
            pma->moveBy(pma->boundingRect().width() * 0.2,
                        pma->boundingRect().height() * 0.2);
            pma->setScale(0.6);
        } else if (emotion == "effects/amazing_grace") {
            pma->moveBy(-pma->boundingRect().width() * 0.9,
                        -pma->boundingRect().height() * 0.9);
            pma->setScale(2.8);
        } else if (emotion == "effects/burning_camps") {
            pma->moveBy(-pma->boundingRect().width() * 0.25,
                        -pma->boundingRect().height() * 0.25);
            pma->setScale(1.5);
        } else if (emotion == "effects/savage_assault") {
            pma->moveBy(pma->boundingRect().width() * 0.1,
                        pma->boundingRect().height() * 0.1);
            pma->setScale(0.8);
        } else if (emotion == "effects/archery_attack") {
            pma->moveBy(-pma->boundingRect().width() * 0.35,
                        -pma->boundingRect().height() * 0.3);
            pma->setScale(1.8);
        } else if (emotion == "effects/snatch") {
            pma->moveBy(pma->boundingRect().width() * 0.25,
                        pma->boundingRect().height() * 0.25);
            pma->setScale(0.5);
        } else if (emotion == "effects/collateral_slash") {
            pma->moveBy(pma->boundingRect().width() * 0.4,
                        pma->boundingRect().height() * 0.2);
            pma->setScale(0.6);
        } else if (emotion == "effects/lightning") {
            pma->moveBy(-pma->boundingRect().width() * 0.15,
                        -pma->boundingRect().height() * 0.25);
            pma->setScale(1.8);
        } else if (emotion == "effects/lure_tiger") {
            pma->moveBy(pma->boundingRect().width() * 0.2,
                        pma->boundingRect().height() * 0.2);
            pma->setScale(0.6);
        } else if (emotion == "effects/fire_attack") {
            pma->moveBy(pma->boundingRect().width() * 0.2,
                        pma->boundingRect().height() * 0.2);
            pma->setScale(0.6);
        } else if (emotion == "effects/iron_chain") {
            pma->moveBy(pma->boundingRect().width() * 0.25,
                        pma->boundingRect().height() * 0.25);
            pma->setScale(0.5);
        } else if (emotion == "effects/supply_shortage") {
            pma->moveBy(pma->boundingRect().width() * 0.1,
                        pma->boundingRect().height() * 0.1);
            pma->setScale(0.8);
        } else if ((emotion.startsWith("effects/") && emotion.contains("slash"))
                   || (emotion.endsWith("damage/normal") || emotion.endsWith("damage/heavy"))) {
            pma->moveBy(pma->boundingRect().width() * 0.2,
                        pma->boundingRect().height() * 0.2);
            pma->setScale(0.6);
        } else if (emotion == "effects/hplost") {
            pma->moveBy(pma->boundingRect().width() * 0.1,
                        pma->boundingRect().height() * 0.1);
            pma->setScale(0.8);
        } else if (emotion == "effects/armor") {
            pma->moveBy(pma->boundingRect().width() * 0.1,
                        pma->boundingRect().height() * 0.1);
            pma->setScale(0.8);
        } else if (emotion == "effects/wake") {
            pma->moveBy(0, -pma->boundingRect().width() * 0.05);
        } else if (emotion == "effects/rout") {
            pma->moveBy(pma->boundingRect().width() * 0.03, -pma->boundingRect().width() * 0.1);
        }
/*
        else if (emotion == "effects/amazing_grace") {
            pma->moveBy(-pma->boundingRect().width() * 0.9,
                        -pma->boundingRect().height() * 0.9);
            pma->setScale(2.8);
        } else if (emotion == "effects/drowning") {
            pma->moveBy(-pma->boundingRect().width() * 0.15,
                        -pma->boundingRect().height() * 0.15);
            pma->setScale(1.3);
        } else if (emotion == "effects/burning_camps") {
            pma->moveBy(-pma->boundingRect().width() * 0.4,
                        -pma->boundingRect().height() * 0.25);
            pma->setScale(1.5);
        }
*/
        if (emotion != "effects/burning_camps")
            pma->moveBy((parent->boundingRect().width() - pma->boundingRect().width()) * 0.5,
                        (parent->boundingRect().height() - pma->boundingRect().height()) * 0.5);

        pma->setParentItem(parent);
        pma->setZValue(20002.0);
        if (emotion.contains("weapon")) {
            pma->hide();
            QTimer::singleShot(600, pma, SLOT(preStart()));
        } else if (emotion.contains("effects")) {
            if (parent->data(998).toString() == "light_box") {
                pma->setZValue(20002.0);
                if (emotion != "effects/burning_camps")
                    pma->moveBy(0, - pma->boundingRect().height() * 0.1);
                connect(pma, SIGNAL(finished()), RoomSceneInstance, SLOT(removeLightBox()));
            }
            pma->hide();
            QTimer::singleShot(33, pma, SLOT(preStart()));
        } else
            pma->startTimer(S_DEFAULT_INTERVAL);

        connect(pma, SIGNAL(finished()), pma, SLOT(deleteLater()));
        return pma;
    } else {
        delete pma;
        return NULL;
    }
}

QPixmap PixmapAnimation::GetFrameFromCache(const QString &filename) {
    QPixmap pixmap;
    if (!QPixmapCache::find(filename, &pixmap)) {
        if (pixmap.load(filename))
            QPixmapCache::insert(filename, pixmap);
    }
    return pixmap;
}

int PixmapAnimation::GetFrameCount(const QString &emotion) {
    QString path = QString("image/system/emotion/%1/").arg(emotion);
    QDir dir(path);
    dir.setNameFilters(QStringList("*.png"));
    return dir.entryList(QDir::Files | QDir::NoDotAndDotDot).count();
}

