--乱神：出牌阶段限一次，若你的手牌数不小于体力值，你可以展示至少一张手牌并让一名其他角色选择一项：弃置你展示的牌并弃置等量的牌；或获得你展示的牌，然后将手牌补至等同于其体力上限的张数并将其人物牌翻面。
local thluanshen_skill = {}
thluanshen_skill.name = "thluanshen"
table.insert(sgs.ai_skills, thluanshen_skill)
thluanshen_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThLuanshenCard") or self.player:getHandcardNum() < self.player:getHp() then return end
	return sgs.Card_Parse("@ThLuanshenCard=.")
end

sgs.ai_skill_use_func.ThLuanshenCard = function(card, use, self)
	local cards = sgs.QList2Table(self.player:getHandcards())
	if #cards == 0 then return end
	self:sortByKeepValue(cards)
	local n = self:getOverflow()
	if n <= 0 then
		local str = "@ThLuanshenCard=" .. cards[1]:getEffectiveId()
		if not self:isValuableCard(cards[1]) then
			self:sort(self.enemies, "handcard")
			for _, p in ipairs(self.enemies) do
				if not p:faceUp() then continue end
				if p:getMaxHp() - p:getHandcardNum() == 1 and self:toTurnOver(p, 0) then
					use.card = sgs.Card_Parse(str)
					if use.to then
						use.to:append(p)
					end
					return
				end
			end
		end
		self:sort(self.enemies, "handcard")
		for _, p in ipairs(self.friends_noself) do
			if p:getMaxHp() - p:getHandcardNum() > 3 or not self:toTurnOver(p, 0) then
				use.card = sgs.Card_Parse(str)
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	else
		self:sort(self.enemies, "handcard")
		self.enemies = sgs.reverse(self.enemies)
		for _, p in ipairs(self.enemies) do
			local m = p:getMaxHp() - p:getHandcardNum()
			if m <= n and m > p:getCardCount() and self:toTurnOver(p, 0) then
				local uses = {}
				for _, c in ipairs(cards) do
					if self:isValuableCard(c) or self:isValuableCard(c, p) then continue end
					table.insert(uses, tostring(c:getEffectiveId()))
					if #uses == m then
						break
					end
				end
				if #uses == m or (#uses == m - 1 and #uses > 0) then
					use.card = sgs.Card_Parse("@ThLuanshenCard=" .. table.concat(uses, "+"))
					if use.to then
						use.to:append(p)
					end
					return
				end
			end
		end
		if n >= 2 then
			for _, p in ipairs(self.enemies) do
				local m = p:getMaxHp() - p:getHandcardNum()
				if m >= 2 and m <= 3 and self:toTurnOver(p, m - 2) then
					local uses = {}
					for _, c in ipairs(cards) do
						if self:isValuableCard(c) or self:isValuableCard(c, p) then continue end
						table.insert(uses, tostring(c:getEffectiveId()))
						if #uses == 2 then
							break
						end
					end
					if #uses == 2 then
						use.card = sgs.Card_Parse("@ThLuanshenCard=" .. table.concat(uses, "+"))
						if use.to then
							use.to:append(p)
						end
						return
					end
				end
			end
		end
	end
end

sgs.ai_skill_choice.thluanshen = function(self, choices, data)
	local ids = data:toIntList()
	if ids:length() > 3 or self.player:getMaxHp() - self.player:getHandcardNum() > 3 or not self.player:faceUp() then
		return "turnover"
	end
	local ret = self:askForDiscard("", ids:length(), ids:length(), false, true)
	if #ret ~= ids:length() then
		return "turnover"
	else
		for _, id in ipairs(ret) do
			local c = sgs.Sanguosha:getCard(id)
			if self:isValuableCard(c) then
				return "turnover"
			end
		end
		return "discard"
	end
	return "turnover"
end

sgs.ai_card_intention.ThLuanshenCard = function(self, card, from, tos)
	local intention = 80 / card:subcardsLength()
	for _, to in ipairs(tos) do
		if not self:toTurnOver(to, to:getMaxHp() - to:getHandcardNum() - card:subcardsLength()) then intention = -intention end
		sgs.updateIntention(from, to, intention)
	end
end

sgs.ai_use_priority.ThLuanshenCard = -10

