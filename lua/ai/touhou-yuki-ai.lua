--【萃梦】ai
sgs.ai_skill_invoke.thcuimeng = function(self, data)
 	--等待同步替换二张， 张宝 等的技能
	--[[if self:willSkipPlayPhase() then
		local erzhang = self.room:findPlayerBySkillName("guzheng")
		if erzhang and self:isEnemy(erzhang) then return false end
		if self.player:getPile("incantation"):length() > 0 then
			local card = sgs.Sanguosha:getCard(self.player:getPile("incantation"):first())
			if not self.player:getJudgingArea():isEmpty() and not self.player:containsTrick("YanxiaoCard") and not self:hasWizard(self.enemies, true) then
				local trick = self.player:getJudgingArea():last()
				if trick:isKindOf("Indulgence") then
					if card:getSuit() == sgs.Card_Heart or (self.player:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade) then return false end
				elseif trick:isKindOf("SupplyShortage") then
					if card:getSuit() == sgs.Card_Club then return false end
				end
			end
			local zhangbao = self.room:findPlayerBySkillName("yingbing")
			if zhangbao and self:isEnemy(zhangbao) and not zhangbao:hasSkill("manjuan")
				and (card:isRed() or (self.player:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade)) then return false end
		end
 	end
	]]
 	return true
end

--【遁甲】ai
sgs.ai_skill_invoke.thdunjia = true
sgs.ai_skill_choice.thdunjia = function(self, choices, data)	
	local target = self.player:getTag("ThDunjiaTarget"):toPlayer()
	if not target or not self:isEnemy(target) then return "draw" end
	if choices:match("discard") then
		local x = math.abs(self.player:getEquips():length()- target:getEquips():length())
		if target:getCards("he"):length() >= x then
			return "discard"
		end
		-- 目前无脑拆
		--其实判断最好加入对于敌人装备值得衡量和自身需要过牌的考虑，而不是无脑发动 = =
		--高阶ai aoe时需要队友会卖血
	end
	return "draw"
end
--sgs.ai_skill_cardchosen.thdunjia = function(self, who, flags)
--交给smart-ai的askForCardChosen去选择,应该没有特别要注意的


sgs.ai_skill_invoke.thlingya = true
sgs.ai_skill_choice.thlingya = function(self, choices, data)	
	local yukari = self.player:getTag("ThLingyaSource"):toPlayer()
	if yukari and choices:match("discard") then 
		if self:isFriend(yukari) and  self:hasSkills(sgs.lose_equip_skill) then
			return "discard"
		elseif self:isEnemy(yukari) then
			local LetDiscard = false
			--高级ai 应该对letdiscard做更详细的评估
			if not yukari:canDiscard(self.player,"h") and self:hasSkills(sgs.lose_equip_skill) then
				LetDiscard  = true
			elseif  self:needKongcheng(p) and self:getHandcardNum()==1  then 
				if not yukari:canDiscard(self.player,"e") or self:hasSkills(sgs.lose_equip_skill) then
					LetDiscard  = true
				end
			end
			if LetDiscard then  return "discard" end
		end
	end
	return "letdraw"
end


