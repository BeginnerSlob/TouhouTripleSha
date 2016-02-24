--凡识：若你的手牌数不小于体力值，你可将一张红色手牌在你的回合内当【杀】，或在你的回合外当【闪】或【桃】使用或者打出。
local thfanshi_skill = {}
thfanshi_skill.name = "thfanshi"
table.insert(sgs.ai_skills, thfanshi_skill)
thfanshi_skill.getTurnUseCard = function(self)
	if self.player:getHandcardNum() < self.player:getHp() then return end
	local cards = self.player:getCards("h")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local red_card

	self:sortByUseValue(cards, true)

	for _, card in ipairs(cards) do
		if card:isRed() and not self:isValuableCard(card) then
			red_card = card
			break
		end
	end

	if not red_card then return nil end
	local suit = red_card:getSuitString()
	local number = red_card:getNumberString()
	local card_id = red_card:getEffectiveId()
	local card_str = ("slash:thfanshi[%s:%s]=%d"):format(suit, number, card_id)
	local slash = sgs.Card_Parse(card_str)
	assert(slash)

	return slash
end

sgs.ai_cardsview.thfanshi = function(self, class_name, player)
	if self.player:getHandcardNum() < self.player:getHp() then return nil end
	if self.player:getPhase() == sgs.Player_NotActive and (class_name ~= "Peach" and class_name ~= "Jink") then return nil end
	if self.player:getPhase() ~= sgs.Player_NotActive and class_name ~= "Slash" then return nil end
	local cards = player:getCards("h")
	for _, id in sgs.qlist(getWoodenOxPile(player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	sgs.ais[player:objectName()]:sortByKeepValue(cards)

	local red_card = nil
	for _, card in ipairs(cards) do
		if card:isRed() and not self:isValuableCard(card, player) then
			red_card = card
			break
		end
	end
	if not red_card then return nil end
	local suit = red_card:getSuitString()
	local number = red_card:getNumberString()
	local card_id = red_card:getEffectiveId()
	if class_name == "Jink" then
		return ("jink:thfanshi[%s:%s]=%d"):format(suit, number, card_id)
	elseif class_name == "Peach" then
		return ("peach:thfanshi[%s:%s]=%d"):format(suit, number, card_id)
	elseif class_name == "Slash" then
		return ("slash:thfanshi[%s:%s]=%d"):format(suit, number, card_id)
	end
	return nil
end

--霁风：锁定技，你的手牌上限+1。
--无

--霓裳：准备阶段开始时，你可以将一名其他角色的人物牌横置或者重置。
sgs.ai_skill_playerchosen.thnichang = function(self, targets)
	return self:findPlayerToChain(targets)
end

--奇门：锁定技，当你计算与人物牌横置的角色的距离、人物牌横置的角色计算与你的距离时，无视你们之外的其他角色。
--无

--贯甲：锁定技，人物牌横置的其他角色于你的回合内使用的第一张牌无效。
--smart-ai.lua SmartAI:askForNullification
--smart-ai.lua SmartAI:willUsePeachTo
--standard-ai.lua sgs.ai_skill_cardask["@multi-jink-start"]
--standard_cards-ai.lua sgs.ai_skill_cardask["slash-jink"]

--【万灵】ai
sgs.ai_skill_invoke.thwanling = function(self,data)
	local move = data:toMoveOneTime()
	local target
	local card =move.m_extraData
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player))do
		if p:objectName()==move.from:objectName() then
			target=p
			break
		end
	end
	if target and card then
		if self:isFriend(target) then
			return true
		else
			if not self.player:canDiscard(self.player, "he") then return false end
			local cards = self:askForDiscard("thwanling", 1, 1, false, true)
			table.insert(cards,card)
			self:sortByUseValue(cards)
			if cards[1]:getEffectiveId()<0 or move.card_ids:contains(cards[1]:getEffectiveId())  then
				return false
			else
				return true
			end
		end
	end
	return false
end
sgs.ai_skill_cardask["@thwanling"] = function(self, data)
	local move = data:toMoveOneTime()
	local target
	local card =move.m_extraData
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player))do
		if p:objectName()==move.from:objectName() then
			target=p
			break
		end
	end
	-- for ai intention
	local ai_data=sgs.QVariant()
	ai_data:setValue(target)
	self.player:setTag("thwanling_target",ai_data)
	
	if not self:isFriend(target) then
		local to_discard=self:askForDiscard("thwanling", 1, 1, false, true)
		return "$" .. to_discard[1]:getId()
	elseif card then
		local optional_discard=self:askForDiscard("thwanling", 1, 1, true, true)
		if #optional_discard>0 then
			local need_obtain
			if card:isKindOf("Slash") then
				need_obtain = getCardsNum("Slash", self.player, self.pplayer)<1
			elseif card:isKindOf("AOE") then
				need_obtain=self:getAoeValue(card, self.player)>0
			end
			if need_obtain then
				return "$" .. optional_discard[1]:getId()
			end
		end
	end
	return "."
