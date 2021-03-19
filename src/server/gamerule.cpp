#include "gamerule.h"
#include "engine.h"
#include "maneuvering.h"
#include "room.h"
#include "scenario.h"
#include "serverplayer.h"
#include "settings.h"
#include "standard.h"

#include <QTime>

GameRule::GameRule(QObject *)
    : TriggerSkill("game_rule")
{
    //@todo: this setParent is illegitimate in QT and is equivalent to calling
    // setParent(NULL). So taking it off at the moment until we figure out
    // a way to do it.
    //setParent(parent);

    events << GameStart << TurnStart << EventPhaseProceeding << EventPhaseEnd << EventPhaseChanging << PreCardUsed << CardUsed
           << CardFinished << CardEffected << HpChanged << EventLoseSkill << EventAcquireSkill << AskForPeaches
           << AskForPeachesDone << BuryVictim << GameOverJudge << SlashHit << SlashEffected << SlashProceed << ConfirmDamage
           << DamageDone << DamageComplete << StartJudge << FinishRetrial << FinishJudge << ChoiceMade;
}

QStringList GameRule::triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *&ask_who) const
{
    ask_who = NULL;
    return QStringList(objectName());
}

int GameRule::getPriority(TriggerEvent) const
{
    return 0;
}

void GameRule::onPhaseProceed(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    switch (player->getPhase()) {
    case Player::PhaseNone: {
        Q_ASSERT(false);
    }
    case Player::RoundStart: {
        break;
    }
    case Player::Start: {
        break;
    }
    case Player::Judge: {
        while (!player->getJudgingArea().isEmpty() && player->isAlive()) {
            const Card *trick = player->getJudgingArea().takeLast();
            bool on_effect = room->cardEffect(trick, NULL, player);
            if (!on_effect)
                trick->onNullified(player);
        }
        break;
    }
    case Player::Draw: {
        int num = 2;
        if (player->hasFlag("Global_FirstRound")) {
            room->setPlayerFlag(player, "-Global_FirstRound");
            if (room->getMode() == "02_1v1")
                num--;
        }

        QVariant data = num;
        room->getThread()->trigger(DrawNCards, room, player, data);
        int n = data.toInt();
        if (n > 0)
            player->drawCards(n, "draw_phase");
        data = n;
        room->getThread()->trigger(AfterDrawNCards, room, player, data);
        break;
    }
    case Player::Play: {
        while (player->isAlive()) {
            // for JnXianmao
            if (!player->hasFlag("JnXianmaoUsed")) {
                CardUseStruct card_use;
                room->activate(player, card_use);
                if (card_use.card != NULL)
                    room->useCard(card_use, true);
                else
                    break;
            } else {
                Sanguosha->currentRoomState()->setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_PLAY); // for slash
                QString pattern = "^Jink+^Nullification";
                if (!Slash::IsAvailable(player))
                    pattern.append("+^Slash");
                if (!Analeptic::IsAvailable(player))
                    pattern.append("+^Analeptic");
                pattern.append("|.|.|hand");
                if (!room->askForUseCard(player, pattern, "@jnxianmao"))
                    break;
            }
        }
        break;
    }
    case Player::Discard: {
        int skip_num = 0;
        QList<int> skip_ids = StringList2IntList(player->property("ignored_hands").toString().split("+"));
        foreach (int id, player->handCards()) {
            if (skip_ids.contains(id))
                ++skip_num;
        }

        int discard_num = player->getHandcardNum() - skip_num - player->getMaxCards();
        if (discard_num > 0)
            room->askForDiscard(player, "gamerule", discard_num, discard_num);
        break;
    }
    case Player::Finish: {
        break;
    }
    case Player::NotActive: {
        break;
    }
    }
}

