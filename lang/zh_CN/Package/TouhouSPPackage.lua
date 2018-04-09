-- translation for SP Package

return {
	["touhou-sp"] = "东方的黑质",
	
	["#sp001"] = "风月无边",
	["sp001"] = "SP东风谷早苗",--风 - 空 - 4血
	["&sp001"] = "SP早苗",
	["designer:sp001"] = "YSV1",
	["illustrator:sp001"] = "雛咲",
	["cv:sp001"] = "暂无",
	["thfanshi"] = "凡识",
	[":thfanshi"] = "若你的手牌数不小于体力值，你可将一张红色手牌在你的回合内当【杀】，或在你的回合外当【闪】或【桃】使用或者打出。",
	["thjifeng"] = "霁风",
	[":thjifeng"] = "锁定技。你的手牌上限+1。",
	
	["#sp002"] = "正体不明少女",
	["sp002"] = "SP封兽鵺",--花 - 幻 - 4血
	["&sp002"] = "SP鵺",
	["designer:sp002"] = "幻桜落",
	["illustrator:sp002"] = "いがくせい",
	["cv:sp002"] = "暂无",
	["thkongsuo"] = "空锁",
	[":thkongsuo"] = "准备阶段开始时，你可以将一名其他角色的人物牌横置或者重置。",
	["@thkongsuo"] = "你可以发动“空锁”",
	["thguimen"] = "鬼门",
	[":thguimen"] = "锁定技。当你计算与人物牌横置的角色的距离、人物牌横置的角色计算与你的距离时，始终为1。",
	["thliuren"] = "六壬",
	[":thliuren"] = "锁定技。人物牌横置的其他角色于你的回合内使用的第一张牌无效。",
	
	["#sp003"] = "酒吞的巫女",
	["sp003"] = "SP伊吹萃香",--雪 - 空 - 3血
	["&sp003"] = "SP萃香",
	["designer:sp003"] = "桃花僧",
	["illustrator:sp003"] = "禰",
	["cv:sp003"] = "暂无",
	["thshenshi"] = "神事",
	[":thshenshi"] = "每当一名角色跳过一个阶段后，你可以弃置一张牌并令其回复1点体力，每回合限一次。",
	["@thshenshi"] = "你可以弃置一张牌发动“神事”，令 %src 回复1点体力",
	["#ThShenshi"] = "%from 发动了“%arg”，令 %to 回复1点体力",
	["thjiefan"] = "解烦",
	[":thjiefan"] = "准备阶段开始时，你可以亮出牌堆顶的一张牌，若为基本牌，将其交给一名角色，否则将其置入弃牌堆。你可以重复此流程，直到亮出的牌为非基本牌为止。",
	
	["#sp004"] = "日出国之贤兽",
	["sp004"] = "SP上白泽慧音",--月 - 空 - 3血
	["&sp004"] = "SP慧音",
	["designer:sp004"] = "狐耳魔师",
	["illustrator:sp004"] = "kaze",
	["cv:sp004"] = "暂无",
	["thchuangshi"] = "创史",
	[":thchuangshi"] = "你可抵消其他角色使用的一张【心网密葬】、【断灵御剑】或【碎月绮斗】对你或你攻击范围内的一名角色的效果，视为锦囊牌的使用者对你使用了一张无视距离且不计入使用限制的【杀】。",
	["thgaotian"] = "高天",
	[":thgaotian"] = "每当你需要使用或打出一张【闪】时，你可以观看牌堆顶的X张牌（X为存活角色的数量，且至多为4），你可以弃置一张牌并获得其中一张相同颜色的牌，然后将其余的牌以任意顺序置于牌堆顶或置入弃牌堆。<br />◆操作提示：发动技能→（若愿意）换牌→双击任意张置入弃牌堆的牌（可跳过）→点“确定”→调整置于牌堆顶的顺序→点“确定”",
	["@gaotian-discard"] = "你可以弃置一张牌并获得其中一张相同颜色的牌",
	
	["#sp005"] = "濡衣的子猫",
	["sp005"] = "SP火焰猫燐",--风 - 空 - 4血
	["&sp005"] = "SP燐",
	["designer:sp005"] = "金皆居士",
	["illustrator:sp005"] = "NAbyssor",
	["cv:sp005"] = "暂无",
	["thwanling"] = "万灵",
	[":thwanling"] = "其他角色使用的红色【杀】或红色非延时类锦囊牌，在结算后置入弃牌堆时，你可以选择一项：令该角色摸一张牌；或弃置一张牌并获得该角色所使用的牌，每回合限三次。",
	["@thwanling"] = "你可以弃置一张牌获得该牌，或点“取消”令 %src 摸一张牌",
	["thzuibu"] = "醉步",
	[":thzuibu"] = "每当你受到伤害时，伤害来源可以令你摸一张牌，然后令此伤害-1，每回合限三次。",
	["#ThZuibu"] = "%from 发动了 %to 的“<font color=\"yellow\"><b>醉步</b></font>”，伤害点数从 %arg 点减少至 %arg2 点",
	
	["#sp006"] = "恋之白魔导士",
	["sp006"] = "SP雾雨魔理沙",--花 - 空 - 4血
	["&sp006"] = "SP魔理沙",
	["designer:sp006"] = "786852516",
	["illustrator:sp006"] = "明星かがよ",
	["cv:sp006"] = "暂无",
	["thmodao"] = "魔盗",
	[":thmodao"] = "摸牌阶段开始时，你可以放弃摸牌，并选择你攻击范围内的一名角色，若你与其的一个相同区域内的牌数差不大于X+1，交换你们该区域的所有牌，然后你与该角色各摸一张牌（X为你已损失的体力值）。",
	["@thmodao"] = "你可以发动“魔盗”",
	["thmodao:h"] = "手牌",
	["thmodao:e"] = "装备区",
	["thmodao:j"] = "判定区",
	
	["#sp007"] = "七色莲华之蝶",
	["sp007"] = "SP博丽灵梦",--雪 - 空 - 3血
	["&sp007"] = "SP灵梦",
	["designer:sp007"] = "幻桜落",
	["illustrator:sp007"] = "望月椎那",
	["cv:sp007"] = "暂无",
	["thmengsheng"] = "梦生",
	[":thmengsheng"] = "每当你受到一次伤害后，你可以获得伤害来源的一张牌，然后令该角色的非专属技无效，直到你的下一个回合的回合结束。",
	["#thmengsheng-clear"] = "梦生（移除）",
	["thqixiang"] = "绮想",
	[":thqixiang"] = "你攻击范围内的一名角色的弃牌阶段结束时，你可以弃置一张牌，令其摸一张牌或弃置一张手牌。",
	["@thqixiang"] = "你可以发动“绮想”",
	["thqixiang:draw"] = "摸一张牌",
	["thqixiang:discard"] = "弃置一张手牌",
	
	["#sp008"] = "霓岚暗月殇",
	["sp008"] = "SP铃仙•优昙华院•稻叶",--月 - 空 - 4血
	["&sp008"] = "SP铃仙",
	["designer:sp008"] = "昂翼天使",
	["illustrator:sp008"] = "nyanya",
	["cv:sp008"] = "暂无",
	["thhuanlong"] = "幻胧",
	[":thhuanlong"] = "出牌阶段开始时，你可以选择一项：你于此回合内：\
	1.攻击范围+1\
	2.出牌阶段可以额外使用一张【杀】\
	3.可以将两张手牌当【杀】使用或打出\
	你每选择一次，手牌上限便于此回合内-1，你可以重复此流程直到你的手牌上限为0。",
	["#ThHuanlong"] = "%from 发动了“%arg”，选择了第 %arg2 项",
	["thhuanlong:thhuanlong1"] = "攻击范围+1",
	["thhuanlong:thhuanlong2"] = "出牌阶段可以额外使用一张【杀】",
	["thhuanlong:thhuanlong3"] = "可以将两张手牌当【杀】使用或打出",
	
	["#sp009"] = "天上天下",
	["sp009"] = "SP射命丸文",--风 - 空 - 3血
	["&sp009"] = "SP文",
	["designer:sp009"] = "军师祭酒",
	["illustrator:sp009"] = "夏希",
	["cv:sp009"] = "暂无",
	["thyudu"] = "御渡",
	[":thyudu"] = "你的回合外，每当你需要使用一张基本牌时，你可以展示当前回合角色的一张手牌，你可以使用之。若你以此法展示的牌花色为梅花，你可以将此牌当任意基本牌使用，每回合限一次。",
	["thyudu_use:use"] = "你可以使用此牌",
	["thyudu_use:change"] = "你可以转化此牌",
	["thzhaoguo"] = "照国",
	[":thzhaoguo"] = "阶段技。你可以弃置至少一张梅花牌，并从牌堆顶亮出等量的牌，然后你获得其中的非黑桃牌，再将其余的牌交给一名其他角色，若此时该角色的手牌数大于你，其将其人物牌翻面。",
	
	["#sp010"] = "豪族的女仆",
	["sp010"] = "SP物部布都",--花 - 空 - 3血
	["&sp010"] = "SP布都",
	["designer:sp010"] = "ws江上风",
	["illustrator:sp010"] = "みや",
	["cv:sp010"] = "暂无",
	["thlunmin"] = "轮皿",
	[":thlunmin"] = "出牌阶段限三次，你可以弃置一张牌然后摸一张牌。",
	["thyupan"] = "雨磐",
	[":thyupan"] = "结束阶段开始时，若你于本回合使用或弃置牌的花色数为四种，你可以令一名角色回复1点体力。",
	["@thyupan"] = "你可以发动“雨磐”",
	
	["#sp011"] = "算术教室",
	["sp011"] = "SP琪露诺",--雪 - 空 - 4血
	["&sp011"] = "SP琪露诺",
	["designer:sp011"] = "ECauchy",
	["illustrator:sp011"] = "Spark621",
	["cv:sp011"] = "暂无",
	["thjiuzhang"] = "九章",
	[":thjiuzhang"] = "锁定技。你的点数大于9的牌的点数视为9。",
	["thshushu"] = "数术",
	[":thshushu"] = "当你成为基本牌或黑色非延时类锦囊牌的目标时，或当你使用基本牌或黑色非延时类锦囊牌指定目标时，你可以用任意张点数的和与之相同的牌替换之。",
	["@thshushu"] = "你可以发动“数术”",
	["~thshushu"] = "选择若干张牌→点击“确定”",
	["$ThShushu"] = "%from 发动“%arg”将 %card 置于桌面",
	["thfengling"] = "封凌",
	[":thfengling"] = "限定技。当你进入濒死状态时，你可以摸五张牌，然后你可以：弃置至少一张点数和为9的牌，若如此做，你回复1点体力，你可以重复此流程。",
	["@thfengling"] = "你可以弃置其中任意张点数和为9的牌，然后回复1点体力",
	["~thfengling"] = "选择点数和为9的牌→点击确定",
	
	["#sp012"] = "终末之暗",
	["sp012"] = "SP露米娅",--月 - 空 - 4血
	["&sp012"] = "SP露米娅",
	["designer:sp012"] = "s0litaire",
	["illustrator:sp012"] = "サクラメ",
	["cv:sp012"] = "暂无",
	["thyingshi"] = "影弑",
	[":thyingshi"] = "出牌阶段限三次，你可以对一名你与其距离为X的角色使用一张不计入使用限制的【杀】（X为你此阶段发动“影弑”的次数+1）。",
	["@thyingshi"] = "你可以发动“影弑”对距离%arg的角色使用【杀】",
	["thzanghun"] = "葬魂",
	[":thzanghun"] = "每当你使用【杀】造成伤害后，你可以摸一张牌，然后你计算与其他角色的距离+1，直到回合结束。",
	["@gallop"] = "葬",
	["#thzanghun-distance"] = "葬魂",

	["#sp013"] = "空想上的绮华",
	["sp013"] = "SP古明地恋",--风 - 幻 - 3血
	["&sp013"] = "SP恋",
	["designer:sp013"] = "五月Fy",
	["illustrator:sp013"] = "いづる",
	["cv:sp013"] = "暂无",
	["thyimeng"] = "抑梦",
	[":thyimeng"] = "每当一名其他角色对你或与你相邻的角色造成伤害后，你可以获得该角色的一张手牌，然后令其摸一张牌。",
	["thxuyou"] = "虚遊",
	[":thxuyou"] = "阶段技。你使用【杀】时可以无视合法性指定一名角色为目标，当此【杀】造成伤害后，你摸一张牌；若此【杀】没有造成伤害，你依次弃置所有与你相邻的角色的一张牌。",
	["@thxuyou"] = "你可以无视合法性指定一名角色为目标",
	["#thxuyou"] = "虚遊",
	
	["#sp014"] = "荒镇的坤姬",
	["sp014"] = "SP四季映姬",--花 - 空 - 3血
	["&sp014"] = "SP映姬",
	["designer:sp014"] = "孤独的女王",
	["illustrator:sp014"] = "kaze",
	["cv:sp014"] = "暂无",
	["thhuanghu"] = "徨笏",
	[":thhuanghu"] = "每当你使用或打出一张【闪】响应一名其他角色对你使用的牌时，你可以弃置至少一张手牌，然后令该角色弃置等量的牌。",
	["@thhuanghu"] = "你可以弃置至少一张手牌对 %src 发动“徨笏”",
	["thlinyao"] = "凛要",
	[":thlinyao"] = "每当一名其他角色需要使用或打出一张【闪】时，若你的人物牌背面朝上，你可以将你的人物牌翻面，视为该角色使用或打出了一张【闪】。",
	["thfeijing"] = "绯镜",
	[":thfeijing"] = "每当你失去最后的手牌时，你可将手牌补至等同于你体力上限的张数，然后将你的人物牌翻面。",

	["#sp015"] = "梦游仙境",
	["sp015"] = "SP爱丽丝•玛嘉特洛伊德",--雪 - 空 - 3血
	["&sp015"] = "SP爱丽丝",
	["designer:sp015"] = "三国KILL",
	["illustrator:sp015"] = "シエラ",
	["cv:sp015"] = "暂无",
	["thouji"] = "偶祭",
	[":thouji"] = "每当你或你攻击范围内的一名角色的装备区于你的回合内改变时，你可以选择一项：弃置一名其他角色的一张手牌；或摸一张牌。",
	["@thouji"] = "请弃置一名其他角色的一张手牌，或点“取消”摸一张牌",
	["thjingyuansp"] = "镜缘",
	[":thjingyuansp"] = "阶段技。你可以弃置一张红色基本牌，然后令装备区里有牌的一至两名角色各选择一项：将其装备区里的一张牌交给除其以外的一名角色；或令你获得其一张手牌。",
	["@thjingyuansp"] = "请选择装备区里的一张牌交给一名其他角色，或点“取消”令 %src 获得你的一张手牌",
	["@thjingyuansp-give"] = "请选择装备区里的一张牌交给一名其他角色",

	["#sp016"] = "白衣恶魔",
	["sp016"] = "SP蕾米莉娅•斯卡雷特",--月 - 空 - 4血
	["&sp016"] = "SP蕾米",
	["designer:sp016"] = "孤独的女王",
	["illustrator:sp016"] = "nyanya",
	["cv:sp016"] = "暂无",
	["thfeihu"] = "绯护",
	[":thfeihu"] = "阶段技。你可以选择一名体力值不大于你的角色并选择一项：弃置其一张牌，然后令该角色回复1点体力；或对其造成1点伤害，然后令该角色回复1点体力，并摸一张牌。若你已损失的体力值不大于2，你发动“绯护”不可以选择你为目标。",
	["thfeihu:recover"] = "弃置其一张牌，然后令该角色回复1点体力",
	["thfeihu:damage"] = "对其造成1点伤害，然后令该角色回复1点体力并摸一张牌",

	["#sp017"] = "涌泉风吕",
	["sp017"] = "SP古明地觉",--风 - 空 - 4血
	["&sp017"] = "SP觉",
	["designer:sp017"] = "千幻",
	["illustrator:sp017"] = "ke-ta",
	["cv:sp017"] = "暂无",
	["thhuanling"] = "唤灵",
	[":thhuanling"] = "当其他角色死亡后，你可以摸两张牌。若如此做，你获得下列技能中的任意一项：“赤莲”、“魅影”或“境穆”。",
	["thyoukong"] = "游空",
	[":thyoukong"] = "锁定技。你计算与其他角色的距离-X（X为存活的幻属性角色数）。",

	["#sp018"] = "拈花微笑",
	["sp018"] = "SP丰聪耳神子",--花 - 空 - 3血
	["&sp018"] = "SP神子",
	["designer:sp018"] = "凌天翼",
	["illustrator:sp018"] = "Mik-cis",
	["cv:sp018"] = "暂无",
	["thguanzhi"] = "纶旨",
	[":thguanzhi"] = "结束阶段开始时，你可以弃置至少一名攻击范围内含有君主的其他角色的一张牌，令其摸一张牌，然后若其手牌比君主角色多，你摸一张牌。",
	["@thguanzhi"] = "你可以发动“纶旨”",
	["~thguanzhi"] = "选择至少一名角色→点击确定",
	["thfuhua"] = "抚华",
	[":thfuhua"] = "当你受到其他角色造成的伤害时，你可选择你的任意数量的牌令该角色观看，然后该角色选择一项：1.获得这些牌中的一张，然后防止此伤害；2.弃置等量的牌。你对一名角色只能发动一次“<b>抚华</b>”。<br />※操作:伤害来源需要点击确定来进行弃牌",
	["#ThFuhua"] = "%from 令 %to 观看了他的 %arg 张牌",
	["#ThFuhua2"] = "%from 发动了“<font color=\"yellow\"><b>抚华</b></font>”，防止了 %to 造成的 %arg 点伤害",
	["@thfuhua"] = "你可以选择任意数量的牌来发动〖抚华〗",
	["~thfuhua"] = "你可以选择装备区的牌",

	["#sp019"] = "现世幽冥",
	["sp019"] = "SP魂魄妖梦",--雪 - 空 - 4血
	["&sp019"] = "SP妖梦",
	["designer:sp019"] = "LXRivers",
	["illustrator:sp019"] = "えふぇ",
	["cv:sp019"] = "暂无",
	["thyongjie"] = "永劫",
	[":thyongjie"] = "你使用【杀】或黑色非延时类锦囊牌可以额外选择X名角色为目标；当你使用【杀】或黑色非延时类锦囊牌指定目标后，若此牌的目标角色数小于X，则X减至0（X为你于本局游戏内造成过伤害的次数）。",
	["~thyongjie"] = "选择使用牌的目标→点击确定",
	["@doom"] = "劫",

	["#sp020"] = "七曜猫贤者",
	["sp020"] = "SP帕秋莉•诺蕾姬",--月 - 幻 - 3血
	["&sp020"] = "SP帕秋莉",
	["designer:sp020"] = "杰米Y",
	["illustrator:sp020"] = "シノバ",
	["cv:sp020"] = "暂无",
	["thhuanyao"] = "幻曜",
	[":thhuanyao"] = "阶段技。你可以展示一张手牌，然后选择距离最近的一名其他角色，该角色声明一个基本牌的名称。直到回合结束，你可以将此手牌当声明的牌使用且你不能被该牌选择为目标。",
	[":thhuanyao1"] = "阶段技。你可以展示一张手牌，然后选择距离最近的一名其他角色，该角色声明一个基本牌或非延时类锦囊牌的名称。直到回合结束，你可以将此手牌当声明的牌使用且你不能被该牌选择为目标。",
	[":thhuanyao2"] = "阶段技。你可以展示一张手牌，然后你声明一个基本牌或非延时类锦囊牌的名称。直到回合结束，你可以将此手牌当声明的牌使用且你不能被该牌选择为目标。",
	["thzhouzhu"] = "咒逐",
	[":thzhouzhu"] = "当你受到伤害后，你可以选择一项：1.将“幻曜”的描述中的“基本牌”改为“基本牌或非延时类锦囊牌”，若已如此做，将“选择距离最近的一名其他角色，该角色”改为“你”；2.摸一张牌。",
	[":thzhouzhu1"] = "当你受到伤害后，你可以选择一项：1.将“幻曜”的描述中的“基本牌”改为“基本牌或非延时类锦囊牌”；2.摸一张牌。",
	[":thzhouzhu2"] = "当你受到伤害后，你可以选择一项：1.将“幻曜”的描述中的“选择距离最近的一名其他角色，该角色”改为“你”；2.摸一张牌。",
	[":thzhouzhu3"] = "当你受到伤害后，你可以摸一张牌。",
	["thzhouzhu:change1"] = "将“幻曜”的描述中的“基本牌”改为“基本牌或非延时类锦囊牌”",
	["thzhouzhu:change2"] = "将“幻曜”的描述中的“选择距离最近的一名其他角色，该角色”改为“你”",
	["thzhouzhu:draw"] = "摸一张牌",

	["#sp999"] = "香霖堂的店主",
	["sp999"] = "森近霖之助",--特 - 特 - 5血
	["designer:sp999"] = "幻桜落",
	["illustrator:sp999"] = "会帆",
	["cv:sp999"] = "暂无",
	--thjibu
	["thfeiniang"] = "非孃",
	[":thfeiniang"] = "锁定技。分发人物牌前，须将该人物牌撕碎，然后移出游戏。",

}