function SmartAI:resetCards(cards, except)
	local result = {}
	for _, c in ipairs(cards) do
		if c:getEffectiveId() ~= except:getEffectiveId() then table.insert(result, c) end
	end
	return result
 end

function SmartAI:shouldUseIkShenai()
	if (self:hasCrossbowEffect() or self:getCardsNum("Crossbow") > 0) and self:getCardsNum("Slash") > 0 then
		self:sort(self.enemies, "defense")
		for _, enemy in ipairs(self.enemies) do
			local inAttackRange = self.player:distanceTo(enemy) == 1 or self.player:distanceTo(enemy) == 2
									and self:getCardsNum("OffensiveHorse") > 0 and not self.player:getOffensiveHorse()
			if inAttackRange and sgs.isGoodTarget(enemy, self.enemies, self) then
				local slashs = self:getCards("Slash")
				local slash_count = 0
				for _, slash in ipairs(slashs) do
					if not self:slashProhibit(slash, enemy) and self:slashIsEffective(slash, enemy) then
						slash_count = slash_count + 1
					end
				end
				if slash_count >= enemy:getHp() then return false end
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if enemy:canSlash(self.player) and not self:slashProhibit(nil, self.player, enemy) then
			if enemy:hasWeapon("guding_blade") and self.player:getHandcardNum() == 1 and getCardsNum("Slash", enemy, self.player) >= 1 then
				return
			elseif self:hasCrossbowEffect(enemy) and getCardsNum("Slash", enemy, self.player) > 1 and self:getOverflow() <= 0 then
				return
			end
		end
	end

	for _, player in ipairs(self.friends_noself) do
		if (player:hasSkill("haoshi") and not player:containsTrick("supply_shortage")) or player:hasSkill("jijiu") then
			return true
		end
	end

	if (self.player:hasSkill("nosrende") and self.player:getMark("nosrende") < 2)
		or self:getOverflow() > 0 then
		return true
	end
	if self.player:getLostHp() < 2 then
		return true
	end
end

--神爱：出牌阶段限一次，你可以将至少一张手牌交给至少一名其他角色，若你以此法给出不少于两张牌，你回复1点体力。
local ikshenai_skill = {}
ikshenai_skill.name = "ikshenai"
table.insert(sgs.ai_skills, ikshenai_skill)
ikshenai_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("IkShenaiCard") or self.player:isKongcheng() then return end
	if self:shouldUseIkShenai() then
		return sgs.Card_Parse("@IkShenaiCard=.")
	end
end

sgs.ai_skill_use_func.IkShenaiCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_askforyiji.ikshenai = function(self, card_ids)
	if self.player:getHandcardNum() <= 2 then
		return nil, -1
	end

	local available_friends = {}
	for _, friend in ipairs(self.friends_noself) do
		if not hasManjuanEffect(friend) and not self:isThJinluTarget(friend) then
			table.insert(available_friends, friend)
		end
	end

	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	local id = card_ids[1]

	local card, friend = self:getCardNeedPlayer(cards, self.friends_noself)
	if card and friend and table.contains(available_friends, friend) then return friend, card:getId() end
	if #available_friends > 0 then
		self:sort(available_friends, "handcard")
		for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
				return afriend, id
			end
		end
	end
	return nil, -1
end

sgs.ai_use_value.IkShenaiCard = 8.5
sgs.ai_use_priority.IkShenaiCard = 8.8

--心契：君主技，当你需要使用或打出一张【杀】时，你可以令其他风势力角色打出一张【杀】（视为由你使用或打出）。
table.insert(sgs.ai_global_flags, "ikxinqisource")
local ikxinqi_filter = function(self, player, carduse)
	if carduse.card:isKindOf("IkXinqiCard") then
		sgs.ikxinqisource = player
	else
		sgs.ikxinqisource = nil
	end
end

table.insert(sgs.ai_choicemade_filter.cardUsed, ikxinqi_filter)

sgs.ai_skill_invoke.ikxinqi = function(self, data)
	if sgs.ikxinqisource then return false end
	local asked = data:toStringList()
	local prompt = asked[2]
	if self:askForCard("slash", prompt, 1) == "." then return false end
	
	local current = self.room:getCurrent()
	if self:isFriend(current) and current:getKingdom() == "kaze" and self:getOverflow(current) > 2 and not self:hasCrossbowEffect(current) then
		return true
	end

	local cards = self.player:getHandcards()
	for _, card in sgs.qlist(cards) do
		if isCard("Slash", card, self.player) then
			return false
		end
	end

	local lieges = self.room:getLieges("kaze", self.player)
	if lieges:isEmpty() then return false end
	local has_friend = false
	for _, p in sgs.qlist(lieges) do
		if not self:isEnemy(p) then
			has_friend = true
			break
		end
	end
	return has_friend
end

sgs.ai_choicemade_filter.skillInvoke.ikxinqi = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		sgs.ikxinqisource = player
	end
end

local ikxinqi_skill = {}
ikxinqi_skill.name = "ikxinqi"
table.insert(sgs.ai_skills, ikxinqi_skill)
ikxinqi_skill.getTurnUseCard = function(self)
	if not self.player:hasLordSkill("ikxinqi") then return end
	local lieges = self.room:getLieges("kaze", self.player)
	if lieges:isEmpty() then return end
	local has_friend
	for _, p in sgs.qlist(lieges) do
		if self:isFriend(p) then
			has_friend = true
			break
		end
	end
	if not has_friend then return end
	if self.player:hasUsed("IkXinqiCard") or self.player:hasFlag("Global_IkXinqiFailed") or not self:slashIsAvailable() then return end
	local card_str = "@IkXinqiCard=."
	local slash = sgs.Card_Parse(card_str)
	assert(slash)
	return slash
end

sgs.ai_skill_use_func.IkXinqiCard = function(card, use, self)
	self:sort(self.enemies, "defenseSlash")

	if not sgs.ikxinqitarget then table.insert(sgs.ai_global_flags, "ikxinqitarget") end
	sgs.ikxinqitarget = {}

	local dummy_use = { isDummy = true }
	dummy_use.to = sgs.SPlayerList()
	if self.player:hasFlag("slashTargetFix") then
		for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if p:hasFlag("SlashAssignee") then
				dummy_use.to:append(p)
			end
		end
	end
	local slash = sgs.cloneCard("slash")
	self:useCardSlash(slash, dummy_use)
	if dummy_use.card and dummy_use.to:length() > 0 then
		use.card = card
		for _, p in sgs.qlist(dummy_use.to) do
			table.insert(sgs.ikxinqitarget, p)
			if use.to then use.to:append(p) end
		end
	end
end

sgs.ai_use_value.IkXinqiCard = 8.5
sgs.ai_use_priority.IkXinqiCard = 2.45

sgs.ai_card_intention.IkXinqiCard = function(self, card, from, tos)
	if not from:isLord() and global_room:getCurrent():objectName() == from:objectName() then
		return sgs.ai_card_intention.Slash(self, card, from, tos)
	end
end

