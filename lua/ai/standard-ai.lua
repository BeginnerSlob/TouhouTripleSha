sgs.ai_skill_choice.jianxiong = function(self, choices, data)
	local function getChoice(id)
		local card = sgs.Sanguosha:getCard(id)
		if card:isKindOf("Lightning") then
			local dummy = { isDummy = true }
			self:useCardLightning(card, dummy)
			return (dummy.card and dummy.card:isKindOf("Lightning")) and "obtain" or "draw"
		elseif card:isKindOf("Slash") then
			if self:getCardsNum("Slash") >= 1
				and ((not self.player:hasSkills("ikyipao|xianzhen") and not self.player:hasWeapon("crossbow") and not self.player:hasWeapon("vs_crossbow"))
					or self:willSkipPlayPhase()) then
				return "draw"
			end
			return "obtain"
		elseif self:willSkipPlayPhase() or self:isWeak() then
			return self:isValuableCard(card) and "obtain" or "draw"
		else
			return "obtain"
		end
	end

	local choice_table = choices:split("+")
	if not table.contains(choice_table, "obtain") then
		return self:needKongcheng(self.player, true) and "cancel" or "draw"
	else
		local damage = data:toDamage()
		if self.player:isKongcheng() and damage.from and damage.from:isAlive() and damage.from:getPhase() == sgs.Player_Play
			and damage.from:hasSkills("longdan+chongzhen") and self:slashIsAvailable(damage.from)
			and card:isKindOf("Jink") and getKnownCard(damage.from, self.player, "Jink", false) > 0 then
			return getKnownCard(self.player, self.player, "Jink", false) >= 2 and "cancel" or "draw"
		end
		local len = damage.card:isVirtualCard() and damage.card:subcardsLength() or 1
		if len >= 3 then return "obtain"
		elseif len == 2 then
			for _, card in sgs.qlist(damage.card:getSubcards()) do
				if self:isValuableCard(card) then return "obtain" end
			end
		end
		if self:needKongcheng(self.player, true) then return "cancel" end
		if len == 2 then
			for _, card in sgs.qlist(damage.card:getSubcards()) do
				local id = card:getEffectiveId()
				if getChoice(id) == "obtain" then return "obtain" end
			end
			return "draw"
		else
			local id = damage.card:getEffectiveId()
			return getChoice(id)
		end
	end
	return "cancel"
end

sgs.ai_skill_invoke.yiji = function(self)
	local sb_diaochan = self.room:getCurrent()
	if sb_diaochan and sb_diaochan:hasSkill("thjinlu") and not sb_diaochan:hasUsed("ThJinluCard") and not self:isFriend(sb_diaochan) and sb_diaochan:getPhase() == sgs.Player_Play then
		return #self.friends_noself > 0
	end
	return true
end

sgs.ai_skill_use["@@yiji"] = function(self, prompt)
	self.yiji = {}
	if self.player:getHandcardNum() <= 2 then return "." end
	local friends = {}
	for _, friend in ipairs(self.friends_noself) do
		if not self:willSkipDrawPhase(friend) then table.insert(friends, friend) end
	end
	if #friends == 0 then return "." end
	local friend_table = {}
	local cards = sgs.QList2Table(self.player:getHandcards())
	while true do
		local card, friend = self:getCardNeedPlayer(cards, friends, false)
		if not (card and friend) then break end
		if not self.yiji[friend:objectName()] then
			self.yiji[friend:objectName()] = { card:getEffectiveId() }
		else
			table.insert(self.yiji[friend:objectName()], card:getEffectiveId())
		end
		cards = self:resetCards(cards, card)
		if #self.yiji[friend:objectName()] == 2 then
			table.insert(friend_table, friend:objectName())
			local temp = {}
			for _, f in ipairs(friends) do
				if f:objectName() ~= friend:objectName() then table.insert(temp, f) end
			end
			friends = temp
		end
		if #cards == 0 or #friend_table == 2 then break end
	end
	if #friend_table > 0 then return "@YijiCard=.->" .. table.concat(friend_table, "+") end
	return "."
end

sgs.ai_card_intention.YijiCard = -80

sgs.ai_skill_discard.yiji = function(self)
	local target = self.player:getTag("YijiTarget"):toString()
	return self.yiji[target]
end

function SmartAI:willSkipPlayPhase(player, no_null)
	local player = player or self.player
	if player:isSkipped(sgs.Player_Play) then return true end

	local fuhuanghou = self.room:findPlayerBySkillName("noszhuikong")
	if fuhuanghou and fuhuanghou:objectName() ~= player:objectName() and self:isEnemy(player, fuhuanghou)
		and fuhuanghou:isWounded() and fuhuanghou:getHandcardNum() > 1 and not player:isKongcheng() and not self:isWeak(fuhuanghou) then
		local max_card = self:getMaxCard(fuhuanghou)
		local player_max_card = self:getMaxCard(player)
		local max_number = max_card and max_card:getNumber() or 0
		local player_max_number = player_max_card and player_max_card:getNumber() or 0
		if fuhuanghou:hasSkill("yingyang") then max_number = math.max(max_number + 3, 13) end
		if player:hasSkill("yingyang") then player_max_number = math.max(player_max_number + 3, 13) end
		if (max_card and player_max_card and max_number > player_max_number) or max_number >= 12 then return true end
	end

	if self.player:containsTrick("YanxiaoCard") or self.player:hasSkills("keji|thanbing") or (self.player:hasSkill("qiaobian") and not self.player:isKongcheng()) then return false end
	local friend_null, friend_snatch_dismantlement = 0, 0
	if self.player:getPhase() == sgs.Player_Play and self.player:objectName() ~= player:objectName() and self:isFriend(player) then
		for _, hcard in sgs.qlist(self.player:getCards("he")) do
			local objectName
			if isCard("Snatch", hcard, self.player) then objectName = "snatch"
			elseif isCard("Dismantlement", hcard, self.player) then objectName = "dismantlement" end
			if objectName then
				local trick = sgs.cloneCard(objectName, hcard:getSuit(), hcard:getNumber())
				local targets = self:exclude({ player }, trick)
				if #targets > 0 then friend_snatch_dismantlement = friend_snatch_dismantlement + 1 end
			end
		end
	end
	if not no_null then
		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if self:isFriend(p) then friend_null = friend_null + getCardsNum("Nullification", p, self.player) end
			if self:isEnemy(p) then friend_null = friend_null - getCardsNum("Nullification", p, self.player) end
		end
		friend_null = friend_null + self:getCardsNum("Nullification")
	end
	return self.player:containsTrick("indulgence") and friend_null + friend_snatch_dismantlement <= 1
end

function SmartAI:willSkipDrawPhase(player, no_null)
	local player = player or self.player
	if player:isSkipped(sgs.Player_Draw) then return true end
	if player:hasSkill("thzhanying") then return true end
	if self.player:containsTrick("YanxiaoCard") or (self.player:hasSkill("qiaobian") and not self.player:isKongcheng()) then return false end
	local friend_null, friend_snatch_dismantlement = 0, 0
	if self.player:getPhase() == sgs.Player_Play and self.player:objectName() ~= player:objectName() and self:isFriend(player) then
		for _, hcard in sgs.qlist(self.player:getCards("he")) do
			local objectName
			if isCard("Snatch", hcard, self.player) then objectName = "snatch"
			elseif isCard("Dismantlement", hcard, self.player) then objectName = "dismantlement" end
			if objectName then
				local trick = sgs.cloneCard(objectName, hcard:getSuit(), hcard:getNumber())
				local targets = self:exclude({ player }, trick)
				if #targets > 0 then friend_snatch_dismantlement = friend_snatch_dismantlement + 1 end
			end
		end
	end
	if not no_null then
		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if self:isFriend(p) then friend_null = friend_null + getCardsNum("Nullification", p, self.player) end
			if self:isEnemy(p) then friend_null = friend_null - getCardsNum("Nullification", p, self.player) end
		end
		friend_null = friend_null + self:getCardsNum("Nullification")
	end
	return self.player:containsTrick("supply_shortage") and friend_null + friend_snatch_dismantlement <= 1
end

local yijue_skill = {}
yijue_skill.name = "yijue"
table.insert(sgs.ai_skills, yijue_skill)
yijue_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("YijueCard") and not self.player:isKongcheng() then return sgs.Card_Parse("@YijueCard=.") end
end

