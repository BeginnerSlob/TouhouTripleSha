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

local thnianke_skill = {}
thnianke_skill.name = "thnianke"
table.insert(sgs.ai_skills, thnianke_skill)
thnianke_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThNiankeCard") then
		return nil
	end
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)

	local card

	self:sortByUseValue(cards, true)

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Jink") then
			card = acard
			break
		end
	end

	if not card then
		return nil
	end

	local card_id = card:getEffectiveId()
	local card_str = "@ThNiankeCard=" .. card_id
	local skillcard = sgs.Card_Parse(card_str)

	assert(skillcard)
	return skillcard
end

sgs.ai_skill_use_func.ThNiankeCard = function(card, use, self)
	local targets_red = {}
	local targets = {}
	for _, friend in ipairs(self.friends_noself) do
		if (friend:isKongcheng() and not self:needKongcheng(friend, true)) or getKnownCard(friend, self.player, "red") == friend:getHandcardNum() then
			table.insert(targets_red, friend)
		else
			table.insert(targets, friend)
		end
	end
	if #targets_red > 0 then
		use.card = card
		self:sort(targets_red, "defense")
		if use.to then
			use.to:append(targets_red[1])
		end
	end
	if self:getCardsNum("Jink", "h") <= 1 then
		return "."
	end
	if #targets > 0 then
		use.card = card
		self:sort(targets, "defense")
		if use.to then
			use.to:append(targets[1])
		end
	end
end

sgs.ai_card_intention.ThNiankeCard = -80

sgs.ai_skill_playerchosen.thjilan = function(self, targets)
	for _, p in ipairs(self.friends) do
		if p:getArmor() and self:needToThrowArmor(p) and p:canDiscard(p, p:getArmor():getEffectiveId()) and p:getLostHp() <= 1 then
			return p
		end
	end
	local enemies = {}
	for _, p in ipairs(self.enemies) do
		if p:canDiscard(p, "he") then
			table.insert(enemies, p)
		end
	end
	self:sort(enemies, "losthp")
	enemies = sgs.reverse(enemies)
	for _, enemy in ipairs(enemies) do
		if self:isWeak(enemy) then
			return enemy
		end
	end
	return enemies[1]
end

sgs.ai_skill_invoke.thwangshou = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		return (target:hasSkill("ikcangyou") and not target:getEquips():isEmpty()) or self:needToThrowArmor(target)
	end
	return self:isEnemy(target)
end

sgs.ai_skill_cardask["@thzhanye"] = function(self, data, pattern, target)
	for _, slash in ipairs(self:getCards("Slash")) do
		if self:isFriend(target) and self:slashIsEffective(slash, target) then
			if self:findLeijiTarget(target, 50, self.player) then return slash:toString() end
			if self:getDamagedEffects(target, self.player, true) then return slash:toString() end
		end

		local nature = sgs.DamageStruct_Normal
		if slash:isKindOf("FireSlash") then nature = sgs.DamageStruct_Fire
		elseif slash:isKindOf("ThunderSlash") then nature = sgs.DamageStruct_Thunder end
		if self:isEnemy(target) and self:slashIsEffective(slash, target) and self:canAttack(target, self.player, nature)
			and not self:getDamagedEffects(target, self.player, true) and not self:findLeijiTarget(target, 50, self.player) then
			return slash:toString()
		end
	end
	return "."
end

local thenan_skill = {}
thenan_skill.name = "thenan"
table.insert(sgs.ai_skills, thenan_skill)
thenan_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThEnanCard") then
		return nil
	end

	local skillcard = sgs.Card_Parse("@ThEnanCard=.")
	assert(skillcard)
	return skillcard
end

sgs.ai_skill_use_func.ThEnanCard = function(card, use, self)
	if self.player:getHandcardNum() <= 1 and self.player:getHp() == 1 and self.player:getMaxHp() > 4 and getCardsNum("Peach", self.player) + getCardsNum("Analeptic", self.player) > 0 then
		use.card = card
		if use.to then
			use.to:append(self.player)
		end
		return
	end
	for _, enemy in ipairs(self.enemies) do
		if self.player:inMyAttackRange(enemy) and self:isWeak(enemy) then
			use.card = card
			if use.to then
				use.to:append(enemy)
			end
			return
		end
	end
	return "."	
end

sgs.ai_card_intention.ThEnanCard = 80

sgs.ai_skill_choice.thbeiyun = function(self, choices, data)
	if string.find(choices, "get") then
		return "get"
	end
	local beiyun_ids = data:toIntList()
	local red, black, not_red, not_black = false, false, false, false
	for _, id in sgs.qlist(beiyun_ids) do
		local card = sgs.Sanguosha:getCard(id)
		if card:isRed() then
			if not red and card:getSuit() == sgs.Card_Diamond then
				red = true
			end
			if card:isKindOf("Peach") or card:isKindOf("Analeptic") then
				not_red = true
			end
		elseif card:isBlack() then
			if not black then
				black = true
			end
			if card:isKindOf("Peach") or card:isKindOf("Analeptic") then
				not_black = true
			end
		end
	end
	if not not_red and red then
		return "red"
	end
	if getCardsNum("Peach", self.player) + getCardsNum("Analeptic", self.player) >= 1 - self.player:getHp() then -- i'm safe
		return black and "black" or "cancel"
	else
		return not not_black and black and "black" or "cancel"
	end
	return "cancel"
end
