sgs.ai_skill_choice.thzhiji = function(self, choices, data)
	if (self:isWeak() or self:needKongcheng(self.player, true)) and string.find(choices, "recover") then
		return "recover"
	else
		return "draw"
	end
end

local thjiyi_skill = {}
thjiyi_skill.name = "thjiyi"
table.insert(sgs.ai_skills, thjiyi_skill)
thjiyi_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThJiyiCard") then
		return sgs.Card_Parse("@ThJiyiCard=.")
	end
end

sgs.ai_skill_use_func.ThJiyiCard = function(card, use, self)
	for _, p in ipairs(self.enemies) do
		if self:isWeak(p) and getKnownCard(p, self.player, "TrickCard") == 0 then
			use.card = sgs.Card_Parse("@ThJiyiCard=.")
			if use.to then
				use.to:append(p)
			end
			return
		end
	end --weak enemy

	for _, p in ipairs(self.friends_noself) do
		if p:getPile("wooden_ox"):isEmpty() and getKnownCard(p, self.player, "TrickCard") > 0 then
			if not self:needKongcheng(self.player, true) then
				use.card = sgs.Card_Parse("@ThJiyiCard=.")
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end --friend who has known trick

	for _, p in ipairs(self.enemies) do
		if getKnownCard(p, self.player, "TrickCard") == 0 then
			use.card = sgs.Card_Parse("@ThJiyiCard=.")
			if use.to then
				use.to:append(p)
			end
			return
		end
	end --enemys
end

sgs.ai_cardshow.thjiyi = function(self, requestor)
	local trick = nil
	local cards = sgs.QList2Table(self.player:getHandcards())
	for _, cd in ipairs(cards) do
		if cd:isKindOf("TrickCard") then
			trick = cd
			break
		end
	end
	if trick then
		return trick
	end
	self:sortByKeepValue(cards)
	return cards[1]
end

sgs.ai_skill_cardask["@thjiyi"] = function(self, data, pattern, target)
	if self:isFriend(target) then
		return "."
	end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, cd in ipairs(cards) do
		if cd:isKindOf("TrickCard") then
			return "$"..cd:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_card_intention.ThJiyiCard = function(self, card, from, to)
	if getKnownCard(to[1], from, "TrickCard") > 0 then
		sgs.updateIntention(from, to[1], -50)
		return
	end
	sgs.updateIntention(from, to[1], 50)
end

--thhuadi @to_do

sgs.ai_skill_invoke.thjilanwen = function(self, data)
	for _, target in sgs.qlist(self.room:getAllPlayers()) do
		if self:isFriend(target) then
			if (target:hasSkill("ikcangyou") and not target:getEquips():isEmpty()) or self:needToThrowArmor(target) then
				return true
			end
			if target:containsTrick("indulgence") or target:containsTrick("supply_shortage") then
				return true
			end
		end
		if self:isEnemy(target) then
			if target:hasSkills("ikyindie+ikguiyue") and target:getPhase() == sgs.Player_NotActive then return false end
			if not target:getEquips():isEmpty() then
				return true
			else
				return false
			end
		end
	end
	return true
end

sgs.ai_skill_playerchosen.thjilanwen = function(self, targets)
	local judge = self.player:getTag("ThJilanwenJudge"):toJudge()
	local suit = tonumber(judge.pattern)
	for _, p in ipairs(self.friends_noself) do
		if not targets:contains(p) then
			continue
		end
		if p:hasSkill("ikcangyou") then
			for _, cd in sgs.qlist(p:getEquips()) do
				if cd:getSuit() ~= suit then
					return p
				end
			end
		end
		if self:needToThrowArmor(p) then
			if p:getArmor() and p:getArmor():getSuit() ~= suit then
				return p
			end
		end
		for _, cd in sgs.qlist(p:getJudgingArea()) do
			if cd:isKindOf("Indulgence") or cd:isKindOf("SupplyShortage") then
				return p
			end
		end
	end
	for _, p in ipairs(self.enemies) do
		if not targets:contains(p) then
			continue
		end
		if p:hasSkills("ikyindie+ikguiyue") and p:getPhase() == sgs.Player_NotActive then
			continue
		end
		local equips = sgs.IntList()
		for _, cd in sgs.qlist(p:getEquips()) do
			equips:append(cd:getEffectiveId())
		end
		if p:getArmor() and self:needToThrowArmor(p) then
			equips:removeOne(p:getArmor():getEffectiveId())
		end
		for _, id in sgs.qlist(equips) do
			if sgs.Sanguosha:getCard(id):getSuit() ~= suit then
				return p
			end
		end
	end
	return nil
end