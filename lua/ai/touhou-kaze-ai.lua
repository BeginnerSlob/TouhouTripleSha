-- 祉迹：弃牌阶段结束时，若你于此阶段弃置了两张或更多的手牌，你可以回复1点体力或摸两张牌。
sgs.ai_skill_choice.thzhiji = function(self, choices, data)
	if (self:isWeak() or self:needKongcheng(self.player, true)) and string.find(choices, "recover") then
		return "recover"
	else
		return "draw"
	end
end

--祭仪：出牌阶段限一次，你可令一名其他角色选择一项：展示一张锦囊牌并摸一张牌；或交给你一张牌。
thjiyitarget = nil
getThJiyiTarget = function(self)
	thjiyitarget = nil

	self:sort(self.enemies, "defense")
	for _, p in ipairs(self.enemies) do
		if not p:isKongcheng() and self:isWeak(p) and getKnownCard(p, self.player, "TrickCard") == 0 then
			thjiyitarget = p
			return
		end
	end --weak enemy

	self:sort(self.friends_noself, "handcard")
	for _, p in ipairs(self.friends_noself) do
		if self:needKongcheng(p, true) and not self:isWeak(p) and p:getHandcardNum() == 1 then
			thjiyitarget = p
			return
		end
	end --friend need kongcheng

	self:sort(self.friends_noself, "defense")
	for _, p in ipairs(self.friends_noself) do
		if not p:isKongcheng() and p:getPile("wooden_ox"):isEmpty() and getKnownCard(p, self.player, "TrickCard") > 0 then
			if not self:needKongcheng(p, true) then
				thjiyitarget = p
				return
			end
		end
	end --friend who has known trick

	for _, p in ipairs(self.enemies) do
		if not p:isKongcheng() and getKnownCard(p, self.player, "TrickCard") == 0 then
			thjiyitarget = p
			return
		end
	end --enemys
end

local thjiyi_skill = {}
thjiyi_skill.name = "thjiyi"
table.insert(sgs.ai_skills, thjiyi_skill)
thjiyi_skill.getTurnUseCard = function(self)
	getThJiyiTarget(self)
	if thjiyitarget and not self.player:hasUsed("ThJiyiCard") then
		return sgs.Card_Parse("@ThJiyiCard=.")
	end
end

sgs.ai_skill_use_func.ThJiyiCard = function(card, use, self)
	use.card = card
	if use.to then
		use.to:append(thjiyitarget)
	end
end

sgs.ai_skill_cardask["@thjiyi"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, cd in ipairs(cards) do
		if cd:isKindOf("TrickCard") then
			return "$"..cd:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_skill_cardask["@thjiyigive"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	return "$"..cards[1]:getEffectiveId()
end

sgs.ai_card_intention.ThJiyiCard = function(self, card, from, to)
	if getKnownCard(to[1], from, "TrickCard") > 0 then
		sgs.updateIntention(from, to[1], -50)
		return
	end
end

--华袛：君主技，当你于回合外失去一次手牌后，其他风势力角色可以各交给你一张手牌。
sgs.ai_skill_cardask["@thhuadi"] = function(self, data, pattern, target)
	if self:isFriend(target) and not self:isWeak(self.player) and not self:isKongcheng() then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards, true)
		return "$"..cards[1]:getEffectiveId()
	end
	return "."
end

--疾岚：摸牌阶段开始时，你可以少摸一张牌，然后进行一次判定，你选择一项：获得此判定牌；或获得场上的一张与此判定牌花色不同的牌。
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
			if target:hasSkills("ikyindie+ikguiyue") and target:getPhase() == sgs.Player_NotActive then continue end
			if target:containsTrick("purple_song") then
				return true
			end
			if target:hasEquip() then
				return true
			else
				continue
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
		for _, cd in sgs.qlist(p:getJudgingArea()) do
			if cd:isKindOf("PurpleSong") then
				return p
			end
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

sgs.ai_choicemade_filter.cardChosen.thjilanwen = sgs.ai_choicemade_filter.cardChosen.snatch

--念刻：出牌阶段限一次，你可以交给一名其他角色一张【闪】，然后展示该角色一张手牌，若该牌为红色，你与该角色各摸一张牌。
local thnianke_skill = {}
thnianke_skill.name = "thnianke"
table.insert(sgs.ai_skills, thnianke_skill)
thnianke_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThNiankeCard") then
		return nil
	end
	local cards = sgs.QList2Table(self.player:getCards("h"))

	local card

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Jink") then
			card = acard
			break
		end
	end

	if not card then
		return nil
	end

	return sgs.Card_Parse("@ThNiankeCard="..card:getEffectiveId())
end

sgs.ai_skill_use_func.ThNiankeCard = function(card, use, self)
	local targets_red = {}
	local targets = {}
	self:sort(self.friends_noself, "defense")
	for _, friend in ipairs(self.friends_noself) do
		if (friend:isKongcheng() and not self:needKongcheng(friend, true)) or getKnownCard(friend, self.player, "red") == friend:getHandcardNum() then
			table.insert(targets_red, friend)
		else
			table.insert(targets, friend)
		end
	end
	if #targets_red > 0 then
		use.card = card
		if use.to then
			use.to:append(targets_red[1])
		end
		return
	end
	if self:getCardsNum("Jink", "h") <= 1 then
		return
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

--极岚：每当你受到1点伤害后，可令一名角色弃置X张牌（X为该角色已损失的体力值，且至少为1）。
sgs.ai_skill_playerchosen.thjilan = function(self, targets)
	for _, p in ipairs(self.friends) do
		if p:getArmor() and self:needToThrowArmor(p) and p:canDiscard(p, p:getArmor():getEffectiveId())
		   and (p:getLostHp() <= 1 or p:getCards("he"):length() < 1
				or (p:getLostHp() == 2 and getKnownCard(p, self.player, "Peach") == 0 and p:getCards("he"):length() > 2)) then
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

sgs.ai_playerchosen_intention.thjilan = function(self, from, to)
	if to:getArmor() and self:needToThrowArmor(to) and to:canDiscard(to, to:getArmor():getEffectiveId())
	   and (to:getLostHp() <= 1 or to:getCards("he"):length() < 1
			or (p:getLostHp() == 2 and getKnownCard(p, self.player, "Peach") == 0 and p:getCards("he"):length() > 2)) then
		sgs.updateIntention(from, to, -50)
	else
		sgs.updateIntention(from, to, 50)
	end
end

--王手：每当你对其他角色造成一次伤害后，可以令其进行一次判定，若结果为黑色，你可以弃置其一张牌。
sgs.ai_skill_invoke.thwangshou = true

sgs.ai_skill_invoke.thwangshou_discard = function(self, data)
	local target = findPlayerByObjectName(self.room, data:toString():split(":")[2])
	if target and self:isFriend(target) then
		return (target:hasSkill("ikcangyou") and target:hasEquip()) or self:needToThrowArmor(target)
	end
	return target and self:isEnemy(target)
end

sgs.ai_choicemade_filter.cardChosen.thwangshou = sgs.ai_choicemade_filter.cardChosen.snatch

--栴叶：你的回合外，当其他角色的判定牌为红色且生效后，你可以弃置一张牌，视为你对其使用一张无视距离的【杀】。
sgs.ai_skill_cardask["@thzhanye"] = function(self, data, pattern, target)
	if self.player:isNude() then
		return "."
	end
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)

	local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
	slash:deleteLater()
	if self:isFriend(target) and self:slashIsEffective(slash, target) then
		if self:findLeijiTarget(target, 50, self.player) then return "$" .. cards[1]:getEffectiveId() end
		if self:getDamagedEffects(target, self.player, true) then return "$" .. cards[1]:getEffectiveId() end
	end

	local nature = sgs.DamageStruct_Normal
	if self:isEnemy(target) and self:slashIsEffective(slash, target) and self:canAttack(target, self.player, nature)
		and not self:getDamagedEffects(target, self.player, true) and not self:findLeijiTarget(target, 50, self.player) then
		return "$" .. cards[1]:getEffectiveId()
	end

	return "."
