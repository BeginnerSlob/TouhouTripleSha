#include "touhou.h"

#include "general.h"
#include "skill.h"
#include "room.h"
#include "carditem.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "general.h"

class ThBian:public TriggerSkill{
public:
    ThBian():TriggerSkill("thbian"){
        events << Dying << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());

        if(splayer == NULL)
            return false;

        if(triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct &change = data.value<PhaseChangeStruct>();
            if(change.to == Player::NotActive && splayer->getMark("thbian") > 0)
                splayer->removeMark("thbian");

            return false;
        }

        if(player->getHp() > 0)
            return false;

        if(splayer->getMark("thbian") > 0 || splayer->getHandcardNum() < 2)
            return false;

        if(splayer->askForSkillInvoke(objectName()))
            if(room->askForDiscard(splayer, objectName(), 2, 2, true, false)) {
                room->broadcastSkillInvoke(objectName());
                splayer->addMark("thbian");
                room->loseHp(player);
            }

        return false;
    }
};

class ThGuihang:public TriggerSkill{
public:
    ThGuihang():TriggerSkill("thguihang"){
        events << Dying << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
        if(splayer == NULL)
            return false;

        if(player->getHp() > 0)
            return false;

        if(player->isKongcheng())
            return false;

        if(room->askForSkillInvoke(splayer, objectName())) {
            int card_id;
            if(splayer == player)
                card_id = room->askForCardShow(splayer, splayer, "@thguihang")->getId();
            else
                card_id = room->askForCardChosen(splayer, player, "h", objectName());
            
            room->showCard(player, card_id);

            room->broadcastSkillInvoke(objectName());
            const Card *card = Sanguosha->getCard(card_id);
            if(card->isBlack())
                return false;

            room->throwCard(card, player);
            RecoverStruct recover;
            recover.who = splayer;
            room->recover(player, recover);
        }

        return false;
    }
};

ThWujianCard::ThWujianCard(){
}

bool ThWujianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->inMyAttackRange(to_select) && to_select != Self;
}

void ThWujianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    targets.first()->gainMark("@wujian");
}

class ThWujianViewAsSkill:public OneCardViewAsSkill{
public:
    ThWujianViewAsSkill():OneCardViewAsSkill("thwujian"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThWujianCard");
    }

    virtual bool viewFilter(const Card* to_select) const{
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        ThWujianCard *card = new ThWujianCard;
        card->addSubcard(originalCard->getId());
        return card;
    }
};

class ThWujian:public TriggerSkill{
public:
    ThWujian():TriggerSkill("thwujian"){
        events << EventPhaseChanging << Death;
        view_as_skill = new ThWujianViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    };

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct &change = data.value<PhaseChangeStruct>();
            if(change.to != Player::Start)
                return false;
        }
        
        foreach(ServerPlayer *p, room->getAllPlayers())
            if(p->getMark("@wujian") > 0)
                p->loseAllMarks("@wujian");

        return false;
    }
};

class ThWujianDistanceSkill:public DistanceSkill{
public:
    ThWujianDistanceSkill():DistanceSkill("#thwujian"){
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(from->getMark("@wujian") > 0)
            return 1;
        else
            return 0;
    }
};

ThXihuaCard::ThXihuaCard(){
}

bool ThXihuaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if(Sanguosha->getCard(getEffectiveId())->getSuit() != Card::Spade)
        return targets.size() == 1;
    else
        return targets.isEmpty();
}

bool ThXihuaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Card::Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    if(suit == Card::Spade)
        return false;
    else if(suit == Card::Diamond)
        return targets.isEmpty() && !to_select->isAllNude() && to_select != Self;
    else
        return targets.isEmpty() && to_select != Self;
}

void ThXihuaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    Card::Suit suit = Sanguosha->getCard(getEffectiveId())->getSuit();
    ServerPlayer *target = NULL;
    if(!targets.isEmpty())
        target = targets.first();

    if(suit == Card::Spade) {
        RecoverStruct recover;
        recover.who = source;
        room->recover(source, recover);
    }
    else if(suit == Card::Heart) {
        target->drawCards(1);
        target->turnOver();
    }
    else if(suit == Card::Club) {
        target->drawCards(2);
        if(!target->isNude())
            room->askForDiscard(target, "thxihua", 1, 1, false, true);
    }
    else if(suit == Card::Diamond) {
        int card_id = room->askForCardChosen(source, target, "hej", "thxihua");
        room->throwCard(card_id, target, source);
    }
}

class ThXihua:public ViewAsSkill{
public:
    ThXihua():ViewAsSkill("thxihua"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if(selected.isEmpty())
            return true;
        else if(selected.length() == 1)
            return to_select->getSuit() == selected.first()->getSuit();
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if(cards.length() == 2){
            ThXihuaCard *card = new ThXihuaCard;
            card->addSubcards(cards);
            return card;
        }else
            return NULL;
    }
};

class ThAnyun:public TriggerSkill{
public:
	ThAnyun():TriggerSkill("thanyun"){
		events << EventPhaseStart;
	}

	virtual bool trigger(TriggerEvent ,Room *room, ServerPlayer *player, QVariant &data) const{
		if(player->getPhase() == Player::Draw && player->askForSkillInvoke(objectName())) {
			room->broadcastSkillInvoke(objectName(), qrand() % 2 + 1);
			room->setPlayerFlag(player, "thanyun");
			return true;
		}
		else if(player->getPhase() == Player::Finish && player->hasFlag("thanyun")) {
			room->setPlayerFlag(player, "-thanyun");
			room->broadcastSkillInvoke(objectName(), 3);
			player->drawCards(2);
		}
		else if(player->getPhase() == Player::NotActive && player->hasFlag("thanyun"))
			room->setPlayerFlag(player, "-thanyun");

		return false;
	}
};

ThMimengCard::ThMimengCard(){
    mute = true;
    will_throw = false;
}

ThMimengDialog *ThMimengDialog::getInstance(const QString &object){
    static ThMimengDialog *instance;
    if(instance == NULL || instance->objectName() != object)
        instance = new ThMimengDialog(object);

    return instance;
}

ThMimengDialog::ThMimengDialog(const QString &object):object_name(object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object));
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(createLeft());
    layout->addWidget(createRight());

    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(selectCard(QAbstractButton*)));
}

void ThMimengDialog::popup(){
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY)
        return;

    foreach(QAbstractButton *button, group->buttons()){
        const Card *card = map[button->objectName()];
        bool enabled = card->isAvailable(Self);
        button->setEnabled(enabled);
    }

    Self->tag.remove(object_name);
    exec();
}

void ThMimengDialog::selectCard(QAbstractButton *button){
    CardStar card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card);
    emit onButtonClick();
    accept();
}

QGroupBox *ThMimengDialog::createLeft(){
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach(const Card *card, cards){
        if(card->getTypeId() == Card::TypeBasic && !map.contains(card->objectName())
           && !Config.BanPackages.contains(card->getPackage())){
            Card *c = Sanguosha->cloneCard(card->objectName(), Card::NoSuitNoColor, 0);
            c->setParent(this);

            layout->addWidget(createButton(c));
        }
    }

    layout->addStretch();

    box->setLayout(layout);
    return box;
}

