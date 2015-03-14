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
		local diff =  target:getCards(flag):length() -  source:getCards(flag):length() 
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