sgs.ai_skill_use_func.YijueCard = function(card, use, self)
	self:sort(self.enemies, "handcard")
	local max_card = self:getMaxCard()
	if not max_card then return end
	local max_point = max_card:getNumber()
	if self.player:hasSkill("yingyang") then max_point = math.min(max_point + 3, 13) end
	if self.player:hasSkill("ikjingyou") and self.player:getHandcardNum() == 1 then
		for _, enemy in ipairs(self.enemies) do
			if not enemy:isKongcheng() and self:hasLoseHandcardEffective(enemy) and not (enemy:hasSkills("ikyindie+ikguiyue") and enemy:getHandcardNum() > 2) then
				sgs.ai_use_priority.YijueCard = 1.2
				self.tianyi_card = max_card:getId()
				use.card = sgs.Card_Parse("@YijueCard=.")
				if use.to then use.to:append(enemy) end
				return
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if enemy:hasFlag("AI_HuangtianPindian") and enemy:getHandcardNum() == 1 then
			sgs.ai_use_priority.YijueCard = 7.2
			self.yijue_card = max_card:getId()
			use.card = sgs.Card_Parse("@YijueCard=.")
			if use.to then
				use.to:append(enemy)
				enemy:setFlags("-AI_HuangtianPindian")
			end
			return
		end
	end

	local zhugeliang = self.room:findPlayerBySkillName("ikjingyou")

	sgs.ai_use_priority.YijueCard = 7.2
	self:sort(self.enemies)
	self.enemies = sgs.reverse(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("ikjingyou") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			local enemy_max_card = self:getMaxCard(enemy)
			local enemy_max_point = enemy_max_card and enemy_max_card:getNumber() or 100
			if enemy_max_card and enemy:hasSkill("yingyang") then enemy_max_point = math.min(enemy_max_point + 3, 13) end
			if max_point > enemy_max_point then
				self.yijue_card = max_card:getId()
				use.card = sgs.Card_Parse("@YijueCard=.")
				if use.to then use.to:append(enemy) end
				return
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("ikjingyou") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			if max_point >= 10 then
				self.yijue_card = max_card:getId()
				use.card = sgs.Card_Parse("@YijueCard=.")
				if use.to then use.to:append(enemy) end
				return
			end
		end
	end

	sgs.ai_use_priority.YijueCard = 1.2
	local min_card = self:getMinCard()
	if not min_card then return end
	local min_point = min_card:getNumber()
	if self.player:hasSkill("yingyang") then min_point = math.max(min_point - 3, 1) end

	local wounded_friends = self:getWoundedFriend()
	if #wounded_friends > 0 then
		for _, wounded in ipairs(wounded_friends) do
			if wounded:getHandcardNum() > 1 and wounded:getLostHp() / wounded:getMaxHp() >= 0.3 then
				local w_max_card = self:getMaxCard(wounded)
				local w_max_number = w_max_card and w_max_card:getNumber() or 0
				if w_max_card and wounded:hasSkill("yingyang") then w_max_number = math.min(w_max_number + 3, 13) end
				if (w_max_card and w_max_number >= min_point) or min_point <= 4 then
					self.yijue_card = min_card:getId()
					use.card = sgs.Card_Parse("@YijueCard=.")
					if use.to then use.to:append(wounded) end
					return
				end
			end
		end
	end

	if zhugeliang and self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 1 and zhugeliang:objectName() ~= self.player:objectName() then
		if min_point <= 4 then
			self.yijue_card = min_card:getId()
			use.card = sgs.Card_Parse("@YijueCard=.")
			if use.to then use.to:append(zhugeliang) end
			return
		end
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByUseValue(cards, true)
		if self:getEnemyNumBySeat(self.player, zhugeliang) >= 1 then
			if isCard("Jink", cards[1], self.player) and self:getCardsNum("Jink") == 1 then return end
			self.yijue_card = cards[1]:getId()
			use.card = sgs.Card_Parse("@YijueCard=.")
			if use.to then use.to:append(zhugeliang) end
			return
		end
	end
end

function sgs.ai_skill_pindian.yijue(minusecard, self, requestor)
	if requestor:getHandcardNum() == 1 then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		return cards[1]
	end
	return self:getMaxCard()
end

sgs.ai_cardneed.yijue = function(to, card, self)
	local cards = to:getHandcards()
	local has_big = false
	for _, c in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s", "visible", self.room:getCurrent():objectName(), to:objectName())
		if c:hasFlag("visible") or c:hasFlag(flag) then
			if c:getNumber() > 10 then
				has_big = true
				break
			end
		end
	end
	return not has_big and card:getNumber() > 10
end

sgs.ai_card_intention.YijueCard = 0
sgs.ai_use_value.YijueCard = 8.5

sgs.ai_choicemade_filter.skillChoice.yijue = function(self, player, promptlist)
	local choice = promptlist[#promptlist]
	local intention = (choice == "recover") and -30 or 30
	local target = nil
	for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
		if p:hasFlag("YijueTarget") then
			target = p
			break
		end
	end
	if not target then return end
	sgs.updateIntention(player, target, intention)
end

function sgs.ai_cardneed.ikyipao(to, card, self)
	local cards = to:getHandcards()
	local has_weapon = to:getWeapon() and not to:getWeapon():isKindOf("Crossbow")
	local slash_num = 0
	for _, c in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s", "visible", self.room:getCurrent():objectName(), to:objectName())
		if c:hasFlag("visible") or c:hasFlag(flag) then
			if c:isKindOf("Weapon") and not c:isKindOf("Crossbow") then
				has_weapon = true
			end
			if c:isKindOf("Slash") then slash_num = slash_num + 1 end
		end
	end
	if not has_weapon then
		return card:isKindOf("Weapon") and not card:isKindOf("Crossbow")
	else
		return to:hasWeapon("spear") or card:isKindOf("Slash") or (slash_num > 1 and card:isKindOf("Analeptic"))
	end
end

sgs.ikyipao_keep_value = {
	Peach = 6,
	Analeptic = 5.8,
	Jink = 5.7,
	FireSlash = 5.6,
	Slash = 5.4,
	ThunderSlash = 5.5,
	ExNihilo = 4.7
}

sgs.ai_skill_invoke.tishen = function(self, data)
	local x = data:toInt()
	return x >= 2 and not self:willSkipPlayPhase()
end

local longdan_skill = {}
longdan_skill.name = "longdan"
table.insert(sgs.ai_skills, longdan_skill)
longdan_skill.getTurnUseCard = function(self)
	local cards = self.player:getCards("h")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local jink_card

	self:sortByUseValue(cards, true)

	for _, card in ipairs(cards) do
		if card:isKindOf("Jink") then
			jink_card = card
			break
		end
	end

	if not jink_card then return nil end
	local suit = jink_card:getSuitString()
	local number = jink_card:getNumberString()
	local card_id = jink_card:getEffectiveId()
	local card_str = ("slash:longdan[%s:%s]=%d"):format(suit, number, card_id)
	local slash = sgs.Card_Parse(card_str)
	assert(slash)

	return slash
end

sgs.ai_view_as.longdan = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place == sgs.Player_PlaceHand then
		if card:isKindOf("Jink") then
			return ("slash:longdan[%s:%s]=%d"):format(suit, number, card_id)
		elseif card:isKindOf("Slash") then
			return ("jink:longdan[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

sgs.longdan_keep_value = {
	Peach = 6,
	Analeptic = 5.8,
	Jink = 5.7,
	FireSlash = 5.7,
	Slash = 5.6,
	ThunderSlash = 5.5,
	ExNihilo = 4.7
}

sgs.ai_skill_invoke.yajiao = true

sgs.ai_skill_playerchosen.yajiao = function(self, targets)
	local id = self.player:getMark("yajiao")
	local card = sgs.Sanguosha:getCard(id)
	local cards = { card }
	local c, friend = self:getCardNeedPlayer(cards, self.friends)
	if friend then return friend end

	self:sort(self.friends)
	for _, friend in ipairs(self.friends) do
		if self:isValuableCard(card, friend) and not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then return friend end
	end
	for _, friend in ipairs(self.friends) do
		if self:isWeak(friend) and not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then return friend end
	end
	local trash = card:isKindOf("Disaster") or card:isKindOf("GodSalvation") or card:isKindOf("AmazingGrace")
	if trash then
		for _, enemy in ipairs(self.enemies) do
			if enemy:getPhase() > sgs.Player_Play and self:needKongcheng(enemy, true) and not hasManjuanEffect(enemy) then return enemy end
		end
	end
	for _, friend in ipairs(self.friends) do
		if not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then return friend end
	end
end

sgs.ai_playerchosen_intention.yajiao = function(self, from, to)
	if not self:needKongcheng(to, true) and not hasManjuanEffect(to) then sgs.updateIntention(from, to, -50) end
end

sgs.ai_skill_choice.yajiao = function(self, choices, data)
	local card = data:toCard()
	local valuable = self:isValuableCard(card)
	local current = self.room:getCurrent()
	if not current then return "throw" end -- avoid potential errors
	if current:isAlive() then
		local currentMayObtain = getKnownCard(current, self.player, "ExNihilo") + getKnownCard(current, self.player, "IronChain") > 0
		if currentMayObtain then
			local valuable = self:isValuableCard(card, current)
			return (self:isFriend(current) == valuable) and "cancel" or "throw"
		end
	end

	local hasLightning, hasIndulgence, hasSupplyShortage
	local nextAlive = current:getNextAlive()
	local tricks = nextAlive:getJudgingArea()
	if not tricks:isEmpty() and not nextAlive:containsTrick("YanxiaoCard") and not nextAlive:hasSkill("qianxi") then
		local trick = tricks:at(tricks:length() - 1)
		if self:hasTrickEffective(trick, nextAlive) then
			if trick:isKindOf("Lightning") then hasLightning = true
			elseif trick:isKindOf("Indulgence") then hasIndulgence = true
			elseif trick:isKindOf("SupplyShortage") then hasSupplyShortage = true
			end
		end
	end

	if nextAlive:hasSkill("ikmengyang") then
		local valid = card:isRed()
		return (self:isFriend(nextAlive) == valid) and "throw" or "cancel"
	end
	if nextAlive:hasSkill("yinghun") and nextAlive:isWounded() then
		return self:isFriend(nextAlive) and "cancel" or "throw"
	end
	if hasLightning then
		local valid = (card:getSuit() == sgs.Card_Spade and card:getNumber() >= 2 and card:getNumber() <= 9)
		return (self:isFriend(nextAlive) == valid) and "throw" or "cancel"
	end
	if hasIndulgence then
		local valid = (card:getSuit() ~= sgs.Card_Heart)
		return (self:isFriend(nextAlive) == valid) and "throw" or "cancel"
	end
	if hasSupplyShortage then
		local valid = (card:getSuit() ~= sgs.Card_Club)
		return (self:isFriend(nextAlive) == valid) and "throw" or "cancel"
	end

	if self:isFriend(nextAlive) and not self:willSkipDrawPhase(nextAlive) and not self:willSkipPlayPhase(nextAlive)
		and not nextAlive:hasSkill("ikmengyang")
		and not nextAlive:hasSkills("ikchibao|nostuxi") and not (nextAlive:hasSkill("qiaobian") and nextAlive:getHandcardNum() > 0) then
		if self:isValuableCard(card, nextAlive) then
			return "cancel"
		end
		if card:isKindOf("Jink") and getCardsNum("Jink", nextAlive, self.player) < 1 then
			return "cancel"
		end
		if card:isKindOf("Nullification") and getCardsNum("Nullification", nextAlive, self.player) < 1 then
			return "cancel"
		end
		if card:isKindOf("Slash") and self:hasCrossbowEffect(nextAlive) then
			return "cancel"
		end
		for _, skill in ipairs(sgs.getPlayerSkillList(nextAlive)) do
			if sgs.ai_cardneed[skill:objectName()] and sgs.ai_cardneed[skill:objectName()](nextAlive, card) then return "cancel" end
		end
	end

	local trash = card:isKindOf("Disaster") or card:isKindOf("AmazingGrace") or card:isKindOf("GodSalvation")
	if trash and self:isEnemy(nextAlive) then return "cancel" end
	return "throw"
end

function SmartAI:isValuableCard(card, player)
	player = player or self.player
	if (isCard("Peach", card, player) and getCardsNum("Peach", player, self.player) <= 2)
		or (self:isWeak(player) and isCard("Analeptic", card, player))
		or (player:getPhase() ~= sgs.Player_Play
			and ((isCard("Nullification", card, player) and getCardsNum("Nullification", player, self.player) < 2 and player:hasSkills("jizhi|ikhuiquan|jilve"))
				or (isCard("Jink", card, player) and getCardsNum("Jink", player, self.player) < 2)))
		or (player:getPhase() == sgs.Player_Play and isCard("ExNihilo", card, player) and not player:isLocked(card)) then
		return true
	end
	local dangerous = self:getDangerousCard(player)
	if dangerous and card:getEffectiveId() == dangerous then return true end
	local valuable = self:getValuableCard(player)
	if valuable and card:getEffectiveId() == valuable then return true end
	return false
end

sgs.ai_skill_cardask["@jizhi-exchange"] = function(self, data)
	local card = data:toCard()
	local handcards = sgs.QList2Table(self.player:getHandcards())
	if self.player:getPhase() ~= sgs.Player_Play then
		if hasManjuanEffect(self.player) then return "." end
		self:sortByKeepValue(handcards)
		for _, card_ex in ipairs(handcards) do
			if self:getKeepValue(card_ex) < self:getKeepValue(card) and not self:isValuableCard(card_ex) then
				return "$" .. card_ex:getEffectiveId()
			end
		end
	else
		if card:isKindOf("Slash") and not self:slashIsAvailable() then return "." end
		self:sortByUseValue(handcards)
		for _, card_ex in ipairs(handcards) do
			if self:getUseValue(card_ex) < self:getUseValue(card) and not self:isValuableCard(card_ex) then
				return "$" .. card_ex:getEffectiveId()
			end
		end
	end
	return "."
end

function sgs.ai_cardneed.jizhi(to, card)
	return card:getTypeId() == sgs.Card_TypeTrick
end

sgs.jizhi_keep_value = {
	Peach = 6,
	Analeptic = 5.9,
	Jink = 5.8,
	ExNihilo = 5.7,
	Snatch = 5.7,
	Dismantlement = 5.6,
	IronChain = 5.5,
	SavageAssault = 5.4,
	Duel = 5.3,
	ArcheryAttack = 5.2,
	AmazingGrace = 5.1,
	Collateral = 5,
	FireAttack = 4.9
}

local function getKurouCard(self, not_slash)
	local card_id
	local hold_crossbow = (self:getCardsNum("Slash") > 1)
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	local lightning = self:getCard("Lightning")

	if self:needToThrowArmor() then
		card_id = self.player:getArmor():getId()
	elseif self.player:getHandcardNum() > self.player:getHp() then
		if lightning and not self:willUseLightning(lightning) then
			card_id = lightning:getEffectiveId()
		else
			for _, acard in ipairs(cards) do
				if (acard:isKindOf("BasicCard") or acard:isKindOf("EquipCard") or acard:isKindOf("AmazingGrace"))
					and not self:isValuableCard(acard) and not (acard:isKindOf("Crossbow") and hold_crossbow)
					and not (acard:isKindOf("Slash") and not_slash) then
					card_id = acard:getEffectiveId()
					break
				end
			end
		end
	elseif not self.player:getEquips():isEmpty() then
		local player = self.player
		if player:getOffensiveHorse() then card_id = player:getOffensiveHorse():getId()
		elseif player:getWeapon() and self:evaluateWeapon(self.player:getWeapon()) < 3
				and not (player:getWeapon():isKindOf("Crossbow") and hold_crossbow) then card_id = player:getWeapon():getId()
		elseif player:getArmor() and self:evaluateArmor(self.player:getArmor()) < 2 then card_id = player:getArmor():getId()
		end
	end
	if not card_id then
		if lightning and not self:willUseLightning(lightning) then
			card_id = lightning:getEffectiveId()
		else
			for _, acard in ipairs(cards) do
				if (acard:isKindOf("BasicCard") or acard:isKindOf("EquipCard") or acard:isKindOf("AmazingGrace"))
					and not self:isValuableCard(acard) and not (acard:isKindOf("Crossbow") and hold_crossbow)
					and not (acard:isKindOf("Slash") and not_slash) then
					card_id = acard:getEffectiveId()
					break
				end
			end
		end
	end
	return card_id
end

local kurou_skill = {}
kurou_skill.name = "kurou"
table.insert(sgs.ai_skills, kurou_skill)
kurou_skill.getTurnUseCard = function(self, inclusive)
	if self.player:hasUsed("KurouCard") or not self.player:canDiscard(self.player, "he") or not self.player:hasSkill("zhaxiang")
		or (self.player:getHp() == 2 and self.player:hasSkill("chanyuan")) then return end
	if (self.player:getHp() > 3 and self.player:getHandcardNum() > self.player:getHp())
		or (self.player:getHp() - self.player:getHandcardNum() >= 2) then
		local id = getKurouCard(self)
		if id then return sgs.Card_Parse("@KurouCard=" .. id) end
	end

	local function can_kurou_with_cb(self)
		if self.player:getHp() > 1 then return true end
		local has_save = false
		local huatuo = self.room:findPlayerBySkillName("jijiu")
		if huatuo and self:isFriend(huatuo) then
			for _, equip in sgs.qlist(huatuo:getEquips()) do
				if equip:isRed() then has_save = true break end
			end
			if not has_save then has_save = (huatuo:getHandcardNum() > 3) end
		end
		if has_save then return true end
		local handang = self.room:findPlayerBySkillName("ikjieyou")
		if handang and self:isFriend(handang) and getCardsNum("Slash", handang, self.player) >= 1 then return true end
		return false
	end

	local slash = sgs.cloneCard("slash")
	if (self.player:hasWeapon("crossbow") or self:getCardsNum("Crossbow") > 0) or self:getCardsNum("Slash") > 1 then
		for _, enemy in ipairs(self.enemies) do
			if self.player:canSlash(enemy) and self:slashIsEffective(slash, enemy)
				and sgs.isGoodTarget(enemy, self.enemies, self, true) and not self:slashProhibit(slash, enemy) and can_kurou_with_cb(self) then
				local id = getKurouCard(self, true)
				if id then return sgs.Card_Parse("@KurouCard=" .. id) end
			end
		end
	end

	if self.player:getHp() <= 1 and self:getCardsNum("Analeptic") + self:getCardsNum("Peach") > 1 then
		local id = getKurouCard(self)
		if id then return sgs.Card_Parse("@KurouCard=.") end
 	end
end

sgs.ai_skill_use_func.KurouCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.KurouCard = 6.8

local fanjian_skill = {}
fanjian_skill.name = "fanjian"
table.insert(sgs.ai_skills, fanjian_skill)
fanjian_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() or self.player:hasUsed("FanjianCard") then return nil end
	return sgs.Card_Parse("@FanjianCard=.")
end

sgs.ai_skill_use_func.FanjianCard = function(card, use, self)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards, true)
	self:sort(self.enemies, "defense")

	if self:getCardsNum("Slash") > 0 then
		local slash = self:getCard("Slash")
		local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
		self:useCardSlash(slash, dummy_use)
		if dummy_use.card and dummy_use.to:length() > 0 then
			sgs.ai_use_priority.FanjianCard = sgs.ai_use_priority.Slash + 0.15
			local target = dummy_use.to:first()
			if self:isEnemy(target) and sgs.card_lack[target:objectName()]["Jink"] ~= 1 and target:getMark("yijue") == 0
				and not target:isKongcheng() and (self:getOverflow() > 0 or target:getHandcardNum() > 2)
				and not (self.player:hasSkill("liegong") and (target:getHandcardNum() >= self.player:getHp() or target:getHandcardNum() <= self.player:getAttackRange()))
				and not (self.player:hasSkill("kofliegong") and target:getHandcardNum() >= self.player:getHp()) then
				if target:hasSkill("ikzhongyan") then
					for _, card in ipairs(cards) do
						if self:getUseValue(card) < 6 and card:isBlack() then
							use.card = sgs.Card_Parse("@FanjianCard=" .. card:getEffectiveId())
							if use.to then use.to:append(target) end
							return
						end
					end
				end
				for _, card in ipairs(cards) do
					if self:getUseValue(card) < 6 and card:getSuit() == sgs.Card_Diamond then
						use.card = sgs.Card_Parse("@FanjianCard=" .. card:getEffectiveId())
						if use.to then use.to:append(target) end
						return
					end
				end
			end
		end
	end

	if self:getOverflow() <= 0 then return end
	sgs.ai_use_priority.FanjianCard = 0.2
	local suit_table = { "spade", "club", "heart", "diamond" }
	local equip_val_table = { 1.2, 1.5, 0.5, 1, 1.3 }
	for _, enemy in ipairs(self.enemies) do
		if enemy:getHandcardNum() > 2 then
			local max_suit_num, max_suit = 0, {}
			for i = 0, 3, 1 do
				local suit_num = getKnownCard(enemy, self.player, suit_table[i + 1])
				for j = 0, 4, 1 do
					if enemy:getEquip(j) and enemy:getEquip(j):getSuit() == i then
						local val = equip_val_table[j + 1]
						if j == 1 and self:needToThrowArmor(enemy) then val = -0.5
						else
							if enemy:hasSkills(sgs.lose_equip_skill) then val = val / 8 end
							if enemy:getEquip(j):getEffectiveId() == self:getValuableCard(enemy) then val = val * 1.1 end
							if enemy:getEquip(j):getEffectiveId() == self:getDangerousCard(enemy) then val = val * 1.1 end
						end
						suit_num = suit_num + j
					end
				end
				if suit_num > max_suit_num then
					max_suit_num = suit_num
					max_suit = { i }
				elseif suit_num == max_suit_num then
					table.insert(max_suit, i)
				end
			end
			if max_suit_num == 0 then
				max_suit = {}
				local suit_value = { 1, 1, 1.3, 1.5 }
				for _, skill in ipairs(sgs.getPlayerSkillList(enemy)) do
					if sgs[skill:objectName() .. "_suit_value"] then
						for i = 1, 4, 1 do
							local v = sgs[skill:objectName() .. "_suit_value"][suit_table[i]]
							if v then suit_value[i] = suit_value[i] + v end
						end
					end
				end
				local max_suit_val = 0
				for i = 0, 3, 1 do
					local suit_val = suit_value[i + 1]
					if suit_val > max_suit_val then
						max_suit_val = suit_val
						max_suit = { i }
					elseif suit_val == max_suit_val then
						table.insert(max_suit, i)
					end
				end
			end
			for _, card in ipairs(cards) do
				if self:getUseValue(card) < 6 and table.contains(max_suit, card:getSuit()) then
					use.card = sgs.Card_Parse("@FanjianCard=" .. card:getEffectiveId())
					if use.to then use.to:append(enemy) end
					return
				end
			end
			if getCardsNum("Peach", enemy, self.player) < 2 then
				for _, card in ipairs(cards) do
					if self:getUseValue(card) < 6 and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@FanjianCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			end
		end
	end
	for _, friend in ipairs(self.friends_noself) do
		if friend:hasSkill("hongyan") then
			for _, card in ipairs(cards) do
				if self:getUseValue(card) < 6 and card:getSuit() == sgs.Card_Spade then
					use.card = sgs.Card_Parse("@FanjianCard=" .. card:getEffectiveId())
					if use.to then use.to:append(friend) end
					return
				end
			end
		elseif friend:hasSkill("thanyue") then
			for _, card in ipairs(cards) do
				if self:getUseValue(card) < 6 and card:getSuit() == sgs.Card_Heart then
					use.card = sgs.Card_Parse("@FanjianCard=" .. card:getEffectiveId())
					if use.to then use.to:append(friend) end
					return
				end
			end
		end
		if friend:hasSkill("zhaxiang") and not self:isWeak(friend) and not (friend:getHp() == 2 and friend:hasSkill("chanyuan")) then
			for _, card in ipairs(cards) do
				if self:getUseValue(card) < 6 then
					use.card = sgs.Card_Parse("@FanjianCard=" .. card:getEffectiveId())
					if use.to then use.to:append(friend) end
					return
				end
			end
		end
	end
end

sgs.ai_card_intention.FanjianCard = function(self, card, from, tos)
	local to = tos[1]
	if to:hasSkill("zhaxiang") then
	elseif card:getSuit() == sgs.Card_Spade and to:hasSkill("hongyan") then
		sgs.updateIntention(from, to, -10)
	elseif card:getSuit() == sgs.Card_Heart and not to:hasSkill("hongyan") and to:hasSkill("thanyue") then
		sgs.updateIntention(from, to, -10)
	else
		sgs.updateIntention(from, to, 60)
	end
end

sgs.ai_use_priority.FanjianCard = 0.2

sgs.ai_skill_invoke.fanjian_discard = function(self, data)
	if self:getCardsNum("Peach") >= 1 and not self:willSkipPlayPhase() then return false end
	if not self:isWeak() and self.player:hasSkill("zhaxiang") and not (self.player:getHp() == 2 and self.player:hasSkill("chanyuan")) then return false end
	if self.player:getHandcardNum() <= 3 or self:isWeak() then return true end
	local suit = self.player:getMark("FanjianSuit")
	local count = 0
	for _, card in sgs.qlist(self.player:getHandcards()) do
		if card:getSuit() == suit then
			count = count + 1
			if self:isValuableCard(card) then count = count + 0.5 end
		end
	end
	local equip_val_table = { 2, 2.5, 1, 1.5, 2.2 }
	for i = 0, 4, 1 do
		if self.player:getEquip(i) and self.player:getEquip(i):getSuit() == suit then
			if i == 1 and self:needToThrowArmor() then
				count = count - 1
			else
				count = equip_val_table[i + 1]
				if self.player:hasSkills(sgs.lose_equip_skill) then count = count + 0.5 end
			end
		end
	end
	return count / self.player:getCardCount(true) <= 0.6
end

sgs.ai_skill_invoke.qianxun = function(self, data)
	local effect = data:toCardEffect()
	if effect.card:isKindOf("Collateral") and self.player:getWeapon() then
		local victim = self.player:getTag("collateralVictim"):toPlayer()
		if victim and sgs.ai_skill_cardask["collateral-slash"](self, nil, nil, victim, effect.from) ~= "." then return false end
	end
	if self.player:getPhase() == sgs.Player_Judge then
		if effect.card:isKindOf("Lightning") and self:isWeak() and self:getCardsNum("Peach") + self:getCardsNum("Analeptic") > 0 then return false end
		return true
	end
	local current = self.room:getCurrent()
	if current and self:isFriend(current) and effect.from and self:isFriend(effect.from) then return true end
	if effect.card:isKindOf("Duel") and sgs.ai_skill_cardask["duel-slash"](self, data, nil, effect.from) ~= "." then return false end
	if effect.card:isKindOf("AOE") and sgs.ai_skill_cardask.aoe(self, data, nil, effect.from, effect.card:objectName()) ~= "." then return false end
	if self.player:getHandcardNum() < self:getLeastHandcardNum(self.player) then return true end
	local l_lim, u_lim = math.max(2, self:getLeastHandcardNum(self.player)), math.max(5, #self.friends)
	if u_lim <= l_lim then u_lim = l_lim + 1 end
	return math.random(0, 100) >= (self.player:getHandcardNum() - l_lim) / (u_lim - l_lim + 1) * 100
end

sgs.ai_skill_use["@@lianying"] = function(self, prompt)
	local move = self.player:getTag("LianyingMoveData"):toMoveOneTime()
	local effect = nil
	if move.reason.m_skillName == "qianxun" then effect = self.player:getTag("QianxunEffectData"):toCardEffect() end
	local upperlimit = self.player:getMark("lianying")

	local targets = {}
	if self.player:getPhase() <= sgs.Player_Play then
		table.insert(targets, self.player:objectName())
		if upperlimit == 1 then return "@LianyingCard=.->" .. self.player:objectName() end
	end
	local exclude_self, sn_dis_eff = false
	if effect and (effect.card:isKindOf("Snatch") or effect.card:isKindOf("Dismantlement"))
		and effect.from:isAlive() and self:isEnemy(effect.from) then
		if self.player:getEquips():isEmpty() then
			exclude_self = true
		else
			sn_dis_eff = true
		end
	end
	if hasManjuanEffect(self.player) then exclude_self = true end
	if not exclude_self and effect and effect.card:isKindOf("FireAttack") and effect.from:isAlive()
		and not effect.from:isKongcheng() and self:isEnemy(effect.from) then
		exclude_self = true
	end

	local getValue = function(friend)
		local def = sgs.getDefense(friend)
		if friend:objectName() == self.player:objectName() then
			if sn_dis_eff then def = def + 5 else def = def - 2 end
		end
		return def
	end
	local cmp = function(a, b)
		return getValue(a) < getValue(b)
	end
	table.sort(self.friends, cmp)

	for _, friend in ipairs(self.friends) do
		if not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) and not table.contains(targets, friend:objectName())
			and not (exclude_self and friend:objectName() == self.player:objectName()) then
			table.insert(targets, friend:objectName())
			if #targets == upperlimit then break end
		end
	end
	if #targets > 0 then return "@LianyingCard=.->" .. table.concat(targets, "+") end
	return "."
end

local guose_skill = {}
guose_skill.name = "guose"
table.insert(sgs.ai_skills, guose_skill)
guose_skill.getTurnUseCard = function(self, inclusive)
	if self.player:hasUsed("GuoseCard") then return end
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		local c = sgs.Sanguosha:getCard(id)
		cards:prepend(c)
	end
	cards = sgs.QList2Table(cards)

	self:sortByUseValue(cards, true)
	local card = nil
	local has_weapon, has_armor = false, false

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Weapon") and not (acard:getSuit() == sgs.Card_Diamond) then has_weapon = true end
	end

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Armor") and not (acard:getSuit() == sgs.Card_Diamond) then has_armor = true end
	end

	for _, acard in ipairs(cards) do
		if (acard:getSuit() == sgs.Card_Diamond) and ((self:getUseValue(acard) < sgs.ai_use_value.Indulgence) or inclusive) then
			local shouldUse = true

			if acard:isKindOf("Armor") then
				if not self.player:getArmor() then shouldUse = false
				elseif self.player:hasEquip(acard) and not has_armor and self:evaluateArmor() > 0 then shouldUse = false
				end
			end

			if acard:isKindOf("Weapon") then
				if not self.player:getWeapon() then shouldUse = false
				elseif self.player:hasEquip(acard) and not has_weapon then shouldUse = false
				end
			end

			if shouldUse then
				card = acard
				break
			end
		end
	end

	if not card then return nil end
	return sgs.Card_Parse("@GuoseCard=" .. card:getEffectiveId())
end

sgs.ai_skill_use_func.GuoseCard = function(card, use, self)
	self:sort(self.friends)
	local id = card:getEffectiveId()
	local indul_only = self.player:handCards():contains(id)
	local rcard = sgs.Sanguosha:getCard(id)
	if not indul_only and not self.player:isJilei(rcard) then
		sgs.ai_use_priority.GuoseCard = 5.5
		for _, friend in ipairs(self.friends) do
			if friend:containsTrick("indulgence") and self:willSkipPlayPhase(friend)
				and not friend:hasSkills("shensu|qingyi|qiaobian") and (self:isWeak(friend) or self:getOverflow(friend) > 1) then
				for _, c in sgs.qlist(friend:getJudgingArea()) do
					if c:isKindOf("Indulgence") and self.player:canDiscard(friend, card:getEffectiveId()) then
						use.card = card
						if use.to then use.to:append(friend) end
						return
					end
				end
			end
		end
	end

	local indulgence = sgs.cloneCard("indulgence")
	indulgence:addSubcard(id)
	if not self.player:isLocked(indulgence) then
		sgs.ai_use_priority.GuoseCard = sgs.ai_use_priority.Indulgence
		local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
		self:useCardIndulgence(indulgence, dummy_use)
		if dummy_use.card and dummy_use.to:length() > 0 then
			use.card = card
			if use.to then use.to:append(dummy_use.to:first()) end
			return
		end
	end

	sgs.ai_use_priority.GuoseCard = 5.5
	if not indul_only and not self.player:isJilei(rcard) then
		for _, friend in ipairs(self.friends) do
			if friend:containsTrick("indulgence") and self:willSkipPlayPhase(friend) then
				for _, c in sgs.qlist(friend:getJudgingArea()) do
					if c:isKindOf("Indulgence") and self.player:canDiscard(friend, card:getEffectiveId()) then
						use.card = card
						if use.to then use.to:append(friend) end
						return
					end
				end
			end
		end
	end

	if not indul_only and not self.player:isJilei(rcard) then
		for _, friend in ipairs(self.friends) do
			if friend:containsTrick("indulgence") then
				for _, c in sgs.qlist(friend:getJudgingArea()) do
					if c:isKindOf("Indulgence") and self.player:canDiscard(friend, card:getEffectiveId()) then
						use.card = card
						if use.to then use.to:append(friend) end
						return
					end
				end
			end
		end
	end
end

sgs.ai_use_priority.GuoseCard = 5.5
sgs.ai_use_value.GuoseCard = 5
sgs.ai_card_intention.GuoseCard = -60

function sgs.ai_cardneed.guose(to, card)
	return card:getSuit() == sgs.Card_Diamond
end

sgs.guose_suit_value = {
	diamond = 3.9
}

sgs.ai_skill_use["@@liuli"] = function(self, prompt, method)
	local others = self.room:getOtherPlayers(self.player)
	local slash = self.player:getTag("liuli-card"):toCard()
	others = sgs.QList2Table(others)
	local source
	for _, player in ipairs(others) do
		if player:hasFlag("LiuliSlashSource") then
			source = player
			break
		end
	end
	self:sort(self.enemies, "defense")

	local doLiuli = function(who)
		if not self:isFriend(who) and who:hasSkill("leiji")
			and (self:hasSuit("spade", true, who) or who:getHandcardNum() >= 3)
			and (getKnownCard(who, self.player, "Jink", true) >= 1 or self:hasEightDiagramEffect(who)) then
			return "."
		end

		local cards = self.player:getCards("h")
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		for _, card in ipairs(cards) do
			if not self.player:isCardLimited(card, method) and self.player:canSlash(who) then
				if self:isFriend(who) and not (isCard("Peach", card, self.player) or isCard("Analeptic", card, self.player)) then
					return "@LiuliCard=" .. card:getEffectiveId() .. "->" .. who:objectName()
				else
					return "@LiuliCard=" .. card:getEffectiveId() .. "->" .. who:objectName()
				end
			end
		end

		local cards = self.player:getCards("e")
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		for _, card in ipairs(cards) do
			local range_fix = 0
			if card:isKindOf("Weapon") then range_fix = range_fix + sgs.weapon_range[card:getClassName()] - self.player:getAttackRange(false) end
			if card:isKindOf("OffensiveHorse") then range_fix = range_fix + 1 end
			if not self.player:isCardLimited(card, method) and self.player:canSlash(who, nil, true, range_fix) then
				return "@LiuliCard=" .. card:getEffectiveId() .. "->" .. who:objectName()
			end
		end
		return "."
	end

	for _, enemy in ipairs(self.enemies) do
		if not (source and source:objectName() == enemy:objectName()) then
			local ret = doLiuli(enemy)
			if ret ~= "." then return ret end
		end
	end

	for _, player in ipairs(others) do
		if self:objectiveLevel(player) == 0 and not (source and source:objectName() == player:objectName()) then
			local ret = doLiuli(player)
			if ret ~= "." then return ret end
		end
	end

	self:sort(self.friends_noself, "defense")
	self.friends_noself = sgs.reverse(self.friends_noself)

	for _, friend in ipairs(self.friends_noself) do
		if not self:slashIsEffective(slash, friend) or self:findLeijiTarget(friend, 50, source) then
			if not (source and source:objectName() == friend:objectName()) then
				local ret = doLiuli(friend)
				if ret ~= "." then return ret end
			end
		end
	end

	for _, friend in ipairs(self.friends_noself) do
		if self:needToLoseHp(friend, source, true) or self:getDamagedEffects(friend, source, true) then
			if not (source and source:objectName() == friend:objectName()) then
				local ret = doLiuli(friend)
				if ret ~= "." then return ret end
			end
		end
	end

	if (self:isWeak() or self:hasHeavySlashDamage(source, slash)) and source:hasWeapon("axe") and source:getCards("he"):length() > 2
		and not self:getCardId("Peach") and not self:getCardId("Analeptic") then
		for _, friend in ipairs(self.friends_noself) do
			if not self:isWeak(friend) then
				if not (source and source:objectName() == friend:objectName()) then
					local ret = doLiuli(friend)
					if ret ~= "." then return ret end
				end
			end
		end
	end

	if (self:isWeak() or self:hasHeavySlashDamage(source, slash)) and not self:getCardId("Jink") then
		for _, friend in ipairs(self.friends_noself) do
			if not self:isWeak(friend) or (self:hasEightDiagramEffect(friend) and getCardsNum("Jink", friend, self.player) >= 1) then
				if not (source and source:objectName() == friend:objectName()) then
					local ret = doLiuli(friend)
					if ret ~= "." then return ret end
				end
			end
		end
	end
	return "."
end

sgs.ai_card_intention.LiuliCard = function(self, card, from, to)
	if not self:isWeak(from) or getCardsNum("Jink", from, to) > 0 then sgs.updateIntention(from, to[1], 50) end
end

function sgs.ai_slash_prohibit.liuli(self, from, to, card)
	if not to:hasSkill("liuli") then return false end
	if self:isFriend(to, from) then return false end
	if from:hasFlag("IkJieyouUsed") then return false end
	if to:isNude() then return false end
	for _, friend in ipairs(self:getFriendsNoself(from)) do
		if to:canSlash(friend, card) and self:slashIsEffective(card, friend, from) then return true end
	end
end

function sgs.ai_cardneed.liuli(to, card)
	return to:getCards("he"):length() <= 2
end

local jieyin_skill = {}
jieyin_skill.name = "jieyin"
table.insert(sgs.ai_skills, jieyin_skill)
jieyin_skill.getTurnUseCard = function(self)
	if self.player:getHandcardNum() < 2 then return nil end
	if self.player:hasUsed("JieyinCard") then return nil end

	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)

	local first, second
	self:sortByUseValue(cards, true)
	for _, card in ipairs(cards) do
		if card:isKindOf("TrickCard") then
			local dummy_use = { isDummy = true }
			self:useTrickCard(card, dummy_use)
			if not dummy_use.card then
				if not first then first = card:getEffectiveId()
				elseif first and not second then second = card:getEffectiveId()
				end
			end
			if first and second then break end
		end
	end
	for _, card in ipairs(cards) do
		if card:getTypeId() ~= sgs.Card_TypeEquip and (not self:isValuableCard(card) or self.player:isWounded()) then
			if not first then first = card:getEffectiveId()
			elseif first and first ~= card:getEffectiveId() and not second then second = card:getEffectiveId()
			end
		end
		if first and second then break end
	end

	if not first or not second then return end
	local card_str = ("@JieyinCard=%d+%d"):format(first, second)
	assert(card_str)
	return sgs.Card_Parse(card_str)
end

function SmartAI:getWoundedFriend(maleOnly, include_self)
	local friends = include_self and self.friends or self.friends_noself
	self:sort(friends, "hp")
	local list1 = {}  -- need help
	local list2 = {}  -- do not need help
	local addToList = function(p, index)
		if (not maleOnly or p:isMale()) and p:isWounded() then
			table.insert(index == 1 and list1 or list2, p)
		end
	end

	local getCmpHp = function(p)
		local hp = p:getHp()
		if p:isLord() and self:isWeak(p) then hp = hp - 10 end
		if p:objectName() == self.player:objectName() and self:isWeak(p) and p:hasSkill("qingnang") then hp = hp - 5 end
		if p:hasSkill("buqu") and p:getPile("buqu"):length() > 0 then hp = hp + math.max(0, 5 - p:getPile("buqu"):length()) end
		if p:hasSkill("nosbuqu") and p:getPile("nosbuqu"):length() > 0 then hp = hp + math.max(0, 5 - p:getPile("nosbuqu"):length()) end
		if p:hasSkills("nosrende|ikshenai|kuanggu|kofkuanggu|zaiqi") and p:getHp() >= 2 then hp = hp + 5 end
		return hp
	end

	local cmp = function (a, b)
		if getCmpHp(a) == getCmpHp(b) then
			return sgs.getDefenseSlash(a, self) < sgs.getDefenseSlash(b, self)
		else
			return getCmpHp(a) < getCmpHp(b)
		end
	end

	for _, friend in ipairs(friends) do
		if friend:isLord() then
			if friend:getMark("hunzi") == 0 and friend:hasSkill("hunzi")
				and self:getEnemyNumBySeat(self.player, friend) <= (friend:getHp() >= 2 and 1 or 0) then
				addToList(friend, 2)
			elseif friend:getHp() >= getBestHp(friend) then
				addToList(friend, 2)
			elseif not sgs.isLordHealthy() then
				addToList(friend, 1)
			end
		else
			addToList(friend, friend:getHp() >= getBestHp(friend) and 2 or 1)
		end
	end
	table.sort(list1, cmp)
	table.sort(list2, cmp)
	return list1, list2
end

sgs.ai_skill_use_func.JieyinCard = function(card, use, self)
	local targets = self.room:getOtherPlayers(self.player)
	local target = nil
	while not target do
		target = self:findPlayerToRecover(1, target, true)
		if self:isWeak(target) or self:isWeak() or self:getOverflow() >= 1 then
			break
		else
			targets:removeOne(target)
			target = nil
			if targets:isEmpty() then
				break
			end
		end
	end

	if not target and self:isWeak() and self:getOverflow() >= 2 and (self.role == "lord" or self.role == "renegade") then
		local others = self.room:getOtherPlayers(self.player)
		for _, other in sgs.qlist(others) do
			if other:isWounded() and other:isMale() then
				if not other:hasSkills(sgs.masochism_skill) then
					target = other
					self.player:setFlags("AI_JieyinToEnemy_" .. other:objectName())
					break
				end
			end
		end
	end

	if target then
		use.card = card
		if use.to then use.to:append(target) end
		return
	end
end

sgs.ai_use_priority.JieyinCard = 3.0

sgs.ai_card_intention.JieyinCard = function(self, card, from, tos)
	if not from:hasFlag("AI_JieyinToEnemy_" .. tos[1]:objectName()) then
		sgs.updateIntention(from, tos[1], -80)
	end
end

sgs.dynamic_value.benefit.JieyinCard = true

sgs.xiaoji_keep_value = {
	Peach = 6,
	Jink = 5.1,
	Weapon = 4.9,
	Armor = 5,
	OffensiveHorse = 4.8,
	DefensiveHorse = 5
}

sgs.ai_cardneed.xiaoji = sgs.ai_cardneed.equip

local chuli_skill = {}
chuli_skill.name = "chuli"
table.insert(sgs.ai_skills, chuli_skill)
chuli_skill.getTurnUseCard = function(self, inclusive)
	if not self.player:canDiscard(self.player, "he") or self.player:hasUsed("ChuliCard") then return nil end

	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)

	if self:needToThrowArmor() then return sgs.Card_Parse("@ChuliCard=" .. self.player:getArmor():getEffectiveId()) end
	for _, card in ipairs(cards) do
		if not self:isValuableCard(card) then
			if card:getSuit() == sgs.Card_Spade then return sgs.Card_Parse("@ChuliCard=" .. card:getEffectiveId()) end
		end
	end
	for _, card in ipairs(cards) do
		if not self:isValuableCard(card) and not (self.player:hasSkill("jijiu") and card:isRed() and self:getOverflow() < 2) then
			if card:getSuit() == sgs.Card_Spade then return sgs.Card_Parse("@ChuliCard=" .. card:getEffectiveId()) end
		end
	end