sgs.ai_choicemade_filter.cardResponded["@ikxinqi-slash"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		sgs.updateIntention(player, sgs.ikxinqisource, -40)
		sgs.ikxinqisource = nil
		sgs.ikxinqitarget = nil
	elseif sgs.ikxinqisource then
		if self:isFriend(player, sgs.ikxinqisource) then sgs.card_lack[player:objectName()]["Slash"] = 1 end
		if player:objectName() == player:getRoom():getLieges("kaze", sgs.ikxinqisource):last():objectName() then
			sgs.ikxinqisource = nil
			sgs.ikxinqitarget = nil
		end
	end
end

sgs.ai_skill_cardask["@ikxinqi-slash"] = function(self, data)
	if not sgs.ikxinqisource or not self:isFriend(sgs.ikxinqisource) then return "." end
	if self:needBear() then return "." end

	local ikxinqitargets = {}
	for _, player in sgs.qlist(self.room:getAllPlayers()) do
		if player:hasFlag("IkXinqiTarget") then
			if self:isFriend(player) and not (self:needToLoseHp(player, sgs.ikxinqisource, true) or self:getDamagedEffects(player, sgs.ikxinqisource, true)) then return "." end
			table.insert(ikxinqitargets, player)
		end
	end

	if #ikxinqitargets == 0 then
		return self:getCardId("Slash") or "."
	end

	self:sort(ikxinqitargets, "defenseSlash")
	local slashes = self:getCards("Slash")
	for _, slash in ipairs(slashes) do
		for _, target in ipairs(ikxinqitargets) do
			if not self:slashProhibit(slash, target, sgs.ikxinqisource) and self:slashIsEffective(slash, target, sgs.ikxinqisource) then
				return slash:toString()
			end
		end
	end
	return "."
end

function sgs.ai_cardsview_valuable.ikxinqi(self, class_name, player, need_lord)
	if class_name == "Slash" and sgs.Sanguosha:getCurrentCardUseReason() == sgs.CardUseStruct_CARD_USE_REASON_RESPONSE_USE
		and not player:hasFlag("Global_IkXinqiFailed") and (need_lord == false or player:hasLordSkill("ikxinqi")) then
		local current = self.room:getCurrent()
		if self:isFriend(current, player) and current:getKingdom() == "kaze" and self:getOverflow(current) > 2 and not self:hasCrossbowEffect(current) then
			return "@IkXinqiCard=."
		end

		local cards = player:getHandcards()
		for _, card in sgs.qlist(cards) do
			if isCard("Slash", card, player) then return end
		end

		local lieges = self.room:getLieges("kaze", player)
		if lieges:isEmpty() then return end
		local has_friend = false
		for _, p in sgs.qlist(lieges) do
			if self:isFriend(p, player) then
				has_friend = true
				break
			end
		end
		if has_friend then return "@IkXinqiCard=." end
	end
end

function SmartAI:getIkXinqiSlashNum(player)
	if not player then self.room:writeToConsole(debug.traceback()) return 0 end
	if not player:hasLordSkill("ikxinqi") then return 0 end
	local slashs = 0
	for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
		if p:getKingdom() == "kaze" and ((sgs.turncount <= 1 and sgs.ai_role[p:objectName()] == "neutral") or self:isFriend(player, p)) then
			slashs = slashs + getCardsNum("Slash", p, self.player)
		end
	end
	return slashs
end

--赤莲：你可以将一张红色牌当【杀】或具火焰伤害的【杀】使用或者打出。
local ikchilian_skill = {}
ikchilian_skill.name = "ikchilian"
table.insert(sgs.ai_skills, ikchilian_skill)
ikchilian_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local red_card, red_slash2
	self:sortByUseValue(cards, true)
	for _, card in ipairs(cards) do
		if card:isRed() and not card:isKindOf("Slash")
			and not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player)
			and (self:getUseValue(card) < sgs.ai_use_value.Slash or inclusive or sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, sgs.cloneCard("slash")) > 0) then
			red_card = card
			break
		end
	end
	for _, card in ipairs(cards) do
		if card:isRed() and not card:isKindOf("FireSlash")
			and not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player)
			and (self:getUseValue(card) < sgs.ai_use_value.FireSlash or inclusive or sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, sgs.cloneCard("fire_slash")) > 0) then
			red_slash2 = card
			break
		end
	end

	local slashs = {}
	if red_card then
		local suit = red_card:getSuitString()
		local number = red_card:getNumberString()
		local card_id = red_card:getEffectiveId()
		local card_str = ("slash:ikchilian[%s:%s]=%d"):format(suit, number, card_id)
		local slash = sgs.Card_Parse(card_str)

		assert(slash)
		table.insert(slashs, slash)
	end
	if red_card2 then
		local suit = red_card2:getSuitString()
		local number = red_card2:getNumberString()
		local card_id = red_card2:getEffectiveId()
		local card_str = ("fire_slash:ikchilian[%s:%s]=%d"):format(suit, number, card_id)
		local slash = sgs.Card_Parse(card_str)

		assert(slash)
		table.insert(slashs, slash)
	end
	if #slashs > 0 then
		return slashs
	end
end

sgs.ai_view_as.ikchilian = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local ret = {}
	if card_place ~= sgs.Player_PlaceSpecial and card:isRed() and not card:isKindOf("Peach") and not card:hasFlag("using") then
		table.insert(ret, ("fire_slash:ikchilian[%s:%s]=%d"):format(suit, number, card_id))
		table.insert(ret, ("slash:ikchilian[%s:%s]=%d"):format(suit, number, card_id))
		return ret
	end
end

sgs.ai_cardneed.ikchilian = function(to, card)
	return to:getHandcardNum() < 3 and card:isRed()
end

--真红：锁定技，你使用红桃【杀】时无距离限制；你使用方块【杀】指定一名角色为目标后，无视其防具。
--standard_cards-ai.lua SmartAI:slashIsEffective

--翼咆：出牌阶段，你可以使用任意数量的【杀】。
--无

--酾酒：你可以将一张武器牌或非延时类锦囊牌当【酒】使用。
ikshijiu_skill = {}
ikshijiu_skill.name = "ikshijiu"
table.insert(sgs.ai_skills, ikshijiu_skill)
ikshijiu_skill.getTurnUseCard = function(self)
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local card

	self:sortByUseValue(cards, true)

	for _, acard in ipairs(cards) do
		if acard:isNDTrick() or acard:isKindOf("Weapon") then
			card = acard
			break
		end
	end

	if not card then return nil end
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("analeptic:ikshijiu[spade:%s]=%d"):format(number, card_id)
	local analeptic = sgs.Card_Parse(card_str)

	if sgs.Analeptic_IsAvailable(self.player, analeptic) then
		assert(analeptic)
		return analeptic
	end
end

sgs.ai_view_as.ikshijiu = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place == sgs.Player_PlaceHand or card_place == sgs.Player_PlaceEquip then
		if card:isNDTrick() or card:isKindOf("Weapon") then
			return ("analeptic:ikshijiu[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

function sgs.ai_cardneed.ikshijiu(to, card, self)
	return (card:isNDTrick() or card:isKindOf("Weapon")) and getCardsNum("Analeptic", to, self.player) < 2
end

--预悉：准备阶段开始时，你可以观看牌堆顶的X张牌（X为存活角色的数量，且至多为5），将其中任意数量的牌以任意顺序置于牌堆顶，其余以任意顺序置于牌堆底。
dofile "lua/ai/guanxing-ai.lua"

sgs.ai_skill_invoke.ikxushi = true

--静幽：锁定技，若你没有手牌，你不能成为【杀】或者【碎月绮斗】的目标。
--无

--御风：每当你使用【杀】指定一名角色为目标后，你可以令该角色的非锁定技无效，直到回合结束，然后你进行一次判定，令其选择一项：不能使用【闪】响应此【杀】；或弃置与判定结果花色相同的一张牌。
sgs.ai_skill_invoke.ikyufeng = function(self, data)
	local target = data:toPlayer()
	return not self:isFriend(target)
end

sgs.ai_skill_cardask["@ikyufeng-discard"] = function(self, data, pattern)
	local suit = pattern:split("|")[2]
	local use = data:toCardUse()
	if self:needToThrowArmor() and self.player:getArmor():getSuitString() == suit then return "$" .. self.player:getArmor():getEffectiveId() end
	if not self:slashIsEffective(use.card, self.player, use.from)
		or (not self:hasHeavySlashDamage(use.from, use.card, self.player)
			and (self:getDamagedEffects(self.player, use.from, true) or self:needToLoseHp(self.player, use.from, true))) then return "." end
	if not self:hasHeavySlashDamage(use.from, use.card, self.player) and self:getCardsNum("Peach") > 0 then return "." end
	if self:getCardsNum("Jink") == 0 or not sgs.isJinkAvailable(use.from, self.player, use.card, true) then return "." end

	local equip_index = { 3, 0, 2, 4, 1 }
	if self.player:hasSkills(sgs.lose_equip_skill) then
		for _, i in ipairs(equip_index) do
			if i == 4 then break end
			if self.player:getEquip(i) and self.player:getEquip(i):getSuitString() == suit then return "$" .. self.player:getEquip(i):getEffectiveId() end
		end
	end

	local jiangqin = self.room:findPlayerBySkillName("niaoxiang")
	local need_double_jink = use.from:hasSkill("wushuang")
							or (use.from:hasSkill("roulin") and self.player:isFemale())
							or (self.player:hasSkill("roulin") and use.from:isFemale())
							or (jiangqin and jiangqin:isAdjacentTo(self.player) and use.from:isAdjacentTo(self.player) and self:isEnemy(jiangqin))

	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		if card:getSuitString() ~= suit or (not self:isWeak() and (self:getKeepValue(card) > 8 or self:isValuableCard(card)))
			or (isCard("Jink", card, self.player) and self:getCardsNum("Jink") - 1 < (need_double_jink and 2 or 1)) then continue end
		return "$" .. card:getEffectiveId()
	end

	for _, i in ipairs(equip_index) do
		if self.player:getEquip(i) and self.player:getEquip(i):getSuitString() == suit then
			if not (i == 1 and self:evaluateArmor() > 3)
				and not (i == 4 and self.player:getTreasure():isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):length() >= 3) then
				return "$" .. self.player:getEquip(i):getEffectiveId()
			end
		end
	end
