sgs.ai_skill_cardask["@ibuki_gourd"] = function(self, data, pattern)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, c in ipairs(cards) do
		if c:getSuit() == sgs.Card_Spade and not self:isValuableCard(c) then
			return c:getEffectiveId()
		end
	end
	for _, c in ipairs(cards) do
		if not self:isValuableCard(c) then
			return c:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_skill_invoke.ice_sword = function(self, data)
	local damage = data:toDamage()
	local target = damage.to
	if self:isFriend(target) then
		if self:getDamagedEffects(target, self.players, true) or self:needToLoseHp(target, self.player, true) then return false
		elseif target:isChained() and self:isGoodChainTarget(target, self.player, nil, nil, damage.card) then return false
		elseif self:isWeak(target) or damage.damage > 1 then return true
		elseif target:getLostHp() < 1 then return false end
		return true
	else
		if self:isWeak(target) or damage.damage > 1 or self:hasHeavySlashDamage(self.player, damage.card, target) then return false end
		if target:hasSkill("lirang") and #self:getFriendsNoSelf(target) > 0 then return false end
		if target:getArmor() and self:evaluateArmor(target:getArmor(), target) > 3 and not (target:hasArmorEffect("silver_lion") and target:isWounded()) then
			return true
		end
		if target:hasSkills("ikyindie+ikguiyue") and target:getPhase() == sgs.Player_NotActive then return false end
		if target:hasSkills(sgs.need_kongcheng) then return false end
		if target:getCards("he"):length() < 4 and target:getCards("he"):length() > 1 then return true end
		return false
	end
end

function SmartAI:useCardFeintAttack(FeintAttack, use)
	local fromList = sgs.QList2Table(self.room:getOtherPlayers(self.player))
	local toList = sgs.QList2Table(self.room:getOtherPlayers(self.player))

	self:sort(fromList, "defense")
	self:sort(toList, "defense")

	local n = nil
	local final_enemy = nil
	for _, enemy in ipairs(fromList) do
		if (not use.current_targets or not table.contains(use.current_targets, enemy:objectName()))
			and self:hasTrickEffective(FeintAttack, enemy)
			and not enemy:hasSkills(sgs.lose_equip_skill)
			and not self:needToThrowArmor(enemy)
			and not (self:needKongcheng(enemy, true) and enemy:getHandcardNum() == 1)
			and not enemy:isKongcheng()
			and self:objectiveLevel(enemy) >= 0 then

			for _, friend in ipairs(toList) do
				if self:objectiveLevel(friend) < -3 and enemy:objectName() ~= friend:objectName() then
					n = 1
					final_enemy = friend
					break
				end
			end

			if not n then
				for _, friend in ipairs(toList) do
					if self:objectiveLevel(friend) >= -3 and self:objectiveLevel(friend) <= 0 and enemy:objectName() ~= friend:objectName() then
						n = 1
						final_enemy = friend
						break
					end
				end
			end

			if n then
				use.card = FeintAttack
				if use.to then
					use.to:append(enemy)
					use.to:append(final_enemy)
				end
				return
			end
		end
		n = nil
	end

	for _, friend in ipairs(fromList) do
		if (not use.current_targets or not table.contains(use.current_targets, friend:objectName()))
			and self:hasTrickEffective(FeintAttack, friend)
			and ((friend:hasSkills(sgs.lose_equip_skill) and friend:hasEquip()) or (self:needKongcheng(friend, true) and friend:getHandcardNum() == 1) or self:getOverflow(friend) > 3)
			and not enemy:isKongcheng()
			and self:objectiveLevel(friend) < 0 then

			for _, friend2 in ipairs(toList) do
				if self:objectiveLevel(friend2) < -3 and friend:objectName() ~= friend2:objectName() then
					use.card = FeintAttack
					if use.to then
						use.to:append(friend)
						use.to:append(enemy)
					end
					return
				end
			end
		end
	end
end

sgs.ai_use_value.FeintAttack = 5.7
sgs.ai_use_priority.FeintAttack = 2.75
sgs.ai_keep_value.FeintAttack = 3.31

sgs.ai_card_intention.FeintAttack = function(self, card, from, tos)
	for _, p in ipairs(tos) do
		local friend = p:getTag("feintTarget"):toPlayer()
		if friend then
			sgs.updateIntention(from, friend, -20)
		end
	end
end

