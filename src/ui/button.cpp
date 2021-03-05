#include "button.h"
#include "audio.h"

#include <QFontDatabase>
#include <QGraphicsRotation>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>

static QRectF ButtonRect(0, 0, 189, 46);

Button::Button(const QString &label, qreal scale)
    : label(label)
    , size(ButtonRect.size() * scale)
    , mute(true)
{
    int fontId = QFontDatabase::addApplicationFont("font/home.ttf");
    QString fzlb = QFontDatabase::applicationFontFamilies(fontId).at(0);
    font = QFont(fzlb, 23);
    title = QPixmap(size.toSize());
    outimg = QImage(size.toSize(), QImage::Format_ARGB32);
    init();
}

Button::Button(const QString &label, const QSizeF &size)
    : label(label)
    , size(size)
    , mute(true)
{
    int fontId = QFontDatabase::addApplicationFont("font/home.ttf");
    QString fzlb = QFontDatabase::applicationFontFamilies(fontId).at(0);
    font = QFont(fzlb, 23);
    title = QPixmap(size.toSize());
    outimg = QImage(size.toSize(), QImage::Format_ARGB32);
    init();
}

Button::Button(const QString &label, const QSizeF &size, const QFont &font)
    : label(label)
    , size(size)
    , mute(true)
    , font(font)
{
    title = QPixmap(size.toSize());
    outimg = QImage(size.toSize(), QImage::Format_ARGB32);
    init();
}

void Button::init()
{
    setFlags(ItemIsFocusable);

    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    title.fill(QColor(0, 0, 0, 0));
    QPainter pt(&title);
    pt.setFont(font);
    pt.setPen(Config.TextEditColor);
    pt.setRenderHint(QPainter::TextAntialiasing);
    pt.drawText(boundingRect(), Qt::AlignCenter, label);

    title_item = new QGraphicsPixmapItem(this);
    title_item->setPixmap(title);
    title_item->show();

    de = new QGraphicsDropShadowEffect;
    de->setOffset(0);
    de->setBlurRadius(12);
    de->setColor(QColor(244, 242, 248));

    title_item->setGraphicsEffect(de);

    QImage bgimg("image/system/button/button.png");

    qreal pad = 10;

    int w = bgimg.width();
    int h = bgimg.height();

    int tw = outimg.width();
    int th = outimg.height();

    qreal xc = (w - 2 * pad) / (tw - 2 * pad);
    qreal yc = (h - 2 * pad) / (th - 2 * pad);

    for (int i = 0; i < tw; i++) {
        for (int j = 0; j < th; j++) {
            int x = i;
            int y = j;

            if (x >= pad && x <= (tw - pad))
                x = pad + (x - pad) * xc;
            else if (x >= (tw - pad))
                x = w - (tw - x);

            if (y >= pad && y <= (th - pad))
                y = pad + (y - pad) * yc;
            else if (y >= (th - pad))
                y = h - (th - y);

            QRgb rgb = bgimg.pixel(x, y);
            outimg.setPixel(i, j, rgb);
        }
    }

    effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(5);
    effect->setOffset(this->boundingRect().height() / 7.0);
    effect->setColor(QColor(0, 0, 0, 200));
    this->setGraphicsEffect(effect);

    glow = 0;
    timer_id = 0;
}

Button::~Button()
{
    de->deleteLater();
    effect->deleteLater();
}

void Button::setMute(bool mute)
{
    this->mute = mute;
}

void Button::setFont(const QFont &font)
{
    this->font = font;
    title.fill(QColor(0, 0, 0, 0));
    QPainter pt(&title);
    pt.setFont(font);
    pt.setPen(Config.TextEditColor);
    pt.setRenderHint(QPainter::TextAntialiasing);
    pt.drawText(boundingRect(), Qt::AlignCenter, label);

    title_item->setPixmap(title);
}

#include "engine.h"

void Button::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    setFocus(Qt::MouseFocusReason);
    if (!mute)
        Sanguosha->playSystemAudioEffect("button-hover", false);
    if (!timer_id)
        timer_id = QObject::startTimer(40);
}

void Button::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (boundingRect().contains(event->pos()))
        event->accept();
}

void Button::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (boundingRect().contains(event->pos())) {
        if (!mute)
            Sanguosha->playSystemAudioEffect("button-down", false);
        emit clicked();
    }
}

QRectF Button::boundingRect() const
{
    return QRectF(QPointF(), size);
}

void Button::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QRectF rect = boundingRect();

    painter->drawImage(rect, outimg);
    painter->fillRect(rect, QColor(255, 255, 255, glow * 10));
}

void Button::timerEvent(QTimerEvent *)
{
    update();
    if (hasFocus()) {
        if (glow < 5)
            glow++;
    } else {
        if (glow > 0)
            glow--;
        else if (timer_id) {
            QObject::killTimer(timer_id);
            timer_id = 0;
        }
    }
}