QGroupBox *ThMimengDialog::createRight(){
    QGroupBox *box = new QGroupBox(Sanguosha->translate("ndtrick"));
    QHBoxLayout *layout = new QHBoxLayout;

    QGroupBox *box1 = new QGroupBox(Sanguosha->translate("single_target"));
    QVBoxLayout *layout1 = new QVBoxLayout;

    QGroupBox *box2 = new QGroupBox(Sanguosha->translate("multiple_targets"));
    QVBoxLayout *layout2 = new QVBoxLayout;


    QStringList ban_list;
    ban_list << "ExNihilo" << "AmazingGrace" << "GodSalvation" << "ArcheryAttack" << "Snatch";
    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach(const Card *card, cards){
        if(card->isNDTrick() && !map.contains(card->objectName()) && !ban_list.contains(card->getClassName())
           && !Config.BanPackages.contains(card->getPackage())){
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

QAbstractButton *ThMimengDialog::createButton(const Card *card){
    QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(card->objectName()));
    button->setObjectName(card->objectName());
    button->setToolTip(card->getDescription());

    map.insert(card->objectName(), card);
    group->addButton(button);
    return button;
}

bool ThMimengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    CardStar card = Self->tag.value("thmimeng").value<CardStar>();
	Card *originalCard = Sanguosha->getCard(getSubcards().first());
	Card *newcard = Sanguosha->cloneCard(card->objectName(), originalCard->getSuit(), originalCard->getNumber());
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, newcard);
}

bool ThMimengCard::targetFixed() const{
    if(Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
    {
        if(!ClientInstance->hasNoTargetResponding()){
            CardStar card = Sanguosha->cloneCard(user_string, NoSuitNoColor, 0);
            Self->tag["thmimeng"] = QVariant::fromValue(card);
            return card && card->targetFixed();
        }
        else return true;
    }

    CardStar card = Self->tag.value("thmimeng").value<CardStar>();
    return card && card->targetFixed();
}

bool ThMimengCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    CardStar card = Self->tag.value("thmimeng").value<CardStar>();
    return card && card->targetsFeasible(targets, Self);
}

const Card *ThMimengCard::validate(const CardUseStruct *card_use) const{
    ServerPlayer *thmimeng_general = card_use->from;

    Room *room = thmimeng_general->getRoom();
    QString to_use = user_string;

    if (user_string == "slash"
        && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        QStringList use_list;
        use_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        to_use = room->askForChoice(thmimeng_general, "thmimeng_skill_slash", use_list.join("+"));
    }

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("thmimeng");
    use_card->addSubcard(this);
    use_card->deleteLater();

    return use_card;
}

const Card *ThMimengCard::validateInResposing(ServerPlayer *user, bool &continuable) const{
    Room *room = user->getRoom();

    QString to_use;
    if(user_string == "peach+analeptic") {
        QStringList use_list;
        use_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "analeptic";
        to_use = room->askForChoice(user, "thmimeng_skill_saveself", use_list.join("+"));
    }
    else if (user_string == "slash") {
        QStringList use_list;
        use_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        to_use = room->askForChoice(user, "thmimeng_skill_slash", use_list.join("+"));
    }
    else
        to_use = user_string;

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("thmimeng");
    use_card->deleteLater();
    return use_card;
}

class ThMimeng: public ViewAsSkill{
public:
    ThMimeng():ViewAsSkill("thmimeng"){
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return player->getHandcardNum() == 1
                && !pattern.startsWith("@")
                && !pattern.startsWith(".");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.isEmpty() && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() != 1) return NULL;
        const Card *originalCard = cards.first();
        if(Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
            ThMimengCard *card = new ThMimengCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        CardStar c = Self->tag.value("thmimeng").value<CardStar>();
        if(c) {
            ThMimengCard *card = new ThMimengCard;
            card->setUserString(c->objectName());
            card->addSubcard(originalCard);
            return card;
        }else
            return NULL;
    }

    virtual QDialog *getDialog() const{
        return ThMimengDialog::getInstance("thmimeng");
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        return player->getHandcardNum() == 1;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHandcardNum() == 1;
    }
};

ThQuanshanGiveCard::ThQuanshanGiveCard(){
	will_throw = false;
}

bool ThQuanshanGiveCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.isEmpty() && to_select != Self && !to_select->hasSkill("thquanshan");
}

void ThQuanshanGiveCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	targets.first()->obtainCard(this, false);
}

