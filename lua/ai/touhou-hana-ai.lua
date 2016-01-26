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

--【彼岸】ai
sgs.ai_skill_cardask["@thbian"] = function(self, data)
	local dying = self.player:getRoom():getCurrentDyingPlayer()
	if self:isEnemy(dying) then
		local tricks={}
		for _,c in sgs.qlist(self.player:getHandcards()) do
			if c:isKindOf("TrickCard")  then
				table.insert(tricks,c)
			end
		end
		if #tricks==0 then return "." end
		self:sortByKeepValue(tricks)
		return "$" .. tricks[1]:getId()
	end
	return "."
end
--【归航】ai
sgs.ai_skill_invoke.thguihang = function(self,data)
	local dying = self.player:getRoom():getCurrentDyingPlayer()
	if dying:isKongcheng() then return false end
	if self:isEnemy(dying) then return false end
	local hasred = false
	if  dying==self.player then
		for _,c in sgs.qlist(self.player:getHandcards()) do
			if c:isRed() then
				hasred = true
				break
			end
		end
	end
	if dying~=self.player or hasred then return true end
	return false
end
sgs.ai_cardshow.thguihang = function(self, requestor)
	local reds={}
	for _,c in sgs.qlist(self.player:getHandcards()) do
		if c:isRed() then
			table.insert(reds,c)
		end
	end
	if #reds==0 then return "." end
	self:sortByKeepValue(reds)
	return reds[1]
end
--【无间】ai
local thwujian_skill = {}
thwujian_skill.name = "thwujian"
table.insert(sgs.ai_skills, thwujian_skill)
thwujian_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThWujianCard") then return nil end
	if self.player:isKongcheng() then return nil end
	return sgs.Card_Parse("@ThWujianCard=.")
end
sgs.ai_skill_use_func.ThWujianCard = function(card, use, self)
	local targets = sgs.SPlayerList()
	local friends = sgs.SPlayerList()
	friends:append(self.player)
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player))do
		if self:isFriend(p) then
			friends:append(p)
        elseif self.player:inMyAttackRange(p) then
            targets:append(p)
        end
    end
	if targets:length()==0 then return end
	local target = 0
	local max_danger = 0
	for _,p in sgs.qlist(targets)do
		local current_danger = 0
		for _,f in sgs.qlist(friends)do
			if p:distanceTo(f)== p:getAttackRange() then
				if (f:getHp()<=2 or f:getHandcardNum()>=1) then current_danger=current_danger+2 end
				current_danger=current_danger+1
			end
        end
        if current_danger > max_danger then
			max_danger = current_danger
            target = p
        end
    end
	if target == 0 then return end
	local c = 0 ,cards
	if self.player:getHandcardNum()> self.player:getMaxHp() then
		cards = self.player:getHandcards()
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		c = cards[1]
	elseif max_danger>=3 then
		cards = self.player:getCards("he")
		cards = sgs.QList2Table(cards)
		self:sortByKeepValue(cards)
		c = cards[1]
	end
	if c == 0 then return end
	use.card = sgs.Card_Parse("@ThWujianCard=" .. c:getId())
	if use.to then
		use.to:append(target)
	end
	return
end
sgs.ai_card_intention.ThWujianCard = 30

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
	local boom = false
	local down, up
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player))do
		if p:getMark("ThTingwuMedium") > 0 then
			down = self.room:findPlayer(p:getNextAlive():objectName())
			up = self.room:findPlayer(p:getLastAlive():objectName())
			break
		end
	end
	if down:isChained() or up:isChained() then
		local cfriend,cenemy
		for _, p in sgs.qlist(self.room:getAllPlayers()) do
			if not p:isChained() then continue end
			if self:isEnemy(p) and not (p:hasSkill("ikmuguang") and not p:isKongcheng()) then 
				cenemy=cenemy+1 
			elseif self:isFriend(p) then
				if p:hasSkill("ikmuguang") and not p:isKongcheng()then
					cfriend=cfriend+2
				elseif p:hasSkill("thmingling") then
					cfriend=cfriend-2
				else
					cfriend=cfriend+1
				end
			end
		end
		if cfriend < cenemy then boom = true end
	end
	local target
	if boom then
		if self:isFriend(up) and self:isFriend(down) then
			if up:isChained() or (up:hasSkill("ikmuguang") and not up:isKongcheng() and up:isWounded()) then
				target = up
			elseif down:isChained() or(down:hasSkill("ikmuguang") and not down:isKongcheng() and down:isWounded()) then
				target = down 
			end
		elseif self:isEnemy(up) and self:isEnemy(down) then 
			if up:hasSkill("ikmuguang") and not up:isKongcheng() then
				target = down
			elseif down:hasSkill("ikmuguang") and not down:isKongcheng() then
				target = up
			else
				if up:getHp()>down:getHp() then
					if up:isChained() then
						target = up
					else
						target = down
					end
				else
					if down:isChained() then
						target = down
					else
						target = up
					end
				end
			end
		elseif self:isEnemy(up) then
			if up:hasSkill("ikmuguang") and not up:isKongcheng() then 
				if down:isChained() then
					target = down
				else 
					return false
				end
			else
				target = up
			end
		elseif self:isEnemy(down) then
			if down:hasSkill("ikmuguang") and not down:isKongcheng() then 
				if up:isChained() then
					target = up
				else 
					return false
				end
			else
				target = down
			end
		end
	else
		if self:isFriend(up) and self:isFriend(down) then
			if up:hasSkill("ikmuguang") and not up:isKongcheng() and up:isWounded() then
				target = up
			elseif down:hasSkill("ikmuguang") and not down:isKongcheng() and down:isWounded() then
				target = down
			end
		end
		if up:isChained() and down:isChained() then return false end
		if self:isEnemy(up) and self:isEnemy(down) then 
			if up:hasSkill("ikmuguang") and not up:isKongcheng() then
				target = down
			elseif down:hasSkill("ikmuguang") and not down:isKongcheng() then
				target = up
			else
				if up:getHp()>down:getHp() then
					if down:isChained() then
						target = up
					else
						target = down
					end
				else
					if up:isChained() then
						target = down
					else
						target = up
					end
				end
			end
		elseif self:isEnemy(up) then
			if up:hasSkill("ikmuguang") and not up:isKongcheng() then return false end
			target = up
		elseif self:isEnemy(down) then
			if down:hasSkill("ikmuguang") and not down:isKongcheng() then return false end
			target = down
		end
		if target:isChained() and self:isEnemy(target) then return false end
	end
	self.room:setPlayerMark(target, "thtingwu_target", 1)
	return true
end
sgs.ai_skill_playerchosen.thtingwu = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	for _, target in ipairs(targetlist) do
		if target:getMark("thtingwu_target") > 0 then 
			self.room:setPlayerMark(target, "thtingwu_target", 0)
			return target
		end
	end
	return nil
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
                --    table.insert(friends,p)
                --end
                if     self:isEnemy(p) and not self:getDamagedEffects(p, self.player, false) then
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
