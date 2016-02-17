function SmartAI:findPlayerToChain(targets)
	self:sort(self.friends, "defense")
	for _, friend in ipairs(self.friends) do
		if not table.contains(targets, friend) then continue end
		if friend:isChained() and not self:isGoodChainPartner(friend) then
			return friend
		elseif not friend:isChained() and friend:hasSkill("thchiwu") and self:isGoodChainPartner(friend) then
			return friend
		end
	end
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if not table.contains(targets, enemy) then continue end
		if not enemy:isChained() and enemy:hasSkill("thchiwu") and self:isGoodChainPartner(enemy) then
			continue
		end
		if not enemy:isChained() and self:objectiveLevel(enemy) > 3
			and not self:getDamagedEffects(enemy) and not self:needToLoseHp(enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then
			return enemy
		end
	end
	return nil
end

--锁命：每当你受到1点伤害后或一名角色的红色基本牌于你的回合内置入弃牌堆时，你可以指定一名角色，横置或重置其人物牌。
sgs.ai_skill_playerchosen.thsuoming = function(self, targets)
	local could_choose = {}
	local damage = self.player:getTag("ThSuomingData"):toDamage()
	if damage.to then
		local all_players = self.room:getAllPlayers()
		local index = 0
		if damage.nature ~= sgs.DamageStruct_Normal then
			if damage.chain then
				index = all_players:indexOf(damage.to)
			end
		end
		
		if (self.room:getTag("is_chained"):toInt() > 0 and damage.nature ~= sgs.DamageStruct_Normal) or damage.chain then
			for i = index, all_players:length() - 1 do
				if all_players:at(i):objectName() == damage.to:objectName() then
					continue
				end
				table.insert(could_choose, all_players:at(i))
			end
		end
	end
	if #could_choose > 0 then
		local to_chain = self:findPlayerToChain(could_choose)
		if to_chain then
			return to_chain
		end
	end
	return self:findPlayerToChain(sgs.QList2Table(targets))
end

--赤雾：锁定技，当你的人物牌横置时，普通【杀】和【碎月绮斗】对你无效。
--maneuvering-ai.lua SmartAI:useCardIronChain
--smart-ai.lua SmartAI:hasTrickEffective
--standard_cards-ai.lua SmartAI:slashIsEffective

--夜君：君主技，其他月势力角色的出牌阶段限一次，若场上所有角色的人物牌都竖置，该角色可以将你的人物牌横置。
local thyejunv_skill = {}
thyejunv_skill.name = "thyejunv"
table.insert(sgs.ai_skills, thyejunv_skill)
thyejunv_skill.getTurnUseCard = function(self)
	if self.player:getKingdom() ~= "tsuki" or self.player:hasFlag("ForbidThYejun") then return nil end
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:isChained() then
			return false
		end
	end
	return sgs.Card_Parse("@ThYejunCard=.")
end

sgs.ai_skill_use_func.ThYejunCard = function(card, use, self)
	local lords = sgs.SPlayerList()
	for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if p:hasLordSkill("thyejun") and not p:hasFlag("ThYejunInvoked") then
			lords:append(p)
		end
	end
	if lords:isEmpty() then return end
	lords = sgs.QList2Table(lords)
	self:sort(lords, "defense")
	for _, p in ipairs(lords) do
		if self:isFriend(p) and self:isGoodChainPartner(p) then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		elseif self:isEnemy(p) and not self:isGoodChainPartner(p) then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
end

--禁果：出牌阶段限一次，你可以选择一项：1. 弃置一张红桃手牌并获得一名其他角色的两张牌（不足则全部获得），然后该角色选择回复1点体力；或摸一张牌；2. 获得技能“血呓”直到回合结束。

--【开运】ai
sgs.ai_skill_cardask["@thkaiyun"] = function(self, data)
	if self.player:isNude() then return "." end
	local judge = data:toJudge()
	if self:needRetrial(judge) then
		local to_discard=self:askForDiscard("thkaiyun", 1, 1, false, true)
		if #to_discard>0 then
			return "$" .. to_discard[1]
		end
	end
	return "."
end
sgs.ai_skill_askforag.thkaiyun = function(self, card_ids)
	--技能代码需要有对应tag
	local judge = self.player:getTag("ThKaiyunJudge"):toJudge()
	local kaiyun={}
	local kaiyun1={}
	local kaiyun2={}
	
	judge.card = sgs.Sanguosha:getCard(card_ids[1])
	table.insert(kaiyun1,judge.card)
	table.insert(kaiyun,judge.card)
	local id1=self:getRetrialCardId(kaiyun1, judge)
	
	judge.card = sgs.Sanguosha:getCard(card_ids[2])
	table.insert(kaiyun2,judge.card)
	table.insert(kaiyun,judge.card)
	local id2=self:getRetrialCardId(kaiyun2, judge)
	
	--id==-1 说明预设的判定不符合利益
	if id1==id2 or (id1~=-1 and  id2~=-1) then
		--此时拿哪一张改判都一样
		self:sortByKeepValue(kaiyun)
		return kaiyun[1]:getId()
	elseif id1==-1 then
		return card_ids[1]
	elseif id2==-1 then
		return card_ids[2]
	end
	return card_ids[1]
end

--【狡兔】ai
sgs.ai_skill_invoke.thjiaotu =function(self,data)
	local target = data:toPlayer()
	return  self:isEnemy(target) 
end
sgs.ai_choicemade_filter.skillInvoke.thjiaotu = function(self, player, promptlist)
	--技能代码需要一个tag
	local target = player:getTag("ThJiaotuTarget"):toPlayer()
	if target and promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, target, 60)
	end