end

sgs.ai_choicemade_filter.skillInvoke.ikyufeng = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		local target = findPlayerByObjectName(self.room, promptlist[#promptlist - 1])
		if target then sgs.updateIntention(player, target, 50) end
	end
end

--慧泉：当你使用一张锦囊牌时，你可以摸一张牌。
sgs.ai_skill_invoke.ikhuiquan = true

function sgs.ai_cardneed.ikhuiquan(to, card)
	return card:isKindOf("TrickCard")
end

sgs.ikhuiquan_keep_value = {
	Peach = 6,
	Analeptic = 5.9,
	Jink = 5.8,
	ExNihilo = 5.7,
	Snatch = 5.7,
	Dismantlement = 5.6,
	Indulgence = 5.6,
	SupplyShortage = 5.6,
	PurpleSong = 5.6,
	IronChain = 5.5,
	SavageAssault = 5.4,
	Duel = 5.3,
	ArcheryAttack = 5.2,
	Drowning = 5.2,
	AmazingGrace = 5.1,
	Collateral = 5,
	LureTiger = 4.9,
	KnownBoth = 4.9,
	FireAttack = 4.9,
	BurningCamps = 4.5
}

--弧顾：锁定技，其他角色不能弃置你装备区的武器牌或防具牌。
--无

--暴殴：其他角色的结束阶段开始时，若该角色于此回合内造成过伤害，你可以对其使用一张无视距离的【杀】。
sgs.ai_skill_cardask["@ikbaoou-slash"] = function(self, data, pattern, target, target2)
	for _, slash in ipairs(self:getCards("Slash")) do
		if self:isFriend(target2) and self:slashIsEffective(slash, target2) then
			if self:findLeijiTarget(target2, 50, self.player) then return slash:toString() end
			if self:getDamagedEffects(target2, self.player, true) then return slash:toString() end
		end

		local nature = sgs.DamageStruct_Normal
		if slash:isKindOf("FireSlash") then nature = sgs.DamageStruct_Fire
		elseif slash:isKindOf("ThunderSlash") then nature = sgs.DamageStruct_Thunder end
		if self:isEnemy(target2) and self:slashIsEffective(slash, target2) and self:canAttack(target2, self.player, nature)
			and not self:getDamagedEffects(target2, self.player, true) and not self:findLeijiTarget(target2, 50, self.player) then
			return slash:toString()
		end
	end
	return "."
end

--夜华：觉醒技，当你造成伤害后，若你已受伤，你须减少1点体力上限，然后获得技能“星雨”。
--无

--星雨：出牌阶段限一次，你可以选择一种牌的类别或颜色，然后亮出牌堆顶的一张牌，若此牌不为该类别或颜色，你重复此流程。你令一名角色获得最后亮出的牌，然后将其余的牌置入弃牌堆。
local ikxingyu_skill = {}
ikxingyu_skill.name = "ikxingyu"
table.insert(sgs.ai_skills, ikxingyu_skill)
ikxingyu_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("IkXingyuCard") then return sgs.Card_Parse("@IkXingyuCard=.") end
end

sgs.ai_skill_use_func.IkXingyuCard = function(card, use, self)
	self.ikxingyu_choice = nil
	self.ikxingyu_enemy = nil
	sgs.ai_use_priority.IkXingyuCard = 9.5
	self:sort(self.friends)
	local use_card
	for _, friend in ipairs(self.friends) do
		if not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then
			if self:isWeak(friend) then self.ikxingyu_choice = "basic" end
			use_card = true
		end
	end
	if use_card then
		use.card = card
		return
	end

	sgs.ai_use_priority.IkXingyuCard = 0.5
	for _, enemy in ipairs(self.enemies) do
		if not hasManjuanEffect(enemy) and self:needKongcheng(enemy, true) then
			self.ikxingyu_choice = "equip"
			self.ikxingyu_enemy = enemy
			use.card = card
			return
		end
	end
end

sgs.ai_use_priority.IkXingyuCard = 9.5

sgs.ai_skill_choice.ikxingyu = function(self, choices)
	if self.ikxingyu_choice then return self.ikxingyu_choice end
	for _, friend in ipairs(self.friends) do
		if not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then
			if friend:getHandcardNum() < 3 and friend:getEquips():length() < 2 then return "equip" end
		end
	end
	for _, friend in ipairs(self.friends) do
		if not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then
			if friend:hasSkills("jizhi|ikhuiquan|jilve") then return "trick" end
		end
	end
	local rand = math.random(0, 100)
	if rand > 60 then return "trick"
	elseif rand > 35 then return "red"
	elseif rand > 10 then return "equip"
	elseif rand > 2 then return "basic"
	else return "black"
	end
end

sgs.ai_skill_playerchosen.ikxingyu = function(self, targets)
	if self.ikxingyu_enemy then return self.ikxingyu_enemy end

	local id = self.player:getMark("ikxingyu")
	local card = sgs.Sanguosha:getCard(id)
	local cards = { card }
	local c, friend = self:getCardNeedPlayer(cards, self.friends)
	if friend then return friend end

	self:sort(self.friends)
	for _, friend in ipairs(self.friends) do
		if self:isValuableCard(card, friend) and not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then return friend end
	end
	for _, friend in ipairs(self.friends) do
		if self:isWeak(friend) and not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then return friend end
	end
	local trash = card:isKindOf("EquipCard") or card:isKindOf("Disaster") or card:isKindOf("GodSalvation") or card:isKindOf("AmazingGrace") or card:isKindOf("Slash")
	if trash then
		for _, enemy in ipairs(self.enemies) do
			if self:doNotDraw(enemy, 1) and not hasManjuanEffect(enemy) and not isCard("Peach", card, enemy) and not isCard("Analeptic", card, enemy) then return enemy end
		end
		for _, enemy in ipairs(self.enemies) do
			if enemy:getPhase() > sgs.Player_Play and self:needKongcheng(enemy, true) and not hasManjuanEffect(enemy) then return enemy end
		end
	end
	for _, friend in ipairs(self.friends) do
		if not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then return friend end
	end
end

--娇蛮：每当你受到一次伤害后，你可以将对你造成伤害的牌交给除伤害来源外的一名角色。
sgs.ai_skill_playerchosen.ikjiaoman = function(self, targets)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	local card = damage.card
	local cards = { card }
	local friends = {}
	for _, p in sgs.qlist(targets) do
		if self:isFriend(p) then
			table.insert(friends, p)
		end
	end
	local _, friend = self:getCardNeedPlayer(cards, friends)
	if friend then return friend end
	if targets:contains(self.player) and not self:needKongcheng(self.player, true) then return self.player end
	if #friends > 0 then
		self:sort(friends)
		return friends[1]
	end
	return nil
end

--唤卫：君主技，当你需要使用或打出一张【闪】时，你可以令其他花势力角色打出一张【闪】（视为由你使用或打出）。
table.insert(sgs.ai_global_flags, "ikhuanweisource")

sgs.ai_skill_invoke.ikhuanwei = function(self, data)
	local asked = data:toStringList()
	local prompt = asked[2]
	if self:askForCard("jink", prompt, 1) == "." then return false end

	local cards = self.player:getHandcards()
	if sgs.ikhuanweisource then return false end
	for _, friend in ipairs(self.friends_noself) do
		if friend:getKingdom() == "hana" and self:hasEightDiagramEffect(friend) then return true end
	end

	local current = self.room:getCurrent()
	if self:isFriend(current) and current:getKingdom() == "hana" and self:getOverflow(current) > 2 then
		return true
	end

	for _, card in sgs.qlist(cards) do
		if isCard("Jink", card, self.player) then
			return false
		end
	end
	local lieges = self.room:getLieges("hana", self.player)
	if lieges:isEmpty() then return end
	local has_friend = false
	for _, p in sgs.qlist(lieges) do
		if not self:isEnemy(p) then
			has_friend = true
			break
		end
	end
	return has_friend
end

sgs.ai_choicemade_filter.skillInvoke.ikhuanwei = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		sgs.ikhuanweisource = player
	end
end

function sgs.ai_slash_prohibit.ikhuanwei(self, from, to)
	if not to:hasLordSkill("ikhuanwei") then return false end
	if self:isFriend(to, from) then return false end
	local guojia = self.room:findPlayerBySkillName("tiandu")
	if guojia and guojia:getKingdom() == "hana" and self:isFriend(to, guojia) then return sgs.ai_slash_prohibit.tiandu(self, from, guojia) end
end

sgs.ai_choicemade_filter.cardResponded["@ikhuanwei-jink"] = function(self, player, promptlist)
	if promptlist[#promptlist] ~= "_nil_" then
		sgs.updateIntention(player, sgs.ikhuanweisource, -80)
		sgs.ikhuanweisource = nil
	elseif sgs.ikhuanweisource then
		if self:isFriend(player, sgs.ikhuanweisource) then sgs.card_lack[player:objectName()]["Jink"] = 1 end
		if player:objectName() == self.room:getLieges("hana", sgs.ikhuanweisource):last():objectName() then sgs.ikhuanweisource = nil end
	end
end

sgs.ai_skill_cardask["@ikhuanwei-jink"] = function(self)
	if not self:isFriend(sgs.ikhuanweisource) then return "." end
	if self:needBear() then return "." end
	local bgm_zhangfei = self.room:findPlayerBySkillName("dahe")
	if bgm_zhangfei and bgm_zhangfei:isAlive() and sgs.ikhuanweisource:hasFlag("dahe") then
		for _, card in ipairs(self:getCards("Jink")) do
			if card:getSuit() == sgs.Card_Heart then
				return card:toString()
			end
		end
		return "."
	end
	return self:getCardId("Jink") or "."
end

--天锁：每当一名角色的判定牌生效前，你可以打出一张牌代替之。
sgs.ai_skill_cardask["@iktiansuo-card"] = function(self, data)
	local judge = data:toJudge()

	if self.room:getMode():find("_mini_46") and not judge:isGood() then return "$" .. self.player:handCards():first() end
	if self:needRetrial(judge) then
		local cards = sgs.QList2Table(self.player:getCards("he"))
		for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
			cards:prepend(sgs.Sanguosha:getCard(id))
		end
		local card_id = self:getRetrialCardId(cards, judge)
		if card_id ~= -1 then
			return "$" .. card_id
		end
	end

	return "."
end

function sgs.ai_cardneed.iktiansuo(to, card, self)
	for _, player in sgs.qlist(self.room:getAllPlayers()) do
		if self:getFinalRetrial(to) == 1 then
			if player:containsTrick("lightning") and not player:containsTrick("YanxiaoCard") then
				return card:getSuit() == sgs.Card_Spade and card:getNumber() >= 2 and (card:getNumber() <= 9 or player:hasSkill("thjiuzhang")) and not player:hasSkills("hongyan|wuyan")
			end
			if self:isFriend(player) and self:willSkipDrawPhase(player) then
				return card:getSuit() == sgs.Card_Club
			end
			if self:isFriend(player) and self:willSkipPlayPhase(player) then
				return card:getSuit() == sgs.Card_Heart and (not player:hasSkill("thanyue") or player:hasSkill("ikchiqiu"))
			end
		end
	end
end

sgs.iktiansuo_suit_value = {
	heart = 3.9,
	club = 3.9,
	spade = 3.5
}

--幻姬：每当你受到1点伤害后，你可以获得伤害来源区域内的一张牌。
sgs.ai_skill_invoke.ikhuanji = function(self, data)
	local target = data:toPlayer()
	if sgs.ai_need_damaged.ikhuanji(self, target, self.player) then return true end

	if self:isFriend(target) then
		if self:getOverflow(target) > 2 then return true end
		return (target:hasSkills(sgs.lose_equip_skill) and target:hasEquip()) or self:needToThrowArmor(target) or (target:containsTrick("indulgence") or target:containsTrick("supply_shortage"))
	end
	if self:isEnemy(target) then				---ikhuanji without zhugeliang and luxun
		if target:containsTrick("purple_song") then return true end
		if target:hasSkills("ikyindie+ikguiyue") and target:getPhase() == sgs.Player_NotActive then return false end
		if (self:needKongcheng(target) or self:getLeastHandcardNum(target) == 1) and target:getHandcardNum() == 1 then
			if target:hasEquip() then
				return true
			else
				return false
			end
		end
	end
	return true
end

sgs.ai_skill_cardchosen.ikhuanji = function(self, who, flags)
	local suit = sgs.ai_need_damaged.ikhuanji(self, who, self.player)
	if not suit then return nil end

	local cards = sgs.QList2Table(who:getEquips())
	local handcards = sgs.QList2Table(who:getHandcards())
	if #handcards == 1 and handcards[1]:hasFlag("visible") then table.insert(cards, handcards[1]) end

	for i = 1, #cards, 1 do
		if (cards[i]:getSuit() == suit and suit ~= sgs.Card_Spade)
			or (cards[i]:getSuit() == suit and suit == sgs.Card_Spade and cards[i]:getNumber() >= 2 and cards[i]:getNumber() <= 9) then
			return cards[i]:getEffectiveId()
		end
	end
	return nil
end

sgs.ai_need_damaged.ikhuanji = function(self, attacker, player)
	if not attacker or not player:hasSkills("iktiansuo|nosguicai") then return false end
	local need_retrial = function(splayer)
		local alive_num = self.room:alivePlayerCount()
		return alive_num + splayer:getSeat() % alive_num > self.room:getCurrent():getSeat()
				and splayer:getSeat() < alive_num + self.player:getSeat() % alive_num
	end
	local retrial_card ={ ["spade"] = false, ["heart"] = false, ["club"] = false, ["diamond"] = false }
	local attacker_card ={ ["spade"] = nil, ["heart"]= nil, ["club"] = nil, ["diamond"] = nil }

	local handcards = sgs.QList2Table(player:getHandcards())
	for i = 1, #handcards, 1 do
		local flag = string.format("%s_%s_%s", "visible", attacker:objectName(), player:objectName())
		if player:objectName() == self.player:objectName() or handcards[i]:hasFlag("visible") or handcards[i]:hasFlag(flag) then
			if handcards[i]:getSuit() == sgs.Card_Spade and handcards[i]:getNumber() >= 2 and handcards[i]:getNumber() <= 9 then
				retrial_card.spade = true
			end
			if handcards[i]:getSuit() == sgs.Card_Heart then
				retrial_card.heart = true
			end
			if handcards[i]:getSuit() == sgs.Card_Club then
				retrial_card.club = true
			end
			if handcards[i]:getSuit() == sgs.Card_Diamond then
				retrial_card.diamond = true
			end
		end
	end

	local cards = sgs.QList2Table(attacker:getEquips())
	local handcards = sgs.QList2Table(attacker:getHandcards())
	if #handcards == 1 and handcards[1]:hasFlag("visible") then table.insert(cards, handcards[1]) end

	for i = 1, #cards, 1 do
		if cards[i]:getSuit() == sgs.Card_Spade and cards[i]:getNumber() >= 2 and cards[i]:getNumber() <= 9 then
			attacker_card.spade = sgs.Card_Spade
		end
		if cards[i]:getSuit() == sgs.Card_Heart then
			attacker_card.heart = sgs.Card_Heart
		end
		if cards[i]:getSuit() == sgs.Card_Club then
			attacker_card.club = sgs.Card_Club
		end
		if cards[i]:getSuit() == sgs.Card_Diamond then
			attacker_card.diamond = sgs.Card_Diamond
		end
	end

	local players = self.room:getOtherPlayers(player)
	for _, p in sgs.qlist(players) do
		if p:containsTrick("lightning") and self:getFinalRetrial(p) == 1 and need_retrial(p) then
			if not retrial_card.spade and attacker_card.spade then return attacker_card.spade end
		end

		if self:isFriend(p, player) and not p:containsTrick("YanxiaoCard") and not p:hasSkill("qiaobian") then
			if p:containsTrick("indulgence") and self:getFinalRetrial(p) == 1 and need_retrial(p) and p:getHandcardNum() >= p:getHp() then
				if not retrial_card.heart and attacker_card.heart then return attacker_card.heart end
			end
			if p:containsTrick("supply_shortage") and self:getFinalRetrial(p) == 1 and need_retrial(p) and p:hasSkill("yongsi") then
				if not retrial_card.club and attacker_card.club then return attacker_card.club end
			end
		end

		if self:isEnemy(p, player) and not p:containsTrick("YanxiaoCard") and not p:hasSkill("qiaobian") then
			if p:containsTrick("purple_song") and self:getFinalRetrial(p) == 1 and need_retrial(p) then
				if not retrial_card.diamond and attacker_card.diamond then return attacker_card.diamond end
			end
		end
	end
	return false
end

sgs.ai_choicemade_filter.skillInvoke.ikhuanji = function(self, player, promptlist)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if damage.from and damage.to and promptlist[#promptlist] ~= "yes" then
		if not self:doNotDiscard(damage.from, "hej") then
			sgs.updateIntention(damage.to, damage.from, -40)
		end
	end
end

sgs.ai_choicemade_filter.cardChosen.ikhuanji = function(self, player, promptlist)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if damage.from and damage.to then
		local id = tonumber(promptlist[3])
		local place = self.room:getCardPlace(id)
		if sgs.ai_need_damaged.ikhuanji(self, damage.from, damage.to) then
		elseif damage.from:getArmor() and self:needToThrowArmor(damage.from) and id == damage.from:getArmor():getEffectiveId() then
		elseif self:getOverflow(damage.from) > 2 or (damage.from:hasSkills(sgs.lose_equip_skill) and place == sgs.Player_PlaceEquip) then
		elseif place == sgs.Player_PlaceDelayedTrick then
			sgs.ai_choicemade_filter.cardChosen.snatch(self, player, promptlist)
		else
			sgs.updateIntention(damage.to, damage.from, 60)
		end
	end
end

--傲戾：每当你受到1点伤害后，可以进行一次判定，若结果为红色，你对伤害来源造成1点伤害；若结果为黑色，你弃置其区域内的一张牌。
sgs.ai_skill_invoke.ikaoli = function(self, data)
	local damage = data:toDamage()
	if not damage.from then
		local zhangjiao = self.room:findPlayerBySkillName("guidao")
		return zhangjiao and self:isFriend(zhangjiao) and not zhangjiao:isNude()
	end
	return not self:isFriend(damage.from) and self:canAttack(damage.from) and (damage.from:isNude() or not self:doNotDiscard(damage.from, "hej"))
end

sgs.ai_need_damaged.ikaoli = function(self, attacker, player)
	if not attacker then return false end
	if self:isEnemy(attacker) and self:isWeak(attacker) and sgs.isGoodTarget(attacker, self:getEnemies(attacker), self) then
		return true
	end
	return false
end

function sgs.ai_slash_prohibit.ikaoli(self, from, to)
	if not to:hasSkill("ikaoli") then return false end
	if self:isFriend(from, to) then return false end
	if from:hasSkill("ikxuwu") or from:getMark("thshenyou") > 0 or (from:hasSkill("ikwanhun") and from:distanceTo(to) == 1) then return false end
	if from:hasFlag("IkJieyouUsed") then return false end
	return self:isWeak(from)
end

sgs.ai_choicemade_filter.skillInvoke.ikaoli = function(self, player, promptlist)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if damage.from and damage.to then
		if promptlist[#promptlist] == "yes" then
			if not self:getDamagedEffects(damage.from, player) and not self:needToLoseHp(damage.from, player)
				and (damage.from:isNude() or not self:doNotDiscard(damage.from, "he")) then
				sgs.updateIntention(damage.to, damage.from, 40)
			end
		elseif self:canAttack(damage.from) then
			sgs.updateIntention(damage.to, damage.from, -40)
		end
	end
end

--清俭：当你于摸牌阶段外获得牌时，你可以将其中至少一张牌交给至少一名其他角色，每回合限一次。
sgs.ai_skill_askforyiji.ikqingjian = function(self, card_ids)
	return sgs.ai_skill_askforyiji.ikyumeng(self, card_ids)
end

--赤宝：摸牌阶段，你可少摸至少一张牌，并选择手牌数不少于你的X名其他角色，获得这些角色的各一张手牌（X为你以此法少摸的牌的数量）。
function SmartAI:getIkChibaoTargets(reason, isDummy)
	if self.player:getPile("yiji"):length() > 1 then return {} end

	reason = reason or "ikchibao"
	self:sort(self.enemies, "handcard")
	local upperlimit = (reason == "ikchibao") and self.player:getMark("ikchibao") or (reason == "koftuxi" and 1 or 2)
	local targets = {}

	local zhugeliang = self.room:findPlayerBySkillName("ikjingyou")
	local luxun = self.room:findPlayerBySkillName("lianying")
	local nosluxun = self.room:findPlayerBySkillName("noslianying")
	local dengai = self.room:findPlayerBySkillName("ikyindie")
	local jiangwei = self.room:findPlayerBySkillName("zhiji")

	local add_player = function(player, isfriend)
		if player:getHandcardNum() == 0 or player:objectName() == self.player:objectName() then return #targets end
		if reason == "ikchibao" and player:getHandcardNum() < self.player:getHandcardNum() then return #targets end
		if reason == "koftuxi" and player:getHandcardNum() <= self.player:getHandcardNum() then return #targets end
		if self:objectiveLevel(player) == 0 and player:isLord() and sgs.current_mode_players["rebel"] > 1 then return #targets end
		if #targets == 0 then
			table.insert(targets, player:objectName())
		elseif #targets > 0 then
			if not table.contains(targets, player:objectName()) then
				table.insert(targets, player:objectName())
			end
		end
		if not isDummy and isfriend and isfriend == 1 then
			self.player:setFlags("AI_IkChibaoToFriend_" .. player:objectName())
		end
		return #targets
	end

	local lord = self.room:getLord()
	if lord and self:isEnemy(lord) and sgs.turncount <= 1 and not lord:isKongcheng() then
		add_player(lord)
	end

	if jiangwei and self:isFriend(jiangwei) and jiangwei:getMark("zhiji") == 0 and jiangwei:getHandcardNum()== 1
		and self:getEnemyNumBySeat(self.player, jiangwei) <= (jiangwei:getHp() >= 3 and 1 or 0) then
		if add_player(jiangwei, 1) == upperlimit then return targets end
	end
	if dengai and dengai:hasSkill("ikguiyue") and self:isFriend(dengai) and (not self:isWeak(dengai) or self:getEnemyNumBySeat(self.player, dengai) == 0)
		and dengai:getMark("ikguiyue") == 0 and dengai:getPile("field"):length() == 2 and add_player(dengai, 1) == upperlimit then
		return targets
	end

	if zhugeliang and self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 1 and self:getEnemyNumBySeat(self.player, zhugeliang) > 0 then
		if zhugeliang:getHp() <= 2 then
			if add_player(zhugeliang, 1) == upperlimit then return targets end
		else
			local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), zhugeliang:objectName())
			local cards = sgs.QList2Table(zhugeliang:getHandcards())
			if #cards == 1 and (cards[1]:hasFlag("visible") or cards[1]:hasFlag(flag)) then
				if cards[1]:isKindOf("TrickCard") or cards[1]:isKindOf("Slash") or cards[1]:isKindOf("EquipCard") then
					if add_player(zhugeliang, 1) == upperlimit then return targets end
				end
			end
		end
	end

	if luxun and self:isFriend(luxun) and luxun:getHandcardNum() == 1 and self:getEnemyNumBySeat(self.player, luxun) > 0 then
		local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), luxun:objectName())
		local cards = sgs.QList2Table(luxun:getHandcards())
		if #cards == 1 and (cards[1]:hasFlag("visible") or cards[1]:hasFlag(flag)) then
			if cards[1]:isKindOf("TrickCard") or cards[1]:isKindOf("Slash") or cards[1]:isKindOf("EquipCard") then
				if add_player(luxun, 1) == upperlimit then return targets end
			end
		end
	end

	if nosluxun and self:isFriend(nosluxun) and nosluxun:getHandcardNum() == 1 and self:getEnemyNumBySeat(self.player, nosluxun) > 0 then
		local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), nosluxun:objectName())
		local cards = sgs.QList2Table(nosluxun:getHandcards())
		if #cards == 1 and (cards[1]:hasFlag("visible") or cards[1]:hasFlag(flag)) then
			if cards[1]:isKindOf("TrickCard") or cards[1]:isKindOf("Slash") or cards[1]:isKindOf("EquipCard") then
				if add_player(nosluxun, 1) == upperlimit then return targets end
			end
		end
	end

	for i = 1, #self.enemies, 1 do
		local p = self.enemies[i]
		local cards = sgs.QList2Table(p:getHandcards())
		local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), p:objectName())
		for _, card in ipairs(cards) do
			if (card:hasFlag("visible") or card:hasFlag(flag)) and (card:isKindOf("Peach") or card:isKindOf("Nullification") or card:isKindOf("Analeptic")) then
				if add_player(p) == upperlimit then return targets end
			end
		end
	end

	for i = 1, #self.enemies, 1 do
		local p = self.enemies[i]
		if p:hasSkills("jijiu|qingnang|xinzhan|leiji|jieyin|beige|kanpo|liuli|qiaobian|ikzhiheng|guidao|longhun|xuanfeng|tianxiang|noslijian|lijian") then
			if add_player(p) == upperlimit then return targets end
		end
	end

	for i = 1, #self.enemies, 1 do
		local p = self.enemies[i]
		local x = p:getHandcardNum()
		local good_target = true
		if x == 1 and p:hasSkills(sgs.need_kongcheng) then good_target = false end
		if x >= 2 and p:hasSkills("ikyindie+ikguiyue") then good_target = false end
		if good_target and add_player(p) == upperlimit then return targets end
	end

	if luxun and add_player(luxun, (self:isFriend(luxun) and 1 or nil)) == upperlimit then
		return targets
	end

	if dengai and self:isFriend(dengai) and (not self:isWeak(dengai) or self:getEnemyNumBySeat(self.player, dengai) == 0) and add_player(dengai, 1) == upperlimit then
		return targets
	end

	local others = self.room:getOtherPlayers(self.player)
	for _, other in sgs.qlist(others) do
		if self:objectiveLevel(other) >= 0 and other:hasSkills("ikyindie+ikguiyue") and add_player(other) == upperlimit then
			return targets
		end
	end

	if reason ~= "nostuxi" then return targets end

	for _, other in sgs.qlist(others) do
		if self:objectiveLevel(other) >= 0 and not other:hasSkills("ikyindie+ikguiyue") and add_player(other) == 1 and math.random(0, 5) <= 1 then
			return targets
		end
	end

	return {}
