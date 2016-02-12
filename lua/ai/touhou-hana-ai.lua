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
		if self.isFriend(target) and self:isGoodChainTarget(target, self.player, sgs.DamageStruct_Fire, 1) then
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

--彼岸：当一名处于濒死状态的角色成为【桃】的目标时，你可弃置一张锦囊牌，令此【桃】对其无效。
sgs.ai_skill_cardask["@thbian"] = function(self, data, pattern, target)
	if self:isEnemy(target) then
		local tricks= {}
		for _,c in sgs.qlist(self.player:getHandcards()) do
			if c:isKindOf("TrickCard")  then
				table.insert(tricks,c)
			end
		end
		if #tricks == 0 then return "." end
		self:sortByKeepValue(tricks)
		return "$" .. tricks[1]:getId()
	end
	return "."
end

sgs.ai_choicemade_filter.cardResponded["@thbian"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		local target = self.room:findPlayer(promptlist[#promptlist - 1])
		if target then
			sgs.updateIntention(player, target, 100)
		end
	end
end

--归航：当一名角色进入濒死状态时，你可展示其一张手牌，若为红色，该角色须弃置之并回复1点体力。
sgs.ai_skill_invoke.thguihang = function(self, data)
	local dying = data:toDying()
	local isFriend = false
	local allBlack = true
	if dying.who:isKongcheng() then return false end

	isFriend = not self:isEnemy(dying.who)
	if not sgs.GetConfig("EnableHegemony", false) and self.role == "renegade"
		and not (dying.who:isLord() or dying.who:objectName() == self.player:objectName())
		and not (self.room:getMode() == "couple" and dying.who:getGeneralName() == "sunjian")
		and (sgs.current_mode_players["loyalist"] + 1 == sgs.current_mode_players["rebel"]
				or sgs.current_mode_players["loyalist"] == sgs.current_mode_players["rebel"]
				or self.room:getCurrent():objectName() == self.player:objectName()) then
		isFriend = false
	end

	local knownNum = 0
	local cards = dying.who:getHandcards()
	for _, card in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s","visible", self.player:objectName(), dying.who:objectName())
		if dying.who:objectName() == self.player:objectName() or card:hasFlag("visible") or card:hasFlag(flag) then
			knownNum = knownNum + 1
			if card:isRed() then allBlack = false end
		end
	end
	if knownNum < dying.who:getHandcardNum() then allBlack = false end

	return isFriend and not allBlack
end

sgs.ai_cardshow.thguihang = function(self, requestor)
	assert(self.player:objectName() == requestor:objectName())

	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		if card:isRed() then
			return card
		end
	end

	return cards[1]
end

sgs.ai_choicemade_filter.skillInvoke.thguihang = function(self, player, promptlist)
	local dying = self.room:getCurrentDyingPlayer()
	if promptlist[#promptlist] == "yes" then
		if dying and dying:objectName() ~= self.player:objectName() then sgs.updateIntention(player, dying, -80) end
	elseif promptlist[#promptlist] == "no" then
		if not dying or dying:isKongcheng() or dying:objectName() == self.player:objectName() then return end
		local allBlack = true
		local knownNum = 0
		local cards = dying:getHandcards()
		for _, card in sgs.qlist(cards) do
			local flag = string.format("%s_%s_%s","visible", player:objectName(), dying:objectName())
			if card:hasFlag("visible") or card:hasFlag(flag) then
				knownNum = knownNum + 1
				if card:isRed() then allBlack = false end
			end
		end
		if knownNum < dying:getHandcardNum() then allBlack = false end
		if not allBlack then sgs.updateIntention(player, dying, 80) end
	end
end

--无间：出牌阶段限一次，你可弃置一张牌并指定你攻击范围内的一名角色，直到你的下回合开始，该角色计算与除其以外的角色的距离时，始终+1。
local thwujian_skill = {}
thwujian_skill.name = "thwujian"
table.insert(sgs.ai_skills, thwujian_skill)
thwujian_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThWujianCard") then return end
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	return sgs.Card_Parse("@ThWujianCard=" .. cards[1]:getEffectiveId())
end

sgs.ai_skill_use_func.ThWujianCard = function(card, use, self)
	local rangefix = 0
	if self.player:getWeapon() and self.player:getWeapon():getId() == card:getSubcards():first() then
		rangefix = rangefix + sgs.weapon_range[sgs.Sanguosha:getCard(self.player:getWeapon()):getClassName()] - self.player:getAttackRange(false)
	end
	if self.player:getOffensiveHorse() and self.player:getOffensiveHorse():getId() == card:getSubcards():first() then
		rangefix = rangefix + 1
	end
	local targets = {}
	for _, p in ipairs(self.enemies) do
		if self.player:inMyAttackRange(p, rangefix) then
			table.insert(targets, p)
		end
	end
	if #targets == 0 then return end
	local target
	local max_danger = 0
	for _, p in ipairs(targets) do
		local current_danger = 0
		for _, f in ipairs(self.friends) do
			if p:distanceTo(f) == p:getAttackRange() then
				if self.isWeak(f) then
					current_danger = current_danger + 2
				end
				current_danger = current_danger + 1
			end
		end
		if current_danger > max_danger then
			max_danger = current_danger
			target = p
		end
	end
	if target and (max_danger >= 3 or self:getOverflow() > 0) then
		use.card = card
		if use.to then
			use.to:append(target)
		end
		return
	end
end

sgs.ai_card_intention.ThWujianCard = 30

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
	local arr1, arr2 = self:getWoundedFriend()
	local target = nil
	if #arr1 > 0 then
		for i = 1, #arr1 do
			if (self:isWeak(arr1[i]) or self:getOverflow() >= 1) and arr1[i]:getHp() < getBestHp(arr1[i]) and targets:contains(arr1[i]) then
				target = arr1[i]
				break
			end
		end
	end
	return target
end

sgs.ai_playerchosen_intention.thxinwang = -40

--绝毒：锁定技，杀死你的角色获得技能“崩坏”。
function sgs.ai_slash_prohibit.thjuedu(self, from, to)
	if from:hasSkill("ikxuwu") or (from:hasSkill("ikwanhun") and from:distanceTo(to) == 1) then return false end
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
		if self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Thunder, 1) then
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

sgs.ai_playerchosen_intention.thjilan = function(self, from, to)
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

--【断罪】ai
local thduanzui_skill = {}
thduanzui_skill.name = "thduanzui"
table.insert(sgs.ai_skills, thduanzui_skill)
thduanzui_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThDuanzuiCard") then return nil end
	if #self.enemies ==0 then return nil end
	return sgs.Card_Parse("@ThDuanzuiCard=.")
end

sgs.ai_skill_use_func.ThDuanzuiCard = function(card, use, self)
	local slashCount=getCardsNum("Slash", self.player, self.player)
	local duel_targets={}
	local slash_targets={}
	for _,p in pairs(self.enemies) do
		local num=p:getHandcardNum()
		if getKnownCard(p, self.player, "Slash",  false, "h")>= num/2 then
			table.insert(duel_targets,p)
		end
		if getKnownCard(p, self.player, "Jink",  false, "h")>= num/2 then
			table.insert(slash_targets,p)
		end
	end
	local target
	if #duel_targets>0 then
		self:sort(duel_targets, "handcard")
		for _, p in pairs(duel_targets) do
			if slashCount >= getCardsNum("Slash", p, self.player) and not p:isKongcheng() then
				target =p
				break
			end
		end
	end
	if not target and #slash_targets>0 then
		self:sort(slash_targets, "defenseSlash")
		for _, p in pairs(slash_targets) do
			local slash = sgs.Sanguosha:cloneCard("slash")
			if not self:slashProhibit(slash,p,self.player) and not p:isKongcheng() then
				--sgs.isGoodTarget(p, self.enemies, self)
				target = p
				break
			end
		end
	end
	if not target then
		self:sort(self.enemies, "handcard")
		for _, p in pairs(self.enemies) do
			if not p:isKongcheng() then
				target = self.enemies[1]
			end
		end
	end
	use.card = card
	if use.to then
		use.to:append(target)
		if use.to:length() > 0 then return end
	end
end
sgs.ai_card_intention.ThDuanzuiCard = 50


--【芽吹】ai
sgs.ai_skill_use["@@thyachui"] = function(self, prompt)
	
	if #self.friends_noself == 0  then return "." end
	local redcards ={}
	for _,c in sgs.qlist(self.player:getHandcards()) do
		if c:isRed() then
			table.insert(redcards,c)
		end
	end
	
	
	if #redcards==0 then return "." end
	
	--SmartAI:findPlayerToDraw(include_self, drawnum)
	--芽吹目标有限制条件，所以这个函数并不好用
	
	self:sort(self.friends_noself,"handcard")
	local targets={}
	for _, p in ipairs(self.friends_noself) do
		if p:getLostHp() <= #redcards then
			table.insert(targets,p)
		end
	end
	local compare_func = function(a, b)
		return a:getLostHp() > b:getLostHp() 
	end
	
	if #targets >=1 then
		table.sort(targets, compare_func)
		self:sortByUseValue(redcards)
		local cardIds = {}
		for var = 1, targets[1]:getLostHp() , 1 do
			table.insert(cardIds,redcards[var]:getId())
		end		
		return "@ThYachuiCard=".. table.concat(cardIds, "+") .."->" .. targets[1]:objectName()
	end
	return "."	 
end
sgs.ai_card_intention.ThYachuiCard = -80


--【春痕】ai
sgs.ai_skill_invoke.thchunhen = true


function SmartAI:ChainDamage(damage,from, to)
	local x=0
	local y=0
	if self:isFriend(to) then
		y= damage
	end
	if self:isEnemy(to) then
		x= damage
	end
	if not to:isChained() then
		return x,y
	end

	--开始传导铁锁
	local tos = sgs.SPlayerList()
	for _,p in sgs.qlist(self.room:getOtherPlayers(to)) do
		if p:isChained() then
			tos:append(p)
		end
	end
	if not tos:isEmpty() then
		self.room:sortByActionOrder(tos)
		for _,p  in sgs.qlist(tos) do
			if self:isFriend(p) and self:damageIsEffective(p, sgs.DamageStruct_Thunder, from) then
				y=y + damage
			end
			if self:isEnemy(p) and self:damageIsEffective(p, sgs.DamageStruct_Thunder, from) then
				x=x + damage
			end
		end
	end
	return x,y
end
function SmartAI:getThunderAttackTargets(targets,consider_chain)
	consider_chain = consider_chain or true
	local enemies = {}
	--local friends = {}
	local weakers = {}
	for _, p in sgs.qlist(targets) do
		if self:damageIsEffective(p, sgs.DamageStruct_Thunder, self.player)  then
			local x,y=0,0 
			if consider_chain then
				x , y =  self:ChainDamage(1,self.player, p)
			end
			if x >= y then
				--if self:isFriend(p) and self:getDamagedEffects(p, self.player, false) then
				--	table.insert(friends,p)
				--end
				if	 self:isEnemy(p) and not self:getDamagedEffects(p, self.player, false) then
					table.insert(enemies,p)
					if self.player:hasSkill("thleishi") and p:getHp() == 1 and self:getLeishiTarget(p) then
						table.insert(weakers,p)
					end
				end
			end
		end
	end
	return enemies, weakers
end
--其实还需要一个预估伤害的函数 记得绮符剧里有属性免疫还是属性伤害加深神马的

function SmartAI:getLeishiTarget(victim)
	local  minDis = 998; 
	local targets = sgs.SPlayerList()
	for _,p in sgs.qlist(self.room:getOtherPlayers(victim))do
		local dis = victim:distanceTo(p); 
		if (dis == -1) then
			continue end
		if (targets:isEmpty() or dis == minDis)  then
			targets:append(p)  
			minDis = victim:distanceTo(p)
		 elseif (dis < minDis) then
			targets= sgs.SPlayerList()
			targets:append(p)
			 minDis = victim:distanceTo(p)
		end
	end
	for _,p in sgs.qlist(targets)do
		if self:isEnemy(p) and self:damageIsEffective(p, sgs.DamageStruct_Thunder, self.player) 
		then
			return true
		end
	end
	return false
end

--【雷矢】ai
sgs.ai_skill_playerchosen.thleishi = function(self, targets)
	local enemies, weakers = self:getThunderAttackTargets(targets,false)
	if #weakers>0 then
		return weakers[1]
	end
	if #enemies>0 then
		self:sort(enemies, "hp")
		return enemies[1]
	end
	return nil
end

--【闪灵】ai
sgs.ai_skill_playerchosen.thshanling = function(self, targets)
	local enemies, weakers = self:getThunderAttackTargets(targets)
	if #weakers>0 then
		return weakers[1]
	end
	if #enemies>0 then
		self:sort(enemies, "hp")
		return enemies[1]
	end
	return nil
end
--目前没有playerchosen仇恨,由伤害仇恨代替
--其实可以加入主动为卖血队友提供伤害
--不过由于ai里damage事件应该会导致直接增加仇恨。 
--需要smart-ai里设置no intention damage 


--【六震】ai
sgs.ai_skill_use["@@thliuzhen"] = function(self, prompt)
	local targetNames={}
	local card = self.player:getTag("thliuzhen_carduse"):toCardUse().card
	for _,p in ipairs(self.enemies) do
		if p:hasFlag("liuzhenold") then continue end
		if (getCardsNum("Jink", p, self.player) < 1 
				or sgs.card_lack[p:objectName()]["Jink"] == 1 )
			and not self:slashProhibit(card, p, self.player)then
			table.insert(targetNames,p:objectName())
		end
	end
	
	if #targetNames>0 then
		return "@ThLiuzhenCard=.->" .. table.concat(targetNames, "+")
	end
	return "."
end
--liuzhen intention = slash intention?


--【天禅】ai
function sgs.ai_cardsview_valuable.thtianchanv(self, class_name, player)
	local spades={}
	if class_name == "Peach" then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or not dying:hasLordSkill("thtianchan") or self.player:getKingdom() ~="hana"
			or dying:objectName() == player:objectName() then 
			return nil 
		end
		if self:isFriend(dying, player) then 
			local cards=player:getCards("h")
			for _,c in sgs.qlist(cards) do
				if c:getSuit()==sgs.Card_Spade then
					table.insert(spades,c)
				end
			end
		end
	end
	if #spades>0 then
		local suit =spades[1]:getSuitString()
		local number = spades[1]:getNumberString()
		local card_id = spades[1]:getEffectiveId()	
		return ("peach:thtianchan[%s:%s]=%d"):format(suit, number, card_id) 
	else
		return nil
	end
end
