--【血兰】ai
sgs.ai_skill_cardask["@thxuelan"] = function(self, data)
	local peach_effect = data:toCardEffect()
	if not self:isEnemy(peach_effect.to) then return "." end
	local reds={}
	for _,c in sgs.qlist(self.player:getCards("he")) do
		if c:isRed()  then
			table.insert(reds,c)
		end
	end
	if #reds==0 then return "." end
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
--血兰的仇恨ai需要技能代码提供Target
--sgs.ai_choicemade_filter.cardResponded["@thxuelan"] = function(self, player, promptlist)

--【心妄】ai
sgs.ai_skill_invoke.thxinwang = true
sgs.ai_skill_playerchosen.thxinwang = function(self, targets)
	local arr1, arr2 = self:getWoundedFriend()
	local target = nil
	if #arr1 > 0 and (self:isWeak(arr1[1]) or self:getOverflow() >= 1) and arr1[1]:getHp() < getBestHp(arr1[1]) then target = arr1[1] end
	if target then
		return target
	end
	return nil
end
sgs.ai_playerchosen_intention.thxinwang = -30	
--【霆舞】ai
sgs.ai_skill_invoke.thtingwu = function(self,data)
	local target=self.player:getTag("ThTingwuTarget"):toPlayer()
	if self:isEnemy(target) then
		if not self:getDamagedEffects(target, self.player,nil) then	
			return true
		elseif self:isGoodChainTarget(target, self.player, sgs.DamageStruct_Thunder, 1, nil) then
			return true
		end
	end
	
	return false
end
sgs.ai_choicemade_filter.skillInvoke.thtingwu = function(self, player, promptlist)
	local target=player:getTag("ThTingwuTarget"):toPlayer()
	if target then
		if promptlist[#promptlist] == "yes" then
			sgs.updateIntention(player, target, 20)
		end	
	end
end
--【羽裳】ai
sgs.ai_cardneed.ThYuchang = function(to, card, self)
	if not self:willSkipPlayPhase(to) and getCardsNum("Slash", to, self.player) <2 then
		return card:isKindOf("Slash") and card:getSuit()==sgs.Card_Club
	end
end
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
