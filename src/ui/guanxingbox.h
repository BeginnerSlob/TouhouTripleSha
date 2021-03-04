#ifndef GUANXINGBOX_H
#define GUANXINGBOX_H

#include "cardcontainer.h"

class GuanxingBox : public CardContainer
{
    Q_OBJECT

public:
    GuanxingBox();
    void reply();
    virtual QRectF boundingRect() const;

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    void doGuanxing(const QList<int> &card_ids, bool up_only);
    void clear();

    void mirrorGuanxingStart(const QString &who, bool up_only, const QList<int> &cards);
    void mirrorGuanxingMove(int from, int to);

private slots:
    void onItemReleased();
    void onItemClicked();

private:
    QList<CardItem *> upItems, downItems;
    bool up_only;
    void adjust();
    int itemNumberOfFirstRow() const;
    bool isOneRow() const;
    QString zhuge;
};

#endif // GUANXINGBOX_H