class ThQuanshanGive: public ViewAsSkill{
public:
    ThQuanshanGive():ViewAsSkill("thquanshangive"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
			return NULL;

		Card *card = new ThQuanshanGiveCard;
		card->addSubcards(cards);
		return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@thquanshangive!";
    }
};

ThQuanshanCard::ThQuanshanCard(){
}

bool ThQuanshanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.isEmpty() && to_select != Self && !to_select->isKongcheng() && to_select->getHp() >= Self->getHp();
}

void ThQuanshanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	if(room->alivePlayerCount() == 2)
		return;

	ServerPlayer *target = targets.first();
	bool used = room->askForUseCard(target, "@@thquanshangive!", "@thquanshan");

	if(!used){
		QList<ServerPlayer *>beggars = room->getOtherPlayers(target);
		beggars.removeOne(source);

		qShuffle(beggars);

        ServerPlayer *beggar = beggars.at(0);

        QList<int> to_give = target->handCards().mid(0, 1);
        ThQuanshanGiveCard *quanshan_card = new ThQuanshanGiveCard;
        foreach(int card_id, to_give)
            quanshan_card->addSubcard(card_id);
        QList<ServerPlayer *> targets;
        targets << beggar;
        quanshan_card->use(room, target, targets);
        delete quanshan_card;
    }
}

class ThQuanshan:public ZeroCardViewAsSkill{
public:
    ThQuanshan():ZeroCardViewAsSkill("thquanshan"){
    }
    
    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ThQuanshanCard");
    }

    virtual const Card *viewAs() const{
        return new ThQuanshanCard;
    }
};

class ThXiangang: public TriggerSkill{
public:
     ThXiangang():TriggerSkill("thxiangang"){
		 events << DamageInflicted;
		 frequency = Frequent;
	 }

	 virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if(!player->askForSkillInvoke(objectName()))
			return false;

		JudgeStruct judge;
		judge.pattern = QRegExp("(.*):(club):(.*)");
		judge.good = true;
		judge.reason = objectName();
		judge.who = player;
		room->judge(judge);

		if(judge.isBad())
			return false;
		
		LogMessage log;
		log.type = "#thxiangang";
		log.from = player;
		log.arg = objectName();
		room->sendLog(log);
		return true;
	 }
};

ThBaochunCard::ThBaochunCard(){
	will_throw = false;
}

bool ThBaochunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.isEmpty() && to_select != Self && subcardsLength() <= to_select->getLostHp();
}

void ThBaochunCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	targets.first()->obtainCard(this, false);
	source->drawCards(subcardsLength());
}

class ThBaochunViewAsSkill: public ViewAsSkill{
public:
	ThBaochunViewAsSkill():ViewAsSkill("thbaochun"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return !to_select->isEquipped() && to_select->isRed();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty())
			return NULL;

		Card *card = new ThBaochunCard;
		card->addSubcards(cards);
		return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@thbaochun";
    }
};

class ThBaochun: public TriggerSkill{
public:
	ThBaochun():TriggerSkill("thbaochun"){
		events << EventPhaseStart;
		view_as_skill = new ThBaochunViewAsSkill;
	}

	virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
		if(player->getPhase() == Player::Draw && !player->isKongcheng() && player->askForSkillInvoke(objectName())
				&& room->askForUseCard(player, "@@thbaochun", "@thbaochun"))
			return true;

		return false;
	}
};

class ThTanchun: public TriggerSkill{
public:
	ThTanchun():TriggerSkill("thtanchun"){
		events << CardsMoveOneTime;
		frequency = Frequent;
	}

	virtual int getPriority() const{
		return 4;
	}

	virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
		CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
		if(move->from == player || move->from == NULL)
            return false;
        if(move->to_place == Player::DiscardPile
                && (move->reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD){
			int card_id;
			for(int i = 0; i < move->card_ids.length(); i++) {
				card_id = move->card_ids.at(i);
				if(Sanguosha->getCard(card_id)->getSuit() == Card::Diamond 
						&&(move->from_places.at(i) != Player::PlaceDelayedTrick && move->from_places.at(i) != Player::PlaceSpecial))
					if(player->askForSkillInvoke(objectName()))
						player->drawCards(1);
			}
		}
		return false;
	}
};