--黑幕的存在使得carduse本身就有变化。。。。比如可以故意作死地去决斗敌人。。。反正会转移使用者当一个离间。。。
-- 一般ai使用决斗不会这么做,这个功能需要改usecard的底层ai本身 = =
sgs.ai_skill_playerchosen.thheimu = function(self, targets)
    local cardUse = self.player:getTag("ThHeimuCardUse"):toCardUse()
    local isRed = cardUse.card:isRed()
    
    --case1  灵压敌人
    local goodLingyaCard = "god_salvation|amazing_grace|iron_chain" 
    --|slash|thunder_slash|fire_slash
    local isGoodLingyaCard =  goodLingyaCard:match(cardUse.card:objectName())
    if self.player:hasSkill("thlingya") and isGoodLingyaCard and isRed then
        if #self.enemies > 0 then
            self:sort(self.enemies, "defense")
            return self.enemies[1]
        end
    end
    
    --case2 助队友收反或使主公杀忠掉牌
    local isDamageCard =  sgs.dynamic_value.damage_card[cardUse.card:getClassName()]
    if isDamageCard then
        local lord = self.room:getLord()
        if self:isFriend(lord) then
            local weakRebel 
            for _,p in sgs.qlist(cardUse.to) do
                if p:getHp()<=1 and self:isEnemy(p) then
                    weakRebel  = p
                    continue
                end
            end
            if weakRebel  then
                for _, p in sgs.qlist(targets) do
                    if self:isFriend(p) and p:hasSkills(sgs.cardneed_skill) then
                        if (cardUse.card:isKindOf("TrickCard") and self:hasTrickEffective(cardUse.card, weakRebel, p)) 
                        or (cardUse.card:isKindOf("Slash") and self:slashIsEffective(cardUse.card, weakRebel, p)) then
                            return p
                        end
                    end
                end
            end
        else
            local weakLoyalist
            for _,p in sgs.qlist(cardUse.to) do
                if p:getHp()<=1 and self:isEnemy(p) then
                    weakLoyalist  = p
                    continue
                end
            end
            if weakLoyalist then
                for _, p in sgs.qlist(targets) do
                    if p:isLord(p) then
                        if (cardUse.card:isKindOf("TrickCard") and self:hasTrickEffective(cardUse.card, weakLoyalist, p)) 
                        or (cardUse.card:isKindOf("Slash") and self:slashIsEffective(cardUse.card, weakLoyalist, p)) then
                            return p
                        end
                    end
                end
            end
        end
    end
    --case3  一般灵压 针对队友
    if isRed() and self.player:hasSkill("thlingya")  then
        for _, p in sgs.qlist(targets) do
            if self:isFriend(p) then
                return p
            end
        end
    end
    return nil
end



--【冬末】ai
sgs.ai_skill_use["@@thdongmo"] = function(self, prompt)
	local targetNames={}
	--need a sort method... 
	for _,p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if (self:isFriend(p) and not p:faceUp()) 
		or (self:isEnemy(p) and  p:faceUp()) then
			table.insert(targetNames,p:objectName())
		end
		if #targetNames>=self.player:getLostHp() then
			break
		end
	end
	if #targetNames ==0 then 
		return "."	 
	elseif #targetNames < self.player:getLostHp() then
		local can=false
		if not self.player:faceUp() then
			can =true
		elseif self.player:getLostHp() -#targetNames<2 then
			can =true
		end
		if can then
			return "@ThDongmoCard=.->" .. table.concat(targetNames, "+")
		end
	else
		return "@ThDongmoCard=.->" .. table.concat(targetNames, "+")
	end
	return "."
end
sgs.ai_card_intention.ThDongmoCard = function(self, card, from, tos)
	for _,to in pairs (tos) do
		if to:faceUp() then
			sgs.updateIntention(from, to, 50)
		else
			sgs.updateIntention(from, to, -50)	
		end
	end
end

--【凛寒】ai
sgs.ai_skill_invoke.thlinhan = true


--【骚葬】ai
--要确保弃牌发动技能，ai都是不用牌不舒服斯基的主
--这个属于修改基础用牌ai 暂时不弄了。。。
sgs.ai_skill_playerchosen.thsaozang = function(self, targets)
	--self:sort(self.enemies, "defense")
	if #self.enemies>0 then
		self:sort(self.enemies, "handcard")
		for _,p in pairs (self.enemies) do
			if not self.player:canDiscard(p, "h") then
				continue
			elseif self:needKongcheng(p) and p:getHandcardNum()==1 then
				continue
			else
				return p
			end
		end
	end
	return nil
end
--【骚葬】的仇恨：比起playerchosen，可能cardchosen更精确
sgs.ai_choicemade_filter.cardChosen.thsaozang = sgs.ai_choicemade_filter.cardChosen.dismantlement

--【絮曲】ai
sgs.ai_skill_playerchosen.thxuqu = function(self, targets)
	local target = self:findPlayerToDraw(false, 1)
	if target then return target end
	return nil
