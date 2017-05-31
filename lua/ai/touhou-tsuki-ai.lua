function SmartAI:findPlayerToChain(targets, chain_only)
	if targets:isEmpty() then return nil end
	targets = sgs.QList2Table(targets)
	local getChainTarget = function(players_table)
		self:sort(self.friends, "defense")
		for _, friend in ipairs(self.friends) do
			if not table.contains(players_table, friend) then continue end
			if friend:isChained() and not self:isGoodChainPartner(friend) and not self:isGoodThQiongfaTarget(friend) and not chain_only then
				return friend
			elseif not friend:isChained() and friend:hasSkill("thchiwu") and self:isGoodChainPartner(friend) then
				return friend
			elseif not friend:isChained() and self:isGoodThQiongfaTarget(friend) then
				return friend
			end
		end
		self:sort(self.enemies, "defense")
		for _, enemy in ipairs(self.enemies) do
			if not table.contains(players_table, enemy) then continue end
			if not enemy:isChained() and enemy:hasSkill("thchiwu") and self:isGoodChainPartner(enemy) then
				continue
			end
			if not enemy:isChained() and self:isGoodThQiongfaTarget(enemy) then
				continue
			end
			if not enemy:isChained() and self:objectiveLevel(enemy) > 3
				and not self:getDamagedEffects(enemy) and not self:needToLoseHp(enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then
				return enemy
			end
		end
		return nil
	end

	local could_choose = {}
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if damage.to then
		local all_players = self.room:getAllPlayers()
		local index = 0
		if damage.nature ~= sgs.DamageStruct_Normal then
			if damage.chain then
				index = all_players:indexOf(damage.to)
			end
		end
		
		if damage.trigger_chain or damage.chain then
			for i = index, all_players:length() - 1 do
				if all_players:at(i):objectName() == damage.to:objectName() then
					continue
				end
				if not table.contains(targets, all_players:at(i)) then
					continue
				end
				table.insert(could_choose, all_players:at(i))
			end
		end
	end
	if #could_choose > 0 then
		local to_chain = getChainTarget(could_choose)
		if to_chain then
			return to_chain
		end
	end
	return getChainTarget(targets)
end

--锁命：每当你受到1点伤害后或一名角色的红色基本牌于你的回合内置入弃牌堆时，你可以指定一名角色，横置或重置其人物牌。
sgs.ai_skill_playerchosen.thsuoming = function(self, targets)
	return self:findPlayerToChain(targets)
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
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		local c = sgs.Sanguosha:getCard(id)
		cards:prepend(c)
	end
	cards = sgs.QList2Table(cards)

	local card
	self:sortByUseValue(cards, true)
	local has_weapon, has_armor = false, false

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Weapon") and acard:getSuit() ~= sgs.Card_Heart then has_weapon = true end
	end

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Armor") and acard:getSuit() ~= sgs.Card_Heart then has_armor = true end
	end

	for _, acard in ipairs(cards) do
		if acard:getSuit() == sgs.Card_Heart and not acard:isKindOf("TrickCard") and ((self:getUseValue(acard) < sgs.ai_use_value.Indulgence) or inclusive) then
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
	local damage = self.player:getTag("ThYuhuoDamage"):toDamage()
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
			return "$" .. ret[1]
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
local thlianhua_skill = {}
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
	if self.player:getWeapon() and self:getCardId("Slash") and self:slashIsAvailable() and card:getEffectiveId() == self.player:getWeapon():getEffectiveId() then return end
	return sgs.Card_Parse("@ThLianhuaCard=" .. card:getEffectiveId())
end

sgs.ai_skill_use_func.ThLianhuaCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_playerchosen.thlianhua = function(self, targets)
	local card_ids = sgs.QList2Table(self.player:getTag("ThLianhuaIds"):toIntList())
	local target, cardId = sgs.ai_skill_askforyiji.ikyumeng(self, card_ids)
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
	if player:getHp() > 1 or (null.m_to and null.m_to:objectName() == player:objectName() and sgs.dynamic_value.damage_card[null.m_trick:getClassName()]) then
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
local thheiguan_skill = {}
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
	for _, p in ipairs(self.enemies) do
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
local thkanyao_skill = {}
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

--折辉：锁定技，每当你受到一次伤害后，你防止你受到的伤害，直到回合结束。且此回合的结束阶段开始时，你须弃置当前回合角色的一张牌；或视为对一名其他角色使用一张无视距离的【杀】。
function sgs.ai_slash_prohibit.thzhehui(self, from, to)
	if not to:hasSkill("thzhehui") then return false end
	if from:hasSkill("ikxuwu") or from:getMark("thshenyou") > 0 or (from:hasSkill("ikwanhun") and from:distanceTo(to) == 1) then return false end
	if from:hasFlag("IkJieyouUsed") then return false end
	return to:getMark("@shine") > 0 and to:hasSkill("thzhehui")
end

sgs.ai_skill_choice.thzhehui = function(self, choices)
	local n = self.player:getLostHp()
	local current = self.room:getCurrent()
	self:sort(self.enemies, "defenseSlash")
	for _, enemy in ipairs(self.enemies) do
		local def = sgs.getDefenseSlash(enemy, self)
		local slash = sgs.cloneCard("slash")
		local eff = self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self)

		if self.player:canSlash(enemy, nil, false) and not self:slashProhibit(nil, enemy) and eff and def < 5 then
			self.thzhehui_target = enemy
			return "slash"
		end
	end
	if self:isEnemy(current) then
		for _, enemy in ipairs(self.enemies) do
			local slash = sgs.cloneCard("slash")
			local eff = self:slashIsEffective(slash, enemy)

			if self.player:canSlash(enemy, nil, false) and not self:slashProhibit(nil, enemy) and self:hasHeavySlashDamage(self.player, slash, enemy) then
				self.thzhehui_target = enemy
				return "slash"
			end
		end
		local armor = current:getArmor()
		if armor and self:evaluateArmor(armor, current) >= 3 and not self:doNotDiscard(current, "e") then return "discard" end
	end
	if self:isFriend(current) then
		if n == 1 and self:needToThrowArmor(current) then return "discard" end
		for _, enemy in ipairs(self.enemies) do
			local slash = sgs.cloneCard("slash")
			local eff = self:slashIsEffective(slash, enemy)

			if self.player:canSlash(enemy, nil, false) and not self:slashProhibit(nil, enemy) then
				self.thzhehui_target = enemy
				return "slash"
			end
		end
		return "slash"
	end
	return "discard"
