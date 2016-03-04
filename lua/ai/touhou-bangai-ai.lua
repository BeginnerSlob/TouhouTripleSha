--辨方：每当你使用【杀】对一名其他角色造成一次伤害后，或一名其他角色使用一张【杀】对你造成一次伤害后，你可以进行X次判定，每有一张判定牌为红色，你便可以获得该角色的一张牌（X为你已损失的体力值，且至少为1）。
sgs.ai_skill_invoke.thbianfang = function(self, data)
	local damage = data:toDamage()
	local target = damage.to
	if target:objectName() == self.player:objectName() then
		target = damage.from
	end
	if target and not target:isNude() and target:isAlive() and self:isEnemy(target) and not self:doNotDiscard(target) then
		return true
	end
	if target and self:needToThrowArmor(target) and self:isFriend(target) then
		return true
	end
	return false
end

--授卷：出牌阶段开始时，你可以摸两张牌，若如此做，此阶段内每当一名其他角色失去牌时，你须交给其一张牌。
sgs.ai_skill_invoke.thshoujuan = function(self, data)
	local use = { isDummy = true }
	self:activate(use)
	if not use.card then
		return true
	end
end

sgs.ai_skill_cardask["@thshoujuan"] = function(self, data, pattern, target)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	if self:isFriend(target) then
		local card, _ = self:getCardNeedPlayer(cards, { target }, false)
		if card then
			return "$" .. card:getEffectiveId()
		end
	else
		for _, c in ipairs(cards) do
			if isCard("Peach", c, target) then
				continue
			else
				return "$" .. c:getEffectiveId()
			end
		end
	end
	return "$" .. c:getEffectiveId()
end

--谧契：结束阶段开始时，若你的手牌数为全场最少的（或之一），你可以弃置场上的至多等同于你手牌数的牌。
sgs.ai_skill_use["@@thmiqi"] = function(self, prompt, method)
	local n = self.player:getHandcardNum()
	local targets = self:findPlayerToDiscard("ej", true, true, nil, true)
	local tos = {}
	for _, t in ipairs(targets) do
		table.insert(tos, t:objectName())
		if #tos == n then
			break
		end
	end
	if #tos > 0 then
		return "@ThMiqiCard=.->" .. table.concat(tos, "+")
	end
	return "."
end

sgs.ai_choicemade_filter.cardChosen.thmiqi = sgs.ai_choicemade_filter.cardChosen.snatch