end
sgs.ai_playerchosen_intention.thxuqu = -30

--【苦戒】ai
local thkujiev_skill = {}
thkujiev_skill.name = "thkujiev"
table.insert(sgs.ai_skills, thkujiev_skill)
thkujiev_skill.getTurnUseCard = function(self)
	if self.player:hasFlag("ForbidThKujie") then return nil end
	local reds={}
	for _,c in sgs.qlist(self.player:getCards("he")) do
		if c:isRed() and c:isKindOf("BasicCard") then
			table.insert(reds,c)
		end
	end
	if #reds==0 then return nil end
	self:sortByKeepValue(reds)
	return sgs.Card_Parse("@ThKujieCard=" .. reds[1]:getEffectiveId())
end
sgs.ai_skill_use_func.ThKujieCard = function(card, use, self)
	local targets ={}
	for _,p in sgs.qlist(self.room:findPlayersBySkillName("thkujie")) do
		if  self.player:inMyAttackRange(p) and not p:hasFlag("ThKujieInvoked") then
			table.insert(targets,p)
		end
	end
	if #targets==0 then return nil end
	self:sort(targets, "hp")
	local good_target 
	for _,p in pairs (targets) do
		if self:isEnemy(p) then
			if p:getHp()==1 and self:getAllPeachNum(p)<=0 then
				good_target = p
				break
			end
		elseif self:isFriend(p) then
			if p:isWounded() and p:getHp()>1 then
				good_target = p
				break
			end
		end
	end
	if good_target then
		use.card = card
		if use.to then
			use.to:append(good_target)
			return
		end
	end
end
sgs.ai_card_intention.ThKujieCard = function(self, card, from, tos)
	for _, to in pairs(tos) do
		if to:getHp()<=1 then
			sgs.updateIntention(from, to, 80)
		else
			sgs.updateIntention(from, to, -20)
		end
	end
end

--【廕庇】ai
sgs.ai_skill_invoke.thyinbi = function(self, data)
	local damage = data:toDamage()
	if self:isFriend(damage.to) then
		if damage.to:getLostHp() >= damage.damage and  self.player:getHp() > damage.damage then
			local isSlash 
			if damage.card and damage.card:isKindOf("Slash") then
				isSlash= true
			end
			return not self:needToLoseHp(damage.to, damage.from, isSlash, true)
		end
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.thyinbi = function(self, player, promptlist)
	local to=player:getTag("thyinbiDamage"):toDamage().to
	if to and promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, to, -80)
	end
end




--【灵蝶】ai
local function countKnownCards(target)
		local count=0
		for _, card in sgs.qlist(target:getHandcards()) do
			--flag的情况其实可以不要。。。
			local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), target:objectName())
			if  card:hasFlag("visible") or card:hasFlag(flag) then	
				count=count+1
			end
		end
		return count
	end
local lingdieCompare_func = function(a, b)
	return	countKnownCards(a)>countKnownCards(b)
end
	
local thlingdie_skill = {}
thlingdie_skill.name = "thlingdie"
table.insert(sgs.ai_skills, thlingdie_skill)
thlingdie_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThLingdieCard") then return nil end
	if #self.friends_noself ==0 then return nil end
	local cards =sgs.QList2Table(self.player:getCards("he"))
	if #cards==0 then return nil end
	
	--没补牌技能时，防御太虚
	if not self.player:hasSkill("thwushou") and #cards <3 then
	--sgs.getDefense(self.player, gameProcess)
		return nil 
	end

	self:sortByKeepValue(cards)
	return sgs.Card_Parse("@ThLingdieCard=" .. cards[1]:getEffectiveId())
	--目前敌友都可以属于看牌目标，也就不预先检测目标了
