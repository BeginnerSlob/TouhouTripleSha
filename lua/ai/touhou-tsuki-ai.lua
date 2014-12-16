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

