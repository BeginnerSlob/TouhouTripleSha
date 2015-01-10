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
