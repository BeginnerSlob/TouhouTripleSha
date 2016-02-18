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

--寸刭：若你的装备区没有武器牌，当你使用的【杀】被【闪】抵消时，你可以弃置一张牌，则此【杀】依然造成伤害。
--smart-ai.lua SmartAI:useEquipCard
sgs.ai_skill_cardask["@thcunjing"] = function(self, data, pattern, target)
	if self:isEnemy(target) then
		local ret = self:askForDiscard("", 1, 1, false, true)
		if #ret > 0 then
			if isCard("Peach", ret[1], self.player) then return "." end
			return "$" .. ret[1]:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_choicemade_filter.cardResponded["@thcunjing"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		local target = self.room:findPlayer(promptlist[#promptlist - 1])
		if target then
			sgs.updateIntention(player, target, 50)
		end
	end
end

--莲华：出牌阶段，你可弃置一张装备牌，然后观看牌堆顶的一张牌并将其交给一名角色。
thlianhua_skill = {}
thlianhua_skill.name = "thlianhua"
table.insert(sgs.ai_skills, thlianhua_skill)
thlianhua_skill.getTurnUseCard = function(self)
	local weapons = {}
	local equips = {}
	local card = nil
	if self:needToThrowArmor() then
		card = self.player:getArmor()
	else
		for _, c in sgs.qlist(self.player:getCards("he")) do
			if c:isKindOf("Weapon") then
				table.insert(weapons, c)
			elseif c:isKindOf("EquipCard") then
				table.insert(equips, c)
			end
		end
		if #weapons + #equips == 0 then return end
		if #weapons > 0 then
			self:sortByKeepValue(weapons)
			card = weapons[1]
		else
			self:sortByKeepValue(equips)
			card = equips[1]
		end
	end
	if self:getCardId("Slash") and self:slashIsAvailable() and card:getEffectiveId() == self:getWeapon():getEffectiveId() then return end
	return sgs.Card_Parse("@ThLianhuaCard=" .. card:getEffectiveId())
end

sgs.ai_skill_use_func.ThLianhuaCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_playerchosen.thlianhua = function(self, targets)
	local card_ids = sgs.QList2Table(self.player:getTag("ThLianhuaIds"):toIntList())
	local target, cardId = sgs.ai_skill_askforyiji.nosyiji(self, card_ids)
	if target and cardId and cardId > -1 then
		return target
	end
	return self.player
end

sgs.ai_playerchosen_intention.tyshouye = -20

--奇术：其他角色的回合结束后，若你的人物牌背面朝上，你可将你的人物牌翻面并获得1枚“时计”标记，然后进行一个额外的回合。
sgs.ai_skill_invoke.thqishu = function(self, data)
	if self:isWeak() then
		return true
	end
	local n_p 
	local extra_list = self.room:getTag("ExtraTurnList"):toStringList()
	if not extra_list or #extra_list == 0 then
		local p = self.room:getTag("NormalNext"):toPlayer()
		if p then
			n_p = p
		else
			n_p = self.room:findPlayer(self.room:getCurrent():getNextAlive(1, false):objectName())
		end
	else
		n_p = self.room:findPlayer(extra_list[1])
	end
	if n_p then
		if n_p:objectName() == self.player:objectName() then
			return true
		elseif n_p:hasSkill("thxianfa") and self:isFriend(n_p) then
			return true
		elseif n_p:hasSkill("thdongmo") then
			return true
		end
	end
	if self.room:findPlayerBySkillName("ikbisuo") then
		return true
	end
end

--时停：准备阶段开始时，若你没有“时计”标记，你可以将你的人物牌翻面并将你的手牌补至三张，然后跳过你此回合的判定阶段、摸牌阶段、出牌阶段和弃牌阶段。
sgs.ai_skill_invoke.thshiting = function(self, data)
	return not (self:isWeak() and self.player:getHandcardNum() >= 3)
end

--幻在：锁定技，结束阶段开始时，你须弃置全部的“时计”标记。
--无

--神脑：若你的手牌数不小于体力值，你可以将一张手牌当【三粒天滴】使用；若你的手牌数小于体力值，你可以失去1点体力并摸一张牌，视为使用了一张【三粒天滴】。
function sgs.ai_cardsview_valuable.thshennao(self, class_name, player)
	if class_name ~= "Nullification" then
		return nil
	end
	if player:getHandcardNum() >= player:getHp() then
		return nil
	end
	local null = self.ask_for_nullification
	if player:getHp() > 1 or (null.m_to:objectName() == player:objectName() and sgs.dynamic_value.damage_card[null.m_trick:getClassName()]) then
		return "@ThShennaoCard=."
	end
	return nil
end

sgs.ai_view_as.thshennao = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place == sgs.Player_PlaceHand then
		if player:getHandcardNum() > player:getHp() and not (player:getHp() == 1 and card:isKindOf("Jink")) then
			return ("nullification:thshennao[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

--妙药：每当你使用或打出一张【闪】时，若你的体力值为1，你可以回复1点体力。
sgs.ai_skill_invoke.thmiaoyao = true

sgs.ai_cardneed.thmiaoyao = function(to, card)
	return to:getHp() == 1 and card:isKindOf("Jink")
end

--黑棺：出牌阶段限一次，你可以交给一名其他角色一张黑色手牌，则你不能成为其使用的【杀】的目标；或获得一名其他角色的一张手牌，则该角色不能成为【杀】的目标。效果持续到你的下回合开始。
thheiguan_skill = {}
thheiguan_skill.name = "thheiguan"
table.insert(sgs.ai_skills, thheiguan_skill)
thheiguan_skill.getTurnUseCard = function(self)
	if #self.friends_noself + #self.enemies == 0 then return end
	if self.player:hasUsed("ThHeiguanCard") then return end
	return sgs.Card_Parse("@ThHeiguanCard=.")
end

sgs.ai_skill_use_func.ThHeiguanCard = function(card, use, self)
	self:sort(self.friends_noself, "defense")
	for _, p in ipairs(self.friends_noself) do
		if not p:isKongcheng() then
			use.card = card
			if use.to then
				use.to:append(p)
			end
			return
		end
	end
	if #self.enemies + #self.friends_noself == 0 then return end
	local cards = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if c:isBlack() then
			table.insert(cards, c)
		end
	end
	if #cards == 0 then return end
	self:sortByKeepValue(cards)
	self:sort(self.enemies, "defense")
	self.enemies = sgs.reverse(self.enemies)
	for _, p in sgs.qlist(self.enemies) do
		if p:canSlash(self.player) then
			for _, c in ipairs(cards) do
				if not isCard("Peach", c, p) and not isCard("Analeptic", c, p) and not isCard("Jink", c, p) and not isCard("ExNihilo", c, p) then
					use.card = sgs.Card_Parse("@ThHeiguanCard=" .. c:getEffectiveId())
					if use.to then
						use.to:append(p)
					end
					return
				end
			end
		end
	end
	self:sort(self.friends_noself, "defense")
	for _, p in sgs.qlist(self.friends_noself) do
		for _, c in ipairs(cards) do
			if isCard("Peach", c, p) or isCard("Analeptic", c, p) or isCard("Jink", c, p) or isCard("ExNihilo", c, p) then
				use.card = sgs.Card_Parse("@ThHeiguanCard=" .. c:getEffectiveId())
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
end

sgs.ai_card_intention.ThHeiguanCard = function(self, card, from, tos)
	if card:getSubcards():isEmpty() then
		for _, to in ipairs(tos) do
			sgs.updateIntention(from, to, -30)
		end
	end
end

--暗月：锁定技，若你没有拥有技能“赤秋”，你的红桃牌均视为黑桃牌。
--无

--宵咏：每当你的体力发生变化时，你可以进行一次判定，若不为红桃，你摸一张牌。
sgs.ai_skill_invoke.thxiaoyong = true

--栞谣：出牌阶段限一次，若你的手牌数不小于你的体力值，你可以展示全部手牌：若均为不同花色，你令一名体力值不小于你的角色失去1点体力；若均为相同花色，你获得一名其他角色的一张牌。
thkanyao_skill = {}
thkanyao_skill.name = "thkanyao"
table.insert(sgs.ai_skills, thkanyao_skill)
thkanyao_skill.getTurnUseCard = function(self)
	if self.player:getHandcardNum() < self.player:getHp() then return end
	if self.player:hasUsed("ThKanyaoCard") then return end
	local suits = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if not table.contains(suits, c:getSuitString()) then
			table.insert(suits, c:getSuitString())
		end
	end
	if #suits == 1 or #suits == self.player:getHandcardNum() then
		return sgs.Card_Parse("@ThKanyaoCard=.")
	end
end

sgs.ai_skill_use_func.ThKanyaoCard = function(card, use, self)
	local suits = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if not table.contains(suits, c:getSuitString()) then
			table.insert(suits, c:getSuitString())
		end
	end
	if #suits == 1 then
		local enemy = self:findPlayerToDiscard("he", false, false)
		if not enemy:isNude() then
			use.card = card
			return
		end
	else
		for _, p in ipairs(self.enemies) do
			if p:getHp() >= self.player:getHp() then
				use.card = card
				return
			end
		end
	end
end

sgs.ai_skill_playerchosen.thkanyao = function(self, targets)
	local suits = {}
	for _, c in sgs.qlist(self.player:getHandcards()) do
		if not table.contains(suits, c:getSuitString()) then
			table.insert(suits, c:getSuitString())
		end
	end
	if #suits == 1 then
		local enemy = self:findPlayerToDiscard("he", false, false)
		assert(enemy)
		return enemy
	else
		self:sort(self.enemies, "hp")
		for _, p in ipairs(self.enemies) do
			if p:getHp() >= self.player:getHp() then
				return p
			end
		end
	end
end

sgs.ai_playerchosen_intention.thkanyao = function(self, from, to)
	local suits = {}
	for _, c in sgs.qlist(from:getHandcards()) do
		if not table.contains(suits, c:getSuitString()) then
			table.insert(suits, c:getSuitString())
		end
	end
	if #suits ~= 1 then
		sgs.updateIntention(from, to, 50)
	end
end

sgs.ai_use_priority.ThKanyaoCard = 20
sgs.ai_choicemade_filter.cardChosen.thkanyao = sgs.ai_choicemade_filter.cardChosen.snatch