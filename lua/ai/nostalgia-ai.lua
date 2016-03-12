nosjujian_skill = {}
nosjujian_skill.name = "nosjujian"
table.insert(sgs.ai_skills, nosjujian_skill)
nosjujian_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("NosJujianCard") then return sgs.Card_Parse("@NosJujianCard=.") end
end

sgs.ai_skill_use_func.NosJujianCard = function(card, use, self)
	local abandon_card = {}
	local index = 0
	local hasPeach = (self:getCardsNum("Peach") > 0)
	local to

	local trick_num, basic_num, equip_num = 0, 0, 0
	if not hasPeach and self.player:isWounded() and self.player:getCards("he"):length() >=3 then
		local cards = self.player:getCards("he")
		cards = sgs.QList2Table(cards)
		self:sortByUseValue(cards, true)
		for _, card in ipairs(cards) do
			if card:getTypeId() == sgs.Card_TypeTrick and not isCard("ExNihilo", card, self.player) then trick_num = trick_num + 1
			elseif card:getTypeId() == sgs.Card_TypeBasic then basic_num = basic_num + 1
			elseif card:getTypeId() == sgs.Card_TypeEquip then equip_num = equip_num + 1
			end
		end
		local result_class
		if trick_num >= 3 then result_class = "TrickCard"
		elseif equip_num >= 3 then result_class = "EquipCard"
		elseif basic_num >= 3 then result_class = "BasicCard"
		end

		for _, fcard in ipairs(cards) do
			if fcard:isKindOf(result_class) and not isCard("ExNihilo", fcard, self.player) then
				table.insert(abandon_card, fcard:getId())
				index = index + 1
				if index == 3 then break end
			end
		end

		if index == 3 then
			to = self:findPlayerToDraw(false, 3)
			if not to then return end
			if use.to then use.to:append(to) end
			use.card = sgs.Card_Parse("@NosJujianCard=" .. table.concat(abandon_card, "+"))
			return
		end
	end

	abandon_card = {}
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	local slash_num = self:getCardsNum("Slash")
	local jink_num = self:getCardsNum("Jink")
	index = 0
	for _, card in ipairs(cards) do
		if index >= 3 then break end
		if card:isKindOf("TrickCard") and not card:isKindOf("Nullification") then
			table.insert(abandon_card, card:getId())
			index = index + 1
		elseif card:isKindOf("EquipCard") then
			table.insert(abandon_card, card:getId())
			index = index + 1
		elseif card:isKindOf("Slash") and slash_num > 1 then
			table.insert(abandon_card, card:getId())
			index = index + 1
			slash_num = slash_num - 1
		elseif card:isKindOf("Jink") and jink_num > 1 then
			table.insert(abandon_card, card:getId())
			index = index + 1
			jink_num = jink_num - 1
		end
	end

	if index == 3 then
		to = self:findPlayerToDraw(false, 3)
		if not to then return end
		if use.to then use.to:append(to) end
		use.card = sgs.Card_Parse("@NosJujianCard=" .. table.concat(abandon_card, "+"))
		return
	end

	if self:getOverflow() > 0 then
		local discard = self:askForDiscard("dummyreason", math.min(self:getOverflow(), 3), nil, false, true)
		to = self:findPlayerToDraw(false, math.min(self:getOverflow(), 3))
		if not to then return end
		use.card = sgs.Card_Parse("@NosJujianCard=" .. table.concat(discard, "+"))
		if use.to then use.to:append(to) end
		return
	end

	if index > 0 then
		to = self:findPlayerToDraw(false, index)
		if not to then return end
		use.card = sgs.Card_Parse("@NosJujianCard=" .. table.concat(abandon_card, "+"))
		if use.to then use.to:append(to) end
		return
	end
end

sgs.ai_use_priority.NosJujianCard = 0
sgs.ai_use_value.NosJujianCard = 6.7

sgs.ai_card_intention.NosJujianCard = -100

sgs.dynamic_value.benefit.NosJujianCard = true

sgs.ai_skill_choice.nosxuanfeng = function(self, choices)
	self:sort(self.enemies, "defense")
	local slash = sgs.cloneCard("slash")
	for _, enemy in ipairs(self.enemies) do
		if self.player:distanceTo(enemy) <= 1 then
			return "damage"
		elseif not self:slashProhibit(slash, enemy) and self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then
			return "slash"
		end
	end
	return "nothing"
end

sgs.ai_skill_playerchosen.nosxuanfeng_damage = sgs.ai_skill_playerchosen.damage
sgs.ai_skill_playerchosen.nosxuanfeng_slash = sgs.ai_skill_playerchosen.zero_card_as_slash

sgs.ai_playerchosen_intention.nosxuanfeng_damage = 80
sgs.ai_playerchosen_intention.nosxuanfeng_slash = 80

sgs.nosxuanfeng_keep_value = sgs.xiaoji_keep_value
sgs.ai_cardneed.nosxuanfeng = sgs.ai_cardneed.equip

sgs.ai_skill_invoke.nosshangshi = sgs.ai_skill_invoke.shangshi

sgs.ai_view_as.nosgongqi = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place ~= sgs.Player_PlaceSpecial and card:getTypeId() == sgs.Card_TypeEquip and not card:hasFlag("using") then
		return ("slash:nosgongqi[%s:%s]=%d"):format(suit, number, card_id)
	end
end

local nosgongqi_skill = {}
nosgongqi_skill.name = "nosgongqi"
table.insert(sgs.ai_skills, nosgongqi_skill)
nosgongqi_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(getWoodenOxPile(self.player)) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)

	local equip_card
	self:sortByUseValue(cards, true)

	for _, card in ipairs(cards) do
		if card:getTypeId() == sgs.Card_TypeEquip and (self:getUseValue(card) < sgs.ai_use_value.Slash or inclusive) then
			equip_card = card
			break
		end
	end

	if equip_card then
		local suit = equip_card:getSuitString()
		local number = equip_card:getNumberString()
		local card_id = equip_card:getEffectiveId()
		local card_str = ("slash:nosgongqi[%s:%s]=%d"):format(suit, number, card_id)
		local slash = sgs.Card_Parse(card_str)

		assert(slash)

		return slash
	end
end


function sgs.ai_cardneed.nosgongqi(to, card, self)
	return card:getTypeId() == sgs.Card_TypeEquip and getKnownCard(to, self.player, "EquipCard", true) == 0
end

function sgs.ai_cardsview_valuable.ikjieyou(self, class_name, player)
	if class_name == "Peach" and not player:hasFlag("Global_IkJieyouFailed") then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying then return nil end
		local current = player:getRoom():getCurrent()
		if not current or current:isDead() or current:getPhase() == sgs.Player_NotActive
			or current:objectName() == player:objectName() or (current:hasSkill("wansha") and player:objectName() ~= dying:objectName())
			or (self:isEnemy(current) and self:findLeijiTarget(current, 50, player)) then return nil end
		return "@IkJieyouCard=."
	end
end

sgs.ai_card_intention.IkJieyouCard = sgs.ai_card_intention.Peach

sgs.ai_skill_cardask["ikjieyou-slash"] = function(self, data, pattern, target)
	if self:isEnemy(target) and self:findLeijiTarget(target, 50, self.player) then return "." end
	for _, slash in ipairs(self:getCards("Slash")) do
		if self:slashIsEffective(slash, target) then
			return slash:toString()
		end
	end
	return "."
end

function sgs.ai_cardneed.ikjieyou(to, card, self)
	return isCard("Slash", card, to) and getKnownCard(to, self.player, "Slash", true) == 0
end

sgs.ai_skill_invoke.nosfuhun = function(self, data)
	local target = 0
	for _, enemy in ipairs(self.enemies) do
		if self.player:inMyAttackRange(enemy) then target = target + 1 end
	end
	return target > 0 and not self.player:isSkipped(sgs.Player_Play)
end

sgs.ai_skill_invoke.noszhenlie = function(self, data)
	local judge = data:toJudge()
	if not judge:isGood() then
	return true end
	return false
end

sgs.ai_skill_playerchosen.nosmiji = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	local n = self.player:getLostHp()
	if self.player:getPhase() == sgs.Player_Start then
		if self.player:getHandcardNum() - n < 2 and not self:needKongcheng() and not self:willSkipPlayPhase() then return self.player end
	elseif self.player:getPhase() == sgs.Player_Finish then
		if self.player:getHandcardNum() - n < 2 and not self:needKongcheng() then return self.player end
	end
	local to = self:findPlayerToDraw(true, n)
	return to or self.player
end

sgs.ai_playerchosen_intention.nosmiji = function(self, from, to)
	if not (self:needKongcheng(to, true) and from:getLostHp() == 1)
		and not hasManjuanEffect(to) then
		sgs.updateIntention(from, to, -80)
	end
end