end

--厄难：出牌阶段限一次，你可减少1点体力上限，并令你或你攻击范围内的一名角色失去1点体力。
local thenan_skill = {}
thenan_skill.name = "thenan"
table.insert(sgs.ai_skills, thenan_skill)
thenan_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThEnanCard") then
		return sgs.Card_Parse("@ThEnanCard=.")
	end
	return nil
end

sgs.ai_skill_use_func.ThEnanCard = function(card, use, self)
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if self.player:getMaxHp() > 1 and self.player:inMyAttackRange(enemy) and self:isWeak(enemy) then
			use.card = card
			if use.to then
				use.to:append(enemy)
			end
			return
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if self.player:getMaxHp() > 1 and self.player:inMyAttackRange(enemy) then
			use.card = card
			if use.to then
				use.to:append(enemy)
			end
			return
		end
	end
	if self.player:getHandcardNum() <= 1 and self.player:getHp() == 1 and (self.player:getMaxHp() == 2 or self:getCardsNum("Peach", "h") > 0 or self:getCardsNum("Analeptic", "h") > 0) then
		use.card = card
		if use.to then
			use.to:append(self.player)
		end
		return
	end
end

sgs.ai_card_intention.ThEnanCard = 80

--悲运：当你进入濒死状态时，你可以亮出牌堆顶的4-X张牌（X为你的体力上限，且最多为3）；你可以将其中全部的红色非锦囊牌置入弃牌堆，并回复1点体力；你还可以将其中全部的黑色非锦囊牌置入弃牌堆，并增加1点体力上限；然后你可以获得其余的牌。
sgs.ai_skill_choice.thbeiyun = function(self, choices, data)
	if string.find(choices, "get") then
		return "get"
	end
	local beiyun_ids = data:toIntList()
	local red, black, not_red, not_black = false, false, false, false
	for _, id in sgs.qlist(beiyun_ids) do
		local card = sgs.Sanguosha:getCard(id)
		if card:isRed() then
			if not red and card:getTypeId() ~= sgs.Card_TypeTrick then
				red = true
			end
			if card:isKindOf("Peach") or card:isKindOf("Analeptic") then
				not_red = true
			end
		elseif card:isBlack() then
			if not black and card:getTypeId() ~= sgs.Card_TypeTrick then
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
	if (self:getCardsNum("Peach", "h") + self:getCardsNum("Analeptic", "h")) >= 1 - self.player:getHp() then -- i'm safe
		return black and "black" or "cancel"
	else
		return not not_black and black and "black" or "cancel"
	end
	return "cancel"
end

--迷彩：专属技，出牌阶段限一次，你可以指定一名其他角色，当该角色的装备区没有武器牌且没有拥有专属技时，视为装备着你装备区的武器牌；当该角色的装备区没有防具牌且没有拥有专属技时，视为装备着你装备区的防具牌，直到你的下一个回合开始。
local thmicai_skill = {}
thmicai_skill.name = "thmicai"
table.insert(sgs.ai_skills, thmicai_skill)
thmicai_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThMicaiCard") then
		return nil
	end

	return sgs.Card_Parse("@ThMicaiCard=.")
end

sgs.ai_skill_use_func.ThMicaiCard = function(card, use, self)
	local lord = self.room:getLord()
	if self:isFriend(lord) and not self.player:isLord() and self:isWeak(lord) and self.player:getArmor() and not self.player:getArmor():isKindOf("Vine") then
		use.card = card
		if use.to then
			use.to:append(p)
		end
		return
	end
	if self.player:getArmor() then
		self:sort(self.friends_noself, "defense")
		for _, p in ipairs(self.friends_noself) do
			if not p:getArmor() and not self:hasEightDiagramEffect() then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
	if self.player:getWeapon() then
		self:sort(self.friends_noself, "handcard")
		self.friends_noself = sgs.reverse(self.friends_noself)
		for _, p in ipairs(self.friends_noself) do
			if not p:getWeapon() then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
	use.card = nil
end

function sgs.ai_cardsview.thmicaiv(self, class_name, player)
	if player:hasWeapon("spear") and class_name == "Slash" then
		return cardsView_spear(self, player, "spear")
	end
end

