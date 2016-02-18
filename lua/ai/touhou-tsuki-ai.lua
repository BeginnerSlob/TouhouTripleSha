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
	local damage = self.player:getTag("CurrentDamageStruct"):toDamage()
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
local thjinguo_skill = {}
thjinguo_skill.name = "thjinguo"
table.insert(sgs.ai_skills, thjinguo_skill)
thjinguo_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThJinguoCard") then return nil end
	return sgs.Card_Parse("@ThJinguoCard=.")
end

sgs.ai_skill_use_func.ThJinguoCard = function(card, use, self)
	local hearts = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if c:getSuit() == sgs.Card_Heart then
			table.insert(hearts, c)
		end
	end
	if #hearts > 0 then
		self:sortByKeepValue(hearts)
		if not isCard("Peach", hearts[1], self.player) then
			local str = "@ThJinguoCard=" .. hearts[1]:getEffectiveId()
			self:sort(self.enemies, "defense")
			for _, p in ipairs(self.enemies) do
				if not p:isWounded() and not p:isNude() then
					use.card = sgs.Card_Parse(str)
					if use.to then
						use.to:append(p)
					end
					return
				end
			end
		end
	end
	use.card = card
end

sgs.ai_skill_choice.thjinguo = function(self, choices)
	if string.find(choices, "recover") then
		return "recover"
	else
		return "draw"
	end
end

sgs.ai_use_priority.ThJinguoCard = 10
sgs.ai_card_intention.ThJinguoCard = 30

--血呓：你可以将红桃非锦囊牌当【春雪幻梦】使用。
local thxueyi_skill = {}
thxueyi_skill.name = "thxueyi"
table.insert(sgs.ai_skills, thxueyi_skill)
thxueyi_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		local c = sgs.Sanguosha:getCard(id)
		cards:prepend(c)
	end
	cards = sgs.QList2Table(cards)

	local card
	self:sortByUseValue(cards, true)
	local has_weapon, has_armor = false, false

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Weapon") and not (acard:getSuit() == sgs.Card_Heart) then has_weapon = true end
	end

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Armor") and not (acard:getSuit() == sgs.Card_Heart) then has_armor = true end
	end

	for _, acard in ipairs(cards) do
		if acard:getSuit() == sgs.Card_Heart and acard:getTypeId() ~= sgs.Card_TypeTrick and ((self:getUseValue(acard) < sgs.ai_use_value.Indulgence) or inclusive) then
			local shouldUse = true

			if acard:isKindOf("Armor") then
				if not self.player:getArmor() then shouldUse = false
				elseif self.player:hasEquip(acard) and not has_armor and self:evaluateArmor() > 0 then shouldUse = false
				end
			end

			if acard:isKindOf("Weapon") then
				if not self.player:getWeapon() then shouldUse = false
				elseif self.player:hasEquip(acard) and not has_weapon then shouldUse = false
				end
			end

			if shouldUse then
				card = acard
				break
			end
		end
	end

	if not card then return nil end
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("indulgence:thxueyi[heart:%s]=%d"):format(number, card_id)
	local indulgence = sgs.Card_Parse(card_str)
	assert(indulgence)
	return indulgence
end

function sgs.ai_cardneed.thjinguo(to, card)
	return card:getSuit() == sgs.Card_Heart
end

sgs.thjinguo_suit_value = {
	heart = 3.9
}

--恋迷：觉醒技，准备阶段开始时，若你装备区里的牌的数量大于你的体力值，你须减少1点体力上限，然后获得技能“狂骨”。
--无

--狂气：出牌阶段，当你使用【杀】或【碎月绮斗】对目标角色造成伤害时，你可以选择一项：弃置一张【桃】、【酒】或装备牌；或失去1点体力。若如此做，则此伤害+1。
sgs.ai_skill_use["@@thkuangqi"] = function(self, prompt, method)
	local damage = self.player:getTag("ThKuangqiData"):toDamage()
	if not damage.to then return "." end
	if self:isEnemy(damage.to) then
		local dis = {}
		for _, c in sgs.qlist(self.player:getCards("he")) do
			if c:isKindOf("Peach") or c:isKindOf("Analeptic") or c:isKindOf("EquipCard") then
				table.insert(dis, c)
			end
		end
		local card_str = "."
		if #dis > 0 then
			self:sortByKeepValue(dis)
			if not (isCard("Peach", dis[1], self.player) and self.player:isWeak()) then
				card_str = tostring(dis[1]:getEffectiveId())
			end
		end
		if damage.card:isKindOf("Slash") then
			if not self:slashIsEffective(damage.card, damage.to, self.player) then
				return "."
			end
		else
			if not self:hasTrickEffective(damage.card, damage.to, self.player) then
				return "."
			end
		end
		if card_str == "." then
			if not (self:isWeak(damage.to) and self.player:getHp() > 2 and damage.damage < damage.to:getHp())
					and self:isWeak() and self:getCardsNum("Peach") < 2 then
				return "."
			end
		end
		return "@ThKuangqiCard=" .. card_str
	end
	return "."
