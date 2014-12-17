sgs.ai_skill_choice.thzhiji = function(self, choices, data)
	if (self:isWeak() or self:needKongcheng(self.player, true)) and string.find(choices, "recover") then
		return "recover"
	else
		return "draw"
	end
end

local thjiyi_skill = {}
thjiyi_skill.name = "thjiyi"
table.insert(sgs.ai_skills, thjiyi_skill)
thjiyi_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("ThJiyiCard") then
		return sgs.Card_Parse("@ThJiyiCard=.")
	end
end

sgs.ai_skill_use_func.ThJiyiCard = function(card, use, self)
	for _, p in ipairs(self.enemies) do
		if self:isWeak(p) and getKnownCard(p, self.player, "TrickCard") == 0 then
			use.card = sgs.Card_Parse("@ThJiyiCard=.")
			if use.to then
				use.to:append(p)
			end
			return
		end
	end --weak enemy

	for _, p in ipairs(self.friends_noself) do
		if p:getPile("wooden_ox"):isEmpty() and getKnownCard(p, self.player, "TrickCard") > 0 then
			if not self:needKongcheng(self.player, true) then
				use.card = sgs.Card_Parse("@ThJiyiCard=.")
				if use.to then
					use.to:append(p)
				end
				return
			end
		end
	end --friend who has known trick

	for _, p in ipairs(self.enemies) do
		if getKnownCard(p, self.player, "TrickCard") == 0 then
			use.card = sgs.Card_Parse("@ThJiyiCard=.")
			if use.to then
				use.to:append(p)
			end
			return
		end
	end --enemys
end

sgs.ai_cardshow.thjiyi = function(self, requestor)
	local trick = nil
	local cards = sgs.QList2Table(self.player:getHandcards())
	for _, cd in ipairs(cards) do
		if cd:isKindOf("TrickCard") then
			trick = cd
			break
		end
	end
	if trick then
		return trick
	end
	self:sortByKeepValue(cards)
	return cards[1]
end

sgs.ai_skill_cardask["@thjiyi"] = function(self, data, pattern, target)
	if self:isFriend(target) then
		return "."
	end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, cd in ipairs(cards) do
		if cd:isKindOf("TrickCard") then
			return "$"..cd:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_card_intention.ThJiyiCard = function(self, card, from, to)
	if getKnownCard(to[1], from, "TrickCard") > 0 then
		sgs.updateIntention(from, to[1], -50)
		return
	end
	sgs.updateIntention(from, to[1], 50)
end

--thhuadi @to_do

sgs.ai_skill_invoke.thjilanwen = function(self, data)
	for _, target in sgs.qlist(self.room:getAllPlayers()) do
		if self:isFriend(target) then
			if (target:hasSkill("ikcangyou") and not target:getEquips():isEmpty()) or self:needToThrowArmor(target) then
				return true
			end
			if target:containsTrick("indulgence") or target:containsTrick("supply_shortage") then
				return true
			end
		end
		if self:isEnemy(target) then
			if target:hasSkills("ikyindie+ikguiyue") and target:getPhase() == sgs.Player_NotActive then return false end
			if not target:getEquips():isEmpty() then
				return true
			else
				return false
			end
		end
	end
	return true
end

sgs.ai_skill_playerchosen.thjilanwen = function(self, targets)
	local judge = self.player:getTag("ThJilanwenJudge"):toJudge()
	local suit = tonumber(judge.pattern)
	for _, p in ipairs(self.friends_noself) do
		if not targets:contains(p) then
			continue
		end
		if p:hasSkill("ikcangyou") then
			for _, cd in sgs.qlist(p:getEquips()) do
				if cd:getSuit() ~= suit then
					return p
				end
			end
		end
		if self:needToThrowArmor(p) then
			if p:getArmor() and p:getArmor():getSuit() ~= suit then
				return p
			end
		end
		for _, cd in sgs.qlist(p:getJudgingArea()) do
			if cd:isKindOf("Indulgence") or cd:isKindOf("SupplyShortage") then
				return p
			end
		end
	end
	for _, p in ipairs(self.enemies) do
		if not targets:contains(p) then
			continue
		end
		if p:hasSkills("ikyindie+ikguiyue") and p:getPhase() == sgs.Player_NotActive then
			continue
		end
		local equips = sgs.IntList()
		for _, cd in sgs.qlist(p:getEquips()) do
			equips:append(cd:getEffectiveId())
		end
		if p:getArmor() and self:needToThrowArmor(p) then
			equips:removeOne(p:getArmor():getEffectiveId())
		end
		for _, id in sgs.qlist(equips) do
			if sgs.Sanguosha:getCard(id):getSuit() ~= suit then
				return p
			end
		end
	end
	return nil
