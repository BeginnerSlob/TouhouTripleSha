--花祭：在你的回合，当其他角色使用一张基本牌或非延时类锦囊牌时，你可弃置一张黑色牌，则该角色需弃置一张与其之前使用的牌名称相同的牌，否则该牌无效。
sgs.ai_skill_cardask["@thhuajiuse"] = function(self, data, pattern, target)
	if not self:isEnemy(target) then
		return "."
	end
	local keepSlash = self:slashIsAvailable() and self:getCardsNum("Slash") <= 1 
	local blacks = {}
	local cards = sgs.QList2Table(self.player:getCards("he"))
	--目的是把红杀排在前面
	self:sortByKeepValue(cards, true)
	for _, c in pairs(cards) do
		if not c:isBlack() then 
			if c:isKindOf("Slash") then
				keepSlash = false
			end
			continue 
		end
		if c:isKindOf("Slash") and keepSlash then
			keepSlash = false
			continue 
		end
		if c:isKindOf("AOE") then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useTrickCard(c, dummy_use)
			if dummy_use.card then
				continue
			end
		end
		table.insert(blacks, c)
	end
	if #blacks == 0 then return "." end
	self:sortByKeepValue(blacks)
	return "$" .. blacks[1]:getId()
end

sgs.ai_choicemade_filter.cardResponded["@thhuajiuse"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		local target = self.room:findPlayer(promptlist[#promptlist - 1])
		if target then
			sgs.updateIntention(player, target, 80)
		end
	end
end

--借物：出牌阶段限一次，你可获得一名你攻击范围内的角色的一张牌，视为该角色对你使用一张无视距离的【杀】，且此【杀】无视你的防具。
local thjiewu_skill = {}
thjiewu_skill.name = "thjiewu"
table.insert(sgs.ai_skills, thjiewu_skill)
thjiewu_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThJiewuCard") then
		return sgs.Card_Parse("@ThJiewuCard=.")
	end
end

sgs.ai_skill_use_func.ThJiewuCard = function(card, use, self)
	sgs.ai_use_priority.ThJiewuCard = 4
	local targets = {}
	for _, enemy in ipairs(self.enemies) do
		if self.player:inMyAttackRange(enemy)
				and (self.player:getHp() > 2
					or self:getCardsNum("Jink") > 0
					or self:findLeijiTarget(self.player, 50, enemy)
					or not enemy:canSlash(self.player, false))
				and not enemy:isNude() then
			table.insert(targets, enemy)
		end
	end

	if #targets == 0 then return end

	sgs.ai_use_priority.ThJiewuCard = 8
	use.card = card
	if use.to then
		self:sort(targets, "defenseSlash")
		use.to:append(targets[1])
	end
end

sgs.ai_card_intention.ThJiewuCard = 80
sgs.ai_use_priority.ThJiewuCard = 4

--根性：觉醒技，准备阶段开始时，若你的体力值为1，你须回复1点体力或摸两张牌，然后减少1点体力上限并获得技能“魔炮”。
sgs.ai_need_damaged.thgenxing = function(self, attacker, player)
	if player:hasSkill("thgenxing") and player:getMark("@genxing") == 0 and not player:hasSkill("chanyuan")
		and self:getEnemyNumBySeat(self.room:getCurrent(), player, player, true) < player:getHp()
		and (player:getHp() > 2 or (player:getHp() == 2 and player:faceUp())) then
		return true
	end
	return false
end

--魔炮：每当你使用或打出一张【闪】时，你可以令一名其他角色摸一张牌，然后你对其造成1点火焰伤害。
sgs.ai_skill_playerchosen.thmopao = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	self:sort(targetlist, "hp")
	local victims = {}
	for _, target in ipairs(targetlist) do
		if self:isFriend(target) and target:isChained() and self:isGoodChainTarget(target, self.player, sgs.DamageStruct_Fire, 1) then
			return target
		end
		if self:isEnemy(target) and self:damageIsEffective(target, sgs.DamageStruct_Fire, self.player) then
			table.insert(victims, target)
		end
	end
	if #victims == 0 then
		for _, target in ipairs(targetlist) do
			if self:isFriend(target) and not self:damageIsEffective(target, sgs.DamageStruct_Fire, self.player) then
				return target
			end
		end
		return nil
	end
	for _, p in ipairs(victims) do
		if p:isKongcheng() and self:needKongcheng(p, true) then
			return p
		end
		if self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Fire, 1) then
			return p
		end
	end
	return victims[1]
end

function sgs.ai_cardneed.thmopao(to, card, self)
	return (isCard("Jink", card, to) and getKnownCard(to, self, "Jink", true) == 0)
			or (card:isKindOf("EightDiagram") and not (self:hasEightDiagramEffect(to) or getKnownCard(to, self, "EightDiagram", false) > 0))
end

--余无：锁定技。当你使用【杀】对体力值不大于1的角色造成伤害时，其立即死亡。
function sgs.ai_cardneed.thyuwu(to, card, self)
	for _, enemy in ipairs(self.enemies) do
		if enemy:getHp() <= 1 then
			if to:canSlash(enemy) then
				return isCard("Slash", card, to) and getKnownCard(to, self, "Slash", true) == 0
			else
				return getKnownCard(to, self, "Slash", true) > 0 and isCard("Weapon", card, to)
			end
		end
	end
	return false
end

--smart-ai.lua SmartAI:hasHeavySlashDamage

--归航：当一名角色进入濒死状态时，你可以减1点体力上限，然后你令其回复1点体力。
sgs.ai_skill_invoke.thguihang = function(self, data)
	local dying = data:toDying()
	local isFriend = false

	isFriend = not self:isEnemy(dying.who)
	if not sgs.GetConfig("EnableHegemony", false) and self.role == "renegade"
		and not (dying.who:isLord() or dying.who:objectName() == self.player:objectName())
		and not (self.room:getMode() == "couple" and dying.who:getGeneralName() == "sunjian")
		and (sgs.current_mode_players["loyalist"] + 1 == sgs.current_mode_players["rebel"]
				or sgs.current_mode_players["loyalist"] == sgs.current_mode_players["rebel"]
				or self.room:getCurrent():objectName() == self.player:objectName()) then
		isFriend = false
	end

	if not isFriend then
		return false
	end

	if self:isFriend(dying.who) and self:getAllPeachNum(dying.who) < 1 - dying.who:getHp() then
		return true
	end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.thguihang = function(self, player, promptlist)
	local dying = self.room:getCurrentDyingPlayer()
	if promptlist[#promptlist] == "yes" then
		if dying and dying:objectName() ~= self.player:objectName() then
			sgs.updateIntention(player, dying, -120)
		end
	end
end

--彼岸：当你于濒死状态回复体力后，若你的体力值不小于1，你可以视为对一至X名角色使用一张无距离限制的【杀】（X为你的体力上限）。
sgs.ai_skill_use["@@thbian"] = function(self, prompt, method)
	local n = self.player:getMaxHp()
	local targets = {}
	local victims = self.room:getOtherPlayers(self.player)
	for i = 1,n,1 do
		local p = sgs.ai_skill_playerchosen.slash_extra_targets(self, victims)
		if p ~= nil then
			table.insert(targets, p:objectName())
		else
			break
		end
	end
	if #targets == 0 then
		return "."
	end
	return "slash:thbian[no_suit:0]=.->" + table.concat(targets, "+")
end

--血兰：你可以弃置一张红色牌并抵消一张【桃】对一名角色的效果，然后若该角色的体力上限不大于其游戏开始时的体力上限，则该角色须增加1点体力上限。
sgs.ai_skill_cardask["@thxuelan"] = function(self, data, pattern, target)
	if not self:isEnemy(target) then return "." end
	local reds = {}
	for _, c in sgs.qlist(self.player:getCards("he")) do
		if c:isRed() then
			table.insert(reds,c)
		end
	end
	if #reds == 0 then return "." end
	self:sortByKeepValue(reds)
	return "$" .. reds[1]:getId()
end

sgs.ai_cardneed.thxuelan = function(to, card, self)
	return card:isRed()
end

sgs.thxuelan_suit_value = {
	heart = 4.8,
	diamond = 4.6
}

sgs.ai_choicemade_filter.cardResponded["@thxuelan"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		local target = self.room:findPlayer(promptlist[#promptlist - 1])
		if target then
			sgs.updateIntention(player, target, 75)
		end
	end
end

--心妄：你的回合外，每当你使用、打出一张红桃牌时，或因弃置而失去一张红桃牌后，你可以摸一张牌或令一名其他角色回复1点体力。
sgs.ai_skill_playerchosen.thxinwang = function(self, targets)
	return self:findPlayerToRecover(1, targets)
end

sgs.ai_playerchosen_intention.thxinwang = -100

--绝毒：锁定技，杀死你的角色获得技能“崩坏”。
function sgs.ai_slash_prohibit.thjuedu(self, from, to)
	if not to:hasSkill("thjuedu") then return false end
	if from:hasSkill("ikxuwu") or from:getMark("thshenyou") > 0 or (from:hasSkill("ikwanhun") and from:distanceTo(to) == 1) then return false end
	if from:hasFlag("IkJieyouUsed") then return false end
	if from:hasSkill("ikbenghuai") then return false end
	if self:isFriend(to, from) and self:isWeak(to) then return true end
	return self:isWeak(to) and from:getHp() > 2
end

--霆舞：出牌阶段限两次，每当你对一名人物牌竖置的其他角色造成雷电伤害，在伤害结算后，你可以进行一次判定，若结果不为红桃，你选择对该角色的上家或下家造成1点雷电伤害。
sgs.thtingwu_target = nil

sgs.ai_skill_invoke.thtingwu = function(self, data)
	sgs.thtingwu_target = nil
	local who = data:toDamage().to
	local up = self.room:findPlayer(who:getLastAlive():objectName())
	local down = self.room:findPlayer(who:getNextAlive():objectName())
	local targets = { up, down }
	self:sort(targets, "defense")
	for _, p in ipairs(targets) do
		if p:isChained() and self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Thunder, 1) then
			sgs.thtingwu_target = p
			return true
		end
	end
	for _, p in ipairs(targets) do
		if self:isEnemy(p) and self:damageIsEffective(p, sgs.DamageStruct_Thunder, self.player) then
			sgs.thtingwu_target = p
			return true
		end
	end
	for _, p in ipairs(targets) do
		if not self:damageIsEffective(p, sgs.DamageStruct_Thunder, self.player) then
			sgs.thtingwu_target = p
			return true
		end
	end
	return false
end

sgs.ai_skill_playerchosen.thtingwu = function(self, targets)
	if sgs.thtingwu_target and targets:contains(sgs.thtingwu_target) then return sgs.thtingwu_target end
	return targets:at(math.random(0, 1))
end

sgs.ai_playerchosen_intention.thtingwu = function(self, from, to)
	if self:damageIsEffective(to, sgs.DamageStruct_Thunder, from) then
		sgs.updateIntention(from, to, -50)
	end
end

--羽裳：锁定技，你使用具雷电伤害的【杀】时无距离限制；你的梅花【杀】均视为具雷电伤害的【杀】。
sgs.ai_cardneed.thyuchang = function(to, card, self)
	if not self:willSkipPlayPhase(to) and getCardsNum("Slash", to, self.player) < 1 then
		return card:isKindOf("Slash") and card:getSuit() == sgs.Card_Club
	end
end

--戏画：出牌阶段限一次，你可以将一张手牌面朝下置于你的人物牌上，称为“戏”，视为对一名其他角色使用一张无视距离且不计入使用限制的【杀】或【碎月绮斗】。此【杀】或【碎月绮斗】即将造成伤害时，亮出“戏”，若不为【杀】，防止此伤害；否则你弃置其一张手牌。在结算后，将“戏”置入弃牌堆。
local thxihua_skill = {}
thxihua_skill.name = "thxihua"
table.insert(sgs.ai_skills, thxihua_skill)
thxihua_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThXihuaCard") then return end
	if self.player:isKongcheng() then return end
	return sgs.Card_Parse("@ThXihuaCard=.")
end

sgs.thxihua_choice = ""

sgs.ai_skill_use_func.ThXihuaCard = function(card, use, self)
	sgs.thxihua_choice = ""
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	local not_slash, slash
	for _, c in ipairs(cards) do
		if c:isKindOf("Slash") then
			slash = c
			break
		end
		if not not_slash and not c:isKindOf("Slash") then
			not_slash = c
		end
		if slash and not_slash then
			break
		end
	end
	if not (not_slash or slash) then use.card = nil end
	if slash and self:getCardsNum("Slash") > self.slashAvail then
		self:sort(self.enemies, "defense")
		local duel = sgs.cloneCard("duel")
		for _, p in ipairs(self.enemies) do
			if self:hasTrickEffective(duel, p, self.player) and sgs.card_lack[p:objectName()]["Slash"] == 1 then
				sgs.thxihua_choice = "duel"
				use.card = sgs.Card_Parse("@ThXihuaCard=" .. slash:getEffectiveId())
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
		local aslash = sgs.cloneCard("slash")
		for _, p in ipairs(self.enemies) do
			if self.player:canSlash(p, aslash, false) and self:slashIsEffective(aslash, p) then
				sgs.thxihua_choice = "slash"
				use.card = sgs.Card_Parse("@ThXihuaCard=" .. slash:getEffectiveId())
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	elseif slash then
		self:sort(self.enemies, "defenseSlash")
		local aslash = sgs.cloneCard("slash")
		for _, p in ipairs(self.enemies) do
			if self.player:canSlash(p, aslash, false) and self:slashIsEffective(aslash, p) then
				sgs.thxihua_choice = "slash"
				use.card = sgs.Card_Parse("@ThXihuaCard=" .. slash:getEffectiveId())
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	elseif not_slash and self:getOverflow() > 0 then
		self:sort(self.enemies, "handcard")
		local aslash = sgs.cloneCard("slash")
		for _, p in ipairs(self.enemies) do
			if self.player:canSlash(p, aslash, false) and self:slashIsEffective(aslash, p) then
				sgs.thxihua_choice = ""
				use.card = sgs.Card_Parse("@ThXihuaCard=" .. not_slash:getEffectiveId())
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
	use.card = nil
end

sgs.ai_skill_choice.thxihua = function(self, choices, data)
	if string.find(choices, sgs.thxihua_choice) then return sgs.thxihua_choice end
	local target = data:toPlayer()
	local duel = sgs.cloneCard("duel")
	if self:hasTrickEffective(duel, target, self.player) and sgs.card_lack[target:objectName()]["Slash"] == 1 and string.find(choices, "duel") then
		return "duel"
	end
	local aslash = sgs.cloneCard("slash")
	if self.player:canSlash(target, aslash, false) and self:slashIsEffective(aslash, target) and sgs.card_lack[target:objectName()]["Jink"] == 1 and string.find(choices, "slash") then
		return "slash"
	end
	if self:getCardsNum("Slash") - getCardsNum("Slash", target, self.player) > 2 and string.find(choices, "duel") then
		return "duel"
	end
	if self.player:canSlash(target, aslash, false) and self:slashIsEffective(aslash, target) and getCardsNum("Jink", target, self.player) < 2 and string.find(choices, "slash") then
		return "slash"
	end
	return math.random(1, 3) == 1 and string.find(choices, "slash") and "slash" or (string.find(choices, "duel") and "duel" or "slash")
end

sgs.ai_card_intention.ThXihuaCard = 20

--迷蒙：你可以将你最后一张手牌当一张基本牌、【碎月绮斗】、【灼狱业焰】、【百鬼夜行】、【断灵御剑】、【赤雾锁魂】、【心网密葬】或【三粒天滴】使用或打出。
local thmimeng_skill = {}
thmimeng_skill.name = "thmimeng"
table.insert(sgs.ai_skills, thmimeng_skill)
thmimeng_skill.getTurnUseCard = function(self)
	if self.player:getHandcardNum() ~= 1 then
		return nil
	end
	local card = self.player:getHandcards():first()
	if card:isKindOf("Peach") then
		for _, p in pairs(self.friends) do
			if self:isWeak(p) then
				return nil
			end
		end
	end
	local choices = {}

	local thmimeng = "slash|peach|duel|savage_assault|archery_attack|collateral|iron_chain|dismantlement"
	local thmimengs = thmimeng:split("|")
	for i = 1, #thmimengs do
		local forbiden = thmimengs[i]
		forbid = sgs.cloneCard(forbiden, card:getSuit(), card:getNumber())
		if not self.player:isCardLimited(forbid, sgs.Card_MethodUse, true) and forbid:isAvailable(self.player) then
			table.insert(choices, thmimengs[i])
		end
	end
	local suit = card:getSuitString()
    local number = card:getNumberString()
    local card_id = card:getEffectiveId()

	local uses = {}
	for _, str in ipairs(choices) do
		table.insert(uses, sgs.cloneCard(str, card:getSuit(), card:getNumber()))
	end
	if #uses > 0 then
		self:sortByUseValue(uses, true)
		return sgs.Card_Parse((uses[1]:objectName() .. ":thmimeng[%s:%s]=%d"):format(suit, number, card_id))
	end
end

function sgs.ai_cardsview_valuable.thmimeng(self, class_name, player)
	if player:getHandcardNum() ~= 1 then
		return nil
	end
	local acard = player:getCards("h"):first()
	local suit = acard:getSuitString()
	local number = acard:getNumberString()
    local card_id = acard:getEffectiveId()

	if class_name == "Peach" then
		return ("peach:thmimeng[%s:%s]=%d"):format(suit, number, card_id)
	end
	if class_name == "Jink" then
		return ("jink:thmimeng[%s:%s]=%d"):format(suit, number, card_id)
	end
	if class_name == "Slash" then
		return ("slash:thmimeng[%s:%s]=%d"):format(suit, number, card_id)
	end
	if class_name == "Nullification" then
		return ("nullification:thmimeng[%s:%s]=%d"):format(suit, number, card_id)
	end
end

--暗云：摸牌阶段开始时，你可以放弃摸牌，若如此做，此回合的结束阶段开始时，你摸两张牌。
sgs.ai_skill_invoke.thanyun = function(self, data)
	return not self.player:isKongcheng()
end

--劝善：出牌阶段限一次，你可以令一名有手牌的其他角色将至少一张手牌交给另一名除你以外的角色，若这些牌均为同一类别，你摸一张牌。
local thquanshan_skill = {}
thquanshan_skill.name = "thquanshan"
table.insert(sgs.ai_skills, thquanshan_skill)
thquanshan_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThQuanshanCard") then return end
	if self.player:aliveCount() < 3 then return end
	return sgs.Card_Parse("@ThQuanshanCard=.")
end

sgs.ai_skill_use_func.ThQuanshanCard = function(card, use, self)
	if #self.enemies == 1 and not self.enemies[1]:isKongcheng() then
		use.card = card
		if use.to then
			use.to:append(self.enemies[1])
		end
		return
	end
	if #self.friends_noself == 1 then
		return
	end
	self:sort(self.friends_noself, "handcard")
	self.friends_noself = sgs.reverse(self.friends_noself)
	for _, p in ipairs(self.friends_noself) do
		if not p:isKongcheng() then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
end

sgs.ai_skill_use["@@thquanshangive!"] = function(self, prompt)
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	local f_table = {}
	for _, p in ipairs(self.friends_noself) do
		if p:hasFlag("thquanshan") then
			continue
		end
		table.insert(f_table, p)
	end
	if #f_table == 0 then
		f_table = nil
	end
	local card, friend = self:getCardNeedPlayer(cards, f_table)
	if card and friend then
		return "@ThQuanshanGiveCard=" .. card:getEffectiveId() .. "->" .. friend:objectName()
	end
	self:sortByKeepValue(cards)
	f_table = {}
	for _, p in ipairs(sgs.QList2Table(self.room:getOtherPlayers(self.player))) do
		if p:hasFlag("thquanshan") then
			continue
		end
		table.insert(f_table, p)
	end
	self:sort(f_table, "defense")
	f_table = sgs.reverse(f_table)
	return "@ThQuanshanGiveCard=" .. cards[1]:getEffectiveId() .. "->" .. f_table[1]:objectName()
end

sgs.ai_card_intention.ThQuanshanGiveCard = -20

--仙罡：每当你受到伤害时，可以进行一次判定，若结果为梅花，防止此伤害。
sgs.ai_skill_invoke.thxiangang = function(self, data)
	local damage = data:toDamage()
	return not self.player:isChained() or not self:isGoodChainTarget(self.player, damage.from, damage.nature, damage.damage, damage.card)
end

--断罪：出牌阶段限一次，你可以展示一名其他角色的一张手牌，若为【杀】，视为你对该角色使用一张【碎月绮斗】，此【碎月绮斗】不能被【三粒天滴】响应；若为【闪】或【桃】，视为你对该角色使用一张无视距离且不计入使用限制的【杀】。
local thduanzui_skill = {}
thduanzui_skill.name = "thduanzui"
table.insert(sgs.ai_skills, thduanzui_skill)
thduanzui_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThDuanzuiCard") then return end
	return sgs.Card_Parse("@ThDuanzuiCard=.")
end

sgs.ai_skill_use_func.ThDuanzuiCard = function(card, use, self)
	local slashCount = self:getCardsNum("Slash")
	local duel_targets = {}
	local slash_targets = {}
	for _, p in pairs(self.enemies) do
		local num = p:getHandcardNum()
		if num == 0 then
			continue
		end
		if getKnownCard(p, self.player, "Slash", false, "h") >= num/2 then
			table.insert(duel_targets, p)
		end
		if getKnownCard(p, self.player, "Jink", false, "h") + getKnownCard(p, self.player, "Peach", false, "h") >= num/2 then
			table.insert(slash_targets, p)
		end
	end
	local target
	if #duel_targets > 0 then
		self:sort(duel_targets, "handcard")
		for _, p in pairs(duel_targets) do
			if slashCount >= getCardsNum("Slash", p, self.player) then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
	if not target and #slash_targets > 0 then
		self:sort(slash_targets, "defenseSlash")
		for _, p in pairs(slash_targets) do
			local slash = sgs.cloneCard("slash")
			if not self:slashProhibit(slash, p, self.player) then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
	if not target then
		self:sort(self.enemies, "handcard")
		for _, p in pairs(self.enemies) do
			if not p:isKongcheng() then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
end

sgs.ai_card_intention.ThDuanzuiCard = 50
sgs.ai_use_priority.ThDuanzuiCard = sgs.ai_use_priority.Slash + 0.2

--蛰隐：每当一名角色使用一张非延时类锦囊牌时，你可以弃置一张手牌，并令该角色选择一项：令该牌无效，然后获得之并摸一张牌；或弃置一张牌。然后该角色不可以使用锦囊牌直到回合结束。
sgs.ai_skill_cardask["@thzheyin"] = function(self, data, pattern, target)
	local dis = sgs.QList2Table(self.player:getHandcards())
	if #dis == 0 then
		return "."
	end
	self:sortByKeepValue(dis)
	dis = dis[1]
	if data:toCard() then
		if self:isEnemy(target) then
			return "$" .. dis:getEffectiveId()
		end
	else
		local use = data:toCardUse()
		if (self:willSkipPlayPhase() and self:getOverflow() > 0) or self:getOverflow() > 2 then
			if self:isEnemy(target) and (use.card:isKindOf("AOE") or use.card:isKindOf("Duel") or use.card:isKindOf("Snatch")) then
				return "$" .. dis:getEffectiveId()
			end
		end
		if self:isEnemy(target) and (getKnownCard(target, self.player, "TrickCard", true) > 0 or use.card:isKindOf("ExNihilo") or use.card:getSkillName() == "jade") then
			return "$" .. dis:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_choicemade_filter.cardResponded["@thzheyin"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		local target = self.room:findPlayer(promptlist[#promptlist - 1])
		if target and getKnownCard(target, player, "TrickCard", true) > 0 then
			sgs.updateIntention(player, target, 40)
		end
	end
end

sgs.ai_skill_cardask["@thzheyin-discard"] = function(self, data, pattern, target)
	if target and self:isFriend(target) then return "." end
	if not self.player:canDiscard(self.player, "he") then return "." end
	local card = data:toCard()
	if not card then
		card = data:toCardUse().card
	end
	if not card then
		return "."
	end
	if card:isKindOf("ExNihilo") or card:getSkillName() == "jade" or self:getCardsNum("TrickCard", "he") > 0 or self:getOverflow() >= 1
		or ((self:needKongcheng() or not self:hasLoseHandcardEffective()) and self.player:getHandcardNum() > 0)
		or (self.player:hasSkills(sgs.lose_equip_skill) and self.player:hasEquip())
		or self:needToThrowArmor() then

		local hcards = {}
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if not (isCard("Slash", c, self.player) and self:hasCrossbowEffect()) then table.insert(hcards, c) end
		end
		self:sortByKeepValue(hcards)
		local cards
		local hand, armor, def, off = 0, 0, 0, 0
		if self:needToThrowArmor() then
			cards = self.player:getArmor():getEffectiveId()
		end
		if not cards and (self.player:hasSkills(sgs.need_kongcheng) or not self:hasLoseHandcardEffective()) and self.player:getHandcardNum() > 0 then
			cards = hcards[1]:getEffectiveId()
		end
		if not cards and self.player:hasSkills(sgs.lose_equip_skill) then
			if not cards and self.player:getOffensiveHorse() then
				cards = self.player:getOffensiveHorse():getEffectiveId()
			end
			if not cards and self.player:getArmor() then
				cards = self.player:getArmor():getEffectiveId()
			end
			if not cards and self.player:getDefensiveHorse() then
				cards = self.player:getDefensiveHorse():getEffectiveId()
			end
		end
		if not cards and self.player:getHandcardNum() > 1 then
			cards = hcards[1]:getEffectiveId()
		end
		if not cards and self.player:getOffensiveHorse() then
			cards = self.player:getOffensiveHorse():getEffectiveId()
		end
		if not cards and self.player:getHandcardNum() > 0 then
			cards = hcards[1]:getEffectiveId()
		end
		if not cards and self.player:getArmor() then
			cards = self.player:getArmor():getEffectiveId()
		end
		if not cards and self.player:getDefensiveHorse() then
			cards = self.player:getDefensiveHorse():getEffectiveId()
		end

		if cards then
			return "$" .. cards
		end
	end
	return "."
end

--萤灯：每当你需要使用或打出一张【闪】时，你可以摸一张牌，然后弃置一张牌。
sgs.ai_skill_invoke.thyingdeng = true

--芽吹：摸牌阶段开始时，你可放弃摸牌，改为将一至X张红色手牌交给一名其他角色，然后摸等量的牌（X为该角色已损失的体力值）。
sgs.ai_skill_use["@@thyachui"] = function(self, prompt)
	if #self.friends_noself == 0 then return "." end
	local redcards = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if c:isRed() then
			table.insert(redcards, c)
		end
	end

	if #redcards == 0 then return "." end
	
	self:sort(self.friends_noself, "handcard")
	local targets = {}
	for _, p in ipairs(self.friends_noself) do
		if p:isAlive() and p:isWounded() and p:getLostHp() <= #redcards then
			table.insert(targets, p)
		end
	end
	if #targets == 0 then return "." end

	local compare_func = function(a, b)
		return a:getLostHp() > b:getLostHp() 
	end

	table.sort(targets, compare_func)
	self:sortByUseValue(redcards)
	local cardIds = {}
	for var = 1, targets[1]:getLostHp() do
		table.insert(cardIds, redcards[var]:getId())
	end
	return "@ThYachuiCard=" .. table.concat(cardIds, "+") .. "->" .. targets[1]:objectName()
end

sgs.ai_card_intention.ThYachuiCard = -80

--春痕：每当一名其他角色因弃置而失去牌时，若其中有方块牌，你可以摸一张牌。
sgs.ai_skill_invoke.thchunhen = true

--遐攻:若你的装备区没有武器牌，你可以对与你距离2以内的角色使用【杀】。
--无

--怪谈：每当你的牌因使用而置入弃牌堆，若该牌造成了伤害，你选择一至两名其他角色，然后依次为每名角色选择一种类别的牌，使其不能使用或打出该类别的牌，直到其再次受到一次伤害后，或其下一个回合的回合结束。
sgs.ai_skill_use["@@thguaitan"] = function(self, prompt)
	local targets = {}
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if not enemy:isKongcheng() then
			table.insert(targets, enemy:objectName())
		end
	end
	if #targets > 1 then
		return "@ThGuaitanCard=.->" .. targets[1] .. "+" .. targets[2]
	elseif #targets == 1 then
		return "@ThGuaitanCard=.->" .. targets[1]
	end
	return "."
end

sgs.ai_skill_choice.thguaitan = function(self, choices, data)
	local target = data:toPlayer()
	local knownNum = 0
	local cards = target:getHandcards()
	local t = {}
	t.basic = 0
	t.trick = 0
	t.equip = 0
	for _, card in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s","visible", self.player:objectName(), target:objectName())
		if target:objectName() == self.player:objectName() or card:hasFlag("visible") or card:hasFlag(flag) then
			knownNum = knownNum + 1
			t[card:getType()] = t[card:getType()] + 1
		end
	end
	if (knownNum < target:getHandcardNum() or t["basic"] ~= 0) and target:getMark("@guaitan_basic") == 0 then
		return "BasicCard"
	end
	if t.trick < t.equip and target:getMark("@guaitan_equip") == 0 then
		return "EquipCard"
	elseif target:getMark("@guaitan_trick") == 0 then
		return "TrickCard"
	end
	if target:getMark("@guaitan_basic") == 0 then
		return "BasicCard"
	end
	if target:getMark("@guaitan_trick") == 0 then
		return "TrickCard"
	end
	if target:getMark("@guaitan_equip") == 0 then
		return "EquipCard"
	end
	return "BasicCard"
end

sgs.ai_need_damaged.thguaitan = function(self, attacker, player)
	if player:getMark("@guaitan_basic") > 0 and player:getHp() > 2 then
		return true
	end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.thguaitan = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		local target = findPlayerByObjectName(self.room, promptlist[#promptlist - 1])
		if target then
			sgs.updateIntention(player, target, 50)
		end
	end
end

--后知：锁定技，专属技，每当你受到伤害时，你防止之，然后获得等量的“坚韧”标记。结束阶段开始时，你弃置全部该标记并失去等量体力。
--smart-ai.lua SmartAI:getDamagedEffects

--歃愈：锁定技，每当你于出牌阶段造成一次伤害后，你须弃置1枚“坚韧”标记。
sgs.ai_cardneed.thshayu = function(to, card, self)
	if not self:willSkipPlayPhase(to) and to:getMark("@stiff") > 0 then
		return card:isKindOf("AOE")
	end
end

--毒稼：出牌阶段限一次，你可以弃置一张基本牌，并获得1枚“坚韧”标记，然后摸三张牌。
local thdujia_skill = {}
thdujia_skill.name = "thdujia"
table.insert(sgs.ai_skills, thdujia_skill)
thdujia_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThDujiaCard") then return end
	local n = self.player:hasSkill("thhouzhi") and self.player:getMark("@stiff") + 2 or 0
	if not self:getCard("AOE") and self.player:getHp() <= n then return end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards)
	for _, c in ipairs(cards) do
		if c:getTypeId() == sgs.Card_TypeBasic then
			return sgs.Card_Parse("@ThDujiaCard=" .. c:getEffectiveId())
		end
	end
end

sgs.ai_skill_use_func.ThDujiaCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.ThDujiaCard = 6.8

--仙法：出牌阶段限一次，你可以失去1点体力或弃置一张牌，然后选择一名角色，并选择一个除出牌阶段外的阶段。若如此做，该角色于你此回合的弃牌阶段后额外进行一个该阶段。
local thxianfa_skill = {}
thxianfa_skill.name = "thxianfa"
table.insert(sgs.ai_skills, thxianfa_skill)
thxianfa_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThXianfaCard") then return end
	return sgs.Card_Parse("@ThXianfaCard=.")
end

sgs.ai_skill_use_func.ThXianfaCard = function(card, use, self)
	sgs.thxianfa_combo = {}
	-- dongmo
	local leidi = self.room:findPlayersBySkillName("thdongmo")
	for _, p in sgs.qlist(leidi) do
		if self:isFriend(leidi) and leidi:isWounded() and not leidi:faceUp() then
			sgs.thxianfa_combo[leidi:objectName()] = "finish"
		end
	end

	-- shiting
	local shiliu = self.room:findPlayersBySkillName("thshiting")
	for _, p in sgs.qlist(shiliu) do
		if self:isFriend(shiliu) and shiliu:faceUp() then
			sgs.thxianfa_combo[shiliu:objectName()] = "start"
		end
	end
	
	-- chenyan
	local you = self.room:findPlayersBySkillName("ikchenyan")
	for _, p in sgs.qlist(you) do
		if self:isEnemy(you) and not you:isKongcheng() then
			sgs.thxianfa_combo[you:objectName()] = "discard"
		end
	end
	
	if #sgs.thxianfa_combo ~= 0 then
		local targets = {}
		for p, _ in pairs(sgs.thxianfa_combo) do
			table.insert(targets, self.room:findPlayer(p))
		end
		local cards = sgs.QList2Table(self.player:getCards("he"))
		if #cards == 0 then
			if self.player:getHp() > 1 then
				use.card = card
				if use.to then
					use.to:append(targets[math.random(1, #targets)])
				end
			end
		else
			self:sortByKeepValue(cards)
			use.card = sgs.Card_Parse("@ThXianfaCard=" .. cards[1]:getEffectiveId())
			if use.to then
				use.to:append(targets[math.random(1, #targets)])
			end
		end
		return
	end

	--wake
	if self.player:getHandcardNum() == 1 and self.player:isWounded() and self.player:hasSkill("thwendao") and self.player:getMark("@wendao") == 0 then
		local h_card = self.player:getHandcards():first()
		if not h_card:isKindOf("Peach") then
			sgs.thxianfa_combo[self.player:objectName()] = "start"
			use.card = sgs.Card_Parse("@ThXianfaCard=" .. h_card:getEffectiveId())
			if use.to then
				use.to:append(self.player)
			end
			return
		end
	end
	
	--draw
	self:sort(self.friends, "defense")
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	if #cards ~= 0 then
		sgs.thxianfa_combo[self.friends[1]:objectName()] = "draw"
		use.card = sgs.Card_Parse("@ThXianfaCard=" .. cards[1]:getEffectiveId())
		if use.to then
			use.to:append(self.friends[1])
		end
		return
	end
end

sgs.ai_skill_choice.thxianfa = function(self, choices, data)
	local target = data:toPlayer()
	if sgs.thxianfa_combo[target:objectName()] then
		return sgs.thxianfa_combo[target:objectName()]
	end
	return self:isFriend(target) and "draw" or "discard"
end

sgs.ai_choicemade_filter.skillChoice.thxianfa = function(self, player, promptlist)
	local target = player:getTag("ThXianfaTarget"):toPlayer()
	if target then
		if promptlist[#promptlist] == "discard" then
			sgs.updateIntention(player, target, 50)
		else
			sgs.updateIntention(player, target, -30)
		end
	end
end

--问道：觉醒技，准备阶段开始时，若你没有手牌，你须回复1点体力或摸两张牌，然后减少1点体力上限，并获得技能“迷途”。
sgs.ai_skill_choice.thwendao = function(self, choice)
	if self.player:getHp() < 2 and self.player:getLostHp() > 1 and string.find(choice, "recover") then return "recover" end
	return "draw"
end

--雷矢：每当你对一名角色造成伤害，在结算后，若其体力值不大于1，你可以弃置一张牌并对该角色与之距离最近的另一名角色造成1点雷电伤害。
sgs.ai_skill_use["@@thleishi"] = function(self, prompt, method)
	local victim = self.room:findPlayer(sgs.GetProperty(self.player, "thleishi"))
	if not victim then
		return "."
	end
	if not self.player:canDiscard(self.player, "he") then
		return "."
	end
	local minDis = 998
	local targets = sgs.SPlayerList()
	for _, p in sgs.qlist(self.room:getOtherPlayers(victim)) do
		local dis = victim:distanceTo(p)
		if dis == -1 then
			continue
		end
		if targets:isEmpty() or dis == minDis then
			targets:append(p)
			minDis = dis
		elseif dis < minDis then
			targets= sgs.SPlayerList()
			targets:append(p)
			minDis = dis
		end
	end
	if targets:isEmpty() then
		return "."
	end
	local target
	local enemies = {}
	for _, p in sgs.qlist(targets) do
		if p:isChained() and self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Thunder, 1) then
			target = p
			break
		end
		local damageStruct = {}
		damageStruct.from = self.player
		damageStruct.to = p
		damageStruct.nature = sgs.DamageStruct_Thunder
		damageStruct.damage = 1
		if self:isEnemy(p) and self:damageIsEffective_(damageStruct) then
			table.insert(enemies, p)
		end
	end
	if not target and #enemies == 0 then
		return "."
	end
	if not target then
		self:sort(enemies, "hp")
		target = enemies[1]
	end
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	return "@ThLeishiCard=" .. cards[1]:getEffectiveId() .. "->" .. target:objectName()
end

--闪灵：准备阶段开始时，若你的体力值是全场最少的（或之一），你可以对你攻击范围内的一名角色造成1点雷电伤害。
sgs.ai_skill_playerchosen.thshanling = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	self:sort(targetlist, "hp")
	local victims = {}
	for _, target in ipairs(targetlist) do
		if self:isEnemy(target) and self:damageIsEffective(target, sgs.DamageStruct_Thunder, self.player) then
			table.insert(victims, target)
		end
	end
	if #victims == 0 then
		return nil
	end
	for _, p in ipairs(victims) do
		if p:isChained() and self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Thunder, 1) then
			return p
		end
	end
	return victims[1]
end

--尸解：每当你回复1点体力后，可以从牌堆顶亮出一张牌置于你的人物牌上，称为“皿”。你可以将一张“皿”当【三粒天滴】使用。
sgs.ai_skill_invoke.thshijie = true

sgs.ai_view_as.thshijie = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place == sgs.Player_PlaceSpecial then
		if player:getPile("utensil"):contains(card_id) then
			return ("nullification:thshijie[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

--圣贽：其他角色的回合开始时，你可以令其选择一项：弃置一张【三粒天滴】；或跳过此回合的一个由你指定的阶段，并对你造成1点伤害。
sgs.thshengzhi_choice = ""

sgs.ai_skill_invoke.thshengzhi = function(self, data)
	local target = data:toPlayer()
	if not target then
		return false
	end
	if self:isWeak() then
		return false
	end
	if self:isEnemy(target) then
		if not self:willSkipPlayPhase(target) then
			local n = self:getOverflow(target)
			if not self:willSkipDrawPhase(target) then
				n = n + 2
			end
			if not target:isSkipped(sgs.Player_Discard) then
				if n > 4 or (n > 2 and not target:isWounded()) then
					sgs.thshengzhi_choice = "play"
					return true
				end
			end
		end
		if not self:willSkipDrawPhase(target) then
			if self.player:getHp() > 2 and (self:isWeak(target) or target:isKongcheng()) then
				sgs.thshengzhi_choice = "draw"
				return true
			end
		end
	end
	return false
end

sgs.ai_skill_cardask["@thshengzhi"] = function(self, data, pattern, target)
	if self:isFriend(target) then
		return "."
	elseif sgs.thshengzhi_choice then
		return self:getCardId("Nullification")
	elseif not self:damageIsEffective(target, nil, self.player) then
		return self:getCardId("Nullification")
	end
	return "."
end

sgs.ai_skill_choice.thshengzhi = function(self, choices, data)
	if string.find(choices, sgs.thshengzhi_choice) then
		return sgs.thshengzhi_choice
	end
	local target = data:toPlayer()
	if self:isFriend(target) and string.find(choices, "discard") then
		return "discard"
	elseif self:isEnemy(target) and string.find(choices, "play") then
		return "play"
	elseif self:isEnemy(target) and string.find(choices, "draw") then
		return "draw"
	end
	choices = choices:split("+")
	return choices[math.random(1, #choices)]
end

sgs.ai_choicemade_filter.skillChoice.thshengzhi = function(self, player, promptlist)
	local target = self.room:getCurrent()
	if target then
		if promptlist[#promptlist] == "discard" then
			sgs.updateIntention(player, target, -50)
		elseif promptlist[#promptlist] == "play" or promptlist[#promptlist] == "draw" then
			sgs.updateIntention(player, target, 30)
		end
	end
end

--诏谕：一名角色的准备阶段开始时，你可以将一张牌置于牌堆顶。然后若其判定区有牌或装备区有武器牌，你可以从牌堆底摸一张牌。
sgs.ai_skill_cardask["@thzhaoyu"] = function(self, data, pattern, target)
	if target:objectName() == self.player:objectName() and target:getHandcardNum() == 1 and target:getLostHp() >= 3
			and target:hasSkill("thrudao") and target:getMark("@rudao") == 0 then
		return "$" .. target:getRandomHandCardId()
	end
	if target:getJudgingArea():isEmpty() and not target:getWeapon() then
		return "."
	end
	local cards = sgs.QList2Table(self.player:getCards("he"))
	if #cards == 0 then
		return "."
	end
	self:sortByKeepValue(cards)
	if not target:getJudgingArea():isEmpty() then
		local trick = target:getJudgingArea():last()
		local expp = ""
		if trick:isKindOf("Indulgence") then
			expp = ".|heart"
		elseif trick:isKindOf("SupplyShortage") then
			expp = ".|club"
		elseif trick:isKindOf("PurpleSong") then
			expp = ".|^diamond"
		end
		for _, c in ipairs(cards) do
			if (self:isFriend(target) and sgs.Sanguosha:matchExpPattern(expp, target, c)) or (self:isEnemy(target) and not sgs.Sanguosha:matchExpPattern(expp, target, c)) then
				return "$" .. c:getEffectiveId()
			end
		end
	elseif target:getWeapon() then
		self:sortByUseValue(cards)
		if self:needToThrowArmor(self.player) then
			return "$" .. self.player:getArmor():getEffectiveId()
		elseif self:isFriend(target) then
			for _, c in ipairs(cards) do
				if sgs.ais[target:objectName()]:cardNeed(c) > self:cardNeed(c) then
					return "$" .. c:getEffectiveId()
				end
			end
		end
	end
	return "."
end

sgs.ai_skill_invoke.thzhaoyu_draw = function(self)
	return self.player:objectName() ~= self.room:getCurrent():objectName() or not self.player:isKongcheng()
end

sgs.ai_choicemade_filter.cardResponded["@thzhaoyu"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		local target = self.room:findPlayer(promptlist[#promptlist - 1])
		if target then
			if not target:getJudgingArea():isEmpty() then
				local card = sgs.Card_Parse(string.sub(promptlist[#promptlist], 2, -2))
				if card then
					local trick = target:getJudgingArea():last()
					local expp = ""
					if trick:isKindOf("Indulgence") then
						expp = ".|heart"
					elseif trick:isKindOf("SupplyShortage") then
						expp = ".|club"
					elseif trick:isKindOf("PurpleSong") then
						expp = ".|^diamond"
					end
					if sgs.Sanguosha:matchExpPattern(expp, target, card) then
						sgs.updateIntention(player, target, -50)
					else
						sgs.updateIntention(player, target, 20)
					end
				end
			end
		end
	end
end

--无忤：锁定技，弃牌阶段开始时，你须弃置一张手牌。
--smart-ai.lua SmartAI:getOverflow

--入道：觉醒技，准备阶段开始时，若你没有手牌，你须摸两张牌，然后减少3点体力上限，并失去技能“无忤”。
--无

--六震：出牌阶段限一次，当你使用【杀】指定目标后，可以选择至少一名不为该【杀】目标的其他角色，该【杀】在结算后置入弃牌堆时，你无视距离对你所选的角色使用，其中每有一名角色使用【闪】抵消了该【杀】的效果，你须弃置一张牌或失去1点体力。
sgs.ai_skill_use["@@thliuzhen"] = function(self, prompt)
	local targetNames = {}
	local card = self.player:getTag("thliuzhen_carduse"):toCardUse().card
	self:sort(self.enemies, "defenseSlash")
	local n = self:getOverflow()
	local extra = 0
	for _, p in ipairs(self.enemies) do
		if p:hasFlag("liuzhenold") then continue end
		if (getCardsNum("Jink", p, self.player) < 1 or sgs.card_lack[p:objectName()]["Jink"] == 1)
				and not self:slashProhibit(card, p, self.player) then
			table.insert(targetNames, p:objectName())
		elseif not self:slashProhibit(card, p, self.player) and extra <= n / 2 then
			extra = extra + 1
			table.insert(targetNames, p:objectName())
		end
	end

	if #targetNames > 0 then
		return "@ThLiuzhenCard=.->" .. table.concat(targetNames, "+")
	end
	return "."
end

sgs.ai_skill_discard.thliuzhen = function(self, discard_num, min_num, optional, include_equip)
	if self:getOverflow() >= 0 then
		local ret = self:askForDiscard("", 1, 1, false, true)
		if #ret ~= 0 then
			if isCard("Peach", ret[1], self.player) then
				return {}
			else
				return ret
			end
		else
			return {}
		end
	end
	return self:isWeak(self.player) and self:askForDiscard("", 1, 1, false, true) or {}
end

sgs.ai_card_intention.ThLiuzhenCard = sgs.ai_card_intention.Slash

--天禅：君主技，当你处于濒死状态时，其他花势力的角色可以将黑桃牌当【桃】使用。
function sgs.ai_cardsview_valuable.thtianchanv(self, class_name, player)
	local spades = {}
	if class_name == "Peach" then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or not dying:hasLordSkill("thtianchan") or self.player:getKingdom() ~= "hana"
				or dying:objectName() == player:objectName() then
			return nil
		end
		if self:isFriend(dying, player) then
			local cards = player:getCards("h")
			for _, c in sgs.qlist(cards) do
				if c:getSuit() == sgs.Card_Spade then
					table.insert(spades, c)
				end
			end
		end
	end
	if #spades > 0 then
		self:sortByUseValue(spades)
		local suit = spades[1]:getSuitString()
		local number = spades[1]:getNumberString()
		local card_id = spades[1]:getEffectiveId()
		return ("peach:thtianchanv[%s:%s]=%d"):format(suit, number, card_id)
	end
	return nil
end