sgs.ai_skill_invoke.ikwanhun = function(self, data)
	local damage = data:toDamage()
	local target = damage.to
	if self:isFriend(target) then return false end
	if target:getLostHp() >= 2 and target:getHp() <= 1 then return false end
	if target:hasSkills(sgs.masochism_skill .. "|" .. sgs.recover_skill .. "|longhun|buqu|nosbuqu") then return true end
	if self:hasHeavySlashDamage(self.player, damage.card, target) then return false end
	return (target:getMaxHp() - target:getHp()) < 2
end

function sgs.ai_cardneed.ikwanhun(to, card, self)
	return isCard("Slash", card, to) and getKnownCard(to, self.player, "Slash", true) == 0
end

sgs.ai_skill_invoke.noschengxiang = sgs.ai_skill_invoke.chengxiang
sgs.ai_skill_askforag.noschengxiang = sgs.ai_skill_askforag.chengxiang

function sgs.ai_cardsview_valuable.nosrenxin(self, class_name, player)
	if class_name == "Peach" and not player:isKongcheng() then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or self:isEnemy(dying, player) or dying:objectName() == player:objectName() then return nil end
		if hasManjuanEffect(dying) then
			local peach_num = 0
			if player:getMark("Global_PreventPeach") == 0 then
				for _, c in sgs.qlist(player:getCards("he")) do
					if isCard("Peach", c, player) then peach_num = peach_num + 1 end
					if peach_num > 1 then return nil end
				end
			end
		end
		if self:playerGetRound(dying) < self:playerGetRound(self.player) and dying:getHp() < 0 then return nil end
		if not player:faceUp() then
			if player:getHp() < 2 and (getCardsNum("Jink", player, self.player) > 0 or getCardsNum("Analeptic", player, self.player) > 0) and not dying:isLord() then return nil end
			return "@RenxinCard=."
		else
			if dying:getMark("Global_PreventPeach") == 0 then
				for _, c in sgs.qlist(player:getHandcards()) do
					if not isCard("Peach", c, player) then return nil end
				end
			end
			return "@NosRenxinCard=."
		end
		return nil
	end
end

function sgs.ai_cardsview.nosrenxin(self, class_name, player)
	if class_name == "Peach" and not player:isKongcheng() then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if not dying or self:isEnemy(dying, player) or dying:objectName() == player:objectName() then return nil end
		if player:getHp() < 2 and (getCardsNum("Jink", player, self.player) > 0 or getCardsNum("Analeptic", player, self.player) > 0) and not dying:isLord() then return nil end
		if not self:isWeak(player) then return "@RenxinCard=." end
		return nil
	end
end

sgs.ai_card_intention.NosRenxinCard = sgs.ai_card_intention.Peach

sgs.ai_skill_invoke.nosdanshou = function(self, data)
	local damage = data:toDamage()
	local phase = self.player:getPhase()
	if phase < sgs.Player_Play then
		return self:willSkipPlayPhase()
	elseif phase == sgs.Player_Play then
		if self.player:isChained() and (damage.chain or self.room:getTag("is_chained"):toInt() > 0) and self:isGoodChainTarget(self.player) then
			return false
		elseif self:getOverflow() >= 2 then
			return true
		else
			if damage.chain or self.room:getTag("is_chained"):toInt() > 0 then
				local nextp
				for _, p in sgs.qlist(self.room:getAllPlayers()) do
					if p:isChained() and self:damageIsEffective(p, damage.nature, self.player) then
						nextp = p
						break
					end
				end
				return not nextp or self:isFriend(nextp)
			end
			if damage.card and damage.card:isKindOf("Slash") and self:getCardsNum("Slash") >= 1 and self:slashIsAvailable() then
				return false
			end
			if (damage.card and damage.card:isKindOf("AOE")) or (self.player:hasFlag("ShenfenUsing") and self.player:faceUp()) then
				if damage.to:getNextAlive():objectName() == self.player:objectName() then return true
				else
					local dmg_val = 0
					local p = damage.to
					repeat
						if self:damageIsEffective(p, damage.nature, self.player) then
							if self:isFriend(p) then
								dmg_val = dmg_val + 1
							else
								if self:cantbeHurt(p, self.player, damage.damage) then dmg_val = dmg_val + 1 end
								if self:getDamagedEffects(p, self.player) then dmg_val = dmg_val + 0.5 end
								if self:isEnemy(p) then dmg_val = dmg_val - 1 end
							end
						end
						p = p:getNextAlive()
					until p:objectName() == self.player:objectName()
					return dmg_val >= 1.5
				end
			end
			if damage.to:hasSkills(sgs.masochism_skill .. "|ikzhichi|zhiyu|thzhehui") then return self:isEnemy(damage.to) end
			return true
		end
	elseif phase > sgs.Player_Play and phase ~= sgs.Player_NotActive then
		return true
	elseif phase == sgs.Player_NotActive then
		local current = self.room:getCurrent()
		if not current or not current:isAlive() or current:getPhase() == sgs.Player_NotActive then return true end
		if self:isFriend(current) then
			if self:getOverflow(current) >= 2 then return true end
			if current:getHandcardNum() < 2 then return self:isEnemy(damage.to) and damage.to:hasSkills(sgs.masochism_skill) end
			return false
		else
			if self:getOverflow(current) <= 2 then
				return true
			else
				local threat = getCardsNum("Duel", current, self.player) + getCardsNum("AOE", current, self.player)
				if self:slashIsAvailable(current) and getCardsNum("Slash", current, self.player) > 0 then threat = threat + math.min(1, getCardsNum("Slash", current, player)) end
				return threat >= 1
			end
		end
	end
	return false
end

sgs.ai_skill_invoke.nosjuece = function(self, data)
	local move = data:toMoveOneTime()
	if not move.from then return false end
	local from = findPlayerByObjectName(self.room, move.from:objectName())
	return from and ((self:isFriend(from) and self:getDamagedEffects(from, self.player)) or (not self:isFriend(from) and self:canAttack(from)))
end

sgs.ai_skill_playerchosen.nosmieji = function(self, targets) -- extra target for Ex Nihilo
	return self:findPlayerToDraw(false, 2)
end

sgs.ai_playerchosen_intention.nosmieji = -50

sgs.ai_skill_use["@@nosmieji"] = function(self, prompt) -- extra target for Collateral
	local collateral = sgs.cloneCard("collateral", sgs.Card_NoSuitBlack)
	local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {} }
	dummy_use.current_targets = self.player:property("extra_collateral_current_targets"):toString():split("+")
	self:useCardCollateral(collateral, dummy_use)
	if dummy_use.card and dummy_use.to:length() == 2 then
		local first = dummy_use.to:at(0):objectName()
		local second = dummy_use.to:at(1):objectName()
		return "@ExtraCollateralCard=.->" .. first .. "+" .. second
	end
end

local function getNosFenchengValue(self, player)
	if not self:damageIsEffective(player, sgs.DamageStruct_Fire, self.player) then return 0 end
	if not player:canDiscard(player, "he") then return self:isWeak(player) and 1.5 or 1 end
	if self.player:hasSkill("nosjuece") and self:isEnemy(player)
		and player:getEquips():isEmpty() and player:getHandcardNum() == 1 and not self:needKongcheng(player)
		and not (player:isChained() or not self:isGoodChainTarget(player, self.player, sgs.DamageStruct_Fire)) then return self:isWeak(player) and 1.5 or 1 end
	if self:isGoodChainTarget(player, self.player, sgs.DamageStruct_Fire) or self:getDamagedEffects(player, self.player) or self:needToLoseHp(player, self.player) then return -0.1 end

	local num = player:getEquips():length() - player:getHandcardNum()
	if num < 0 then
		if self:needToThrowArmor(player) then num = 1 else num = 0 end
	elseif num == 0 then
		num = 1
	end
	local equip_table = {}
	local needToTA = self:needToThrowArmor(player)
	if needToTA then table.insert(equip_table, 1) end
	if player:getOffensiveHorse() then table.insert(equip_table, 3) end
	if player:getWeapon() then table.insert(equip_table, 0) end
	if player:getDefensiveHorse() then table.insert(equip_table, 2) end
	if player:getArmor() and not needToTA then table.insert(equip_table, 1) end

	local value = 0
	for i = 1, num, 1 do
		local index = equip_table[i]
		if index == 0 then value = value + 0.4
		elseif index == 1 then
			value = value + (needToTA and -0.5 or 0.8)
		elseif index == 2 then value = value + 0.7
		elseif index == 3 then value = value + 0.3 end
	end
	if player:hasSkills("kofxiaoji|xiaoji") then value = value - 0.8 * num end
	if player:hasSkills("xuanfeng|nosxuanfeng") and num > 0 then value = value - 0.8 end

	local handcard = player:getHandcardNum() - num
	value = value + 0.1 * handcard
	if self:needKongcheng(player) or self:getLeastHandcardNum(player) > num then value = value - 0.15
	elseif num == 0 then value = value + 0.1 end
	return value
end

