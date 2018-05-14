--凡识：若你的手牌数不小于体力值，你可将一张红色手牌在你的回合内当【杀】，或在你的回合外当【闪】或【桃】使用或者打出。
local thfanshi_skill = {}
thfanshi_skill.name = "thfanshi"
table.insert(sgs.ai_skills, thfanshi_skill)
thfanshi_skill.getTurnUseCard = function(self)
	if self.player:getHandcardNum() < self.player:getHp() then return end
	local cards = self.player:getCards("h")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local red_card

	self:sortByUseValue(cards, true)

	for _, card in ipairs(cards) do
		if card:isRed() and not self:isValuableCard(card) then
			red_card = card
			break
		end
	end

	if not red_card then return nil end
	local suit = red_card:getSuitString()
	local number = red_card:getNumberString()
	local card_id = red_card:getEffectiveId()
	local card_str = ("slash:thfanshi[%s:%s]=%d"):format(suit, number, card_id)
	local slash = sgs.Card_Parse(card_str)
	assert(slash)

	return slash
end

sgs.ai_cardsview.thfanshi = function(self, class_name, player)
	if self.player:getHandcardNum() < self.player:getHp() then return nil end
	if self.player:getPhase() == sgs.Player_NotActive and (class_name ~= "Peach" and class_name ~= "Jink") then return nil end
	if self.player:getPhase() ~= sgs.Player_NotActive and class_name ~= "Slash" then return nil end
	local cards = player:getCards("h")
	for _, id in sgs.qlist(getWoodenOxPile(player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	sgs.ais[player:objectName()]:sortByKeepValue(cards)

	local red_card = nil
	for _, card in ipairs(cards) do
		if card:isRed() and not self:isValuableCard(card, player) then
			red_card = card
			break
		end
	end
	if not red_card then return nil end
	local suit = red_card:getSuitString()
	local number = red_card:getNumberString()
	local card_id = red_card:getEffectiveId()
	if class_name == "Jink" then
		return ("jink:thfanshi[%s:%s]=%d"):format(suit, number, card_id)
	elseif class_name == "Peach" then
		return ("peach:thfanshi[%s:%s]=%d"):format(suit, number, card_id)
	elseif class_name == "Slash" then
		return ("slash:thfanshi[%s:%s]=%d"):format(suit, number, card_id)
	end
	return nil
end

--霁风：锁定技，你的手牌上限+1。
--无

--空锁：准备阶段开始时，你可以选择一名其他角色▶你选择一项：1.令其将人物牌横置；2.令其将人物牌重置。
sgs.ai_skill_playerchosen.thkongsuo = function(self, targets)
	return self:findPlayerToChain(targets)
end

--鬼门：锁定技。其他角色不计入你/另一名处于连环状态的其他角色到另一名处于连环状态的其他角色/你的距离的计算。
--无

--六壬：锁定技。当处于连环状态的其他角色于你的回合内使用第一张牌时，你令此牌对所有目标无效。
--smart-ai.lua SmartAI:askForNullification
--smart-ai.lua SmartAI:willUsePeachTo
--standard-ai.lua sgs.ai_skill_cardask["@multi-jink-start"]
--standard_cards-ai.lua sgs.ai_skill_cardask["slash-jink"]

--神事：每当一名角色跳过一个阶段后，你可以弃置一张牌并令其回复1点体力，每回合限一次。
sgs.ai_skill_cardask["@thshenshi"] = function(self, data, pattern, target)
	if self:isFriend(target) and target:isWounded() then
		local ret = self:askForDiscard("", 1, 1, false, true)
		if #ret == 1 then
			return "$" .. ret[1]
		end
	end
	return "."
end

--解烦：准备阶段开始时，你可以亮出牌堆顶的一张牌，若为基本牌，将其交给一名角色，否则将其置入弃牌堆。你可以重复此流程，直到亮出的牌为非基本牌为止。
sgs.ai_skill_invoke.thjiefan = true

sgs.ai_skill_playerchosen.thjiefan = function(self, targets)
	local ids = { self.player:getTag("ThJiefanId"):toInt() }
	local target, _ = sgs.ai_skill_askforyiji.ikyumeng(self, ids)
	return target or self.player
end

sgs.ai_playerchosen_intention.thjiefan = -20

--创史：你可抵消其他角色使用的一张【心网密葬】、【断灵御剑】或【碎月绮斗】对你或你攻击范围内的一名角色的效果，视为锦囊牌的使用者对你使用了一张无视距离且不计入使用限制的【杀】。
sgs.ai_skill_invoke.thchuangshi = function(self, data)
	local effect = self.player:getTag("ThChuangshiData"):toCardEffect()
	if effect.from and self:isEnemy(effect.from) then
		local canSlash = effect.from:canSlash(self.player, nil, false)
		if not canSlash then
			return true
		else
			local num = self:getCardsNum("Jink", "he", false)
			if num >= 1 then
				return true
			end
		end
	end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.thchuangshi = function(self, player, promptlist)
	local effect = player:getTag("ThChuangshiData"):toCardEffect()
	if effect.from and promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, effect.from, 50)
	end
end

--高天：每当你需要使用或打出一张【闪】时，你可以观看牌堆顶的X张牌（X为存活角色的数量，且至多为4），你可以弃置一张牌并获得其中一张相同颜色的牌，然后将其余的牌以任意顺序置于牌堆顶或置入弃牌堆。
sgs.ai_skill_invoke.thgaotian = true

sgs.ai_skill_cardask["@gaotian-discard"] = function(self, data, pattern)
	if self.player:isNude() then return "." end
	local ids = data:toIntList()
	local cards = sgs.QList2Table(self.player:getCards("he"))
	local cs, reds, blacks = {}, {}, {}
	for _, id in sgs.qlist(ids) do
		local c = sgs.Sanguosha:getCard(id)
		table.insert(cs, c)
		if c:isRed() then
			table.insert(reds, c)
		else
			table.insert(blacks, c)
		end
	end
	self:sortByKeepValue(cards)
	local i = 1
	local useless = cards[1]
	while (useless:isRed() and #reds == 0) or (useless:isBlack() and #blacks == 0) do
		i = i + 1
		if i > #cards then return "." end
		useless = cards[i]
	end
	table.insert(cs, useless)
	self:sortByKeepValue(cs, true, cards)
	if cs[1] == useless then return "." end
	return "$" .. useless:getEffectiveId()
end

sgs.ai_skill_askforag.thgaotian = function(self, card_ids)
	if not self.player:hasFlag("ThGaotianSecond") then return end
	return -1
end

--万灵：其他角色使用的红色【杀】或红色非延时类锦囊牌，在结算后置入弃牌堆时，你可以选择一项：令该角色摸一张牌；或弃置一张牌并获得该角色所使用的牌，每回合限三次。
sgs.ai_skill_invoke.thwanling = function(self, data)
	local move = data:toMoveOneTime()
	local target = self.room:findPlayer(move.from:objectName())
	local card = move.reason.m_extraData:toCard()

	if target and card then
		if self:isFriend(target) then
			return true
		else
			if not self.player:canDiscard(self.player, "he") then return false end
			local ret = self:askForDiscard("thwanling", 1, 1, false, true)
			local cards = { sgs.Sanguosha:getCard(ret[1]) }
			if self:isValuableCard(cards[1]) then return false end
			table.insert(cards, card)
			self:sortByUseValue(cards)
			if card:toString() == cards[1]:toString() then
				return true
			end
		end
	end
	return false
end

sgs.ai_skill_cardask["@thwanling"] = function(self, data, pattern, target)
	local move = data:toMoveOneTime()
	local card = move.reason.m_extraData:toCard()
	
	if not self:isFriend(target) then
		local to_discard = self:askForDiscard("thwanling", 1, 1, false, true)
		return "$" .. to_discard[1]
	elseif card then
		local optional_discard = self:askForDiscard("thwanling", 1, 1, false, true)
		if #optional_discard > 0 then
			local need_obtain
			if card:isKindOf("Slash") then
				need_obtain = self:getCardsNum("Slash") < 1
			elseif card:isKindOf("AOE") and not c:isKindOf("BurningCamps") then
				need_obtain = self:getAoeValue(card) > 0
			elseif self:isValuableCard(card) then
				need_obtain = true
			end
			if need_obtain then
				return "$" .. optional_discard[1]
			end
		end
	end
	return "."
end

sgs.ai_choicemade_filter.cardResponded["@thwanling"] = function(self, player, promptlist)
	if promptlist[#promptlist] == "_nil_" then
		local target = self.room:findPlayer(promptlist[#promptlist - 1])
		if not target then return end
		sgs.updateIntention(player, target, -40)
	end
end

--醉步：每当你受到伤害时，伤害来源可以令你摸一张牌，然后令此伤害-1，每回合限三次。
--smart-ai.lua  SmartAI:damageIsEffective 
--smart-ai.lua  SmartAI:getAoeValueTo
sgs.ai_skill_invoke.thzuibu = function(self,data)
	local target = data:toPlayer()
	return self:isFriend(target)
end

sgs.ai_choicemade_filter.skillInvoke.thzuibu = function(self, player, promptlist)
	local target = self.room:findPlayer(promptlist[#promptlist - 1])
	if target and promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, target, -60)
	end
end

--魔盗：摸牌阶段开始时，你可以放弃摸牌，并选择你攻击范围内的一名角色，若你与其的一个相同区域内的牌数差不大于X+1，交换你们该区域的所有牌，然后你与该角色各摸一张牌（X为你已损失的体力值）。
local parseTargetForModao = function(self, source, target)
	local flag = "h"
	local diff = 0
	local isGood = false
	local handDiff = target:getCards("h"):length() - source:getCards("h"):length()
	local equipDiff = target:getCards("e"):length() - source:getCards("e"):length()
	local judgeDiff = target:getCards("j"):length() - source:getCards("j"):length()
	local standardMax = source:getLostHp() + 1
	if self:isFriend(source, target) then
		if judgeDiff > 0 and judgeDiff <= standardMax and (target:containsTrick("indulgence") or target:containsTrick("supply_shortage")) then
			return true, "j", 1
		end
		if equipDiff ~= 0 and math.abs(equipDiff) <= standardMax and (target:hasSkills(sgs.lose_equip_skill) or self:needToThrowArmor(target)) then
			return true, "e", 3
		end
	else
		local value = 0
		if target:containsTrick("indulgence") then
			value = value - 1
		end
		if target:containsTrick("supply_shortage") then
			value = value - 1
		end
		if target:containsTrick("purple_song") then
			value = value + 1
		end
		if judgeDiff > 0 and judgeDiff <= standardMax and value > 0 then
			return true, "j", 2
		end
		if equipDiff > handDiff and handDiff > 0 and equipDiff <= standardMax and not target:hasSkills(sgs.lose_equip_skill) then
			return true, "e", equipDiff
		end
		if handDiff > 0 and handDiff <= standardMax then
			return true, "h", handDiff
		end
	end
	return false, "h", 0
end

sgs.ai_skill_playerchosen.thmodao = function(self, targets)
	self.room:sortByActionOrder(targets)
	local maxdiff = 0
	local target = nil
	for _, p in sgs.qlist(targets) do
		local g, _, d = parseTargetForModao(self, self.player, p)
		if g and d > maxdiff then
			maxdiff = d
			target = p
		end
	end
	return target
end

sgs.ai_skill_choice.thmodao = function(self, choices, data)	
	local target = data:toPlayer()
	local _, flag = parseTargetForModao(self, self.player, target)
	return flag
end

sgs.ai_choicemade_filter.skillChoice.thmodao = function(self, player, promptlist)
	local target = player:getTag("ThModaoData"):toPlayer()
	if target then
		local flag =  promptlist[#promptlist] 
		local diff =  target:getCards(flag):length() - player:getCards(flag):length() 
		local friendly
		if flag == "e" then
			if not target:hasSkills(sgs.lose_equip_skill) and not self:needToThrowArmor(target) then
				friendly = diff <= 0
			else
				friendly = true
			end
		elseif flag == "h" then
			friendly = diff <= 0
		elseif flag == "j" then
			if target:containsTrick("indulgence") or target:containsTrick("supply_shortage") then
				friendly = true
			else
				friendly = false
			end
		end
		if friendly then
			sgs.updateIntention(player, target, -50)
		else
			sgs.updateIntention(player, target, 50)
		end	
	end
end

--梦生：每当你受到一次伤害后，你可以获得伤害来源的一张牌，然后令该角色的非专属技无效，直到你的下一个回合的回合结束。
sgs.ai_skill_invoke.thmengsheng = function(self, data)
	local target = data:toPlayer()
	return not self:isFriend(target)
end

sgs.ai_choicemade_filter.skillInvoke.thmengsheng = function(self, player, promptlist)
	local target = self.room:findPlayer(promptlist[#promptlist - 1])
	if target and promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, target, 50)
	end
end

--绮想：你攻击范围内的一名角色的弃牌阶段结束时，你可以弃置一张牌，令其摸一张牌或弃置一张手牌。
sgs.ai_skill_cardask["@thqixiang"] = function(self, data, pattern)
	local target = data:toPlayer()
	if self:isEnemy(target) and not target:canDiscard(target, "h") then return "." end
	if not self:isEnemy(target) and not self:isFriend(target) then return "." end
	local ret = self:askForDiscard("thqixiang", 1, 1, false, true)
	if #ret > 0 then
		local c = sgs.Sanguosha:getCard(ret[1])
		if not self:isValuableCard(c) then
			return "$" .. ret[1]
		end
	end
	return "."
end

sgs.ai_skill_choice.thqixiang = function(self, choices, data)
	local target = data:toPlayer()
	return self:isFriend(target) and "draw" or "discard"
end

sgs.ai_choicemade_filter.skillChoice.thqixiang = function(self, player, promptlist)
	local str = promptlist[#promptlist]
	local target = nil
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:getPhase() == sgs.Player_Discard then
			target = p
			break
		end
	end
	if target then
		if str == "draw" then
			sgs.updateIntention(player, target, -40)
		else
			sgs.updateIntention(player, target, 40)
		end
	end
end

--幻胧：出牌阶段开始时，你可以选择一至三项：你于此回合内：1.攻击范围+1；2.出牌阶段可以额外使用一张【杀】；3.可以将两张手牌当【杀】使用或打出。你每选择一项，你的手牌上限便-1，直到回合结束。
local findThHuanlongResult = function(self)
	local player = self.player
	local choices = {}
	table.insert(choices, "thhuanlong1")
	table.insert(choices, "thhuanlong2")
	if not player:hasFlag("thhuanlong") then
		table.insert(choices, "thhuanlong3")
	end
	if #choices == 0 then return "cancel" end
	local slash = sgs.cloneCard("slash")
	local use = { isDummy = true }
	self:useCardSlash(slash, use)
	if self:getCardsNum("Slash") == 0 and use.card and player:getHandcardNum() > 1 and table.contains(choices, "thhuanlong3") then
		return "thhuanlong3"
	elseif self:getCardsNum("Slash") > 1 and use.card and table.contains(choices, "thhuanlong2") then
		return "thhuanlong2"
	elseif not use.card and self:getCardsNum("Slash") > 0 and table.contains(choices, "thhuanlong1") then
		slash:setSkillName("thhuanlong")
		self:useCardSlash(slash, use)
		if use.card then
			return "thhuanlong1"
		end
	end
	local n = self:getOverflow()
	if n < 0 then
		if table.contains(choices, "thhuanlong2") then
			return "thhuanlong2"
		elseif table.contains(choices, "thhuanlong3") then
			return "thhuanlong3"
		elseif table.contains(choices, "thhuanlong1") then
			return "thhuanlong1"
		end
	end
	return "cancel"
end

sgs.ai_skill_invoke.thhuanlong = function(self, data)
	local ret = findThHuanlongResult(self)
	return ret ~= "cancel"
end

sgs.ai_skill_choice.thhuanlong = function(self, choices, data)
	return findThHuanlongResult(self)
end

local thhuanlong_skill = {}
thhuanlong_skill.name = "thhuanlong"
table.insert(sgs.ai_skills, thhuanlong_skill)
thhuanlong_skill.getTurnUseCard = function(self, inclusive)
	if self.player:hasFlag("thhuanlong") then
		return turnUse_spear(self, inclusive, "thhuanlong")
	end
end

function sgs.ai_cardsview.thhuanlong(self, class_name, player)
	if class_name == "Slash" and player:hasFlag("thhuanlong") then
		return cardsView_spear(self, player, "thhuanlong")
	end
end

--御渡：当你于回合外需要使用基本牌时，你可以展示当前回合角色的一张手牌▶若此牌：为♣，你可以将之当任意基本牌使用；不为♣，你可以使用之。每回合限一次。
sgs.ai_skill_invoke.thyudu = function(self, data)
	local target = self.room:getCurrent()
	if not self:isFriend(target) or self:getCardsNum("Jink") < 1 then
		local flag = ("%s_%s_%s"):format("visible", self.player:objectName(), target:objectName())
		for _, c in sgs.qlist(target:getHandcards()) do
			if (c:hasFlag("visible") or c:hasFlag(flag)) and not c:isKindOf("Jink") and c:getSuit() ~= sgs.Card_Club then
				continue
			end
			return true
		end
	end
	return false
end

function sgs.ai_cardsview_valuable.thyudu(self, class_name, player)
	if string.find(class_name, "Slash") or class_name == "Jink" or class_name == "Peach" or class_name == "Analeptic" then
		if player:hasFlag("thyudu") or player:hasFlag("Global_ThYuduFailed") then
			return nil
		end
		if sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE then
			local target = self.room:getCurrent()
			if not target:isKongcheng() and (not self:isFriend(target, player) or getCardsNum(class_name, self.player) < 1) then
				local all_not = true
				local flag = ("%s_%s_%s"):format("visible", player:objectName(), target:objectName())
				for _, c in sgs.qlist(target:getHandcards()) do
					if (c:hasFlag("visible") or c:hasFlag(flag)) and not c:isKindOf(class_name) and c:getSuit() ~= sgs.Card_Club then
						continue
					end
					all_not = false
				end
				if not all_not then
					local obj_name = string.lower(class_name)
					if class_name == "FireSlash" then
						obj_name = "fire_slash"
					elseif class_name == "ThunderSlash" then
						obj_name = "thunder_slash"
					end
					return "@ThYuduCard=.:" .. obj_name
				end
			end
		end
	end
	return nil
end

sgs.ai_skill_invoke.thyudu_use = function(self, data)
	return true
end

--血塚：阶段技。你可以弃置至少一张♣牌，亮出牌堆顶等量的牌，你获取其中所有的非♠牌，然后将其余的牌交给一名其他角色，若如此做且若该角色手牌多于你，其将人物牌翻面。
local thzhaoguo_skill = {}
thzhaoguo_skill.name = "thzhaoguo"
table.insert(sgs.ai_skills, thzhaoguo_skill)
thzhaoguo_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThZhaoguoCard") then return nil end
	if #self.enemies == 0 or #self.friends_noself == 0 then return nil end
	local clubs = {}
	for _, c in sgs.qlist(self.player:getCards("he")) do
		if c:getSuit() == sgs.Card_Club and not self:isValuableCard(c) then
			table.insert(clubs, c:getEffectiveId())
		end
	end
	if #clubs == 0 then return nil end
	return sgs.Card_Parse("@ThZhaoguoCard=" .. table.concat(clubs, "+"))
end

sgs.ai_skill_use_func.ThZhaoguoCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_playerchosen.thzhaoguo = function(self, targets)
	local dummy = self.player:getTag("ThZhaoguoData"):toCard()
	local ids = dummy:getSubcards()
	local n = ids:length()
	self:sort(self.enemies)
	self:sort(self.friends_noself)
	for _, enemy in ipairs(self.enemies) do
		if n <= 2 and enemy:getHandcardNum() + n > self.player:getHandcardNum() and self:toTurnOver(enemy, 0, "thzhaoguo") then
			local no_value = true
			for _, id in sgs.qlist(ids) do
				local c = sgs.Sanguosha:getCard(id)
				if self:isValuableCard(c, enemy) then
					no_value = false
					break
				end
			end
			if no_value then
				return enemy
			end
		end
	end
	for _, friend in ipairs(self.friends_noself) do
		if friend:getHandcardNum() + n <= self.player:getHandcardNum() or n >= 3 then
			return friend
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if enemy:getHandcardNum() + n > self.player:getHandcardNum() and self:toTurnOver(enemy, 0, "thzhaoguo") then
			return enemy
		end
	end
	return targets:at(math.random(0, targets:length() - 1))
end

sgs.ai_playerchosen_intention.thzhaoguo = function(self, from, to)
	local dummy = from:getTag("ThZhaoguoData"):toCard()
	if dummy then
		local ids = dummy:getSubcards()
		local n = ids:length()
		if to:getHandcardNum() + n > from:getHandcardNum() then
			sgs.updateIntention(from, to, 40)
		else
			sgs.updateIntention(from, to, -40)
		end
	end
end

--轮皿：出牌阶段限三次，你可以弃置一张牌然后摸一张牌。
local thlunmin_skill = {}
thlunmin_skill.name = "thlunmin"
table.insert(sgs.ai_skills, thlunmin_skill)
thlunmin_skill.getTurnUseCard = function(self)
	if self.player:usedTimes("ThLunminCard") >= 3 then return nil end
	if not self.player:canDiscard(self.player, "he") then return nil end
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByUseValue(cards, true)
	local need_recover = self:findPlayerToRecover()
	if need_recover then
		local need_suit = {}
		if self.player:hasSkill("thyupan") then
			for i = 0, 3 do
				if bit32.band(self.player:getMark("thyupan"), bit32.lshift(1, i)) == 0 then
					table.insert(need_suit, i)
				end
			end
		end
		if (#need_suit <= 2 and self:isFriend(need_recover) and self:isWeak(need_recover)) or #need_suit <= 1 then
			for _, c in ipairs(cards) do
				if table.contains(need_suit, c:getSuit()) and not self:isValuableCard(c) then
					return sgs.Card_Parse("@ThLunminCard=" .. c:getEffectiveId())
				end
			end
		end
	end
	if not self:isValuableCard(cards[1]) then
		return sgs.Card_Parse("@ThLunminCard=" .. cards[1]:getEffectiveId())
	end
end

sgs.ai_skill_use_func.ThLunminCard = function(card, use, self)
	use.card = card
end

--雨磐：结束阶段开始时，若你于本回合使用或弃置牌的花色数为四种，你可以令一名角色回复1点体力。
sgs.ai_skill_playerchosen.thyupan = function(self, targets)
	return self:findPlayerToRecover(1, targets)
end

sgs.ai_playerchosen_intention.thyupan = -100

--九章：锁定技，你的点数为10~K的牌均视为点数为9的牌。
--无

--数术：每当你距离2以内的一名角色成为一张除【绯想镜诗】外的基本牌或非延时类锦囊牌的目标时，你可以用任意张点数的和与之相同的牌替换之。
function SmartAI:findTableByPlusValue(cards, neednumber, plus, pointer, need_cards, player)
	if neednumber == 0 then
		return true
	end
	if pointer > #cards then
		return false
	end
	for i = pointer, #cards do
		if isCard("Peach", cards[i], player) then continue end
		if cards[i]:getNumber() <= neednumber then
			if self:findTableByPlusValue(cards, neednumber - cards[i]:getNumber(), plus + cards[i]:getNumber(), i + 1, need_cards, player) then
				table.insert(need_cards, cards[i]:getId())
				return true
			end
		end
	end
	if neednumber == 0 then
		return true
	else
		return false 
	end
end

sgs.ai_skill_use["@@thshushu"] = function(self, prompt, method)
	local use = self.player:getTag("ThShushuData"):toCardUse()
	local card = use.card
	local ids = {}
	if card:isVirtualCard() then
		ids = sgs.QList2Table(card:getSubcards())
	else
		ids = { card:getEffectiveId() }
	end
	local need_obtain = false
	for _, id in ipairs(ids) do
		local c = sgs.Sanguosha:getCard(id)
		if c:isKindOf("Slash") then
			need_obtain = self:getCardsNum("Slash") < 1
		elseif c:isKindOf("AOE") and not c:isKindOf("BurningCamps") then
			need_obtain = self:getAoeValue(c) > 0
		elseif self:isValuableCard(c) then
			need_obtain = true
		end
	end
	if need_obtain then
		local n = card:getNumber()
		local need_cards = {}
		local cards = sgs.QList2Table(self.player:getCards("he"))
		self:sortByUseValue(cards, true)
		local findn = self:findTableByPlusValue(cards, n, 0, 1, need_cards, self.player)
		if findn and #need_cards > 0 then
			for _, id in ipairs(need_cards) do
				local c = sgs.Sanguosha:getCard(id)
				if self:isValuableCard(c) then
					return "."
				end
			end
			return "@ThShushuCard=" .. table.concat(need_cards, "+")
		end
	end
	return "."
end

--封凌：限定技，当你进入濒死状态时，你可以摸五张牌，然后你可以重复以下流程：弃置任意张点数和为9的牌，然后回复1点体力。
sgs.ai_skill_invoke.thfengling = function(self,data)
	return true
end

sgs.ai_skill_use["@@thfengling"] = function(self, prompt)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	if #cards == 0 then return "." end 
	self:sortByKeepValue(cards)

	local function numberCompareFunc(card1,card2)
		return card2:getNumber() > card1:getNumber()
	end

	local function bubbleSort(cards, numberCompareFunc)
		local len = #cards 
		local i = len
		while i > 0 do
			local j = 1
			while j < len do
				if numberCompareFunc(cards[j], cards[j+1]) then
					cards[j], cards[j+1] = cards[j+1], cards[j]
				end
				j = j + 1
			end
			i = i - 1
		end
	end

	bubbleSort(cards,numberCompareFunc)

	local need_cards = {}
	local find9 = self:findTableByPlusValue(cards, 9, 0, 1, need_cards, self.player)

	if find9 and #need_cards > 0 then
		return "@ThFenglingCard=" .. table.concat(need_cards, "+")
	end
	return "."
end

--影弑：出牌阶段限三次，你可以对一名你与其距离为X的角色使用一张不计入使用限制的【杀】（X为你此阶段发动“影弑”的次数+1）。
local thyingshi_skill = {}
thyingshi_skill.name = "thyingshi"
table.insert(sgs.ai_skills, thyingshi_skill)
thyingshi_skill.getTurnUseCard = function(self)
	if self.player:usedTimes("ThYingshiCard") >= 3 or self.player:hasFlag("Global_ThYingshiFailed") then return end
	return sgs.Card_Parse("@ThYingshiCard=.")
end

sgs.ai_skill_use_func.ThYingshiCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.ThYingshiCard = sgs.ai_use_priority.Slash + 0.1

--葬魂：每当你使用【杀】造成伤害后，你可以摸一张牌，然后你计算与其他角色的距离+1，直到回合结束。
sgs.ai_skill_invoke.thzanghun = function(self)
	local slash = sgs.cloneCard("slash")
	slash:setSkillName("thzanghun")
	local use = { isDummy = true }
	self:useCardSlash(slash, use)
	if use.card then
		return true
	end
end

--抑梦：每当一名其他角色对你或与你相邻的角色造成一次伤害后，你可以获得该角色的一张牌，然后令其摸一张牌。
sgs.ai_skill_invoke.thyimeng = function(self, data)
	local target = data:toPlayer()
	if target and self:isEnemy(target) then
		if target:hasSkills(sgs.lose_equip_skill) and target:isKongcheng() then
			return false
		end
		if self:needKongcheng(target) and not self:needKongcheng(target, true) and target:getHandcardNum() == 1 and not target:hasEquip() then
			return false
		end
	end
	return true
end

--虚遊：出牌阶段限一次，你使用【杀】时可以无视合法性指定一名角色为目标，若该【杀】没有造成伤害，你依次弃置所有与你相邻的角色的一张牌；当该【杀】造成一次伤害后，你摸一张牌。
local thxuyou_skill = {}
thxuyou_skill.name = "thxuyou"
table.insert(sgs.ai_skills, thxuyou_skill)
thxuyou_skill.getTurnUseCard = function(self)
	local slash = self:getCard("Slash")
	if slash and self:slashIsAvailable(self.player, slash) then
		if not self.player:hasFlag("ThXuyou") and not self.player:hasFlag("Global_ThXuyouFailed") then
			return sgs.Card_Parse("@ThXuyouCard=.")
		end
	end
end

sgs.ai_skill_use_func.ThXuyouCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.ThXuyouCard = sgs.ai_use_priority.Slash + 0.1
sgs.ai_choicemade_filter.cardChosen.thxuyou = sgs.ai_choicemade_filter.cardChosen.snatch

--徨笏：每当你使用或打出一张【闪】响应一名其他角色对你使用的牌时，你可以弃置至少一张手牌，然后令该角色弃置等量的牌。
sgs.ai_skill_discard.thhuanghu = function(self, discard_num, min_num, optional, include_equip, pattern)
	if discard_num == 998 and min_num == 1 then
		local target = self.player:getTag("ThHuanghuTarget"):toPlayer()
		local n = 0
		if self:isFriend(target) then
			if self:needToThrowArmor(target)
					or (target:hasSkills(sgs.lose_equip_skill) and target:hasEquip())
					or (self:needKongcheng(target) and target:getHandcardNum() == 1) then
				n = 1
			else
				return {}
			end
		else
			n = target:getCardCount()
		end
		local ret = self:askForDiscard("", n, 1, false, false)
		if #ret then
			local ret2 = {}
			for _, id in ipairs(ret) do
				if isCard("Peach", id, self.player) then
					return ret2
				else
					table.insert(ret2, id)
				end
			end
			return ret2
		end
	else
		return self:askForDiscard("", discard_num, min_num, optional, include_equip, pattern)
	end
	return {}
end

--凛要：每当一名其他角色需要使用或打出一张【闪】时，若你的人物牌背面朝上，你可以将你的人物牌翻面，视为该角色使用或打出了一张【闪】。
sgs.ai_skill_invoke.thlinyao = function(self, data)
	local target = data:toPlayer()
	if target and self:isFriend(target) then
		return true
	end
	return false
end

sgs.ai_slash_prohibit.thlinyao = function(self, from, to, card)
	if self:isFriend(to, from) then return false end
	if not sgs.isJinkAvailable(from, to, card) then return false end
	local linyaos = self.room:findPlayersBySkillName("thlinyao")
	for _, linyao in sgs.qlist(linyaos) do
		if self:isFriend(to, linyao) and not linyao:faceUp() then
			return true
		end
	end
	return false
end

--绯镜：每当你失去最后的手牌时，你可将手牌补至等同于你体力上限的张数，然后将你的人物牌翻面。
sgs.ai_skill_invoke.thfeijing = true

--偶祭：每当你或你攻击范围内的一名角色的装备区于你的回合内改变时，你可以选择一项：弃置一名其他角色的一张手牌；或摸一张牌。
sgs.ai_skill_invoke.thouji = true

sgs.ai_skill_playerchosen.thouji = function(self, targets)
	local target = self:findPlayerToDiscard("h", false, true, targets)
	if target and self:isEnemy(target) and ((target:getHandcardNum() == 1 and not self:needKongcheng(target, true)) or (target:getHandcardNum() == 2 and getKnownCard(target, self.player, "Peach") > 0)) then
		return target
	end
	return nil
end

sgs.ai_playerchosen_intention.thouji = 30

--镜缘：出牌阶段限一次，你可以弃置一张红色基本牌，然后令装备区里有牌的一至两名角色各选择一项：将其装备区里的一张牌交给除其以外的一名角色；或令你获得其一张手牌。
local thjingyuansp_skill = {}
thjingyuansp_skill.name = "thjingyuansp"
table.insert(sgs.ai_skills, thjingyuansp_skill)
thjingyuansp_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThJingyuanspCard") or not self.player:canDiscard(self.player, "h") then return end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards)
	for _, c in ipairs(cards) do
		if c:isRed() and c:getTypeId() == sgs.Card_TypeBasic then
			return sgs.Card_Parse("@ThJingyuanspCard=" .. c:getId())
		end
	end
end

function thjingyuansp_inMyAttackRange(from, target, equip)
	if from:objectName() == target:objectName() then
		return true
	end
	local fix = 0
	if equip:isKindOf("DefensiveHorse") then
		fix = fix - 1
	end
	return from:inMyAttackRange(target, fix)
end

sgs.ai_skill_use_func.ThJingyuanspCard = function(card, use, self)
	local targets = {}
	self:sort(self.friends, "defense")
	self.friends = sgs.reverse(self.friends)
	for _, p in ipairs(self.friends) do
		if not p:hasEquip() and self:isWeak(p) then
			continue
		end
		for _, c in sgs.qlist(p:getEquips()) do
			if thjingyuansp_inMyAttackRange(self.player, p, c) then
				table.insert(targets, p)
				break
			end
		end
	end
	self:sort(self.enemies, "defense")
	for _, p in ipairs(self.enemies) do
		if not p:hasEquip() then
			continue
		end
		if self.player:inMyAttackRange(p) then
			table.insert(targets, p)
			break
		end
	end
	for _, p in ipairs(self.enemies) do
		if not p:hasEquip() then
			continue
		end
		if not table.contains(targets, p) then
			table.insert(targets, p)
			break
		end
	end
	for _, p in ipairs(self.friends) do
		if not p:hasEquip() or self:isWeak(p) then
			continue
		end
		if not table.contains(targets, p) then
			table.insert(targets, p)
			break
		end
	end
	if #targets ~= 0 then
		use.card = card
		if use.to then
			use.to:append(targets[1])
			if #targets > 1 then
				use.to:append(targets[2])
			end
		end
		return
	end
	use.card = nil
end

sgs.ai_skill_cardask["@thjingyuansp"] = function(self, data, pattern, target)
	if self:isFriend(target) then
		return sgs.ai_skill_cardask["@thjingyuansp-give"](self, data, pattern, target)
	else
		if self.player:getEquips():length() == 1 and self.player:getArmor() then
			return "."
		else
			local cards = sgs.QList2Table(self.player:getCards("e"))
			self:sortByKeepValue(cards)
			for _, cd in ipairs(cards) do
				if thjingyuansp_inMyAttackRange(target, self.player, cd) then
					continue
				else
					return "$" .. cd:getEffectiveId()
				end
			end
		end
	end
	return "."
end

sgs.ai_skill_cardask["@thjingyuansp-give"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getCards("e"))
	self:sortByKeepValue(cards)
	for _, cd in ipairs(cards) do
		if thjingyuansp_inMyAttackRange(target, self.player, cd) then
			if self:isFriend(target) then
				return "$" .. cd:getEffectiveId()
			else
				continue
			end
		end
	end
	return "$" .. cards[1]:getEffectiveId()
end

sgs.ai_skill_playerchosen.thjingyuansp = function(self, targets)
	local ailisis = self.room:findPlayersBySkillName("thjingyuansp")
	for _, ailisi in sgs.qlist(ailisis) do
		if self:isFriend(ailisi) and targets:contains(ailisi) and ailisi:getPhase() == sgs.Player_Play then
			return ailisi
		end
	end
	local card = self.player:getTag("ThJingyuanspCard"):toCard()
	local _, target = self:getCardNeedPlayer({card}, self.friends_noself, false)
	if target then
		return target
	end
	return self.friends_noself[1]
end

sgs.ai_playerchosen_intention.thjingyuansp = -30

--绯护：出牌阶段限一次，你可以选择一名体力值不大于你的角色并选择一项：弃置其一张牌，然后令该角色回复1点体力；或对其造成1点伤害，然后令该角色回复1点体力，并摸一张牌。若你已损失的体力值不大于2，你发动“绯护”不可以选择你为目标。
local thfeihu_skill = {}
thfeihu_skill.name = "thfeihu"
table.insert(sgs.ai_skills, thfeihu_skill)
thfeihu_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThFeihuCard") then return end
	return sgs.Card_Parse("@ThFeihuCard=.")
end

sgs.ai_skill_use_func.ThFeihuCard = function(card, use, self)
	local players = sgs.QList2Table(self.room:getAlivePlayers())
	local friends = {}
	local enemies = {}
	for _, p in ipairs(players) do
		if self.player:objectName() == p:objectName() and p:getLostHp() <= 2 then
			continue
		end
		if p:getHp() > self.player:getHp() then
			continue
		end
		if self:isFriend(p) then
			if not self:damageIsEffective(p, nil, self.player)
					or (self:needToThrowArmor(p) and (not p:hasArmorEffect("silver_lion") or p:getLostHp() > 1))
					or (p:hasSkills(sgs.lose_equip_skill) and p:hasEquip())
					or (self:needKongcheng(p, true) and p:getHandcardNum() == 1)
					or (self:getOverflow(p) > 1 and p:isWounded())
					or (p:getHp() > 1 and p:hasSkills(sgs.masochism_skill)) then
				table.insert(friends, p)
			end
		elseif self:isEnemy(p) then
			if not self:doNotDiscard(p, "he") then
				table.insert(enemies, p)
			end
		end
	end
	if #friends > 0 then
		self:sort(friends, "hp")
		use.card = card
		if use.to then
			use.to:append(friends[1])
		end
		return
	end
	if #enemies > 0 then
		self:sort(enemies, "losthp")
		for _, p in ipairs(enemies) do
			if not p:isWounded() then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
		self:sort(enemies, "hp")
		for _, p in ipairs(enemies) do
			if p:getHp() == 1 and self:getAllPeachNum(p) <= 0 then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
	self:sort(self.friends, "defense")
	for _, p in ipairs(self.friends) do
		if self.player:objectName() == p:objectName() and p:getLostHp() <= 2 then
			continue
		end
		if p:getHp() > self.player:getHp() then
			continue
		end
		if p:isWounded() and self.player:canDiscard(p, "he") then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
		if p:getHp() > 1 then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
end

sgs.ai_skill_choice.thfeihu = function(self, choices, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		if not self:damageIsEffective(target, nil, self.player) then
			return "damage"
		end
		if self:needToThrowArmor(target) and (not target:hasArmorEffect("silver_lion") or target:getLostHp() > 1) and string.find(choices, "recover") then
			return "recover"
		end
		if target:hasSkills(sgs.lose_equip_skill) and target:hasEquip() and string.find(choices, "recover") then
			return "recover"
		end
		if self:needKongcheng(target, true) and target:getHandcardNum() == 1 and string.find(choices, "recover") then
			return "recover"
		end
		if self:getOverflow(target) > 1 and target:isWounded() and string.find(choices, "recover") then
			return "recover"
		end
		if target:getHp() > 1 and target:hasSkills(sgs.masochism_skill) then
			return "damage"
		end
		if target:isWounded() and self.player:canDiscard(target, "he") and string.find(choices, "recover") then
			return "recover"
		end
		if target:getHp() > 1 then
			return "damage"
		end
	elseif self:isEnemy(target) then
		if not target:isWounded() and string.find(choices, "recover") then
			return "recover"
		end
	end
	return "damage"
end

sgs.ai_skill_cardchosen.thfeihu = function(self, who, flags, method)
	local card_id = self:askForCardChosen(who, flags, "", method)
	if self:isFriend(who) then
		if who:getLostHp() == 1 and who:getArmor() and who:getArmor():getEffectiveId() == card_id then
			if who:hasArmorEffect("silver_lion") then
				return self:askForCardChosen(who, "h", "", method)
			end
		end
	elseif self:isEnemy(who) then
		if who:getLostHp() == 1 and self:needToThrowArmor(who) and who:hasArmorEffect("silver_lion") then
			return who:getArmor():getEffectiveId()
		end
	end
	return card_id
end

sgs.ai_card_intention.ThFeihuCard = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		if not to:isWounded() then continue end
		sgs.updateIntention(from, to, -50)
	end
end