sgs.ai_skill_cardask["feint-attack-effect"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	if self:isFriend(target) then
		local c, p = self:getCardNeedPlayer(cards, { target })
		if p then
			return c:toString()
		end
		self:sortByKeepValue(cards, true)
		return "$" .. cards[1]:getEffectiveId()
	end
	self:sortByKeepValue(cards)
	return "$" .. cards[1]:getEffectiveId()
end

function SmartAI:useCardLureTiger(LureTiger, use)
	sgs.ai_use_priority.LureTiger = 4.9
	if not LureTiger:isAvailable(self.player) then return end

	local players = sgs.PlayerList()

	local card = self:getCard("BurningCamps")
	if card and card:isAvailable(self.player) then
		local nextp = self.player:getNextAlive()
		local first
		while true do
			if LureTiger:targetFilter(players, nextp, self.player) and self:hasTrickEffective(LureTiger, nextp, self.player) then
				if not first then
					if self:isEnemy(nextp) then
						first = nextp
					else
						players:append(nextp)
					end
				else
					if first:getKingdom() ~= nextp:getKingdom() or self:isFriend(nextp) then
						players:append(nextp)
					end
				end
				nextp = nextp:getNextAlive()
			else
				break
			end
		end
		if first and players:length() > 0 then
			use.card = LureTiger
			if use.to then
				for _, p in sgs.qlist(players) do
					use.to:append(self.room:findPlayer(p:objectName()))
				end
			end
			return
		end
	end

	players = sgs.PlayerList()

	card = self:getCard("ArcheryAttack")
	if card and card:isAvailable(self.player) and self:getAoeValue(card) > 0 then
		self:sort(self.friends_noself, "hp")
		for _, friend in ipairs(self.friends_noself) do
			if self:isFriend(friend) and LureTiger:targetFilter(players, friend, self.player) and self:hasTrickEffective(LureTiger, friend, self.player) then
				players:append(friend)
			end
		end
		for _, friend in ipairs(self.friends_noself) do
			if LureTiger:targetFilter(players, friend, self.player) and not players:contains(friend) and self:hasTrickEffective(LureTiger, friend, self.player) then
				players:append(friend)
			end
		end
		if players:length() > 0 then
			sgs.ai_use_priority.LureTiger = sgs.ai_use_priority.ArcheryAttack + 0.2
			use.card = LureTiger
			if use.to then
				for _, p in sgs.qlist(players) do
					use.to:append(self.room:findPlayer(p:objectName()))
				end
			end
			return
		end
	end

	players = sgs.PlayerList()

	card = self:getCard("SavageAssault")
	if card and card:isAvailable(self.player) and self:getAoeValue(card) > 0 then
		self:sort(self.friends_noself, "hp")
		for _, friend in ipairs(self.friends_noself) do
			if self:isFriend(friend) and LureTiger:targetFilter(players, friend, self.player) and self:aoeIsEffective(LureTiger, friend, self.player) then
				players:append(friend)
			end
		end
		for _, friend in ipairs(self.friends_noself) do
			if LureTiger:targetFilter(players, friend, self.player) and not players:contains(friend) and self:aoeIsEffective(LureTiger, friend, self.player) then
				players:append(friend)
			end
		end
		if players:length() > 0 then
			sgs.ai_use_priority.LureTiger = sgs.ai_use_priority.SavageAssault + 0.2
			use.card = LureTiger
			if use.to then
				for _, p in sgs.qlist(players) do
					use.to:append(self.room:findPlayer(p:objectName()))
				end
			end
			return
		end
	end

	players = sgs.PlayerList()

	card = self:getCard("Drowning")
	if card and card:isAvailable(self.player) and self:getAoeValue(card) > 0 then
		self:sort(self.enemies, "hp")
		for _, enemy in ipairs(self.enemies) do
			if self:needToThrowArmor(enemy) and enemy:getEquips():length() == 1 and self.player:canDiscard(enemy, enemy:getArmor():getId())
					and LureTiger:targetFilter(players, friend, self.player) and self:aoeIsEffective(LureTiger, friend, self.player) then
				players:append(friend)
			end
		end
		self:sort(self.friends_noself, "hp")
		for _, friend in ipairs(self.friends_noself) do
			if not self:needToThrowArmor(friend) and self:isFriend(friend) and not players:contains(friend)
					and LureTiger:targetFilter(players, friend, self.player) and self:aoeIsEffective(LureTiger, friend, self.player) then
				players:append(friend)
			end
		end
		for _, friend in ipairs(self.friends_noself) do
			if not self:needToThrowArmor(friend) and LureTiger:targetFilter(players, friend, self.player)
					and not players:contains(friend) and self:aoeIsEffective(LureTiger, friend, self.player) then
				players:append(friend)
			end
		end
		if players:length() > 0 then
			sgs.ai_use_priority.LureTiger = sgs.ai_use_priority.Drowning + 0.2
			use.card = LureTiger
			if use.to then
				for _, p in sgs.qlist(players) do
					use.to:append(self.room:findPlayer(p:objectName()))
				end
			end
			return
		end
	end

	players = sgs.PlayerList()

	card = self:getCard("Slash")
	if card and self:slashIsAvailable(self.player, card) then
		local dummyuse = { isDummy = true, to = sgs.SPlayerList() }
		self.player:setFlags("slashNoDistanceLimit")
		self:useCardSlash(card, dummyuse)
		self.player:setFlags("-slashNoDistanceLimit")
		if dummyuse.card then
			local total_num = 2 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, LureTiger)
			local function getPlayersFromTo(one)
				local targets1 = sgs.PlayerList()
				local targets2 = sgs.PlayerList()
				local nextp = self.player:getNextAlive()
				while true do
					if LureTiger:targetFilter(targets1, nextp, self.player) and self:hasTrickEffective(LureTiger, nextp, self.player) then
						if one:objectName() ~= nextp:objectName() then
							targets1:append(nextp)
						else
							break
						end
						nextp = nextp:getNextAlive()
					else
						targets1 = sgs.PlayerList()
						break
					end
				end
				nextp = one:getNextAlive()
				while true do
					if LureTiger:targetFilter(targets2, nextp, self.player) and self:hasTrickEffective(LureTiger, nextp, self.player) then
						if self.player:objectName() ~= nextp:objectName() then
							targets2:append(nextp)
						else
							break
						end
						nextp = nextp:getNextAlive()
					else
						targets2 = sgs.PlayerList()
						break
					end
				end
				if targets1:length() > 0 and targets2:length() >= targets1:length() and targets1:length() <= total_num then
					return targets1
				elseif targets2:length() > 0 and targets1:length() >= targets2:length() and targets2:length() <= total_num then
					return targets2
				end
				return
			end

			for _, to in sgs.qlist(dummyuse.to) do
				if self.player:distanceTo(to) > self.player:getAttackRange() and self.player:distanceTo(to, -total_num) <= self.player:getAttackRange() then
					local sps = getPlayersFromTo(to)
					if sps then
						sgs.ai_use_priority.LureTiger = 3
						use.card = LureTiger
						if use.to then
							for _, p in sgs.qlist(sps) do
								use.to:append(self.room:findPlayer(p:objectName()))
							end
						end
						return
					end
				end
			end
		end

	end

	players = sgs.PlayerList()

	card = self:getCard("GodSalvation")
	if card and card:isAvailable(self.player) then
		self:sort(self.enemies, "hp")
		for _, enemy in ipairs(self.enemies) do
			if LureTiger:targetFilter(players, enemy, self.player) and self:hasTrickEffective(LureTiger, enemy, self.player) then
				players:append(enemy)
			end
		end
		if players:length() > 0 then
			sgs.ai_use_priority.LureTiger = sgs.ai_use_priority.GodSalvation + 0.1
			use.card = LureTiger
			if use.to then
				for _, p in sgs.qlist(players) do
					use.to:append(self.room:findPlayer(p:objectName()))
				end
			end
			return
		end
	end

	players = sgs.PlayerList()

	if self.player:objectName() == self.room:getCurrent():objectName() then
		for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if LureTiger:targetFilter(players, player, self.player) and self:hasTrickEffective(LureTiger, player, self.player) then
				sgs.ai_use_priority.LureTiger = 0.3
				use.card = LureTiger
				if use.to then use.to:append(player) end
				return
			end
		end
	end