nosfencheng_skill = {}
nosfencheng_skill.name = "nosfencheng"
table.insert(sgs.ai_skills, nosfencheng_skill)
nosfencheng_skill.getTurnUseCard = function(self)
	if self.player:getMark("@nosburn") <= 0 or sgs.turncount <= 1 then return end
	local lord = self.room:getLord()
	if (self.role == "loyalist" or self.role == "renegade") and (sgs.isLordInDanger() or (lord and self:isWeak(lord))) then return end
	local value = 0
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self:isFriend(player) then value = value - getFenchengValue(self, player)
		elseif self:isEnemy(player) then value = value + getFenchengValue(self, player) end
	end
	if #self.friends_noself >= #self.enemies and value > 0 then return sgs.Card_Parse("@NosFenchengCard=.") end
	local ratio = value / (#self.enemies - #self.friends_noself)
	if ratio >= 0.4 then return sgs.Card_Parse("@NosFenchengCard=.") end
end

sgs.ai_skill_use_func.NosFenchengCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_discard.nosfencheng = function(self, discard_num, min_num, optional, include_equip)
	if discard_num == 1 and self:needToThrowArmor() then return { self.player:getArmor():getEffectiveId() } end
	local liru = self.room:getCurrent()
	local nosjuece_effect
	if liru and liru:isAlive() and liru:hasSkill("nosjuece") then nosjuece_effect = true end
	if not self:damageIsEffective(self.player, sgs.DamageStruct_Fire, liru) then return {} end
	if nosjuece_effect and self:isEnemy(liru) and self.player:getEquips():isEmpty() and self.player:getHandcardNum() == 1 and not self:needKongcheng()
		and not (self.player:isChained() or not self:isGoodChainTarget(self.player, liru, sgs.DamageStruct_Fire)) then return {} end
	if self:isGoodChainTarget(self.player, liru, sgs.DamageStruct_Fire) or self:getDamagedEffects(self.player, liru) or self:needToLoseHp(self.player, liru) then return {} end
	local to_discard = self:askForDiscard("dummyreason", discard_num, min_num, false, include_equip)
	if #to_discard < discard_num then return {} end
	for _, id in ipairs(to_discard) do
		local c = sgs.Sanguosha:getCard(id)
		if isCard("Peach", c, self.player) then return {}
		elseif self.player:getHp() == 1 and isCard("Analeptic", c, self.player) then return {}
		end
	end
 
	if not nosjuece_effect then return to_discard
	else
		if self.player:isKongcheng() then return to_discard end
		for _, id in sgs.qlist(self.player:handCards()) do
			if not table.contains(to_discard, id) then return to_discard end
		end
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards, true)
		local wep_id, arm_id, def_id, off_id
		if self.player:getWeapon() then wep_id = self.player:getWeapon():getEffectiveId() end
		if self.player:getArmor() then arm_id = self.player:getArmor():getEffectiveId() end
		if self.player:getDefensiveHorse() then def_id = self.player:getDefensiveHorse():getEffectiveId() end
		if self.player:getOffensiveHorse() then off_id = self.player:getOffensiveHorse():getEffectiveId() end
		if self:needToThrowArmor() and not table.contains(to_discard, arm_id) then table.insert(to_discard, arm_id)
		else
			if self.player:getOffensiveHorse() and not table.contains(to_discard, off_id) then table.insert(to_discard, off_id)
			elseif self.player:getWeapon() and not table.contains(to_discard, wep_id) then table.insert(to_discard, wep_id)
			elseif self.player:getDefensiveHorse() and not table.contains(to_discard, def_id) then
				if self:isWeak() then table.insert(to_discard, def_id)
				else return {} end
			elseif self.player:getArmor() and not table.contains(to_discard, arm_id) then
				if self:isWeak() or (not liru:hasSkill("ikxuwu") and (self.player:hasArmorEffect("vine") or self.player:getMark("@gale") > 0)) then table.insert(to_discard, arm_id)
				else return {} end
			end
			if #to_discard == discard_num + 1 then table.removeOne(to_discard, cards[1]:getEffectiveId()) end
			return to_discard
		end
	end
end

sgs.ai_skill_invoke.noszhuikong = function(self, data)
	if self.player:getHandcardNum() <= (self:isWeak() and 3 or 1) then return false end
	local current = self.room:getCurrent()
	if not current or self:isFriend(current) then return false end

	local max_card = self:getMaxCard()
	local max_point = max_card:getNumber()
	if self.player:hasSkill("yingyang") then max_point = math.min(max_point + 3, 13) end
	if not (current:hasSkill("zhiji") and current:getMark("zhiji") == 0 and current:getHandcardNum() == 1) then
		local enemy_max_card = self:getMaxCard(current)
		local enemy_max_point = enemy_max_card and enemy_max_card:getNumber() or 100
		if enemy_max_card and enemy:hasSkill("yingyang") then enemy_max_point = math.min(enemy_max_point + 3, 13) end
		if max_point > enemy_max_point or max_point > 10 then
			self.noszhuikong_card = max_card:getEffectiveId()
			return true
		end
	end
	if current:distanceTo(self.player) == 1 and not self:isValuableCard(max_card) then
		self.noszhuikong_card = max_card:getEffectiveId()
		return true
	end
	return false
end

sgs.ai_skill_playerchosen.nosqiuyuan = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	self:sort(targetlist, "handcard")
	local enemy
	for _, p in ipairs(targetlist) do
		if self:isEnemy(p) and not (p:getHandcardNum() == 1 and (p:hasSkill("ikjingyou") or (p:hasSkill("zhiji") and p:getMark("zhiji") == 0))) then
			if p:hasSkills(sgs.cardneed_skill) then return p
			elseif not enemy and not self:canLiuli(p, self.friends_noself) then enemy = p end
		end
	end
	if enemy then return enemy end
	targetlist = sgs.reverse(targetlist)
	local friend
	for _, p in ipairs(targetlist) do
		if self:isFriend(p) then
			if (p:hasSkill("ikjingyou") and p:getHandcardNum() == 1) or (p:getCardCount() >= 2 and self:canLiuli(p, self.enemies)) then return p
			elseif not friend and getCardsNum("Jink", p, player) >= 1 then friend = p end
		end
	end
	return friend
end

