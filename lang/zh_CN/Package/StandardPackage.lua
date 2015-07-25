-- translation for StandardPackage

local t = {
	["standard_cards"] = "标准版",

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
	[":qinggang_sword"] = "装备牌·武器<br /><b>攻击范围</b>：２<br /><b>武器技能</b>：锁定技，当你使用【杀】指定一名角色为目标后，无视其防具。",

	["blade"] = "离魂之镰",
	["illustrator:blade"] = "寒极",
	[":blade"] = "装备牌·武器<br /><b>攻击范围</b>：３<br /><b>武器技能</b>：当你使用的【杀】被【闪】抵消时，你可以对相同的目标再使用一张【杀】。",
	["blade-slash"] = "你可以发动【离魂之镰】再对 %src 使用一张【杀】",
	["#BladeUse"] = "%from 对 %to 发动了【<font color=\"yellow\"><b>离魂之镰</b></font>】效果",

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
	["jueying"] = "紫境",
	["illustrator:jueying"] = "寒极",
	["dilu"] = "要石",
	["illustrator:dilu"] = "寒极",
	["zhuahuangfeidian"] = "坤神天叶",
	["illustrator:zhuahuangfeidian"] = "寒极",
	[":-1 horse"] = "装备牌·坐骑<br /><b>坐骑技能</b>：你与其他角色的距离-1。",
	["chitu"] = "刹那亚空穴",
	["illustrator:chitu"] = "寒极",
	["dayuan"] = "雾雨箒",
	["illustrator:dayuan"] = "寒极",
	["zixing"] = "超妖怪风桨",
	["illustrator:zixing"] = "寒极",

	["amazing_grace"] = "竹取谜宝",
	[":amazing_grace"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：所有角色<br /><b>效果</b>：你亮出牌堆顶的X张牌（X为存活角色的数量），每名目标角色获得其中剩余的一张牌。在结算后，将剩余的牌置入弃牌堆。",
	["illustrator:amazing_grace"] = "宮瀬まひろ",

	["god_salvation"] = "神惠雨降",
	[":god_salvation"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：所有角色<br /><b>效果</b>：每名已受伤的目标角色回复1点体力。",
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



	["standard_ex_cards"] = "标准版EX",

	["purple_song"] = "紫莲圣咏",
	[":purple_song"] = "延时锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：一名其他角色<br /><b>效果</b>：将此牌面朝上置于目标角色的判定区内。目标角色的判定阶段，该角色进行一次判定，若结果不为方块，目标角色跳过此回合的弃牌阶段并于回合结束后摸两张牌。使用结算结束后，将此牌置入弃牌堆。",
	["illustrator:purple_song"] = "ひそな",

	["ice_sword"] = "寒氷剑",
	[":ice_sword"] = "装备牌·武器<br /><b>攻击范围</b>：２<br /><b>武器技能</b>：当你使用【杀】对目标角色造成伤害时，若该角色有牌，你可以防止此伤害，改为依次弃置其两张牌。",
	["illustrator:ice_sword"] = "寒极",
	["ice_sword:yes"] = "你可以依次弃置其两张牌",

	["renwang_shield"] = "龙鱼羽衣",
	[":renwang_shield"] = "装备牌·防具<br /><b>防具技能</b>：锁定技，黑色的【杀】对你无效。",
	["illustrator:renwang_shield"] = "寒极",

	["iron_armor"] = "火鼠裘",
	[":iron_armor"] = "装备牌·防具<br /><b>防具技能</b>：锁定技，你不能成为【灼狱业焰】、【血凤涅烟】、【赤雾锁魂】和红色【杀】的目标。",
	["illustrator:iron_armor"] = "寒极",

	["lure_tiger"] = "罔两空界",
	[":lure_tiger"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：至多两名其他角色<br /><b>效果</b>：目标角色不计入距离和座次结算，不能使用牌且不能成为任何牌的目标直到回合结束。你摸一张牌。",
	["illustrator:lure_tiger"] = "うき",

	["moon_spear"] = "人魂灯",
	[":moon_spear"] = "装备牌·武器<br /><b>攻击范围</b>：３<br /><b>武器技能</b>：每当你于回合外使用或打出黑色手牌时（代替判定牌除外），你可以令你攻击范围内的一名角色选择一项：打出一张【闪】，或受到你对其造成的1点伤害。",
	["illustrator:moon_spear"] = "寒极",
	["@moon_spear"] = "你可以对攻击范围内的一名角色发动【人魂灯】",
	["@moon-spear-jink"] = "【人魂灯】效果被触发，请打出一张【闪】",

	["breastplate"] = "御灵札",
	[":breastplate"] = "装备牌·防具<br /><b>防具技能</b>：每当你受到伤害时，若该伤害不小于X点，你可以防止此伤害，然后将【御灵札】弃置并摸一张牌（X为你的体力值）。",
	["illustrator:breastplate"] = "寒极",
	["#Breastplate"] = "%from 防止了 %to 对其造成的 %arg 点伤害[%arg2]",

	["wooden_ox"] = "万宝槌",
	[":wooden_ox"] = "装备牌·宝物<br /><b>宝物技能</b>：<br />" ..
					"出牌阶段限一次，你可以将一张手牌面朝下置于你装备区内的宝物牌上，若如此做，你可以将此装备牌移动至一名其他角色区域内的相应位置。" ..
					"你可以将此装备牌上的牌如手牌般使用或打出。",
	["illustrator:wooden_ox"] = "寒极",
	["@wooden_ox-move"] = "你可以将【万宝槌】移动至一名其他角色的装备区",
	["#WoodenOx"] = "%from 使用/打出了 %arg 张 %arg2 牌",

	["burning_camps"] = "血凤涅烟",
	[":burning_camps"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：你的下家和与其势力属性相同且连续的角色<br /><b>效果</b>：每名目标角色受到由你造成的1点火焰伤害。",
	["illustrator:burning_camps"] = "藍空",

	["scroll"] = "幻想绘卷",
	[":scroll"] = "装备牌·宝物<br /><b>宝物技能</b>：出牌阶段，你可以获得技能“梦玄”（每当你受到1点伤害后，你可以摸一张牌。），直到你的下一个回合开始。然后将【幻想绘卷】弃置。锁定技，你不能成为延时类锦囊牌的目标；君主角色的游戏开始时，将【幻想绘卷】置入其手牌。",
	["illustrator:scroll"] = "寒极",
	["scroll_trigger"] = "幻想绘卷",
	
	["thmengxuan"] = "梦玄",
	[":thmengxuan"] = "每当你受到1点伤害后，你可以摸一张牌。",

	["drowning"] = "荒波暴流",
	[":drowning"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：所有其他角色<br /><b>效果</b>：每名目标角色需选择一项：1.令你弃置其装备区的一张牌；2.令你弃置其两张手牌；3.受到由你造成的1点伤害。",
	["illustrator:drowning"] = "kaze",
	["drowning:throw"] = "令其弃置装备区的一张牌",
	["drowning:discard"] = "令其弃置两张手牌",
	["drowning:damage"] = "受到1点伤害",

	["known_both"] = "幽瞳占略",
	[":known_both"] = "锦囊牌<br /><b>时机</b>：出牌阶段<br /><b>目标</b>：一名其他角色<br /><b>效果</b>：你观看目标角色的全部的手牌，然后你摸一张牌。",
	["illustrator:known_both"] = "とりのあくあ",

	["jade"] = "八尺琼曲玉",
	[":jade"] = "装备牌·宝物<br /><b>宝物技能</b>：你可以将红色手牌当【无懈可击】使用，你以此法使用的【无懈可击】生效后，你可以选择任意名未结算的角色，令该锦囊牌对这些角色无效。",
	["illustrator:jade"] = "寒极",
	["@jade"] = "你可以发动【八尺琼曲玉】的效果，令此锦囊牌对任意数量角色无效",
	["~jade"] = "选择任意数量的角色→点击确定",
}

local ohorses = { "chitu", "dayuan", "zixing" }
local dhorses = { "zhuahuangfeidian", "dilu", "jueying", "hualiu" }

for _, horse in ipairs(ohorses) do
	t[":" .. horse] = t[":-1 horse"]
end

for _, horse in ipairs(dhorses) do
	t[":" .. horse] = t[":+1 horse"]
end

return t