end

sgs.ai_skill_use["@@ikchibao"] = function(self, prompt)
	local targets = self:getIkChibaoTargets()
	if #targets > 0 then return "@IkChibaoCard=.->" .. table.concat(targets, "+") end
	return "."
end

sgs.ai_card_intention.IkChibaoCard = function(self, card, from, tos)
	local lord = self.room:getLord()
	if sgs.evaluatePlayerRole(from) == "neutral" and sgs.evaluatePlayerRole(tos[1]) == "neutral"
		and (not tos[2] or sgs.evaluatePlayerRole(tos[2]) == "neutral") and lord and not lord:isKongcheng()
		and not (lord:hasSkills("ikjingyou|zhiji") and lord:getHandcardNum() == 1)
		and self:hasLoseHandcardEffective(lord) and not lord:hasSkills("ikyindie+ikguiyue") and from:aliveCount() >= 4 then
		sgs.updateIntention(from, lord, -35)
		return
	end
	if from:getState() == "online" then
		for _, to in ipairs(tos) do
			if (to:hasSkills("ikjingyou|zhiji|lianying|noslianying") and to:getHandcardNum() == 1) or to:hasSkills("ikyindie+ikguiyue") then
			else
				sgs.updateIntention(from, to, 80)
			end
		end
	else
		for _, to in ipairs(tos) do
			local intention = from:hasFlag("AI_IkChibaoToFriend_" .. to:objectName()) and -5 or 80
			sgs.updateIntention(from, to, intention)
		end
	end