end
sgs.ai_skill_use_func.ThLingdieCard = function(card, use, self)

	local good_enemy
	if #self.enemies>0 then
		table.sort(self.enemies, lingdieCompare_func)
		good_enemy=self.enemies[1]
	end
	local good_friend
	if good_enemy then
		for _,p in pairs (self.friends_noself) do
			--考虑急火？ canslash？
			if p:inMyAttackRange(good_enemy)  then
				good_friend =p
				break
			end
		
		end
	end
	use.card = card
	if use.to then
		if good_friend then
			use.to:append(good_friend)
		else
			use.to:append(self.friends_noself[math.random(1,#self.friends_noself)])
		end
		return
	end
end
sgs.ai_card_intention.ThLingdieCard = -50
sgs.ai_skill_playerchosen.thlingdie = function(self, targets)
	if #self.enemies>0 then
		table.sort(self.enemies, lingdieCompare_func)
		good_enemy=self.enemies[1]
	end
	if good_enemy and not good_enemy:isKongcheng() then
		return good_enemy
	end
	return targets:first()
end
sgs.ai_playerchosen_intention.thlingdie =function(self, from, to)
	if not self:isFriend(from,to) then
		sgs.updateIntention(from, to, 20)
	end
end
--灵蝶优先度应该很低。。。

--【无寿】ai
sgs.ai_need_damaged.thwushou = function(self, attacker, player)
	--卖血条件：体力值大于1，且能补3张以上
	if attacker and attacker:hasSkill("ikxuwu") then return false end
	local num = 4 - player:getHp()
	return num >= 2
end

--【浮月】ai
local thfuyuev_skill = {}
thfuyuev_skill.name = "thfuyuev"
table.insert(sgs.ai_skills, thfuyuev_skill)
thfuyuev_skill.getTurnUseCard = function(self)
	if self.player:getKingdom()~="yuki" then return nil end
	if self.player:isKongcheng() or self:needBear()  or self.player:hasFlag("ForbidThFuyue") then return nil end
	return sgs.Card_Parse("@ThFuyueCard=.")
end
sgs.ai_skill_use_func.ThFuyueCard = function(card, use, self)
	local lord
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if player:hasLordSkill("thfuyue") and not player:isKongcheng() 
		and not player:hasFlag("ThFuyueInvoked") and player:isWounded()
		then
			lord=player
			break
		end
	end
	if not lord then return nil end
	
	--暂时不考虑反贼获利
	if self:isEnemy(lord)  then
		return nil
	end
	
	
	
	
	local cards = self.player:getHandcards()
	local max_num = 0, max_card
	local min_num = 14, min_card
		for _, hcard in sgs.qlist(cards) do
			if hcard:isKindOf("Peach") then continue end
			if hcard:getNumber() > max_num then
				max_num = hcard:getNumber()
				max_card = hcard
			end

			if hcard:getNumber() <= min_num then
				if hcard:getNumber() == min_num then
					if min_card and self:getKeepValue(hcard) > self:getKeepValue(min_card) then
						min_num = hcard:getNumber()
						min_card = hcard
					end
				else
					min_num = hcard:getNumber()
					min_card = hcard
				end
			end
		end
	if not min_card then return nil end
	
	--很大概率主公赢不了
	if min_card:getNumber()>=12 then return nil end
	if min_card:getNumber()>9 and lord:getHandcardNum()<=4 then
		local lord_card = self:getMaxCard(lord)
		if not lord_card or lord_card:getNumber() < min_card:getNumber() then
			return nil
		end
	end
	if self:isFriend(lord) then
		self.thfuyue_card = min_card:getEffectiveId()
		use.card = card
		if use.to then use.to:append(lord) end
		return
	end
end
--响应拼点者的应对
function sgs.ai_skill_pindian.thfuyue(minusecard, self, requestor, maxcard)
	return self:getMaxCard()
end

sgs.ai_choicemade_filter.pindian.thfuyue = function(self, from, promptlist)
	local number = sgs.Sanguosha:getCard(tonumber(promptlist[4])):getNumber()
	local lord = findPlayerByObjectName(self.room, promptlist[5])
	if not lord then return end
	
	if number < 6 then sgs.updateIntention(from, lord, -60)
	elseif number > 8 and lord:getHandcardNum()<=4 and self:isEnemy(lord,from) then 
	--反贼拼点？
		sgs.updateIntention(from, lord, 60) 
	end
end