end

sgs.ai_skill_playerchosen.thzhehui = function(self, targets)
	local to = self.thzhehui_target
	if to then 
		self.thzhehui_target = nil
		return to
	end
	to = sgs.ai_skill_playerchosen.zero_card_as_slash(self, targets)
	return to or targets[1]
end

sgs.ai_choicemade_filter.cardChosen.thzhehui = sgs.ai_choicemade_filter.cardChosen.snatch

--沉寂：你的回合外，每当失去牌时，你可以将一名角色的人物牌横置或者重置。
sgs.ai_skill_playerchosen.thchenji = function(self, targets)
	return self:findPlayerToChain(targets)
end

--狂想：当人物牌横置的角色被选择为基本牌或非延时类锦囊牌的目标后，若你不为其目标，你可无视合法性成为该牌的目标；当你被选择为基本牌或非延时类锦囊牌的目标后，你可令所有人物牌横置的不为其目标的角色都无视合法性成为该牌的目标。在结算后，将这些角色的人物牌重置。
sgs.ai_skill_invoke.thkuangxiang = function(self, data)
	local use = data:toCardUse()
	if use.from and use.to:isEmpty() then
		use.to:append(use.from)
	end
	for _, p in sgs.qlist(use.to) do
		if p:isChained() and not use.to:contains(self.player) then
			if (use.card:isKindOf("Peach") and self.player:isWounded()) or use.card:isKindOf("ExNihilo") then
				return true
			end
			local current = self.room:getCurrent()
			if current and current:isAlive() and current:getPhase() < sgs.Player_Play and self:isEnemy(current) and current:canSlash(self.player, nil, false) then
				if use.card:isKindOf("LureTiger") then
					return true
				end
			end
			break
		end
	end
	if use.to:contains(self.player) then
		local targets = {}
		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if not use.to:contains(p) and p:isChained() then
				table.insert(targets, p)
			end
		end
		local good, bad = 0, 0
		for _, p in ipairs(targets) do
			local value = 0.2
			if not use.card:isKindOf("FireAttack") and sgs.dynamic_value.damage_card[use.card:getClassName()] then
				if use.card:isKindOf("Slash") and not self:slashIsEffective(use.card, p, use.from) then
				elseif use.card:isKindOf("TrickCard") and not self:hasTrickEffective(use.card, p, use.from, true) then
				else
					value = value - 1
					if self:isWeak(p) then
						value = value - 0.5
					end
				end
			elseif use.card:isKindOf("Dismantlement") then
				if not self:hasTrickEffective(use.card, p, use.from, true) then
				elseif p:isAllNude() then
				else
					if self:isFriend(use.from, p) and self:needToThrowArmor(p) then
						value = value + 1
					elseif self:needKongcheng(p) and p:getHandcardNum() == 1 and (self:isFriend(use.from, p) or p:getCards("hej"):length() == 1) then
						value = value + 0.3
					elseif (p:containsTrick("indulgence") or p:containsTrick("supply_shortage")) and (self:isFriend(use.from, p) or p:isNude()) then
						value = value + 0.5
					else
						value = value - 0.5
						if self:isWeak(p) then
							value = value - 0.3
						end
					end
				end
			elseif use.card:isKindOf("Peach") or use.card:isKindOf("ExNihilo") then
				if use.card:isKindOf("Peach") and not p:isWounded() then
				elseif use.card:isKindOf("ExNihilo") and not self:hasTrickEffective(use.card, p, use.from, true) then
				else
					value = value + 1
					if self:isWeak(p) then
						value = value + 0.3
					end
				end
			end
			if self:isFriend(p) then
				good = good + value
			elseif self:isEnemy(p) then
				bad = bad + value
			end
		end
		return good - bad > 1
	end
	return false