bool GameRule::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
{
    if (room->getTag("SkipGameRule").toBool()) {
        room->removeTag("SkipGameRule");
        return false;
    }

    // Handle global events
    if (player == NULL) {
        if (triggerEvent == GameStart) {
            foreach (ServerPlayer *player, room->getPlayers()) {
                if (player->getGeneral()->getKingdom() == "kami" && player->getGeneralName() != "anjiang")
                    room->setPlayerProperty(player, "kingdom", room->askForKingdom(player));
                foreach (const Skill *skill, player->getVisibleSkillList(false, true)) {
                    if (skill->getFrequency(player) == Skill::Limited && !skill->getLimitMark().isEmpty()
                        && (!skill->isLordSkill() || player->hasLordSkill(skill->objectName())))
                        room->addPlayerMark(player, skill->getLimitMark());
                }
            }
            room->setTag("FirstRound", true);
            bool kof_mode = room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical";
            int scroll_id = -1;
            if (isNormalGameMode(room->getMode())) {
                if (room->getLord()) {
                    scroll_id = room->getCardFromPile("scroll");
                    if (scroll_id != -1)
                        room->moveCardTo(Sanguosha->getCard(scroll_id), room->getLord(), Player::PlaceTable, true);
                }
            }
            QList<int> n_list;
            foreach (ServerPlayer *p, room->getPlayers()) {
                int n = kof_mode ? p->getMaxHp() : 4;
                QVariant data = n;
                room->getThread()->trigger(DrawInitialCards, room, p, data);
                n_list << data.toInt();
            }
            room->drawCards(room->getPlayers(), n_list, QString());
            if (Config.EnableLuckCard)
                room->askForLuckCard();
            int i = 0;
            foreach (ServerPlayer *p, room->getPlayers()) {
                QVariant num_data = n_list.at(i);
                room->getThread()->trigger(AfterDrawInitialCards, room, p, num_data);
                i++;
            }
            if (scroll_id != -1)
                room->moveCardTo(Sanguosha->getCard(scroll_id), room->getLord(), Player::PlaceHand, true);
        }
        return false;
    }

    switch (triggerEvent) {
    case TurnStart: {
        player = room->getCurrent();
        if (room->getTag("FirstRound").toBool()) {
            room->setTag("FirstRound", false);
            room->setPlayerFlag(player, "Global_FirstRound");
        }

        LogMessage log;
        log.type = "$AppendSeparator";
        room->sendLog(log);
        room->addPlayerMark(player, "Global_TurnCount");

        if (!player->faceUp()) {
            room->setPlayerFlag(player, "-Global_FirstRound");
            player->turnOver();
        } else if (player->isAlive())
            player->play();

        break;
    }
    case EventPhaseProceeding: {
        onPhaseProceed(player);
        break;
    }
    case EventPhaseEnd: {
        if (player->getPhase() == Player::Play)
            room->addPlayerHistory(player, ".");
        break;
    }
    case EventPhaseChanging: {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark("drank") > 0) {
                    LogMessage log;
                    log.type = "#UnsetDrankEndOfTurn";
                    log.from = player;
                    log.to << p;
                    room->sendLog(log);

                    room->setPlayerMark(p, "drank", 0);
                }
            }
            room->setPlayerFlag(player, ".");
            room->clearPlayerCardLimitation(player, true);
        } else if (change.to == Player::Play) {
            room->addPlayerHistory(player, ".");
        }
        break;
    }
    case PreCardUsed: {
        if (data.canConvert<CardUseStruct>()) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            if (card_use.from->hasFlag("Global_ForbidSurrender")) {
                card_use.from->setFlags("-Global_ForbidSurrender");
                room->doNotify(card_use.from, QSanProtocol::S_COMMAND_ENABLE_SURRENDER, QVariant(true));
            }

            if (card_use.card->isKindOf("Peach")) {
                if (card_use.to.isEmpty() || card_use.to.contains(card_use.from))
                    room->setCardFlag(card_use.card, "PeachSelf");
            }
            card_use.from->broadcastSkillInvoke(card_use.card);
            if (!card_use.card->getSkillName().isNull()
                && card_use.card->getSkillName(true) == card_use.card->getSkillName(false) && card_use.m_isOwnerUse
                && card_use.from->hasSkill(card_use.card->getSkillName()))
                room->notifySkillInvoked(card_use.from, card_use.card->getSkillName());
        }
        break;
    }
    case CardUsed: {
        if (data.canConvert<CardUseStruct>()) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            RoomThread *thread = room->getThread();

            if (card_use.card->hasPreAction())
                card_use.card->doPreAction(room, card_use);

            if (card_use.from && !card_use.to.isEmpty()) {
                thread->trigger(TargetSpecifying, room, card_use.from, data);
                CardUseStruct card_use = data.value<CardUseStruct>();
                QList<ServerPlayer *> targets = card_use.to;
                foreach (ServerPlayer *to, card_use.to) {
                    if (targets.contains(to)) {
                        thread->trigger(TargetConfirming, room, to, data);
                        CardUseStruct new_use = data.value<CardUseStruct>();
                        targets = new_use.to;
                    }
                }
            }
            card_use = data.value<CardUseStruct>();

            try {
                QVariantList jink_list_backup;
                if (card_use.card->isKindOf("Slash")) {
                    jink_list_backup = card_use.from->tag["Jink_" + card_use.card->toString()].toList();
                    QVariantList jink_list;
                    for (int i = 0; i < card_use.to.length(); i++)
                        jink_list.append(QVariant(1));
                    card_use.from->tag["Jink_" + card_use.card->toString()] = QVariant::fromValue(jink_list);
                }
                if (card_use.from && !card_use.to.isEmpty()) {
                    thread->trigger(TargetSpecified, room, card_use.from, data);
                    foreach (ServerPlayer *p, room->getAllPlayers())
                        thread->trigger(TargetConfirmed, room, p, data);
                }
                card_use = data.value<CardUseStruct>();
                room->setTag("CardUseNullifiedList", QVariant::fromValue(card_use.nullified_list));
                card_use.card->use(room, card_use.from, card_use.to);
                if (!jink_list_backup.isEmpty())
                    card_use.from->tag["Jink_" + card_use.card->toString()] = QVariant::fromValue(jink_list_backup);
            } catch (TriggerEvent triggerEvent) {
                if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                    card_use.from->tag.remove("Jink_" + card_use.card->toString());
                throw triggerEvent;
            }
        }

        break;
    }
    case CardFinished: {
        CardUseStruct use = data.value<CardUseStruct>();
        room->clearCardFlag(use.card);

        if (use.card->isKindOf("AOE") || use.card->isKindOf("GlobalEffect")) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->doNotify(p, QSanProtocol::S_COMMAND_NULLIFICATION_ASKED, QVariant("."));
        }
        if (use.card->isKindOf("Slash"))
            use.from->tag.remove("Jink_" + use.card->toString());

        break;
    }
    case EventAcquireSkill:
    case EventLoseSkill: {
        QString skill_name = data.toString();
        const Skill *skill = Sanguosha->getSkill(skill_name);
        bool refilter = skill->inherits("FilterSkill");

        if (!refilter && skill->inherits("TriggerSkill")) {
            const TriggerSkill *trigger = qobject_cast<const TriggerSkill *>(skill);
            const ViewAsSkill *vsskill = trigger->getViewAsSkill();
            if (vsskill && vsskill->inherits("FilterSkill"))
                refilter = true;
        }

        if (refilter)
            room->filterCards(player, player->getHandcards(), triggerEvent == EventLoseSkill);

        break;
    }
    case HpChanged: {
        if (player->getHp() > 0)
            break;
        if (data.isNull() || data.canConvert<RecoverStruct>())
            break;
        if (data.canConvert<DamageStruct>()) {
            DamageStruct damage = data.value<DamageStruct>();
            room->enterDying(player, &damage);
        } else {
            room->enterDying(player, NULL);
        }

        break;
    }
    case AskForPeaches: {
        DyingStruct dying = data.value<DyingStruct>();
        const Card *peach = NULL;

        while (dying.who->getHp() <= 0) {
            peach = NULL;

            // coupling IkSishideng here to deal with complicated rule problems
            if (room->isSomeonesTurn() && room->getCurrent()->hasSkill("iksishideng") && player != room->getCurrent()
                && player != dying.who) {
                player->setFlags("iksishideng");
                room->addPlayerMark(player, "Global_PreventPeach");
            }

            if (dying.who->isAlive())
                peach = room->askForSinglePeach(player, dying.who);

            if (player->hasFlag("iksishideng") && player->getMark("Global_PreventPeach") > 0) {
                player->setFlags("-iksishideng");
                room->removePlayerMark(player, "Global_PreventPeach");
            }

            if (peach == NULL)
                break;
            if (peach->isKindOf("ThChouceCard"))
                continue;
            room->useCard(CardUseStruct(peach, player, dying.who));
        }
        break;
    }
    case AskForPeachesDone: {
        if (player->getHp() <= 0 && player->isAlive()) {
            DyingStruct dying = data.value<DyingStruct>();
            room->killPlayer(player, dying.damage);
        }

        break;
    }
    case ConfirmDamage: {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.to->getMark("SlashIsDrank") > 0) {
            LogMessage log;
            log.type = "#AnalepticBuff";
            log.from = damage.from;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);

            damage.damage += damage.to->getMark("SlashIsDrank");
            damage.to->setMark("SlashIsDrank", 0);

            log.arg2 = QString::number(damage.damage);

            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        break;
    }
    case DamageDone: {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && !damage.from->isAlive())
            damage.from = NULL;

        // Chain ==========
        if (damage.nature != DamageStruct::Normal && player->isChained() && !damage.chain)
            damage.invoke_skills << "chain";
        // ================

        data = QVariant::fromValue(damage);

        LogMessage log;

        if (damage.from) {
            log.type = "#Damage";
            log.from = damage.from;
        } else {
            log.type = "#DamageNoSource";
        }

        log.to << damage.to;
        log.arg = QString::number(damage.damage);

        switch (damage.nature) {
        case DamageStruct::Normal:
            log.arg2 = "normal_nature";
            break;
        case DamageStruct::Fire:
            log.arg2 = "fire_nature";
            break;
        case DamageStruct::Thunder:
            log.arg2 = "thunder_nature";
            break;
        }

        room->sendLog(log);

        int new_hp = damage.to->getHp() - damage.damage;

        QString change_str = QString("%1:%2").arg(damage.to->objectName()).arg(-damage.damage);
        switch (damage.nature) {
        case DamageStruct::Fire:
            change_str.append("F");
            break;
        case DamageStruct::Thunder:
            change_str.append("T");
            break;
        default:
            break;
        }

        JsonArray args;
        args << damage.to->objectName();
        args << -damage.damage;
        args << int(damage.nature);
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_CHANGE_HP, args);

        room->setTag("HpChangedData", data);
        room->setPlayerProperty(damage.to, "hp", new_hp);

        break;
    }
    case DamageComplete: {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.prevented)
            break;
        if (damage.nature != DamageStruct::Normal && player->isChained()
            && (damage.chain || damage.invoke_skills.contains("chain")))
            room->setPlayerProperty(player, "chained", false);
        if (damage.invoke_skills.contains("chain")) {
            if (damage.nature != DamageStruct::Normal && !damage.chain) {
                QList<ServerPlayer *> chained_players;
                if (room->getCurrent()->isDead())
                    chained_players = room->getOtherPlayers(room->getCurrent());
                else
                    chained_players = room->getAllPlayers();
                foreach (ServerPlayer *chained_player, chained_players) {
                    if (chained_player->isChained()) {
                        room->getThread()->delay();
                        LogMessage log;
                        log.type = "#IronChainDamage";
                        log.from = chained_player;
                        room->sendLog(log);

                        DamageStruct chain_damage = damage;
                        chain_damage.to = chained_player;
                        chain_damage.chain = true;
                        chain_damage.transfer_reason = QString(); // temp way for tianxiang(ikzhihui)

                        room->damage(chain_damage);
                    }
                }
            }
        }
        if (room->getMode() == "02_1v1" || room->getMode() == "06_XMode") {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag("Global_DebutFlag")) {
                    p->setFlags("-Global_DebutFlag");
                    if (room->getMode() == "02_1v1") {
                        room->getThread()->trigger(Debut, room, p);
                        room->getThread()->trigger(GameStart, room, p);
                    }
                }
            }
        }
        break;
    }
    case CardEffected: {
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.card->isKindOf("Slash") && effect.nullified) {
                LogMessage log;
                log.type = "#CardNullified";
                log.from = effect.to;
                log.arg = effect.card->objectName();
                room->sendLog(log);

                return true;
            } else if (effect.card->getTypeId() == Card::TypeTrick) {
                if (room->isCanceled(effect)) {
                    effect.to->setFlags("Global_NonSkillNullify");
                    if (effect.from && effect.from->isAlive())
                        room->getThread()->trigger(TrickMissed, room, effect.from, data);
                    return true;
                } else {
                    room->getThread()->trigger(TrickEffect, room, effect.to, data);
                }
            }
            if (effect.to->isAlive() || effect.card->isKindOf("Slash"))
                effect.card->onEffect(effect);
        }

        break;
    }
    case SlashEffected: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.nullified) {
            LogMessage log;
            log.type = "#CardNullified";
            log.from = effect.to;
            log.arg = effect.slash->objectName();
            room->sendLog(log);

            return true;
        }
        if (effect.jink_num > 0)
            room->getThread()->trigger(SlashProceed, room, effect.from, data);
        else
            room->slashResult(effect, NULL);
        break;
    }
    case SlashProceed: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        QString slasher = effect.from->objectName();
        if (!effect.to->isAlive())
            break;
        if (effect.jink_num == 1) {
            const Card *jink = room->askForCard(effect.to, "jink", "slash-jink:" + slasher, data, Card::MethodUse, effect.from);
            room->slashResult(effect, room->isJinkEffected(effect.to, jink) ? jink : NULL);
        } else {
            DummyCard *jink = new DummyCard;
            const Card *asked_jink = NULL;
            for (int i = effect.jink_num; i > 0; i--) {
                QString prompt
                    = QString("@multi-jink%1:%2::%3").arg(i == effect.jink_num ? "-start" : QString()).arg(slasher).arg(i);
                asked_jink = room->askForCard(effect.to, "jink", prompt, data, Card::MethodUse, effect.from);
                if (!room->isJinkEffected(effect.to, asked_jink)) {
                    delete jink;
                    room->slashResult(effect, NULL);
                    return false;
                } else {
                    jink->addSubcard(asked_jink->getEffectiveId());
                }
            }
            room->slashResult(effect, jink);
        }

        break;
    }
    case SlashHit: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if (effect.drank > 0)
            effect.to->setMark("SlashIsDrank", effect.drank);
        DamageStruct::Nature nature = effect.nature;
        if (effect.slash->hasFlag("rodusing"))
            nature = DamageStruct::Fire;
        room->damage(DamageStruct(effect.slash, effect.from, effect.to, 1, nature));

        break;
    }
    case GameOverJudge: {
        if (room->getMode() == "02_1v1") {
            QStringList list = player->tag["1v1Arrange"].toStringList();
            QString rule = Config.value("1v1/Rule", "2013").toString();
            if (list.length() > ((rule == "2013") ? 3 : 0))
                break;
        }

        QString winner = room->getWinner(player);
        if (!winner.isNull()) {
            room->gameOver(winner);
            return true;
        }

        break;
    }
    case BuryVictim: {
        DeathStruct death = data.value<DeathStruct>();
        player->bury();

        if (room->getTag("SkipNormalDeathProcess").toBool())
            return false;

        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (killer && !killer->hasSkill("iktianzuoyounai"))
            rewardAndPunish(killer, player);

        if (room->getMode() == "02_1v1") {
            QStringList list = player->tag["1v1Arrange"].toStringList();
            QString rule = Config.value("1v1/Rule", "2013").toString();
            if (list.length() <= ((rule == "2013") ? 3 : 0))
                break;

            if (rule == "Classical") {
                player->tag["1v1ChangeGeneral"] = list.takeFirst();
                player->tag["1v1Arrange"] = list;
            } else {
                player->tag["1v1ChangeGeneral"] = list.first();
            }

            changeGeneral1v1(player);
            if (death.damage == NULL) {
                room->getThread()->trigger(Debut, room, player);
                room->getThread()->trigger(GameStart, room, player);
            } else
                player->setFlags("Global_DebutFlag");
            return false;
        } else if (room->getMode() == "06_XMode") {
            changeGeneralXMode(player);
            if (death.damage != NULL)
                player->setFlags("Global_DebutFlag");
            return false;
        }

        break;
    }
    case StartJudge: {
        int card_id = room->drawCard();

        JudgeStruct *judge = data.value<JudgeStruct *>();
        judge->card = Sanguosha->getCard(card_id);

        LogMessage log;
        log.type = "$InitialJudge";
        log.from = player;
        log.card_str = QString::number(judge->card->getEffectiveId());
        room->sendLog(log);

        room->moveCardTo(
            judge->card, NULL, judge->who, Player::PlaceJudge,
            CardMoveReason(CardMoveReason::S_REASON_JUDGE, judge->who->objectName(), QString(), QString(), judge->reason),
            true);
        judge->updateResult();
        break;
    }
    case FinishRetrial: {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        LogMessage log;
        log.type = "$JudgeResult";
        log.from = player;
        log.card_str = QString::number(judge->card->getEffectiveId());
        room->sendLog(log);

        int delay = Config.AIDelay;
        if (judge->time_consuming)
            delay /= 1.25;
        room->getThread()->delay(delay);
        if (judge->play_animation) {
            room->sendJudgeResult(judge);
            room->getThread()->delay(Config.S_JUDGE_LONG_DELAY);
        }

        break;
    }
    case FinishJudge: {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
            CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, judge->who->objectName(), judge->reason, QString());
            if (judge->retrial_by_response)
                reason.m_extraData = QVariant::fromValue(judge->retrial_by_response);
            room->moveCardTo(judge->card, judge->who, NULL, Player::DiscardPile, reason, true);
        }

        break;
    }
    case ChoiceMade: {
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            foreach (QString flag, p->getFlagList()) {
                if (flag.startsWith("Global_") && flag.endsWith("Failed"))
                    room->setPlayerFlag(p, "-" + flag);
            }
        }
        break;
    }
    default:
        break;
    }

    return false;
}

