#include "standard.h"
#include "skill.h"
#include "wind.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "ai.h"
#include "general.h"

// skill cards

class Jushou: public PhaseChangeSkill{
public:
    Jushou():PhaseChangeSkill("jushou"){

    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if(target->getPhase() == Player::Finish){
            Room *room = target->getRoom();
            if(room->askForSkillInvoke(target, objectName())){
                target->drawCards(3);
                target->turnOver();

                room->broadcastSkillInvoke(objectName());
            }
        }

        return false;
    }
};

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>

GuhuoCard::GuhuoCard(){
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool GuhuoCard::guhuo(ServerPlayer* yuji, const QString& message) const{
    Room *room = yuji->getRoom();
    room->setTag("Guhuoing", true);
    room->setTag("GuhuoType", this->user_string);

    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);
    QSet<ServerPlayer *> questioned;

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    foreach(int card_id, getSubcards()){
        used_cards << card_id;
    }

    foreach(ServerPlayer *player, players){
        if(player->getHp() <= 0){
            LogMessage log;
            log.type = "#GuhuoCannotQuestion";
            log.from = player;
            log.arg = QString::number(player->getHp());
            room->sendLog(log);

            room->setEmotion(player, "no-question");

            continue;
        }

        if(player->getState()=="online")
            player->invoke("log", message);
        QString choice = room->askForChoice(player, "guhuo", "noquestion+question");
        if(choice == "question"){
            room->setEmotion(player, "question");
            questioned << player;
        }else
            room->setEmotion(player, "no-question");

        LogMessage log;
        log.type = "#GuhuoQuery";
        log.from = player;
        log.arg = choice;

        room->sendLog(log);
    }
    
    LogMessage log;
    log.type = "$GuhuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    bool success = false;
    if(questioned.isEmpty()){
        success = true;

        foreach(ServerPlayer *player, players)
            room->setEmotion(player, ".");
        if(yuji->getPhase() == Player::Play){
            CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), user_string);
            CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceTable, reason);
            moves.append(move);
            room->moveCardsAtomic(moves, true);
        }
        else if(yuji->getPhase() == Player::NotActive)
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                CardMoveReason(CardMoveReason::S_REASON_RESPONSE, yuji->objectName(), QString(), user_string), true);

    }else{
        const Card *card = Sanguosha->getCard(subcards.first());
        bool real;
        if(user_string == "peach+analeptic")
            real = card->objectName() == "peach" || card->objectName() == "analeptic";
        else
            real = card->match(user_string);

        success = real && card->getSuit() == Card::Heart;
        if(success && yuji->getPhase() == Player::Play){
            CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), user_string);
            CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceTable, reason);
            moves.append(move);
            room->moveCardsAtomic(moves, true);
        }
        else if(success && yuji->getPhase() == Player::NotActive){
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                CardMoveReason(CardMoveReason::S_REASON_RESPONSE, yuji->objectName(), QString(), user_string), true);
        }
        else{
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), user_string), true);
        }
        foreach(ServerPlayer *player, players){
            room->setEmotion(player, ".");

            if(questioned.contains(player)){
                if(real)
                    room->loseHp(player);
                else
                    player->drawCards(1);
            }
        }
    }
    
    room->setTag("Guhuoing", false);
    room->removeTag("GuhuoType");
    if(!success)
        room->setPlayerFlag(yuji, "guhuo_failed");

    return success;
}

GuhuoDialog *GuhuoDialog::getInstance(const QString &object, bool left, bool right){
    static GuhuoDialog *instance;
    if(instance == NULL || instance->objectName() != object)
        instance = new GuhuoDialog(object, left, right);

    return instance;
}

GuhuoDialog::GuhuoDialog(const QString &object, bool left, bool right):object_name(object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object));
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    if(left)
        layout->addWidget(createLeft());
    if(right)
        layout->addWidget(createRight());

    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(selectCard(QAbstractButton*)));
}

void GuhuoDialog::popup(){
    //if(ClientInstance->getStatus() != Client::Playing)
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY)
        return;

    foreach(QAbstractButton *button, group->buttons()){
        const Card *card = map[button->objectName()];
        bool enabled = !Self->isCardLimited(card, Card::MethodUse, true) && card->isAvailable(Self);
        button->setEnabled(enabled);
    }

    Self->tag.remove(object_name);
    exec();
}

void GuhuoDialog::selectCard(QAbstractButton *button){
    CardStar card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card);
    emit onButtonClick();
    accept();
}

QGroupBox *GuhuoDialog::createLeft(){
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach(const Card *card, cards){
        if(card->getTypeId() == Card::TypeBasic && !map.contains(card->objectName())){
            Card *c = Sanguosha->cloneCard(card->objectName(), Card::NoSuitNoColor, 0);
            c->setParent(this);

            layout->addWidget(createButton(c));
        }
    }

    layout->addStretch();

    box->setLayout(layout);
    return box;
}