class ThGuaitan: public TriggerSkill{
public:
	ThGuaitan():TriggerSkill("thguaitan"){
		events << EventPhaseStart << Damage;
	}

	virtual bool triggerable(ServerPlayer *target) const{
		return (target != NULL);
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if (triggerEvent == Damage && TriggerSkill::triggerable(player))
		{
			DamageStruct damage = data.value<DamageStruct>();
			if (!damage.chain && !damage.transfer && player->askForSkillInvoke(objectName()))
			{
				QString choice = room->askForChoice(player, objectName(), "basic+equip+trick");
				
				QStringList guaitanlist = damage.to->tag.value("guaitan").toStringList();
				if (choice == "basic")
					choice = "BasicCard";
				else if (choice == "equip")
					choice = "EquipCard";
				else
					choice = "TrickCard";

				choice = choice + "|.|.|hand"; // Handcards only
				if (!guaitanlist.contains(choice))
					guaitanlist << choice;

				damage.to->tag["guaitan"] = QVariant::fromValue(guaitanlist);
			}
		}
		else if (triggerEvent == EventPhaseStart)
			if (player->getPhase() == Player::RoundStart && !player->tag.value("guaitan").toStringList().isEmpty())
			{
				QStringList guaitanlist = player->tag.value("guaitan").toStringList();
				foreach(QString str, guaitanlist)
					room->setPlayerCardLimitation(player, "use,response", str, true);

				player->tag["guaitan"] = QVariant::fromValue(QStringList());
			}

		return false;
	}
};

class ThYinghua: public TriggerSkill{
public:
	ThYinghua():TriggerSkill("thyinghua"){
		frequency = Frequent;
		events << DamageInflicted;
	}

	virtual bool trigger(TriggerEvent , Room *, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if(player->getPhase() != Player::NotActive || !player->askForSkillInvoke(objectName()))
			return false;

		player->gainMark("@jianren", damage.damage);
		return true;
	}
};

class ThLiaoyu: public TriggerSkill{
public:
	ThLiaoyu():TriggerSkill("thliaoyu"){
		frequency = Frequent;
		events << EventPhaseStart;
	}

	virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
		if(player->getPhase() != Player::Start || player->getMark("@jianren") <= 0 || player->isNude() || !player->askForSkillInvoke(objectName()))
			return false;

		QString choice;
		QStringList allsuit;
		allsuit << "spade" << "heart" << "club" << "diamond";
		QStringList canchoice;
		QStringList choiced;
		JudgeStruct judge;
		LogMessage log;
		forever{
			if(!room->askForDiscard(player, objectName(), 1, 1, true, true))
				break;

			if(player->getMark("@jianren") < 4) {
				choiced.clear();
				canchoice = allsuit;
				for(int i = 0; i < player->getMark("@jianren"); i++) {
					choice = room->askForChoice(player, objectName(), canchoice.join("+"));
					log.type = "#ChooseSuit";
					log.from = player;
					log.arg  = choice;
					room->sendLog(log);
					canchoice.removeOne(choice);
					choiced << choice;
				}
			}
			else
				choiced = allsuit;
			
			judge.pattern = QRegExp("(.*):(.*):(.*)");
			judge.good = true;
			judge.reason = objectName();
			judge.who = player;

			room->judge(judge);

			if(choiced.contains(judge.card->getSuitString()))
				player->loseMark("@jianren");

			if(player->isNude() || player->getMark("@jianren") <= 0)
				break;
		}

		return false;
	}
};