end
sgs.ai_choicemade_filter.cardResponded["@thwanling"] = function(self, player, promptlist)
	if promptlist[#promptlist] == "_nil_" then
		local target =player:getTag("thwanling_target"):toPlayer()
		if not target then return end
		sgs.updateIntention(player, target, -80)
	end
end

--【醉步】ai
sgs.ai_skill_invoke.thzuibu = function(self,data)
	local target = data:toPlayer()
	return self:isFriend(target)
end
--smart-ai  damageIsEffective  getAoeValueTo

--【魔盗】ai
--送关键牌给队友什么的高级手法。。。不考虑
--其实还需要一个排序函数 不过懒了
local parseTargetForModao = function(self,source,target)
	local flag = "h"
	local diff = 0
	 local isGood = false
	local handDiff = target:getCards("h"):length() -  source:getCards("h"):length() 
	local equipDiff = target:getCards("e"):length() -  source:getCards("e"):length() 
	local judgeDiff = target:getCards("j"):length() -  source:getCards("j"):length() 
	local standardMax = source:getLostHp() + 1
	if self:isFriend(source,target) then
		if  target:hasSkills(sgs.lose_equip_skill)  then
			diff = equipDiff
			flag = "e"
			isGood = true
		end
		--闪电比较复杂 暂时不管了
		if handDiff > 0 and math.abs(handDiff) <= standardMax and  (target:containsTrick("indulgence") or target:containsTrick("supply_shortage") ) then
			diff = judgeDiff
			flag = "j"
			isGood = true
		end
	else
		if equipDiff > 0 and math.abs(equipDiff) <= standardMax and not target:hasSkills(sgs.lose_equip_skill) then
			diff = equipDiff
			flag = "e"
			isGood = true
		end
		if handDiff > 0 and math.abs(handDiff) <= standardMax then
			isGood = true
			if diff > 0  then
				if diff < handDiff then
					diff = handDiff
					flag = "h"
				end
			else
				diff = handDiff
				flag = "h"
			end
		end
	end
	return isGood,flag,diff
end
sgs.ai_skill_playerchosen.thmodao = function(self, targets)
	for _,p in sgs.qlist(targets)do
		if parseTargetForModao(self,self.player, p) then
			return p
		end
	end
	return nil
end
sgs.ai_skill_choice.thmodao= function(self, choices, data)	
	local target = self.player:getTag("ThModaoTarget"):toPlayer()
	local isGood,flag =  parseTargetForModao(self,self.player, target)
	return flag
end
sgs.ai_choicemade_filter.skillInvoke.thmodao = function(self, player, promptlist)
	local target = player:getTag("ThModaoTarget"):toPlayer()
	if target then
		local flag =  promptlist[#promptlist] 
		local diff =  target:getCards(flag):length() - player:getCards(flag):length() 
		local friendly
		if flag == "e" then
			if not target:hasSkills(sgs.lose_equip_skill) then
				friendly = diff < 0
			else
				friendly = true
			end
		elseif flag == "j" or flag == "h" then
			friendly = diff < 0
		end
		if friendly then
			sgs.updateIntention(player, target, -50)
		else
			sgs.updateIntention(player, target, 50)
		end	
	end
end


--如何更好的获取和为9的集合？？
function SmartAI:findTableByPlusValue(cards, neednumber, plus, pointer,need_cards)
		if neednumber == 0 and plus == 9 then 
			return true
		end
		if  pointer > #cards then
			return false 
		end
		for i = pointer, #cards, 1 do
			if self:getRealNumber(cards[i]) <= neednumber then
				if self:findTableByPlusValue(cards, 9-plus-self:getRealNumber(cards[i]), plus+self:getRealNumber(cards[i]),i+1,need_cards) then
					table.insert(need_cards,cards[i]:getId())
					return true
				end
			end
		end
		if neednumber == 0 and plus == 9 then 
			return true
		else
			return false 
		end 
	end
--【数术】ai
--不清楚收益，暂时不写 = =

--【封凌】ai
sgs.ai_skill_invoke.thfengling = function(self,data)
	return true
end
sgs.ai_skill_use["@@thfengling"] = function(self, prompt)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	if #cards == 0 then return "." end 
	self:sortByKeepValue(cards)
	

	local function numberCompareFunc(card1,card2)
		return self:getRealNumber(card2) > self:getRealNumber(card1)
	end
	--按从大到小排序 同点数则按keepvalue
	local function bubbleSort(cards,numberCompareFunc)  
		local len = #cards 
		local i = len  
		while i > 0 do  
			j=1  
			while j< len do  
				if numberCompareFunc(cards[j],cards[j+1]) then  
					cards[j],cards[j+1] = cards[j+1],cards[j]  
				end  
				j = j + 1  
			end  
			i = i - 1  
		end  
	end 
	bubbleSort(cards,numberCompareFunc)
	
	local need_cards ={}
	local find9  = self:findTableByPlusValue(cards, 9, 0, 1,need_cards)
	
	if find9 and #need_cards > 0 then
		return "@ThFenglingCard="..  table.concat(need_cards, "+")
	end
	return "."
end

--偶祭：每当你或你攻击范围内的一名角色的装备区于你的回合内改变时，你可以选择一项：弃置一名其他角色的一张手牌；或摸一张牌。
sgs.ai_skill_invoke.thouji = true

sgs.ai_skill_playerchosen.thouji = function(self, targets)
	local target = self:findPlayerToDiscard("h", false, true, targets)
	if target and self:isEnemy(target) and ((target:getHandcardNum() == 1 and not self:needKongcheng(target, true)) or (target:getHandcardNum() == 2 and getKnownCard(target, self.player, "Peach") > 0)) then
		return target
	end
	return nil
end

sgs.ai_playerchosen_intention.thouji = 30

--镜缘：出牌阶段限一次，你可以弃置一张红色基本牌，然后令装备区里有牌的一至两名角色各选择一项：将其装备区里的一张牌交给除其以外的一名角色；或令你获得其一张手牌。
local thjingyuansp_skill = {}
thjingyuansp_skill.name = "thjingyuansp"
table.insert(sgs.ai_skills, thjingyuansp_skill)
thjingyuansp_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThJingyuanspCard") or not self.player:canDiscard(self.player, "h") then return end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards)
	for _, c in ipairs(cards) do
		if c:isRed() and c:getTypeId() == sgs.Card_TypeBasic then
			return sgs.Card_Parse("@ThJingyuanspCard=" .. c:getId())
		end
	end
end

function thjingyuansp_inMyAttackRange(from, target, equip)
	if from:objectName() == target:objectName() then
		return true
	end
	local fix = 0
	if equip:isKindOf("DefensiveHorse") then
		fix = fix - 1
	end
	return from:inMyAttackRange(target, fix)
end

sgs.ai_skill_use_func.ThJingyuanspCard = function(card, use, self)
	local targets = {}
	self:sort(self.friends, "defense")
	self.friends = sgs.reverse(self.friends)
	for _, p in ipairs(self.friends) do
		if not p:hasEquip() and self:isWeak(p) then
			continue
		end
		for _, c in sgs.qlist(p:getEquips()) do
			if thjingyuansp_inMyAttackRange(self.player, p, c) then
				table.insert(targets, p)
				break
			end
		end
	end
	self:sort(self.enemies, "defense")
	for _, p in ipairs(self.enemies) do
		if not p:hasEquip() then
			continue
		end
		if self.player:inMyAttackRange(p) then
			table.insert(targets, p)
			break
		end
	end
	for _, p in ipairs(self.enemies) do
		if not p:hasEquip() then
			continue
		end
		if not table.contains(targets, p) then
			table.insert(targets, p)
			break
		end
	end
	for _, p in ipairs(self.friends) do
		if not p:hasEquip() or self:isWeak(p) then
			continue
		end
		if not table.contains(targets, p) then
			table.insert(targets, p)
			break
		end
	end
	if #targets ~= 0 then
		use.card = card
		if use.to then
			use.to:append(targets[1])
			if #targets > 1 then
				use.to:append(targets[2])
			end
		end
		return
	end
	use.card = nil
end

sgs.ai_skill_cardask["@thjingyuansp"] = function(self, data, pattern, target)
	if self:isFriend(target) then
		return sgs.ai_skill_cardask["@thjingyuansp-give"](self, data, pattern, target)
	else
		if self.player:getEquips():length() == 1 and self.player:getArmor() then
			return "."
		else
			local cards = sgs.QList2Table(self.player:getCards("e"))
			self:sortByKeepValue(cards)
			for _, cd in ipairs(cards) do
				if thjingyuansp_inMyAttackRange(target, self.player, cd) then
					continue
				else
					return "$" .. cd:getEffectiveId()
				end
			end
		end
	end
	return "."
end

sgs.ai_skill_cardask["@thjingyuansp-give"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getCards("e"))
	self:sortByKeepValue(cards)
	for _, cd in ipairs(cards) do
		if thjingyuansp_inMyAttackRange(target, self.player, cd) then
			if self:isFriend(target) then
				return "$" .. cd:getEffectiveId()
			else
				continue
			end
		end
	end
	return "$" .. cards[1]:getEffectiveId()
end

sgs.ai_skill_playerchosen.thjingyuansp = function(self, targets)
	local ailisis = self.room:findPlayersBySkillName("thjingyuansp")
	for _, ailisi in sgs.qlist(ailisis) do
		if self:isFriend(ailisi) and targets:contains(ailisi) and ailisi:getPhase() == sgs.Player_Play then
			return ailisi
		end
	end
	local card = self.player:getTag("ThJingyuanspCard"):toCard()
	local _, target = self:getCardNeedPlayer({card}, self.friends_noself, false)
	return self.friends_noself[1]
end

sgs.ai_playerchosen_intention.thjingyuansp = -30