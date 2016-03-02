--辨方：每当你使用【杀】对一名其他角色造成一次伤害后，或一名其他角色使用一张【杀】对你造成一次伤害后，你可以进行X次判定，每有一张判定牌为红色，你便可以获得该角色的一张牌（X为你已损失的体力值，且至少为1）。
sgs.ai_skill_invoke.thzhancao = function(self, data)
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