class ThHouzhi: public TriggerSkill{
public:
	ThHouzhi():TriggerSkill("thhouzhi"){
		frequency = Compulsory;
		events << EventPhaseEnd;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if(player->getPhase() != Player::Finish)
			return false;

		int x = player->getMark("@jianren");
		if(x <= 0)
			return false;

		LogMessage log;
		log.type = "#TriggerSkill";
		log.from = player;
		log.arg  = objectName();
		room->sendLog(log);
		room->loseHp(player, x);
		player->loseAllMarks("@jianren");

		return false;
	}
};

ThShijieCard::ThShijieCard(){
	target_fixed = true;
	will_throw = false;
}

const Card *ThShijieCard::validate(const CardUseStruct *card_use) const{
	ServerPlayer *player = card_use->from;
	Room *room = player->getRoom();

	QList<int> card_ids = player->getPile("shijiepile");
	if(card_ids.isEmpty())
		return NULL;
	room->fillAG(card_ids, NULL);
	int card_id = room->askForAG(player, card_ids, false, "thshijie");
	foreach(ServerPlayer *p, room->getPlayers())
		p->invoke("clearAG");

	const Card *card = Sanguosha->getCard(card_id);
	Card *use_card = Sanguosha->cloneCard("nullification", card->getSuit(), card->getNumber());
	use_card->addSubcard(card);
	use_card->setSkillName("thshijie");
	use_card->deleteLater();
	return use_card;
}

const Card *ThShijieCard::validateInResposing(ServerPlayer *player, bool &continuable) const{
	continuable = true;
	Room *room = player->getRoom();

	QList<int> card_ids = player->getPile("shijiepile");
	if(card_ids.isEmpty())
		return NULL;
	room->fillAG(card_ids, NULL);
	int card_id = room->askForAG(player, card_ids, true, "thshijie");
	foreach(ServerPlayer *p, room->getPlayers())
		p->invoke("clearAG");

	if(card_id == -1)
		return NULL;

	const Card *card = Sanguosha->getCard(card_id);
	Card *use_card = Sanguosha->cloneCard("nullification", card->getSuit(), card->getNumber());
	use_card->addSubcard(card);
	use_card->setSkillName("thshijie");
	use_card->deleteLater();
	return use_card;
}

class ThShijieViewAsSkill: public ZeroCardViewAsSkill{
public:
	ThShijieViewAsSkill():ZeroCardViewAsSkill("thshijie"){
	}

    virtual const Card *viewAs() const{
		return new ThShijieCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "nullification" && !player->getPile("shijiepile").isEmpty();
    }

	virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
		return !player->getPile("shijiepile").isEmpty();
	}
};

class ThShijie: public TriggerSkill{
public:
	ThShijie():TriggerSkill("thshijie"){
		events << HpRecover;
		view_as_skill = new ThShijieViewAsSkill;
	}

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if(player->hasFlag("dying")) {
			RecoverStruct recover = data.value<RecoverStruct>();
			for(int i = 0; i < recover.recover; i++)
				if(player->askForSkillInvoke(objectName()))
					player->addToPile("shijiepile", room->drawCard(), true);
		}
		return false;
	}
};

class ThShengzhi: public TriggerSkill{
public:
	ThShengzhi():TriggerSkill("thshengzhi"){
		events << EventPhaseStart;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if(player->getPhase() != Player::RoundStart)
			return false;

		ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
		if(splayer == NULL || splayer == player)
			return false;

		if(splayer->isKongcheng() && splayer->getPile("shijiepile").isEmpty())
			return false;

		if(splayer->askForSkillInvoke(objectName())){
			bool invoke = false;
			if(!player->isKongcheng() && room->askForCard(splayer, ".black", "@thshengzhi", data))
				invoke = true;

			if(!invoke && !splayer->getPile("shijiepile").isEmpty()){
				QList<int> card_ids = splayer->getPile("shijiepile");
				room->fillAG(card_ids, NULL);
				int card_id = room->askForAG(splayer, card_ids, true, objectName());
				foreach(ServerPlayer *p, room->getPlayers())
					p->invoke("clearAG");

				if(card_id != -1) {
					CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), player->objectName(), objectName(), QString());
					room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
					invoke = true;
				}
			}