end

sgs.ai_skill_use_func.ChuliCard = function(card, use, self)
	self.chuli_id_choice = {}
	local players = self:findPlayerToDiscard("he", false, true, nil, true)
	local kingdoms = {}
	local targets = {}
	for _, player in ipairs(players) do
		if self:isFriend(player) and not table.contains(kingdoms, player:getKingdom()) then
			table.insert(targets, player)
			table.insert(kingdoms, player:getKingdom())
		end
	end
	for _, player in ipairs(players) do
		if not table.contains(targets, player, true) and not table.contains(kingdoms, player:getKingdom()) then
			table.insert(targets, player)
			table.insert(kingdoms, player:getKingdom())
		end
	end
	if #targets == 0 then return end
	for _, p in ipairs(targets) do
		local id = self:askForCardChosen(p, "he", "dummyreason", sgs.Card_MethodDiscard)
		local chosen_card
		if id then chosen_card = sgs.Sanguosha:getCard(id) end
		if id and chosen_card and (self:isFriend(p) or not p:hasEquip(chosen_card) or sgs.Sanguosha:getCard(id):getSuit() ~= sgs.Card_Spade) then
			if not use.card then use.card = card end
			self.chuli_id_choice[p:objectName()] = id
			if use.to then use.to:append(p) end
		end
	end
