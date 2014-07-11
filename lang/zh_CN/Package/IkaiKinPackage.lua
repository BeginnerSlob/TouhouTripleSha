-- translation for IkaiKinPackage

return {
	["ikai-kin"] = "异世界的金兰",
	
--wind
	["#wind016"] = "魅魔喵喵拳",
	["wind016"] = "莉莉姆",--风 - 空 - 3血
	["ikhuowen"] = "惑吻",
	[":ikhuowen"] = "摸牌阶段开始时，你可以放弃摸牌，改为令一名其他角色摸两张牌，然后该角色选择一项：对其攻击范围内的由你选择的一名角色使用一张【杀】；或令你获得其两张牌。",
	["ikhuowen-invoke"] = "你可以发动“惑吻”<br/> <b>操作提示</b>: 选择一名其他角色→点击确定<br/>",
	["ikhuowen_slash"] = "惑吻",
	["ikhuowen-slash"] = "请对 %dest 使用一张【杀】",
	["ikenyuan"] = "恩怨",
	[":ikenyuan"] = "每当你获得一名其他角色两张或更多牌时，你可令其摸一张牌；每当你受到1点伤害后，你可令伤害来源交给你一张手牌，或失去1点体力。",
	["IkEnyuanGive"] = "请交给 %dest %arg 张手牌",

	["#wind017"] = "鬼神的童谣",
	["wind017"] = "宫崎和香",--风 - 空 - 3血
	["ikxinchao"] = "心抄",
	[":ikxinchao"] = "出牌阶段限一次，若你的手牌数大于你的体力值，你可以观看牌堆顶的三张牌，然后展示其中任意数量的红桃牌并获得之，其余的以任意顺序置于牌堆顶。",
	["ikshangshi"] = "殇逝",
	[":ikshangshi"] = "你死亡时，可令杀死你的角色弃置其所有的牌。",

	["#wind018"] = "异国的看板娘",
	["wind018"] = "汤音",--风 - 空 - 3血
	["ikmitu"] = "迷途",
	[":ikmitu"] = "锁定技，你防止你造成或者受到的任何锦囊牌的伤害。",
	["#IkMituBad"] = "%from 的“%arg”被触发，本次伤害被防止",
	["#IkMituGood"] = "%from 的“%arg”被触发，防止了本次伤害",
	["iksishi"] = "伺侍",
	[":iksishi"] = "出牌阶段限一次，你可以弃置一张非基本牌，并令一名角色选择一项：摸两张牌，或回复1点体力，或将其人物牌翻至正面朝上并重置之。",
	["iksishi:draw"] = "摸两张牌",
	["iksishi:recover"] = "回复1点体力",
	["iksishi:reset"] = "将人物牌翻转至正面朝上并重置之",

	["#wind019"] = "鬼面的炼金术师",
	["wind019"] = "阿露菲米",--风 - 空 - 4血
	--thxiagong
	["ikwanhun"] = "剜魂",
	[":ikwanhun"] = "每当你使用【杀】或【碎月绮幕】对距离2以内的目标角色造成伤害时，你可进行一次判定，若不为红桃，你防止此伤害，改为令其减少1点体力上限。",
	
	["#wind020"] = "命运的魔女",
	["wind020"] = "Ｃ．Ｃ．",--风 - 空 - 4血
	["ikmeiying"] = "魅影",
	[":ikmeiying"] = "锁定技，回合开始时，你进行一个额外的出牌阶段或观看一名其他角色的手牌。",
	["@ikmeiying"] = "你可以观看一名其他角色的手牌，或点“取消”进行一个额外的出牌阶段",
	["ikfansheng"] = "返生",
	[":ikfansheng"] = "限定技，当你处于濒死状态时，你可以摸两张牌，并将体力回复至X点（X为场上的势力数），然后将你的人物牌翻面。",

	["#wind021"] = "十字架的真祖",
	["wind021"] = "赤夜萌香",--风 - 空 - 4血
	["ikliyao"] = "力妖",
	[":ikliyao"] = "出牌阶段，你可以将一张方块牌当【杀】使用或打出；出牌阶段，你可以无视每阶段的使用限制使用一张与此阶段使用的前一张【杀】颜色不同的【杀】。",

	["#wind026"] = "残念的彼女",
	["wind026"] = "柏崎星奈",--风 - 空 - 4血
	["ikxianyu"] = "陷瑜",
	[":ikxianyu"] = "准备阶段开始时，你可以将至多两名角色的各一张牌置于你的人物牌上，称为“瑕”。每当其他角色需要对你使用一张【杀】时，该角色可以弃置你人物牌上的两张“瑕”，视为对你使用一张【杀】。",
	["ikxianyupile"] = "瑕",
	["@ikxianyu-card"] = "你可以发动“陷瑜”",
	["~ikxianyu"] = "选择 1-2 名角色→点击确定",
	["ikxianyu_slash"] = "陷瑜(杀)",

	["#wind027"] = "追奇的千金",
	["wind027"] = "千反田爱瑠",--风 - 空 - 3血
	["ikqizhi"] = "奇志",
	[":ikqizhi"] = "出牌阶段开始时，你可以与一名其他角色拼点，若你赢，你使用的下一张基本牌或非延时类锦囊牌可以无视距离多或少指定任意一个目标；若你没赢，你不能使用锦囊牌直到回合结束。",
	["#ikqizhi-use"] = "奇志",
	["ikqizhi:add"] = "增加一名目标",
	["ikqizhi:remove"] = "减少一名目标",
	["@ikqizhi-card"] = "你可以发动“巧说”",
	["~ikqizhi1"] = "选择一名其他角色→点击确定",
	["~ikqizhi"] = "选择【断灵御剑】的目标角色→选择【杀】的目标角色→点击确定",
	["ikzongshi"] = "纵适",
	[":ikzongshi"] = "每当你拼点赢，你可以获得对方此次拼点的牌；每当你拼点没赢，你可以获得你此次拼点的牌。",

	["#wind028"] = "断罪的圣辉",
	["wind028"] = "萝卡•鲁丝柯特",--风 - 空 - 4血
	["ikyaolun"] = "耀轮",
	[":ikyaolun"] = "每当一名角色于其出牌阶段使用【杀】时，你可以弃置一张牌，令此【杀】不计入每阶段的使用限制，若此【杀】为红色，你摸一张牌。",
	["@ikyaolun"] = "你可以弃置一张牌发动“耀轮”",

	["#wind031"] = "早夭的影忍",
	["wind031"] = "铃女",--风 - 空 - 4血
	--thjibu
	["ikqiansha"] = "潜杀",
	[":ikqiansha"] = "准备阶段开始时，你可以进行一次判定，然后指定一名距离为1的角色，该角色不能使用或打出与判定牌颜色相同的手牌直到回合结束。",
	["#IkQiansha"] = "由于“<font color=\"yellow\"><b>潜杀</b></font>”效果，%from 本回合不能使用或打出 %arg 手牌",

	["#wind032"] = "真祖的白姬",
	["wind032"] = "爱尔奎特•布伦史塔德",--风 - 空 - 4血
	["iklichi"] = "戾赤",
	[":iklichi"] = "你可以将两张手牌当【杀】使用；出牌阶段，若你以此法使用的【杀】造成伤害，在伤害结算后，你获得技能“赤莲”和“戾咆”，直到回合结束。",

	["#wind037"] = "兄恋的风灵",
	["wind037"] = "桐谷直叶",--风 - 空 - 4血
	["ikxuanren"] = "炫刃",
	[":ikxuanren"] = "你可以将一张装备牌当【杀】使用或打出；你以此法使用的【杀】无距离限制。",
	["iklanjian"] = "岚剑",
	[":iklanjian"] = "当你于出牌阶段内使用的【杀】被目标角色的【闪】抵消时，你可将此【闪】交给除该角色外的一名角色。若获得该牌的角色不是你，你可以对相同的目标再使用一张【杀】。",
	["iklanjian-invoke"] = "你可以发动“岚剑”<br/> <b>操作提示</b>: 选择除 %src 外的一名角色→点击确定<br/>",
	["iklanjian-slash"] = "你可以发动“岚剑”再对 %src 使用一张【杀】",

	["#wind038"] = "终焉的天翼族",
	["wind038"] = "吉普莉尔",--风 - 空 - 3血
	["ikqiangshi"] = "强识",
	[":ikqiangshi"] = "出牌阶段开始时，你可以展示一名其他角色的一张手牌。若如此做，每当你于此阶段内使用与此牌类别相同的牌时，你可以摸一张牌。",
	["ikqiangshi-invoke"] = "你可以发动“强识”<br/> <b>操作提示</b>: 选择一名有手牌的其他角色→点击确定<br/>",
	["ikfengxin"] = "奉心",
	[":ikfengxin"] = "一名其他角色的出牌阶段开始时，你可以摸两张牌，然后交给其两张牌。若如此做，此阶段结束时，若该角色未于此阶段内杀死过一名角色，则你须失去1点体力或弃置两张手牌。",
	["@ikfengxin-give"] = "请交给 %dest %arg 张牌",
	["#IkFengxin"] = "%from 未于本阶段杀死过角色，%to 的“%arg”被触发",
	["ikfengxin-discard"] = "请弃置两张手牌，或点“取消”失去1点体力",

	["#wind039"] = "根源之始",
	["wind039"] = "两仪式",--风 - 空 - 4血
	["ikshensha"] = "神杀",
	[":ikshensha"] = "锁定技，你每于回合内使用一次牌后，你计算与其他角色的距离便减少1，直到回合结束；你的回合内，若你与所有其他角色的距离均为1，你无视其他角色的防具，且你使用的【杀】可以额外指定一个目标。",
	
--bloom
	["#bloom016"] = "锦心绣口",
	["bloom016"] = "天野远子",--花 - 空 - 3血
	["ikshihua"] = "拾华",
	[":ikshihua"] = "当其他角色的一张梅花牌，因弃置或判定而置入弃牌堆时，你可以获得之。\
※操作:双击移除不想获得的牌，点击确定获得余下的全部牌。",
	["ikjiushi"] = "酒诗",
	[":ikjiushi"] = "若你的人物牌正面朝上，你可以将你的人物牌翻面，视为使用一张【酒】；若你的人物牌背面朝上时你受到伤害，你可以在结算后将你的人物牌翻至正面朝上。",

	["#bloom017"] = "无存在感的主役",
	["bloom017"] = "赤座灯里",--花 - 空 - 4血
	["ikzhuyan"] = "朱颜",
	[":ikzhuyan"] = "锁定技，若你的装备区没有防具牌，黑色的普通【杀】对你无效。",
	["ikpiaohu"] = "飘忽",
	[":ikpiaohu"] = "你的回合外，当你攻击范围内的一名角色成为【杀】的目标时，你可弃置一张防具牌，或将该角色与你的人物牌横置，若如此做，将此【杀】转移给你。",
	["@ikpiaohu"] = "你可以弃置一张防具牌发动“飘忽”",
	["ikpiaohu:chain"] = "你可以横置 %src 和你的人物牌发动“飘忽”",

	["#bloom018"] = "零之魔法使",
	["bloom018"]="露易丝•拉•瓦利埃尔",--花 - 幻 - 3血
	["ikxuwu"]="虚无",
	[":ikxuwu"]="锁定技，你即将造成的伤害均视为失去体力。",
	["ikjiaolian"]="娇恋",
	[":ikjiaolian"]="每当你的手牌数小于X时，你可以将手牌补至X张（X为你已损失的体力值，且至多为2）。",
	
	["#bloom019"] = "终末之血",
	["bloom019"] = "桂言叶",--花 - 空 - 4血
	["ikbengshang"] = "崩殇",
	[":ikbengshang"] = "每当你受到1点伤害后，你可摸一张牌，然后将一张手牌置于你的人物牌上，称为“殇”；你的人物牌上每有一张牌，你的手牌上限便+1。",
	["IkBengshangPush"] = "请将一张手牌置于武将牌上",
	["ikbengshangpile"] = "殇",
	["ikanhun"] = "暗魂",
	[":ikanhun"] = "觉醒技，准备阶段开始时，若“殇”的数量达到三张或更多，你须减少1点体力上限，然后回复1点体力，或摸两张牌，并获得技能“诛异”（出牌阶段限一次，你可以将一张“殇”置入弃牌堆，并令一名角色摸两张牌，然后若其手牌数比你多，你对其造成1点伤害。）。",
	["#IkAnhunWake"] = "%from 的“殇”为 %arg 张，触发“%arg2”觉醒",
	["ikanhun:recover"] = "回复1点体力",
	["ikanhun:draw"] = "摸两张牌",
	["ikzhuyi"] = "诛异",
	[":ikzhuyi"] = "出牌阶段限一次，你可以将一张“殇”置入弃牌堆，并令一名角色摸两张牌，然后若其手牌数比你多，你对其造成1点伤害。",

	["#bloom020"] = "迷途的猫咪",
	["bloom020"] = "雾谷希",--花 - 空 - 3血
	["ikmice"] = "谜策",
	[":ikmice"] = "出牌阶段限一次，你可以将所有手牌当任意一张非延时类锦囊牌使用。",
	["ikzhiyu"] = "智愚",
	[":ikzhiyu"] = "每当你受到一次伤害后，你可以摸一张牌，然后展示所有手牌，若颜色均相同，伤害来源弃置一张手牌。",
	
	["#bloom021"] = "水边之花",
	["bloom021"] = "玖我夏树",--花 - 空 - 4血
	["ikguanchong"] = "贯铳",
	[":ikguanchong"] = "摸牌阶段摸牌时，你可以选择一项：<br />1.额外摸一张牌，若如此做，你不能使用或打出【杀】，直到回合结束。<br />2.少摸一张牌，若如此做，你于出牌阶段使用【杀】时无距离限制，且你可以额外使用一张【杀】，直到回合结束。",
	["#IkGuanchong1"] = "%from 发动了“%arg”，额外摸一张牌",
	["#IkGuanchong2"] = "%from 发动了“%arg”，少摸一张牌",
	["ikguanchong:guan"] = "额外摸一张牌",
	["ikguanchong:chong"] = "少摸一张牌",

	["#bloom022"] = "奇迹的魔女",
	["bloom022"] = "芙蕾德莉卡•贝伦卡斯泰露",--花 - 幻 - 3血
	["iklundao"] = "轮道",
	[":iklundao"] = "在一名角色的判定牌生效前，你可以亮出牌堆顶的一张牌代替之。",
	["ikxuanwu"] = "玄舞",
	[":ikxuanwu"] = "结束阶段开始时，你可以进行一次判定，若结果为黑色，你观看牌堆顶的1+X张牌（X为你已损失的体力值），然后将这些牌交给一名角色。",
	
	["#bloom025"] = "繁星之忆",
	["bloom025"] = "梅娅•艾丝•艾菲梅拉尔",--花 - 空 - 3血
	["ikxingshi"] = "星筮",
	[":ikxingshi"] = "每当你受到一次伤害后，你可以亮出牌堆顶的四张牌。然后获得其中任意数量点数之和小于或等于13的牌，将其余的牌置入弃牌堆。",
	["ikshouyan"] = "狩魇",
	[":ikshouyan"] = "每当体力值为1的一名其他角色受到伤害时，你可以将人物牌翻面并弃置一张装备牌，然后防止此伤害。",
	["@ikshouyan-card"] = "你可以弃置一张装备牌发动“狩魇”防止 %src 受到的伤害",
	["#IkShouyan"] = "%from 受到的伤害由于“%arg”效果被防止",
	
	["#bloom026"] = "恶趣的千金",
	["bloom026"] = "凉月奏",--花 - 空 - 4血
	["ikjingce"] = "精策",
	[":ikjingce"] = "出牌阶段结束时，若你于此回合使用的牌的数量不小于你的体力值，你可以摸两张牌。",

	["#bloom027"] = "冰雾的雪姬",
	["bloom027"] = "司波深雪",--花 - 空 - 3血
	["ikbingyan"] = "冰焰",
	[":ikbingyan"] = "出牌阶段限一次，你可以弃置至少一张手牌，令一名其他角色选择一项：弃置一张与你所弃置牌类型均不同的手牌；或该角色将人物牌翻面并摸等同于你弃牌数的牌。",
	["@ikbingyan-discard"] = "请弃置一张与“冰焰”弃牌类型均不同的手牌",
	["ikxuelian"] = "雪涟",
	[":ikxuelian"] = "每当你受到一次伤害后，你可以展示一张手牌，令伤害来源弃置一张类别不同的手牌，否则你回复1点体力。",
	["@ikxuelian-show"] = "你可以发动“雪涟”展示一张手牌",
	["@ikxuelian-discard"] = "%src 发动了“雪涟”，请弃置一张 %arg 或 %arg2",

	["#bloom031"] = "绝世的妖魂",
	["bloom031"] = "玉藻前",--花 - 幻 - 3血
	["ikqingguo"] = "倾国",
	[":ikqingguo"] = "结束阶段开始时，若你已受伤，你可以摸至多X张牌，然后将等量的手牌依次交给一名其他角色（X为你已损失的体力值）。",
	["ikqingguo_draw"] = "倾国摸牌数",
	["ikjingshi"] = "镜石",
	[":ikjingshi"] = "当你成为一名其他角色使用的【杀】或非延时类锦囊牌的目标时，你可失去1点体力，令此牌对你无效，然后你弃置该角色一张牌。",

	["#bloom037"] = "目隐演绎着",
	["bloom037"] = "小樱茉莉",--花 - 空 - 4血
	["ikmuhe"] = "目合",
	[":ikmuhe"] = "每当你使用或其他角色在你的回合内使用【闪】时，你可以将牌堆顶的一张牌面朝上置于你的人物牌上；一名其他角色的出牌阶段开始时，你可以将你人物牌上的一张牌置入弃牌堆，然后该角色本阶段可以使用【杀】的次数上限-1。",
	["ikmuhe:remove"] = "你可以将一张“目合牌”置入弃牌堆令当前回合角色本阶段可以使用【杀】的次数上限-1",

	["#bloom038"] = "赤红的灭杀姬",
	["bloom038"] = "莉亚丝•吉蒙里",--花 - 空 - 3血
	["ikdingpin"] = "定品",
	[":ikdingpin"] = "出牌阶段，你可以弃置一张与你此回合使用或弃置的牌类别均不同的手牌，然后令一名已受伤的角色进行一次判定，若结果为黑色，该角色摸X张牌（X为该角色已损失的体力值），然后你此回合不能再对其发动“定品”；若结果为红色，将你的人物牌翻面。",
	["ikmoyi"] = "魔弈",
	[":ikmoyi"] ="每当一名角色的人物牌翻面或横置时，你可以令其摸一张牌。",

	["#bloom039"] = "时光的溯行者",
	["bloom039"] = "晓美焰",--花 - 空 - 4血
	["ikhuaan"] = "华岸",
	[":ikhuaan"] = "每当你的黑色基本牌因弃置而进入弃牌堆时，你可以将之当做【枯羽华庭】置于一名其他角色的判定区里。",
	["@ikhuaan-use"] = "你可以发动“华岸”将其中一张牌当做【枯羽华庭】置于一名其他角色的判定区里",
	["~ikhuaan"] = "选择一张黑色基本牌→选择【枯羽华庭】的目标角色→点击确定",
	["ikyongxin"] = "勇心",
	[":ikyongxin"] ="你攻击范围内的一名角色的判定阶段开始时，你可弃置其判定区里的一张牌，视为对其使用一张【杀】。若此【杀】未造成伤害，你摸一张牌。",

--snow
	["#snow016"] = "雷光的恶魔",
	["snow016"] = "安藤美雷",--雪 - 空 - 4血
	["ikleilan"] = "雷岚",
	[":ikleilan"] = "锁定技，当你失去一次装备区里的装备牌时；或弃牌阶段结束时，若你在此阶段弃置了两张或更多的手牌，你须获得1枚“疾雷”标记。一名角色的结束阶段开始时，你须弃置1枚“疾雷”标记，然后选择一项：<br />1. 视为对一名其他角色使用一张具雷电伤害的【杀】；<br />2. 对距离为1的一名角色造成1点雷电伤害；<br />3. 摸一张牌，然后弃置一张手牌。<br />你须重复此流程，直到你没有“疾雷”标记为止。",
	["ikleilan:slash"] = "视为对一名其他角色使用一张具雷电伤害的【杀】",
	["ikleilan:damage"] = "对距离为1的一名角色造成1点雷电伤害",
	["ikleilan:draw"] = "摸一张牌，然后弃置一张手牌",
	["@ikleilan-damage"] = "请选择距离1的一名角色",
	["@jilei"] = "疾雷",

	["#snow017"] = "返魂的魔女",
	["snow017"] = "右代宫缘寿",--雪 - 幻 - 3血
	["ikyuanfa"] = "源法",
	[":ikyuanfa"] = "当一名角色进入濒死状态时，你可以展示其一张手牌，若不为基本牌，该角色须弃置此牌，然后回复1点体力。",
	["ikguanju"] = "观剧",
	[":ikguanju"] = "出牌阶段限一次，你可以选择两名装备区的装备牌数差不大于X的角色，交换他们装备区的全部装备牌（X为你已损失的体力值）。",
	["#IkGuanjuSwap"] = "%from 交换了 %to 的装备",

	["#snow018"] = "香魂翩跹",
	["snow018"] = "巴麻美",--雪 - 空 - 4血
	["ikzhongqu"] = "终曲",
	[":ikzhongqu"] = "你每使用【杀】造成一次伤害后，可以令受到该伤害的角色摸X张牌（X为该角色的体力值，且至多为5），然后该角色将其人物牌翻面。",

	["#snow019"] = "超世之英魂",
	["snow019"] = "黑岩舒",--雪 - 空 - 4血
	["iklingpao"] = "灵炮",
	[":iklingpao"] = "你可以将一张普通【杀】当具火焰伤害的【杀】使用。若以此法使用的【杀】造成了伤害，则在此【杀】结算后你失去1点体力；你使用具火焰伤害的【杀】时，可以额外指定一个目标。",
	["#iklingpao"] = "灵炮（失去体力）",
	["ikxiaozui"] = "销罪",
	[":ikxiaozui"] = "结束阶段开始时，若你的人物牌上没有牌，你可将任意数量的【杀】置于你的人物牌上，称为“罪”。当一名角色处于濒死状态时，你可将一张“罪”置入弃牌堆，视为该角色使用一张【桃】。",
	["ikxiaozuipile"] = "罪",
	["@ikxiaozui"] = "你可以发动“销罪”",
	["~ikxiaozui"] = "选择若干张【杀】→点击确定",

	["#snow020"] = "未闻此花之名",
	["snow020"] = "本间芽衣子",--雪 - 幻 - 3血
	["ikanxu"] = "安恤",
	[":ikanxu"] = "出牌阶段限一次，你可以指定两名手牌数不同的其他角色，其中手牌较少的角色获得手牌较多的角色一张手牌，并展示之。若此牌非黑桃，则你摸一张牌。",
	["ikzhuiyi"] = "追忆",
	[":ikzhuiyi"] = "你死亡时，可以指定一名除杀死你的角色之外的角色，该角色摸三张牌并回复1点体力。",
	["ikzhuiyi-invoke"] = "你可以发动“追忆”<br/> <b>操作提示</b>: 选择一名其他角色→点击确定<br/>",
	["ikzhuiyi-invokex"] = "你可以发动“追忆”<br/> <b>操作提示</b>: 选择除 %src 外的一名其他角色→点击确定<br/>",

	["#snow021"] = "杏林之烁剑",
	["snow021"] = "结城明日奈",--雪 - 空 - 4血
	--ikxuanren
	["ikjieyou"] = "解忧",
	[":ikjieyou"] = "当一名角色进入濒死状态时，你可对当前回合的角色使用一张【杀】，此【杀】造成伤害时，你防止此伤害，视为对该濒死角色使用了一张【桃】。",
	["ikjieyou-slash"] = "你可以对当前回合角色使用一张【杀】发动“解忧”",
	["#IkJieyouPrevent"] = "%from 的“<font color=\"yellow\"><b>解忧</b></font>”效果被触发，防止了对 %to 的伤害",
	["#IkJieyouNull1"] = "%from 已经脱离濒死状态，“<font color=\"yellow\"><b>解忧</b></font>”第二项效果无法执行",
	["#IkJieyouNull2"] = "%from 已经死亡，“<font color=\"yellow\"><b>解忧</b></font>”第二项效果无法执行",

	["#snow023"] = "绯樱的妖精",
	["snow023"] = "艾露莎•斯卡雷特",--雪 - 空 - 4血
	["ikqianbian"] = "千变",
	[":ikqianbian"] = "当你失去一次装备区里的牌时，你可以依次弃置一至两名其他角色的共计两张牌，你以此法每弃置了一张基本牌，你摸一张牌。",

	["#snow024"] = "绯红的棉花糖",
	["snow024"] = "樱野栗梦",--雪 - 空 - 4血
	["designer:snow024"] = "ByArt",
	["illustrator:snow024"] = "正体不明",
	["cv:snow024"] = "暂无",
	["ikmengjing"] = "萌境",
	[":ikmengjing"] = "出牌阶段限一次，你可以弃置一张牌，令你的攻击范围无限，直到回合结束；若你以此法弃置的牌为装备牌，你可以弃置一名其他角色的一张牌。",
	["@ikmengjing-discard"] = "你可以弃置一名其他角色的一张牌",
	["ikzhizhan"] = "止战",
	[":ikzhizhan"] = "限定技，出牌阶段，你可以指定一名角色，攻击范围内含有该角色的所有角色须依次选择一项：弃置一张武器牌；或令该角色摸一张牌。",
	["@ikzhizhan-discard"] = "请弃置一张武器牌，否则 %dest 摸一张牌",

	["#snow026"] = "赤莲的铳使",
	["snow026"] = "优子•莉特纳",--雪 - 空 - 4血
	["designer:snow026"] = "正体不明",
	["illustrator:snow026"] = "正体不明",
	["cv:snow026"] = "暂无",
	["ikduoren"] = "夺刃",
	[":ikduoren"] = "每当你受到【杀】造成的一次伤害后，你可以弃置一张牌，然后获得伤害来源装备区的武器牌。",
	["@ikduoren-get"] = "你可以弃置一张牌发动“夺刃”",
	["ikanju"] = "暗狙",
	[":ikanju"] = "当你使用的【杀】对目标角色造成伤害时，若你不在其攻击范围内，则此伤害+1。",
	["#IkAnjuBuff"] = "%from 发动了“<font color=\"yellow\"><b>暗狙</b></font>”，伤害从 %arg 点增加至 %arg2 点",

	["#snow027"] = "乐园的王女",
	["snow027"] = "茉茉•贝莉雅•戴比露克",--雪 - 空 - 3血
	["ikzongxuan"] = "纵玄",
	[":ikzongxuan"] = "每当你的牌因弃置进入弃牌堆前，你可以将此牌置于牌堆顶。",
	["@ikzongxuan-put"] = "你可以发动“纵玄”",
	["~ikzongxuan"] = "选择任意数量的牌→点击确定（这些牌将以与你点击顺序相反的顺序置于牌堆顶）",
	["ikzhice"] = "直策",
	[":ikzhice"] = "结束阶段开始时，你可令一名角色摸一张牌并展示之，若此牌为装备牌，该角色回复1点体力并使用此牌。",
	["ikzhice-invoke"] = "你可以发动“直策”<br/> <b>操作提示</b>: 选择一名角色→点击确定<br/>",

	["#snow028"] = "完璧之妹",
	["snow028"] = "高坂桐乃",--雪 - 空 - 4血
	["designer:snow028"] = "正体不明",
	["illustrator:snow028"] = "正体不明",
	["cv:snow028"] = "暂无",
	["ikyinzhai"] = "隐宅",
	[":ikyinzhai"] = "每当你造成一次伤害后，你可以摸一张牌。若如此做，终止一切结算，且当前回合结束。",
	
	["#snow037"] = "自称的血族真祖",
	["snow037"] = "羽濑川小鸠",--雪 - 幻 - 3血
	["ikjiaoshi"] = "矫誓",
	[":ikjiaoshi"] = "出牌阶段限一次，当你使用【杀】或黑色非延时类锦囊牌指定唯一目标时，你可令可以成为此牌目标的另一名其他角色选择一项：交给你一张牌并成为此牌的使用者；或成为此牌的额外目标。",
	["ikjiaoshi-invoke"] = "你可以发动“矫誓”<br/> <b>操作提示</b>: 选择除 %src 外的一名角色→点击确定<br/>",
	["@ikjiaoshi-collateral"] = "请选择【断灵御剑】 %src 使用【杀】的目标",
	["@ikjiaoshi-give"] = "请交给 %src 一张牌成为此牌的使用者，否则你成为此牌的目标",
	["iklinghuang"] = "灵煌",
	[":iklinghuang"] = "每当你受到一名空属性角色造成的伤害时，你可以弃置一张装备牌，令此伤害-1。",
	["@iklinghuang"] = "你可以弃置一张装备牌发动“灵煌”令此伤害-1",
	["#IkLinghuang"] = "%from 发动了“<font color=\"yellow\"><b>灵煌</b></font>”，伤害从 %arg 点减少至 %arg2 点",
	
	["#snow039"] = "凡尘天使",
	["snow039"] = "榛名",--雪 - 空 - 3血
	["ikshenxing"] = "慎行",
	[":ikshenxing"] = "出牌阶段，你可以弃置两张牌，然后摸一张牌。",
	["ikxiangzhao"] = "祥昭",
	[":ikxiangzhao"] = "结束阶段开始时，你可以展示所有手牌，若均为同一颜色，则你令至多X名角色各摸一张牌（X为你的手牌数）。",
	["@ikxiangzhao-card"] = "你可以展示所有手牌发动“祥昭”",
	["~ikxiangzhao"] = "若手牌均为同一颜色，选择至多X名角色→点击确定；否则直接点击确定",
	
	["#snow041"] = "冰凌的狙手",
	["snow041"] = "朝田诗乃",--雪 - 空 - 4血
	["ikyoudan"] = "幽弹",
	[":ikyoudan"] = "出牌阶段，你可以选择你攻击范围内的一名角色，然后弃置X张牌（X为此阶段你发动“幽弹”的次数），若以此法弃置的牌的数量为一张，你弃置该角色一张牌；若为两张，令该角色交给你一张牌；若为三张，你对该角色造成1点伤害；若不小于四张，你与该角色各摸两张牌。",
	["@ikyoudan-give"] = "请交给 %src 一张牌",
	
--luna
	["#luna010"] = "无声的和绘",
	["luna010"] = "立华奏",--月 - 空 - 4血
	["iklvdong"] = "律动",
	[":iklvdong"] = "出牌阶段限一次，你可以与一名其他角色拼点。若你赢，你获得以下技能直到回合结束：无视与该角色的距离及其防具；你可以对其使用任意数量的【杀】。若你没赢，你不能使用【杀】直到回合结束。",
	["ikguozai"] = "过载",
	[":ikguozai"] = "锁定技，你的【酒】均视为【杀】。",

	["#luna013"] = "摇摆的命运",
	["luna013"] = "牧濑红莉栖",--月 - 空 - 3血
	["ikmingce"] = "明策",
	[":ikmingce"] = "出牌阶段限一次，你可以交给一名其他角色一张装备牌或【杀】，然后令其选择一项：视为对其攻击范围内的一名由你指定的角色使用一张【杀】；或摸一张牌。",
	["ikmingce:use"] = "对攻击范围内的一名角色使用一张【杀】",
	["ikmingce:draw"] = "摸一张牌",
	["ikzhichi"] = "智迟",
	[":ikzhichi"] = "锁定技，你的回合外，每当你受到一次伤害后，【杀】或非延时类锦囊牌对你无效，直到回合结束。",
	["#ikzhichi-protect"] = "智迟（无效）",
	["#IkZhichiDamaged"] = "%from 受到了伤害，本回合内【<font color=\"yellow\"><b>杀</b></font>】和非延时锦囊都将对其无效",
	["#IkZhichiAvoid"] = "%from 的“%arg”被触发，【<font color=\"yellow\"><b>杀</b></font>】和非延时锦囊对其无效",

	["#luna015"] = "原野日和之春",
	["luna015"] = "小牧爱佳",--月 - 空 - 4血
	["iktianjing"] = "恬静",
	[":iktianjing"] = "锁定技，若你已受伤，你的手牌上限+4。",
	["ikdanbo"] = "澹泊",
	[":ikdanbo"] = "摸牌阶段摸牌时，你可以额外摸X张牌（X为你已损失的体力值），然后跳过你此回合的出牌阶段。",

	["#luna016"] = "情殇的雷神",
	["luna016"] = "伊诗塔尔•弗利吉",--月 - 空 - 6血
	["ikxinshang"] = "心殇",
	[":ikxinshang"] = "锁定技，每当你受到一次非梅花【杀】造成的伤害后，你须减少1点体力上限。",
	["ikqilei"] = "悽雷",
	[":ikqilei"] = "每当你减少1点体力上限后；或使用具雷电伤害的【杀】造成一次雷电伤害，在伤害结算后，你可以对你攻击范围内的一名其他角色造成1点雷电伤害。",
	["@ikqilei"] = "你可以发动“悽雷”",

	["#luna027"] = "夏娃",
	["luna027"] = "樱满真名",--月 - 空 - 3血
	["ikjuece"] = "绝策",
	[":ikjuece"] = "在你的回合，一名角色失去最后的手牌时，你可以对其造成1点伤害。",
	["ikshangye"] = "殇夜",
	[":ikshangye"] = "你使用黑色非延时类锦囊牌仅指定一个目标后，可以额外指定一个目标。",
	["ikfenshi"] = "焚世",
	[":ikfenshi"] = "限定技，出牌阶段，你可令所有其他角色依次选择一项：弃置X张牌；或受到由你造成的1点火焰伤害（X为其装备区里牌的数量且至少为1）。",

	["#luna028"] = "摄魂的妖姬",
	["luna028"] = "阿狸",--月 - 幻 - 3血
	["ikduopo"] = "夺魄",
	[":ikduopo"] = "一名其他角色的回合开始时，若你已受伤，你可与其拼点。若你赢，该角色跳过此回合的出牌阶段；否则该角色计算与你的距离时，无视除你之外的其他角色及场上的坐骑牌，直到回合结束。",
	["ikmeihun"] = "魅魂",
	[":ikmeihun"] = "当你成为【杀】的目标时，你可以令一名其他角色选择一项：交给你一张【闪】；或该角色也成为此【杀】的目标。",

	["#luna038"] = "魔性的王者",
	["luna038"] = "宫永照",--月 - 空 - 3血
	["iklianzhuang"] = "连庄",
	[":iklianzhuang"] = "每当你于出牌阶段内使用的牌与你此阶段使用的上一张牌点数或花色相同时，你可以摸一张牌。",
	["ikguijing"] = "鬼镜",
	[":ikguijing"] = "锁定技，每当你受到伤害后，若此伤害是你本回合第一次受到的伤害，你回复1点体力；否则你失去1点体力。",

	["#luna041"] = "邪气的烈焰",
	["luna041"] = "克图格亚",--月 - 空 - 3血
	["ikhuowu"] = "火舞",
	[":ikhuowu"] = "结束阶段开始时，你可以对一名没有手牌的角色造成1点火焰伤害。",
	["iktianxie"] = "天邪",
	[":iktianxie"] = "出牌阶段限一次，你可以展示一张黑色锦囊牌并将之置于牌堆顶，然后令有手牌的一名其他角色选择一项：弃置一张锦囊牌；或弃置两张非锦囊牌。",
	["ikbengyan"] = "崩焰",
	[":ikbengyan"] = "限定技，出牌阶段，你可令所有其他角色各选择一项：弃置至少X张牌（X为其上家以此法弃置牌的数量+1）；或受到你对其造成的2点火焰伤害。",
}