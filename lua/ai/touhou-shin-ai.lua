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