void GameRule::changeGeneral1v1(ServerPlayer *player) const
{
    Config.AIDelay = Config.OriginAIDelay;

    Room *room = player->getRoom();
    bool classical = (Config.value("1v1/Rule", "2013").toString() == "Classical");
    QString new_general;
    if (classical) {
        new_general = player->tag["1v1ChangeGeneral"].toString();
        player->tag.remove("1v1ChangeGeneral");
    } else {
        QStringList list = player->tag["1v1Arrange"].toStringList();
        if (player->getAI())
            new_general = list.first();
        else
            new_general = room->askForGeneral(player, list);
        list.removeOne(new_general);
        player->tag["1v1Arrange"] = QVariant::fromValue(list);
    }

    if (player->getPhase() != Player::NotActive)
        player->changePhase(player->getPhase(), Player::NotActive);

    room->revivePlayer(player);
    room->changeHero(player, new_general, true, true);
    if (player->getGeneral()->getKingdom() == "kami")
        room->setPlayerProperty(player, "kingdom", room->askForKingdom(player));
    else if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
    room->addPlayerHistory(player, ".");

    QList<ServerPlayer *> notified = classical ? room->getOtherPlayers(player, true) : room->getPlayers();
    room->doBroadcastNotify(notified, QSanProtocol::S_COMMAND_REVEAL_GENERAL,
                            JsonArray() << player->objectName() << new_general);

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag("FirstRound", true); //For Manjuan
    int draw_num = classical ? 4 : player->getMaxHp();
    QVariant data = draw_num;
    room->getThread()->trigger(DrawInitialCards, room, player, data);
    draw_num = data.toInt();
    try {
        player->drawCards(draw_num);
        room->setTag("FirstRound", false);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            room->setTag("FirstRound", false);
        throw triggerEvent;
    }
    data = draw_num;
    room->getThread()->trigger(AfterDrawInitialCards, room, player, data);
}