sgs.ai_skill_cardask["@nosqiuyuan-give"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		local e_card = sgs.Sanguosha:getEngineCard(card:getEffectiveId())
		if e_card:isKindOf("Jink")
			and not (target and target:isAlive() and target:hasSkill("wushen") and (e_card:getSuit() == sgs.Card_Heart or (target:hasSkill("hongyan") and e_card:getSuit() == sgs.Card_Spade))) then
			return "$" .. card:getEffectiveId()
		end
	end
	for _, card in ipairs(cards) do
		if not self:isValuableCard(card) and self:getKeepValue(card) < 5 then return "$" .. card:getEffectiveId() end
	end
	return "$" .. cards[1]:getEffectiveId()
end

function sgs.ai_slash_prohibit.nosqiuyuan(self, from, to)
	if not to:hasSkill("nosqiuyuan") then return false end
	if self:isFriend(to, from) then return false end
	if from:hasFlag("IkJieyouUsed") then return false end
	for _, friend in ipairs(self:getFriendsNoself(from)) do
		if not to:isKongcheng() and not (to:getHandcardNum() == 1 and (to:hasSkill("ikjingyou") or (to:hasSkill("zhiji") and to:getMark("zhiji") == 0))) then return true end
	end
end

sgs.ai_skill_invoke.noszhenggong = function(self, data)
	local target = data:toPlayer()

	if target:getCards("e"):length() == 1 and target:getArmor() and self.player:hasSkills("bazhen|yizhong|bossmanjia") then return false end
	if target:hasSkills(sgs.lose_equip_skill) and not (self:isFriend(target) and not self:isWeak(target)) then return false end
	local benefit = (target:getCards("e"):length() == 1 and target:getArmor() and self:needToThrowArmor(target))
	if not self:isFriend(target) then benefit = not benefit end
	if not benefit then return false end

	for i = 0, 3 do
		if not self.player:getEquip(i) and target:getEquip(i) and not (i == 1 and self.player:hasSkills("bazhen|yizhong|bossmanjia")) then
			return true
		end
	end
	if target:getArmor() and self:evaluateArmor(target:getArmor()) >= self:evaluateArmor(self.player:getArmor()) then return true end
	if target:getDefensiveHorse() or target:getOffensiveHorse() then return true end
	if target:getWeapon() and self:evaluateWeapon(target:getWeapon()) >= self:evaluateWeapon(self.player:getWeapon()) then return true end

	return false
end

function sgs.ai_cardneed.noszhenggong(to, card, self)
	if not to:containsTrick("indulgence") and to:getMark("nosbaijiang") == 0 then
		return card:getTypeId() == sgs.Card_TypeEquip
	end
end

sgs.ai_skill_cardchosen.noszhenggong = function(self, who, flags)
	if who:getTreasure() then return who:getTreasure():getEffectiveId() end
	for i = 0, 3 do
		if not self.player:getEquip(i) and who:getEquip(i) and not (i == 1 and self.player:hasSkills("bazhen|yizhong|bossmanjia")) then
			return who:getEquip(i):getEffectiveId()
		end
	end
	if who:getArmor() and self:evaluateArmor(who:getArmor()) >= self:evaluateArmor(self.player:getArmor()) then return who:getArmor():getEffectiveId() end
	if who:getDefensiveHorse() then return who:getDefensiveHorse():getEffectiveId() end
	if who:getOffensiveHorse() then return who:getOffensiveHorse():getEffectiveId() end
	if who:getWeapon() and self:evaluateWeapon(who:getWeapon()) >= self:evaluateWeapon(self.player:getWeapon()) then return who:getWeapon():getEffectiveId() end
end

sgs.ai_skill_invoke.nosquanji = function(self, data)
	local current = self.room:getCurrent()
	if self:isFriend(current) then
		if current:hasSkill("zhiji") and not current:hasSkill("ikyuxi") and current:getHandcardNum() == 1 then
			self.nosquanji_card = self:getMinCard(self.player):getId()
			return true
		end
	elseif self:isEnemy(current) then
		if self.player:getHandcardNum() <= 1 and not self:needKongcheng(self.player) then return "." end
		local invoke = false
		if current:hasSkill("yinghun") and current:getLostHp() > 2 then invoke = true end
		if current:hasSkill("luoshen") and not self:isWeak() then invoke = true end
		if current:hasSkill("baiyin") and not current:hasSkill("jilve") and current:getMark("@bear") >= 4 then invoke = true end
		if current:hasSkill("ikguiyue") and not current:hasSkill("jixi") and current:getPile("field"):length() >= 3 then invoke = true end
		if current:hasSkill("zili") and not current:hasSkill("paiyi") and current:getPile("power"):length() >= 3 then invoke = true end
		if current:hasSkill("hunzi") and not current:hasSkill("yingzi") and current:getHp() == 1 then invoke = true end
		if current:hasSkill("zuixiang") and current:getMark("@dream") > 0 then invoke = true end
		if self:isWeak(current) and self.player:getHandcardNum() > 1 and current:getCards("j"):isEmpty() then invoke = true end

		if invoke then
			local max_card = self:getMaxCard()
			local max_point = max_card:getNumber()
			if self.player:hasSkill("yingyang") then max_point = math.min(max_point + 3, 13) end
			local enemy_max_card = self:getMaxCard(current)
			local enemy_number = enemy_max_card and enemy_max_card:getNumber() or 0
			if enemy_max_card and current:hasSkill("yingyang") then enemy_number = math.min(enemy_number + 3, 13) end
			local allknown = 0
			if getKnownNum(current) == current:getHandcardNum() then
				allknown = allknown + 1
			end
			if (enemy_max_card and max_point > enemy_number and allknown > 0)
				or (enemy_max_card and max_point > enemy_number and allknown < 1 and max_point > 10)
				or (not enemy_max_card and max_point > 10) then
				self.nosquanji_card = max_card:getId()
				return true
			end
		end
	end

	return false
end

sgs.ai_skill_invoke.nosyexin = function(self, data)
	return true
end

local nosyexin_skill = {}
nosyexin_skill.name = "nosyexin"
table.insert(sgs.ai_skills, nosyexin_skill)
nosyexin_skill.getTurnUseCard = function(self)
	if self.player:getPile("nospower"):isEmpty() or self.player:hasUsed("NosYexinCard") then
		return
	end

	return sgs.Card_Parse("@NosYexinCard=.")
end

sgs.ai_skill_use_func.NosYexinCard = function(card, use, self)
	use.card = sgs.Card_Parse("@NosYexinCard=.")
end

sgs.ai_skill_askforag.nosyexin = function(self, card_ids)
	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	self:sortByCardNeed(cards)
	return cards[#cards]:getEffectiveId()
end

sgs.ai_skill_invoke.nospaiyi = function(self, data)
	return true
end

sgs.ai_skill_askforag.nospaiyi = function(self, card_ids)
	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	self:sortByCardNeed(cards)

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Indulgence") or acard:isKindOf("SupplyShortage") then
			sgs.nosPaiyiCard = acard
			return acard:getEffectiveId()
		end
	end

	local card = cards[#cards]
	sgs.nosPaiyiCard = card
	return card:getEffectiveId()
end

local function hp_subtract_handcard(a, b)
	local diff1 = a:getHp() - a:getHandcardNum()
	local diff2 = b:getHp() - b:getHandcardNum()

	return diff1 < diff2
end

local function handcard_subtract_hp(a, b)
	local diff1 = a:getHandcardNum() - a:getHp()
	local diff2 = b:getHandcardNum() - b:getHp()

	return diff1 < diff2
end

sgs.ai_skill_playerchosen.nospaiyi = function(self, targets)
	if sgs.nosPaiyiCard:isKindOf("Indulgence") then
		table.sort(self.enemies, hp_subtract_handcard)

		local enemies = self.enemies
		for _, enemy in ipairs(enemies) do
			if enemy:hasSkills("noslijian|lijian|fanjian|nosfanjian") and not enemy:containsTrick("indulgence")
				and not enemy:isKongcheng() and enemy:faceUp() and self:objectiveLevel(enemy) > 3 then
				sgs.nosPaiyiTarget = enemy
				sgs.nosPaiyiCard = nil
				return enemy
			end
		end

		for _, enemy in ipairs(enemies) do
			if not enemy:containsTrick("indulgence") and not enemy:hasSkill("keji") and enemy:faceUp() and self:objectiveLevel(enemy) > 3 then
				sgs.nosPaiyiTarget = enemy
				sgs.nosPaiyiCard = nil
				return enemy
			end
		end
	end

	if sgs.nosPaiyiCard:isKindOf("SupplyShortage") then
		table.sort(self.enemies, handcard_subtract_hp)

		local enemies = self.enemies
		for _, enemy in ipairs(enemies) do
			if (enemy:hasSkills("yongsi|haoshi|tuxi|nostuxi") or (enemy:hasSkill("zaiqi") and enemy:getLostHp() > 1))
				and not enemy:containsTrick("supply_shortage") and enemy:faceUp() and self:objectiveLevel(enemy) > 3 then
				sgs.nosPaiyiTarget = enemy
				sgs.nosPaiyiCard = nil
				return enemy
			end
		end
		for _, enemy in ipairs(enemies) do
			if (#enemies == 1 or not enemy:hasSkills("tiandu|guidao")) and not enemy:containsTrick("supply_shortage") and enemy:faceUp() and self:objectiveLevel(enemy) > 3 then
				sgs.nosPaiyiTarget = enemy
				sgs.nosPaiyiCard = nil
				return enemy
			end
		end
	end

	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	for _, target in ipairs(targets) do
		if self:isEnemy(target) and target:hasSkill("zhiji") and not target:hasSkill("ikyuxi") and target:getHandcardNum() == 0 then
			sgs.nosPaiyiTarget = target
			sgs.nosPaiyiCard = nil
			return target
		end
	end

	for _, target in ipairs(targets) do
		if self:isFriend(target) and target:objectName() ~= self.player:objectName() then
			sgs.nosPaiyiTarget = target
			sgs.nosPaiyiCard = nil
			return target
		end
	end

	sgs.nosPaiyiTarget = self.player
	sgs.nosPaiyiCard = nil
	return self.player
end

sgs.ai_skill_choice.nospaiyi = function(self, choices)
	local choice_table = choices:split("+")
	if table.contains(choice_table, "Judging") and self:isEnemy(sgs.nosPaiyiTarget) then
		sgs.nosPaiyiTarget = nil
		return "Judging"
	end

	if table.contains(choice_table, "Equip") and self:isFriend(sgs.nosPaiyiTarget) then
		sgs.nosPaiyiTarget = nil
		return "Equip"
	end

	sgs.nosPaiyiTarget = nil
	return "Hand"
end

sgs.ai_skill_invoke.nosjianxiong = function(self, data)
	local damage = data:toDamage()
	local id = damage.card:getEffectiveId()
	local card = sgs.Sanguosha:getCard(id)
	if self.player:isKongcheng() and damage.from and damage.from:isAlive() and damage.from:getPhase() == sgs.Player_Play
		and damage.from:hasSkills("longdan+chongzhen") and self:slashIsAvailable(damage.from)
		and card:isKindOf("Jink") and getKnownCard(damage.from, self.player, "Jink", false) > 0 then
		return false
	end
	if damage.card:isVirtualCard() and damage.card:subcardsLength() >= 3 then return true
	elseif damage.card:isVirtualCard() and damage.card:subcardsLength() == 2 then
		for _, card in sgs.qlist(damage.card:getSubcards()) do
			if self:isValuableCard(card) then return true end
		end
	end
	return not self:needKongcheng(self.player, true)
end

sgs.ai_skill_invoke.nosfankui = sgs.ai_skill_invoke.fankui
sgs.ai_skill_cardchosen.nosfankui = sgs.ai_skill_cardchosen.fankui
sgs.ai_need_damaged.nosfankui = sgs.ai_need_damaged.fankui
sgs.ai_choicemade_filter.skillInvoke.nosfankui = sgs.ai_choicemade_filter.skillInvoke.fankui
sgs.ai_choicemade_filter.cardChosen.nosfankui = sgs.ai_choicemade_filter.cardChosen.fankui

sgs.ai_skill_cardask["@nosguicai-card"] = function(self, data)
	local judge = data:toJudge()

	if self.room:getMode():find("_mini_46") and not judge:isGood() then return "$" .. self.player:handCards():first() end
	if self:needRetrial(judge) then
		local cards = sgs.QList2Table(self.player:getHandcards())
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

sgs.ai_cardneed.nosguicai = sgs.ai_cardneed.guicai
sgs.nosguicai_suit_value = sgs.guicai_suit_value

sgs.ai_skill_invoke.nosganglie = function(self, data)
	local mode = self.room:getMode()
	if mode:find("_mini_40") or mode:find("_mini_46") then return true end
	local damage = data:toDamage()
	if not damage.from then
		local zhangjiao = self.room:findPlayerBySkillName("guidao")
		return zhangjiao and self:isFriend(zhangjiao) and not zhangjiao:isNude()
	end
	if self:getDamagedEffects(damage.from, self.player) then
		if self:isFriend(damage.from) then
			sgs.ai_nosganglie_effect = string.format("%s_%s_%d", self.player:objectName(), damage.from:objectName(), sgs.turncount)
			return true
		end
		return false
	end
	return not self:isFriend(damage.from) and self:canAttack(damage.from)
end

sgs.ai_need_damaged.nosganglie = function(self, attacker, player)
	if not attacker then return false end
	if not attacker:hasSkill("nosganglie") and self:getDamagedEffects(attacker, player) then return self:isFriend(attacker, player) end
	if self:isEnemy(attacker) and attacker:getHp() + attacker:getHandcardNum() <= 3
		and not (attacker:hasSkills(sgs.need_kongcheng .. "|buqu") and attacker:getHandcardNum() > 1) and sgs.isGoodTarget(attacker, self:getEnemies(attacker), self) then
		return true
	end
	return false
end

function nosganglie_discard(self, discard_num, min_num, optional, include_equip, skillName)
	local xiahou = self.room:findPlayerBySkillName(skillName)
	if xiahou and (not self:damageIsEffective(self.player, sgs.DamageStruct_Normal, xiahou) or self:getDamagedEffects(self.player, xiahou)) then return {} end
	if xiahou and self:needToLoseHp(self.player, xiahou) then return {} end
	local to_discard = {}
	local cards = sgs.QList2Table(self.player:getHandcards())
	local index = 0
	local all_peaches = 0
	for _, card in ipairs(cards) do
		if isCard("Peach", card, self.player) then
			all_peaches = all_peaches + 1
		end
	end
	if all_peaches >= 2 and self:getOverflow() <= 0 then return {} end
	self:sortByKeepValue(cards)
	cards = sgs.reverse(cards)

	for i = #cards, 1, -1 do
		local card = cards[i]
		if not isCard("Peach", card, self.player) and not self.player:isJilei(card) then
			table.insert(to_discard, card:getEffectiveId())
			table.remove(cards, i)
			index = index + 1
			if index == 2 then break end
		end
	end
	if #to_discard < 2 then return {}
	else
		return to_discard
	end
end

sgs.ai_skill_discard.nosganglie = function(self, discard_num, min_num, optional, include_equip)
	return nosganglie_discard(self, discard_num, min_num, optional, include_equip, "nosganglie")
end

function sgs.ai_slash_prohibit.nosganglie(self, from, to)
	if not to:hasSkill("nosganglie") then return false end
	if self:isFriend(from, to) then return false end
	if from:hasSkill("ikxuwu") or from:getMark("thshenyou") > 0 or (from:hasSkill("ikwanhun") and from:distanceTo(to) == 1) then return false end
	if from:hasFlag("IkJieyouUsed") then return false end
	return from:getHandcardNum() + from:getHp() < 4
end

sgs.ai_choicemade_filter.skillInvoke.nosganglie = function(self, player, promptlist)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if damage.from and damage.to then
		if promptlist[#promptlist] == "yes" then
			if not self:getDamagedEffects(damage.from, player) and not self:needToLoseHp(damage.from, player) then
				sgs.updateIntention(damage.to, damage.from, 40)
			end
		elseif self:canAttack(damage.from) then
			sgs.updateIntention(damage.to, damage.from, -40)
		end
	end
end

sgs.ai_skill_use["@@nostuxi"] = function(self, prompt)
	local targets = self:getTuxiTargets("nostuxi")
	if #targets > 0 then return "@NosTuxiCard=.->" .. table.concat(targets, "+") end
	return "."
end

sgs.ai_card_intention.NosTuxiCard = sgs.ai_card_intention.TuxiCard

sgs.ai_skill_invoke.nosluoyi = function(self, data)
	if self.player:isSkipped(sgs.Player_Play) then return false end
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	local slashtarget = 0
	local dueltarget = 0
	self:sort(self.enemies, "hp")
	for _, card in ipairs(cards) do
		if isCard("Slash", card, self.player) then
			local slash = card:isKindOf("Slash") and card or sgs.cloneCard("slash", card:getSuit(), card:getNumber())
			for _, enemy in ipairs(self.enemies) do
				if self.player:canSlash(enemy, slash, true) and self:slashIsEffective(slash, enemy) and self:objectiveLevel(enemy) > 3 and sgs.isGoodTarget(enemy, self.enemies, self) then
					if getCardsNum("Jink", enemy, self.player) < 1 or (self.player:hasWeapon("axe") and self.player:getCards("he"):length() > 4) then
						slashtarget = slashtarget + 1
					end
				end
			end
		end
		if card:isKindOf("Duel") then
			local duel = sgs.cloneCard("duel", card:getSuit(), card:getNumber())
			for _, enemy in ipairs(self.enemies) do
				if self:getCardsNum("Slash") >= getCardsNum("Slash", enemy, self.player) and self:hasTrickEffective(duel, enemy)
					and self:objectiveLevel(enemy) > 3 and not self:cantbeHurt(enemy, self.player, 2) and self:damageIsEffective(enemy) then
					dueltarget = dueltarget + 1
				end
			end
		end
	end
	return slashtarget + dueltarget > 0
end

sgs.ai_cardneed.nosluoyi = sgs.ai_cardneed.luoyi
sgs.nosluoyi_keep_value = sgs.luoyi_keep_value

sgs.ai_skill_invoke.nosyiji = function(self)
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

sgs.ai_skill_askforyiji.nosyiji = function(self, card_ids)
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

sgs.ai_need_damaged.nosyiji = function(self, attacker, player)
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

local noslijian_skill = {}
noslijian_skill.name = "noslijian"
table.insert(sgs.ai_skills, noslijian_skill)
noslijian_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("NosLijianCard") or not self.player:canDiscard(self.player, "he") then return end
	local card_id = self:getLijianCard()
	if card_id then return sgs.Card_Parse("@NosLijianCard=" .. card_id) end
end

sgs.ai_skill_use_func.NosLijianCard = function(card, use, self)
	local first, second = self:findLijianTarget("NosLijianCard", use)
	if first and second then
		use.card = card
		if use.to then
			use.to:append(first)
			use.to:append(second)
		end
	end
end

sgs.ai_use_value.NosLijianCard = sgs.ai_use_value.LijianCard
sgs.ai_use_priority.NosLijianCard = sgs.ai_use_priority.LijianCard
sgs.ai_card_intention.NosLijianCard = sgs.ai_card_intention.LijianCard

local nosrende_skill = {}
nosrende_skill.name = "nosrende"
table.insert(sgs.ai_skills, nosrende_skill)
nosrende_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() then return end
	local mode = string.lower(global_room:getMode())
	if self.player:getMark("nosrende") > 1 and mode:find("04_1v3") then return end

	if self:shouldUseRende() then
		return sgs.Card_Parse("@NosRendeCard=.")
	end
end

sgs.ai_skill_use_func.NosRendeCard = function(card, use, self)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards, true)

	local notFound
	for i = 1, #cards do
		local card, friend = self:getCardNeedPlayer(cards)
		if card and friend then
			cards = self:resetCards(cards, card)
		else
			notFound = true
			break
		end

		if friend:objectName() == self.player:objectName() or not self.player:getHandcards():contains(card) then continue end
		local canIkXinqi = self.player:hasLordSkill("ikxinqi") and friend:getKingdom() == "kaze"
		if card:isAvailable(self.player) and ((card:isKindOf("Slash") and not canIkXinqi) or card:isKindOf("Duel") or card:isKindOf("Snatch") or card:isKindOf("Dismantlement")) then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			local cardtype = card:getTypeId()
			self["use" .. sgs.ai_type_name[cardtype + 1] .. "Card"](self, card, dummy_use)
			if dummy_use.card and dummy_use.to:length() > 0 then
				if card:isKindOf("Slash") or card:isKindOf("Duel") then
					local t1 = dummy_use.to:first()
					if dummy_use.to:length() > 1 then continue
					elseif t1:getHp() == 1 or sgs.card_lack[t1:objectName()]["Jink"] == 1
							or t1:isCardLimited(sgs.cloneCard("jink"), sgs.Card_MethodResponse) then continue
					end
				elseif (card:isKindOf("Snatch") or card:isKindOf("Dismantlement")) and self:getEnemyNumBySeat(self.player, friend) > 0 then
					local hasDelayedTrick
					for _, p in sgs.qlist(dummy_use.to) do
						if self:isFriend(p) and (self:willSkipDrawPhase(p) or self:willSkipPlayPhase(p)) then hasDelayedTrick = true break end
					end
					if hasDelayedTrick then continue end
				end
			end
		elseif card:isAvailable(self.player) and self:getEnemyNumBySeat(self.player, friend) > 0 and (card:isKindOf("Indulgence") or card:isKindOf("SupplyShortage")) then
			local dummy_use = { isDummy = true }
			self:useTrickCard(card, dummy_use)
			if dummy_use.card then continue end
		end

		if friend:hasSkill("enyuan") and #cards >= 1 and not (self.room:getMode() == "04_1v3" and self.player:getMark("nosrende") == 1) then
			use.card = sgs.Card_Parse("@NosRendeCard=" .. card:getId() .. "+" .. cards[1]:getId())
		else
			use.card = sgs.Card_Parse("@NosRendeCard=" .. card:getId())
		end
		if use.to then use.to:append(friend) end
		return
	end

	if notFound then
		local pangtong = self.room:findPlayerBySkillName("manjuan")
		if not pangtong then return end
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByUseValue(cards, true)
		if self.player:isWounded() and self.player:getHandcardNum() > 3 and self.player:getMark("nosrende") < 2 then
			self:sortByUseValue(cards, true)
			local to_give = {}
			for _, card in ipairs(cards) do
				if not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player) then table.insert(to_give, card:getId()) end
				if #to_give == 2 - self.player:getMark("nosrende") then break end
			end
			if #to_give > 0 then
				use.card = sgs.Card_Parse("@NosRendeCard=" .. table.concat(to_give, "+"))
				if use.to then use.to:append(pangtong) end
			end
		end
	end
end

sgs.ai_use_value.NosRendeCard = sgs.ai_use_value.IkShenaiCard
sgs.ai_use_priority.NosRendeCard = sgs.ai_use_priority.IkShenaiCard

sgs.ai_card_intention.NosRendeCard = sgs.ai_card_intention.IkShenaiCard

sgs.dynamic_value.benefit.NosRendeCard = true

sgs.ai_skill_invoke.nostieji = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then return false end

	local zj = self.room:findPlayerBySkillName("guidao")
	if zj and self:isEnemy(zj) and self:canRetrial(zj) then return false end

	if target:hasArmorEffect("eight_diagram") and not self.player:hasWeapon("qinggang_sword") then return true end
	if target:hasLordSkill("hujia") then
		for _, p in ipairs(self.enemies) do
			if p:getKingdom() == "hana" and (p:hasArmorEffect("eight_diagram") or p:getHandcardNum() > 0) then return true end
		end
	end
	if target:hasSkill("longhun") and target:getHp() == 1 and self:hasSuit("club", true, target) then return true end
	if target:isKongcheng() or (getKnownNum(target) == target:getHandcardNum() and getKnownCard(target, self.player, "Jink", true) == 0) then return false end
	return true
end

sgs.ai_choicemade_filter.skillInvoke.nostieji = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		local target
		for _, p in sgs.qlist(self.room:getAllPlayers()) do
			if p:hasFlag("NosTiejiTarget") then
				target = p
				break
			end
		end
		if target then sgs.updateIntention(player, target, 50) end
	end
end

function sgs.ai_cardneed.nosjizhi(to, card)
	return card:isNDTrick()
end

sgs.nosjizhi_keep_value = sgs.jizhi_keep_value

local noskurou_skill = {}
noskurou_skill.name = "noskurou"
table.insert(sgs.ai_skills, noskurou_skill)
noskurou_skill.getTurnUseCard = function(self, inclusive)
	if (self.player:getHp() > 3 and self.player:getHandcardNum() > self.player:getHp())
		or (self.player:getHp() - self.player:getHandcardNum() >= 2) then
		return sgs.Card_Parse("@NosKurouCard=.")
	end

	local function can_noskurou_with_cb(self)
		if self.player:getHp() > 1 then return true end
		local has_save = false
		local huatuo = self.room:findPlayerBySkillName("jijiu")
		if huatuo and self:isFriend(huatuo) then
			for _, equip in sgs.qlist(huatuo:getEquips()) do
				if equip:isRed() then has_save = true break end
			end
			if not has_save then has_save = (huatuo:getHandcardNum() > 3) end
		end
		if has_save then return true end
		local handang = self.room:findPlayerBySkillName("ikjieyou")
		if handang and self:isFriend(handang) and getCardsNum("Slash", handang, self.player) >= 1 then return true end
		return false
	end

	local slash = sgs.cloneCard("slash")
	if self.player:hasWeapon("crossbow") or self:getCardsNum("Crossbow") > 0 then
		for _, enemy in ipairs(self.enemies) do
			if self.player:canSlash(enemy) and self:slashIsEffective(slash, enemy)
				and sgs.isGoodTarget(enemy, self.enemies, self, true) and not self:slashProhibit(slash, enemy) and can_noskurou_with_cb(self) then
				return sgs.Card_Parse("@NosKurouCard=.")
			end
		end
	end

	if self.player:getHp() <= 1 and self:getCardsNum("Analeptic") + self:getCardsNum("Peach") > 1 then
		return sgs.Card_Parse("@NosKurouCard=.")
 	end
end

sgs.ai_skill_use_func.NosKurouCard = function(card, use, self)
	use.card = card
end

sgs.ai_use_priority.NosKurouCard = 6.8

local nosfanjian_skill = {}
nosfanjian_skill.name = "nosfanjian"
table.insert(sgs.ai_skills, nosfanjian_skill)
nosfanjian_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() or self.player:hasUsed("NosFanjianCard") then return nil end
	return sgs.Card_Parse("@NosFanjianCard=.")
end

sgs.ai_skill_use_func.NosFanjianCard = function(card, use, self)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards, true)
	if #cards == 1 and cards[1]:getSuit() == sgs.Card_Diamond then return end
	if #cards <= 4 and (self:getCardsNum("Peach") > 0 or self:getCardsNum("Analeptic") > 0) then return end
	self:sort(self.enemies, "defense")

	local suits = {}
	local suits_num = 0
	for _, c in ipairs(cards) do
		if not suits[c:getSuitString()] then
			suits[c:getSuitString()] = true
			suits_num = suits_num + 1
		end
	end

	local wgt = self.room:findPlayerBySkillName("buyi")
	if wgt and self:isFriend(wgt) then wgt = nil end
	local basic_num = 0
	for _, c in ipairs(cards) do
		if c:getTypeId() == sgs.Card_TypeBasic then basic_num = basic_num + 1 end
	end
	local visible = 0
	for _, enemy in ipairs(self.enemies) do
		local visible = 0
		for _, c in ipairs(cards) do
			local flag = string.format("%s_%s_%s", "visible", enemy:objectName(), self.player:objectName())
			if c:hasFlag("visible") or c:hasFlag(flag) then visible = visible + 1 end
		end
		if visible > 0 and (#cards <= 2 or suits_num <= 2) then continue end
		if self:canAttack(enemy) and not enemy:hasSkills("qingnang|jijiu|tianxiang")
			and not (wgt and basic_num / enemy:getHandcardNum() <= 0.3 and (enemy:getHandcardNum() <= 1 or enemy:objectName() == wgt:objectName())) then
			use.card = card
			if use.to then use.to:append(enemy) end
			return
		end
	end
end

sgs.ai_card_intention.NosFanjianCard = 70

function sgs.ai_skill_suit.nosfanjian(self)
	local map = { 0, 0, 1, 2, 2, 3, 3, 3 }
	local suit = map[math.random(1, 8)]
	local tg = self.room:getCurrent()
	local suits = {}
	local maxnum, maxsuit = 0
	for _, c in sgs.qlist(tg:getHandcards()) do
		local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), tg:objectName())
		if c:hasFlag(flag) or c:hasFlag("visible") then
			if not suits[c:getSuitString()] then suits[c:getSuitString()] = 1 else suits[c:getSuitString()] = suits[c:getSuitString()] + 1 end
			if suits[c:getSuitString()] > maxnum then
				maxnum = suits[c:getSuitString()]
				maxsuit = c:getSuit()
			end
		end
	end
	local return_suit = maxsuit or suit
	if self.player:hasSkill("hongyan") and return_suit == sgs.Card_Spade then return sgs.Card_Heart
	elseif not self.player:hasSkill("ikchiqiu") and self.player:hasSkill("thanyue") and return_suit == sgs.Card_Heart then return sgs.Card_Spade end
	return return_suit
end

sgs.dynamic_value.damage_card.NosFanjianCard = true

sgs.ai_skill_invoke.noslianying = function(self, data)
	if self:needKongcheng(self.player, true) then
		return self.player:getPhase() == sgs.Player_Play
	end
	return true
end

local nosguose_skill = {}
nosguose_skill.name = "nosguose"
table.insert(sgs.ai_skills, nosguose_skill)
nosguose_skill.getTurnUseCard = function(self, inclusive)
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
		if acard:isKindOf("Weapon") and not (acard:getSuit() == sgs.Card_Diamond) then has_weapon = true end
	end

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Armor") and not (acard:getSuit() == sgs.Card_Diamond) then has_armor = true end
	end

	for _, acard in ipairs(cards) do
		if (acard:getSuit() == sgs.Card_Diamond) and ((self:getUseValue(acard) < sgs.ai_use_value.Indulgence) or inclusive) then
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
	local card_str = ("indulgence:nosguose[diamond:%s]=%d"):format(number, card_id)
	local indulgence = sgs.Card_Parse(card_str)
	assert(indulgence)
	return indulgence
end

sgs.ai_cardneed.nosguose = sgs.ai_cardneed.guose
sgs.nosguose_suit_value = sgs.guose_suit_value

local qingnang_skill = {}
qingnang_skill.name = "qingnang"
table.insert(sgs.ai_skills, qingnang_skill)
qingnang_skill.getTurnUseCard = function(self)
	if self.player:getHandcardNum() < 1 then return nil end
	if self.player:usedTimes("QingnangCard") > 0 then return nil end

	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)

	local compare_func = function(a, b)
		local v1 = self:getKeepValue(a) + (a:isRed() and 50 or 0) + (a:isKindOf("Peach") and 50 or 0)
		local v2 = self:getKeepValue(b) + (b:isRed() and 50 or 0) + (b:isKindOf("Peach") and 50 or 0)
		return v1 < v2
	end
	table.sort(cards, compare_func)

	local card_str = ("@QingnangCard=%d"):format(cards[1]:getId())
	return sgs.Card_Parse(card_str)