end

local thnianke_skill = {}
thnianke_skill.name = "thnianke"
table.insert(sgs.ai_skills, thnianke_skill)
thnianke_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThNiankeCard") then
		return nil
	end
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)

	local card

	self:sortByUseValue(cards, true)

	for _, acard in ipairs(cards) do
		if acard:isKindOf("Jink") then
			card = acard
			break
		end
	end

	if not card then
		return nil
	end

	local card_id = card:getEffectiveId()
	local card_str = "@ThNiankeCard=" .. card_id
	local skillcard = sgs.Card_Parse(card_str)

	assert(skillcard)
	return skillcard
end

sgs.ai_skill_use_func.ThNiankeCard = function(card, use, self)
	local targets_red = {}
	local targets = {}
	for _, friend in ipairs(self.friends_noself) do
		if (friend:isKongcheng() and not self:needKongcheng(friend, true)) or getKnownCard(friend, self.player, "red") == friend:getHandcardNum() then
			table.insert(targets_red, friend)
		else
			table.insert(targets, friend)
		end
	end
	if #targets_red > 0 then
		use.card = card
		self:sort(targets_red, "defense")
		if use.to then
			use.to:append(targets_red[1])
		end
	end
	if self:getCardsNum("Jink", "h") <= 1 then
		return "."
	end
	if #targets > 0 then
		use.card = card
		self:sort(targets, "defense")
		if use.to then
			use.to:append(targets[1])
		end
	end
end

sgs.ai_card_intention.ThNiankeCard = -80

sgs.ai_skill_playerchosen.thjilan = function(self, targets)
	for _, p in ipairs(self.friends) do
		if p:getArmor() and self:needToThrowArmor(p) and p:canDiscard(p, p:getArmor():getEffectiveId()) and p:getLostHp() <= 1 then
			return p
		end
	end
	local enemies = {}
	for _, p in ipairs(self.enemies) do
		if p:canDiscard(p, "he") then
			table.insert(enemies, p)
		end
	end
	self:sort(enemies, "losthp")
	enemies = sgs.reverse(enemies)
	for _, enemy in ipairs(enemies) do
		if self:isWeak(enemy) then
			return enemy
		end
	end
	return enemies[1]
end

sgs.ai_skill_invoke.thwangshou = function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		return (target:hasSkill("ikcangyou") and not target:getEquips():isEmpty()) or self:needToThrowArmor(target)
	end
	return self:isEnemy(target)
end

sgs.ai_skill_cardask["@thzhanye"] = function(self, data, pattern, target)
	for _, slash in ipairs(self:getCards("Slash")) do
		if self:isFriend(target) and self:slashIsEffective(slash, target) then
			if self:findLeijiTarget(target, 50, self.player) then return slash:toString() end
			if self:getDamagedEffects(target, self.player, true) then return slash:toString() end
		end

		local nature = sgs.DamageStruct_Normal
		if slash:isKindOf("FireSlash") then nature = sgs.DamageStruct_Fire
		elseif slash:isKindOf("ThunderSlash") then nature = sgs.DamageStruct_Thunder end
		if self:isEnemy(target) and self:slashIsEffective(slash, target) and self:canAttack(target, self.player, nature)
			and not self:getDamagedEffects(target, self.player, true) and not self:findLeijiTarget(target, 50, self.player) then
			return slash:toString()
		end
	end
	return "."
end

local thenan_skill = {}
thenan_skill.name = "thenan"
table.insert(sgs.ai_skills, thenan_skill)
thenan_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ThEnanCard") then
		return nil
	end

	local skillcard = sgs.Card_Parse("@ThEnanCard=.")
	assert(skillcard)
	return skillcard