void GameRule::changeGeneralXMode(ServerPlayer *player) const
{
    Config.AIDelay = Config.OriginAIDelay;

    Room *room = player->getRoom();
    ServerPlayer *leader = player->tag["XModeLeader"].value<ServerPlayer *>();
    Q_ASSERT(leader);
    QStringList backup = leader->tag["XModeBackup"].toStringList();
    QString general = room->askForGeneral(leader, backup);
    if (backup.contains(general))
        backup.removeOne(general);
    else
        backup.takeFirst();
    leader->tag["XModeBackup"] = QVariant::fromValue(backup);

    if (player->getPhase() != Player::NotActive)
        player->changePhase(player->getPhase(), Player::NotActive);

    room->revivePlayer(player);
    room->changeHero(player, general, true, true);
    if (player->getGeneral()->getKingdom() == "kami")
        room->setPlayerProperty(player, "kingdom", room->askForKingdom(player));
    room->addPlayerHistory(player, ".");

    if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag("FirstRound", true); //For Manjuan
    QVariant data(4);
    room->getThread()->trigger(DrawInitialCards, room, player, data);
    int num = data.toInt();
    try {
        player->drawCards(num);
        room->setTag("FirstRound", false);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            room->setTag("FirstRound", false);
        throw triggerEvent;
    }
    data = num;
    room->getThread()->trigger(AfterDrawInitialCards, room, player, data);
}