end

sgs.ai_use_value.ChuliCard = 5
sgs.ai_use_priority.ChuliCard = 4.6

sgs.ai_card_intention.ChuliCard = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		if self.chuli_id_choice and self.chuli_id_choice[to:objectName()] then
			local em_prompt = { "cardChosen", "chuli", tostring(self.chuli_id_choice[to:objectName()]), from:objectName(), to:objectName() }
			sgs.ai_choicemade_filter.cardChosen.snatch(self, nil, em_prompt)
		end
	end
end

sgs.ai_view_as.jijiu = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place ~= sgs.Player_PlaceSpecial and card:isRed() and player:getPhase() == sgs.Player_NotActive
		and player:getMark("Global_PreventPeach") == 0 then
		return ("peach:jijiu[%s:%s]=%d"):format(suit, number, card_id)
	end
end

sgs.jijiu_suit_value = {
	heart = 6,
	diamond = 6
}

sgs.ai_cardneed.jijiu = function(to, card)
	return card:isRed()
end

sgs.ai_skill_cardask["@wushuang-slash-1"] = function(self, data, pattern, target)
	if sgs.ai_skill_cardask.nullfilter(self, data, pattern, target) then return "." end
	if self:canUseThLuliDecrease(target) then return "." end
	if not target:hasSkill("ikxuwu") and (self.player:hasSkill("wuyan") or target:hasSkill("wuyan")) then return "." end
	if self:getCardsNum("Slash") < 2 and not (self.player:getHandcardNum() == 1 and self.player:hasSkills(sgs.need_kongcheng)) then return "." end