end

sgs.ai_skill_use_func.QingnangCard = function(card, use, self)
	local target = self:findPlayerToRecover()

	if target and (self:isWeak(target) or self:getOverflow() >= 1) then
		use.card = card
		if use.to then
			use.to:append(target)
		end
		return
	end
end

sgs.ai_use_priority.QingnangCard = 4.2
sgs.ai_card_intention.QingnangCard = -100

sgs.dynamic_value.benefit.QingnangCard = true

function sgs.ai_skill_invoke.nosjushou(self, data)
	local sbdiaochan = self.room:findPlayerBySkillName("thjinlu")
	if sbdiaochan and sbdiaochan:faceUp() and not self:willSkipPlayPhase(sbdiaochan)
		and (self:isEnemy(sbdiaochan) or (sgs.turncount <= 1 and sgs.evaluatePlayerRole(sbdiaochan) == "neutral")) then return false end
	if not self.player:faceUp() then return true end
	for _, friend in ipairs(self.friends) do
		if friend:hasSkills("fangzhu|jilve") then return true end
		if friend:hasSkill("junxing") and friend:faceUp() and not self:willSkipPlayPhase(friend)
			and not (friend:isKongcheng() and self:willSkipDrawPhase(friend)) then
			return true
		end
	end
	return self:isWeak()