end

sgs.ai_choicemade_filter.cardUsed.thkuangqi = function(self, player, carduse)
	if carduse.card:isKindOf("ThKuangqiCard") then
		local damage = player:getTag("ThKuangqiData"):toDamage()
		if damage.to then
			sgs.updateIntention(player, damage.to, 60)
		end
	end
end

--开运：在一名角色的判定牌生效前，你可以弃置一张牌，然后观看牌堆顶的两张牌，将其中一张牌代替判定牌，然后获得另一张牌。
sgs.ai_skill_cardask["@thkaiyun"] = function(self, data)
	if self.player:isNude() then return "." end
	local all_cards = self.player:getCards("he")
	local judge = data:toJudge()
	if self:needRetrial(judge) then
		local to_discard = self:askForDiscard("thkaiyun", 1, 1, false, true)
		if #to_discard > 0 then
			return "$" .. to_discard[1]
		end
	end
	return "."
end

sgs.ai_skill_askforag.thkaiyun = function(self, card_ids)
	local judge = self.player:getTag("ThKaiyunJudge"):toJudge()
	local kaiyun = {}
	local kaiyun1 = {}
	local kaiyun2 = {}
	
	local judge_card = sgs.Sanguosha:getCard(card_ids[1])
	table.insert(kaiyun1, judge_card)
	table.insert(kaiyun, judge_card)
	local id1 = self:getRetrialCardId(kaiyun1, judge)
	
	judge_card = sgs.Sanguosha:getCard(card_ids[2])
	table.insert(kaiyun2, judge_card)
	table.insert(kaiyun, judge_card)
	local id2 = self:getRetrialCardId(kaiyun2, judge)

	if id1 == id2 or (id1 ~= -1 and id2 ~= -1)then
		self:sortByKeepValue(kaiyun)
		return kaiyun[1]:getId()
	elseif id1 == -1 then
		return card_ids[2]
	elseif id2 == -1 then
		return card_ids[1]
	end
	return card_ids[math.random(#card_ids)]
end

--狡兔：每当你受到1点伤害后，可以令伤害来源进行一次判定，若结果不为红桃，该角色获得技能“无谋”直到该角色的下一个回合的回合结束。
sgs.ai_skill_invoke.thjiaotu =function(self, data)
	local target = data:toPlayer()
	return self:isEnemy(target)
end

sgs.ai_choicemade_filter.skillInvoke.thjiaotu = function(self, player, promptlist)
	local target = self.room:findPlayer(promptlist[#promptlist - 1])
	if target and promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, target, 60)
	end
end

--授业：专属技，摸牌阶段摸牌时，你可以少摸一张牌并指定一名其他角色，若如此做，此回合结束时，该角色须进行一个额外的摸牌阶段。
sgs.ai_skill_playerchosen.thshouye = function(self, targets)
	if self:isWeak(self.player) and not self:willSkipPlayPhase() then return nil end
	local target = self:findPlayerToDraw(false, 2)
	return target
end

sgs.ai_playerchosen_intention.tyshouye = -60

--虚史:你的回合外，当你需要使用或打出一张【杀】或【闪】时，你可从牌堆顶亮出一张牌，若此牌为基本牌，你获得这张牌，否则将其置入弃牌堆。
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

--凤翔：每当有一名角色的人物牌被横置或重置时，你可以摸一张牌，每回合限四次。
sgs.ai_skill_invoke.thfengxiang = true

--快晴：锁定技，你使用的非延时类锦囊牌对其他未受伤的角色的效果不可以被【三粒天滴】抵消。
--无

--浴火：限定技，当你受到一次伤害时，你可以转移此伤害给造成伤害的来源，然后该角色将其人物牌翻面。
sgs.ai_skill_invoke.thyuhuo = function(self, data)
	local damage = player:getTag("ThYuhuoDamage"):toDamage()
	local target = damage.from
	damage.to = target
	if self:isEnemy(target) then
		local score = 0
		if self:damageIsEffective_(damage) then
			score = score + 1
		end
		if damage.damage > 2 then
			score = score + 1
		end
		if target:faceUp() then
			score = score + 1
		end
		if self.player:getHp() - damage.damage <= 0 then
			score = score + 2
		end
		return score > 1
	end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.thyuhuo = function(self, player, promptlist)
	local from = self.room:findPlayer(promptlist[#promptlist - 1])
	if from and promptlist[#promptlist] == "yes" and from:faceUp() then
		sgs.updateIntention(player, from, 60)
	end
end