end

--裸衣：你可以跳过摸牌阶段，并亮出牌堆顶的三张牌，然后获得其中的基本牌、武器牌和【碎月绮斗】，再将其余的牌置入弃牌堆，若如此做，你因执行【杀】或【碎月绮斗】的效果而造成的伤害+1，直到你的下回合开始。
sgs.ai_skill_invoke.ikluoyi = function(self)
	if self.player:getPile("yiji"):length() > 1 then return false end
	local diaochan = self.room:findPlayerBySkillName("lijian") or self.room:findPlayerBySkillName("noslijian")
	if diaochan and self:isEnemy(diaochan) then
		for _, friend in ipairs(self.friends_noself) do
			if self:isWeak(friend) or friend:getHp() <= 2 then return false end
		end
	end
	return not self:isWeak()
end

function sgs.ai_cardneed.ikluoyi(to, card, self)
	local slash_num = 0
	local target
	local slash = sgs.cloneCard("slash")

	local cards = to:getHandcards()
	local need_slash = true
	for _, c in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s", "visible", self.room:getCurrent():objectName(), to:objectName())
		if c:hasFlag("visible") or c:hasFlag(flag) then
			if isCard("Slash", c, to) then
				need_slash = false
				break
			end
		end
	end

	self:sort(self.enemies, "defenseSlash")
	for _, enemy in ipairs(self.enemies) do
		if to:canSlash(enemy) and not self:slashProhibit(slash, enemy) and self:slashIsEffective(slash, enemy) and sgs.getDefenseSlash(enemy, self) <= 2 then
			target = enemy
			break
		end
	end

	if need_slash and target and isCard("Slash", card, to) then return true end
	return isCard("Duel", card, to)