			if(!invoke)
				return false;

			QStringList choices;
			choices << "start" << "judge" << "draw" << "play" << "discard" << "finish";
			QString choice = room->askForChoice(splayer, objectName(), choices.join("+"));
			if(choice == "draw")
				room->loseHp(splayer);
			else if(choice == "play") {
				RecoverStruct recover;
				recover.who = splayer;
				room->recover(player, recover);
			}

			if(choice == "start")
				player->skip(Player::Start);
			else if(choice == "judge")
				player->skip(Player::Judge);
			else if(choice == "draw")
				player->skip(Player::Draw);
			else if(choice == "play")
				player->skip(Player::Play);
			else if(choice == "discard")
				player->skip(Player::Discard);
			else if(choice == "finish")
				player->skip(Player::Finish);
		}
		return false;
	}
};

class ThZhaoyu: public TriggerSkill{
public:
	ThZhaoyu():TriggerSkill("thzhaoyu"){
		events << EventPhaseStart;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

	virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if(player->getPhase() != Player::Start)
			return false;

		ServerPlayer *splayer = room->findPlayerBySkillName(objectName());
		if(splayer == NULL || splayer->isNude() || !splayer->askForSkillInvoke(objectName()))
			return false;

		int card_id = room->askForCardChosen(splayer, splayer, "he", objectName());
		room->broadcastSkillInvoke(objectName());
		const Card *card = Sanguosha->getCard(card_id);
		room->moveCardTo(card, NULL, Player::DrawPile, false);

		return false;
	}
};

class ThWuwu: public TriggerSkill{
public:
	ThWuwu():TriggerSkill("thwuwu"){
		frequency = Compulsory;
		events << EventPhaseStart << EventPhaseEnd << CardDiscarded;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
		if(player->getPhase() != Player::Discard)
			return false;

		if(triggerEvent == EventPhaseStart) {
			player->addMark(objectName());
			return false;
		}
		else if(triggerEvent == EventPhaseEnd) {
			if(player->getMark(objectName()) == 0)
				return false;

			room->setPlayerMark(player, objectName(), 0);
			if(player->isKongcheng())
				return false;

			room->broadcastSkillInvoke(objectName());
			LogMessage log;
			log.type = "#TriggerSkill";
			log.from = player;
			log.arg  = objectName();
			room->sendLog(log);
			room->askForDiscard(player, objectName(), 1, 1, false, false);
			return false;
		}
		else if(triggerEvent == CardDiscarded) {
			room->setPlayerMark(player, objectName(), 0);
			return false;
		}

		return false;
	}
};

class ThRudao: public TriggerSkill{
public:
	ThRudao():TriggerSkill("thrudao"){
		events << EventPhaseStart;
		frequency = Wake;
	}

	virtual int getPriority() const{
		return 0;
	}

	virtual bool trigger(TriggerEvent , Room *, ServerPlayer *player, QVariant &) const{
		if(player->getMark(objectName()) > 0)
			return false;

		if(player->getPhase() != Player::Start || !player->isKongcheng())
			return false;

		Room *room = player->getRoom();
		LogMessage log;
		log.type = "#thrudao";
		log.from = player;
		log.arg  = objectName();
		room->sendLog(log);
		room->broadcastSkillInvoke(objectName());
		room->getThread()->delay();
		player->drawCards(2);
		room->loseMaxHp(player, 3);
		player->detachSkill("thwuwu");
		player->addMark(objectName());
		player->gainMark("@waked");
		return false;
	}
};

ThLiuzhenCard::ThLiuzhenCard(){
}

bool ThLiuzhenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return to_select != Self && !to_select->hasFlag("liuzhenold") && Self->inMyAttackRange(to_select);
}

void ThLiuzhenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
	foreach(ServerPlayer *p, targets)
		room->setPlayerFlag(p, "liuzhennew");
}

