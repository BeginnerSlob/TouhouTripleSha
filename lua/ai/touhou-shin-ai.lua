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