end

sgs.ai_skill_askforag.nosbuqu = function(self, card_ids)
	for i, card_id in ipairs(card_ids) do
		for j, card_id2 in ipairs(card_ids) do
			if i ~= j and sgs.Sanguosha:getCard(card_id):getNumber() == sgs.Sanguosha:getCard(card_id2):getNumber() then
				return card_id
			end
		end
	end

	return card_ids[1]
end

function sgs.ai_skill_invoke.nosbuqu(self, data)
	if #self.enemies == 1 and self.enemies[1]:hasSkill("nosguhuo") then
		return false
	else
		local damage = data:toDamage()
		if self.player:getHp() == 1 and damage.to and damage:getReason() == "duwu" and self:getSaveNum(true) >= 1 then return false end
		return true
	end
end

sgs.ai_skill_playerchosen.nosleiji = function(self, targets)
	local mode = self.room:getMode()
	if mode:find("_mini_17") or mode:find("_mini_19") or mode:find("_mini_20") or mode:find("_mini_26") then
		local players = self.room:getAllPlayers()
		for _, aplayer in sgs.qlist(players) do
			if aplayer:getState() ~= "robot" then
				return aplayer
			end
		end
	end

	self:updatePlayers()
	return self:findLeijiTarget(self.player, 100, nil, -1)
