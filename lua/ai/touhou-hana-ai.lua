--【花祭】ai
sgs.ai_skill_cardask["@thhuajiuse"] = function(self, data)
	local target = self.player:getTag("thhuajiTarget"):toPlayer()
	if not self:isEnemy(target) then return "." end
	local keepSlash =  self:slashIsAvailable() and self:getCardsNum("Slash")<=1 
	local blacks={}
	local cards = sgs.QList2Table(self.player:getCards("he"))
	--目的是把红杀排在前面
	self:sortByKeepValue(cards,true)
	for _,c in pairs(cards) do
		if not c:isBlack()  then 
			if c:isKindOf("Slash") then
				keepSlash =false
			end
			continue 
		end
		if c:isKindOf("Slash") and keepSlash then
			keepSlash =false
			continue 
		end
		if c:isKindOf("AOE") then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useTrickCard(c, dummy_use)
			if  dummy_use.card then 
				continue
			end
		end
		table.insert(blacks,c)
	end
	if #blacks==0 then return "." end
	self:sortByKeepValue(blacks)
	return "$" .. blacks[1]:getId()
end
sgs.ai_choicemade_filter.cardResponded["@thhuajiuse"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		local target =player:getTag("thhuajiTarget"):toPlayer()
		if not target then return end	
		sgs.updateIntention(player, target, 80)
	end
end
--sgs.ai_skill_cardask["@thhuaji"] = function(self, data)
--default: discard card
-- since the enemy can make huaji opportunity, this player should not use card at first


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
sgs.ai_card_intention.ThYachuiCard = 80


--【春痕】ai
sgs.ai_skill_invoke.thchunhen = true




--【六震】ai
sgs.ai_skill_use["@@thliuzhen"] = function(self, prompt)
	local targetNames={}
	local card = self.player:getTag("thliuzhen_carduse"):toCardUse().card
	for _,p in sgs.qlist(self.enemies) do
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