end

sgs.ai_skill_cardask["@multi-jink-start"] = function(self, data, pattern, target, target2, arg)
	local rest_num = tonumber(arg)
	--for  ThLiuren
	if self.player:isChained() and self.player:getMark("thliuren") == 0 and not self.player:hasFlag("AIGlobal_ThLiuren") then
		local current = self.room:getCurrent()
		if current and current:isAlive() and current:getPhase() ~= sgs.Player_NotActive and current:hasSkill("thliuren") then
			self.room:setPlayerFlag(self.player, "AIGlobal_ThLiuren")
			local ret = sgs.ai_skill_cardask["@multi-jink-start"](self, data, pattern, target, nil, rest_num + 1)
			self.room:setPlayerFlag(self.player, "-AIGlobal_ThLiuren")
			return ret
		end
	end
	if rest_num == 1 then return sgs.ai_skill_cardask["slash-jink"](self, data, pattern, target) end
	if sgs.ai_skill_cardask.nullfilter(self, data, pattern, target) then return "." end
	if self:canUseThLuliDecrease(target) then return "." end
	if sgs.ai_skill_cardask["slash-jink"](self, data, pattern, target) == "." then return "." end
	if self.player:hasSkill("ikjingyou") then
		if self.player:getHandcardNum() == 1 and self:getCardsNum("Jink") == 1 and target:hasWeapon("guding_blade") then return "." end
	else
		if self:getCardsNum("Jink") < rest_num and self:hasLoseHandcardEffective() then return "." end
	end
