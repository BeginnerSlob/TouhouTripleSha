--客星：准备阶段开始时，你可以从牌堆顶亮出X张牌（X为存活角色的数量，且至多为5），将其中任意数量的非锦囊牌以任意顺序置于牌堆底，然后将其余的牌置入弃牌堆。
sgs.ai_skill_invoke.thkexing = true

sgs.ai_skill_askforag.thkexing = function(self, card_ids)
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		local typeId = card:getTypeId()
		local use = { to = sgs.SPlayerList() }
		self["use" .. sgs.ai_type_name[typeId + 1] .. "Card"](self, card, use)
		if not use.card then return id end
	end
	return -1
end

--神风：出牌阶段限一次，你可将两张相同颜色的牌交给一名体力值比你多的角色，然后该角色须对其距离为1的由你指定的一名角色造成1点伤害。
local thshenfeng_skill = {}
thshenfeng_skill.name = "thshenfeng"
table.insert(sgs.ai_skills, thshenfeng_skill)
thshenfeng_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThShenfengCard") and self.player:getCardCount() > 1 then
		return sgs.Card_Parse("@ThShenfengCard=.")
	end
end

sgs.ai_skill_use_func.ThShenfengCard = function(card, use, self)
	local distanceTo = function(from, to, has_horse)
		local range_fix = 0
		if to:objectName() == self.player:objectName() and has_horse then
			range_fix = range_fix - 1
		end
		return from:distanceTo(to, range_fix)
	end
	local getCardsValue = function(target, card1, card2)
		local value = 2
		if isCard("Peach", card1, target) then
			value = value + 5
		elseif self:isValuableCard(card1, target) then
			value = value + 2
		end
		if isCard("Peach", card2, target) then
			value = value + 5
		elseif self:isValuableCard(card2, target) then
			value = value + 2
		end
		return value
	end
	local getThShenfengValue = function(target, cards, has_horse)
		if self.player:getRole() == "rebel" and target:isLord() then
			for _, p in sgs.qlist(self.room:getOtherPlayers(target)) do
				if distanceTo(target, p, has_horse) == 1 and self:isEnemy(p) and p:getHp() == 1 and self:damageIsEffective(p, nil, target) and self:getAllPeachNum(p) == 0 then
					if not isCard("Peach", card1, target) and not isCard("Peach", card2, target) then
						local card_value = getCardsValue(target, cards[1], cards[2])
						return 7 - card_value
					end
				end
			end
		end
		for _, p in sgs.qlist(self.room:getOtherPlayers(target)) do
			if distanceTo(target, p, has_horse) == 1 and self:isEnemy(p) and p:getHp() == 1 and self:damageIsEffective(p, nil, target) and self:getAllPeachNum(p) == 0 then
				local value = 5
				if p:getRole() == "rebel" then value = value + 3 end
				local card_value = getCardsValue(target, cards[1], cards[2])
				local update = self:isFriend(target) and card_value or -card_value
				return value + update
			end
		end
		for _, p in sgs.qlist(self.room:getOtherPlayers(target)) do
			if distanceTo(target, p, has_horse) == 1 and self:isEnemy(p) and self:damageIsEffective(p, nil, target) then
				local value = 4
				local card_value = getCardsValue(target, cards[1], cards[2])
				local update = self:isFriend(target) and card_value or -card_value
				return value + update
			end
		end
		for _, p in sgs.qlist(self.room:getOtherPlayers(target)) do
			if distanceTo(target, p, has_horse) == 1 and not self:damageIsEffective(p, nil, target) then
				local value = 0
				local card_value = getCardsValue(target, cards[1], cards[2])
				local update = self:isFriend(target) and card_value or -card_value
				return value + update
			end
		end
		return -100
	end
	local comparePlayersByShenfengValue = function(a, b)
		if a.v == b.v then
			return table.indexOf(a) < table.indexOf(b)
		else
			return a.v > b.v
		end
	end
	local compareCardsByShenfengValue = function(a, b)
		if a.target and not b.target then
			return true
		elseif b.target and not a.target then
			return false
		elseif a.target and b.target then
			return a.v > b.v
		else
			return true
		end
	end
	local cards = sgs.QList2Table(self.player:getCards("he"))
	local can_use = {}
	for _, c in ipairs(cards) do
		for _, c2 in ipairs(cards) do
			if c:getEffectiveId() == c2:getEffectiveId() then
				continue
			end
			if c:sameColorWith(c2) then
				table.insert(can_use, { cards = {c, c2} })
			end
		end
	end
	if #can_use == 0 then return end
	local targets = sgs.QList2Table(self.room:getOtherPlayers(self.player))
	self:sort(targets, "handcard")
	for index, sub_t in ipairs(can_use) do
		local cs = sub_t.cards
		local sort_table = {}
		local has_horse = self.player:getDefensiveHorse() and (cs[1]:getEffectiveId() == self.player:getDefensiveHorse():getEffectiveId() or cs[2]:getEffectiveId() == self.player:getDefensiveHorse():getEffectiveId())
		for _, target in ipairs(targets) do
			local value = getThShenfengValue(target, cs, has_horse)
			table.insert(sort_table, { p = target, v = value })
		end
		table.sort(sort_table, comparePlayersByShenfengValue)
		if sort_table[1].v > 0 then
			can_use[index].target = sort_table[1].p
			can_use[index].value = sort_table[1].v
		end
	end
	table.sort(can_use, compareCardsByShenfengValue)
	if can_use[1] and can_use[1].target then
		use.card = sgs.Card_Parse(("@ThShenfengCard=%d+%d"):format(can_use[1].cards[1]:getEffectiveId(), can_use[1].cards[2]:getEffectiveId()))
		if use.to then
			use.to:append(can_use[1].target)
		end
	end
end