end

sgs.ai_nullification.LureTiger = function(self, card, from, to, positive)
	return false
end

sgs.ai_use_value.LureTiger = 5
sgs.ai_use_priority.LureTiger = 4.9
sgs.ai_keep_value.LureTiger = 3.22

sgs.ai_skill_invoke.control_rod = function(self, data)
	local target = data:toPlayer()
	return not self:isFriend(target)
end

sgs.ai_choicemade_filter.skillInvoke.control_rod = function(self, player, promptlist)
	if promptlist[#promptlist] == "yes" then
		local target = findPlayerByObjectName(self.room, promptlist[#promptlist - 1])
		if target then sgs.updateIntention(player, target, 50) end
	end
end

local scroll_skill = {}
scroll_skill.name = "scroll"
table.insert(sgs.ai_skills, scroll_skill)
scroll_skill.getTurnUseCard = function(self)
	-- change treasure
	local cards = sgs.QList2Table(self.player:getHandcards())
	for _, c in sgs.qlist(getWoodenOxPile(self.player)) do
		table.insert(cards, sgs.Sanguosha:getCard(c))
	end
	local other_treasure = self:getCard("Treasure", self.player)
	if other_treasure and (self.player:getTreasure() and self.player:getTreasure():getId() ~= other_treasure:getId()) then
		return sgs.Card_Parse("@ScrollCard=.")
	end
	
	-- friend wooden_ox
	for _, p in ipairs(self.friends_noself) do
		if p:hasTreasure("wooden_ox") then
			return sgs.Card_Parse("@ScrollCard=.")
		end
	end

	if self.player:hasSkill("thhouzhi") then return end
	if self.player:isKongcheng() or self:isWeak(self.player) then
		return sgs.Card_Parse("@ScrollCard=.")
	end
end

sgs.ai_skill_use_func.ScrollCard = function(card, use, self)
	use.card = card
end

function SmartAI:useCardKnownBoth(KnownBoth, use)
	if not KnownBoth:isAvailable(self.player) then return false end
	sgs.ai_use_priority.KnownBoth = 9.1
	local targets = sgs.PlayerList()
	local total_num = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, KnownBoth)
	if self.player:hasFlag("ThChouceUse") then total_num = 1 end

	self:sort(self.enemies, "handcard")
	sgs.reverse(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if KnownBoth:targetFilter(targets, enemy, self.player) and enemy:getHandcardNum() - getKnownNum(enemy) > 3 and not targets:contains(enemy)
				and self:hasTrickEffective(KnownBoth, enemy, self.player) then
			use.card = KnownBoth
			targets:append(enemy)
			if use.to then use.to:append(enemy) end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if KnownBoth:targetFilter(targets, enemy, self.player) and enemy:getHandcardNum() - getKnownNum(enemy) > 0 and not targets:contains(enemy)
				and self:hasTrickEffective(KnownBoth, enemy, self.player) then
			use.card = KnownBoth
			targets:append(enemy)
			if use.to then use.to:append(enemy) end
		end
	end
	self:sort(self.friends_noself, "handcard")
	self.friends_noself = sgs.reverse(self.friends_noself)
	if use.card then
		for _, friend in ipairs(self.friends_noself) do
			if getKnownNum(friend) ~= friend:getHandcardNum() and KnownBoth:targetFilter(targets, friend, self.player) and not targets:contains(friend)
					and self:hasTrickEffective(KnownBoth, friend, self.player) then
				targets:append(friend)
				if use.to then use.to:append(friend) end
			end
		end
	end
	if self.player:objectName() == self.room:getCurrent():objectName() then
		for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if KnownBoth:targetFilter(targets, player, self.player) and self:hasTrickEffective(KnownBoth, player, self.player) then
				sgs.ai_use_priority.KnownBoth = 0.3
				use.card = KnownBoth
				targets:append(player)
				if use.to then
					use.to:append(player)
				end
				return
			end
		end
	end
end

sgs.ai_nullification.KnownBoth = function(self, card, from, to, positive)
	return false
end
sgs.ai_use_value.KnownBoth = 5.5
sgs.ai_keep_value.KnownBoth = 3.33

function SmartAI:useCardRout(card, use)
	local usecard = false
	local targets = {}
	local targets_num = 1 + sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_ExtraTarget, self.player, card)
	if self.player:hasFlag("ThChouceUse") then
		targets_num = 1
	end

	local addTarget = function(player)
		if not table.contains(targets, player:objectName())
			and (not use.current_targets or not table.contains(use.current_targets, player:objectName()))
			and not (use.to and use.to:length() > 0 and player:hasSkill("danlao")) then
			if not usecard then
				use.card = card
				usecard = true
			end
			table.insert(targets, player:objectName())
			if usecard and use.to and use.to:length() < targets_num then
				use.to:append(player)
			end
			if #targets == targets_num then return true end
		end
	end

	local victims = self:findPlayerToDiscard("e", false, true, nil, true, "Weapon,Armor,Horse")
	for _, p in ipairs(victims) do
		if self:hasTrickEffective(card, p) then
			addTarget(p)
		end
	end
end

sgs.ai_use_value.Rout = 5.6
sgs.ai_use_priority.Rout = 4.4
sgs.ai_keep_value.Rout = 3.18

sgs.ai_skill_choice.rout = function(self, choices, data)
	local target = data:toPlayer()
	local id = self:askForCardChosen(target, "e", "", sgs.Card_MethodDiscard)
	if target:getWeapon() and target:getWeapon():getEffectiveId() == id then
		return "weapon"
	end
	if target:getArmor() and target:getArmor():getEffectiveId() == id then
		return "armor"
	end
	if target:getOffensiveHorse() and target:getOffensiveHorse():getEffectiveId() == id then
		return "weapon"
	end
	if target:getDefensiveHorse() and target:getDefensiveHorse():getEffectiveId() == id then
		return "armor"
	end
	return string.find(choices, "weapon") and "weapon" or "armor"
end

sgs.ai_choicemade_filter.skillChoice.rout = function(self, player, promptlist)
	local target = player:getTag("RoutTarget"):toPlayer()
	if target then
		local result =  promptlist[#promptlist]
		if result == "weapon" then
			if target:getWeapon() and self.player:canDiscard(target, target:getWeapon():getEffectiveId()) then
				sgs.ai_choicemade_filter.cardChosen.snatch(self, player, {"cardChosen", "dismantlement", tostring(target:getWeapon():getEffectiveId()), self.player:objectName(), target:objectName()})
			end
			if target:getOffensiveHorse() and self.player:canDiscard(target, target:getOffensiveHorse():getEffectiveId()) then
				sgs.ai_choicemade_filter.cardChosen.snatch(self, player, {"cardChosen", "dismantlement", tostring(target:getOffensiveHorse():getEffectiveId()), self.player:objectName(), target:objectName()})
			end
		elseif result == "armor" then
			if target:getArmor() and self.player:canDiscard(target, target:getArmor():getEffectiveId()) then
				sgs.ai_choicemade_filter.cardChosen.snatch(self, player, {"cardChosen", "dismantlement", tostring(target:getArmor():getEffectiveId()), self.player:objectName(), target:objectName()})
			end
			if target:getDefensiveHorse() and self.player:canDiscard(target, target:getDefensiveHorse():getEffectiveId()) then
				sgs.ai_choicemade_filter.cardChosen.snatch(self, player, {"cardChosen", "dismantlement", tostring(target:getDefensiveHorse():getEffectiveId()), self.player:objectName(), target:objectName()})
			end
		end	
	end
end

sgs.ai_view_as.jade = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place == sgs.Player_PlaceEquip and player:getTreasure() and player:getTreasure():getEffectiveId() == card_id then
		return ("nullification:jade[%s:%s]=%d"):format(suit, number, card_id)
	end
end

sgs.ai_skill_use["@@jade"] = function(self, prompt)
	local card_str = sgs.GetProperty(self.player, "jade_trick")
	local from = self.room:findPlayer(sgs.GetProperty(self.player, "jade_trick_from"))
	local trick = sgs.Card_Parse(card_str)
	local target_table = {}
	if trick then
		for _, p in sgs.qlist(self.room:getAlivePlayers()) do
			if p:getMark("cardEffect_" .. card_str) > 0 then
				if self:askForNullification(trick, from, p, true, true) then
					table.insert(target_table, p:objectName())
				end
			end
		end
	end
	if #target_table > 0 then
		return "@JadeCard=.->" .. table.concat(target_table, "+")
	end
end

sgs.ai_card_intention.JadeCard = function(self, card, from, tos)
	local cardx = sgs.Card_Parse(sgs.GetProperty(from, "jade_trick"))
	if not cardx then return end
	local intention = (cardx:isKindOf("AOE") and -50 or 50)
	for _, to in ipairs(tos) do
		if to:hasSkill("danlao") or not self:hasTrickEffective(cardx, to, from) then continue end
		if cardx:isKindOf("GodSalvation") and not (to:isWounded() or to:isChained()) then continue end
		sgs.updateIntention(from, to, intention)
	end
end

function SmartAI:useCardPurpleSong(card, use)
	local friends = {}
	if #self.friends_noself ~= 0 then
		friends = self:exclude(self.friends_noself, card)
	end
	if self.player:hasFlag("ThChouceUse") then
		table.insert(friends, self.player)
	end
	if #friends == 0 then return end

	local zhanghe = self.room:findPlayerBySkillName("qiaobian")
	local zhanghe_seat = zhanghe and zhanghe:faceUp() and not self:isFriend(zhanghe) and zhanghe:getSeat() or 0

	local getvalue = function(friend)
		if friend:containsTrick("purple_song") or friend:containsTrick("YanxiaoCard") then
			return -100
		end

		if zhanghe_seat > 0 and (self:playerGetRound(zhanghe) <= self:playerGetRound(friend) or not friend:faceUp()) then
			return -100
		end

		local value = friend:getHandcardNum() - friend:getHp()

		if friend:hasSkills("noslijian|lijian|fanjian|nosfanjian|dimeng|jijiu|jieyin|anxu|yongsi|ikzhiheng|manjuan|nosrende|ikshenai|ikkuipo|jixi") then value = value + 10 end
		if friend:hasSkills("qice|nosguose|guose|duanliang|nosjujian|ikmengyang|ikhuiquan|jizhi|jilve|wansha|mingce") then value = value + 5 end
		if friend:hasSkills("guzheng|luoying|yinling|iklingshi|shenfen|ganlu|duoshi") then value = value + 3 end
		if self:isWeak(friend) then value = value + 3 end
		if friend:isLord() then value = value + 3 end

		if self:objectiveLevel(friend) < 3 then value = value - 10 end
		if not friend:faceUp() then value = value - 10 end
		if friend:hasSkills("keji|shensu|thanbing") then value = value - friend:getHandcardNum() end
		if friend:hasSkills("ikxushi|xiuluo") then value = value - 5 end
		if friend:hasSkills("lirang") then value = value - 5 end
		if friend:hasSkills("ikchibao|nostuxi|noszhenlie|ikxushi|qinyin|zongshi|tiandu|thzhiji|thchuiji") then value = value - 3 end
		if self:needBear(friend) then value = value - 20 end
		value = value + (self.room:alivePlayerCount() - self:playerGetRound(friend)) / 2
		return value
	end

	local cmp = function(a, b)
		return getvalue(a) > getvalue(b)
	end

	table.sort(friends, cmp)

	local target = friends[1]
	if not target then
		self.room:writeToConsole("no Target!")
	end
	if not target:inherits("ServerPlayer") then
		self.room:writeToConsole("not ServerPlayer!")
		if target:inherits("Player") then
			self.room:writeToConsole("is Player!")
		end
	end
	if getvalue(target) > -100 then
		use.card = card
		if use.to then use.to:append(target) end
		return
	end
end

sgs.ai_use_value.PurpleSong = 10
sgs.ai_use_priority.PurpleSong = 0.6
sgs.ai_keep_value.PurpleSong = 3
sgs.ai_card_intention.PurpleSong = -80

sgs.ai_skill_playerchosen.moon_spear = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	for _, target in ipairs(targets) do
		if self:isEnemy(target) and self:damageIsEffective(target) and sgs.isGoodTarget(target, targets, self) then
			return target
		end
	end
	return nil
end

sgs.ai_playerchosen_intention.moon_spear = 80

sgs.weapon_range.MoonSpear = 3
sgs.ai_use_priority.MoonSpear = 2.635

function SmartAI:useCardReinforce(card, use)
	local target = self:findPlayerToDraw(true, 3, card)
	if target then
		use.card = card
		if use.to then
			use.to:append(target)
		end
	end
end

sgs.ai_skill_discard.reinforce = function(self, discard_num, min_num, optional, include_equip, pattern)
	if pattern == "^BasicCard" then
		local ret = self:askForDiscard("", discard_num, min_num, false, include_equip, pattern)
		if #ret > 0 then
			if not self:isValuableCard(ret[1]) then
				return ret
			else
				local ret2 = self:askForDiscard("", 2, 2, false, true)
				if #ret2 == 2 and not self:isValuableCard(ret2[1]) and not self:isValuableCard(ret2[2]) then
					if not sgs.Sanguosha:getCard(ret2[1]):isKindOf("BasicCard") then
						return {ret2[1]}
					elseif not sgs.Sanguosha:getCard(ret2[2]):isKindOf("BasicCard") then
						return {ret2[2]}
					else
						return {}
					end
				else
					return ret
				end
			end
		end
	else
		return self:askForDiscard("", discard_num, min_num, optional, include_equip, pattern)
	end
end

sgs.ai_card_intention.Reinforce = -80

sgs.ai_keep_value.Reinforce = 3.6
sgs.ai_use_value.Reinforce = 10
sgs.ai_use_priority.Reinforce = 9.3

sgs.dynamic_value.benefit.Reinforce = true

function SmartAI:useCardBurningCamps(card, use)
	if not card:isAvailable(self.player) then return end
	if self.player:hasFlag("ThChouceUse") then
		local targetlist = sgs.QList2Table(self.room:getAllPlayers())
		self:sort(targetlist, "hp")
		local victims = {}
		for _, target in ipairs(targetlist) do
			if self:isEnemy(target) and self:damageIsEffective(target, sgs.DamageStruct_Fire, self.player) and self:hasTrickEffective(card, target, self.player) then
				table.insert(victims, target)
			end
		end
		local to = nil
		if #victims ~= 0 then
			for _, p in ipairs(victims) do
				if p:isChained() and self:isGoodChainTarget(p, self.player, sgs.DamageStruct_Fire, 1) then
					to = p
					break
				end
			end
			if not to then
				to = victim[1]
			end
		end
		if to then
			use.card = card
			if use.to then
				use.to:append(to)
			end
		end
		return
	end

	local player = self.room:findPlayer(self.player:getNextAlive():objectName())
	local players = player:getFormation()
	if players:isEmpty() then return end

	local shouldUse = 0
	for i = 0 , players:length() - 1, 1 do
		player = self.room:findPlayer(players:at(i):objectName())
		if not self:hasTrickEffective(card, player, self.player) then
			continue
		end
		if self:isFriend(player) then
			shouldUse = shouldUse - 1.5
		end
		local damage = {}
		damage.from = self.player
		damage.to = player
		damage.nature = sgs.DamageStruct_Fire
		damage.damage = 1
		if self:damageIsEffective_(damage) then
			if player:isChained() and self:isGoodChainTarget(damage.to, damage.from, damage.nature, damage.damage, card) then
				shouldUse = shouldUse + 2
			elseif self:objectiveLevel(player) > 3 then
				shouldUse = shouldUse + 1
			else
				shouldUse = shouldUse - 0.5
			end
		end
	end
	if shouldUse > 0 then
		use.card = card
	end
end

sgs.ai_nullification.BurningCamps = function(self, card, from, to, positive, keep)
	local targets = sgs.SPlayerList()
	local players = sgs.SPlayerList()
	for _, q in sgs.qlist(self.room:getAlivePlayers()) do
		if q:getMark("cardEffect_" .. card:toString()) > 0 then
			targets:append(q)
		end
	end
	if positive then
		if from:objectName() == self.player:objectName() then return false end
		local chained = {}
		local dangerous
		if self:damageIsEffective(to, sgs.DamageStruct_Fire) and to:isChained() and not from:hasSkill("ikxuwu") then
			for _, p in sgs.qlist(self.room:getOtherPlayers(to)) do
				if not self:isGoodChainTarget(to, p, sgs.DamageStruct_Fire) and self:damageIsEffective(p, sgs.DamageStruct_Fire) and self:isFriend(p) then
					table.insert(chained, p)
					if self:isWeak(p) then dangerous = true end
				end
			end
		end
		if to:hasArmorEffect("vine") and #chained > 0 then dangerous = true end
		local friends = {}
		if self:isFriend(to) then
			for _, p in sgs.qlist(targets) do
				if self:damageIsEffective(p, sgs.DamageStruct_Fire) then
					table.insert(friends, p)
					if self:isWeak(p) or p:hasArmorEffect("vine") then dangerous = true end
				end
			end
		end
		if #chained + #friends > 2 or dangerous then return true, #friends <= 1 end
		if keep then return false end
		if self:isFriend(to) and self:isEnemy(from) then return true, #friends <= 1 end
	else
		if not self:isFriend(from) then return false end
		local chained = {}
		local dangerous
		local enemies = {}
		local good
		if self:damageIsEffective(to, sgs.DamageStruct_Fire) and to:isChained() and not from:hasSkill("ikxuwu") then
			for _, p in sgs.qlist(self.room:getOtherPlayers(to)) do
				if not self:isGoodChainTarget(to, p, sgs.DamageStruct_Fire) and self:damageIsEffective(p, sgs.DamageStruct_Fire) and self:isFriend(p) then
					table.insert(chained, p)
					if self:isWeak(p) then dangerous = true end
				end
				if not self:isGoodChainTarget(to, p, sgs.DamageStruct_Fire) and self:damageIsEffective(p, sgs.DamageStruct_Fire) and self:isEnemy(p) then
					table.insert(enemies, p)
					if self:isWeak(p) then good = true end
				end
			end
		end
		if to:hasArmorEffect("vine") and #chained > 0 then dangerous = true end
		if to:hasArmorEffect("vine") and #enemies > 0 then good = true end
		local friends = {}
		if self:isFriend(to) then
			for _, p in sgs.qlist(targets) do
				if self:damageIsEffective(p, sgs.DamageStruct_Fire) then
					table.insert(friends, p)
					if self:isWeak(p) or p:hasArmorEffect("vine") then dangerous = true end
				end
			end
		end
		if self:isEnemy(to) then
			for _, p in sgs.qlist(targets) do
				if self:damageIsEffective(p, sgs.DamageStruct_Fire) then
					if self:isWeak(p) or p:hasArmorEffect("vine") then good = true end
				end
			end
		end
		if #chained + #friends > 2 or dangerous then return false end
		if keep then
			local nulltype = card:isKindOf("Nullification") and card:getSkillName() == "jade"
			if nulltype and targets:length() > 1 then good = true end
			if good then keep = false end
		end
		if keep then return false end
		if self:isFriend(from) and self:isEnemy(to) then return true, true end
	end
	return
end

sgs.ai_use_value.BurningCamps = 7.1
sgs.ai_use_priority.BurningCamps = 4.7
sgs.ai_keep_value.BurningCamps = 3.38
sgs.ai_card_intention.BurningCamps = 10
sgs.dynamic_value.damage_card.BurningCamps = true

sgs.ai_skill_invoke.breastplate = true

sgs.ai_skill_choice.drowning = function(self, choices, data)
	local effect = data:toCardEffect()
	if self:needKongcheng(self.player) and self.player:getHandcardNum() == 2 and string.find(choices, "discard") then
		return "discard"
	end
	if self:needToThrowArmor(self.player) and string.find(choices, "throw")
			and (self.player:getEquips():length() == 1 or (sgs.ai_role[self.player:objectName()] ~= "neutral" and self:isFriend(effect.from))) then
		return "throw"
	end
	if self.player:hasSkills(sgs.lose_equip_skill) and string.find(choices, "throw") then
		return "throw"
	end

	if not self:damageIsEffective(nil, nil, effect.from) then return "damage" end
	if self:getDamagedEffects(self.player, effect.from) or self:needToLoseHp(self.player, effect.from) then return "damage" end
	if self.player:hasSkill("ikmitu") and not effect.from:hasSkill("ikxuwu") then return "damage" end
	if effect.from:hasSkill("ikmitu") and not effect.from:hasSkill("ikxuwu") then return "damage" end
	if self.player:getMark("@shine") > 0 and not effect.from:hasSkill("ikxuwu") then return "damage" end

	local peaches = self:getCardsNum("Peach")
	if self.player:getHp() < 3 then
		peaches = peaches + self:getCardsNum("Analeptic")
	end
	if peaches / self.player:getHandcardNum() < 0.25 then
		if string.find(choices, "discard") then
			return "discard"
		end
	elseif peaches > 0 then
		return "damage"
	end
	if self:isWeak(self.player) then
		return string.find(choices, "discard") and "discard" or (string.find(choices, "throw") and "throw" or "damage")
	end
	return string.find(choices, "throw") and "throw" or "damage"
end

sgs.ai_choicemade_filter.cardChosen.drowning = sgs.ai_choicemade_filter.cardChosen.snatch

sgs.ai_use_value.Drowning = 3.7
sgs.ai_use_priority.Drowning = 3.5
sgs.ai_keep_value.Drowning = 3.63
sgs.dynamic_value.damage_card.Drowning = true

local wooden_ox_skill = {}
wooden_ox_skill.name = "wooden_ox"
table.insert(sgs.ai_skills, wooden_ox_skill)
wooden_ox_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("WoodenOxCard") or self.player:isKongcheng() or not self.player:hasTreasure("wooden_ox") then return end
	self.wooden_ox_assist = nil
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards, true)
	local card, friend = self:getCardNeedPlayer(cards)
	if card and friend and friend:objectName() ~= self.player:objectName() and (self:getOverflow() > 0 or self:isWeak(friend)) then
		self.wooden_ox_assist = friend
		return sgs.Card_Parse("@WoodenOxCard=" .. card:getEffectiveId())
	end
	if self:getOverflow() > 0 or (self:needKongcheng() and #cards == 1) then
		return sgs.Card_Parse("@WoodenOxCard=" .. cards[1]:getEffectiveId())
	end
end

sgs.ai_skill_use_func.WoodenOxCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_playerchosen.wooden_ox = function(self, targets)
	if self.wooden_ox_assist and not self.wooden_ox_assist:getTreasure() then return self.wooden_ox_assist end
	if self.player:hasSkill("yongsi") then
		local kingdoms = {}
		for _, p in sgs.qlist(self.room:getAlivePlayers()) do
			local kingdom = p:getKingdom()
			if not table.contains(kingdoms, kingdom) then table.insert(kingdoms, kingdom) end
		end
		if self.player:getCardCount(true) <= #kingdoms then
			self:sort(self.friends_noself)
			for _, friend in ipairs(self.friends_noself) do
				if not friend:getTreasure() then return friend end
			end
		end
	end
end

sgs.ai_playerchosen_intention.wooden_ox = -60
