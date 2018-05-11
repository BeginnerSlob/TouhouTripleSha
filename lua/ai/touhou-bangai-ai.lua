--辨方：当你相邻的角色于其回合外失去手牌后，若你的手牌数不大于该角色，你可以摸一张牌。
sgs.ai_skill_invoke.thbianfang = true

--知时：当你于回合外得到牌/失去手牌后，你可以亮出牌堆顶的一张牌，然后你将之置于牌堆顶或置入弃牌堆。
sgs.ai_skill_invoke.thzhishi = true

sgs.ai_skill_choice.thzhishi = function(self, choices, data)
	local card = data:toCard()
	if (self:isValuableCard(card)) then
		local next_player
		for _, p in sgs.qlist(global_room:getOtherPlayers(self.player)) do
			if p:faceUp() then next_player = p break end
		end
		next_player = next_player or self.player:faceUp() and self.player or self.player:getNextAlive(1, false)
		return self.isFriend(next_player) and "draw" or "discard"
	end
	return math.random(1, 3) == 3 and "discard" or "draw"
end

--授卷：出牌阶段开始时，你可以摸两张牌，若如此做，此阶段内每当一名其他角色失去牌时，你须交给其一张牌。
sgs.ai_skill_invoke.thshoujuan = function(self, data)
	local use = { isDummy = true }
	self:activate(use)
	if not use.card then
		return true
	end
end