end

sgs.ai_skill_use_func.ThEnanCard = function(card, use, self)
	if self.player:getHandcardNum() <= 1 and self.player:getHp() == 1 and self.player:getMaxHp() > 4 and self:getCardsNum({"Peach", "Analeptic"}, "h") > 0 then
		use.card = card
		if use.to then
			use.to:append(self.player)
		end
		return
	end
	for _, enemy in ipairs(self.enemies) do
		if self.player:inMyAttackRange(enemy) and self:isWeak(enemy) then
			use.card = card
			if use.to then
				use.to:append(enemy)
			end
			return
		end
	end
	return "."	
end

sgs.ai_card_intention.ThEnanCard = 80

sgs.ai_skill_choice.thbeiyun = function(self, choices, data)
	if string.find(choices, "get") then
		return "get"
	end
	local beiyun_ids = data:toIntList()
	local red, black, not_red, not_black = false, false, false, false
	for _, id in sgs.qlist(beiyun_ids) do
		local card = sgs.Sanguosha:getCard(id)
		if card:isRed() then
			if not red and card:getSuit() == sgs.Card_Diamond then
				red = true
			end
			if card:isKindOf("Peach") or card:isKindOf("Analeptic") then
				not_red = true
			end
		elseif card:isBlack() then
			if not black then
				black = true
			end
			if card:isKindOf("Peach") or card:isKindOf("Analeptic") then
				not_black = true
			end
		end
	end
	if not not_red and red then
		return "red"
	end
	if self:getCardsNum({"Peach", "Analeptic"}, "h") >= 1 - self.player:getHp() then -- i'm safe
		return black and "black" or "cancel"
	else
		return not not_black and black and "black" or "cancel"
	end
	return "cancel"
end


--thmicai @to_do
--[[ thqiaogong -future @to_do_future
local thqiaogong = {}
thqiaogong.name = "thqiaogong"
table.insert(sgs.ai_skills, thqiaogong)
thqiaogong.getTurnUseCard = function(self)
	if self.player:hasUsed("ThQiaogong") then
		return nil
	end

	local skillcard = sgs.Card_Parse("@ThQiaogong=.")
	assert(skillcard)
	return skillcard
end

sgs.ai_skill_use_func.ThQiaogong = function(card, use, self)
	
end]]

local thzhouhua_skill = {}
thzhouhua_skill.name = "thzhouhua"
table.insert(sgs.ai_skills, thzhouhua_skill)
thzhouhua_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)
	for _, acard in ipairs(cards) do
		if isCard("Analeptic", acard, self.player) then return end
	end
	self:sortByUseValue(cards)
	local newcards = {}
	local has_slash = false
	for _, card in ipairs(cards) do
		if self:getCardsNum("Slash") == 1 and isCard("Slash", card, self.player) then
			continue
		end
		if self:getCardsNum("Slash") == 2 and isCard("Slash", card, self.player) and has_slash then
			continue
		end
		if not isCard("Analeptic", card, self.player) and not isCard("Peach", card, self.player) and not (isCard("ExNihilo", card, self.player) and self.player:getPhase() == sgs.Player_Play) then
			if isCard("Slash", card, self.player) then
				has_slash = true
			end
			table.insert(newcards, card)
		end
	end
	if #newcards <= self.player:getHp() - 1 and self.player:getHp() <= 4 and self:needKongcheng()
		and not (self.player:hasSkill("ikshengtian") and self.player:getMark("@shengtian") == 0) then return end
	if #newcards < 2 then return end

	local card_id1 = newcards[1]:getEffectiveId()
	local card_id2 = newcards[2]:getEffectiveId()

	local card_str = ("analeptic:%s[%s:%s]=%d+%d"):format("thzhouhua", "to_be_decided", 0, card_id1, card_id2)
	local analeptic = sgs.Card_Parse(card_str)
	return analeptic
end