end

sgs.ai_playerchosen_intention.nosleiji = sgs.ai_playerchosen_intention.leiji

function sgs.ai_slash_prohibit.nosleiji(self, from, to, card)
	if not to:hasSkill("nosleiji") then return false end
	if self:isFriend(to, from) then return false end
	if (to:hasFlag("QianxiTarget") or to:getMark("yijue") > 0) and (not self:hasEightDiagramEffect(to) or self.player:hasWeapon("qinggang_sword")) then return false end
	if not sgs.isJinkAvailable(from, to, card, to:hasSkill("guidao")) then return false end
	if from:getRole() == "rebel" and to:isLord() then
		local other_rebel
		for _, player in sgs.qlist(self.room:getOtherPlayers(from)) do
			if sgs.evaluatePlayerRole(player) == "rebel" or sgs.compareRoleEvaluation(player, "rebel", "loyalist") == "rebel" then
				other_rebel = player
				break
			end
		end
		if not other_rebel and ((from:getHp() >= 4 and (getCardsNum("Peach", from, to) > 0 or from:hasSkills("ganglie|nosganglie|vsganglie"))) or from:hasSkill("hongyan")) then
			return false
		end
	end

	if getKnownCard(to, self.player, "Jink", true) >= 1 or (self:hasSuit("spade", true, to) and hcard >= 2) then return true end
	if self:hasEightDiagramEffect(to) then return true end
end

sgs.ai_cardneed.nosleiji = sgs.ai_cardneed.leiji

table.insert(sgs.ai_global_flags, "questioner")