end

sgs.ai_skill_cardask["@multi-jink"] = sgs.ai_skill_cardask["@multi-jink-start"]

local function getPriorFriendsOfLiyu(self, lvbu)
	lvbu = lvbu or self.player
	local prior_friends = {}
	if not string.startsWith(self.room:getMode(), "06_") and not sgs.GetConfig("EnableHegemony", false) then
		if lvbu:isLord() then
			for _, friend in ipairs(self:getFriendsNoself(lvbu)) do
				if sgs.evaluatePlayerRole(friend) == "loyalist" then table.insert(prior_friends, friend) end
			end
		elseif lvbu:getRole() == "loyalist" then
			local lord = self.room:getLord()
			if lord then prior_friends = { lord } end
		elseif self.room:getMode() == "couple" then
			local diaochan = self.room:findPlayer("diaochan")
			if diaochan then prior_friends = { diaochan } end
		end
	elseif self.room:getMode() == "06_3v3" then
		if lvbu:getRole() == "loyalist" then
			for _, friend in ipairs(self:getFriendsNoself(lvbu)) do
				if friend:getRole() == "lord" then table.insert(prior_friends, friend) break end
			end
		elseif lvbu:getRole() == "rebel" then
			for _, friend in ipairs(self:getFriendsNoself(lvbu)) do
				if friend:getRole() == "renegade" then table.insert(prior_friends, friend) break end
			end
		end
	elseif self.room:getMode() == "06_XMode" then
		local leader = lvbu:getTag("XModeLeader"):toPlayer()
		local backup = 0
		if leader then
			backup = #leader:getTag("XModeBackup"):toStringList()
			if backup == 0 then
				prior_friends = self:getFriendsNoself(lvbu)
			end
		end
	end
	return prior_friends
end

function SmartAI:hasLiyuEffect(target, slash)
	local upperlimit = (self.player:hasSkill("wushuang") and 2 or 1)
	if #self.friends_noself == 0 or self.player:hasSkill("ikxuwu") then return false end
	if not self:slashIsEffective(slash, target, self.player) then return false end

	local targets = { target }
	if not self.player:hasSkill("ikxuwu") and target:isChained() and slash:isKindOf("NatureSlash") then
		for _, p in sgs.qlist(self.room:getOtherPlayers(target)) do
			if p:isChained() and p:objectName() ~= self.player:objectName() then table.insert(targets, p) end
		end
	end
	local unsafe = false
	for _, p in ipairs(targets) do
		if self:isEnemy(target) and not target:isNude() then
			unsafe = true
			break
		end
	end
	if not unsafe then return false end

	local duel = sgs.cloneCard("Duel")
	if self.player:isLocked(duel) then return false end

	local enemy_null = 0
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isFriend(p) then enemy_null = enemy_null - getCardsNum("Nullification", p, self.player) end
		if self:isEnemy(p) then enemy_null = enemy_null + getCardsNum("Nullification", p, self.player) end
	end
	enemy_null = enemy_null - self:getCardsNum("Nullification")
	if enemy_null <= -1 then return false end

	local prior_friends = getPriorFriendsOfLiyu(self)
	if #prior_friends == 0 then return false end
	for _, friend in ipairs(prior_friends) do
		if self:hasTrickEffective(duel, friend, self.player) and self:isWeak(friend) and (getCardsNum("Slash", friend, self.player) < upperlimit or self:isWeak()) then
			return true
		end
	end

	if sgs.isJinkAvailable(self.player, target, slash) and getCardsNum("Jink", target, self.player) >= upperlimit
		and not self:needToLoseHp(target, self.player, true) and not self:getDamagedEffects(target, self.player, true) then return false end
	if slash:hasFlag("AIGlobal_KillOff") or (target:getHp() == 1 and self:isWeak(target) and self:getSaveNum() < 1) then return false end

	if self.player:hasSkill("ikwumou") and self.player:getMark("@wrath") == 0 and (self:isWeak() or not self.player:hasSkill("zhaxiang")) then return true end
	if self.player:hasSkills("jizhi|ikhuiquan") or (self.player:hasSkill("jilve") and self.player:getMark("@bear") > 0) then return false end
	if not string.startsWith(self.room:getMode(), "06_") and not sgs.GetConfig("EnableHegemony", false) and self.role ~= "rebel" then
		for _, friend in ipairs(self.friends_noself) do
			if self:hasTrickEffective(duel, friend, self.player) and self:isWeak(friend) and (getCardsNum("Slash", friend, self.player) < upperlimit or self:isWeak())
				and self:getSaveNum(true) < 1 then
				return true
			end
		end
	end
	return false