sgs.ai_skill_playerchosen.thshenfeng = function(self, targets)
	local source = self.player:getTag("ThShenfengTarget"):toPlayer()
	targets = sgs.QList2Table(targets)
	self:sort(targets, "hp")
	local null = nil
	for _, p in ipairs(targets) do
		if self:isEnemy(p) and self:damageIsEffective(p, nil, source) then
			return p
		end
		if not self:damageIsEffective(p, nil, source) then
			null = p
		end
	end
	return null or targets[#targets]
end

sgs.ai_card_intention.ThShenfengCard = -40

--开海：每当你失去最后的手牌时，可从牌堆底摸一张牌。
sgs.ai_skill_invoke.thkaihai = true

--天宝：锁定技，准备阶段和结束阶段开始时，你须弃置你人物牌上的灵宝牌，然后将一张随机灵宝牌置于你的人物牌上，并获得相应技能直到失去该灵宝牌。
--无

--仙命：锁定技，你须弃置即将进入你装备区的装备牌，每当你以此法弃置牌后，你须发动“天宝”。
--无

--巫道：锁定技，游戏开始时，你失去4点体力。准备阶段开始时，你失去或回复体力至X点（X为你已损失的体力值）。
--smart-ai.lua getBestHp
function sgs.ai_slash_prohibit.thwudao(self, from, to, card)
	if not to:hasSkill("thwudao") then return false end
	if self:isFriend(to, from) then return false end
	if from:hasFlag("IkJieyouUsed") then return false end
	if to:getHp() > getBestHp(to) then return true end
	return false
end

--幻君：锁定技，你的方块【闪】均视为【碎月绮斗】，你手牌中的防具牌均视为【酒】，你获得即将进入你装备区的防具牌。
--smart-ai.lua isCard

--孤高：出牌阶段限一次，你可以与一名其他角色拼点，赢的角色对没有赢的角色造成1点伤害；“千狱”发动后，防止你因没有赢而受到的伤害；“皇仪”发动后，若你赢，视为你此阶段没有发动“孤高”。
local thgugao_skill = {}
thgugao_skill.name = "thgugao"
table.insert(sgs.ai_skills, thgugao_skill)
thgugao_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThGugaoCard") and not self.player:isKongcheng() then
		return sgs.Card_Parse("@ThGugaoCard=.")
	end
end

sgs.ai_skill_use_func.ThGugaoCard = function(card, use, self)
	self:sort(self.enemies, "handcard")
	local max_card = self:getMaxCard()
	if not max_card then return end
	local max_point = max_card:getNumber()

	self:sort(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("ikjingyou") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			local enemy_max_card = self:getMaxCard(enemy)
			local enemy_max_point = enemy_max_card and enemy_max_card:getNumber() or 14
			if max_point > enemy_max_point then
				self.thgugao_card = max_card:getId()
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
				self.thgugao_card = max_card:getId()
				use.card = card
				if use.to then
					use.to:append(enemy)
				end
				return
			end
		end
	end
end

function sgs.ai_skill_pindian.thgugao(minusecard, self, requestor)
	if requestor:getHandcardNum() == 1 then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		return cards[1]
	end
	return self:getMaxCard()
end

sgs.ai_cardneed.thgugao = sgs.ai_cardneed.bignumber

sgs.ai_use_priority.ThGugaoCard = 7.2
sgs.ai_card_intention.ThGugaoCard = 60
sgs.ai_use_value.ThGugaoCard = 8.5

--千狱：觉醒技，一名角色的回合结束后，若你的体力值为1，你须将体力上限减少至1点，并获得技能“狂魔”，然后进行一个额外的回合。
--无

--狂魔：锁定技，专属技，每当你对一名角色造成除【杀】以外的1点伤害后，你增加1点体力上限，然后回复1点体力。
sgs.ai_cardneed.thkuangmo = function(to, card, self)
	return not card:isKindOf("Slash") and sgs.dynamic_value.damage_card[card:getClassName()]
end

--皇仪：觉醒技，“千狱”发动后，准备阶段开始时，若你的体力上限大于4点，你须减少至4点，并摸等同于减少数量的牌，然后回复1点体力，并失去技能“狂魔”。
--无

--肃狐：摸牌阶段开始时，你可放弃摸牌，改为将牌堆顶的一张牌面朝上置于你的人物牌上，称为“面”，然后进行一个额外的出牌阶段。
sgs.ai_skill_invoke.thsuhu = function(self, data)
	return not (self.player:getPile("mask"):length() > 0 and self.player:getHp() == 1 and self:getAllPeachNum() == 0 and self:getCardsNum("Analeptic") == 0)
end

--忿狼：锁定技，专属技，每当你因受到伤害而扣减1点体力后，回复1点体力并摸两张牌。
--无

--乐狮：出牌阶段限一次，你可以选择一种牌的类别并摸两张牌，视为你此阶段没有使用过此类别的牌。
getLeshiString = function(self)
	if self.player:hasSkill("thjingyuan") then
		local cards = self.player:getHandcards()
		for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
			cards:prepend(sgs.Sanguosha:getCard(id))
		end
		cards = sgs.QList2Table(cards)
	
		local turnUse = {}
		local slashAvail = self.slashAvail
		for _, skill in ipairs(sgs.ai_skills) do
			if self.player:hasSkill(skill.name) or (skill.name == "shuangxiong" and self.player:hasFlag("shuangxiong")) then
				local skill_card = skill.getTurnUseCard(self, #cards == 0)
				if skill_card then
					if type(skill_card) == "table" then
						table.insertTable(cards, skill_card)
					else
						table.insert(cards, skill_card)
					end
				end
			end
		end
		self:sortByUseValue(cards)

		for _, card in ipairs(cards) do
			local dummy_use = { isDummy = true }

			local typeId = card:getTypeId()
			self["use" .. sgs.ai_type_name[typeId + 1] .. "Card"](self, card, dummy_use)

			if dummy_use.card then
				if dummy_use.card:isKindOf("Slash") then
					if slashAvail > 0 then
						slashAvail = slashAvail - 1
						table.insert(turnUse, dummy_use.card)
					elseif dummy_use.card:hasFlag("AIGlobal_KillOff") then table.insert(turnUse, dummy_use.card) end
				else
					table.insert(turnUse, dummy_use.card)
				end
			end
		end

		type_table = { basic = "BasicCard", trick = "TrickCard", equip = "EquipCard" }
		for _, card in ipairs(turnUse) do
			if card:isKindOf("SkillCard") then
				continue
			end
			local use = { isDummy = true }
			local typeId = card:getTypeId()
			self["use" .. sgs.ai_type_name[typeId + 1] .. "Card"](self, card, use)
			if not use.card then continue end
			if self.player:hasFlag("thjingyuan_" .. type_table[card:getType()]) then
				return type_table[card:getType()]
			end
		end
		for _, str in pairs(type_table) do
			if self.player:hasFlag("thjingyuan_" .. str) then
				return str
			end
		end
		return "BasicCard"
	else
		return "BasicCard"
	end
end

local thleshi_skill = {}
thleshi_skill.name = "thleshi"
table.insert(sgs.ai_skills, thleshi_skill)
thleshi_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThLeshiCard") and not self:doNotDraw(self.player, 4) then
		return sgs.Card_Parse("@ThLeshiCard=.")
	end
end

sgs.ai_skill_use_func.ThLeshiCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_choice.thleshi = function(self, choices, data)
	return getLeshiString(self)
end

--忧狸：锁定技，你的手牌上限始终为4。每当你的手牌数变化后，若大于四张，须将多余的作为“面”置于你的人物牌上。每当你的“面”的数量不小于两张时，须弃置两张“面”并失去1点体力。
function SmartAI:doNotDraw(target, num, TrickCard)
	target = target or self.player
	num = num or 2
	if TrickCard then
		if TrickCard:isVirtualCard() then
			for _, id in sgs.qlist(TrickCard:getSubcards()) do
				if target:handCards():contains(id) then
					num = num - 1
				end
			end
		else
			if target:handCards():contains(TrickCard:getEffectiveId()) then
				num = num - 1
			end
		end
	end
	if target:hasSkill("thyouli") then
		return target:getHandcardNum() + num > 4 or (target:getHandcardNum() + num == 5 and target:getPile("mask"):length() > 0)
	end
	return false
end

--惊猿：锁定技，出牌阶段，每种类别的牌你只能使用一张。
--无

--彷魂：摸牌阶段结束时，你可以弃置一张牌，然后你计算与所有其他角色的距离始终为1，且你的【杀】均视为【赤雾锁魂】，直到回合结束。
sgs.ai_skill_invoke.thpanghun = function(self, data)
	for _, p in ipairs(self.friends_noself) do
		if getCardsNum("NatureSlash", p, self.player) > 0 or getCardsNum("Fan", p, self.player) > 0 then
			return true
		end
	end
	for _, p in ipairs(self.enemies) do
		if self:getDangerousCard(p) or self:getValuableCard(p) then
			return true
		end
	end
	local black_trick = false
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if c:isKindOf("TrickCard") and c:isBlack() then
			black_trick = true
			break
		end
	end
	if not black_trick then
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if c:isKindOf("Slash") and c:isBlack() then
				black_trick = true
				break
			end
		end
		if black_trick then
			for _, p in ipairs(self.friends) do
				if (p:containsTrick("indulgence") and p:getOverflow() > 0) or p:containsTrick("supply_shortage") then
					return true
				end
			end
		end
	end
	local snatch = self:getCard("Snatch")
	if snatch then
		snatch = sgs.cloneCard("snatch")
		local use = { isDummy = true }
		self:useCardSnatchOrDismantlement(snatch, use)
		if not use.card then
			snatch:setSkillName("thpanghun")
			self:useCardSnatchOrDismantlement(snatch, use)
			if use.card then
				return true
			end
		end
	end
	local slash = self:getCard("Slash")
	if slash then
		slash = sgs.cloneCard("slash")
		local use = { isDummy = true }
		self:useCardSlash(slash, use)
		if not use.card then
			return true
		end
	end
	return false
end

sgs.ai_skill_cardask["@thpanghun"] = function(self, data, pattern)
	if sgs.ai_skill_invoke.thpanghun(self, data) then
		local ret = self:askForDiscard("thpanghun", 1, 1, false, true)
		if self:isValuableCard(sgs.Sanguosha:getCard(ret[1])) then
			return "."
		end
		return "$" .. ret[1]
	end
	return "."
end

--镜悟：出牌阶段限一次，你可以展示一张黑色锦囊牌，然后弃置场上的一张牌。
local thjingwu_skill = {}
thjingwu_skill.name = "thjingwu"
table.insert(sgs.ai_skills, thjingwu_skill)
thjingwu_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThJingwuCard") then
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if c:isKindOf("TrickCard") and c:isBlack() then
				return sgs.Card_Parse("@ThJingwuCard=" .. c:getEffectiveId())
			end
		end
	end
end

sgs.ai_skill_use_func.ThJingwuCard = function(card, use, self)
	local target = self:findPlayerToDiscard("ej", true, true, nil)
	if not target then return end
	use.card = card
	if use.to then
		use.to:append(target)
	end
end

sgs.ai_use_priority.ThJingwuCard = 9.5
sgs.ai_use_value.ThJingwuCard = sgs.ai_use_priority.Dismantlement
sgs.ai_choicemade_filter.cardChosen.thjingwu = sgs.ai_choicemade_filter.cardChosen.snatch

--轮狱：连舞技，当一张你不为其使用的目标（或之一）的牌置入弃牌堆时，你将这张牌置于牌堆顶，然后此阶段结束时你从牌堆底摸一张牌。
sgs.ai_skill_invoke.thlunyu = function(self, data)
	local str = data:toString()
	local str_t = str:split(":")
	local id = str_t[2]
	local card = sgs.Sanguosha:getEngineCard(id)
	local nextAlive = self.player
	repeat
		nextAlive = self.room:findPlayer(nextAlive:getNextAlive(1, false):objectName())
		if nextAlive:objectName() == self.player:objectName() then
			if not nextAlive:faceUp() then
				nextAlive = self.room:findPlayer(nextAlive:getNextAlive(1, false):objectName())
			end
			break
		end
	until nextAlive:faceUp()

	local willUseExNihilo, willRecast
	if self:getCardsNum("ExNihilo") > 0 then
		local ex_nihilo = self:getCard("ExNihilo")
		if ex_nihilo then
			local dummy_use = { isDummy = true }
			self:useTrickCard(ex_nihilo, dummy_use)
			if dummy_use.card then willUseExNihilo = true end
		end
	end
	if self:getCardsNum("IronChain") > 0 then
		local iron_chain = self:getCard("IronChain")
		if iron_chain then
			local dummy_use = { to = sgs.SPlayerList(), isDummy = true, canRecast = true }
			self:useTrickCard(iron_chain, dummy_use)
			if dummy_use.card and dummy_use.to:isEmpty() then willRecast = true end
		end
	end
	if (willUseExNihilo or willRecast) and self.player:getPhase() == sgs.Player_Play then
		if card:isKindOf("Peach") then
			return true
		end
		if card:isKindOf("TrickCard") or card:isKindOf("Indulgence") or card:isKindOf("SupplyShortage") then
			local dummy_use = { isDummy = true }
			self:useTrickCard(card, dummy_use)
			if dummy_use.card then
				return true
			end
		end
		if card:isKindOf("Jink") and self:getCardsNum("Jink") == 0 then
			return true
		end
		if card:isKindOf("Nullification") and self:getCardsNum("Nullification") == 0 then
			return true
		end
		if card:isKindOf("Slash") and self:slashIsAvailable() then
			local dummy_use = { isDummy = true }
			self:useBasicCard(card, dummy_use)
			if dummy_use.card then
				return true
			end
		end
	end

	local hasLightning, hasIndulgence, hasSupplyShortage, hasPurpleSong
	local tricks = nextAlive:getJudgingArea()
	if not tricks:isEmpty() and not nextAlive:containsTrick("YanxiaoCard") then
		local trick = tricks:at(tricks:length() - 1)
		if self:hasTrickEffective(trick, nextAlive) then
			if trick:isKindOf("Lightning") then hasLightning = true
			elseif trick:isKindOf("Indulgence") then hasIndulgence = true
			elseif trick:isKindOf("SupplyShortage") then hasSupplyShortage = true
			elseif trick:isKindOf("PurpleSong") then hasSupplyShortage = true
			end
		end
	end

	if nextAlive:hasSkill("ikmengyang") then
		return self:isEnemy(nextAlive) == card:isRed()
	end
	if nextAlive:hasSkill("yinghun") then
		return self:isFriend(nextAlive)
	end
	if hasLightning and not (nextAlive:hasSkill("qiaobian") and nextAlive:getHandcardNum() > 0) then
		if self:isEnemy(nextAlive) == ((card:getSuit() == sgs.Card_Spade or (card:getSuit() == sgs.Card_Haert and not nextAlive:hasSkill("ikchiqiu") and nextAlive:hasSkill("thanyue"))) and card:getNumber() >= 2 and (card:getNumber() <= 9 or nextAlive:hasSkill("thjiuzhang"))) then
			return true
		end
	end
	if hasIndulgence then
		if self:isFriend(nextAlive) == (card:getSuit() == sgs.Card_Heart or (card:getSuit() == sgs.Card_Spade and nextAlive:hasSkill("ikchiqiu"))) then
			return true
		end
	end
	if hasSupplyShortage and not (nextAlive:hasSkill("qiaobian") and nextAlive:getHandcardNum() > 0) then
		if self:isFriend(nextAlive) == (card:getSuit() == sgs.Card_Club or (card:getSuit() == sgs.Card_Diamond and nextAlive:hasSkill("ikmohua"))) then
			return true
		end
	end
	if hasPurpleSong then
		if self:isEnemy(nextAlive) == (card:getSuit() == sgs.Card_Diamond and nextAlive:hasSkill("ikmohua")) then
			return true
		end
	end
	return false
end

--返魂：专属技，当你处于濒死状态时，你可获得1枚“桜咲”标记且体力回复至1点，然后将你的人物牌翻至正面朝上并重置之。
sgs.ai_skill_invoke.thfanhun = function(self, data)
	local dying = data:toDying()
	local damage = dying.damage
	if self.player:getMark("@bloom") < 3 or not self.player:hasSkill("thmanxiao") then
		return true
	end
	if self:getAllPeachNum() == 0 and self:getCardsNum("Analeptic") == 0 then
		return not (damage and damage.from and self.player:getRole() == "rebel" and self:isFriend(damage.from))
	end
	return false
end

--诱殇：专属技，每当你的非黑桃牌对目标角色造成伤害时，你可以防止此伤害，并令该角色减少1点体力上限，然后获得1枚“桜咲”标记。
sgs.ai_skill_invoke.thyoushang = function(self, data)
	local damage = data:toDamage()
	if self:isFriend(damage.to) then
		return false
	elseif self:isEnemy(damage.to) then
		if damage.damage > 1 then return false end
		if damage.to:isWounded() then return false end
		if (self.player:getMark("@bloom") < 3 or not self.player:hasSkill("thmanxiao")) and self:objectiveLevel(damage.to) > 3 then
			return true
		end
		if self.player:getMark("@bloom") == 3 and #self.friends_noself > 0
				and self:isWeak() and self:getAllPeachNum() == 0 and self:getCardsNum("Analeptic") == 0
				and self:objectiveLevel(damage.to) == 5 then
			return true
		end
	end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.thyoushang = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		local target = player:getTag("ThYoushangData"):toDamage().to
		if target then
			sgs.updateIntention(player, target, 120)
		end
	end
end

--幽雅：每当你受到伤害时，你可以弃置一张牌并令一至X名角色选择一项：打出一张【闪】；或令你获得其一张牌。（X为你的“桜咲”标记的数量）
sgs.ai_skill_use["@@thyouya"] = function(self, prompt, method)
	local ret = self:askForDiscard("thyouya", 1, 1, false, true)
	if self:isValuableCard(sgs.Sanguosha:getCard(ret[1])) then
		return "."
	end
	card_str = "@ThYouyaCard=" .. ret[1]
	local to_table = {}
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if not enemy:isNude() and (getCardsNum("Jink", enemy, self.player) < 1 or sgs.card_lack[enemy:objectName()]["Jink"] == 1) then
			table.insert(to_table, enemy:objectName())
			if #to_table >= self.player:getMark("@bloom") then
				return card_str .. "->" .. table.concat(to_table, "+")
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if not enemy:isNude() and not table.contains(to_table, enemy:objectName()) then
			table.insert(to_table, enemy:objectName())
			if #to_table >= self.player:getMark("@bloom") then
				return card_str .. "->" .. table.concat(to_table, "+")
			end
		end
	end
	return "."
end

sgs.ai_skill_cardask["@thyouya-jink"] = function(self, data, pattern, target)
	if not self.player:isNude() and not self:isFriend(target) then
		return self:getCardId("Jink")
	end
	return "."
end

sgs.ai_card_intention.ThYouyaCard = 50

--满咲：锁定技，你每拥有1枚“桜咲”标记，你的手牌上限便+1。当你拥有4枚“桜咲”标记时，你立即死亡。
--无

--尽戮：出牌阶段限一次，你可以弃置一张手牌并指定一名其他角色，获得其全部的牌。此出牌阶段结束时，你须交给该角色等同于其体力值数量的牌，且结束阶段开始时将你的人物牌翻面。
local thjinlu_skill = {}
thjinlu_skill.name = "thjinlu"
table.insert(sgs.ai_skills, thjinlu_skill)
thjinlu_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThJinluCard") or self.player:isNude() then return end
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
					and not acard:isKindOf("Peach") then
					card_id = acard:getEffectiveId()
					break
				end
			end
		end
	end
	if not card_id then
		if lightning and not self:willUseLightning(lightning) then
			card_id = lightning:getEffectiveId()
		else
			for _, acard in ipairs(cards) do
				if (acard:isKindOf("BasicCard") or acard:isKindOf("EquipCard") or acard:isKindOf("AmazingGrace"))
					and not acard:isKindOf("Peach") then
					card_id = acard:getEffectiveId()
					break
				end
			end
		end
	end
	if not card_id then
		return nil
	else
		return sgs.Card_Parse("@ThJinluCard=" .. card_id)
	end
end

sgs.ai_skill_use_func.ThJinluCard = function(card, use, self)
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)

	if not self.player:hasUsed("ThJinluCard") then
		self:sort(self.enemies, "cardcount")
		self.enemies = sgs.reverse(self.enemies)
		local target
		for _, enemy in ipairs(self.enemies) do
			if not enemy:isNude() and not enemy:hasSkill("ikjingyou") then
				if (self:needKongcheng(enemy) and not enemy:isKongcheng() and self:damageMinusHp(self, enemy, 1) > 0)
					or (enemy:getHp() < 3 and self:damageMinusHp(self, enemy, 0) > 0 and enemy:getCardCount() > 0)
					or (enemy:getCardCount() >= enemy:getHp() and enemy:getHp() > 2 and self:damageMinusHp(self, enemy, 0) >= -1)
					or (enemy:getCardCount() - enemy:getHp() > 2) then
					target = enemy
					break
				end
			end
		end
		if not self.player:faceUp() and not target then
			for _, enemy in ipairs(self.enemies) do
				if not enemy:isNude() then
					if enemy:getCardCount() >= enemy:getHp() then
						target = enemy
						break
					end
				end
			end
		end

		if not target and (self:hasCrossbowEffect() or self:getCardsNum("Crossbow") > 0) then
			local slash = self:getCard("Slash") or sgs.cloneCard("slash")
			for _, enemy in ipairs(self.enemies) do
				if self:slashIsEffective(slash, enemy) and self.player:distanceTo(enemy) == 1
					and not enemy:hasSkills("thzhehui|ikzhichi|ikhuanji|nosfankui|ikaoli|vsganglie|nosganglie|enyuan|thfusheng|langgu|guixin|ikjingyou")
					and self:getCardsNum("Slash") + getKnownCard(enemy, self.player, "Slash") >= 3 then
					target = enemy
					break
				end
			end
		end

		if target then
			use.card = card
			if use.to then use.to:append(target) end
		end
	end
end

function SmartAI:isThJinluTarget(player, drawCardNum)
	player = player or self.player
	drawCardNum = drawCardNum or 1
	if type(player) == "table" then
		if #player == 0 then return false end
		for _, ap in ipairs(player) do
			if self:isThJinluTarget(ap, drawCardNum) then return true end
		end
		return false
	end

	local handCardNum = player:getHandcardNum() + drawCardNum

	local sb_diaochan = self.room:findPlayerBySkillName("thjinlu")
	local thjinlu = sb_diaochan and not sb_diaochan:hasUsed("ThJinluCard") and not self:isFriend(sb_diaochan)

	if not thjinlu then return false end

	if sb_diaochan:getPhase() == sgs.Player_Play then
		if (handCardNum - player:getHp() >= 2)
			or (handCardNum > 0 and handCardNum - player:getHp() >= -1 and not sb_diaochan:faceUp()) then
			return true
		end
	else
		if sb_diaochan:faceUp() and not self:willSkipPlayPhase(sb_diaochan)
			and self:playerGetRound(player) > self:playerGetRound(sb_diaochan) and handCardNum >= player:getHp() + 2 then
			return true
		end
	end

	return false
end

sgs.ai_skill_discard.thjinlu = function(self, discard_num, min_num, optional, include_equip)
	local to_discard = {}

	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	local card_ids = {}
	for _, card in ipairs(cards) do
		table.insert(card_ids, card:getEffectiveId())
	end

	local temp = table.copyFrom(card_ids)
	for i = 1, #temp, 1 do
		local card = sgs.Sanguosha:getCard(temp[i])
		if self.player:getArmor() and temp[i] == self.player:getArmor():getEffectiveId() and self:needToThrowArmor() then
			table.insert(to_discard, temp[i])
			table.removeOne(card_ids, temp[i])
			if #to_discard == discard_num then
				return to_discard
			end
		end
	end

	temp = table.copyFrom(card_ids)

	for i = 1, #card_ids, 1 do
		local card = sgs.Sanguosha:getCard(card_ids[i])
		table.insert(to_discard, card_ids[i])
		if #to_discard == discard_num then
			return to_discard
		end
	end

	if #to_discard < discard_num then return {} end
end

sgs.ai_use_value.ThJinluCard = 8.5
sgs.ai_use_priority.ThJinluCard = 6
sgs.ai_card_intention.ThJinluCard = 80

--狂戾：每当你的人物牌翻面时，你可以摸一张牌。
--thicket-ai.lua toTurnOver

--愈心：摸牌阶段开始时，你可以放弃摸牌▶亮出牌堆顶的两张牌，然后获取这些牌，若这些牌均为红色，你可以令一名角色选择一项：1.回复1点体力；2.摸一张牌。
sgs.ai_skill_invoke.thyuxin = function(self, data)
	local draw_pile = self.room:getDrawPile()
	if draw_pile:length() < 2 then
		return true
	end
	local id1 = self.room:getDrawPile():first()
	local id2 = self.room:getDrawPile():at(1)
	local card1 = sgs.Sanguosha:getCard(id1)
	local card2 = sgs.Sanguosha:getCard(id2)
	if card1:hasFlag("visible") and card2:hasFlag("visible") and not (card1:isRed() and card2:isRed()) then
		return false
	end
	local target = self:findPlayerToRecover()
	if target then
		return true
	else
		return math.random(1, 2) == 1
	end
end

sgs.ai_skill_playerchosen.thyuxin = function(self, targets)
	local target = self:findPlayerToRecover(1, targets)
	if target then
		sgs.thyuxin_str = "recover"
		return target
	end
	sgs.thyuxin_str = "draw"
	return self:findPlayerToDraw(true, 1)
end

sgs.ai_skill_choice.thyuxin = function(self, choices, data)
	return sgs.thyuxin_str
end

sgs.ai_playerchosen_intention.thyuxin = -100

--疮心：出牌阶段开始时，你可弃置X张牌，然后选择X项：1. 获得技能“灵视”，直到回合结束2. 获得技能“闭月”，直到回合结束
sgs.ai_skill_use["@@thchuangxin"] = function(self, prompt, method)
	local needGongxin = false
	local useBiyueValue, useGongxinValue = false, false
	if self:isWeak() then
		useBiyueValue = true
	end
	for _, p in ipairs(self.enemies) do
		local flag = ("%s_%s_%s"):format("visible", self.player:objectName(), p:objectName())
		local has_peach = false
		for _, c in sgs.qlist(p:getHandcards()) do
			if c:hasFlag("visible") or c:hasFlag(flag) then
				if isCard("Peach", c, p) and c:getSuit() == sgs.Card_Heart then
					has_peach = true
					break
				end
			end
		end
		if has_peach then
			needGongxin = true
			useGongxinValue = true
			break
		end
	end
	if self.player:getHandcardNum() > 3 then
		needGongxin = true
	end
	local needNum = 1
	if needGongxin then
		needNum = needNum + 1
	end
	local valueNum = 0
	if useBiyueValue then
		valueNum = valueNum + 1
	end
	if useGongxinValue then
		valueNum = valueNum + 1
	end
	if needNum < valueNum then
		self.room:writeToConsole("ThChuangxin F**k Dog!!!")
		needNum = valueNum
	end
	local use_cards = {}
	local ret1 = self:askForDiscard("thchuangxin", 1, 1, false, true)
	if #ret1 == 1 then
		local id1 = ret1[1]
		local card1 = sgs.Sanguosha:getCard(id1)
		if valueNum > 0 and not isCard("Peach", card1, self.player) then
			table.insert(use_cards, id1)
		elseif not self:isValuableCard(card1) then
			local type = card1:getTypeId()
			local use = { isDummy = true }
			self["use" .. sgs.ai_type_name[type + 1] .. "Card"](self, card1, use)
			if not use.card then
				table.insert(use_cards, id1)
			end
		end
		if #use_cards == 1 and needNum > 1 then
			local ret2 = self:askForDiscard("thchuangxin", 2, 2, false, true)
			if #ret2 == 2 then
				local id2 = ret2[2]
				local card2 = sgs.Sanguosha:getCard(id2)
				if valueNum > 1 and not isCard("Peach", card2, self.player) then
					table.insert(use_cards, id2)
				elseif not self:isValuableCard(card2) then
					local type = card2:getTypeId()
					local use = { isDummy = true }
					self["use" .. sgs.ai_type_name[type + 1] .. "Card"](self, card2, use)
					if not use.card then
						table.insert(use_cards, id2)
					end
				end
			end
		end
	end
	if #use_cards > 0 then
		if #use_cards == 1 then
			sgs.thchuangxin_choice = "ikbiyue"
			if not useBiyueValue and useGongxinValue then
				sgs.thchuangxin_choice = "iklingshi"
			end
		end
		return "@ThChuangxinCard=" .. table.concat(use_cards, "+")
	end
	return "."
end

sgs.ai_skill_choice.thchuangxin = function(self, choices, data)
	return sgs.thchuangxin_choice
end

--天心：你的回合开始时，你可弃置X张牌，然后选择X项：1. 获得技能“预悉”，直到回合结束2. 获得技能“天妒”，直到回合结束
sgs.ai_skill_use["@@thtianxin"] = function(self, prompt, method)
	local needGuanxing, needTiandu = false
	if self.player:containsTrick("indulgence") or self.player:containsTrick("supply_shortage") or self.player:containsTrick("lightning") then
		needGuanxing = true
	end
	if self.player:containsTrick("supply_shortage") and self.player:getJudgingArea():length() > 1 then
		needTiandu = true
	end
	if self:getOverflow() > 0 then
		needGuanxing = true
	end
	local weak = self:findPlayerToRecover()
	if weak and self:isFriend(weak) and self.player:getHandcardNum() > 1 then
		needGuanxing = true
	end
	local needNum = 0
	if needGuanxing then
		needNum = needNum + 1
	end
	if needTiandu then
		needNum = needNum + 1
	end
	if needNum == 0 then return "." end
	local use_cards = {}
	local ret1 = self:askForDiscard("thtianxin", 1, 1, false, true)
	if #ret1 == 1 then
		local id1 = ret1[1]
		if not isCard("Peach", id1, self.player) or self:willSkipPlayPhase() then
			table.insert(use_cards, id1)
		end
		if #use_cards == 1 and needNum > 1 then
			local ret2 = self:askForDiscard("thtianxin", 2, 2, false, true)
			if #ret2 == 2 then
				local id2 = ret2[2]
				if not isCard("Peach", id2, self.player) or self:willSkipPlayPhase() then
					table.insert(use_cards, id2)
				end
			end
		end
	end
	if #use_cards > 0 then
		if #use_cards == 1 then
			sgs.thtianxin_choice = "ikxushi"
			if not needGuanxing and needTiandu then
				sgs.thtianxin_choice = "iktiandu"
			end
		end
		return "@ThTianxinCard=" .. table.concat(use_cards, "+")
	end
	return "."
end

sgs.ai_skill_choice.thtianxin = function(self, choices, data)
	return sgs.thtianxin_choice
end

--禳灯：锁定技，出牌阶段开始时，你须选择一项：依次将至多三张与你人物牌上的任何一张牌点数都不相同的手牌面朝上置于你的人物牌上，称为“灯”；或弃置一张手牌。你的人物牌上每有一种花色的“灯”，你获得相应的技能：红桃“闭月”；黑桃“飞影”；方块“沉红”；梅花“霁风”。
sgs.ai_skill_cardask["@thrangdeng"] = function(self, data, pattern, target, target2, arg, arg2)
	local has_suits, has_num = {}, {}
	for _, id in sgs.qlist(self.player:getPile("lantern")) do
		local c = sgs.Sanguosha:getCard(id)
		if not table.contains(has_suits, c:getSuit()) then
			table.insert(has_suits, c:getSuit())
		end
		table.insert(has_num, c:getNumber())
	end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards, true)
	if #has_suits < 4 then
		for _, c in ipairs(cards) do
			if not table.contains(has_suits, c:getSuit()) and not table.contains(has_num, c:getNumber()) then
				return "$" .. c:getEffectiveId()
			end
		end
	end
	for _, c in ipairs(cards) do
		if not table.contains(has_num, c:getNumber()) then
			if #has_num + (4 - arg) >= 13 or arg == 1 or not self:isValuableCard(c) then
				return "$" .. c:getEffectiveId()
			end
		end
	end
	return "."
end

sgs.ai_cardneed.thrangdeng = function(to, card)
	local has_suits, has_num = {}, {}
	for _, id in sgs.qlist(to:getPile("lantern")) do
		local c = sgs.Sanguosha:getCard(id)
		if not table.contains(has_suits, c:getSuit()) then
			table.insert(has_suits, c:getSuit())
		end
		table.insert(has_num, c:getNumber())
	end
	return not table.contains(has_suits, card:getSuit()) and not table.contains(has_num, card:getNumber())
end

--拜魂：出牌阶段，你可以将十三张“灯”置入弃牌堆，然后令一名其他角色立即死亡。
local thbaihun_skill = {}
thbaihun_skill.name = "thbaihun"
table.insert(sgs.ai_skills, thbaihun_skill)
thbaihun_skill.getTurnUseCard = function(self)
	if self.player:getPile("lantern"):length() >= 13 then
		return sgs.Card_Parse("@ThBaihunCard=.")
	end
end

sgs.ai_skill_use_func.ThBaihunCard = function(card, use, self)
	if self.player:getRole() == "rebel" then
		use.card = card
		if use.to then
			use.to:append(self.room:getLord())
		end
		return
	end
	self:sort(self.enemies, "defense")
	self.enemies = sgs.reverse(self.enemies)
	for _, p in ipairs(self.enemies) do
		if self:objectiveLevel(p) == 5 then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
end

sgs.ai_cardneed.thbaihun = function(to, card)
	local has_num = {}
	for _, id in sgs.qlist(to:getPile("lantern")) do
		local c = sgs.Sanguosha:getCard(id)
		table.insert(has_num, c:getNumber())
	end
	return not table.contains(has_num, card:getNumber()) and #has_num > 9
end

sgs.ai_card_intention.ThBaihunCard = 998

--虚境：你的回合外，每当你成为以下牌的目标后，你可以失去一项人物技能并摸一张牌，然后令此牌的使用者获得相应技能，直到你的下一个回合结束：1.黑色非延时类锦囊牌-“幻葬”和“暗月”；2.你的【桃】或【酒】-“霁风”和“隙境”。
sgs.ai_skill_invoke.thxujing = function(self, data)
	local has_useless_skill = false
	for _, skill in sgs.qlist(self.player:getVisibleSkillList()) do
		if skill:isAttachedLordSkill() or skill:objectName() == "thxujing" or skill:objectName() == "thlingyun" then
			continue
		end
		if skill:objectName() == "thzhaoai" then
			if #self.friends_noself == 0 and #self.enemies == self.player:aliveCount() - 1 then
				has_useless_skill = true
				break
			else
				continue
			end
		end
		has_useless_skill = true
		break
	end
	if not has_useless_skill then
		return false
	end
	local target = data:toCardUse().from
	if target:objectName() == self.player:objectName() then
		return has_useless_skill
	end
	if not target or target:isDead() or self:isEnemy(target) then
		return has_useless_skill
	end
	return false
end

sgs.ai_skill_choice.thxujing = function(self, choices, data)
	local skills = {}
	local discard_zhaoai = false
	for _, skill in sgs.qlist(self.player:getVisibleSkillList()) do
		if skill:isAttachedLordSkill() or skill:objectName() == "thxujing" or skill:objectName() == "thlingyun" then
			continue
		end
		if skill:objectName() == "thzhaoai" then
			if #self.friends_noself == 0 and #self.enemies == self.player:aliveCount() - 1 then
				discard_zhaoai = true
			else
				continue
			end
		end
		table.insert(skills, skill:objectName())
	end
	if #skills > 0 then
		return table.contains(skills, "thsanling") and "thsanling" or skills[math.random(1, #skills)]
	end
	if discard_zhaoai then
		return "thzhaoai"
	end
	self.room:writeToConsole("ThXujing No Skill!")
end

sgs.ai_choicemade_filter.skillInvoke.thxujing = function(self, player, promptlist)
	local has_useless_skill = false
	for _, skill in sgs.qlist(player:getVisibleSkillList()) do
		if skill:isAttachedLordSkill() or skill:objectName() == "thxujing" or skill:objectName() == "thlingyun" then
			continue
		end
		if skill:objectName() == "thzhaoai" then
			if #self:getFriendsNoself(player) == 0 and #self:getEnemies(player) == player:aliveCount() - 1 then
				has_useless_skill = true
				break
			else
				continue
			end
		end
		has_useless_skill = true
		break
	end
	local target = player:getTag("ThXujingTarget"):toCardUse().from
	local invoke = (promptlist[#promptlist] == "yes")
	if invoke then
		if target and target:isAlive() and player:objectName() ~= target:objectName() then
			local intention = has_useless_skill and 40 or 100
			sgs.updateIntention(player, target, intention)
		end
	else
		if target and target:isAlive() and player:objectName() ~= target:objectName() then
			local intention = has_useless_skill and -20 or 0
			sgs.updateIntention(player, target, intention)
		end
	end
end

--灵殒：出牌阶段，若你不拥有相应技能，你可以弃置一张牌，并获得该技能，视为使用以下一张牌：1.【绯想镜诗】－“崩坏”；2.【灼狱业焰】－“散灵”；3.【心网密葬】－“心殇”；4.【赤雾锁魂】－“禁恋”。
local thlingyun_skill = {}
thlingyun_skill.name = "thlingyun"
table.insert(sgs.ai_skills, thlingyun_skill)
thlingyun_skill.getTurnUseCard = function(self)
	if not self.player:canDiscard(self.player, "he") then return end
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByUseValue(cards, true)
	for _, c in ipairs(cards) do
		return sgs.Card_Parse("@ThLingyunCard=" .. c:getEffectiveId())
	end
end

sgs.ai_skill_use_func.ThLingyunCard = function(card, use, self)
	local original_card = sgs.Sanguosha:getCard(card:getEffectiveId())
	--ex_nihilo
	if not self.player:hasSkill("ikbenghuai") and self:getUseValue(original_card) < sgs.ai_use_value.ExNihilo then
		local ex_nihilo = sgs.cloneCard("ex_nihilo")
		ex_nihilo:setSkillName("_thlingyun")
		if ex_nihilo:isAvailable(self.player) and not self.player:isCardLimited(ex_nihilo, sgs.Card_MethodUse) then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useCardExNihilo(ex_nihilo, dummy_use)
			if dummy_use.card then
				use.card = sgs.Card_Parse("@ThLingyunCard=" .. original_card:getEffectiveId() .. ":ex_nihilo")
				if use.to then
					use.to = dummy_use.to
				end
				return
			end
		end
	end
	--iron_chain
	if not self.player:hasSkill("ikjinlian") and self:getUseValue(original_card) < sgs.ai_use_value.IronChain then
		local iron_chain = sgs.cloneCard("iron_chain")
		iron_chain:setSkillName("_thlingyun")
		if iron_chain:isAvailable(self.player) and not self.player:isCardLimited(iron_chain, sgs.Card_MethodUse) then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useCardIronChain(iron_chain, dummy_use)
			if dummy_use.card then
				use.card = sgs.Card_Parse("@ThLingyunCard=" .. original_card:getEffectiveId() .. ":iron_chain")
				if use.to then
					use.to = dummy_use.to
				end
				return
			end
		end
	end
	--dismantlement
	if not self.player:hasSkill("ikxinshang") and self:getUseValue(original_card) < sgs.ai_use_value.Dismantlement then
		local dismantlement = sgs.cloneCard("dismantlement")
		dismantlement:setSkillName("_thlingyun")
		if dismantlement:isAvailable(self.player) and not self.player:isCardLimited(dismantlement, sgs.Card_MethodUse) then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useCardSnatchOrDismantlement(dismantlement, dummy_use)
			if dummy_use.card then
				use.card = sgs.Card_Parse("@ThLingyunCard=" .. original_card:getEffectiveId() .. ":dismantlement")
				if use.to then
					use.to = dummy_use.to
				end
				return
			end
		end
	end
	--fire_attack
	local n = 0
	for _, skill in sgs.qlist(self.player:getVisibleSkillList()) do
		if skill:isAttachedLordSkill() then continue end
		n = n + 1
	end
	if not self.player:hasSkill("thsanling") and n >= 5 and #self.friends_noself > 0 then
		local fire_attack = sgs.cloneCard("fire_attack")
		fire_attack:setSkillName("_thlingyun")
		if fire_attack:isAvailable(self.player) and not self.player:isCardLimited(fire_attack, sgs.Card_MethodUse) then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useCardFireAttack(fire_attack, dummy_use)
			if dummy_use.card then
				use.card = sgs.Card_Parse("@ThLingyunCard=" .. original_card:getEffectiveId() .. ":fire_attack")
				if use.to then
					use.to = dummy_use.to
				end
				return
			end
		end
	end
end

--朝霭：锁定技，你死亡时，令一名其他角色获得技能“神宝”和“浴火”，并摸X张牌（X为你的技能数量）。
sgs.ai_skill_playerchosen.thzhaoai = function(self, targets)
	if #self.friends_noself == 0 then return nil end
	self:sort(self.friends_noself, "defense")
	for _, p in ipairs(self.friends_noself) do
		if self:objectiveLevel(p) <= -2 then
			if not p:hasSkills("thshenbao+thyuhuo") then
				return p
			end
		end
	end
	for _, p in ipairs(self.friends_noself) do
		if self:objectiveLevel(p) <= -2 then
			if not p:hasSkill("thshenbao") or not p:hasSkill("thyuhuo") then
				return p
			end
		end
	end
	for _, p in ipairs(self.friends_noself) do
		if self:objectiveLevel(p) <= -2 then
			return p
		end
	end
	return self.friends_noself[1]
end

sgs.ai_playerchosen_intention.thzhaoai = -150

--五难：当一名其他角色使用【神惠雨降】或【竹取谜宝】时，或当一名其他角色于濒死状态回复体力后（若其体力值不小于1），或当一名其他角色受到火属性伤害后，或当一名其他角色使用【净琉璃镜】的效果而使用或打出【闪】时，或当一名其他角色使用【杀】对没有手牌的角色造成伤害时，你可以选择一项：1.摸一张牌；2.弃置该角色的一张牌。然后若你是体力值最小的角色，你可以回复1点体力。每回合限一次。
sgs.ai_skill_invoke.thwunan = true

sgs.ai_skill_choice.thwunan = function(self, choices, data)
	local target = data:toPlayer()
	local players = sgs.SPlayerList()
	players:append(target)
	choices = choices:split("+")
	if target and target:isAlive() and self:isEnemy(target) then
		if target:getHandcardNum() < 3 and self:findPlayerToDiscard("he", true, true, players) and table.contains(choices, "discard") then
			return "discard"
		end
	elseif target and target:isAlive() and self:isFriend(target) then
		if self:findPlayerToDiscard("he", true, true, players) and table.contains(choices, "discard") then
			return "discard"
		end
	end
	if target then
		if (self:getOverflow(target) <= 0 and self.player:canDiscard(target, "h") and table.contains(choices, "discard")) or (target:hasEquip() and self.player:canDiscard(target, "e") and table.contains(choices, "discard")) then
			return "discard"
		end
	end
	return "draw"
end

sgs.ai_choicemade_filter.cardChosen.thwunan = sgs.ai_choicemade_filter.cardChosen.snatch

--散灵：锁定技，一名角色的回合结束后，若你没有手牌，你立即死亡。
sgs.ai_cardneed.thsanling = function(to, card, self)
	return not self:needDeath(to) and self:getOverflow(to) < 0
end

--氷障：每当你受到伤害结算开始时或即将失去体力时，你可以弃置两张牌，然后防止此伤害或此次失去体力。
sgs.ai_skill_use["@@thbingzhang"] = function(self, prompt, method)
	local ret = self:askForDiscard("thbingzhang", 2, 2, false, true)
	if #ret ~= 2 then
		return "."
	end
	if self.player:getCardCount() == 2 then
		local card1 = sgs.Sanguosha:getCard(ret[1])
		local card2 = sgs.Sanguosha:getCard(ret[2])
		if not card1:isKindOf("Jink") and not card2:isKindOf("Jink") then
			return "."
		end
	end
	return "@ThBingzhangCard=" .. table.concat(ret, "+")
end

--极武：锁定技，你的【桃】和【酒】均视为【闪】。你的回合外，每当你的【闪】、【桃】或【酒】置入弃牌堆或其他角色每获得你的一张手牌时，你摸一张牌。
--smart-ai.lua isCard

--祀祟：专属技，准备阶段开始时，你可以令所有其他角色各弃置一张手牌，你从弃牌堆获得这些牌中的至多三张牌，然后你可以依次交给一名其他角色。
sgs.ai_skill_invoke.thsisui = true

sgs.ai_skill_askforyiji.thsisui = sgs.ai_skill_askforyiji.ikyumeng

--湛樱：锁定技，你始终跳过你的摸牌阶段；你的手牌上限+4。
--standard-ai.lua SmartAI:willSkipDrawPhase

--陆离：当你对体力值不小于你的一名其他角色造成伤害，或一名体力值不小于你的其他角色对你造成伤害时，你可以弃置一张黑色手牌令你造成的伤害+1；或弃置一张红色手牌令你受到的伤害-1。
sgs.ai_skill_cardask["@thluli-increase"] = function(self, data)
	local damage = data:toDamage()
	local target = damage.to
	if self:isFriend(target) then return "." end
	if self:hasSilverLionEffect(target) then return "." end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		if card:isBlack() then return "$" .. card:getEffectiveId() end
	end
	return "."
end

sgs.ai_skill_cardask["@thluli-decrease"] = function(self, data)
	local damage = data:toDamage()
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	if damage.card and damage.card:isKindOf("Slash") then
		if self:hasHeavySlashDamage(damage.from, damage.card, self.player) then
			for _, card in ipairs(cards) do
				if card:isRed() then return "$" .. card:getEffectiveId() end
			end
		end
	end
	if self:getDamagedEffects(self.player, damage.from) and damage.damage <= 1 then return "." end
	if self:needToLoseHp(self.player, damage.from) and damage.damage <= 1 then return "." end
	for _, card in ipairs(cards) do
		if card:isRed() then return "$" .. card:getEffectiveId() end
	end
	return "."
end

function sgs.ai_cardneed.thluli(to, card)
	return to:getHandcardNum() < 4 and (to:getHp() >= 3 or card:isRed())
end

--诡幻：限定技，若你的身份不是君主，当你杀死一名身份不是君主的其他角色，在其翻开身份牌之前，你可以与该角色交换身份牌。
sgs.ai_skill_invoke.thguihuan = function(self, data)
	local target = data:toPlayer()
	local target_role = sgs.evaluatePlayerRole(target)
	local self_role = self.player:getRole()
	if target_role == "renegade" or target_role == "neutral" then return false end
	local process = sgs.gameProcess(self.room)
	return (target_role == "rebel" and self.role ~= "rebel" and process:match("rebel"))
			or (target_role == "loyalist" and self.role ~= "loyalist" and process:match("loyal"))
end

--至尊：准备阶段和结束阶段开始时，你可以改变一名其他角色的势力属性，然后可以获得一项技能（你只能以此法获得“心契”、“唤卫”、“济援”、“御姬”、“华袛”、“颂威”、“春度”或“舞华”，且无法获得场上其他存活角色拥有的以上技能）。
sgs.ai_skill_invoke.thzhizun = true

sgs.thzhizun_kingdoms_map = {
	["thfeizhan"] = "hana",
	["thchundu"] = "yuki",
	["thhuazhi"] = "kaze",
	["thtianchan"] = "hana",
	["thfuyue"] = "yuki",
	["ikjiyuan"] = "yuki",
	["ikwuhua"] = "tsuki",
	["ikyuji"] = "tsuki",
	["iksongwei"] = "hana",
	["ikhuanwei"] = "hana",
	["ikxinqi"] = "kaze",
	["ikbiansheng"] = "yuki",
	["thxinhua"] = "kaze",
	["thyejun"] = "tsuki",
	["thyunyin"] = "tsuki"
}

local function findPlayerForModifyKingdom(self, players)
	if players and not players:isEmpty() then
		for _, player in sgs.qlist(players) do
			if player:hasSkill("huashen") then continue end
			local good_table, bad_table = {}, {}
			for _, lord in sgs.qlist(self.room:getOtherPlayers(player)) do
				if self:isFriend(lord) then
					for skill, kingdom in pairs(sgs.thzhizun_kingdoms_map) do
						if lord:hasLordSkill(skill) then
							table.insert(good_table, kingdom)
						end
					end
				else
					for skill, kingdom in pairs(sgs.thzhizun_kingdoms_map) do
						if lord:hasLordSkill(skill) and (skill == "thfeizhan" or skill == "thchundu") then
							table.insert(bad_table, kingdom)
						end
					end
				end
			end
			if self:isFriend(player) and table.contains(good_table, player:getKingdom()) then
				continue
			end
			if not self:isFriend(player) and not table.contains(bad_table, player:getKingdom()) then
				continue
			end
			return player
		end
		for _, player in sgs.qlist(players) do
			if player:hasLordSkill("thfeizhan") then
				for _, liege in sgs.qlist(self.room:getOtherPlayers(player)) do
					local isGood = self:isFriend(player)
					local goodKingdom = liege:getKingdom() == "hana"
					if isGood ~= goodKingdom then
						return liege
					end
				end
			end
		end
		for _, player in sgs.qlist(players) do
			if player:hasLordSkill("thchundu") then
				for _, liege in sgs.qlist(self.room:getOtherPlayers(player)) do
					local isGood = self:isFriend(player)
					local goodKingdom = liege:getKingdom() == "yuki"
					if isGood ~= goodKingdom then
						return liege
					end
				end
			end
		end
	end
	return nil
end

local function chooseKingdomForPlayer(self, to_modify)
	for _, lord in sgs.qlist(self.room:getOtherPlayers(to_modify)) do
		if self:isFriend(lord) then
			for skill, kingdom in pairs(sgs.thzhizun_kingdoms_map) do
				if lord:hasLordSkill(skill) and to_modify:getKingdom() ~= kingdom then
					return kingdom
				end
			end
		end
	end
	for _, lord in sgs.qlist(self.room:getOtherPlayers(to_modify)) do
		local enabled_kingdoms = { "kaze", "hana", "yuki", "tsuki" }
		table.removeOne(enabled_kingdoms, to_modify:getKingdom())
		for skill, kingdom in pairs(sgs.thzhizun_kingdoms_map) do
			if lord:hasLordSkill(skill)
					and (self:isFriend(to_modify, lord) or skill == "thfeizhan" or skill == "thchundu")
					and table.contains(enabled_kingdoms, kingdom) then
				table.removeOne(enabled_kingdoms, kingdom)
			end
		end
		if #enabled_kingdoms == 0 then
			self.room:writeToConsole("Error ThZhizun Kingdom Target!")
			return
		end
		for _, lord in ipairs(self.friends) do
			for skill, kingdom in pairs(sgs.thzhizun_kingdoms_map) do
				if lord:hasLordSkill(skill) and table.contains(enabled_kingdoms, kingdom) then
					return kingdom
				end
			end
		end
		return enabled_kingdoms[math.random(1, #enabled_kingdoms)]
	end
end

sgs.ai_skill_choice.thzhizun_kingdom = function(self, choices, data)
	local to_modify = data:toPlayer()
	return chooseKingdomForPlayer(self, to_modify)
end

sgs.ai_skill_choice.thzhizun_lordskills = function(self, choices)
	if choices:match("thfeizhan") and not self.room:getLieges("hana", self.player):isEmpty() then return "thfeizhan" end
	if choices:match("thchundu") and not self.room:getLieges("yuki", self.player):isEmpty() then return "thchundu" end
	return choices:split("+")[1]
end

sgs.ai_skill_playerchosen.thzhizun = function(self, players)
	if players and not players:isEmpty() then
		return findPlayerForModifyKingdom(self, players)
	end
end

--飞影：锁定技，当其他角色计算与你的距离时，始终+1。
--无

--离剑：限定技，准备阶段开始时，你可以展示一张手牌，所有其他角色须依次选择一项：交给你一张与此牌类型相同的牌；或受到你对其造成的1点伤害。
sgs.ai_skill_invoke.thlijian = function(self, data)
	local good, bad = 0, 0
	local lord = self.room:getLord()
	if self.role ~= "rebel" and lord and self:isWeak(lord) and self.player:objectName() ~= lord:objectName() then return false end
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isWeak(player) then
			if self:isFriend(player) then bad = bad + 1
			else good = good + 1
			end
		end
	end
	if good == 0 then return end

	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		local hp = math.max(player:getHp(), 1)
		if self:isFriend(player) then good = good + 1.0 / hp
		else bad = bad + 1.0 / hp
		end

		if self:isFriend(player) then good = good + math.max(getCardsNum("Peach", player, self.player), 1)
		else bad = bad + math.max(getCardsNum("Peach", player, self.player), 1)
		end
	end

	if good > bad then return true end
end

sgs.ai_skill_cardask["@thlijian-give"] = function(self, data, pattern, target)
	local pattern_map = {
		[".Basic"] = "basic",
		[".Equip"] = "equip",
		[".Trick"] = "trick"
	}
	
	local card_type = pattern_map[pattern]
	local cards = sgs.QList2Table(self.player:getCards("he"))
	if self:isFriend(target) then
		self:sortByUseValue(cards)
		for _, c in ipairs(cards) do
			if c:getType() == card_type then
				return "$" .. c:getEffectiveId()
			end
		end
	else
		self:sortByKeepValue(cards)
		for _, c in ipairs(cards) do
			if c:getType() == card_type and not self:isValuableCard(c, target) then
				return "$" .. c:getEffectiveId()
			end
		end
	end
end

--死枪：限定技，出牌阶段，你可以选择一名其他角色，令其不能使用或打出牌，直到回合结束。
local thsiqiang_skill = {}
thsiqiang_skill.name = "thsiqiang"
table.insert(sgs.ai_skills, thsiqiang_skill)
thsiqiang_skill.getTurnUseCard = function(self)
	if self.player:getMark("@siqiang") > 0 then
		self:sortEnemies(self.enemies)
		for _, enemy in ipairs(self.enemies) do
			if self:objectiveLevel(enemy) > 3 and not enemy:isKongcheng() and self:damageMinusHp(self, enemy, 0) >= 0 then
				return sgs.Card_Parse("@ThSiqiangCard=.")
			end
		end
	end
end

sgs.ai_skill_use_func.ThSiqiangCard = function(card, use, self)
	self:sortEnemies(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if self:objectiveLevel(enemy) > 3 and not enemy:isKongcheng() and self:damageMinusHp(self, enemy, 0) >= 0 then
			use.card = card
			if use.to then
				use.to:append(enemy)
			end
			return
		end
	end
end

sgs.ai_use_priority.ThSiqiangCard = 8
sgs.ai_card_intention.ThSiqiangCard = 100

--戒符：限定技，出牌阶段，你可弃置一名其他角色装备区的所有牌，然后令其全部的人物技能无效，直到回合结束。
local thjiefu_skill = {}
thjiefu_skill.name = "thjiefu"
table.insert(sgs.ai_skills, thjiefu_skill)
thjiefu_skill.getTurnUseCard = function(self)
	if self.player:getMark("@jiefu") > 0 then
		local target = self.player:getTag("ThSiqiangTarget"):toPlayer()
		if target and (target:getArmor() and not self:needToThrowArmor(target) or target:hasSkills(sgs.masochism_skill)) then
			return sgs.Card_Parse("@ThJiefuCard=.")
		end
		if not self.player:hasSkill("thsiqiang") or self.player:getMark("@siqiang") == 0 then
			self:sortEnemies(self.enemies)
			for _, enemy in ipairs(self.enemies) do
				if self:objectiveLevel(enemy) > 3 and (enemy:getArmor() and not self:needToThrowArmor(enemy)) then
					return sgs.Card_Parse("@ThJiefuCard=.")
				end
			end
		end
	end
end

sgs.ai_skill_use_func.ThJiefuCard = function(card, use, self)
	local target = self.player:getTag("ThSiqiangTarget"):toPlayer()
	if target and (target:getArmor() and not self:needToThrowArmor(target) or target:hasSkills(sgs.masochism_skill)) then
		use.card = card
		if use.to then
			use.to:append(target)
		end
		return
	end
	if not self.player:hasSkill("thsiqiang") or self.player:getMark("@siqiang") == 0 then
		self:sortEnemies(self.enemies)
		for _, enemy in ipairs(self.enemies) do
			if self:objectiveLevel(enemy) > 3 and (enemy:getArmor() and not self:needToThrowArmor(enemy)) then
				use.card = card
				if use.to then
					use.to:append(enemy)
				end
				return
			end
		end
	end
end

sgs.ai_use_priority.ThJiefuCard = 7.8
sgs.ai_card_intention.ThJiefuCard = 60

--幻乡：限定技，当你受到伤害结算开始时，你可以防止此伤害，且将你的体力回复至3点；然后你不能成为其他角色的牌的目标，且每当你受到伤害结算开始时，防止该伤害，直到你的下回合开始。
sgs.ai_skill_invoke.thhuanxiang = function(self, data)
	local damage = data:toDamage()
	if self:damageIsEffective_(damage, true) >= self.player:getHp() then
		return true
	end
end