sgs.ai_skill_choice.nosguhuo = function(self, choices)
	local yuji = self.room:findPlayerBySkillName("nosguhuo")
	local nosguhuoname = self.room:getTag("NosGuhuoType"):toString()
	if nosguhuoname == "peach+analeptic" then nosguhuoname = "peach" end
	if nosguhuoname == "normal_slash" then nosguhuoname = "slash" end
	local nosguhuocard = sgs.cloneCard(nosguhuoname)
	local nosguhuotype = nosguhuocard:getClassName()
	if nosguhuotype and self:getRestCardsNum(nosguhuotype, yuji) == 0 and self.player:getHp() > 0 then return "question" end
	if nosguhuotype and nosguhuotype == "AmazingGrace" then return "noquestion" end
	if nosguhuotype:match("Slash") then
		if yuji:getState() ~= "robot" and math.random(1, 4) == 1 and self:isEnemy(yuji) and not sgs.questioner then return "question" end
		if not self:hasCrossbowEffect(yuji) then return "noquestion" end
	end
	if yuji:hasFlag("NosGuhuoFailed") and math.random(1, 6) == 1 and self:isEnemy(yuji) and self.player:getHp() >= 3
		and self.player:getHp() > self.player:getLostHp() then return "question" end
	local players = self.room:getOtherPlayers(self.player)
	players = sgs.QList2Table(players)
	local x = math.random(1, 5)

	self:sort(self.friends, "hp")
	if self.player:getHp() < 2 and self:getCardsNum("Peach") < 1 and self.room:alivePlayerCount() > 2 then return "noquestion" end
	if self:isFriend(yuji) then return "noquestion"
	elseif sgs.questioner then return "noquestion"
	elseif self.player:getHp() < self.friends[#self.friends]:getHp() then return "noquestion"
	end
	if self:needToLoseHp(self.player) and not self.player:hasSkills(sgs.masochism_skill) and x ~= 1 then return "question" end

	local questioner
	for _, friend in ipairs(self.friends) do
		if friend:getHp() == self.friends[#self.friends]:getHp() then
			if friend:hasSkills("nosrende|ikshenai|kuanggu|kofkuanggu|zaiqi|buqu|nosbuqu|yinghun|longhun|xueji|baobian") then
				questioner = friend
				break
			end
		end
	end
	if not questioner then questioner = self.friends[#self.friends] end
	if self.player:hasSkill("zhaxiang") then return "question" end
	return self.player:objectName() == questioner:objectName() and x ~= 1 and "question" or "noquestion"
end

sgs.ai_choicemade_filter.skillChoice.nosguhuo = function(self, player, promptlist)
	if promptlist[#promptlist] == "question" then
		sgs.questioner = player
		local yuji = self.room:findPlayerBySkillName("nosguhuo")
		if not yuji then return end
		local nosguhuoname = self.room:getTag("NosGuhuoType"):toString()
		if nosguhuoname == "peach+analeptic" or nosguhuoname == "peach" then
			sgs.updateIntention(player, yuji, 80)
			return
		end
		if nosguhuoname == "normal_slash" then nosguhuoname = "slash" end
		local nosguhuocard = sgs.cloneCard(nosguhuoname)
		if nosguhuocard then
			local nosguhuotype = nosguhuocard:getClassName()
			if nosguhuotype and self:getRestCardsNum(nosguhuotype, yuji) > 0 then
				sgs.updateIntention(player, yuji, 80)
				return
			end
		end
	end
end

local nosguhuo_skill = {}
nosguhuo_skill.name = "nosguhuo"
table.insert(sgs.ai_skills, nosguhuo_skill)
nosguhuo_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() then return end

	local cards = sgs.QList2Table(self.player:getHandcards())
	local otherSuit_str, NosGuhuoCard_str = {}, {}

	for _, card in ipairs(cards) do
		if card:isNDTrick() then
			local dummyuse = { isDummy = true }
			self:useTrickCard(card, dummyuse)
			if dummyuse.card then
				local cardstr = "@NosGuhuoCard=" .. card:getId() .. ":" .. card:objectName()
				if card:getSuit() == sgs.Card_Heart then
					table.insert(NosGuhuoCard_str, cardstr)
				else
					table.insert(otherSuit_str, cardstr)
				end
			end
		end
	end

	local other_suit, enemy_is_weak, zgl_kongcheng = true
	local can_fake_nosguhuo = sgs.turncount > 1
	for _, enemy in ipairs(self.enemies) do
		if enemy:getHp() > 2 then
			other_suit = false
		end
		if enemy:getHp() > 1 then
			can_fake_nosguhuo = false
		end
		if self:isWeak(enemy) then
			enemy_is_weak = true
		end
		if enemy:hasSkill("ikjingyou") and enemy:isKongcheng() then
			zgl_kongcheng = true
		end
	end

	if #otherSuit_str > 0 and other_suit then
		table.insertTable(NosGuhuoCard_str, otherSuit_str)
	end

	local peach_str = self:getGuhuoCard("Peach", true, -1)
	if peach_str then table.insert(NosGuhuoCard_str, peach_str) end

	local fakeCards = {}

	for _, card in sgs.qlist(self.player:getHandcards()) do
		if (card:isKindOf("Slash") and self:getCardsNum("Slash", "h") >= 2 and not self:hasCrossbowEffect())
			or (card:isKindOf("Jink") and self:getCardsNum("Jink", "h") >= 3)
			or (card:isKindOf("EquipCard") and self:getSameEquip(card))
			or card:isKindOf("Disaster") then
			table.insert(fakeCards, card)
		end
	end
	self:sortByUseValue(fakeCards, true)

	local function fake_nosguhuo(objectName, can_fake_nosguhuo)
		if #fakeCards == 0 then return end

		local fakeCard
		local nosguhuo = "peach|ex_nihilo|snatch|dismantlement|amazing_grace|archery_attack|savage_assault|god_salvation"
		local ban = table.concat(sgs.Sanguosha:getBanPackages(), "|")
		if not ban:match("maneuvering") then nosguhuo = nosguhuo .. "|fire_attack" end
		local nosguhuos = nosguhuo:split("|")
		for i = 1, #nosguhuos do
			local forbidden = nosguhuos[i]
			local forbid = sgs.cloneCard(forbidden)
			if self.player:isLocked(forbid) then
				table.remove(nosguhuos, i)
				i = i - 1
			end
		end
		if can_fake_nosguhuo then
			for i = 1, #nosguhuos do
				if nosguhuos[i] == "god_salvation" then table.remove(nosguhuos, i) break end
			end
		end
		for i = 1, 10 do
			local card = fakeCards[math.random(1, #fakeCards)]
			local newnosguhuo = objectName or nosguhuos[math.random(1, #nosguhuos)]
			local nosguhuocard = sgs.cloneCard(newnosguhuo, card:getSuit(), card:getNumber())
			if self:getRestCardsNum(nosguhuocard:getClassName()) > 0 then
				local dummyuse = { isDummy = true }
				if newnosguhuo == "peach" then self:useBasicCard(nosguhuocard, dummyuse) else self:useTrickCard(nosguhuocard, dummyuse) end
				if dummyuse.card then
					fakeCard = sgs.Card_Parse("@NosGuhuoCard=" .. card:getId() .. ":" .. newnosguhuo)
					break
				end
			end
		end
		return fakeCard
	end

	if #NosGuhuoCard_str > 0 then
		local nosguhuo_str = NosGuhuoCard_str[math.random(1, #NosGuhuoCard_str)]

		local str = nosguhuo_str:split("=")
		str = str[2]:split(":")
		local cardid, cardname = str[1], str[2]
		if sgs.Sanguosha:getCard(cardid):objectName() == cardname and cardname == "ex_nihilo" then
			if math.random(1, 3) == 1 then
				local fake_exnihilo = fake_nosguhuo(cardname)
				if fake_exnihilo then return fake_exnihilo end
			end
			return sgs.Card_Parse(nosguhuo_str)
		elseif math.random(1, 5) == 1 then
			local fake_NosGuhuoCard = fake_nosguhuo()
			if fake_NosGuhuoCard then return fake_NosGuhuoCard end
		else
			return sgs.Card_Parse(nosguhuo_str)
		end
	elseif can_fake_nosguhuo and math.random(1, 4) ~= 1 then
		local fake_NosGuhuoCard = fake_nosguhuo(nil, can_fake_nosguhuo)
		if fake_NosGuhuoCard then return fake_NosGuhuoCard end
	elseif zgl_kongcheng and #fakeCards > 0 then
		return sgs.Card_Parse("@NosGuhuoCard=" .. fakeCards[1]:getEffectiveId() .. ":amazing_grace")
	else
		local lord = self.room:getLord()
		local drawcard = false
		if lord and self:isFriend(lord) and self:isWeak(lord) and not self.player:isLord() then
			drawcard = true
		elseif not enemy_is_weak then
			if sgs.current_mode_players["loyalist"] > sgs.current_mode_players["renegade"] + sgs.current_mode_players["rebel"]
				and self.role == "loyalist" and sgs.current_mode_players["rebel"] > 0 then
				drawcard = true
			elseif sgs.current_mode_players["rebel"] > sgs.current_mode_players["loyalist"] + sgs.current_mode_players["renegade"] + 2
				and self.role == "rebel" then
				drawcard = true
			end
		end

		if drawcard and #fakeCards > 0 then
			local card_objectname
			local objectNames = { "ex_nihilo", "snatch", "dismantlement", "amazing_grace", "archery_attack", "savage_assault", "god_salvation", "duel" }
			for _, objectName in ipairs(objectNames) do
				local acard = sgs.cloneCard(objectName)
				if self:getRestCardsNum(acard:getClassName()) == 0 then
					card_objectname = objectName
					break
				end
			end
			if card_objectname then
				return sgs.Card_Parse("@NosGuhuoCard=" .. fakeCards[1]:getEffectiveId() .. ":" .. card_objectname)
			end
		end
	end

	if self:isWeak() then
		local peach_str = self:getGuhuoCard("Peach", true, -1)
		if peach_str then
			local card = sgs.Card_Parse(peach_str)
			local peach = sgs.cloneCard("peach", card:getSuit(), card:getNumber())
			local dummy_use = { isDummy = true }
			self:useBasicCard(peach, dummy_use)
			if dummy_use.card then return card end
		end
	end
	local slash_str = self:getGuhuoCard("Slash", true, -1)
	if slash_str and self:slashIsAvailable() then
		local card = sgs.Card_Parse(slash_str)
		local slash = sgs.cloneCard("slash", card:getSuit(), card:getNumber())
		local dummy_use = { isDummy = true }
		self:useBasicCard(slash, dummy_use)
		if dummy_use.card then return card end
	end
end

sgs.ai_skill_use_func.NosGuhuoCard = function(card, use, self)
	local userstring = card:toString()
	userstring = (userstring:split(":"))[3]
	local nosguhuocard = sgs.cloneCard(userstring, card:getSuit(), card:getNumber())
	nosguhuocard:setSkillName("nosguhuo")
	if nosguhuocard:getTypeId() == sgs.Card_TypeBasic then
		self:useBasicCard(nosguhuocard, use)
		if not use.isDummy and use.card and nosguhuocard:isKindOf("Slash") and (not use.to or use.to:isEmpty()) then return end
	else
		assert(nosguhuocard)
		self:useTrickCard(nosguhuocard, use)
	end
	if not use.card then return end
	use.card = card
end

sgs.ai_use_priority.NosGuhuoCard = 10

sgs.nosguhuo_suit_value = {
	heart = 5,
}

sgs.ai_skill_choice.nosguhuo_saveself = sgs.ai_skill_choice.guhuo_saveself
sgs.ai_skill_choice.nosguhuo_slash = sgs.ai_skill_choice.guhuo_slash

function sgs.ai_cardneed.nosguhuo(to, card)
	return card:getSuit() == sgs.Card_Heart and (card:isKindOf("BasicCard") or card:isNDTrick())
end