end

sgs.ai_skill_playerchosen.liyu = function(self, targets)
	local enemies = {}
	for _, target in sgs.qlist(targets) do
		if self:isEnemy(target) then table.insert(enemies, target) end
	end
	-- should give one card to Lv Bu
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	local lvbu = damage.from
	if not lvbu then return nil end
	local duel = sgs.cloneCard("duel")
	if lvbu:isLocked(duel) then
		if self:isFriend(lvbu) and self:needToThrowArmor() and #enemies > 0 then
			return enemies[1]
		else
			return nil
		end
	end
	if self:isFriend(lvbu) then
		if self.player:getHandcardNum() >= 3 or self:needKongcheng()
			or (self:getLeastHandcardNum() > 0 and self.player:getHandcardNum() <= self:getLeastHandcardNum())
			or self:needToThrowArmor() or self.player:getOffensiveHorse() or (self.player:getWeapon() and self:evaluateWeapon(self.player:getWeapon()) < 5)
			or (not self.player:getEquips():isEmpty() and lvbu:hasSkills("zhijian|yuanhu|huyuan")) then
		else
			return nil
		end
	else
		if self.player:getEquips():isEmpty() then
			local all_peach = true
			for _, card in sgs.qlist(self.player:getHandcards()) do
				if not isCard("Peach", card, lvbu) then all_peach = false break end
			end
			if all_peach then return nil end
		end
		local upperlimit = (self.player:hasSkill("wushuang") and 2 or 1)
		local prior_friends = getPriorFriendsOfLiyu(self, lvbu)
		if #prior_friends > 0 then
			for _, friend in ipairs(prior_friends) do
				if self:hasTrickEffective(duel, friend, lvbu) and self:isWeak(friend)
					and (getCardsNum("Slash", friend, self.player) < upperlimit or self:isWeak(lvbu)) then
					return friend
				end
			end
		end
		if self:getValuableCard(self.player) then return nil end
		local valuable = 0
		for _, card in sgs.qlist(self.player:getHandcards()) do
			if self:isValuableCard(card) then valuable = valuable + 1 end
		end
		if valuable / self.player:getHandcardNum() > 0.4 then return nil end
	end

	-- the target of the Duel
	self:sort(enemies)
	for _, enemy in ipairs(enemies) do
		if self:hasTrickEffective(duel, enemy, lvbu) then
			if not (self:isFriend(lvbu) and getCardsNum("Slash", enemy, self.player) > upperlimit and self:isWeak(lvbu)) then
				return enemy
			end
		end
	end
end

sgs.ai_playerchosen_intention.liyu = 60

function SmartAI:getLijianCard()
	local card_id
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	local lightning = self:getCard("Lightning")

	if self:needToThrowArmor() then
		card_id = self.player:getArmor():getId()
	elseif self.player:getHandcardNum() > self.player:getHp() then
		if lightning and not self:willUseLightning(lightning) then
			card_id = lightning:getEffectiveId()
		else
			for _, acard in ipairs(cards) do
				if (acard:isKindOf("BasicCard") or acard:isKindOf("EquipCard") or acard:isKindOf("AmazingGrace"))
					and not self:isValuableCard(acard) then
					card_id = acard:getEffectiveId()
					break
				end
			end
		end
	elseif not self.player:getEquips():isEmpty() then
		local player = self.player
		if player:getOffensiveHorse() then card_id = player:getOffensiveHorse():getId()
		elseif player:getWeapon() then card_id = player:getWeapon():getId()
		elseif player:getDefensiveHorse() then card_id = player:getDefensiveHorse():getId()
		elseif player:getArmor() and player:getHandcardNum() <= 1 then card_id = player:getArmor():getId()
		end
	end
	if not card_id then
		if lightning and not self:willUseLightning(lightning) then
			card_id = lightning:getEffectiveId()
		else
			for _, acard in ipairs(cards) do
				if (acard:isKindOf("BasicCard") or acard:isKindOf("EquipCard") or acard:isKindOf("AmazingGrace"))
					and not self:isValuableCard(acard) then
					card_id = acard:getEffectiveId()
					break
				end
			end
		end
	end
	return card_id
end

