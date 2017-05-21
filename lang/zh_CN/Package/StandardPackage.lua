-- translation for StandardPackage

local t = {
	["standard_cards"] = "标准包",

	["slash"] = "杀",
	[":slash"] = "基本牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：攻击范围内的一名其他角色<br /><b>效果</b>：对目标角色造成1点伤害。",
	["illustrator:slash"] = "東天紅",
	["slash-jink"] = "%src 使用了【杀】，请使用一张【闪】",
	["@multi-jink-start"] = "%src 使用了【杀】，你须连续使用 %arg 张【闪】",
	["@multi-jink"] = "%src 使用了【杀】，你须再使用 %arg 张【闪】",
	["@slash_extra_targets"] = "请选择此【杀】的额外目标",

	["jink"] = "闪",
	[":jink"] = "基本牌<br /><b>时机</b>：【杀】对你生效时<br /><b>目标</b>：此【杀】<br /><b>效果</b>：抵消此【杀】的效果。",
	["illustrator:jink"] = "An2A",
	["#NoJink"] = "%from 不能使用【<font color=\"yellow\"><b>闪</b></font>】响应此【<font color=\"yellow\"><b>杀</b></font>】",

	["peach"] = "桃",
	[":peach"] = "基本牌<br /><b>时机</b>：出牌阶段/一名角色处于濒死状态时<br /><b>目标</b>：已受伤的你/处于濒死状态的角色<br /><b>效果</b>：目标角色回复1点体力。",
	["illustrator:peach"] = "亜音",

	["crossbow"] = "泄矢铁轮",
	[":crossbow"] = "装备牌·武器<br /><b>攻击范围</b>：１<br /><b>武器技能</b>：出牌阶段，你可以使用任意数量的【杀】。",
	["illustrator:crossbow"] = "寒极",

	["double_sword"] = "绯想剑",
	[":double_sword"] = "装备牌·武器<br /><b>攻击范围</b>：２<br /><b>武器技能</b>：当你使用【杀】指定一名特殊属性不同的角色为目标后，你可令其选择一项：弃置一张手牌，或令你摸一张牌。",
	["illustrator:double_sword"] = "寒极",
	["double-sword-card"] = "%src 发动了【绯想剑】效果，你须弃置一张手牌，或令 %src 摸一张牌",

	["qinggang_sword"] = "楼观剑",
	["illustrator:qinggang_sword"] = "寒极",
	[":qinggang_sword"] = "装备牌·武器<br /><b>攻击范围</b>：２<br /><b>武器技能</b>：锁定技。当你使用【杀】指定一名角色为目标后，无视其防具。",

	["blade"] = "离魂幽镰",
	["illustrator:blade"] = "寒极",
	[":blade"] = "装备牌·武器<br /><b>攻击范围</b>：３<br /><b>武器技能</b>：当你使用的【杀】被【闪】抵消时，你可以对相同的目标再使用一张【杀】。",
	["blade-slash"] = "你可以发动【离魂幽镰】再对 %src 使用一张【杀】",
	["#BladeUse"] = "%from 对 %to 发动了【<font color=\"yellow\"><b>离魂幽镰</b></font>】效果",

	["spear"] = "虹法贤书",
	[":spear"] = "装备牌·武器<br /><b>攻击范围</b>：３<br /><b>武器技能</b>：你可以将两张手牌当【杀】使用或打出。",
	["illustrator:spear"] = "寒极",

	["axe"] = "左扇",
	[":axe"] = "装备牌·武器<br /><b>攻击范围</b>：３<br /><b>武器技能</b>：每当你使用的【杀】被【闪】抵消时，你可以弃置两张牌，则此【杀】依然造成伤害。",
	["illustrator:axe"] = "寒极",
	["@axe"] = "你可以弃置两张牌令此【杀】继续造成伤害",
	["~axe"] = "选择两张牌→点击确定",

	["halberd"] = "冈格尼尔",
	[":halberd"] = "装备牌·武器<br /><b>攻击范围</b>：４<br /><b>武器技能</b>：当你使用【杀】时，且此【杀】是你最后的手牌，你可以额外指定至多两个目标。",
	["illustrator:halberd"] = "寒极",

	["kylin_bow"] = "红叶团扇",
	[":kylin_bow"] = "装备牌·武器<br /><b>攻击范围</b>：５<br /><b>武器技能</b>：当你使用【杀】对目标角色造成伤害时，你可以弃置其装备区里的一张坐骑牌。",
	["illustrator:kylin_bow"] = "寒极",
	["kylin_bow:dhorse"] = "+1坐骑",
	["kylin_bow:ohorse"] = "-1坐骑",

	["eight_diagram"] = "净琉璃镜",
	["illustrator:eight_diagram"] = "寒极",
	[":eight_diagram"] = "装备牌·防具<br /><b>防具技能</b>：每当你需要使用或打出一张【闪】时，你可以进行一次判定：若判定结果为红色，则视为你使用或打出了一张【闪】。",

	["horse"] = "坐骑",
	[":+1 horse"] = "装备牌·坐骑<br /><b>坐骑技能</b>：其他角色与你的距离+1。",
	["zijing"] = "紫境",
	["illustrator:zijing"] = "寒极",
	["yaoshi"] = "要石",
	["illustrator:yaoshi"] = "寒极",
	["kunshentianye"] = "坤神天叶",
	["illustrator:kunshentianye"] = "寒极",
	[":-1 horse"] = "装备牌·坐骑<br /><b>坐骑技能</b>：你与其他角色的距离-1。",
	["chanayakongxue"] = "刹那亚空穴",
	["illustrator:chanayakongxue"] = "寒极",
	["wuyuzhou"] = "雾雨箒",
	["illustrator:wuyuzhou"] = "寒极",
	["chaoyaoguaifengjiang"] = "超妖怪风桨",
	["illustrator:chaoyaoguaifengjiang"] = "寒极",

	["amazing_grace"] = "竹取谜宝",
	[":amazing_grace"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：所有角色<br /><b>效果</b>：你亮出牌堆顶的X张牌（X为存活角色的数量），每名目标角色获得其中剩余的一张牌。在结算后，将剩余的牌置入弃牌堆。",
	["illustrator:amazing_grace"] = "宮瀬まひろ",

	["god_salvation"] = "神惠雨降",
	[":god_salvation"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：所有角色<br /><b>效果</b>：每名已受伤的目标角色回复1点体力，然后重置其人物牌。",
	["illustrator:god_salvation"] = "kannnu",

	["savage_assault"] = "百鬼夜行",
	[":savage_assault"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：所有其他角色<br /><b>效果</b>：每名目标角色需打出一张【杀】，否则受到由你造成的1点伤害。",
	["illustrator:savage_assault"] = "KD",
	["savage-assault-slash"] = "%src 使用了【百鬼夜行】，请打出一张【杀】来响应",

	["archery_attack"] = "魔闪花火",
	[":archery_attack"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：所有其他角色<br /><b>效果</b>：每名目标角色需打出一张【闪】，否则受到由你造成的1点伤害。",
	["illustrator:archery_attack"] = "亜音",
	["archery-attack-jink"] = "%src 使用了【魔闪花火】，请打出一张【闪】来响应",

	["duel"] = "碎月绮斗",
	[":duel"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：一名其他角色<br /><b>效果</b>：由目标角色开始，该角色与你轮流打出一张【杀】，直到其中一名角色未打出【杀】为止。未打出【杀】的角色受到另一名角色造成的1点伤害。",
	["illustrator:duel"] = "RAN",
	["duel-slash"] = "%src 对你【碎月绮斗】，你需要打出一张【杀】",

	["ex_nihilo"] = "绯想镜诗",
	[":ex_nihilo"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：包括你在内的一名角色<br /><b>效果</b>：目标角色摸两张牌。",
	["illustrator:ex_nihilo"] = "kaze",

	["snatch"] = "妙手探云",
	[":snatch"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：你与其距离为1且区域里有牌的一名角色<br /><b>效果</b>：你获得目标角色区域里的一张牌。",
	["illustrator:snatch"] = "茨乃",

	["dismantlement"] = "心网密葬",
	[":dismantlement"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：区域里有牌的一名其他角色。<br /><b>效果</b>：你弃置目标角色区域里的一张牌。",
	["illustrator:dismantlement"] = "RainLan",

	["collateral"] = "断灵御剑",
	[":collateral"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：装备区里有武器牌且攻击范围内有使用【杀】的合理目标的一名其他角色<br /><b>效果</b>：目标角色需对其攻击范围内由你选择的一名合理的角色使用一张【杀】，否则将装备区里的武器牌交给你。",
	["illustrator:collateral"] = "うき",
	["collateral-slash"] = "%dest 使用了【断灵御剑】，请对 %src 使用一张【杀】",
	["#CollateralSlash"] = "%from 选择了此【<font color=\"yellow\"><b>杀</b></font>】的目标 %to",
	["#CollateralNoSlash"] = "%from 攻击范围内没有使用【<font color=\"yellow\"><b>杀</b></font>】的合法目标",

	["nullification"] = "三粒天滴",
	[":nullification"] = "锦囊牌<br /><b>时机</b>：一张锦囊牌对一个目标生效前<br /><b>目标</b>：此牌<br /><b>效果</b>：抵消此牌对该目标产生的效果。",
	["illustrator:nullification"] = "うき",

	["indulgence"] = "春雪幻梦",
	[":indulgence"] = "延时锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：一名其他角色<br /><b>效果</b>：将此牌面朝上置于目标角色的判定区内。目标角色的判定阶段，该角色进行一次判定，若结果不为红桃，目标角色跳过此回合的出牌阶段。使用结算结束后，将此牌置入弃牌堆。",
	["illustrator:indulgence"] = "c7肘",

	["lightning"] = "玄海仙鸣",
	[":lightning"] = "延时锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：你<br /><b>效果</b>：将此牌面朝上置于目标角色的判定区内。目标角色的判定阶段，该角色进行一次判定，若结果为黑桃2-9，目标角色受到3点无来源的雷电伤害，然后将此牌置入弃牌堆。若结果不在此范围内，使用结算结束后，将此牌置入其下家的判定区内（若下家不是此牌合理的目标，顺延之，若场上角色均不为此牌合理的目标，将之置入你的判定区内）。",
	["illustrator:lightning"] = "紀奈",
}

local ohorses = { "chanayakongxue", "wuyuzhou", "chaoyaoguaifengjiang", "yicunshenmiaowan" }
local dhorses = { "zijing", "yaoshi", "kunshentianye", "tianpanzhou" }

for _, horse in ipairs(ohorses) do
	t[":" .. horse] = t[":-1 horse"]
end

for _, horse in ipairs(dhorses) do
	t[":" .. horse] = t[":+1 horse"]
end

return t