end

sgs.ikluoyi_keep_value = {
	Peach = 6,
	Analeptic = 5.8,
	Jink = 5.2,
	Duel = 5.5,
	FireSlash = 5.6,
	Slash = 5.4,
	ThunderSlash = 5.5,
	Axe = 5,
	Blade = 4.9,
	Spear = 4.9,
	Fan = 4.8,
	KylinBow = 4.7,
	Halberd = 4.6,
	MoonSpear = 4.5,
	SPMoonSpear = 4.5,
	DefensiveHorse = 4
}

--天妒：在你的判定牌生效后，你可以获得此牌。
sgs.ai_skill_invoke.tiandu = function(self, data)
	if not to:hasSkill("tiandu") then return false end
	return not self:needKongcheng(self.player, true)
end

function sgs.ai_slash_prohibit.tiandu(self, from, to)
	if to:hasSkill("tiandu") and self:isEnemy(to, from) and self:hasEightDiagramEffect(to) then return true end
end

--羽梦：每当你受到1点伤害后，可观看牌堆顶的两张牌，将其中一张交给一名角色，然后将另一张交给一名角色。
sgs.ai_skill_invoke.ikyumeng = function(self)
	local sb_diaochan = self.room:getCurrent()
	if sb_diaochan and sb_diaochan:hasSkill("thjinlu") and not sb_diaochan:hasUsed("ThJinluCard") and not self:isFriend(sb_diaochan) and sb_diaochan:getPhase() == sgs.Player_Play then
		local invoke
		for _, friend in ipairs(self.friends) do
			if (not friend:isMale() or (friend:getHandcardNum() < friend:getHp() + 1 and sb_diaochan:faceUp())
				or (friend:getHandcardNum() < friend:getHp() - 2 and not sb_diaochan:faceUp())) and not self:needKongcheng(friend, true)
				and not self:isThJinluTarget(friend) then
				invoke = true
				break
			end
		end
		return invoke
	end
	return true