function cardsView_thzhouhua(self, player)
	local cards = player:getCards("he")
	for _, id in sgs.qlist(player:getPile("wooden_ox")) do
		cards:prepend(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)
	for _, acard in ipairs(cards) do
		if isCard("Analeptic", acard, player) then return end
	end
	local newcards = {}
	for _, card in ipairs(cards) do
		if not isCard("Analeptic", card, player) and not isCard("Peach", card, player) and not (isCard("ExNihilo", card, player) and player:getPhase() == sgs.Player_Play) then
			table.insert(newcards, card)
		end
	end
	if #newcards < 2 then return end
	sgs.ais[player:objectName()]:sortByKeepValue(newcards)
	
	local card_id1 = newcards[1]:getEffectiveId()
	local card_id2 = newcards[2]:getEffectiveId()
	
	local card_str = ("analeptic:%s[%s:%s]=%d+%d"):format("thzhouhua", "to_be_decided", 0, card_id1, card_id2)
	return card_str
end

function sgs.ai_cardsview.thzhouhua(self, class_name, player)
	if class_name == "Analeptic" and player:getPhase() ~= sgs.Player_NotActive then
		return cardsView_thzhouhua(self, player)
	end
end

function sgs.ai_cardsview.thzhouhuav(self, class_name, player)
	if class_name == "Analeptic" then
		local obj_name = player:property("zhouhua_source"):toString()
		local splayer = self.room:findPlayer(obj_name)
        if splayer and splayer:hasSkill("thzhouhua") then
            return cardsView_thzhouhua(self, player)
		end
	end
end

sgs.ai_skill_playerchosen["@thxugu"] = sgs.ai_skill_playerchosen.zero_card_as_slash

sgs.thshenzhou_current = false

