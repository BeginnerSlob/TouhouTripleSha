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
			return a.value > b.value
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