end

sgs.ai_skill_askforyiji.ikyumeng = function(self, card_ids)
	local Shenfen_user
	for _, player in sgs.qlist(self.room:getAllPlayers()) do
		if player:hasFlag("ShenfenUsing") then
			Shenfen_user = player
			break
		end
	end

	if self.player:getHandcardNum() <= 2 and not Shenfen_user then
		return nil, -1
	end

	local available_friends = {}
	for _, friend in ipairs(self.friends) do
		local insert = true
		if insert and hasManjuanEffect(friend) then insert = false end
		if insert and Shenfen_user and friend:objectName() ~= Shenfen_user:objectName() and friend:getHandcardNum() < 4 then insert = false end
		if insert and self:isThJinluTarget(friend) then insert = false end
		if insert then table.insert(available_friends, friend) end
	end

	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	local id = card_ids[1]

	local card, friend = self:getCardNeedPlayer(cards, self.friends)
	if card and friend and table.contains(available_friends, friend) then return friend, card:getId() end
	if #available_friends > 0 then
		self:sort(available_friends, "handcard")
		if Shenfen_user and table.contains(available_friends, Shenfen_user) then
			return Shenfen_user, id
		end
		for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
				return afriend, id
			end
		end
	end
	return nil, -1
end

sgs.ai_need_damaged.ikyumeng = function(self, attacker, player)
	local need_card = false
	local current = self.room:getCurrent()
	if current:hasWeapon("Crossbow") or current:hasSkill("ikyipao") or current:hasFlag("shuangxiong") then need_card = true end
	if current:hasSkills("jieyin|jijiu") and self:getOverflow(current) <= 0 then need_card = true end
	if self:isFriend(current, player) and need_card then return true end

	local friends = self:getFriends(player)
	self:sort(friends, "hp")

	if #friends > 0 and friends[1]:objectName() == player:objectName() and self:isWeak(player) and getCardsNum("Peach", player, attacker) == 0 then return false end
	if #friends > 1 and self:isWeak(friends[2]) then return true end

	return player:getHp() > 2 and sgs.turncount > 2 and #friends > 1
end

--濛漾：准备阶段开始时，你可以进行一次判定，若结果为黑色，你获得此牌，你可以重复此流程，直到出现红色的判定结果为止。
sgs.ai_skill_invoke.ikmengyang = function(self, data)
 	if self:willSkipPlayPhase() then
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
 	return true
end

--重岩：你可以将一张黑色手牌当【闪】使用或打出。
sgs.ai_view_as.ikzhongyan = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card:isBlack() and card_place == sgs.Player_PlaceHand then
		return ("jink:ikzhongyan[%s:%s]=%d"):format(suit, number, card_id)
	end
end

function sgs.ai_cardneed.ikzhongyan(to, card)
	return to:getCards("h"):length() < 2 and card:isBlack()
end

sgs.ikzhongyan_suit_value = {
	spade = 4.1,
	club = 4.2
}

--恂恂：摸牌阶段开始时，你可以放弃摸牌，改为观看牌堆顶的四张牌并获得其中的两张牌，然后将其余的牌以任意顺序置于牌堆底。
sgs.ai_skill_invoke.ikxunxun = true