sgs.ai_skill_choice.thshenzhou = function(self, choices, data)
	local card_ids = data:toIntList()
	local has_peach = false
	local num = {basic = 0,
				 equip = 0,
				 trick = 0}
	for _, id in sgs.qlist(card_ids) do
		local card = sgs.Sanguosha:getCard(card_ids)
		if card:isKindOf("Peach") then
			has_peach = true
		end
		num[card:getType()] = num[card:getType()] + 1
	end
	self:sort(self.friends, "defense")
	if has_peach then
		if self:isWeak(self.friends[1]) then
			return "basic"
		end
	end
	if num.basic > num.trick then
		return "basic"
	end
	local current = self.room:getCurrent()
	if current and current:isAlive() and self:isFriend(current) and current:getPhase() ~= sgs.Player_NotActive and current:getPhase() <= sgs.Player_Play then
		sgs.thshenzhou_current = true
		if num.trick > num.basic then
			return "trick"
		elseif num.basic > 0 then
			return "basic"
		else
			return num.trick > num.equip and "trick" or "equip"
		end
	end
	if num.basic > num.trick then
		return "basic"
	elseif num.trick > num.basic then
		return "trick"
	elseif num.basic > 0 then
		local n = math.random(1, 3)
		if n == 1 then
			return "trick"
		else
			return "basic"
		end
	else
		local choice_list = choices:split("+")
		return choice_list[math.random(1, #choice_list)]
	end
end

sgs.ai_skill_playerchosen.thshenzhou = function(self, targets)
	local true_target = nil
	if sgs.thshenzhou_current then
		sgs.thshenzhou_current = false
		true_target = self.room:getCurrent()
	else
		self:sort(self.friends, "defense")
		true_target = self.friends[1]
	end
	local n = math.random(1, 5)
	if n > 2 then
		return true_target
	elseif n == 2 then
		return self.player
	else
		return self.friends[math.random(1, #self.friends)]
	end
end

sgs.ai_playerchosen_intention.thshenzhou = -80

local thqianyi_skill = {}
thqianyi_skill.name = "thqianyi"
table.insert(sgs.ai_skills, thqianyi_skill)
thqianyi_skill.getTurnUseCard = function(self)
	if self.player:getMark("@qianyi") <= 0 then return end
	local good, bad = 0, 0
	local lord = self.room:getLord()
	if self.role ~= "rebel" and lord and self:isWeak(lord) then
		return sgs.Card_Parse("@ThQianyiCard=.")
	end
	if not self.player:faceUp() then
		return sgs.Card_Parse("@ThQianyiCard=.")
	end
	for _, p in ipairs(self.friends) do
		if self:isWeak(p) or sgs.getDefense(p) < 2 then
			return sgs.Card_Parse("@ThQianyiCard=.")
		end
	end
end

sgs.ai_skill_use_func.ThQianyiCard = function(card, use, self)
	use.card = card
	if use.to then
		local lord = self.room:getLord()
		if self.role ~= "rebel" and lord and self:isWeak(lord) then
			use.to:append(lord)
			return
		end
		self:sort(self.friends)
		use.to:append(self.friends[1])
	end
end

sgs.ai_skill_choice.thqianyi = function(self, choices, data)
	local player = target
	if player:hasSkills(sgs.cardneed_skill) and not self:isWeak(player) then
		return "draw"
	end
	if self:isWeak(player) and player:hasSkills(sgs.masochism_skill) then
		return "recover"
	end
	return math.random(1, 2) == 1 and "draw" or "recover"
end

sgs.ai_use_priority.ThQianyiCard = -5
sgs.ai_card_intention.ThQianyiCard = -150

--【埋火】ai
sgs.string2suit = {
        spade = 0 ,
        club = 1 ,
        heart = 2 ,
        diamond = 3
}
local countKnownSuits = function(target)
	local suits = {}
	local knowncards={}
	for _, card in sgs.qlist(target:getHandcards()) do
		--flag的情况其实可以不要。。。
		local flag = string.format("%s_%s_%s", "visible", global_room:getCurrent():objectName(), target:objectName())
		if  card:hasFlag("visible") or card:hasFlag(flag) then	
			table.insert(knowncards,card)
		end
	end
	
	if #knowncards==0 then return 1, nil end
	
	for _,c in pairs(knowncards)do
		local suit = c:getSuitString()
        if not suits[suit] then suits[suit] = 1 end
        suits[suit] = suits[suit] + 1
	end
	local maxsuit = knowncards[1]:getSuitString()
    for s, n in pairs(suits) do
        if n > suits[maxsuit] then maxsuit = s end
    end

		return math.min(suits[maxsuit],3),sgs.string2suit[maxsuit] 
end
local maihuoCompare_func = function(a, b)
	return countKnownSuits(a)> countKnownSuits(b)
end
local thmaihuo_skill = {}
thmaihuo_skill.name = "thmaihuo"
table.insert(sgs.ai_skills, thmaihuo_skill)
function thmaihuo_skill.getTurnUseCard(self)
	if self.player:hasUsed("ThMaihuoCard") then return nil end
	local hearts={}
	for _,c in sgs.qlist (self.player:getHandcards()) do
		if c:getSuit() == sgs.Card_Heart then
			table.insert(hearts,c)
		end
	end
	if #hearts==0 then return nil end
	self:sortByKeepValue(hearts)
	return sgs.Card_Parse("@ThMaihuoCard=" .. hearts[1]:getEffectiveId())
end
sgs.ai_skill_use_func.ThMaihuoCard = function(card, use, self)
	local targets={}
	for _,p in pairs (self.friends_noself) do
		if not self:willSkipPlayPhase(p)  then
			table.insert(targets,p)
		end
	end
	if #targets ==0 then return nil end
	--单纯从埋火摸牌收益考虑 没有考虑cardneed的信息 没有考虑findPlayerToDraw
	table.sort(targets, maihuoCompare_func)
	use.card = card
	if use.to then
		local maihuo_data=sgs.QVariant()
		maihuo_data:setValue(targets[1])
		self.player:setTag("thmaihuo_target",maihuo_data)
		use.to:append(targets[1])
		if use.to:length() >= 1 then return end
	end
end
--ThMaihuoCard 优先度不好拿捏啊。。。
sgs.ai_use_priority.ThMaihuoCard =sgs.ai_use_priority.Peach +0.2
sgs.ai_card_intention.ThMaihuoCard = -70
sgs.ai_skill_suit.thmaihuo = function(self)
    local target= self.player:getTag("thmaihuo_target"):toPlayer()
	if target then
		local num, suit = countKnownSuits(target)
		if suit then
			return suit
		end
	end
    return  sgs.Card_Heart
end

--【无念】ai
--所有需要伤害来源的needDamage都要记得检测持有【无念】技能的attacker
--smart-ai hasTrickEffective
--standardcards-ai slashIsEffective





