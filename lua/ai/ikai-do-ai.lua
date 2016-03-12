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

sgs.ai_use_value.IkShenaiCard = 8.5
sgs.ai_use_priority.IkShenaiCard = 8.8

sgs.ai_skill_askforyiji.ikshenai = sgs.ai_skill_askforyiji.nosyiji

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