end

--恶戏：结束阶段开始时，你可以弃置一张牌并选择两名有手牌的角色，这两名角色须同时展示一张手牌：若这两张牌点数不同，你获得其中一张牌，所展示的牌点数大的角色获得另一张牌；若点数相同，你须在此回合结束后进行一个额外的回合，且此额外的回合内你不可以发动“恶戏”。
sgs.ai_skill_use["@@thexi"] = function(self, prompt, method)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, c in ipairs(cards) do
		if not self:isValuableCard(c) then
			local targets = {}
			for _, c2 in ipairs(cards) do
				if c2:getEffectiveId() == c:getEffectiveId() then continue end
				if c2:getNumber() == 13 then
					table.insert(targets, self.player:objectName())
					break
				end
			end
			self:sort(self.enemies, "handcard")
			for _, p in ipairs(self.enemies) do
				if not p:isKongcheng() then
					table.insert(targets, p:objectName())
					if #targets > 1 then
						return "@ThExiCard=" .. c:getEffectiveId() .. "->" .. table.concat(targets, "+")
					end
				end
			end
		end
	end	
end

sgs.ai_cardshow.thexi = function(self, requestor)
	if self.player:objectName() == requestor:objectName() then
		return self:getMaxCard()
	elseif self:isFriend(requestor) then
		local a, b = self:getCardNeedPlayer(nil, { requestor })
		return a
	else
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		return cards[1]
	end
	return self.player:getRandomHandCard()
end

sgs.ai_skill_choice.thexi = function(self, choices, data)
	local ids = data:toIntList()
	local cards, targets = {}, {}
	for _, id in sgs.qlist(ids) do
		local c = sgs.Sanguosha:getCard(id)
		local p = self.room:getCardOwner(id)
		table.insert(cards, c)
		table.insert(targets, p)
	end
	local big = nil
	if cards[1]:getNumber() > cards[2]:getNumber() then
		big = targets[1]
	else
		big = targets[2]
	end
	if self:isFriend(big) then
		self:sortByUseValue(cards)
		if cards[2]:getNumber() > cards[1]:getNumber() then
			return "big"
		else
			return "small"
		end
	else
		self:sortByKeepValue(cards, false, sgs.QList2Table(self.player:getHandcards()))
		if cards[2]:getNumber() > cards[1]:getNumber() then
			return "big"
		else
			return "small"
		end
	end
end