function SmartAI:findLijianTarget(card_name, use)
	local lord = self.room:getLord()
	local duel = sgs.cloneCard("duel")

	local findFriend_maxSlash = function(self, first)
		local maxSlash = 0
		local friend_maxSlash
		local nos_fazheng
		for _, friend in ipairs(self.friends_noself) do
			if friend:isMale() and self:hasTrickEffective(duel, first, friend) then
				if friend:hasSkill("thfusheng") and friend:getHp() > 1 then nos_fazheng = friend end
				if (getCardsNum("Slash", friend, self.player) > maxSlash) then
					maxSlash = getCardsNum("Slash", friend, self.player)
					friend_maxSlash = friend
				end
			end
		end

		if friend_maxSlash then
			local safe = false
			if first:hasSkills("ikaoli|vsganglie|ikhuanji|nosfankui|enyuan|nosganglie|thfusheng") and not first:hasSkills("wuyan|noswuyan") then
				if (first:getHp() <= 1 and first:getHandcardNum() == 0) then safe = true end
			elseif (getCardsNum("Slash", friend_maxSlash, self.player) >= getCardsNum("Slash", first, self.player)) then safe = true end
			if safe then return friend_maxSlash end
		end
		if nos_fazheng then return nos_fazheng end
		return nil
	end

	if not sgs.GetConfig("EnableHegemony", false)
		and (self.role == "rebel" or (self.role == "renegade" and sgs.current_mode_players["loyalist"] + 1 > sgs.current_mode_players["rebel"])) then
		if lord and lord:objectName() ~= self.player:objectName() and lord:isMale() and not lord:isNude() then
			self:sort(self.enemies, "handcard")
			local e_peaches = 0
			local loyalist
			for _, enemy in ipairs(self.enemies) do
				e_peaches = e_peaches + getCardsNum("Peach", enemy, self.player)
				if enemy:getHp() == 1 and self:hasTrickEffective(duel, enemy, lord) and enemy:objectName() ~= lord:objectName() and enemy:isMale() then
					loyalist = enemy
					break
				end
			end
			if loyalist and e_peaches < 1 then return loyalist, lord end
		end

		if #self.friends_noself >= 2 and self:getAllPeachNum() < 1 then
			local nextplayerIsEnemy
			local nextp = self.player:getNextAlive()
			for i = 1, self.room:alivePlayerCount() do
				if not self:willSkipPlayPhase(nextp) then
					if not self:isFriend(nextp) then nextplayerIsEnemy = true end
					break
				else
					nextp = nextp:getNextAlive()
				end
			end
			if nextplayerIsEnemy then
				local round = 50
				local to_die, nextfriend
				self:sort(self.enemies,"hp")

				for _, a_friend in ipairs(self.friends_noself) do
					if a_friend:getHp() == 1 and a_friend:isKongcheng() and not a_friend:hasSkill("ikjingyou") and a_friend:isMale() then
						for _, b_friend in ipairs(self.friends_noself) do
							if b_friend:objectName() ~= a_friend:objectName() and b_friend:isMale() and self:playerGetRound(b_friend) < round
								and self:hasTrickEffective(duel, a_friend, b_friend) then

								round = self:playerGetRound(b_friend)
								to_die = a_friend
								nextfriend = b_friend
							end
						end
						if to_die and nextfriend then break end
					end
				end
				if to_die and nextfriend then return to_die, nextfriend end
			end
		end
	end

	if lord and lord:objectName() ~= self.player:objectName() and self:isFriend(lord)
		and lord:hasSkill("hunzi") and lord:getHp() == 2 and lord:getMark("hunzi") == 0 then
		local enemycount = self:getEnemyNumBySeat(self.player, lord) 
		local peaches = self:getAllPeachNum()
		if peaches >= enemycount then
			local f_target, e_target
			for _, ap in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if ap:objectName() ~= lord:objectName() and ap:isMale() and self:hasTrickEffective(duel, lord, ap) and ap:getMark("ikluoyi") == 0 then
					if ap:hasSkills("jiang|ikhuiquan|jizhi") and self:isFriend(ap) and not ap:isLocked(duel) then
						if not use.isDummy then lord:setFlags("AIGlobal_NeedToWake") end
						return lord, ap
					elseif self:isFriend(ap) then
						f_target = ap
					else
						e_target = ap
					end
				end
			end
			if f_target or e_target then
				local target
				if f_target and not f_target:isLocked(duel) then
					target = f_target
				elseif e_target and not e_target:isLocked(duel) then
					target = e_target
				end
				if target then
					if not use.isDummy then lord:setFlags("AIGlobal_NeedToWake") end
					return lord, target
				end
			end
		end
	end

	local shenguanyu = self.room:findPlayerBySkillName("wuhun")
	if shenguanyu and shenguanyu:isMale() and shenguanyu:objectName() ~= self.player:objectName() then
		if self.role == "rebel" and lord and lord:isMale() and not lord:hasSkill("ikxuwu") and self:hasTrickEffective(duel, shenguanyu, lord) then
			return shenguanyu, lord
		elseif self:isEnemy(shenguanyu) and #self.enemies >= 2 then
			for _, enemy in ipairs(self.enemies) do
				if enemy:objectName() ~= shenguanyu:objectName() and enemy:isMale() and not enemy:isLocked(duel)
					and self:hasTrickEffective(duel, shenguanyu, enemy) then
					return shenguanyu, enemy
				end
			end
		end
	end

	self:sort(self.enemies, "defense")
	local males, others = {}, {}
	local first, second
	local zhugeliang_kongcheng, xunyu, xuchu
		for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if player:isMale() and player:getMark("ikluoyi") > 0 and not player:isLocked(duel) then
			xuchu = enemy
			break
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if enemy:isMale() and not enemy:hasSkills("wuyan|noswuyan") then
			if enemy:hasSkill("ikjingyou") and enemy:isKongcheng() then zhugeliang_kongcheng = enemy
			elseif enemy:hasSkill("jieming") then xunyu = enemy
			else
				for _, anotherenemy in ipairs(self.enemies) do
					if anotherenemy:isMale() and anotherenemy:objectName() ~= enemy:objectName() then
						if #males == 0 and self:hasTrickEffective(duel, enemy, anotherenemy) then
							if not (enemy:hasSkill("hunzi") and enemy:getMark("hunzi") == 0 and enemy:getHp() == 2) then
								table.insert(males, enemy)
							else
								table.insert(others, enemy)
							end
						end
						if #males == 1 and self:hasTrickEffective(duel, males[1], anotherenemy) and not anotherenemy:isLocked(duel) then
							if not anotherenemy:hasSkills("ikhuiquan|jizhi|jiang")
								and not (anotherenemy:hasSkill("jilve") and anotherenemy:getMark("@bear") > 0) then
								table.insert(males, anotherenemy)
							else
								table.insert(others, anotherenemy)
							end
						end
					end
				end
			end
			if #males >= 2 then break end
		end
	end

	local insert_friend_xuchu = false
	if #males > 0 and xuchu and self:hasTrickEffective(duel, males[1], xuchu) then
		if not table.contains(males, xuchu, true) then
			local males1 = males[1]
			if self:isEnemy(xuchu) then
				if not xuchu:hasSkills("ikhuiquan|jizhi|jiang") and not (xuchu:hasSkill("jilve") and xuchu:getMark("@bear") > 0) then
					males = { males1, xuchu }
				end
			elseif self:isFriend(xuchu) then
				local fac = xuchu:hasSkill("wushuang") and 2 or 1
				if getCardsNum("Slash", males1, self.player) + 1 <= getCardsNum("Slash", xuchu, self.player) * fac then
					insert_friend_xuchu = true
				end
			end
		end
	end

	if #males >= 1 and sgs.ai_role[males[1]:objectName()] == "rebel" and males[1]:getHp() == 1 then
		if lord and self:isFriend(lord) and lord:isMale()
			and lord:objectName() ~= males[1]:objectName() and lord:objectName() ~= self.player:objectName()
			and self:hasTrickEffective(duel, males[1], lord)
			and not lord:isLocked(duel) and (getCardsNum("Slash", males[1], self.player) < 1
											or getCardsNum("Slash", males[1], self.player) < getCardsNum("Slash", lord, self.player)
											or (getKnownNum(males[1]) == males[1]:getHandcardNum() and getKnownCard(males[1], self.player, "Slash", true, "he") == 0)) then
			return males[1], lord
		end
		local afriend = findFriend_maxSlash(self, males[1])
		if afriend and afriend:objectName() ~= males[1]:objectName() and not afriend:isLocked(duel) then
			return males[1], afriend
		end
	end

	if #males == 1 then
		if males[1]:isLord() and sgs.turncount <= 1 and self.role == "rebel" and self.player:aliveCount() >= 3 then
			local p_slash, max_p, max_pp = 0
			for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if p:isMale() and not self:isFriend(p) and p:objectName() ~= males[1]:objectName() and self:hasTrickEffective(duel, males[1], p)
					and not p:isLocked(duel) and p_slash < getCardsNum("Slash", p, self.player) then
					if p:getKingdom() == males[1]:getKingdom() then
						max_p = p
						break
					elseif not max_pp then
						max_pp = p
					end
				end
			end
			if max_p then table.insert(males, max_p) end
			if max_pp and #males == 1 then table.insert(males, max_pp) end
		end
	end

	if #males == 1 then
		if insert_friend_xuchu then
			table.insert(males, xuchu)
		elseif #others >= 1 and not others[1]:isLocked(duel) then
			table.insert(males, others[1])
		elseif xunyu and self:hasTrickEffective(duel, males[1], xunyu) and not xunyu:isLocked(duel) then
			if getCardsNum("Slash", males[1], self.player) < 1 then
				table.insert(males, xunyu)
			else
				local drawcards = 0
				for _, enemy in ipairs(self.enemies) do
					local x = math.max(math.min(5, enemy:getMaxHp()) - enemy:getHandcardNum(), 0)
					if x > drawcards then drawcards = x end
				end
					if drawcards <= 2 then
					table.insert(males, xunyu)
				end
			end
		end
	end

	if #males == 1 and #self.friends_noself > 0 then
		first = males[1]
		if zhugeliang_kongcheng and hasTrickEffective(duel, first, zhugeliang_kongcheng) and not zhugeliang_kongcheng:isLocked(duel) then
			table.insert(males, zhugeliang_kongcheng)
		else
			local friend_maxSlash = findFriend_maxSlash(self, first)
			if friend_maxSlash and not friend_maxSlash:isLocked(duel) then table.insert(males, friend_maxSlash) end
		end
	end

	if #males >= 2 then
		first = males[1]
		second = males[2]
		if first:getHp() <= 1 then
			if self.player:isLord() or sgs.isRolePredictable() then
				local friend_maxSlash = findFriend_maxSlash(self, first)
				if friend_maxSlash and not friend_maxSlash:isCardLimited(duel, sgs.Card_MethodUse) then second = friend_maxSlash end
			elseif lord and lord:objectName() ~= self.player:objectName() and lord:isMale() and not lord:hasSkills("wuyan|noswuyan") then
				if self.role == "rebel" and not lord:isLocked(duel) and not first:isLord() and self:hasTrickEffective(duel, first, lord) then
					second = lord
				else
					if (self.role == "loyalist" or self.role == "renegade") and not first:hasSkills("ikaoli|enyuan|vsganglie|nosganglie|thfusheng")
						and getCardsNum("Slash", first, self.player) <= getCardsNum("Slash", second, self.player) and not lord:isLocked(duel) then
						second = lord
					end
				end
			end
		end

		if first and second and first:objectName() ~= second:objectName() and not second:isLocked(duel) then
			return first, second
		end
	end
end

local lijian_skill = {}
lijian_skill.name = "lijian"
table.insert(sgs.ai_skills, lijian_skill)
lijian_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("LijianCard") or not self.player:canDiscard(self.player, "he") then return end
	local card_id = self:getLijianCard()
	if card_id then return sgs.Card_Parse("@LijianCard=" .. card_id) end
end

sgs.ai_skill_use_func.LijianCard = function(card, use, self)
	local first, second = self:findLijianTarget("LijianCard", use)
	if first and second then
		use.card = card
		if use.to then
			use.to:append(first)
			use.to:append(second)
		end
	end
end

sgs.ai_use_value.LijianCard = 8.5
sgs.ai_use_priority.LijianCard = 4

sgs.ai_card_intention.LijianCard = function(self, card, from, to)
	if sgs.evaluatePlayerRole(to[1]) == sgs.evaluatePlayerRole(to[2]) then
		if sgs.evaluatePlayerRole(from) == "rebel" and sgs.evaluatePlayerRole(to[1]) == sgs.evaluatePlayerRole(from) and to[1]:getHp() == 1 then
		elseif to[1]:hasSkill("hunzi") and to[1]:getHp() == 2 and to[1]:getMark("hunzi") == 0 then
		else
			sgs.updateIntentions(from, to, 40)
		end
	elseif sgs.evaluatePlayerRole(to[1]) ~= sgs.evaluatePlayerRole(to[2]) and not to[1]:hasSkill("wuhun") then
		sgs.updateIntention(from, to[1], 80)
	end
end

sgs.ai_skill_invoke.biyue = function(self, data)
	return not self:needKongcheng(self.player, true)
end

function SmartAI:canUseThLuliDecrease(damage_from, player)
	if not damage_from then return false end
	local player = player or self.player
	if player:hasSkill("thluli") and damage_from:getHp() >= player:getHp() then
		for _, card in sgs.qlist(player:getHandcards()) do
			local flag = string.format("%s_%s_%s", "visible", self.room:getCurrent():objectName(), player:objectName())
			if player:objectName() == self.player:objectName() or card:hasFlag("visible") or card:hasFlag(flag) then
				if card:isRed() and not isCard("Peach", card, player) then return true end
			end
		end
	end
	return false
end

sgs.ai_skill_choice.yaowu = function(self, choices)
	if self.player:getHp() >= getBestHp(self.player) or (self:needKongcheng(self.player, true) and self.player:getPhase() == sgs.Player_NotActive) then
		return "draw"
	end
	return "recover"
end

sgs.ai_skill_invoke.wangzun = function(self, data)
	local lord = self.room:getCurrent()
	if self.player:getPhase() == sgs.Player_NotActive and self:needKongcheng(self.player, true) then
		return self.player:hasSkill("manjuan") and self:isEnemy(lord)
	end
	if self:isEnemy(lord) then return true
	else
		if not self:isWeak(lord) and (self:getOverflow(lord) < -2 or (self:willSkipDrawPhase(lord) and self:getOverflow(lord) < 0)) then
			return true
		end
	end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.wangzun = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		local lord = self.room:getCurrent()
		if not self:isWeak(lord) and (self:getOverflow(lord) < -2 or (self:willSkipDrawPhase(lord) and self:getOverflow(lord) < 0)) then return end
		sgs.updateIntention(player, lord, 30)
	end
end

sgs.ai_skill_invoke.qiaomeng = function(self, data)
	local damage = data:toDamage()
	if self:isFriend(damage.to) then return damage.to:getArmor() and self:needToThrowArmor(damage.to) end
	return not self:doNotDiscard(damage.to, "e")
end

sgs.ai_playerchosen_intention.yajiao = function(self, from, to)
	if not self:needKongcheng(to, true) and not hasManjuanEffect(to) then sgs.updateIntention(from, to, -50) end
end