sgs.ai_view_as.thmicaiv = function(card, player, card_place)
	if player:hasWeapon("fan") then
		local suit = card:getSuitString()
		local number = card:getNumberString()
		local card_id = card:getEffectiveId()
		if sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE
			and card_place ~= sgs.Player_PlaceSpecial and card:objectName() == "slash" then
			return ("fire_slash:fan[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

local thmicaiv_skill = {}
thmicaiv_skill.name = "spear"
table.insert(sgs.ai_skills, thmicaiv_skill)
thmicaiv_skill.getTurnUseCard = function(self, inclusive)
	if self.player:hasWeapon("spear") then
		return turnUse_spear(self, inclusive, "spear")
	elseif self.player:hasWeapon("fan") then
		local cards = self.player:getCards("h")
		for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
			cards:prepend(sgs.Sanguosha:getCard(id))
		end
		cards = sgs.QList2Table(cards)
		local slash_card
	
		for _, card in ipairs(cards) do
			if card:isKindOf("Slash") and not (card:isKindOf("FireSlash") or card:isKindOf("ThunderSlash")) then
				slash_card = card
				break
			end
		end
	
		if not slash_card then return nil end
		local suit = slash_card:getSuitString()
		local number = slash_card:getNumberString()
		local card_id = slash_card:getEffectiveId()
		local card_str = ("fire_slash:fan[%s:%s]=%d"):format(suit, number, card_id)
		local fireslash = sgs.Card_Parse(card_str)
		assert(fireslash)
	
		return fireslash
	end
end

sgs.ai_card_intention.ThMicaiCard = -5

--巧工：出牌阶段限一次，你可以弃置两张相同颜色的牌，并获得场上的一张该颜色的装备牌。
local thqiaogong_skill = {}
thqiaogong_skill.name = "thqiaogong"
table.insert(sgs.ai_skills, thqiaogong_skill)
thqiaogong_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThQiaogongCard") then
		return nil
	end

	return sgs.Card_Parse("@ThQiaogongCard=.")
end

sgs.ai_skill_use_func.ThQiaogongCard = function(card, use, self)
	if self.player:getArmor() and self:needToThrowArmor(self.player) then
		local suit = self.player:getArmor():getSuit()
		local card_id1 = self.player:getArmor():getEffectiveId()
		local can_use = {}
		local cards = self.player:getCards("he")
		for _, cd in sgs.qlist(cards) do
			if cd:getSuit() == suit and cd:getId() ~= card_id1 and not isCard("Peach", cd, self.player) and not isCard("ExNihilo", cd, self.player) then
				table.insert(can_use, cd)
			end
		end
		if #can_use >= 1 then
			self:sort(self.enemies, "defense")
			for _, p in ipairs(self.enemies) do
				if p:hasEquip() then
					for _, cd in sgs.qlist(p:getEquips()) do
						if cd:getSuit() == suit then
							local card_id2 = can_use[1]:getEffectiveId()

							local card_str = ("@ThQiaogong=%d+%d"):format(card_id1, card_id2)
							use.card = sgs.Card_Parse(card_str)
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
	for _, p in ipairs(self.friends_noself) do
		if p:getArmor() and self:needToThrowArmor(p) then
			local suit = p:getArmor():getSuit()
			local target_id = p:getArmor():getId()
			local can_use = {}
			local cards = self.player:getCards("he")
			for _, cd in sgs.qlist(cards) do
				if cd:getSuit() == suit and cd:getId() ~= target_id and not isCard("Peach", cd, self.player) and not isCard("ExNihilo", cd, self.player) then
					table.insert(can_use, cd)
				end
			end
			if #can_use >= 2 then
				self:sortByKeepValue(can_use)

				local card_id1 = can_use[1]:getEffectiveId()
				local card_id2 = can_use[2]:getEffectiveId()

				local card_str = ("@ThQiaogong=%d+%d"):format(card_id1, card_id2)
				use.card = sgs.Card_Parse(card_str)
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
	if self:getOverflow() > 0 then
		self:sort(self.enemies, "defense")
		for _, p in ipairs(self.enemies) do
			if p:hasEquip() then
				for _, cd in sgs.qlist(p:getEquips()) do
					local suit = cd:getSuit()
					local target_id = cd:getId()
					local can_use = {}
					local cards = self.player:getCards("he")
					for _, c in sgs.qlist(cards) do
						if c:getSuit() == suit and c:getId() ~= target_id and not isCard("Peach", c, self.player) and not isCard("ExNihilo", c, self.player) then
							table.insert(can_use, c)
						end
					end
					if #can_use >= 2 then
						self:sortByKeepValue(can_use)
		
						local card_id1 = can_use[1]:getEffectiveId()
						local card_id2 = can_use[2]:getEffectiveId()
		
						local card_str = ("@ThQiaogong=%d+%d"):format(card_id1, card_id2)
						use.card = sgs.Card_Parse(card_str)
						if use.to then
							use.to:append(p)
						end
						return
					end
				end
			end
		end
	end
	use.card = nil
end

sgs.ai_choicemade_filter.cardChosen.thqiaogong = sgs.ai_choicemade_filter.cardChosen.snatch

--鬼狱：当你使用【杀】对目标角色造成一次伤害后，你可以令该角色获得此【杀】，然后你选择一项：在结算后，令此【杀】不计入使用限制；或弃置一名其他角色的一张牌。
sgs.ai_skill_invoke.thguiyu = function(self, data)
	local damage = data:toDamage()
	local target = damage.to
	local slash = damage.card:getEffectiveId()
	if self:isFriend(target) and not self:needKongcheng(target, true) then
		return true
	end
	if self:isEnemy(target) and (self:needKongcheng(target, true) or self:willSkipPlayPhase(target)) then
		return true
	end
	if self:isEnemy(target) and not isCard("Jink", slash, target) and not isCard("Peach", slash, target) and not isCard("Analeptic", slash, target) then
		return true
	end
	return false
end

sgs.ai_skill_playerchosen.thguiyu = function(self, targets)
	if self:getCardsNum("Slash") > 0 then
		return nil
	end
	return self:findPlayerToDiscard("he", false, true, targets)
end

sgs.ai_choicemade_filter.cardChosen.thguiyu = sgs.ai_choicemade_filter.cardChosen.snatch

--酎华：限定技，出牌阶段，若存活角色数不大于游戏开始时的一半，你可以回复1点体力或摸两张牌，然后获得技能“坏灭”（锁定技，你的黑色锦囊牌均视为【杀】，且造成的伤害+1。）。
local thzhouhua_skill = {}
thzhouhua_skill.name = "thzhouhua"
table.insert(sgs.ai_skills, thzhouhua_skill)
thzhouhua_skill.getTurnUseCard = function(self)
    local alive = self.room:alivePlayerCount()
    local all = self.room:getPlayers():length()
    if alive <= all / 2 and self.player:getMark("@zhouhua") > 0 then
		return sgs.Card_Parse("@ThZhouhuaCard=.")
	end
end

sgs.ai_skill_use_func.ThZhouhuaCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_choice.thzhouhua = function(self, choice)
	if self.player:getHp() < self.player:getMaxHp() - 1 then return "recover" end
	return "draw"
end

--坏灭：锁定技，你的黑色锦囊牌均视为【杀】，且造成的伤害+1。
--SmartAI:hasHeavySlashDamage

--[[local thzhouhua_skill = {}
thzhouhua_skill.name = "thzhouhua"
table.insert(sgs.ai_skills, thzhouhua_skill)
thzhouhua_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)
	for _, acard in ipairs(cards) do
		if isCard("Analeptic", acard, self.player) then return end
	end
	self:sortByUseValue(cards)
	local newcards = {}
	local has_slash = false
	for _, card in ipairs(cards) do
		if self:getCardsNum("Slash") == 1 and isCard("Slash", card, self.player) then
			continue
		end
		if self:getCardsNum("Slash") == 2 and isCard("Slash", card, self.player) and has_slash then
			continue
		end
		if not isCard("Analeptic", card, self.player) and not isCard("Peach", card, self.player) and not (isCard("ExNihilo", card, self.player) and self.player:getPhase() == sgs.Player_Play) then
			if isCard("Slash", card, self.player) then
				has_slash = true
			end
			table.insert(newcards, card)
		end
	end
	if #newcards <= self.player:getHp() - 1 and self.player:getHp() <= 4 and self:needKongcheng()
		and not (self.player:hasSkill("ikshengtian") and self.player:getMark("@shengtian") == 0) then return end
	if #newcards < 2 then return end

	local card_id1 = newcards[1]:getEffectiveId()
	local card_id2 = newcards[2]:getEffectiveId()

	local card_str = ("analeptic:%s[%s:%s]=%d+%d"):format("thzhouhua", "to_be_decided", 0, card_id1, card_id2)
	local analeptic = sgs.Card_Parse(card_str)
	return analeptic
end

function cardsView_thzhouhua(self, player)
	local cards = player:getCards("he")
	for _, id in sgs.qlist(player:getPile("wooden_ox")) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)
	for _, acard in ipairs(cards) do
		if isCard("Analeptic", acard, player) then return end
	end
	local newcards = {}
	for _, card in ipairs(cards) do
		if not isCard("Analeptic", card, player) and not isCard("Peach", card, player) and not (isCard("ExNihilo", card, player) and player:getPhase() == sgs.Player_Play) then
			table.insert(newcards, card)
		end
	end
	if #newcards < 2 then return end
	sgs.ais[player:objectName()]:sortByKeepValue(newcards)
	
	local card_id1 = newcards[1]:getEffectiveId()
	local card_id2 = newcards[2]:getEffectiveId()
	
	local card_str = ("analeptic:%s[%s:%s]=%d+%d"):format("thzhouhua", "to_be_decided", 0, card_id1, card_id2)
	return card_str
end

function sgs.ai_cardsview.thzhouhua(self, class_name, player)
	if class_name == "Analeptic" and player:getPhase() ~= sgs.Player_NotActive then
		return cardsView_thzhouhua(self, player)
	end
end

function sgs.ai_cardsview.thzhouhuav(self, class_name, player)
	if class_name == "Analeptic" then
		local obj_name = player:property("zhouhua_source"):toString()
		local splayer = self.room:findPlayer(obj_name)
        if splayer and splayer:hasSkill("thzhouhua") then
            return cardsView_thzhouhua(self, player)
		end
	end
end

sgs.ai_skill_playerchosen["@thxugu"] = sgs.ai_skill_playerchosen.zero_card_as_slash]]