void GameRule::rewardAndPunish(ServerPlayer *killer, ServerPlayer *victim) const
{
    Room *room = killer->getRoom();
    if (killer->isDead() || room->getMode() == "06_XMode")
        return;

    if (room->getMode() == "02_1v1") {
        killer->drawCards(1, "kill");
    } else if (room->getMode() == "06_3v3") {
        if (Config.value("3v3/OfficialRule", "2013").toString().startsWith("201"))
            killer->drawCards(2, "kill");
        else
            killer->drawCards(3, "kill");
    } else {
        bool is_chunxue = room->getScenario() && room->getScenario()->objectName() == "chunxue";
        if (victim->getRole() == "rebel" && killer != victim) {
            if (is_chunxue && killer->getRole() == "rebel")
                killer->throwAllHandCardsAndEquips();
            else
                killer->drawCards(3, "kill");
        } else if (victim->getRole() == "loyalist" && killer->getRole() == "lord")
            killer->throwAllHandCardsAndEquips();
    }
}

HulaoPassMode::HulaoPassMode(QObject *parent)
    : GameRule(parent)
{
    setObjectName("hulaopass_mode");
    events << HpChanged << StageChange;
}

bool HulaoPassMode::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
{
    switch (triggerEvent) {
    case StageChange: {
        ServerPlayer *lord = room->getLord();
        room->setPlayerMark(lord, "secondMode", 1);
        room->changeHero(lord, "story003", false, true, false, false);

        LogMessage log;
        log.type = "$AppendSeparator";
        room->sendLog(log);

        log.type = "#HulaoTransfigure";
        log.arg = "#story002";
        log.arg2 = "#story003";
        room->sendLog(log);

        //room->doLightbox("$StageChange", 5000);

        QList<const Card *> tricks = lord->getJudgingArea();
        if (!tricks.isEmpty()) {
            DummyCard *dummy = new DummyCard;
            foreach (const Card *trick, tricks)
                dummy->addSubcard(trick);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
        }
        if (lord->getHp() < qMin(lord->getMaxHp(), 4))
            room->recover(lord, RecoverStruct(NULL, NULL, 4 - lord->getHp()));
        if (!lord->faceUp())
            lord->turnOver();
        if (lord->isChained())
            room->setPlayerProperty(lord, "chained", false);
        break;
    }
    case GameStart: {
        // Handle global events
        if (player == NULL) {
            QList<int> n_list;
            foreach (ServerPlayer *p, room->getPlayers()) {
                int n = 0;
                if (p->isLord())
                    n = 8;
                else
                    n = p->getSeat() + 1;
                QVariant data = n;
                if (p->getGeneral()->getKingdom() == "kami")
                    room->setPlayerProperty(p, "kingdom", room->askForKingdom(p));
                room->getThread()->trigger(DrawInitialCards, room, p, data);
                n_list << data.toInt();
            }
            room->drawCards(room->getPlayers(), n_list);
            if (Config.EnableLuckCard)
                room->askForLuckCard();
            int i = 0;
            foreach (ServerPlayer *p, room->getPlayers()) {
                QVariant num_data = n_list.at(i);
                room->getThread()->trigger(AfterDrawInitialCards, room, p, num_data);
                ++i;
            }
            return false;
        }
        break;
    }
    case HpChanged: {
        if (player->isLord() && player->getHp() <= 4 && player->getMark("secondMode") == 0)
            throw StageChange;
        break;
    }
    case GameOverJudge: {
        if (player->isLord())
            room->gameOver("rebel");
        else if (room->aliveRoles(player).length() == 1)
            room->gameOver("lord");

        return false;
    }
    case BuryVictim: {
        if (player->hasFlag("actioned"))
            room->setPlayerFlag(player, "-actioned");

        LogMessage log;
        log.type = "#Reforming";
        log.from = player;
        room->sendLog(log);

        player->bury();
        room->setPlayerProperty(player, "hp", 0);

        foreach (ServerPlayer *p, room->getOtherPlayers(room->getLord())) {
            if (p->isAlive() && p->askForSkillInvoke("draw_1v3"))
                p->drawCards(1, "draw_1v3");
        }

        return false;
    }
    case TurnStart: {
        if (player->isDead()) {
            JsonArray args;
            args << (int)QSanProtocol::S_GAME_EVENT_PLAYER_REFORM;
            args << player->objectName();
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            if (!player->isWounded()) {
                LogMessage log;
                log.type = "#ReformingDraw";
                log.from = player;
                log.arg = "1";
                room->sendLog(log);
                player->drawCards(1, "reform");
            } else {
                LogMessage log;
                log.type = "#ReformingRecover";
                log.from = player;
                log.arg = "1";
                room->sendLog(log);
                room->setPlayerProperty(player, "hp", player->getHp() + 1);
            }

            if (player->getHp() + player->getHandcardNum() == 4) {
                LogMessage log;
                log.type = "#ReformingRevive";
                log.from = player;
                room->sendLog(log);

                room->revivePlayer(player);

                int n = 3;
                if (player->getMaxHp() >= 4)
                    n = 2;
                player->drawCards(n, "revive");
            }
        } else {
            LogMessage log;
            log.type = "$AppendSeparator";
            room->sendLog(log);
            room->addPlayerMark(player, "Global_TurnCount");

            if (!player->faceUp())
                player->turnOver();
            else
                player->play();
        }

        return false;
    }
    default:
        break;
    }

    return GameRule::trigger(triggerEvent, room, player, data);
}