--飞蛮：出牌阶段开始时，你可以对一名其他角色造成1点伤害，然后令其选择一项：对其攻击范围内的一名角色造成一点火焰伤害，或获得场上的一张牌。
sgs.ai_skill_playerchosen.thfeiman = function(self, targets)
	local isGoodThFeimanTarget = function(player)
		if self:isFriend(player) and not (self:isWeak(player) and self:damageIsEffective(player, sgs.DamageStruct_Normal, self.player)) then
			for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
				if not player:inMyAttackRange(p) then continue end
				if self:isEnemy(p) and self:damageIsEffective(p, sgs.DamageStruct_Fire, player) then
					return true
				end
				if p:isChained() and self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Fire, 1) then
					return true
				end
			end
		elseif self:isEnemy(player) and self:damageIsEffective(player, sgs.DamageStruct_Normal, self.player) then
			if player:getHp() == 1 and self:getAllPeachNum(player) < 1 then
				local weak_friend = nil
				for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
					if not player:inMyAttackRange(p) then continue end
					if self:isFriend(p) and self:isWeak(p) then
						weak_friend = p
						break
					end
				end
				if not weak_friend or self:getAllPeachNum(weak_friend) >= 1 then
					return true
				end
			end
			for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
				if not player:inMyAttackRange(p) then continue end
				if self:isFriend(p) and self:damageIsEffective(p, sgs.DamageStruct_Fire, player) then
					return false
				end
				if p:isChained() and not self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Fire, 1) then
					return false
				end
			end
			return true
		end
		return false
	end

	local targetlist = sgs.QList2Table(targets)
	self:sort(targetlist, "hp")
	local victims, seconds = {}, {}
	for _, target in ipairs(targetlist) do
		if self:isEnemy(target) and isGoodThFeimanTarget(target) then
			table.insert(victims, target)
		end
		if self:isFriend(target) and isGoodThFeimanTarget(target) then
			table.insert(seconds, target)
		end
	end
	if #victims + #seconds == 0 then
		return nil
	end
	if #victims > 0 then
		return victims[1]
	else
		return seconds[1]
	end
	return nil
end

sgs.ai_skill_choice.thfeiman = function(self, choices)
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if not self.player:inMyAttackRange(p) then continue end
		if self:isEnemy(p) and self:damageIsEffective(p, sgs.DamageStruct_Fire, self.player) and self:isWeak(p) and self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Fire, 1) then
			return "damage"
		end
	end
	local targets = self:findPlayerToDiscard("ej", true, false, nil, true)
	local has_e = false
	for _, p in ipairs(targets) do
		if self:isEnemy(p) then
			has_e = true
			break
		end
	end
	if has_e and self:isFriend(targets[1]) then
		return "obtain"
	end
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if not self.player:inMyAttackRange(p) then continue end
		if self:isEnemy(p) and self:damageIsEffective(p, sgs.DamageStruct_Fire, self.player) then
			return "damage"
		end
	end
	return string.find(choices, "obtain") and "obtain" or "damage"
end