--神粥：每当你的人物牌翻至正面朝上时，或受到1点伤害后，你可以从牌堆顶亮出X张牌（X为存活角色的数量，且至多为5），将其中一种类别的牌交给一名角色，并将其余的置入弃牌堆。
sgs.ai_skill_invoke.thshenzhou = true
sgs.thshenzhou_current = false
sgs.ai_skill_choice.thshenzhou = function(self, choices, data)
	local card_ids = data:toIntList()
	local has_peach = false
	local num = {basic = 0,
				 equip = 0,
				 trick = 0}
	for _, id in sgs.qlist(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		if card:isKindOf("Peach") then
			has_peach = true
		end
		num[card:getType()] = num[card:getType()] + 1
	end
	self:sort(self.friends, "defense")
	if has_peach then
		if self:isWeak(self.friends[1]) then
			return "basic"
		end
	end
	if num.basic > num.trick then
		return "basic"
	end
	local current = self.room:getCurrent()
	if current and current:isAlive() and self:isFriend(current) and current:getPhase() ~= sgs.Player_NotActive and current:getPhase() <= sgs.Player_Play then
		sgs.thshenzhou_current = true
		if num.trick > num.basic then
			return "trick"
		elseif num.basic > 0 then
			return "basic"
		else
			return num.trick > num.equip and "trick" or "equip"
		end
	end
	if num.basic > num.trick then
		return "basic"
	elseif num.trick > num.basic then
		return "trick"
	elseif num.basic > 0 then
		local n = math.random(1, 3)
		if n == 1 then
			return "trick"
		else
			return "basic"
		end
	else
		local choice_list = choices:split("+")
		return choice_list[math.random(1, #choice_list)]
	end
end

sgs.ai_skill_playerchosen.thshenzhou = function(self, targets)
	local true_target = nil
	if sgs.thshenzhou_current then
		sgs.thshenzhou_current = false
		true_target = self.room:getCurrent()
	else
		self:sort(self.friends, "defense")
		true_target = self.friends[1]
	end
	local n = math.random(1, 5)
	if n > 2 then
		return true_target
	elseif n == 2 then
		return self.player
	else
		return self.friends[math.random(1, #self.friends)]
	end
end

sgs.ai_playerchosen_intention.thshenzhou = -80

--天流：锁定技，摸牌阶段开始时，你须放弃摸牌，改为进行一次判定：若结果为红桃，你摸三张牌；若结果为方块，你摸两张牌；若结果为梅花，你摸一张牌。
--无

--乾仪：限定技，出牌阶段，你可以将你的人物牌翻面，并令一名角色回复1点体力或摸两张牌。
local thqianyi_skill = {}
thqianyi_skill.name = "thqianyi"
table.insert(sgs.ai_skills, thqianyi_skill)
thqianyi_skill.getTurnUseCard = function(self)
	if self.player:getMark("@qianyi") <= 0 then return end
	return sgs.Card_Parse("@ThQianyiCard=.")
end

sgs.ai_skill_use_func.ThQianyiCard = function(card, use, self)
	if self:isWeak(self.player) then
		use.card = card
		if use.to then
			use.to:append(self.player)
		end
		return
	end
	self:sort(self.friends, "defense")
	if not self.player:faceUp() then
		use.card = card
		if use.to then
			use.to:append(self.friends[1])
		end
		return
	end
	for _, p in ipairs(self.friends) do
		if self:isWeak(p) then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
end

sgs.ai_skill_choice.thqianyi = function(self, choices, data)
	local player = data:toPlayer()
	if not player:isWounded() then
		return "draw"
	elseif self:willSkipPlayPhase(player) then
		return "recover"
	elseif player:hasSkills(sgs.cardneed_skill) and not self:isWeak(player) then
		return "draw"
	elseif self:isWeak(player) or player:hasSkills(sgs.masochism_skill) then
		return "recover"
	end
	return math.random(1, 2) == 1 and "draw" or "recover"
end

sgs.ai_use_priority.ThQianyiCard = -5
sgs.ai_card_intention.ThQianyiCard = -150

--祸祟：出牌阶段限一次，你可以弃置一张手牌并选择你攻击范围内的一名角色，该角色需打出一张【闪】，否则你摸一张牌，或对该角色使用一张不计入使用限制的【杀】。
local thhuosui_skill = {}
thhuosui_skill.name = "thhuosui"
table.insert(sgs.ai_skills, thhuosui_skill)
thhuosui_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThHuosuiCard") then return end
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)

	return sgs.Card_Parse("@ThHuosuiCard=" .. cards[1]:getEffectiveId())
end

sgs.ai_skill_use_func.ThHuosuiCard = function(card, use, self)
	local target = nil
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if self.player:inMyAttackRange(enemy) then
			use.card = card
			if use.to then
				use.to:append(enemy)
			end
			return
		end
	end
	for _, friend in ipairs(self.friends_noself) do
		if self.player:inMyAttackRange(friend) then
			use.card = card
			if use.to then
				use.to:append(friend)
			end
			return
		end
	end
end

sgs.ai_skill_cardask["@thhuosuijink"] = function(self, data, pattern, target)
	if self:isFriend(target) then
		return "."
	end
	for _, card in ipairs(self:getCards("Jink")) do
		return card:toString()
	end
	return "."
end

sgs.ai_skill_cardask["@thhuosui-slash"] = function(self, data, pattern, target)
	if self:getCardsNum("Slash") == 1 then
		return "."
	end
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

sgs.ai_use_priority.ThHuosuiCard = sgs.ai_use_priority.Slash + 0.3

--天滴：当你成为【杀】的目标时，你可以摸一张牌。
sgs.ai_skill_invoke.thtiandi = true

--坤仪：限定技，出牌阶段，你可以将你的人物牌翻面，并对一名其他角色造成1点伤害。
local thkunyi_skill = {}
thkunyi_skill.name = "thkunyi"
table.insert(sgs.ai_skills, thkunyi_skill)
thkunyi_skill.getTurnUseCard = function(self)
	if self.player:getMark("@kunyi") <= 0 then return end
	return sgs.Card_Parse("@ThKunyiCard=.")
end

sgs.ai_skill_use_func.ThKunyiCard = function(card, use, self)
	if not self.player:faceUp() then
		local in_range = {}
		for _, enemy in ipairs(self.enemies) do
			if self:damageIsEffective(enemy, nil, self.player) then
				table.insert(in_range, enemy)
			end
		end
		self:sort(in_range, "hp")
		if #in_range > 0 then
			use.card = card
			if use.to then
				use.to:append(in_range[1])
			end
			return 
		end
	else
		self:sort(self.enemies, "defense")
		for _, p in ipairs(self.enemies) do
			if (self:isWeak(self.player) or self:isWeak(p)) and self:damageIsEffective(p, nil, self.player) then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
end

sgs.ai_card_intention.ThKunyiCard = 80

local thcannue_skill = {}
thcannue_skill.name = "thcannue"
table.insert(sgs.ai_skills, thcannue_skill)
thcannue_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThCannueCard") then return end
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)

	self:sortByUseValue(cards, true)
	
	local card = nil
	for _, cd in ipairs(cards) do
		if cd:getSuit() == sgs.Card_Diamond then
			card = cd
			break
		end
	end

	if not card then return end
	local card_id = card:getEffectiveId()
	local card_str = "@ThCannueCard=" .. card_id
	local skillcard = sgs.Card_Parse(card_str)

	assert(skillcard)
	return skillcard
end

sgs.thcannue_slash_target = nil

sgs.ai_skill_use_func.ThCannueCard = function(card, use, self)
	sgs.thcannue_slash_target = nil
	local original_card = sgs.Sanguosha:getCard(card:getEffectiveId())
	if original_card:isKindOf("Slash") then
		self:sort(self.enemies, "defense")
		for _, enemy in ipairs(self.enemies) do
			for _, friend in ipairs(self.friends_noself) do
				if friend:canSlash(enemy, original_card) then
					sgs.thcannue_slash_target = enemy
					use.card = card
					if use.to then
						use.to:append(friend)
					end
					return
				end
			end
		end
	elseif original_card:isKindOf("Jink") or original_card:isKindOf("Peach") or original_card:isKindOf("Analeptic") then
		self:sort(self.friends_noself, "defense")
		self:sort(self.enemies, "defense")
		for _, friend in ipairs(self.friends_noself) do
			if getKnownCard(friend, self.player, "Slash") > 0 then
				for _, enemy in ipairs(self.enemies) do
					if friend:canSlash(enemy) then
						sgs.thcannue_slash_target = enemy
						use.card = card
						if use.to then
							use.to:append(friend)
						end
						return
					end
				end
			end
		end
	else
		self:sort(self.enemies, "hp")
		for _, enemy in ipairs(self.enemies) do
			if getKnownCard(enemy, self.player, "Slash") == 0 then
				local in_range = {}
				local has_friend = false
				for _, p in sgs.qlist(self.room:getOtherPlayers(enemy)) do
					if not self:isFriend(p) and enemy:canSlash(p) then
						table.insert(in_range, p)
					elseif self:isFriend(p) then
						has_friend = true
					end
				end
				if #in_range == 0 and has_friend then
					continue
				end
				if #in_range > 0 then
					self:sort(in_range, "defense")
					sgs.thcannue_slash_target = in_range[1]
				end
				use.card = card
				if use.to then
					use.to:append(enemy)
				end
				return
			end
		end
	end
end

sgs.ai_skill_playerchosen.thcannue = function(self, targets)
	if sgs.thcannue_slash_target and targets:contains(sgs.thcannue_slash_target) then
		return sgs.thcannue_slash_target
	end
	local enemies = {}
	for _, p in sgs.qlist(targets) do
		if not self:isFriend(p) then
			table.insert(enemies, p)
		end
	end
	if #enemies > 0 then
		self:sort(enemies, "defense")
		return enemies[1]
	end
	return targets:at(math.random(0, targets:length() - 1))
end

sgs.ai_skill_choice.thcannue = function(self, choices, data)
	local target = data:toPlayer()
	if self:isFriend(target) and self:needToThrowArmor(target) and target:getArmor() then
		return string.find(choices, "get") and "get" or "hit"
	elseif self:isEnemy(target) and self:isWeak(target) then
		return "hit"
	end
	local choice_list = choices:split("+")
	return math.random(0, 1) == 0 and choice_list[#choice_list] or choice_list[1]
end

sgs.ai_use_priority.ThCannueCard = sgs.ai_use_priority.Slash + 0.1
sgs.ai_playerchosen_intention.thcannue = 30

thsibao_skill = {}
thsibao_skill.name = "thsibao"
table.insert(sgs.ai_skills, thsibao_skill)
thsibao_skill.getTurnUseCard = function(self)
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local card

	self:sortByKeepValue(cards, true)

	for _, acard in ipairs(cards) do
		if acard:isKindOf("EquipCard") then
			card = acard
			break
		end
	end

	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("analeptic:thsibao[%s:%s]=%d"):format(suit, number, card_id)
	local analeptic = sgs.Card_Parse(card_str)

	if sgs.Analeptic_IsAvailable(self.player, analeptic) then
		assert(analeptic)
		return analeptic
	end
end

sgs.ai_view_as.thsibao = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place == sgs.Player_PlaceHand or card_place == sgs.Player_PlaceEquip then
		if card:isKindOf("EquipCard") then
			return ("analeptic:thsibao[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

function sgs.ai_cardneed.thsibao(to, card, self)
	return card:isKindOf("EquipCard") and getKnownCard(to, self.player, "EquipCard", nil, "he") < 2
end

sgs.ai_skill_invoke.thwangqin = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) and not target:faceUp() then
		return true
	end
	if self:isEnemy(target) and target:faceUp() then
		return true
	end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.thwangqin = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		local target = player:getTag("ThWangqinData"):toPlayer()
		if target:faceUp() then
			sgs.updateIntention(player, target, 50)
		else
			sgs.updateIntention(player, target, -50)
		end
	end
end

sgs.ai_skill_playerchosen.thfusuo = function(self, targets)
	if self.player:getHp() + self:getCardsNum("Peach") <= 2 then
		return nil
	end
	local lord = self.room:getLord()
	if lord and self:isEnemy(lord) and targets:contains(lord) then
		return lord
	end
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if self:isWeak(enemy) and targets:contains(enemy) then
			return enemy
		end
	end
	return nil
end

sgs.ai_playerchosen_intention.thfusuo = 40

--[[sgs.ai_skill_cardask["@thfusuo"] = function(self, data, pattern, target)
	return "."
end]]

local thgelong_skill = {}
thgelong_skill.name = "thgelong"
table.insert(sgs.ai_skills, thgelong_skill)
thgelong_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThGelongCard") and not self.player:isKongcheng()
		then return sgs.Card_Parse("@ThGelongCard=.")
	end
end

sgs.ai_skill_use_func.ThGelongCard = function(card, use, self)
	self:sort(self.enemies, "handcard")
	local min_card = self:getMinCard()
	if not min_card then return end
	local min_point = min_card:getNumber()

	local zhugeliang = self.room:findPlayerBySkillName("ikjingyou")

	sgs.ai_use_priority.ThGelongCard = 7.2
	self:sort(self.enemies)
	self.enemies = sgs.reverse(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("ikjingyou") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			local enemy_min_card = self:getMinCard(enemy)
			local enemy_min_point = enemy_min_card and enemy_min_card:getNumber() or 0
			if min_point < enemy_min_point then
				self.thgelong_card = min_card:getId()
				use.card = sgs.Card_Parse("@ThGelongCard=.")
				if use.to then use.to:append(enemy) end
				return
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			if min_point <= 4 then
				self.thgelong_card = min_card:getId()
				use.card = sgs.Card_Parse("@ThGelongCard=.")
				if use.to then use.to:append(enemy) end
				return
			end
		end
	end
end

function sgs.ai_skill_pindian.thgelong(minusecard, self, requestor)
	if requestor:getHandcardNum() == 1 then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		return cards[1]
	end
	return self:getMinCard()
end

sgs.ai_cardneed.thgelong = function(to, card, self)
	if not self:willSkipPlayPhase(to) and self:getUseValue(card) < 6 then
		return card:getNumber() < 4
	end
end

sgs.ai_card_intention.ThGelongCard = 30
sgs.ai_use_value.ThGelongCard = 8.5

sgs.ai_skill_cardask["@thgelonggive"] = function(self, data, pattern, target)
	if self:isFriend(target) then
		return
	end
	if self:getCardsNum("Peach") > 0 then
		return "."
	end
	local cards = self.player:getHandcards()
	self:sortByUseValue(cards)
	for _, card in sgs.qlist(cards) do
		if not (isCard("Peach", card, self.player) or (isCard("ExNihilo", card, self.player) and self.player:getPhase() == sgs.Player_Play)) then
			return card:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_skill_choice.thgelong = function(self, choices, data)
	local target = data:toPlayer()
	if self:isEnemy(target) and self:isWeak(target) and self:damageIsEffective(target, nil, self.player) then
		return "damage"
	end
	if self:isFriend(target) and self:needToThrowArmor(target) and target:getArmor() then
		return "get"
	end
	return self:damageIsEffective(target, nil, self.player) and "damage" or (string.find(choices, "get") and "get" or "damage")
end

sgs.ai_skill_playerchosen["@thyuanzhou"] = function(self, targets)
	local ps = sgs.QList2Table(targets)
	self:sort(ps, "defense")
	for _, p in ipairs(ps) do
		if self:isFriend(p) and not p:getJudgingArea():isEmpty() then
			return p
		end
	end
	for _, p in ipairs(ps) do
		if self:isEnemy(p) then
			return p
		end
	end
	return nil
end
sgs.ai_choicemade_filter.cardChosen.thyuanzhou = sgs.ai_choicemade_filter.cardChosen.dismantlement


local thdasui_skill = {}
thdasui_skill.name = "thdasui"
table.insert(sgs.ai_skills, thdasui_skill)
thdasui_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThDasuiCard") then return nil end
	if self:getOverflow() < 1 then
		return
	end
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	local n = math.min(self:getOverflow(), 2)
	local int_table = {}
	for i = 1, n, 1 do
		table.insert(int_table, tostring(cards[i]:getId()))
	end
	local card_str = ("@ThDasuiCard=%s"):format(table.concat(int_table, "+"))
	return sgs.Card_Parse(card_str)
end

sgs.ai_skill_use_func.ThDasuiCard = function(card, use, self)
	use.card = card
	return 
end

sgs.ai_use_priority.ThDasuiCard = -5

sgs.ai_skill_invoke.thdasui = function(self, data)
	local target = data:toPlayer()
	return self:isFriend(target)
end

sgs.ai_choicemade_filter.skillInvoke.thdasui = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		local target = player:getTag("ThDasuiData"):toPlayer()
		sgs.updateIntention(player, target, -30)
	end
end

sgs.ai_skill_choice.thfengren = function(self, choices, data)
	local ids = self.player:getPile("dasuipile")
	if ids:length() == 2 then
		for _, id in sgs.qlist(ids) do
			if sgs.Sanguosha:getCard(id):isKindOf("Peach") then
				return "obtain"
			end
		end
		return self.player:isWounded() and "recover" or "obtain"
	end
	return "obtain"
end

sgs.ai_skill_invoke.thkudao = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		if (target:hasSkill("ikcangyou") and not target:getEquips():isEmpty()) or self:needToThrowArmor(target) then
			return true
		end
	end
	if self:isEnemy(target) then
		if target:hasSkills("ikyindie+ikguiyue") and target:getPhase() == sgs.Player_NotActive then return false end
		return true
	end
end

sgs.ai_skill_invoke.thsuilun = true

local thransang_skill = {}
thransang_skill.name = "thransang"
table.insert(sgs.ai_skills, thransang_skill)
thransang_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThRansangCard") and not self.player:isKongcheng()
		then return sgs.Card_Parse("@ThRansangCard=.")
	end
end

sgs.ai_skill_use_func.ThRansangCard = function(card, use, self)
	local trick_num = 0
	for _, card in sgs.qlist(self.player:getHandcards()) do
		if card:isNDTrick() and not card:isKindOf("Nullification") then trick_num = trick_num + 1 end
	end
	self:sort(self.enemies, "handcard")
	local max_card = self:getMaxCard()
	local max_point = max_card:getNumber()

	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("ikjingyou") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			local enemy_max_card = self:getMaxCard(enemy)
			local enemy_max_point = enemy_max_card and enemy_max_card:getNumber() or 100
			if max_point > enemy_max_point then
				self.thransang_card = max_card:getEffectiveId()
				use.card = card
				if use.to then
					use.to:append(enemy)
				end
				return
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("ikjingyou") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			if max_point >= 10 then
				self.thransang_card = max_card:getEffectiveId()
				use.card = card
				if use.to then
					use.to:append(enemy)
				end
				return
			end
		end
	end

	self:sort(self.friends_noself, "handcard")
	for index = #self.friends_noself, 1, -1 do
		local friend = self.friends_noself[index]
		if not friend:isKongcheng() then
			local friend_min_card = self:getMinCard(friend)
			local friend_min_point = friend_min_card and friend_min_card:getNumber() or 100
			if max_point > friend_min_point then
				self.thransang_card = max_card:getEffectiveId()
				use.card = card
				if use.to then
					use.to:append(friend)
				end
				return
			end
		end
	end

	local zhugeliang = self.room:findPlayerBySkillName("ikjingyou")
	if zhugeliang and self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 1 and zhugeliang:objectName() ~= self.player:objectName() then
		if max_point >= 7 then
			self.thransang_card = max_card:getEffectiveId()
			use.card = card
			if use.to then
				use.to:append(zhugeliang)
			end
			return
		end
	end

	for index = #self.friends_noself, 1, -1 do
		local friend = self.friends_noself[index]
		if not friend:isKongcheng() then
			if max_point >= 7 then
				self.thransang_card = max_card:getEffectiveId()
				use.card = card
				if use.to then
					use.to:append(friend)
				end
				return
			end
		end
	end

	if trick_num == 0 and not self:isValuableCard(max_card) then
		for _, enemy in ipairs(self.enemies) do
			if not (enemy:hasSkill("ikjingyou") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() and self:hasLoseHandcardEffective(enemy) then
				self.thransang_card = max_card:getEffectiveId()
				use.card = card
				if use.to then
					use.to:append(enemy)
				end
				return
			end
		end
	end
	return
end

sgs.ai_card_intention.ThRansangCard = 0
sgs.ai_cardneed.thransang = sgs.ai_cardneed.bignumber
sgs.ai_use_priority.ThDasuiCard = sgs.ai_use_priority.FireAttack + 0.1

sgs.ai_skill_invoke.thyanlun = true

local thyanlun_skill = {}
thyanlun_skill.name = "thyanlun"
table.insert(sgs.ai_skills, thyanlun_skill)
thyanlun_skill.getTurnUseCard = function(self)
	local cards = self.player:getCards("h")
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local card
	self:sortByUseValue(cards, true)

	for _, acard in ipairs(cards) do
		if not acard:isRed() then continue end
		if not isCard("Peach", acard, self.player) and (self:getUsePriority(acard) < sgs.ai_use_value.FireAttack or self:getOverflow() > 0) then
			if acard:isKindOf("Slash") and self:getCardsNum("Slash") == 1 then
				local keep
				local dummy_use = { isDummy = true , to = sgs.SPlayerList() }
				self:useBasicCard(acard, dummy_use)
				if dummy_use.card and dummy_use.to:length() > 0 then
					for _, p in sgs.qlist(dummy_use.to) do
						if p:getHp() <= 1 then keep = true break end
					end
					if dummy_use.to:length() > 1 then keep = true end
				end
				if keep then sgs.ai_use_priority.Slash = sgs.ai_use_priority.FireAttack + 0.1 end
			else
				sgs.ai_use_priority.Slash = 2.6
				card = acard
				break
			end
		else
			card = acard
			break
		end
	end

	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("fire_attack:thyanlun[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)

	assert(skillcard)
	return skillcard
end

--【八咫】ai
function checkBazhiHp(player)
	if not player:hasSkills("thyanxing+thbazhi") then return false end
	local preLoseHp = 1
	if player:getLostHp() >=2 then
		preLoseHp = 0
	--elseif player:getLostHp() <=1 and self:getCardsNum("Peach") >0 then
	--严格来说还需要调整peach的优先度，防止扣血后没有八咫效果时先使用了普通杀	
	end
	for _,p in sgs.qlist(player:getRoom():getOtherPlayers(player)) do
		if p:getHp() < player:getHp() - preLoseHp then 
			return true 
		end
	end
	return false
end
sgs.ai_cardneed.thbazhi = function(to, card, self)
	if not self:willSkipPlayPhase(to) and checkBazhiHp(to) 
		and getCardsNum("FireSlash", to, self.player) <1 then
		
		return (card:getSuit()==sgs.Card_Diamond  and card:isKindOf("Jink"))
			or card:isKindOf("Lightning")
	end
end

--【焰星】ai
local thyanxing_skill = {}
thyanxing_skill.name = "thyanxing"
table.insert(sgs.ai_skills, thyanxing_skill)
thyanxing_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThYanxingCard") then return nil end
	if self:getCardsNum("NatureSlash")<1 then return nil end
	if not checkBazhiHp(self.player) then 
		return nil
	end
	--ignore ironchain effect and slash friend who is chained
	local willHit = false
	local cards = self.player:getCards("h")
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	for _, c in ipairs(cards) do
		if c:isKindOf("NatureSlash") then
			local dummy_use = { isDummy = true , to = sgs.SPlayerList() }
			self:useBasicCard(c, dummy_use)
			if dummy_use.card and dummy_use.to:length() > 0 then
				for _, to in sgs.qlist(dummy_use.to) do
					if self:isEnemy(to) and 
					(getCardsNum("Jink", to, self.player) < 1 
					or sgs.card_lack[to:objectName()]["Jink"] == 1 or self:isWeak(to))  then    
						willHit = true
						break
					end
				end
			end
		end
		if willHit then
			break
		end
	end
	if willHit then
		return sgs.Card_Parse("@ThYanxingCard=.")
	end
	return nil
end
sgs.ai_skill_use_func.ThYanxingCard = function(card, use, self)
	use.card = card
end
sgs.ai_skill_choice.thyanxing=function(self, choices)
	if self.player:getLostHp() >=2 then
		return "maxhp"
	else
		return "hp"
	end
	return "maxhp"
end
sgs.ai_use_priority.ThYanxingCard =sgs.ai_use_priority.Slash +0.2

--为什么keepvalue只接受number.....
sgs.thyanxing_keep_value = { 
	NatureSlash = 6.4,
}

--貌似不用调整属性杀的use_priority
--属性杀本来就比普通杀优先
sgs.ai_skill_invoke.thheyu = function(self, data)
	local target = data:toPlayer()
	return not self:isFriend(target) and not self:needToLoseHp(target, self.player, false, true) 
	and not self:getDamagedEffects(target, self.player, false) 
	--其实可以考虑给队友卖血
	--不过需要在filterevent中将此伤害仇恨屏蔽，
	--改为用ai_choicemade_filter.skillInvoke 来增加仇恨。
end

--【埋火】ai
sgs.string2suit = {
        spade = 0 ,
        club = 1 ,
        heart = 2 ,
        diamond = 3
}
local countKnownSuits = function(target)
	local suits = {}
	local knowncards={}
	for _, card in sgs.qlist(target:getHandcards()) do
		--flag的情况其实可以不要。。。
		local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), target:objectName())
		if  card:hasFlag("visible") or card:hasFlag(flag) then	
			table.insert(knowncards,card)
		end
	end
	
	if #knowncards==0 then return 1, nil end
	
	for _,c in pairs(knowncards)do
		local suit = c:getSuitString()
        if not suits[suit] then suits[suit] = 1 end
        suits[suit] = suits[suit] + 1
	end
	local maxsuit = knowncards[1]:getSuitString()
    for s, n in pairs(suits) do
        if n > suits[maxsuit] then maxsuit = s end
    end

		return math.min(suits[maxsuit],3),sgs.string2suit[maxsuit] 
end
local maihuoCompare_func = function(a, b)
	return countKnownSuits(a)> countKnownSuits(b)
end
local thmaihuo_skill = {}
thmaihuo_skill.name = "thmaihuo"
table.insert(sgs.ai_skills, thmaihuo_skill)
function thmaihuo_skill.getTurnUseCard(self)
	if self.player:hasUsed("ThMaihuoCard") then return nil end
	local hearts={}
	for _,c in sgs.qlist (self.player:getHandcards()) do
		if c:getSuit() == sgs.Card_Heart then
			table.insert(hearts,c)
		end
	end
	if #hearts==0 then return nil end
	self:sortByKeepValue(hearts)
	return sgs.Card_Parse("@ThMaihuoCard=" .. hearts[1]:getEffectiveId())
end
sgs.ai_skill_use_func.ThMaihuoCard = function(card, use, self)
	local targets={}
	for _,p in pairs (self.friends_noself) do
		if not self:willSkipPlayPhase(p)  then
			table.insert(targets,p)
		end
	end
	if #targets ==0 then return nil end
	--单纯从埋火摸牌收益考虑 没有考虑cardneed的信息 没有考虑findPlayerToDraw
	table.sort(targets, maihuoCompare_func)
	use.card = card
	if use.to then
		local maihuo_data=sgs.QVariant()
		maihuo_data:setValue(targets[1])
		self.player:setTag("thmaihuo_target",maihuo_data)
		use.to:append(targets[1])
		if use.to:length() >= 1 then return end
	end
end
--ThMaihuoCard 优先度不好拿捏啊。。。
sgs.ai_use_priority.ThMaihuoCard =sgs.ai_use_priority.Peach +0.2
sgs.ai_card_intention.ThMaihuoCard = -70
sgs.ai_skill_suit.thmaihuo = function(self)
    local target= self.player:getTag("thmaihuo_target"):toPlayer()
	if target then
		local num, suit = countKnownSuits(target)
		if suit then
			return suit
		end
	end
    return  sgs.Card_Heart
end

--【无念】ai
--所有需要伤害来源的needDamage都要记得检测持有【无念】技能的attacker
--smart-ai hasTrickEffective
--standardcards-ai slashIsEffective

local thsangzhi_skill = {}
thsangzhi_skill.name = "thsangzhi"
table.insert(sgs.ai_skills, thsangzhi_skill)
thsangzhi_skill.getTurnUseCard = function(self)
	-- not Peach
	if not self.player:hasUsed("ThSangzhiCard") then
		local cards = sgs.QList2Table(self.player:getCards("he"))
		self:sortByKeepValue(cards)
		for _, card in ipairs(cards) do
			if card:isKindOf("EquipCard") and self.player:canDiscard(self.player, card:getId()) then
				return sgs.Card_Parse("@ThSangzhiCard=" .. card:getId())
			end
		end
	end
end

sgs.ai_skill_use_func.ThSangzhiCard = function(card, use, self)
	-- todo
	return
end

local thxinhuav_skill = {}
thxinhuav_skill.name = "thxinhuav"
table.insert(sgs.ai_skills, thxinhuav_skill)
thxinhuav_skill.getTurnUseCard = function(self)
	if self.player:hasFlag("ForbidThXinhua") then return nil end
	if self.player:getKingdom() ~= "kaze" then return nil end

	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)

	local card

	self:sortByUseValue(cards, true)

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Weapon") then
			card = acard
			break
		end
	end

	if not card then
		return nil
	end

	local card_id = card:getEffectiveId()
	local card_str = "@ThXinhuaCard=" .. card_id
	local skillcard = sgs.Card_Parse(card_str)

	assert(skillcard)
	return skillcard
end

sgs.ai_skill_use_func.ThXinhuaCard = function(card, use, self)
	if self:needBear() and self.room:getCardPlace(card:getEffectiveId()) == sgs.Player_PlaceHand then
		return "."
	end
	local targets = {}
	for _, friend in ipairs(self.friends_noself) do
		if friend:hasLordSkill("thxinhua") and not friend:hasFlag("ThXinhuaInvoked") and not friend:hasSkill("manjuan") then
			table.insert(targets, friend)
		end
	end

	if #targets > 0 then
		use.card = card
		self:sort(targets, "defense")
		if use.to then
			use.to:append(targets[1])
		end
	end
end

sgs.ai_card_intention.ThXinhuaCard = -50