BasaraMode::BasaraMode(QObject *parent)
    : GameRule(parent)
{
    setObjectName("basara_mode");
    events << EventPhaseStart << DamageInflicted << BeforeGameOverJudge;
}

QString BasaraMode::getMappedRole(const QString &role)
{
    static QMap<QString, QString> roles;
    if (roles.isEmpty()) {
        roles["wei"] = "lord";
        roles["shu"] = "loyalist";
        roles["wu"] = "rebel";
        roles["qun"] = "renegade";
    }
    return roles[role];
}

int BasaraMode::getPriority(TriggerEvent) const
{
    return 15;
}

void BasaraMode::playerShowed(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    QString name = player->property("basara_generals").toString();
    if (name.isEmpty())
        return;
    QStringList names = name.split("+");

    if (Config.EnableHegemony) {
        QMap<QString, int> kingdom_roles;
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            kingdom_roles[p->getKingdom()]++;

        if (kingdom_roles[Sanguosha->getGeneral(names.first())->getKingdom()] >= Config.value("HegemonyMaxShown", 2).toInt()
            && player->getGeneralName() == "anjiang")
            return;
    }

    QString answer = room->askForChoice(player, "RevealGeneral", "yes+no");
    if (answer == "yes") {
        QString general_name = room->askForGeneral(player, names);

        generalShowed(player, general_name);
        if (Config.EnableHegemony)
            room->getThread()->trigger(GameOverJudge, room, player);
        playerShowed(player);
    }
}

