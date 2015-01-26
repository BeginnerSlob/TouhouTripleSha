--【开运】ai
sgs.ai_skill_cardask["@thkaiyun"] = function(self, data)
	if self.player:isNude() then return "." end
	local judge = data:toJudge()
	if self:needRetrial(judge) then
		local to_discard=self:askForDiscard("thkaiyun", 1, 1, false, true)
		if #to_discard>0 then
			return "$" .. to_discard[1]:getId()
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
	local target =self:touhouFindPlayerToDraw(false, 2)--it could be 3 (for instance:yingzi)
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
		return "@ThXushiCard=."
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