class ThLiuzhenViewAsSkill:public ZeroCardViewAsSkill{
public:
    ThLiuzhenViewAsSkill():ZeroCardViewAsSkill("thliuzhen"){
    }
    
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

	virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@thliuzhen";
    }

    virtual const Card *viewAs() const{
        return new ThLiuzhenCard;
    }
};

/*
class ThLiuzhen: public TriggerSkill{
public:
	ThLiuzhen():TriggerSkill("thliuzhen"){
		events << TargetConfirmed << PostCardEffected << SlashMissed;
		view_as_skill = new ThLiuzhenViewAsSkill;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
		if(triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play) {
			CardUseStruct use = data.value<CardUseStruct>();
			
			if(use.card->isKindOf("Slash") && !player->hasUsed("ThLiuzhenCard")) {
				foreach(ServerPlayer *tar, use.to)
					room->setPlayerFlag(tar, "liuzhenold");

				if(room->askForUseCard(player, "@@thliuzhen", "@thliuzhen"))
					room->setPlayerFlag(player, "liuzhenuse");

				foreach(ServerPlayer *tar, use.to)
					room->setPlayerFlag(tar, "-liuzhenold");
			}
		}
		else if(triggerEvent == SlashMissed && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play) {
			SlashEffectStruct effect = data.value<SlashEffectStruct>();
			if(effect.to->hasFlag("liuzhennew"))
			local log= sgs.LogMessage()
				log.type = "#thliuzhentr"
				log.from = player
				log.to:append(effect.to)
				log.arg  = "thliuzhen"
			room:sendLog(log)
			if player:getCardCount(true)>1 then
				if not room:askForDiscard(player,"thliuzhen",2,2,true,true) then
					room:loseHp(player)
				end
			else
				room:loseHp(player)
			end
		end
*/



void TouhouPackage::addHanaGenerals(){
    General *hana003 = new General(this, "hana003", "wei", 3, false);
    hana003->addSkill(new ThBian);
    hana003->addSkill(new ThGuihang);
    hana003->addSkill(new ThWujian);

    General *hana006 = new General(this, "hana006", "wei");
    hana006->addSkill(new ThXihua);

    General *hana007 = new General(this, "hana007", "wei", 3, false);
	hana007->addSkill(new ThAnyun);
	hana007->addSkill(new ThMimeng);

	General *hana008 = new General(this, "hana008", "wei", 3);
	hana008->addSkill(new ThQuanshan);
	hana008->addSkill(new ThXiangang);

	General *hana011 = new General(this, "hana011", "wei", 3);
	hana011->addSkill(new ThBaochun);
	hana011->addSkill(new ThTanchun);

	General *hana012 = new General(this, "hana012", "wei");
	hana012->addSkill(new ThGuaitan);
	hana012->addSkill("xiagong");

	General *hana013 = new General(this, "hana013", "wei");
	hana013->addSkill(new ThYinghua);
	hana013->addSkill(new ThLiaoyu);
	hana013->addSkill(new ThHouzhi);
	
	General *hana016 = new General(this, "hana016", "wei", 3);
	hana016->addSkill(new ThShijie);
	hana016->addSkill(new ThShengzhi);
	
	General *hana017 = new General(this, "hana017", "wei", 7);
	hana017->addSkill(new ThZhaoyu);
	hana017->addSkill(new ThWuwu);
	hana017->addSkill(new ThRudao);
	
	//General *hana018 = new General(this, "hana018$", "wei");
	//hana018->addSkill(new ThLiuzhen);

    addMetaObject<ThWujianCard>();
    addMetaObject<ThXihuaCard>();
    addMetaObject<ThMimengCard>();
    addMetaObject<ThQuanshanGiveCard>();
    addMetaObject<ThQuanshanCard>();
    addMetaObject<ThBaochunCard>();
    addMetaObject<ThShijieCard>();
    //addMetaObject<ThLiuzhenCard>();
	
    skills << new ThWujianDistanceSkill << new ThQuanshanGive;
}