
--【冬末】ai
sgs.ai_skill_use["@@thdongmo"] = function(self, prompt)
	local score=0
	local threshold=
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

--【灵蝶】ai
sgs.ai_need_damaged.thwushou = function(self, attacker, player)
	--卖血条件：体力值大于1，且能补3张以上
	if attacker and attacker:hasSkill("jueqing") then return false end
	local num = 4- player:getHp()
	return num>=3  
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