--星露：当你进入濒死状态时，你可以与一名其他角色拼点：若你赢，你回复1点体力。
sgs.ai_skill_playerchosen.thxinglu = function(self, targets)
	local max_card = self:getMaxCard()
	if isCard("Peach", max_card, self.player) or isCard("Analeptic", max_card, self.player) then return nil end
	local max_point = max_card:getNumber()

	self:sort(self.enemies, "handcard")
	for _, enemy in ipairs(self.enemies) do
		if not (self:needKongcheng(enemy) and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			local enemy_max_card = self:getMaxCard(enemy)
			local enemy_max_point = enemy_max_card and enemy_max_card:getNumber() or 100
			if max_point > enemy_max_point then
				self.thxinglu_card = max_card:getEffectiveId()
				return enemy
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if not (self:needKongcheng(enemy) and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			if max_point >= 10 then
				self.thxinglu_card = max_card:getEffectiveId()
				return enemy
			end
		end
	end

	self:sort(self.friends_noself, "handcard")
	for index = #self.friends_noself, 1, -1 do
		local friend = self.friends_noself[index]
		if not friend:isKongcheng() then
			local friend_min_card = self:getMinCard(friend)
			local friend_min_point = friend_min_card and friend_min_card:getNumber() or 100
			if max_point > friend_min_point then
				self.thxinglu_card = max_card:getEffectiveId()
				return friend
			end
		end
	end

	local zhugeliang = self.room:findPlayerBySkillName("ikjingyou")
	if zhugeliang and self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 1 and zhugeliang:objectName() ~= self.player:objectName() then
		if max_point >= 7 then
			self.thxinglu_card = max_card:getEffectiveId()
			return zhugeliang
		end
	end

	for index = #self.friends_noself, 1, -1 do
		local friend = self.friends_noself[index]
		if not friend:isKongcheng() then
			if max_point >= 7 then
				self.thxinglu_card = max_card:getEffectiveId()
				return friend
			end
		end
	end

	return nil
end

function sgs.ai_skill_pindian.thxinglu(minusecard, self, requestor)
	if requestor:getHandcardNum() == 1 then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		return cards[1]
	end
	local maxcard = self:getMaxCard()
	return self:isFriend(requestor) and self:getMinCard() or (maxcard:getNumber() < 6 and minusecard or maxcard)
end

sgs.ai_cardneed.thxinglu = sgs.ai_cardneed.bignumber

--暗病：锁定技，你的手牌上限始终-1，且若你于出牌阶段未使用过锦囊牌，你跳过此回合的弃牌阶段。
--smart-ai.lua SmartAI:useTrickCard

--慧略：出牌阶段，若你此阶段使用的上一张非延时类锦囊牌是非转化的，你可以将一张相同花色的手牌当此牌使用。
--smart-ai.lua SmartAI:getUseValue
local thhuilve_skill = {}
thhuilve_skill.name = "thhuilve"
table.insert(sgs.ai_skills, thhuilve_skill)
thhuilve_skill.getTurnUseCard = function(self)
	local cards = self.player:getCards("h")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	local huilve_table = sgs.GetProperty(self.player, "thhuilve"):split("+")
	if #huilve_table ~= 2 then return end
	local asuit = tonumber(huilve_table[2])
	local trick = sgs.cloneCard(huilve_table[1], asuit)
	if not trick:isAvailable(self.player) then return end

	local acard
	for _, card in ipairs(cards) do
		if card:getSuit() == asuit then
			acard = card
			break
		end
	end

	if not acard then return nil end
	local suit = acard:getSuitString()
	local number = acard:getNumberString()
	local card_id = acard:getEffectiveId()
	local card_str = ("%s:thhuilve[%s:%s]=%d"):format(huilve_table[1], suit, number, card_id)
	local trick = sgs.Card_Parse(card_str)
	assert(trick)

	return trick
end

function sgs.ai_cardneed.thhuilve(to, card)
	return card:isNDTrick() and sgs.dynamic_value.damage_card[card:getClassName()]
end

--疾智：锁定技，你使用锦囊牌时无距离限制。
--无

--神佑：摸牌阶段开始时，你可以放弃摸牌，改为选择一名其他角色，且若其手牌数不小于你，你摸一张牌；然后你可以获得其区域内的一张牌，你还可以对其使用一张无视距离的【杀】，且你可以令此【杀】即将造成的伤害视为失去体力。
sgs.ai_skill_playerchosen.thshenyou = function(self, targets)
	local targets = self:findPlayerToDiscard("hej", false, false, nil, true)
	if targets and #targets > 0 and self:isFriend(targets[1]) then
		return targets[1]
	end
	local slashs = self:getCards("Slash")
	if #slashs > 0 then
		for _, p in ipairs(targets) do
			if p:getHandcardNum() >= self.player:getHandcardNum() and self:isEnemy(p) then
				local yes = false
				self.player:addMark("thshenyou")
				for _, s in ipairs(slashs) do
					if self.player:canSlash(p, s, false) and not self:slashProhibit(s, p, self.player) then
						yes = true
						break
					end
				end
				self.player:removeMark("thshenyou")
				if yes then
					return p
				end
			end
		end
		for _, p in ipairs(targets) do
			if self:isEnemy(p) then
				local yes = false
				self.player:addMark("thshenyou")
				for _, s in ipairs(slashs) do
					if not self.player:canSlash(p, s) and self.player:canSlash(p, s, false) and not self:slashProhibit(s, p, self.player) then
						yes = true
						break
					end
				end
				self.player:removeMark("thshenyou")
				if yes then
					return p
				end
			end
		end
	end
	for _, p in ipairs(targets) do
		if p:getHandcardNum() >= self.player:getHandcardNum() and self:isEnemy(p) then
			self.player:addMark("thshenyou")
			local yes = self.player:canSlash(p, nil, false) and not self:slashProhibit(nil, p, self.player)
			self.player:removeMark("thshenyou")
			if yes then
				return p
			end
		end
	end
	for _, p in ipairs(targets) do
		if p:getHandcardNum() >= self.player:getHandcardNum() and self:isEnemy(p) then
			return p
		end
	end
	return nil
end

sgs.ai_skill_invoke.thshenyou = function(self, data)
	local damage = self.player:getTag("ThShenyouDamage"):toDamage()
	if damage and damage.to then
		if self:isEnemy(damage.to) then
			if self:slashProhibit(damage.card, damage.to, self.player) or damage.to:hasSkills(sgs.masochism_skill) then
				return true
			end
			if damage.nature ~= sgs.DamageStruct_Normal and damage.to:isChained() and self:isGoodChainTarget(damage.to, self.player, damage.nature, damage.damage) then
				return false
			end
			return math.random(1, 3) ~= 1
		end
		return false
	end
	return true
end

sgs.ai_choicemade_filter.cardChosen.thshenyou = sgs.ai_choicemade_filter.cardChosen.snatch

sgs.ai_choicemade_filter.skillInvoke.thshenyou = function(self, player, promptlist)
	local damage = self.player:getTag("ThShenyouDamage"):toDamage()
	if damage and damage.to and promptlist[#promptlist] == "yes" then
		if self:slashProhibit(damage.card, damage.to, self.player) or damage.to:hasSkills(sgs.masochism_skill) then
			sgs.updateIntention(player, damage.to, 40)
		end
	end
end

--天阙：每当你于出牌阶段使用一张牌时，若与你此阶段已使用的牌类别均不同，你摸一张牌。
sgs.ai_skill_invoke.thtianque = true

--归墟：结束阶段开始时，若你此回合使用的最后一张牌是锦囊牌，你可以将场上的一张牌移动到另一名角色的区域内的相应位置。
local function card_for_thguixu(self, who, return_prompt)
	local card, target
	if self:isFriend(who) then
		local judges = who:getJudgingArea()
		if not judges:isEmpty() then
			for _, judge in sgs.qlist(judges) do
				card = sgs.Sanguosha:getCard(judge:getEffectiveId())
				if not judge:isKindOf("YanxiaoCard") and not judge:isKindOf("PurpleSong") then
					for _, enemy in ipairs(self.enemies) do
						if not enemy:containsTrick(judge:objectName()) and not enemy:containsTrick("YanxiaoCard")
							and not self.room:isProhibited(self.player, enemy, judge)
							and not (enemy:hasSkills("hongyan|wuyan") and judge:isKindOf("Lightning")) then
							target = enemy
							break
						end
					end
					if target then break end
				end
			end
		end

		local equips = who:getCards("e")
		local weak = false
		if not target and not equips:isEmpty() and (who:hasSkills(sgs.lose_equip_skill) or self:needToThrowArmor(who)) then
			if self:needToThrowArmor(who) then
				card = who:getArmor()
			else
				for _, equip in sgs.qlist(equips) do
					if equip:isKindOf("OffensiveHorse") then card = equip break
					elseif equip:isKindOf("Weapon") then card = equip break
					elseif equip:isKindOf("DefensiveHorse") and not self:isWeak(who) then
						card = equip
						break
					elseif equip:isKindOf("Armor") and (not self:isWeak(who) or self:needToThrowArmor(who)) then
						card = equip
						break
					else
						card = equip
						break
					end
				end
			end

			if card then
				if card:isKindOf("Armor") or card:isKindOf("DefensiveHorse") or card:isKindOf("WoodenOx") then
					self:sort(self.friends, "defense")
				else
					self:sort(self.friends, "handcard")
					self.friends = sgs.reverse(self.friends)
				end
				for _, friend in ipairs(self.friends) do
					if not self:getSameEquip(card, friend) and friend:objectName() ~= who:objectName()
						and friend:hasSkills(sgs.need_equip_skill .. "|" .. sgs.lose_equip_skill) then
						target = friend
						break
					end
				end
				for _, friend in ipairs(self.friends) do
					if not self:getSameEquip(card, friend) and friend:objectName() ~= who:objectName() then
						target = friend
						break
					end
				end
			end
		end
	else
		local judges = who:getJudgingArea()
		if who:containsTrick("YanxiaoCard") or who:containsTrick("purple_song") then
			for _, judge in sgs.qlist(judges) do
				if judge:isKindOf("YanxiaoCard") or judge:isKindOf("PurpleSong")then
					card = sgs.Sanguosha:getCard(judge:getEffectiveId())
					for _, friend in ipairs(self.friends) do
						if not friend:containsTrick(judge:objectName()) and not self.room:isProhibited(self.player, friend, judge)
							and not friend:getJudgingArea():isEmpty() then
							target = friend
							break
						end
					end
					if target then break end
					for _, friend in ipairs(self.friends) do
						if not friend:containsTrick(judge:objectName()) and not self.room:isProhibited(self.player, friend, judge) then
							target = friend
							break
						end
					end
					if target then break end
				end
			end
		end

		if card == nil or target == nil then
			if not who:hasEquip() or (who:hasSkills(sgs.lose_equip_skill) and not who:getTreasure()) then return nil end
			local card_id = self:askForCardChosen(who, "e", "dummy")
			if who:hasEquip(sgs.Sanguosha:getCard(card_id)) then card = sgs.Sanguosha:getCard(card_id) end
			if card then
				if card:isKindOf("Armor") or card:isKindOf("DefensiveHorse") or card:isKindOf("WoodenOx") then
					self:sort(self.friends, "defense")
				else
					self:sort(self.friends, "handcard")
					self.friends = sgs.reverse(self.friends)
				end
				for _, friend in ipairs(self.friends) do
					if not self:getSameEquip(card, friend) and friend:objectName() ~= who:objectName() and friend:hasSkills(sgs.lose_equip_skill .. "|shensu") then
						target = friend
						break
					end
				end
				if not target then
					for _, friend in ipairs(self.friends) do
						if not self:getSameEquip(card, friend) and friend:objectName() ~= who:objectName() then
							target = friend
							break
						end
					end
				end
			end
		end
	end

	if return_prompt == "card" then return card
	elseif return_prompt == "target" then return target
	else
		return (card and target)
	end
end

sgs.ai_skill_use["@@thguixu"] = function(self, prompt)
	self:sort(self.enemies, "hp")
	local has_armor = true
	local judge
	for _, friend in ipairs(self.friends) do
		if not friend:getCards("j"):isEmpty() and card_for_thguixu(self, friend, ".") then
			return "@ThGuixuCard=.->" .. friend:objectName()
		end
	end

	for _, enemy in ipairs(self.enemies) do
		if not enemy:getCards("j"):isEmpty() and card_for_thguixu(self, enemy, ".") then
			return "@ThGuixuCard=.->" .. enemy:objectName()
		end
	end

	for _, friend in ipairs(self.friends_noself) do
		if not friend:getCards("e"):isEmpty() and card_for_thguixu(self, friend, ".") then
			return "@ThGuixuCard=.->" .. friend:objectName()
		end
		if not friend:getArmor() then has_armor = false end
	end

	local targets = {}
	for _, enemy in ipairs(self.enemies) do
		if card_for_thguixu(self, enemy, ".") then
			table.insert(targets, enemy)
		end
	end

	if #targets > 0 then
		self:sort(targets, "defense")
		return "@ThGuixuCard=.->" .. targets[#targets]:objectName()
	end

	return "."
end

sgs.ai_skill_cardchosen.thguixu = function(self, who, flags)
	return card_for_thguixu(self, who, "card")
end

sgs.ai_skill_playerchosen.thguixu = function(self, targets)
	local who = self.player:getTag("ThGuixuTarget"):toPlayer()
	if who then
		if not card_for_thguixu(self, who, "target") then self.room:writeToConsole("NULL") end
		return card_for_thguixu(self, who, "target")
	end
end

--永夜：出牌阶段，当你使用一张非延时类锦囊牌时，你可以弃置一张黑色牌并为该牌多或少指定一个目标。
sgs.ai_skill_cardask["@thyongye"] = function(self, data, pattern)
	local cards = {}
	for _, c in sgs.qlist(self.player:getCards("he")) do
		if c:isBlack() then
			table.insert(cards, c)
		end
	end
	if #cards == 0 then return "." end
	self:sortByUseValue(cards, true)
	if not self:isValuableCard(cards[1]) then
		local ret = sgs.ai_skill_choice.thyongye(self, "", data)
		if ret ~= "" then
			return "$" .. cards[1]:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_skill_choice.thyongye = function(self, choices, data)
	self.thyongye_extra_target = nil
	self.thyongye_remove_target = nil
	local use = data:toCardUse()
	if use.card:isKindOf("Collateral") then
		local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {} }
		for _, p in sgs.qlist(use.to) do
			table.insert(dummy_use.current_targets, p:objectName())
		end
		self:useCardCollateral(use.card, dummy_use)
		if dummy_use.card and dummy_use.to:length() == 2 then
			local first = dummy_use.to:at(0):objectName()
			local second = dummy_use.to:at(1):objectName()
			self.thyongye_collateral = { first, second }
			return "add"
		else
			self.thyongye_collateral = nil
		end
	elseif use.card:isKindOf("ExNihilo") then
		local friend = self:findPlayerToDraw(false, 2, use.card)
		if friend then
			self.thyongye_extra_target = friend
			return "add"
		end
	elseif use.card:isKindOf("GodSalvation") then
		self:sort(self.enemies, "hp")
		for _, enemy in ipairs(self.enemies) do
			if enemy:isWounded() and self:hasTrickEffective(use.card, enemy, self.player) then
				self.thyongye_remove_target = enemy
				return "remove"
			end
		end
	elseif use.card:isKindOf("AmazingGrace") then
		self:sort(self.enemies)
		for _, enemy in ipairs(self.enemies) do
			if self:hasTrickEffective(use.card, enemy, self.player) and not hasManjuanEffect(enemy)
				and not self:needKongcheng(enemy, true) then
				self.thyongye_remove_target = enemy
				return "remove"
			end
		end
	elseif use.card:isKindOf("AOE") then
		self:sort(self.friends_noself)
		local lord = self.room:getLord()
		if lord and lord:objectName() ~= self.player:objectName() and self:isFriend(lord) and self:isWeak(lord) then
			self.thyongye_remove_target = lord
			return "remove"
		end
		for _, friend in ipairs(self.friends_noself) do
			if self:hasTrickEffective(use.card, friend, self.player) then
				self.thyongye_remove_target = friend
				return "remove"
			end
		end
	elseif use.card:isKindOf("Snatch") or use.card:isKindOf("Dismantlement") then
		local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {} }
		for _, p in sgs.qlist(use.to) do
			table.insert(dummy_use.current_targets, p:objectName())
		end
		self:useCardSnatchOrDismantlement(use.card, dummy_use)
		if dummy_use.card and dummy_use.to:length() > 0 then
			self.thyongye_extra_target = dummy_use.to:first()
			return "add"
		end
	elseif use.card:isKindOf("Duel") then
		local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {} }
		for _, p in sgs.qlist(use.to) do
			table.insert(dummy_use.current_targets, p:objectName())
		end
		self:useCardByClassName(use.card, dummy_use)
		if dummy_use.card and dummy_use.to:length() > 0 then
			self.thyongye_extra_target = dummy_use.to:first()
			return "add"
		end
	end
	if choices == "" then
		return ""
	end
	self.room:writeToConsole("ThYongye Target Error!")
	return ""
end

sgs.ai_skill_playerchosen.thyongye = function(self, targets)
	if not self.thyongye_extra_target and not self.thyongye_remove_target then self.room:writeToConsole("ThYongye player chosen error!!") end
	return self.thyongye_extra_target or self.thyongye_remove_target
end

sgs.ai_skill_use["@@thyongye!"] = function(self, prompt) -- extra target for Collateral
	if not self.thyongye_collateral then self.room:writeToConsole("ThYongye player chosen error!!") end
	return "@ExtraCollateralCard=.->" .. self.thyongye_collateral[1] .. "+" .. self.thyongye_collateral[2]
end

--世明：摸牌阶段开始时，你可以放弃摸牌，改为从牌堆顶亮出四张牌，你获得其中的红色牌或者点数不大于9的牌，将其余的牌置入弃牌堆。
sgs.ai_skill_invoke.thshiming = true

sgs.ai_skill_choice.thshiming = function(self, choices, data)
	t = choices:split("+")
	if #t == 1 then
		return t[1]
	end
	local red, nine = {}, {}
	local ids = data:toIntList()
	for _, id in sgs.qlist(ids) do
		local c = sgs.Sanguosha:getCard(id)
		if c:isRed() then
			table.insert(red, c)
		end
		if c:getNumber() <= 9 then
			table.insert(nine, c)
		end
	end
	if #red ~= 0 and #nine == 0 then
		return "red"
	elseif #red == 0 and #nine ~= 0 then
		return "nine"
	else
		local red_slash, nine_slash
		local slash = self:getCard("Slash")
		if slash then
			red_slash, nine_slash = true, true
		end
		local red_n, nine_n = 0, 0
		for _, c in ipairs(red) do
			if c:isKindOf("Slash") then
				if not red_slash then
					red_n = red_n + 1
					red_slash = true
				end
				continue
			end
			red_n = red_n + 1
		end
		for _, c in ipairs(nine) do
			if c:isKindOf("Slash") then
				if not nine_slash then
					nine_n = nine_n + 1
					nine_slash = true
				end
				continue
			end
			nine_n = nine_n + 1
		end
		if red_n > nine_n then
			return "red"
		elseif nine_n > red_n then
			return "nine"
		else
			return #red > #nine and "red" or (#nine > #red and "nine" or "red")
		end
	end
end

--神宝：限定技，出牌阶段，你可以从一名其他角色的区域获得等同于你攻击范围数量的牌（至多获得三张，不足则全部获得），若如此做，结束阶段开始时，你须弃置等同于你攻击范围数量的牌（不足则全弃）。
local thshenbao_skill = {}
thshenbao_skill.name = "thshenbao"
table.insert(sgs.ai_skills, thshenbao_skill)
thshenbao_skill.getTurnUseCard = function(self)
	if self.player:getMark("@shenbao") > 0 then
		return sgs.Card_Parse("@ThShenbaoCard=.")
	end
end

sgs.ai_skill_use_func.ThShenbaoCard = function(card, use, self)
	local n = math.min(self.player:getAttackRange(), 3)
	local slashs = self:getCards("Slash")
	if #slashs > 0 then
		for _, p in ipairs(self.enemies) do
			if p:isNude() then continue end
			for _, s in ipairs(slashs) do
				if self.player:hasWeapon("guding_blade") and self.player:canSlash(p, s) and self:slashIsAvailable(self.player, s) and self:slashIsEffective(s, p, self.player, true) and p:getCardCount() == n then
					use.card = card
					if use.to then
						use.to:append(p)
					end
					return
				end
			end
		end
		if n >= 2 then
			for _, p in ipairs(self.enemies) do
				if p:isNude() then continue end
				for _, s in ipairs(slashs) do
					if self.player:canSlash(p, s) and self:slashIsEffective(s, p, self.player, true) and self:slashIsAvailable(self.player, s) and p:getCardCount() > n then
						use.card = card
						if use.to then
							use.to:append(p)
						end
						return
					end
				end
			end
		end
	end
	if self:isWeak() then
		for _, p in ipairs(self.enemies) do
			if p:isNude() then continue end
			if getCardsNum("Peach", p, self.player) >= 1 and p:getCardCount() <= n then
				use.card = card
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end
end

sgs.ai_use_priority.ThShenbaoCard = sgs.ai_use_priority.Slash + 0.2

--云隠：君主技，每当其他月势力角色使用【杀】造成一次伤害后，可以进行一次判定，若为黑色，将场上的一张武器牌移动至你的手牌。
sgs.ai_skill_invoke.thyunyin = function(self, data)
	if self:isFriend(data:toPlayer()) then
		for _, p in ipairs(self.enemies) do
			if p:getWeapon() then
				return true
			end
		end
	end
	return false
end

sgs.ai_skill_playerchosen.thyunyin = function(self, targets)
	self:sort(self.enemies, "handcard")
	self.enemies = sgs.reverse(self.enemies)
	for _, p in ipairs(self.enemies) do
		if p:getWeapon() then
			return p
		end
	end
	for _, p in sgs.qlist(targets) do
		if not self:isFriend(p) then
			return p
		end
	end
	return targets:at(math.random(0, targets:length() - 1))
end

sgs.ai_choicemade_filter.skillInvoke.thyinbi = function(self, player, promptlist)
	local to = self.room:findPlayer(promptlist[#promptlist - 1])
	if to and promptlist[#promptlist] == "yes" then
		sgs.updateIntention(player, to, -50)
	end
end