sgs.ai_skill_playerchosen.thfeiman_damage = function(self, targets)
	local targets_list = sgs.QList2Table(targets)
	self:sort(targets_list, "hp")
	for _, p in ipairs(targets_list) do
		if self:isEnemy(p) and self:damageIsEffective(p, sgs.DamageStruct_Fire, self.player) and self:isWeak(p) and self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Fire, 1) then
			return p
		end
	end
	for _, p in ipairs(targets_list) do
		if self:isEnemy(p) and self:damageIsEffective(p, sgs.DamageStruct_Fire, self.player) then
			return p
		end
	end
	for _, p in ipairs(targets_list) do
		if self:isFriend(p) and p:isChained() and self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Fire, 1) then
			return p
		end
	end
	for _, p in ipairs(targets_list) do
		if not self:damageIsEffective(p, sgs.DamageStruct_Fire, self.player) then
			return p
		end
	end
	for _, p in ipairs(targets_list) do
		if p:hasSkills(sgs.masochism_skill) then
			return p
		end
	end
	return targets_list[#targets_list]
end

sgs.ai_skill_playerchosen.thfeiman_obtain = function(self, targets)
	return self:findPlayerToDiscard("ej", true, false, targets, true)
end

sgs.ai_choicemade_filter.cardChosen.thfeiman = sgs.ai_choicemade_filter.cardChosen.snatch

--怪奇：一名角色的出牌阶段结束时，若其此阶段内至少有一名角色受到了2点或更多的伤害，你可以令当前回合的角色摸一张牌。
sgs.ai_skill_invoke.thguaiqi = function(self, data)
	local target = data:toPlayer()
	return self:isFriend(target)
end

sgs.ai_choicemade_filter.skillInvoke.thguaiqi = function(self, player, promptlist)
	local to = self.room:findPlayer(promptlist[#promptlist - 1])
	if to then
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, to, -20)
		else
			sgs.updateIntention(player, to, 5)
		end
	end
end

--惊涛：每当你失去最后的手牌时，你可以摸两张牌。
sgs.ai_skill_invoke.thjingtao = true

--纵溺：你回合内的摸牌阶段开始时，你可以将此阶段视为出牌阶段，若如此做，此阶段结束时，你须弃置至少一张手牌。
sgs.ai_skill_invoke.thzongni = function(self, data)
	local use = { isDummy = true }
	self:activate(use)
	if use.card then
		return true
	end
end

sgs.ai_skill_discard.thzongni = function(self)
	local handcards = sgs.QList2Table(self.player:getHandcards())
	local peach_num, slash_num = 0, 0
	local to_discard = {}
	for _, c in ipairs(handcards) do
		if self.player:isCardLimited(c, sgs.Card_MethodUse) then
			table.insert(to_discard, c:getEffectiveId())
			continue
		end
		if c:isKindOf("Peach") then
			if peach_num < self.player:getLostHp() then
				peach_num = peach_num + 1
			else
				table.insert(to_discard, c:getEffectiveId())
			end
			continue
		end
		if c:isKindOf("Slash") then
			if slash_num < (self:hasCrossbowEffect() and 888 or 1) then
				local use = { isDummy = true }
				self:useCardSlash(c, use)
				if use.card then
					slash_num = slash_num + 1
				else
					table.insert(to_discard, c:getEffectiveId())
				end
			else
				table.insert(to_discard, c:getEffectiveId())
			end
			continue
		end

		local use = { isDummy = true }
		local typeId = c:getTypeId()
		self["use" .. sgs.ai_type_name[typeId + 1] .. "Card"](self, c, use)

		if not use.card then
			table.insert(to_discard, c:getEffectiveId())
		end
	end
	if #to_discard == 0 then
		return self:askForDiscard("thzongni", 1, 1, false, false)
	else
		return to_discard
	end
end

--岚奏：每当你弃置一次牌时，若其中有非【杀】基本牌或【三粒天滴】，你可以令一名其他角色摸一张牌；每当其他角色弃置一次牌时，若其中有非【杀】基本牌或【三粒天滴】，该角色可以令你摸一张牌。
sgs.ai_skill_playerchosen.thlanzou = function(self, targets)
	return self:findPlayerToDraw(false, 1)
end

sgs.ai_skill_invoke.thlanzou = function(self, data)
	local target = data:toPlayer()
	return self:isFriend(target)
end

sgs.ai_playerchosen_intention.thlanzou = -20
sgs.ai_choicemade_filter.skillInvoke.thlanzou = function(self, player, promptlist)
	local to = self.room:findPlayer(promptlist[#promptlist - 1])
	if to then
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, to, -20)
		else
			sgs.updateIntention(player, to, 5)
		end
	end
end

--心绮：锁定技，当其他角色使用黑色非延时类锦囊牌指定你为目标时，需令你摸一张牌，且若其有手牌需令你观看之，否则该锦囊牌对你无效。
sgs.ai_skill_choice.thxinqi = function(self, choices, data)
	local effect = data:toCardUse()
	local target = self.player:getTag("ThXinqiTarget"):toPlayer()
	if (effect.card:isKindOf("GodSalvation") and self.player:isWounded()) or effect.card:isKindOf("ExNihilo") then
		return self:isFriend(target) and "show" or "cancel"
	elseif effect.card:isKindOf("AmazingGrace") then
		return self:isFriend(target) and "show" or "cancel"
	else
		return self:isFriend(target) and "cancel" or "show"
	end
end

--能舞：其他角色的准备阶段开始时，若你有手牌，你可以令该角色选择其一张手牌，然后你选择一种牌的类别并亮出该牌：若为你所选的类别，该角色须失去1点体力；否则你须弃置一张牌。该角色不可以使用其他的牌，直到该角色因使用、打出或弃置而失去这张手牌或回合结束。
sgs.ai_skill_invoke.thnengwu = function(self, data)
	local target = data:toPlayer()
	if self:isEnemy(target) then
		local ret = self:askForDiscard("thnengwu", 1, 1, false, true)
		if #ret == 1 and not self:isValuableCard(sgs.Sanguosha:getCard(ret[1])) then
			return true
		end
	end
	return false
end

sgs.ai_cardshow.thnengwu = function(self, requestor)
	local flag = string.format("%s_%s_%s", "visible", requestor:objectName(), self.player:objectName())
	local rets = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if not c:isAvailable(self.player) then continue end
		if c:hasFlag("visible") or c:hasFlag(flag) then
			table.insert(rets, c)
		else
			table.insert(rets, c)
			table.insert(rets, c)
		end
	end
	if #rets == 0 then
		return self.player:getRandomHandCard()
	end
	return rets[math.random(1, #rets)]
end

sgs.ai_skill_choice.thnengwu = function(self, choices, data)
	local target = data:toPlayer()
	local types = {}
	local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), target:objectName())
	for _, c in sgs.qlist(target:getHandcards()) do
		if not c:isAvailable(target) then continue end
		if c:hasFlag("visible") or c:hasFlag(flag) then
			table.insert(types, c:getType())
			table.insert(types, c:getType())
			table.insert(types, c:getType())
		else
			table.insert(types, "basic")
			table.insert(types, "equip")
			table.insert(types, "trick")
		end
	end
	if #types == 0 then
		table.insert(types, "basic")
		table.insert(types, "equip")
		table.insert(types, "trick")
	end
	return types[math.random(1, #types)]
end

sgs.ai_choicemade_filter.skillInvoke.thnengwu = function(self, player, promptlist)
	local to = self.room:findPlayer(promptlist[#promptlist - 1])
	if to then
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, to, 50)
		end
	end
end

--宝锤：其他角色的出牌阶段开始时，若你有手牌，你可令其交给你一张手牌，然后你将全部的手牌置于你的人物牌上，若如此做，该角色可于此出牌阶段将你人物牌上的牌如手牌般使用或打出，且此出牌阶段结束时，你获得你人物牌上全部的牌，并摸一张牌。
sgs.ai_skill_invoke.thbaochui = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		return true
	end
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if c:isAvailable(target) then
			return false
		end
	end
	return true
end

sgs.ai_skill_cardask["@thbaochui"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getHandcards())
	if self:isFriend(target) then
		self:sortByKeepValue(cards, true)
		return "$" .. cards[1]:getEffectiveId()
	else
		self:sortByUsePriority(cards)
		return "$" .. cards[1]:getEffectiveId()
	end
end

sgs.ai_choicemade_filter.skillInvoke.thbaochui = function(self, player, promptlist)
	local to = self.room:findPlayer(promptlist[#promptlist - 1])
	if to then
		if promptlist[#promptlist] == "yes" then
			for _, c in sgs.qlist(player:getHandcards()) do
				if c:isAvailable(to) then
					sgs.updateIntention(player, to, -50)
					break
				end
			end
		end
	end
end

--倚势：锁定技，你人物牌上的牌对你无效。
--无
function SmartAI:isThYishiCard(card, from)
	if not card:isVirtualCard() then
		return from:getPile("currency"):contains(card:getEffectiveId())
	else
		local ids = sgs.QList2Table(card:getSubcards())
		if #ids == 1 then
			local acard = sgs.Sanguosha:getCard(ids[1])
			if acard:getClassName() == card:getClassName() and from:getPile("currency"):contains(ids[1]) then
				return true
			end
		end
	end
	return false
end

--魔具：锁定技，摸牌阶段，若你的装备区有武器牌，你摸牌的数量改为你攻击范围的数量（至少摸两张）；出牌阶段，当你使用【杀】指定一名角色为目标后，若你的装备区有防具牌，此【杀】不可以被【闪】响应；你的装备区每有一张坐骑牌，你的手牌上限便+1。
function sgs.ai_cardneed.thmoju(to, card, self)
	return (isCard("Slash", card, to) and getKnownCard(to, self.player, "Slash", true) == 0)
			or (card:isKindOf("Weapon") and sgs.weapon_range[weapon:getClassName()] > 2)
			or (card:isKindOf("Armor") and getKnownCard(to, self.player, "Armor", true) == 0)
end

--莲影：出牌阶段限一次，你可以弃置两张牌，若如此做，你获得技能“赤莲”和“疾步”，直到回合结束。
local thlianying_skill = {}
thlianying_skill.name = "thlianying"
table.insert(sgs.ai_skills, thlianying_skill)
thlianying_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThLianyingCard") then return end
	if self.player:getCardCount() < 2 then return end
	return sgs.Card_Parse("@ThLianyingCard=.")
end

sgs.ai_skill_use_func.ThLianyingCard = function(card, use, self)
	local canUseSlash = self:slashIsAvailable()
	if canUseSlash then
		local must_use = false
		local slash = sgs.cloneCard("slash")
		local use = { isDummy = true }
		self:useCardSlash(slash, use)
		if not use.card then
			slash:setSkillName("thlianying")
			self:useCardSlash(slash, use)
			if not use.card then
				return
			else
				must_use = true
			end
		end
		if self:getOverflow() > 0 or must_use then
			local cards = sgs.QList2Table(self.player:getCards("he"))
			self:sortByKeepValue(cards)
			if #cards < 3 then
				return
			end
			local c1, c2 = nil, nil
			for i, c in ipairs(cards) do
				if not self:isValuableCard(c) then
					local slash = false
					for n = #cards, i, -1 do
						if n == i then continue end
						if isCard("Slash", cards[n], self.player) or cards[n]:isRed() then
							slash = true
							break
						end
					end
					if slash then
						if c1 then
							c2 = c
							break
						else
							c1 = c
						end
					end
				end
			end
			if c1 and c2 then
				use.card = sgs.Card_Parse(("@ThLianyingCard=%d+%d"):format(c1:getEffectiveId(), c2:getEffectiveId()))
				return
			end
		end
	elseif self:getOverflow() > 1 then
		local cards = sgs.QList2Table(self.player:getCards("he"))
		self:sortByKeepValue(cards)
		local c1, c2 = nil, nil
		for _, c in ipairs(cards) do
			if not self:isValuableCard(c) then
				if c1 then
					c2 = c
					break
				else
					c1 = c
				end
			end
		end
		if c1 and c2 then
			use.card = sgs.Card_Parse(("@ThLianyingCard=%d+%d"):format(c1:getEffectiveId(), c2:getEffectiveId()))
			return
		end
	end
end

sgs.ai_use_priority.ThLianyingCard = sgs.ai_use_priority.Slash + 0.1

--远哮：每当你使用【杀】指定一名目标角色后，若该角色的手牌数大于你，你可以获得其一张手牌并展示之，若此牌为红色，其不能使用【闪】抵消此【杀】。
sgs.ai_skill_invoke.thyuanxiao = function(self, data)
	local target = data:toPlayer()
	if not self:isFriend(target) then
		return not (self:needKongcheng(target) and target:getHandcardNum() == 1)
	end
end

sgs.ai_choicemade_filter.skillInvoke.thyuanxiao = function(self, player, promptlist)
	local target = findPlayerByObjectName(self.room, promptlist[#promptlist - 1])
	if target then
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, target, 60)
		else
			sgs.updateIntention(player, target, -40)
		end
	end
end

--寤呓：一名角色的结束阶段开始时，若你此回合内因弃置而置入弃牌堆的牌中有基本牌，你可以视为使用一张无视距离的【杀】；若这些牌中也有装备牌，则此【杀】无视目标角色装备区的装备牌；若这些牌中也有锦囊牌，则此【杀】可以额外指定一名目标。
sgs.ai_skill_playerchosen.thwuyi = sgs.ai_skill_playerchosen.zero_card_as_slash

--苜迷：当你需要使用或打出一张【闪】时，可以弃置两张牌，视为使用或打出一张【闪】。
sgs.ai_skill_use["@@thmumi"] = function(self, prompt, method)
	for _, c in sgs.qlist(self.player:getCards("he")) do
		if isCard("Jink", c, self.player) then
			return "."
		end
	end
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		if isCard("Jink", id, self.player) then
			return "."
		end
	end
	local to_dis = {}
	for _, c in sgs.qlist(self.player:getCards("he")) do
		if not self:isValuableCard(c) then
			table.insert(to_dis, tostring(c:getEffectiveId()))
			if #to_dis == 2 then
				return "@ThMumiCard=" .. table.concat(to_dis, "+")
			end
		end
	end
	return "."
end

--妄狱：每当你使用【杀】造成一次伤害后，你可以将此【杀】交给一名手牌数不大于体力上限的其他角色；每当其他角色使用【杀】造成一次伤害后，若你的手牌数不大于体力上限，其可以将此【杀】交给你。
sgs.ai_skill_playerchosen.thwangyu = function(self, targets)
	local damage = self.player:getTag("ThWangyuSlash"):toDamage()
	local card = damage.card
	local cards = {}
	if card:isVirtualCard() then
		for _, id in sgs.qlist(card:getSubcards()) do
			table.insert(cards, sgs.Sanguosha:getCard(id))
		end
	else
		cards = { card }
	end
	if #cards == 0 then return nil end
	local _, target = self:getCardNeedPlayer(cards, sgs.QList2Table(targets), false)
	return target
end

sgs.ai_playerchosen_intention.thwangyu = -40

--光蚀：每当你受到一次伤害后，你可以摸或弃置一张牌，然后展示你全部的手牌，若红色牌数大于黑色牌，你摸一张牌；若黑色牌数大于红色牌，你弃置伤害来源一张牌。
sgs.ai_skill_invoke.thguangshi = function(self, data)
	local damage = data:toDamage()
	if not damage.from or self:isEnemy(damage.from) then
		return true
	else
		local red, black = 0, 0
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if c:isRed() then
				red = red + 1
			elseif c:isBlack() then
				black = black + 1
			end
		end
		if red > black + 1 then
			return true
		elseif red == black then
			return true
		end
	end
	return false
end

sgs.ai_skill_cardask["@thguangshi"] = function(self, data)
	local red, black = 0, 0
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if c:isRed() then
			red = red + 1
		elseif c:isBlack() then
			black = black + 1
		end
	end
	if math.abs(red - black) >= 2 then return "." end
	local damage = data:toDamage()
	if damage.from and self:isFriend(damage.from) then
		if red == black then
			local cards = sgs.QList2Table(self.player:getHandcards())
			self:sortByKeepValue(cards)
			for _, c in ipairs(cards) do
				if c:isBlack() then
					return "$" .. c:getEffectiveId()
				end
			end
		end
	end
	return "."
end

sgs.ai_choicemade_filter.cardChosen.thguangshi = sgs.ai_choicemade_filter.cardChosen.snatch

--隼武：当一张武器牌或防具牌置入弃牌堆时，若你的人物牌上没有牌，你可以将之置于你的人物牌上。
sgs.ai_skill_invoke.thsunwu = true

--僚感：出牌阶段，你可以令一名角色获得你的人物牌上的牌。若如此做，该角色视为拥有技能“遐攻”和“净涅”，直到你的下个回合开始。
local thliaogan_skill = {}
thliaogan_skill.name = "thliaogan"
table.insert(sgs.ai_skills, thliaogan_skill)
thliaogan_skill.getTurnUseCard = function(self)
	if self.player:getPile("frost"):isEmpty() then return end
	return sgs.Card_Parse("@ThLiaoganCard=.")
end

sgs.ai_skill_use_func.ThLiaoganCard = function(card, use, self)
	local isGoodThLiaoganTarget = function(target, equip)
		local same = self:getSameEquip(equip, target)
		if not same then
			if (not target:getWeapon() and not target:hasSkill("thxiagong")) or (not target:getArmor() and not target:hasSkill("ikjingnie")) then
				return true
			end
		end
		return target:isWounded() and (equip:isKindOf("SilverLion") or (same and same:isKindOf("SilverLion")))
	end
	local euqip = sgs.Sanguosha:getCard(self.player:getPile("frost"):first())
	local same = self:getSameEquip(euqip)
	if same and isGoodThLiaoganTarget(self.player, euqip) then
		use.card = card
		if use.to then
			use.to:append(self.player)
		end
		return
	end
	if euqip:isKindOf("Weapon") then
		self:sort(self.friends, "handcard")
		self.friends = sgs.reverse(self.friends)
		for _, p in ipairs(self.friends) do
			if isGoodThLiaoganTarget(p, euqip) then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	else
		self:sort(self.friends, "defense")
		for _, p in ipairs(self.friends) do
			if isGoodThLiaoganTarget(p, euqip) then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
end

sgs.ai_use_priority.ThLiaoganCard = 9
sgs.ai_card_intention.ThLiaoganCard = -50

--戋月：摸牌阶段或出牌阶段开始时，你可以视为使用一张【心网密葬】，该牌生效后，你可以选择一项：令目标角色获得被弃置的牌，且其此回合不能使用或打出该牌；或结束当前阶段。
sgs.ai_skill_use["@@thjianyue"] = function(self, prompt, method)
	local dismantlement = sgs.cloneCard("dismantlement")
	dismantlement:setSkillName("thjianyue")
	local use = { isDummy = true, to = sgs.SPlayerList() }
	self:useCardSnatchOrDismantlement(dismantlement, use)
	if use.card and not use.to:isEmpty() then
		local str = {}
		for _, p in sgs.qlist(use.to) do
			table.insert(str, p:objectName())
		end
		return "dismantlement:thjianyue[no_suit:0]=.->" .. table.concat(str, "+")
	end
	return "."
end

sgs.ai_skill_choice.thjianyue = function(self, choices, data)
	local target = data:toPlayer()
	if self:isFriend(target) and string.find(choices, "obtain") then
		return "obtain"
	end
	if self:isEnemy(target) then
		if self.player:getPhase() == sgs.Player_Draw and self:getOverflow() >= 0 then
			if self.player:isSkipped(sgs.Player_Play) then
				return "skip"
			end
		elseif self.player:getPhase() == sgs.Player_Play then
			local use = { isDummy = true }
			self:activate(use)
			if not use.card then
				return "skip"
			end
		end
	end
	return "obtain"
end

--幻鉴：一名其他角色的回合开始时，若你和其皆有手牌，你可以令其与你各将一张手牌面朝上置于你的人物牌上；此回合结束时，你获得你人物牌上的一张牌，然后该角色获得另一张牌。
sgs.ai_skill_invoke.thhuanjian = function(self, data)
	local target = data:toPlayer()
	local use = nil
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if self:isValuableCard(c) then continue end
		use = c
		break
	end
	if not use then return false end
	if self:isEnemy(target) then
		return true
	else
		return self:getOverflow(target) > 0
	end
	return false
end

sgs.ai_skill_cardask["@thhuanjian-ask"] = function(self)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards, true)
	return cards[1]
end

sgs.ai_skill_cardask["@thhuanjian-self"] = function(self)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	return cards[1]
end

--深秘：你可以将你人物牌上颜色相同的两张牌当【闪】使用或打出；或将你人物牌上颜色不同的两张牌当【三粒天滴】使用。
sgs.ai_cardsview_valuable.thshenmi = function(self, class_name, player)
	local ids = player:getPile("note")
	if ids:length() < 2 then return nil end
	local card1 = sgs.Sanguosha:getCard(ids:first())
	local card2 = sgs.Sanguosha:getCard(ids:last())
	if card1:sameColorWith(card2) then
		if class_name == "Jink" then
			return ("jink:thshenmi[to_be_decided:0]=%d+%d"):format(ids:first(), ids:last())
		end
	else
		if class_name == "Nullification" then
			return ("nullification:thshenmi[to_be_decided:0]=%d+%d"):format(ids:first(), ids:last())
		end
	end
	return nil
end

--暮狱：出牌阶段限一次，你可以弃置一张牌，然后令一名其他角色选择一项：将一张牌置于你的人物牌上；或弃置两张牌。当你人物牌上有牌时，你可以将红色牌当【闪】、黑色牌当【三粒天滴】使用或打出。准备阶段开始时，你须获得你人物牌上的全部的牌。
local thmuyu_skill = {}
thmuyu_skill.name = "thmuyu"
table.insert(sgs.ai_skills, thmuyu_skill)
thmuyu_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThMuyuCard") or not self.player:canDiscard(self.player, "h") then return end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	return sgs.Card_Parse("@ThMuyuCard=" .. cards[1]:getEffectiveId())
end

sgs.ai_skill_use_func.ThMuyuCard = function(card, use, self)
	local target = self:findPlayerToDiscard("he", false, true)
	if target then
		use.card = card
		if use.to then
			use.to:append(target)
		end
	end
end

sgs.ai_skill_cardask["@thmuyu-put"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	if pattern == "..!" or self:isFriend(target) then
		return "$" .. cards[1]:getEffectiveId()
	end
	local ret = self:askForDiscard("", 2, 2, false, true)
	if #ret == 2 then
		for _, id in ipairs(ret) do
			if self:isValuableCard(sgs.Sanguosha:getCard(id)) then
				return "$" .. cards[1]:getEffectiveId()
			end
		end
		return "."
	end
	return "$" .. cards[1]:getEffectiveId()
end

sgs.ai_view_as.thmuyu = function(card, player, card_place, class_name)
	if not player:getPile("prison"):isEmpty() then
		if card_place == sgs.Player_PlaceHand or card_place == sgs.Player_PlaceEquip then
			local suit = card:getSuitString()
			local point = card:getNumber()
			local id = card:getEffectiveId()
			if class_name == "Jink" and card:isRed() then
				return string.format("jink:thmuyu[%s:%d]=%d", suit, point, id)
			elseif class_name == "Nullification" and card:isBlack() then
				return string.format("nullification:thmuyu[%s:%d]=%d", suit, point, id)
			end
		end
	end
end

sgs.ai_use_priority.ThMuyuCard = 6

--逆秽：出牌阶段限一次，你可以摸三张牌，然后弃置一张牌。若如此做，交换此技能描述中的“摸”与“弃置”。
local thnihui_skill = {}
thnihui_skill.name = "thnihui"
table.insert(sgs.ai_skills, thnihui_skill)
thnihui_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThNihuiCard") or self.player:hasUsed("ThNihuiEditCard") then return end
	return sgs.Card_Parse("@ThNihuiCard=.")
end

sgs.ai_skill_use_func.ThNihuiCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.ThNihuiCard = 6.8

local thnihui_edit_skill = {}
thnihui_edit_skill.name = "thnihui-edit"
table.insert(sgs.ai_skills, thnihui_edit_skill)
thnihui_edit_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThNihuiCard") or self.player:hasUsed("ThNihuiEditCard") then return end
	local cards = {}
	for _, c in sgs.qlist(self.player:getCards("he")) do
		if not self:isValuableCard(c) then
			table.insert(cards, c)
		end
	end
	if #cards < 3 then return end
	self:sortByKeepValue(cards)
	return sgs.Card_Parse(("@ThNihuiEditCard=%d+%d+%d"):format(cards[1]:getEffectiveId(), cards[2]:getEffectiveId(), cards[3]:getEffectiveId()))
end

sgs.ai_skill_use_func.ThNihuiEditCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.ThNihuiEditCard = -6.8

--弹冠：结束阶段开始时，你可以与一名其他角色拼点，然后你们获得对方拼点的牌。若这两张牌皆为红色，你可以回复1点体力；若皆为黑色，你可以令该角色摸两张牌。
sgs.ai_skill_playerchosen.thtanguan = function(self, targets)
	self:sort(self.enemies, "handcard")
	for _, p in ipairs(self.enemies) do
		if self:isWeak(p) and p:getHandcardNum() == 1 and not p:getHandcards():first():hasFlag("visible") then
			for _, c in sgs.qlist(self.player:getHandcards()) do
				if self:isValuableCard(c, p) then continue end
				self.thtanguan_card = c:getEffectiveId()
				return p
			end
		end
	end
	if #self.friends_noself == 0 then return nil end
	if self.player:isWounded() then
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if c:isRed() then
				self:sort(self.friends_noself, "handcard")
				self.friends_noself = sgs.reverse(self.friends_noself)
				for _, p in ipairs(self.friends_noself) do
					if p:isKongcheng() then continue end
					local flag = ("%s_%s_%s"):format("visible", self.player:objectName(), p:objectName())
					for _, cc in sgs.qlist(p:getHandcards()) do
						if cc:isRed() and (cc:hasFlag("visible") or cc:hasFlag(flag)) then
							self.thtanguan_card = c:getEffectiveId()
							return p
						end
					end
				end
				if not self.friends_noself[1]:isKongcheng() then
					self.thtanguan_card = c:getEffectiveId()
					return self.friends_noself[1]
				end
				break
			end
		end
	end
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if c:isBlack() then
			self:sort(self.friends_noself, "handcard")
			for _, p in ipairs(self.friends_noself) do
				if p:isKongcheng() then continue end
				local flag = ("%s_%s_%s"):format("visible", self.player:objectName(), p:objectName())
				for _, cc in sgs.qlist(p:getHandcards()) do
					if cc:isBlack() and (cc:hasFlag("visible") or cc:hasFlag(flag)) then
						self.thtanguan_card = c:getEffectiveId()
						return p
					end
				end
			end
			if not self.friends_noself[#self.friends_noself]:isKongcheng() then
				self.thtanguan_card = c:getEffectiveId()
				return self.friends_noself[#self.friends_noself]
			end
			break
		end
	end
	return nil
end

sgs.ai_skill_pindian.thtanguan = function(minusecard, self, requestor, maxcard, mincard)
	if self.player:getHandcardNum() == 1 then
		return self.player:getHandcards():first()
	end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	local black, red = nil, nil
	for _, c in ipairs(cards) do
		if not black and c:isBlack() then
			black = c
		elseif not red and c:isRed() then
			red = c
		end
	end
	if self:isFriend(requestor) then
		if requestor:isWounded() then
			return red or black or cards[#cards]
		else
			return black or cards[#cards]
		end
	else
		return black or minusecard
	end
end

sgs.ai_skill_invoke.thtanguan = function(self, data)
	local str = data:toString()
	if str == "recover" then
		return true
	else
		local p_list = str:split(":")
		local target = self.room:findPlayer(p_list[#p_list])
		if target then
			return self:isFriend(target)
		end
	end
	return false
end

--爰粹：锁定技，准备阶段开始时，若你未受伤，你失去1点体力；否则，你回复1点体力。
--无

--秽狂：每当你失去体力时，你可以摸两张牌；每当你回复体力时，你可以弃置两张牌。你可以展示你以此法摸或弃置的牌，若颜色相同，视为你使用一张【百鬼夜行】。
sgs.ai_skill_invoke.thhuikuang = true

sgs.ai_skill_discard.thhuikuang = function(self)
	local sa = sgs.cloneCard("savage_assault")
	local use = { isDummy = true }
	self:useTrickCard(sa, use)
	if not use.card then return {} end
	local ret = self:askForDiscard("", 1, 1, false, true)
	if #ret == 1 then
		local c = sgs.Sanguosha:getCard(ret[1])
		if not self:isValuableCard(c) then
			local pattern = ".|"
			if c:isRed() then
				pattern = pattern .. "red"
			else
				pattern = pattern .. "black"
			end
			local ret2 = self:askForDiscard("", 2, 2, false, true, pattern)
			if #ret2 == 2 then
				if not self:isValuableCard(sgs.Sanguosha:getCard(ret2[2])) then
					return ret2
				end
			end
		end
	end
	return {}
end

sgs.ai_skill_invoke.thhuikuang_sa = function(self, data)
	local str = data:toString()
	local sa = sgs.cloneCard("savage_assault")
	local use = { isDummy = true }
	self:useTrickCard(sa, use)
	if not use.card then return false end
	if str == "use" then
		return true
	elseif str == "show" then
		local cards = self.player:getHandcards()
		local n = cards:length()
		if n < 2 then return false end
		local a = cards:at(n - 1)
		local b = cards:at(n - 2)
		if a:sameColorWith(b) then
			return true
		end
	end
	return false
end