--忘隙：每当你对其他角色造成1点伤害后，或受到其他角色造成的1点伤害后，你可以令你与该角色各摸一张牌。
function sgs.ai_skill_invoke.ikwangxi(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		return not self:needKongcheng(target, true) and not (hasManjuanEffect(self.player) and hasManjuanEffect(target))
	else
		if hasManjuanEffect(self.player) then return false end
		return self:needKongcheng(target, true) or hasManjuanEffect(target)
	end
end

sgs.ai_choicemade_filter.skillInvoke.ikwangxi = function(self, player, promptlist)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	local target = nil
	if damage.from and damage.from:objectName() == player:objectName() then
		target = damage.to
	elseif damage.to and damage.to:objectName() == player:objectName() then
		target = damage.from
	end
	if target and promptlist[#promptlist] == "yes" then
		if self:needKongcheng(target, true) then sgs.updateIntention(player, target, 10)
		elseif not hasManjuanEffect(target) and player:getState() == "robot" then sgs.updateIntention(player, target, -60)
		end
	end
end

--制衡：出牌阶段限一次，你可以弃置一至X张牌，然后摸等量的牌（X为你的体力上限）。
local ikzhiheng_skill = {}
ikzhiheng_skill.name = "ikzhiheng"
table.insert(sgs.ai_skills, ikzhiheng_skill)
ikzhiheng_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("IkZhihengCard") then
		return sgs.Card_Parse("@IkZhihengCard=.")
	end
end

sgs.ai_skill_use_func.IkZhihengCard = function(card, use, self)
	local unpreferedCards = {}
	local cards = sgs.QList2Table(self.player:getHandcards())

	if self.player:getHp() < 3 then
		local zcards = self.player:getCards("he")
		local use_slash, keep_jink, keep_analeptic, keep_weapon = false, false, false
		for _, zcard in sgs.qlist(zcards) do
			if not isCard("Peach", zcard, self.player) and not isCard("ExNihilo", zcard, self.player) then
				local shouldUse = true
				if isCard("Slash", zcard, self.player) and not use_slash then
					local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
					self:useBasicCard(zcard, dummy_use)
					if dummy_use.card then
						if keep_slash then shouldUse = false end
						if dummy_use.to then
							for _, p in sgs.qlist(dummy_use.to) do
								if p:getHp() <= 1 then
									shouldUse = false
									if self.player:distanceTo(p) > 1 then keep_weapon = self.player:getWeapon() end
									break
								end
							end
							if dummy_use.to:length() > 1 then shouldUse = false end
						end
						if not self:isWeak() then shouldUse = false end
						if not shouldUse then use_slash = true end
					end
				end
				if zcard:getTypeId() == sgs.Card_TypeTrick then
					local dummy_use = { isDummy = true }
					self:useTrickCard(zcard, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
				if zcard:getTypeId() == sgs.Card_TypeEquip and not self.player:hasEquip(zcard) then
					local dummy_use = { isDummy = true }
					self:useEquipCard(zcard, dummy_use)
					if dummy_use.card then shouldUse = false end
					if keep_weapon and zcard:getEffectiveId() == keep_weapon:getEffectiveId() then shouldUse = false end
				end
				if self.player:hasEquip(zcard) and zcard:isKindOf("Armor") and not self:needToThrowArmor() then shouldUse = false end
				if self.player:hasTreasure(zcard:objectName()) then shouldUse = false end
				if isCard("Jink", zcard, self.player) and not keep_jink then
					keep_jink = true
					shouldUse = false
				end
				if self.player:getHp() == 1 and isCard("Analeptic", zcard, self.player) and not keep_analeptic then
					keep_analeptic = true
					shouldUse = false
				end
				if shouldUse then table.insert(unpreferedCards, zcard:getId()) end
			end
		end
	end

	if #unpreferedCards == 0 then
		local use_slash_num = 0
		self:sortByKeepValue(cards)
		for _, card in ipairs(cards) do
			if card:isKindOf("Slash") then
				local will_use = false
				if use_slash_num <= sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, card) then
					local dummy_use = { isDummy = true }
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then
						will_use = true
						use_slash_num = use_slash_num + 1
					end
				end
				if not will_use then table.insert(unpreferedCards, card:getId()) end
			end
		end

		local num = self:getCardsNum("Jink") - 1
		if self.player:getArmor() then num = num + 1 end
		if num > 0 then
			for _, card in ipairs(cards) do
				if card:isKindOf("Jink") and num > 0 then
					table.insert(unpreferedCards, card:getId())
					num = num - 1
				end
			end
		end
		for _, card in ipairs(cards) do
			if (card:isKindOf("Weapon") and self.player:getHandcardNum() < 3) or card:isKindOf("OffensiveHorse")
				or self:getSameEquip(card, self.player) or card:isKindOf("Lightning") then
				table.insert(unpreferedCards, card:getId())
			end
		end

		if self.player:getWeapon() and self.player:getHandcardNum() < 3 then
			table.insert(unpreferedCards, self.player:getWeapon():getId())
		end

		if self:needToThrowArmor() then
			table.insert(unpreferedCards, self.player:getArmor():getId())
		end

		if self.player:getOffensiveHorse() and self.player:getWeapon() then
			table.insert(unpreferedCards, self.player:getOffensiveHorse():getId())
		end

		for _, card in ipairs(cards) do
			if card:isNDTrick() then
				local dummy_use = { isDummy = true }
				self:useTrickCard(card, dummy_use)
				if not dummy_use.card then table.insert(unpreferedCards, card:getId()) end
			end
		end
	end

	local use_cards = {}
	for index = #unpreferedCards, 1, -1 do
		if not self.player:isJilei(sgs.Sanguosha:getCard(unpreferedCards[index])) then table.insert(use_cards, unpreferedCards[index]) end
	end

	if #use_cards > 0 then	
		local n = self.player:getMaxHp()
		local subcards = {}
		for i = 1, #use_cards do
			table.insert(subcards, use_cards[i])
			if #subcards == n then
				break
			end
		end
		use.card = sgs.Card_Parse("@IkZhihengCard=" .. table.concat(subcards, "+"))
		return
	end
end

sgs.ai_use_value.IkZhihengCard = 9
sgs.ai_use_priority.IkZhihengCard = 2.61
sgs.dynamic_value.benefit.IkZhihengCard = true

function sgs.ai_cardneed.ikzhiheng(to, card)
	return not card:isKindOf("Jink")
end

--济援：君主技，锁定技，其他雪势力角色使用的【桃】指定你为目标后，回复的体力+1。
--无

--窥破：出牌阶段，你可以将一张黑色牌当【心网密葬】使用。
local ikkuipo_skill = {}
ikkuipo_skill.name = "ikkuipo"
table.insert(sgs.ai_skills, ikkuipo_skill)
ikkuipo_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local black_card
	self:sortByUseValue(cards, true)
	local has_weapon = false

	for _, card in ipairs(cards) do
		if card:isKindOf("Weapon") and card:isBlack() then has_weapon = true end
	end

	for _, card in ipairs(cards) do
		if card:isBlack() and ((self:getUseValue(card) < sgs.ai_use_value.Dismantlement) or inclusive or self:getOverflow() > 0) then
			local shouldUse = true
			if card:isKindOf("Armor") then
				if not self.player:getArmor() then shouldUse = false
				elseif self.player:hasEquip(card) and not self:needToThrowArmor() then shouldUse = false
				end
			end

			if card:isKindOf("Weapon") then
				if not self.player:getWeapon() then shouldUse = false
				elseif self.player:hasEquip(card) and not has_weapon then shouldUse = false
				end
			end

			if card:isKindOf("Slash") then
				local dummy_use = { isDummy = true }
				if self:getCardsNum("Slash") == 1 then
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
			end

			if self:getUseValue(card) > sgs.ai_use_value.Dismantlement and card:isKindOf("TrickCard") then
				local dummy_use = { isDummy = true }
				self:useTrickCard(card, dummy_use)
				if dummy_use.card then shouldUse = false end
			end

			if shouldUse then
				black_card = card
				break
			end

		end
	end

	if black_card then
		local suit = black_card:getSuitString()
		local number = black_card:getNumberString()
		local card_id = black_card:getEffectiveId()
		local card_str = ("dismantlement:ikkuipo[%s:%s]=%d"):format(suit, number, card_id)
		local dismantlement = sgs.Card_Parse(card_str)

		assert(dismantlement)

		return dismantlement
	end
end

sgs.ikkuipo_suit_value = {
	spade = 3.9,
	club = 3.9
}

function sgs.ai_cardneed.ikkuipo(to, card)
	return card:isBlack()
end

--诡思：限定技，当一名角色使用锦囊牌指定至少两名角色为目标时，你可以令此牌对其中的至少一名角色无效。
local function getIkGuisiValue(self, who, card, from)
	if not self:hasTrickEffective(card, who, from) then return 0 end
	if card:isKindOf("AOE") then
		if not self:isFriend(who) then return 0 end
		local value = self:getAoeValueTo(card, who, from)
		if value < 0 then return -value / 30 end
	elseif card:isKindOf("GodSalvation") then
		if not self:isEnemy(who) or not who:isWounded() or who:getHp() >= getBestHp(who) then return 0 end
		if self:isWeak(who) then return 1.2 end
		if who:hasSkills(sgs.masochism_skill) then return 1.0 end
		return 0.9
	elseif card:isKindOf("AmazingGrace") then
		if not self:isEnemy(who) or hasManjuanEffect(who) then return 0 end
		local v = 1.2
		local p = self.room:getCurrent()
		while p:objectName() ~= who:objectName() do
			v = v * 0.9
			p = p:getNextAlive()
		end
		return v
	end
	return 0
end

sgs.ai_skill_use["@@ikguisi"] = function(self, prompt)
	local liuxie = self.room:findPlayerBySkillName("huangen")
	if liuxie and self:isFriend(liuxie) and not self:isWeak(liuxie) then return "." end

	local card = self.player:getTag("ikguisi"):toCardUse().card
	local from = self.player:getTag("ikguisi"):toCardUse().from

	local players = sgs.QList2Table(self.room:getAllPlayers())
	self:sort(players, "defense")
	local target_table = {}
	local targetslist = self.player:property("ikguisi_targets"):toString():split("+")
	local value = 0
	for _, player in ipairs(players) do
		if not player:hasSkill("danlao") and table.contains(targetslist, player:objectName()) then
			local val = getIkGuisiValue(self, player, card, from)
			if val > 0 then
				value = value + val
				table.insert(target_table, player:objectName())
			end
		end
	end
	if #target_table == 0 or value / (self.room:alivePlayerCount() - 1) < 0.55 then return "." end
	return "@IkGuisiCard=.->" .. table.concat(target_table, "+")
end

sgs.ai_card_intention.IkGuisiCard = function(self, card, from, tos)
	local cardx = self.player:getTag("ikguisi"):toCardUse().card
	local from = self.player:getTag("ikguisi"):toCardUse().from
	if not cardx then return end
	local intention = (cardx:isKindOf("AOE") and -50 or 50)
	for _, to in ipairs(tos) do
		if to:hasSkill("danlao") or not self:hasTrickEffective(cardx, to, from) then continue end
		if cardx:isKindOf("GodSalvation") and not (to:isWounded() or to:isChained()) then continue end
		sgs.updateIntention(from, to, intention)
	end
end