sgs.ai_skill_cardask["@thshoujuan"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByUseValue(cards, false)
	if self:isFriend(target) then
		local card, _ = self:getCardNeedPlayer(cards, { target }, false)
		if card then
			return "$" .. card:getEffectiveId()
		end
	else
		for _, c in ipairs(cards) do
			if isCard("Peach", c, target) then
				continue
			else
				return "$" .. c:getEffectiveId()
			end
		end
	end
	return "$" .. c:getEffectiveId()
end

--谧契：结束阶段开始时，若你的手牌数为全场最少的（或之一），你可以弃置场上的至多等同于你手牌数的牌。
sgs.ai_skill_use["@@thmiqi"] = function(self, prompt, method)
	local n = self.player:getHandcardNum()
	local targets = self:findPlayerToDiscard("ej", true, true, nil, true)
	local tos = {}
	for _, t in ipairs(targets) do
		table.insert(tos, t:objectName())
		if #tos == n then
			break
		end
	end
	if #tos > 0 then
		return "@ThMiqiCard=.->" .. table.concat(tos, "+")
	end
	return "."
end

sgs.ai_choicemade_filter.cardChosen.thmiqi = sgs.ai_choicemade_filter.cardChosen.snatch

--疾步：锁定技，当你计算与其他角色的距离时，始终-1。
--无

--织月：当你使用【杀】时，可以进行一次判定，若为黑色，额外指定一个目标；若为红色，此【杀】指定目标后，弃置目标角色一张牌。
sgs.ai_skill_invoke.thzhiyue = function(self, data)
	local use = data:toCardUse()
	for _, p in sgs.qlist(use.to) do
		if self:isFriend(p) and self.player:canDiscard(p, "he") and not self:needToThrowArmor(p) then
			return false
		end
	end
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isEnemy(p) and self.player:canSlash(p, use.card) and not use.to:contains(p) then
			return true
		end
	end
	return false
end

sgs.ai_skill_playerchosen.thzhiyue = function(self, targets)
	local slash = self.player:getTag("ThZhiyueSlash"):toCard()
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defenseSlash")
	for _, target in ipairs(targets) do
		if self:isEnemy(target) and not self:slashProhibit(slash, target) and sgs.isGoodTarget(target, targets, self) and self:slashIsEffective(slash, target) then
			return target
		end
	end
	for _, target in ipairs(targets) do
		if self:isEnemy(target) then
			return target
		end
	end
	for _, target in ipairs(targets) do
		if self:isFriend(target) and not self:slashIsEffective(slash, target) then
			return target
		end
	end
	return targets[#targets]
end

sgs.ai_playerchosen_intention.thzhiyue = function(self, from, to)
	local slash = from:getTag("ThZhiyueSlash"):toCard()
	sgs.ai_card_intention.Slash(self, slash, from, { to })
end

--忠节：每当你受到一次伤害后，你可以展示一名角色的所有手牌，每少一种类型的牌，其摸一张牌。
sgs.ai_skill_playerchosen.thzhongjie = function(self, targets)
	local getKnownTypeNum = function(target)
		local types = {}
		local unknown = 0
		local flag = ("%s_%s_%s"):format("visible", self.player:objectName(), target:objectName())
		for _, c in sgs.qlist(target:getHandcards()) do
			if self.player:objectName() == target:objectName() or c:hasFlag("visible") or c:hasFlag(flag) then
				if not table.contains(types, c:getType()) then
					table.insert(types, c:getType())
				end
				continue
			else
				unknown = unknown + 1
			end
		end
		return #types, unknown
	end
	local thzhongjie_comp = function(a, b)
		local a1, a2 = getKnownTypeNum(a)
		local b1, b2 = getKnownTypeNum(b)
		if a1 ~= b1 then
			return a2 < b2
		else
			return a1 < b1
		end
	end
	local friends = {}
	for _, p in sgs.qlist(targets) do
		if self:isFriend(p) then
			local n = getKnownTypeNum(p)
			if n ~= 3 then
				table.insert(friends, p)
			end
		end
	end
	if #friends == 0 then return nil end
	table.sort(friends, thzhongjie_comp)
	return friends[1]
end

sgs.ai_playerchosen_intention.thzhongjie = -20

--虚魅：出牌阶段限一次，你可以弃置一张基本牌并亮出牌堆顶的三张牌，然后令一名角色获得其中一种类别的牌，将其余的牌置入弃牌堆。若如此做，该角色不能使用或打出该类别的牌，直到回合结束。
local thxumei_skill = {}
thxumei_skill.name = "thxumei"
table.insert(sgs.ai_skills, thxumei_skill)
thxumei_skill.getTurnUseCard = function(self)
	if self.player:canDiscard(self.player, "h") and not self.player:hasUsed("ThXumeiCard") then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByUseValue(cards, true)
		for _, c in ipairs(cards) do
			if c:isKindOf("BasicCard") and not self:isValuableCard(c) then
				return sgs.Card_Parse("@ThXumeiCard=" .. c:getEffectiveId())
			end
		end
	end
end

sgs.ai_skill_use_func.ThXumeiCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_choice.thxumei = function(self, choices, data)
	sgs.thxumei_target = nil
	local ids = data:toIntList()
	local choice_map = {}
	for _, id in sgs.qlist(ids) do
		local c = sgs.Sanguosha:getCard(id)
		local str = c:getType()
		if str == "skill" then continue end
		if not choice_map[str] then choice_map[str] = {} end
		table.insert(choice_map[str], c)
	end
	for _, id in sgs.qlist(ids) do
		local c = sgs.Sanguosha:getCard(id)
		self:sort(self.friends_noself, "defense")
		for _, p in ipairs(self.friends_noself) do
			if isCard("Peach", c, p) then
				sgs.thxumei_target = p
				return c:getType()
			end
		end
	end
	if #choice_map < 3 then
		for str, t in pairs(choice_map) do
			if #t > 1 then
				return str
			end
		end
	end
	local target, cardId = sgs.ai_skill_askforyiji.ikyumeng(self, sgs.QList2Table(ids))
	if target and cardId then
		sgs.thxumei_target = target
		return sgs.Sanguosha:getCard(cardId):getType()
	else
		sgs.thxumei_target = self.player
		choices = choices:split("+")
		return choices[math.random(1, #choices)]
	end
end

sgs.ai_skill_playerchosen.thxumei = function(self, targets)
	if sgs.thxumei_target then
		return sgs.thxumei_target
	end
	local ids = self.player:getTag("ThXumeiDummy"):toIntList()
	local target, _ = sgs.ai_skill_askforyiji.ikyumeng(self, sgs.QList2Table(ids))
	return target or self.player
end

sgs.ai_use_priority.ThXumeiCard = -2

--梦违：当一名角色的手牌被展示后，你可以摸一张牌。
sgs.ai_skill_invoke.thmengwei = true

--隙境：当一名角色的判定牌生效前，你可以令该角色展示手牌，然后你选择打出其中的一张牌代替之。
sgs.ai_skill_invoke.thxijing = function(self, data)
	local judge = data:toJudge()
	if self:needRetrial(judge) and judge.who:getHandcardNum() > 1 then
		return true
	end
	return false
end

sgs.ai_skill_askforag.thxijing = function(self, card_ids)
	local cards = {}
	for _, id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(id))
	end
	local judge = self.player:getTag("ThXijingJudge"):toJudge()
	local target = judge.who

	local cmp = function(a, b)
		local a_keep_value, b_keep_value = sgs.ai_keep_value[a:getClassName()] or 0, sgs.ai_keep_value[b:getClassName()] or 0
		a_keep_value = a_keep_value + a:getNumber() / 100
		b_keep_value = b_keep_value + b:getNumber() / 100
		return a_keep_value < b_keep_value
	end

	local card_id = self:getRetrialCardId(cards, judge, false)
	if card_id ~= -1 then return card_id end
	if target and not self:isEnemy(target) then
		local valueless = {}
		for _, card in ipairs(cards) do
			if not self:isValuableCard(card, target) then table.insert(valueless, card) end
		end
		if #valueless == 0 then valueless = cards end
		table.sort(valueless, cmp)
		return valueless[1]:getEffectiveId()
	else
		for _, card in ipairs(cards) do
			if judge:isBad(card) then return card:getEffectiveId() end
		end
		local valuable = {}
		for _, card in ipairs(cards) do
			if self:isValuableCard(card, target) then table.insert(valuable, card) end
		end
		if #valuable == 0 then valuable = cards end
		table.sort(valuable, cmp)
		return valuable[#valuable]:getEffectiveId()
	end
	return -1
end

--死镰：锁定技，专属技，你的武器牌均视为【杀】；你获得即将进入你装备区的武器牌；若你的装备区没有武器牌，你视为装备着【离魂之镰】。
--无

--灵战：每当你使用【杀】对目标角色造成一次伤害后，你可以进行一次判定，将非红桃的判定牌置于你的人物牌上称为“幻”，你可以将一张“幻”当【杀】使用或者打出。
sgs.ai_skill_invoke.thlingzhan = true

local thlingzhan_skill = {}
thlingzhan_skill.name = "thlingzhan"
table.insert(sgs.ai_skills, thlingzhan_skill)
thlingzhan_skill.getTurnUseCard = function(self)
	if self.player:getPile("nightmare"):isEmpty() then
		return
	end
	for i = 0, self.player:getPile("nightmare"):length() - 1 do
		local slash = sgs.Sanguosha:getCard(self.player:getPile("nightmare"):at(i))
		local slash_str = ("slash:thlingzhan[%s:%s]=%d"):format(slash:getSuitString(), slash:getNumberString(), self.player:getPile("nightmare"):at(i))
		local lingzhanslash = sgs.Card_Parse(slash_str)
		if self:slashIsAvailable(self.player, lingzhanslash) then
			return lingzhanslash
		end
	end
end

sgs.ai_view_as.thlingzhan = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place == sgs.Player_PlaceSpecial and player:getPileName(card_id) == "nightmare" then
		return ("slash:thlingzhan[%s:%s]=%d"):format(suit, number, card_id)
	end
end

--衍梦：锁定技，其他角色不能令你的人物技能无效或失去。
--无

--琴韶：弃牌阶段开始时，若你的手牌数：大于体力值，你可以选择一名其他角色，令其摸X张牌；小于体力值，你可以摸X-1张牌（X为你手牌数与体力值的差）。
sgs.ai_skill_playerchosen.thqinshao = function(self, targets)
	local n = self.player:getHandcardNum() - self.player:getHp() - 1
	return self:findPlayerToDraw(false, n)
end

sgs.ai_skill_invoke.thqinshao = true

sgs.ai_skill_playerchosen.thqinshao = -40

--星屑：出牌阶段限一次，你可以弃置一张手牌，然后将一名角色装备区内的全部牌置于你的人物牌上，此回合结束时，令其依次获得并使用这些牌。
local thxingxie_skill = {}
thxingxie_skill.name = "thxingxie"
table.insert(sgs.ai_skills, thxingxie_skill)
thxingxie_skill.getTurnUseCard = function(self)
	if self.player:canDiscard(self.player, "h") and not self.player:hasUsed("ThXingxieCard") then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByUseValue(cards, true)
		for _, c in ipairs(cards) do
			if not self:isValuableCard(c) then
				return sgs.Card_Parse("@ThXingxieCard=" .. c:getEffectiveId())
			end
		end
	end
end

sgs.ai_skill_use_func.ThXingxieCard = function(card, use, self)
	local slash = self:getCard("Slash")
	if slash and self:slashIsAvailable(self.player, slash) then
		local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
		self:useCardSlash(slash, dummy_use)
		if dummy_use.card then
			for _, p in sgs.qlist(dummy_use.to) do
				if self:isEnemy(p) and p:getArmor() and not self:needToThrowArmor(p) then
					use.card = card
					if use.to then
						use.to:append(p)
					end
					return
				end
			end
		end
	end
	for _, p in ipairs(self.friends) do
		if p:hasArmorEffect("silver_lion") and p:getArmor() and p:isWounded() then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
	for _, p in ipairs(self.enemies) do
		if p:hasTreasure("wooden_ox") and p:getPile("wooden_ox"):length() > 0 then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
end

sgs.ai_use_priority.ThXingxieCard = sgs.ai_use_priority.Slash + 0.1

sgs.ai_card_intention.ThXingxieCard = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		if to:hasArmorEffect("silver_lion") and to:getArmor() and to:isWounded() then
		else
			sgs.updateIntention(from, to, 30)
		end
	end
end

--羽帛：出牌阶段限一次，你可以弃置一张黑色手牌并将一至两名角色的人物牌横置。
local thyubo_skill = {}
thyubo_skill.name = "thyubo"
table.insert(sgs.ai_skills, thyubo_skill)
thyubo_skill.getTurnUseCard = function(self)
	if self.player:canDiscard(self.player, "h") and not self.player:hasUsed("ThYuboCard") then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByUseValue(cards, true)
		for _, c in ipairs(cards) do
			if c:isBlack() and not self:isValuableCard(c) then
				return sgs.Card_Parse("@ThYuboCard=" .. c:getEffectiveId())
			end
		end
	end
end

sgs.ai_skill_use_func.ThYuboCard = function(card, use, self)
	local victims = self.room:getAlivePlayers()
	local target1 = self:findPlayerToChain(victims, true)
	if target1 then
		use.card = card
		if use.to then
			use.to:append(target1)
			victims:removeOne(target1)
			local target2 = self:findPlayerToChain(victims, true)
			if target2 then
				use.to:append(target2)
			end
		end
	end
end

sgs.ai_card_intention.ThYuboCard = 60
sgs.ai_use_priority.ThYuboCard = 5

function SmartAI:isGoodThQiongfaTarget(target)
	local current = self.room:getCurrent()
	if not current then return false end
	if current:getPhase() >= sgs.Player_Finish then
		current = self.room:findPlayer(current:getNextAlive(1, false):objectName())
	end
	local lingxians = self.room:findPlayersBySkillName("thqiongfa")
	local has_lingxian = false
	for _, s in sgs.qlist(lingxians) do
		if self:isFriend(s, current) then
			has_lingxian = true
			break
		end
	end
	if not has_lingxian then return false end
	if target:objectName() == current:objectName() then return true end
	if self:getEnemyNumBySeat(current, target, nil, true) == 0 then
		return true
	end
	return false
end

--穹法：人物牌横置的角色的结束阶段开始时，你可以令其选择一项：弃置由你指定的另一名角色的一张牌或令你摸一张牌。然后其重置其人物牌。
sgs.ai_skill_invoke.thqiongfa = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) and not self:isGoodChainPartner(target) then
		return true
	elseif not self:isFriend(target) and self:isGoodChainPartner(target) then
		return true
	end
	return false
end

sgs.ai_skill_playerchosen.thqiongfa = function(self, targets)
	local current = nil
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:getPhase() == sgs.Player_Finish then
			current = p
			break
		end
	end
	if not current then
		self.room:writeToConsole("No Qiongfa Source!")
		current = self.player
	end
	if self.player:objectName() == current:objectName() then
		local victim = self:findPlayerToDiscard("he", true, true, self.room:getOtherPlayers(current), false)
		if victim then
			local card_id = self:askForCardChosen(victim, "he", "thqiongfa", sgs.Card_MethodDiscard)
			if victim:hasEquip(sgs.Sanguosha:getCard(card_id)) or victim:getCardCount() < 3 then
				return victim
			end
		end
		return nil
	end
	if self:isFriend(current) then
		local vcitim = self:findPlayerToDiscard("he", true, true, self.room:getOtherPlayers(current), false)
		if victim then
			return victim
		else
			return targets:at(math.random(0, targets:length() - 1))
		end
	else
		self:sort(self.enemies, "handcard")
		for _, p in ipairs(self.enemies) do
			if not self:needToThrowArmor(p) and not (self:needKongcheng(p) and p:getHandcardNum() == 1) then
				return p
			end
		end
		return targets:at(math.random(0, targets:length() - 1))
	end
end

sgs.ai_skill_choice.thqiongfa = function(self, choices, data)
	local source = self.player:getTag("ThQiongfaSource"):toPlayer()
	local target = data:toPlayer()
	if self:isFriend(source) and not self:isEnemy(target) then
		return "cancel"
	end
	if self:isEnemy(source) and self:isFriend(target) then
		if self:getOverflow(target) > 1 then
			return "discard"
		end
		return "cancel"
	end
	if self:isFriend(source) and self:isEnemy(target) then
		local card_id = self:askForCardChosen(target, "he", "thqiongfa", sgs.Card_MethodDiscard)
		if target:hasEquip(sgs.Sanguosha:getCard(card_id)) or target:getCardCount() < 3 then
			return "discard"
		end
		return "cancel"
	end
	return not self:isEnemy(source) and "cancel" or "discard"
end

--威德：摸牌阶段摸牌时，你可以放弃摸牌，改为观看牌堆顶的两张牌并交给一名角色，然后若该角色不为你且你已受伤，你可以获得一名其他角色的一张手牌。
sgs.ai_skill_invoke.thweide = true

sgs.ai_skill_playerchosen.thweide = function(self, targets)
	local ids = self.player:getTag("ThWeide"):toIntList()
	if not ids or ids:isEmpty() then
		return self:findPlayerToDiscard("h", false, false) or targets:at(math.random(0, targets:length() - 1))
	else
		local target, _ = sgs.ai_skill_askforyiji.ikyumeng(self, sgs.QList2Table(ids))
		return target or self.player
	end
end

sgs.ai_playerchosen_intention.thweide = function(self, from, to)
	local ids = from:getTag("ThWeide"):toIntList()
	if ids and not ids:isEmpty() then
		sgs.updateIntention(from, to, -30)
	end
end

--诡卷：出牌阶段，你可以摸一张牌，然后展示之并选择一项：使用此牌，或失去1点体力。若你以此法使用了一张【杀】或装备牌，你不可以发动“诡卷”，直到回合结束。
local thguijuan_skill = {}
thguijuan_skill.name = "thguijuan"
table.insert(sgs.ai_skills, thguijuan_skill)
thguijuan_skill.getTurnUseCard = function(self)
	if not self.player:hasFlag("ForbidThGuijuan") then
		if self:getCardsNum("Peach") >= 1 or (self.player:getHp() <= 1 and self:getCardsNum("Analeptic") >= 1) or self.player:getHp() > 2 then
			return sgs.Card_Parse("@ThGuijuanCard=.")
		end
	end
end

sgs.ai_skill_use_func.ThGuijuanCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.ThGuijuanCard = 8

--诈诱：当其他角色使用的【杀】被你的【闪】抵消后，你可以摸一张牌，然后令该角色选择一项：对你使用一张无视距离的【杀】，或受到你对其造成的1点伤害。
sgs.ai_skill_invoke.thzhayou = function(self, data)
	local effect = data:toSlashEffect()
	return self:isEnemy(effect.from) or not self:damageIsEffective(effect.from, nil, self.player)
end

sgs.ai_skill_cardask["@thzhayou"] = function(self, data, pattern, target, target2)
	if self:isFriend(target) and not self:damageIsEffective(self.player, nil, target) then
		return "."
	end
	for _, slash in ipairs(self:getCards("Slash")) do
		if self:isFriend(target2) and self:slashIsEffective(slash, target2) then
			if self:findLeijiTarget(target2, 50, self.player) then return slash:toString() end
			if self:getDamagedEffects(target2, self.player, true) then return slash:toString() end
		end

		local nature = sgs.DamageStruct_Normal
		if slash:isKindOf("FireSlash") then nature = sgs.DamageStruct_Fire
		elseif slash:isKindOf("ThunderSlash") then nature = sgs.DamageStruct_Thunder end
		if self:isEnemy(target2) and self:slashIsEffective(slash, target2) and self:canAttack(target2, self.player, nature)
			and not self:getDamagedEffects(target2, self.player, true) and not self:findLeijiTarget(target2, 50, self.player) then
			return slash:toString()
		end
	end
	return "."
end

--辉轮：锁定技，你的黑色【杀】均视为【桃】；你的红色【桃】均视为【杀】。
--smart-ai.lua isCard

--妄道：出牌阶段，你可以选择一名其他角色并展示一张【桃】，该角色须选择一项：1.对你使用一张与此【桃】花色相同的无视距离的【杀】；2.令你弃置此【桃】，然后令你选择弃置其两张牌或令其失去1点体力。
local thwangdao_skill = {}
thwangdao_skill.name = "thwangdao"
table.insert(sgs.ai_skills, thwangdao_skill)
thwangdao_skill.getTurnUseCard = function(self)
	if #self.enemies > 0 and #self:getCards("Peach") > 0 and (#self:getCards("Peach") > 1 or self:getOverflow() > 0) then
		return sgs.Card_Parse("@ThWangdaoCard=.")
	end
end

sgs.ai_skill_use_func.ThWangdaoCard = function(card, use, self)
	self:sort(self.enemies, "defense")
	local peaches = self:getCards("Peach")
	self:sortByUseValue(peaches, true)
	for _, p in ipairs(self.enemies) do
		for _, peach in ipairs(peaches) do
			if isCard("Peach", peach, p) then continue end
			local suit = peach:getSuit()
			local flag = ("%s_%s_%s"):format("visible", self.player:objectName(), p:objectName())
			local cards = p:getCards("he")
			for _, id in sgs.qlist(getWoodenOxPile(p)) do
				cards:prepend(sgs.Sanguosha:getCard(id))
			end
			local has_slash = false
			for _, c in sgs.qlist(cards) do
				if c:hasFlag("visible") or c:hasFlag(flag) then
					if c:getSuit() == suit and isCard("Slash", c, p) and p:canSlash(self.player, c, false) then
						has_slash = true
						break
					end
				end
			end
			if has_slash then
				continue
			end
			use.card = sgs.Card_Parse("@ThWangdaoCard=" .. peach:getEffectiveId())
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
end

sgs.ai_skill_choice.thwangdao = function(self, choices, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		if self:isWeak(target) then return "discard" end
		if target:getLostHp() < 1 then return "lose" end
		return "discard"
	else
		if self:isWeak(target) then return "lose" end
		if target:hasSkill("lirang") and #self:getFriendsNoSelf(target) > 0 then return "lose" end
		if target:getArmor() and self:evaluateArmor(target:getArmor(), target) > 3 and not (target:hasArmorEffect("silver_lion") and target:isWounded()) then
			return "discard"
		end
		if target:hasSkills("ikyindie+ikguiyue") and target:getPhase() == sgs.Player_NotActive then return "lose" end
		if target:hasSkills(sgs.need_kongcheng) then return "lose" end
		if target:getCards("he"):length() < 4 and target:getCards("he"):length() > 1 then return "discard" end
		return "lose"
	end
end

sgs.ai_card_intention.ThWangdaoCard = 50

--四象：出牌阶段，你可以选择一项：1. 弃置两张黑桃牌并令一名角色回复1点体力。2. 弃置两张方块牌并弃置至多两名其他角色区域的各一张牌；3. 弃置两张梅花牌并令一名其他角色摸两张牌；4. 弃置两张红桃牌并令一名其他角色摸一张牌，然后弃置一张手牌，最后该角色将其人物牌翻面。
local thkongxiang_skill = {}
thkongxiang_skill.name = "thkongxiang"
table.insert(sgs.ai_skills, thkongxiang_skill)
thkongxiang_skill.getTurnUseCard = function(self)
	if self.player:canDiscard(self.player, "he") and self.player:getCardCount() > 1 then
		return sgs.Card_Parse("@ThKongxiangCard=.")
	end
end

sgs.ai_skill_use_func.ThKongxiangCard = function(card, use, self)
	local function getSiXiangTarget(str)
		local to = sgs.SPlayerList()
		if str == "spade" then
			local target = self:findPlayerToRecover()
			if target then
				to:append(target)
				return to
			end
			return nil
		elseif str == "heart" then
			self:sort(self.friends_noself, "defense")
			for _, friend in ipairs(self.friends_noself) do
				if not self:toTurnOver(friend, 1) then
					to:append(friend)
					return to
				end
			end
			self:sort(self.enemies, "hp")
			for _, enemy in ipairs(self.enemies) do
				if self:toTurnOver(enemy, 1) then
					to:append(enemy)
					return to
				end
			end
			return nil
		elseif str == "club" then
			local target = self:findPlayerToDraw(false, 2)
			if target then
				to:append(target)
				return to
			end
			return nil
		elseif str == "diamond" then
			local targets = self:findPlayerToDiscard("hej", false, true, nil, true)
			for _, target in ipairs(targets) do
				if not to:contains(target) then
					to:append(target)
					if to:length() == 2 then
						return to
					end
				end
			end
			if not to:isEmpty() then
				if to:length() == 1 then
					if self:isFriend(to:first()) or self:getOverflow() > 0 then
						return to
					else
						return nil
					end
				else
					return to
				end
			else
				return nil
			end
		end
		return nil
	end

	local can_use = {}
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByUseValue(cards, true)
	for _, c in ipairs(cards) do
		if can_use[c:getSuitString()] then continue end
		if self:isValuableCard(c) then continue end
		for _, c2 in ipairs(cards) do
			if c:getEffectiveId() == c2:getEffectiveId() then continue end
			if self:isValuableCard(c2) then continue end
			if c2:getSuit() == c:getSuit() then
				can_use[c:getSuitString()] = c:getEffectiveId() .. "+" .. c2:getEffectiveId()
			end
		end
	end
	for str, ids in pairs(can_use) do
		local to = getSiXiangTarget(str)
		if to then
			use.card = sgs.Card_Parse("@ThKongxiangCard=" .. ids)
			if use.to then
				use.to = to
			end
			return
		end
	end
end

sgs.ai_card_intention.ThKongxiangCard = function(self, card, from, tos)
	local str = sgs.Sanguosha:getCard(card:getSubcards():first()):getSuitString()
	for _, to in ipairs(tos) do
		if str == "spade" then
			sgs.updateIntention(from, to, -80)
		elseif str == "heart" then
			if self:toTurnOver(to, 1) then
				sgs.updateIntention(from, to, 80)
			end
		elseif str == "club" then
			sgs.updateIntention(from, to, -80)
		end
	end
end

sgs.ai_use_priority.ThKongxiangCard = 5
sgs.ai_choicemade_filter.cardChosen.thkongxiang = sgs.ai_choicemade_filter.cardChosen.snatch