QGroupBox *GuhuoDialog::createRight(){
    QGroupBox *box = new QGroupBox(Sanguosha->translate("ndtrick"));
    QHBoxLayout *layout = new QHBoxLayout;

    QGroupBox *box1 = new QGroupBox(Sanguosha->translate("single_target"));
    QVBoxLayout *layout1 = new QVBoxLayout;

    QGroupBox *box2 = new QGroupBox(Sanguosha->translate("multiple_targets"));
    QVBoxLayout *layout2 = new QVBoxLayout;


    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach(const Card *card, cards){
        if(card->isNDTrick() && !map.contains(card->objectName())){
            Card *c = Sanguosha->cloneCard(card->objectName(), Card::NoSuitNoColor, 0);
            c->setSkillName(object_name);
            c->setParent(this);

            QVBoxLayout *layout = c->isKindOf("SingleTargetTrick") ? layout1 : layout2;
            layout->addWidget(createButton(c));
        }
    }

    box->setLayout(layout);
    box1->setLayout(layout1);
    box2->setLayout(layout2);

    layout1->addStretch();
    layout2->addStretch();

    layout->addWidget(box1);
    layout->addWidget(box2);
    return box;
}

QAbstractButton *GuhuoDialog::createButton(const Card *card){
    QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(card->objectName()));
    button->setObjectName(card->objectName());
    button->setToolTip(card->getDescription());

    map.insert(card->objectName(), card);
    group->addButton(button);

    return button;
}

bool GuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    CardStar card = Self->tag.value("guhuo").value<CardStar>();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card);
}

bool GuhuoCard::targetFixed() const{
    if(Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE)
    {
        if(!ClientInstance->hasNoTargetResponding()){
            CardStar card = Sanguosha->cloneCard(user_string, NoSuitNoColor, 0);
            Self->tag["guhuo"] = QVariant::fromValue(card);
            return card && card->targetFixed();
        }
        else return true;
    }

    CardStar card = Self->tag.value("guhuo").value<CardStar>();
    return card && card->targetFixed();
}

bool GuhuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    CardStar card = Self->tag.value("guhuo").value<CardStar>();
    return card && card->targetsFeasible(targets, Self);
}

const Card *GuhuoCard::validate(const CardUseStruct *card_use) const{
    Room *room = card_use->from->getRoom();
    room->broadcastSkillInvoke("guhuo");

    LogMessage log;
    log.type = card_use->to.isEmpty() ? "#GuhuoNoTarget" : "#Guhuo";
    log.from = card_use->from;
    log.to = card_use->to;
    log.arg = user_string;
    log.arg2 = "guhuo";

    room->sendLog(log);

    if(guhuo(card_use->from, log.toString())){
        const Card *card = Sanguosha->getCard(subcards.first());
        Card *use_card = Sanguosha->cloneCard(user_string, card->getSuit(), card->getNumber());
        use_card->setSkillName("guhuo");
        use_card->addSubcard(this);
        /* @todo: verify this...
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, card_use->from->objectName());
        room->throwCard(this, reason, NULL); */
        use_card->deleteLater();
        return use_card;
    }else
        return NULL;
}

const Card *GuhuoCard::validateInResposing(ServerPlayer *yuji, bool &continuable) const{
    continuable = true;
    Room *room = yuji->getRoom();
    room->broadcastSkillInvoke("guhuo");

    QString to_guhuo;
    if(user_string == "peach+analeptic")
        to_guhuo = room->askForChoice(yuji, "guhuo-saveself", user_string);
    else
        to_guhuo = user_string;

    LogMessage log;
    log.type = "#GuhuoNoTarget";
    log.from = yuji;
    log.arg = to_guhuo;
    log.arg2 = "guhuo";
    room->sendLog(log);

    if (guhuo(yuji,log.toString())){
        const Card *card = Sanguosha->getCard(subcards.first());
        Card *use_card = Sanguosha->cloneCard(to_guhuo, card->getSuit(), card->getNumber());
        use_card->setSkillName("guhuo");
        use_card->deleteLater();
        return use_card;
    }else
        return NULL;
}

class Guhuo: public OneCardViewAsSkill{
public:
    Guhuo():OneCardViewAsSkill("guhuo"){
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return !player->isKongcheng()
                && ! pattern.startsWith("@")
                && ! pattern.startsWith(".");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if(Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
            GuhuoCard *card = new GuhuoCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        CardStar c = Self->tag.value("guhuo").value<CardStar>();
        if(c) {
            GuhuoCard *card = new GuhuoCard;
            card->setUserString(c->objectName());
            card->addSubcard(originalCard);

            return card;
        } else return NULL;
    }

    virtual QDialog *getDialog() const{
        return GuhuoDialog::getInstance("guhuo");
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 0;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return !player->isKongcheng();
    }
};

WindPackage::WindPackage()
    :Package("wind")
{
    /*General *xiahouyuan, *caoren, *huangzhong, *weiyan, *zhangjiao, *zhoutai;

    xiahouyuan = new General(this, "xiahouyuan", "wei");
    xiahouyuan->addSkill(new Shensu);

    caoren = new General(this, "caoren", "wei");
    caoren->addSkill(new Jushou);

    General *yuji = new General(this, "yuji", "qun", 3);
    yuji->addSkill(new Guhuo);

    addMetaObject<GuhuoCard>();
    */
}

ADD_PACKAGE(Wind)