void BasaraMode::generalShowed(ServerPlayer *player, QString general_name) const
{
    Room *room = player->getRoom();
    QString name = player->property("basara_generals").toString();
    if (name.isEmpty())
        return;
    QStringList names = name.split("+");

    if (player->getGeneralName() == "anjiang") {
        room->changeHero(player, general_name, false, false, false, false);
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

        if (player->getGeneral()->getKingdom() == "kami")
            room->setPlayerProperty(player, "kingdom", room->askForKingdom(player));

        if (Config.EnableHegemony)
            room->setPlayerProperty(player, "role", getMappedRole(player->getKingdom()));
    } else {
        room->changeHero(player, general_name, false, false, true, false);
    }

    names.removeOne(general_name);
    player->setProperty("basara_generals", names.join("+"));
    room->notifyProperty(player, player, "basara_generals");

    LogMessage log;
    log.type = "#BasaraReveal";
    log.from = player;
    log.arg = player->getGeneralName();
    if (player->getGeneral2()) {
        log.type = "#BasaraRevealDual";
        log.arg2 = player->getGeneral2Name();
    }
    room->sendLog(log);
}

bool BasaraMode::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
{
    // Handle global events
    if (player == NULL) {
        if (triggerEvent == GameStart) {
            if (Config.EnableHegemony)
                room->setTag("SkipNormalDeathProcess", true);
            foreach (ServerPlayer *sp, room->getAlivePlayers()) {
                room->setPlayerProperty(sp, "general", "anjiang");
                sp->setGender(General::Sexless);
                room->setPlayerProperty(sp, "kingdom", "god");

                LogMessage log;
                log.type = "#BasaraGeneralChosen";
                log.arg = sp->property("basara_generals").toString().split("+").first();

                if (Config.Enable2ndGeneral) {
                    room->setPlayerProperty(sp, "general2", "anjiang");
                    log.type = "#BasaraGeneralChosenDual";
                    log.arg2 = sp->property("basara_generals").toString().split("+").last();
                }

                room->sendLog(log, sp);
            }
        }
        return false;
    }

    player->tag["triggerEvent"] = triggerEvent;
    player->tag["triggerEventData"] = data; // For AI

    switch (triggerEvent) {
    case CardEffected: {
        if (player->getPhase() == Player::NotActive) {
            CardEffectStruct ces = data.value<CardEffectStruct>();
            if (ces.card)
                if (ces.card->isKindOf("TrickCard") || ces.card->isKindOf("Slash"))
                    playerShowed(player);

            const ProhibitSkill *prohibit = room->isProhibited(ces.from, ces.to, ces.card);
            if (prohibit && ces.to->hasSkill(prohibit->objectName())) {
                if (prohibit->isVisible()) {
                    LogMessage log;
                    log.type = "#SkillAvoid";
                    log.from = ces.to;
                    log.arg = prohibit->objectName();
                    log.arg2 = ces.card->objectName();
                    room->sendLog(log);

                    room->broadcastSkillInvoke(prohibit->objectName());
                    room->notifySkillInvoked(ces.to, prohibit->objectName());
                }

                return true;
            }
        }
        break;
    }
    case EventPhaseStart: {
        if (player->getPhase() == Player::RoundStart)
            playerShowed(player);

        break;
    }
    case DamageInflicted: {
        playerShowed(player);
        break;
    }
    case BeforeGameOverJudge: {
        if (player->getGeneralName() == "anjiang") {
            QStringList generals = player->property("basara_generals").toString().split("+");
            room->changePlayerGeneral(player, generals.at(0));

            room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
            if (Config.EnableHegemony)
                room->setPlayerProperty(player, "role", getMappedRole(player->getKingdom()));

            generals.takeFirst();
            player->setProperty("basara_generals", generals.join("+"));
            room->notifyProperty(player, player, "basara_generals");
        }
        if (Config.Enable2ndGeneral && player->getGeneral2Name() == "anjiang") {
            QStringList generals = player->property("basara_generals").toString().split("+");
            room->changePlayerGeneral2(player, generals.at(0));
            player->setProperty("basara_generals", QString());
            room->notifyProperty(player, player, "basara_generals");
        }
        break;
    }
    case BuryVictim: {
        DeathStruct death = data.value<DeathStruct>();
        player->bury();
        if (Config.EnableHegemony) {
            ServerPlayer *killer = death.damage ? death.damage->from : NULL;
            if (killer && killer->getKingdom() != "god") {
                if (killer->getKingdom() == player->getKingdom())
                    killer->throwAllHandCardsAndEquips();
                else if (killer->isAlive())
                    killer->drawCards(3, "kill");
            }
            return true;
        }

        break;
    }
    default:
        break;
    }
    return false;
}