end


--【授业】ai
sgs.ai_skill_playerchosen.thshouye = function(self, targets)
	if self:isWeak(self.player) and not self:willSkipPlayPhase() then return nil end
	local target =self:findPlayerToDraw(false, 2)--it could be 3 (for instance:yingzi)
	if target then return target end
	return nil
end
sgs.ai_playerchosen_intention.tyshouye = -60
--【虚史】ai
sgs.ai_skill_invoke.thxushi = true
function sgs.ai_cardsview_valuable.thxushi(self, class_name, player)
	if class_name == "Slash" then
		if player:getPhase() ~= sgs.Player_NotActive or player:hasFlag("Global_ThXushiFailed") then
			return nil 
		end
		if sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE then
			return "@ThXushiCard=."
		end
	end
end

--【凤翔】ai
sgs.ai_skill_invoke.thfengxiang = true
--【浴火】ai
sgs.ai_skill_invoke.thyuhuo = function(self,data)
	local damage = data:toDamage()
	local target=damage.from
	if self:isEnemy(target) then
		local score=0
		--masochism
		if not (damage.damage<=1 and self:getDamagedEffects(target, self.player)) then
			score=score+1
		end
		if (damage.damage>2) then
			score=score+1
		end
		if target:faceUp() then
			score=score+1
		end
		if self.player:getHp()-damage.damage<=0 then
			score=score+2
		end
		return score>=2
	end
	return false
end
sgs.ai_choicemade_filter.skillInvoke.thyuhuo = function(self, player, promptlist)
	local from=player:getTag("ThYuhuoDamage"):toDamage().from
	if from and promptlist[#promptlist] == "yes" and from:getLostHp()>0 then
		sgs.updateIntention(player, from, 60)
	end
end
sgs.ai_need_damaged.thyuhuo = function(self, attacker, player)
	if player:getMark("@yuhuo") == 0  then return false end
	if not attacker or attacker:hasSkill("thwunian") then return false end
	if  self:isEnemy(attacker,player) then
		if not self:getDamagedEffects(attacker,player) and attacker:faceUp() then
			return true
		end
	end
	return false
end
sgs.ai_slash_prohibit.thyuhuo = function(self, from, to, card)
	local callback=sgs.ai_need_damaged["thyuhuo"]
	if callback then
		return  callback(self, from, to)
	end
	return false
end
--need bulid trick_prohibit or damage_prohibit in smart-ai and standardcards-ai
