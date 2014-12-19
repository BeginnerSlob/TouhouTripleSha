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

