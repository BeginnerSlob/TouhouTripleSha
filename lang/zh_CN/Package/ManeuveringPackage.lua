-- translation for ManeuveringPackage

return {
	["maneuvering"] = "军争包",

	["fire_slash"] = "火杀",
	[":fire_slash"] = "基本牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：攻击范围内的一名角色<br /><b>效果</b>：对目标角色造成1点火焰伤害。",
	["illustrator:fire_slash"] = "うき",

	["thunder_slash"] = "雷杀",
	[":thunder_slash"] = "基本牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：攻击范围内的一名角色<br /><b>效果</b>：对目标角色造成1点雷电伤害。",
	["illustrator:thunder_slash"] = "遠坂あさぎ",

	["analeptic"] = "酒",
	[":analeptic"] = "基本牌<br /><b>时机</b>：出牌阶段/你处于濒死状态时<br /><b>目标</b>：你<br /><b>效果</b>：目标角色本回合使用的下一张【杀】将要造成的伤害+1/目标角色回复1点体力。",
	["illustrator:analeptic"] = "えふぇ",
	["#UnsetDrankEndOfTurn"] = "%from 回合结束，%to 的【<font color=\"yellow\"><b>酒</b></font>】效果消失",

	["fan"] = "八卦炉",
	[":fan"] = "装备牌·武器<br /><b>攻击范围</b>：４<br /><b>武器技能</b>：你可以将一张普通【杀】当具火焰伤害的【杀】使用。",
	["illustrator:fan"] = "寒极",

	["guding_blade"] = "蓬莱玉枝",
	[":guding_blade"] = "装备牌·武器<br /><b>攻击范围</b>：２<br /><b>武器技能</b>：锁定技，当你使用【杀】对目标角色造成伤害时，若其没有手牌，此伤害+1。",
	["illustrator:guding_blade"] = "寒极",
	["#GudingBladeEffect"] = "%from 的【<font color=\"yellow\"><b>蓬莱玉枝</b></font>】效果被触发， %to 没有手牌，伤害从 %arg 增加至 %arg2",

	["vine"] = "渡厄人形",
	[":vine"] = "装备牌·防具<br /><b>防具技能</b>：锁定技，【百鬼夜行】、【魔闪花火】、【荒波暴流】和普通【杀】对你无效。当你受到火焰伤害时，此伤害+1。",
	["illustrator:vine"] = "寒极",
	["#VineDamage"] = "%from 的防具【<font color=\"yellow\"><b>渡厄人形</b></font>】效果被触发，火焰伤害由 %arg 点增加至 %arg2 点",

	["silver_lion"] = "神道清茗",
	[":silver_lion"] = "装备牌·防具<br /><b>防具技能</b>：锁定技，当你受到伤害时，若该伤害多于1点，则防止多余的伤害；当你失去装备区里的【神道清茗】时，你回复1点体力。",
	["illustrator:silver_lion"] = "寒极",
	["#SilverLion"] = "%from 的防具【%arg2】防止了 %arg 点伤害，减至 <font color=\"yellow\"><b>1</b></font> 点",

	["fire_attack"] = "灼狱业焰",
	[":fire_attack"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：一名有手牌的角色<br /><b>效果</b>：目标角色展示一张手牌，然后你可以弃置与之花色或名称相同的一张手牌，若如此做，该角色受到由你造成的1点火焰伤害。",
	["illustrator:fire_attack"] = "東天紅",
	["@fire-attack"] = "%src 展示的牌的花色为 %arg，请弃置一张与其花色或名称相同的手牌",

	["iron_chain"] = "赤雾锁魂",
	[":iron_chain"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：至多两名角色<br /><b>效果</b>：每名目标角色横置或重置其人物牌。<br />重铸：将此牌置入弃牌堆并摸一张牌。",
	["illustrator:iron_chain"] = "茨乃",

	["supply_shortage"] = "枯羽华庭",
	[":supply_shortage"] = "延时锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：你与之距离为1的一名其他角色<br /><b>效果</b>：将此牌面朝上置于目标角色的判定区内。目标角色的判定阶段，该角色进行一次判定，若结果不为草花，目标角色跳过此回合的摸牌阶段。使用结算结束后，将此牌置入弃牌堆。",
	["illustrator:supply_shortage"] = "昼間行燈",

	["tianpanzhou"] = "天磐舟",
	["illustrator:tianpanzhou"] = "寒极",
}
