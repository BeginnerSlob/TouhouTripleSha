-- translation for IkaiHiPackage

return {
	["ikai-ka"] = "异世界的火种",
	
--wind
	["#wind025"] = "计测万端",
	["wind025"] = "白",--风 - 空 - 3血
	["ikzhiju"] = "智局",
	[":ikzhiju"] = "出牌阶段限一次，你可以弃置场上的一张牌，重置或横置一名角色的人物牌，然后你回复1点体力，若如此做，弃牌阶段开始时，你须将两张手牌以任意顺序置于牌堆顶（不足则全部置于）。<br />※操作:这两张牌将以与你点击顺序相反的顺序置于牌堆顶",
	["@ikzhiju-chain"] = "请重置或横置一名角色的人物牌",
	["@ikzhiju"] = "请将 %arg 张牌置于牌堆顶",
	["ikyingqi"] = "影契",
	[":ikyingqi"] = "准备阶段开始时，你可选择一名其他角色，你与其的下一个出牌阶段结束时或跳过出牌阶段后，若手牌数小于体力值，当前回合角色摸一张牌。",
	["@ikyingqi"] = "你可以发动“影契”",

	["#wind033"] = "朝阳的游击士",
	["wind033"] = "艾丝蒂尔•布莱特",--风 - 空 - 4血
	["&wind033"] = "艾丝蒂尔",
	["ikjilun"] = "极轮",
	[":ikjilun"] = "出牌阶段限一次，你可以弃置一张黑色手牌，并指定你攻击范围内一名装备区内有牌的角色，你令该角色选择获得其装备区的一张牌，然后视为你对其使用一张【杀】（此【杀】不计入每阶段的使用限制）。",

	["#wind034"] = "人型电脑天使心",
	["wind034"] = "芙莱雅＆艾露妲",--风 - 空 - 4血
	["ikjiqiao"] = "机巧",
	[":ikjiqiao"] = "摸牌阶段结束时，若你的手牌数小于体力上限，你可以弃置一张牌，若弃置的牌的种类为：<br />1.基本牌，视为你对一名其他角色使用一张无视距离的【杀】<br />2.装备牌，你令一名其他角色摸两张牌<br />3.锦囊牌，你令一名其他角色回复1点体力",
	["@ikjiqiao"] = "你可以发动“机巧”",
	["@ikjiqiao-basic"] = "请选择一名其他角色，视为对其使用一张无视距离的【杀】",
	["@ikjiqiao-equip"] = "请选择一名其他角色，令其摸两张牌",
	["@ikjiqiao-trick"] = "请选择一名其他角色，令其回复1点体力",

	["#wind035"] = "赤珠之锁",
	["wind035"] = "摩尔迦娜",--风 - 空 - 4血
	["ikkangjin"] = "亢劲",
	[":ikkangjin"] = "出牌阶段，你可以将一张手牌交给一名其他角色（须与你此阶段上一次发动“亢劲”交给一名其他角色的牌颜色不同），视为你对其使用一张【碎月绮斗】；受到该【碎月绮斗】造成的伤害的角色摸等同于其已损失的体力值数量的牌（至多摸五张）。",
	["#ikkangjin"] = "亢劲（摸牌）",

	["#wind036"] = "蠕动之混沌",
	["wind036"] = "奈亚拉托提普",--风 - 空 - 4血
	["ikhunkao"] = "魂铐",
	[":ikhunkao"] = "出牌阶段限两次，你可以展示全部的手牌，并弃置其中一种花色全部的牌，然后你指定至多两名且至多等同于弃置的牌的数量的其他角色，并令她们依次选择一项：交给你一张该花色的牌；或视为你对其使用一张无视距离的【杀】。",
	["@ikhunkao-give"] = "受“魂铐”影响，请交给 %src 一张 %arg 牌，否则视为其对你使用一张无视距离的【杀】",

	["#wind045"] = "讨魔的暗杀者",
	["wind045"] = "雪风•帕尼托尼",--风 - 空 - 4血
	["&wind045"] = "雪风",
	["ikhudie"] = "狐谍",
	[":ikhudie"] = "每当你受到一次伤害时，若你与伤害来源的势力属性不同，你可以将之改变为与伤害来源相同。",
	["ikyinsha"] = "隐杀",
	[":ikyinsha"] = "锁定技，当你计算与势力属性相同的角色距离时，无视除该角色外的其他角色及场上的坐骑牌。",
	["ikhualan"] = "花岚",
	[":ikhualan"] = "每当你对势力属性不同的角色造成一次伤害后，你可以摸一张牌，或将势力属性改变为风势力。",
	["ikhualan:draw"] = "摸一张牌",
	["ikhualan:change"] = "将势力属性改变为风势力",

	["#wind047"] = "焰弓的空舰",
	["wind047"] = "加贺",--风 - 空 - 3血
	["iktianhua"] = "天华",
	[":iktianhua"] = "锁定技，回合结束时，或你的回合外每当你失去手牌时，若你的手牌数不大于一，你观看牌堆顶的五张牌，然后获得其中至少一张牌，将其余的牌以任意顺序置于牌堆顶。你始终跳过你的摸牌阶段和弃牌阶段。",
	["ikhuangshi"] = "煌矢",
	[":ikhuangshi"] = "出牌阶段开始时，或每当你受到1点伤害后，你可以弃置至多两张手牌，然后令你攻击范围外的一名角色弃置等量的牌。",
	["@ikhuangshi"] = "你可以发动“煌矢”",
	["~ikhuangshi"] = "选择至多两张手牌→选择目标角色→点击确定",

	["#wind048"] = "无印的堇心",
	["wind048"] = "涅普蒂努",--风 - 空 - 4血
	["ikxizi"] = "夕紫",
	[":ikxizi"] = "准备阶段开始时，若你有手牌，你可以展示之，并弃置其中一种颜色的所有的牌，然后摸两张牌。",

	["#wind051"] = "所罗门的梦魇",
	["wind051"] = "夕立",--风 - 空 - 4血
	["ikelu"] = "噩露",
	[":ikelu"] = "其他角色的出牌阶段开始时，你可以无视距离对其使用一张【杀】。若此【杀】造成了伤害，该角色不能使用【杀】，直到回合结束；否则，该角色计算与你的距离时，无视除你以外的其他角色和场上的坐骑牌，直到回合结束。",
	["@ikelu"] = "你可以发动“噩露”对 %src 使用一张【杀】",

--bloom
	["#bloom032"] = "灭犽的元神灵",
	["bloom032"] = "狮子神黑",--花 - 空 - 4血
	["ikfengxing"] = "风行",
	[":ikfengxing"] = "摸牌阶段开始时，你可以放弃摸牌，改为展示全部的手牌，若其中没有【杀】，你摸一张牌。你可以重复此流程，直到你展示的牌中有【杀】为止。",

	["#bloom034"] = "乐土的和声",
	["bloom034"] = "莉特丝•特尔提娜＆萨露莎•特尔提娜",--花 - 空 - 4血
	["&bloom034"] = "莉特丝＆萨露莎",
	["ikqizhong"] = "祈钟",
	[":ikqizhong"] = "当你于出牌阶段使用牌时，若此牌与你此阶段使用的上一张牌颜色不同，你可以亮出牌堆顶的一张牌，若亮出的牌与你使用的牌颜色不同，你获得之；否则，你可以用一张手牌替换之，或将该牌置入弃牌堆。",
	["@ikqizhong-exchange"] = "你可以用一张手牌替换亮出的【%arg】",

	["#bloom035"] = "天光的勇者",
	["bloom035"] = "游佐恵美",--花 - 空 - 4血
	["ikduduan"] = "独断",
	[":ikduduan"] = "准备阶段开始时，你可以摸一张牌或弃置你或你攻击范围内的一名角色的一张牌。若如此做，且你于此回合的出牌阶段没有造成伤害，你的手牌上限-2，直到回合结束。",
	["@ikduduan"] = "你发动了“独断”，可以弃置你或你攻击范围内的一名角色的一张牌，或点“取消”摸一张牌",

	["#bloom036"] = "豪雷的魔王",
	["bloom036"] = "北上",--花 - 空 - 4血
	["ikjimu"] = "亟幕",
	[":ikjimu"] = "限定技，出牌阶段，你可以令一名其他角色获得一枚“青火”标记。你计算与拥有“青火”标记的角色的距离时无视除该角色外的其他角色及场上的坐骑牌；你对其使用黑色【杀】在结算后，可以令一名其他角色选择一项：对该角色使用一张无视距离的【杀】，或令你摸一张牌。该角色死亡时，你将场上的“青火”标记转移给一名其他角色。",
	["@qinghuo"] = "青火",
	["@ikjimu"] = "请选择“亟幕”的目标",
	["@ikjimu-slash"] = "受“亟幕”影响，请对 %dest 使用一张无视距离的【杀】，或点“取消”令 %src 摸一张牌",

	["#bloom045"] = "鸣露的策士",
	["bloom045"] = "新子憧",--花 - 幻 - 4血
	["ikdengpo"] = "登破",
	[":ikdengpo"] = "出牌阶段限一次，你可以弃置X张牌并令一名牌数不少于X的其他角色弃置等量的牌（至多弃置四张），若如此做，此阶段结束时，你与其各摸X张牌。若你以此法弃置了你装备区的最后一张牌，你无视与该角色的距离，直到回合结束。",

	["#bloom047"] = "黑死斑的魔王",
	["bloom047"] = "佩丝特",--花 - 空 - 3血
	["ikguoshang"] = "国殇",
	[":ikguoshang"] = "锁定技，每当一名其他角色于你的回合内受到一次伤害后，若其有【酒】的效果，无效该效果，然后你获得其一张牌。",
	["ikzuiyan"] = "醉宴",
	[":ikzuiyan"] = "若你拥有技能“国殇”，出牌阶段开始时，你可令所有角色均视为使用一张【酒】；结束阶段开始时，有【酒】的效果的角色可以使用一张【杀】。",
	["#ikzuiyan"] = "醉宴（杀）",
	["@ikzuiyan-slash"] = "你可以发动 %src 的“醉宴”使用一张【杀】",
	["ikqihun"] = "祁魂",
	[":ikqihun"] = "你的回合内，每当一名有手牌的其他角色需要使用【杀】时，可以令你选择是否获得其全部手牌，若你如此做，视为其使用一张【杀】。",
	["ikqihunv"] = "祁魂",
	[":ikqihunv"] = "佩丝特的回合内，当你需要使用【杀】时，若你有手牌，可以令佩丝特选择是否获得你的全部手牌，若其如此做，视为你使用一张【杀】。",
	["ikqihun_slash:obtain"] = "你可以获得 %src 的全部手牌，视为其使用一张【杀】",

	["#bloom048"] = "琉璃之歌",
	["bloom048"] = "天海春香",--花 - 空 - 4血
	["ikdiebei"] = "楪呗",
	[":ikdiebei"] = "锁定技，准备阶段开始时，若你的手牌数不等于你的体力值，你摸两张牌，否则，你须失去1点体力，且你的手牌上限+1，直到回合结束。",

	["#bloom049"] = "南照之绘扇",
	["bloom049"] = "佐仓千代",--花 - 空 - 3血
	["ikxunfeng"] = "熏风",
	[":ikxunfeng"] = "出牌阶段，你于此阶段使用的第一张牌结算后，你可以摸一张牌，若如此做，你于此阶段使用的第二张牌结算后，你弃置一张牌。",
	["ikluhua"] = "露华",
	[":ikluhua"] = "一名其他角色的出牌阶段，该角色与此阶段使用的第三张牌结算后，你可以令其摸一张牌，若如此做，该角色于此阶段使用的第四张牌结算后，你摸一张牌。",

	["#bloom051"] = "凛熠的铳姬",
	["bloom051"] = "千斗五十铃",--花 - 空 - 4血
	["ikzhiyu"] = "织煜",
	[":ikzhiyu"] = "出牌阶段限一次，你可弃置一张牌，并失去1点体力，然后根据弃置的牌的类别获得相应效果直到回合结束：基本牌-你摸一张牌，且可将一张基本牌当任意基本牌使用或打出；锦囊牌-你摸两张牌，且使用的牌无距离限制；装备牌-你使用【杀】指定目标后，目标角色须弃置两张牌。",
	["ikzhiyu_slash"] = "织煜出杀",
	["ikzhiyu_saveself"] = "织煜自救",

--snow
	["#snow031"] = "兴国的公主",
	["snow031"] = "梅露露琳丝•蕾蒂•阿鲁兹",--雪 - 空 - 3血
	["&snow031"] = "梅露露琳丝",
	["iklingyun"] = "灵运",
	[":iklingyun"] = "你使用的基本牌在结算后置入弃牌堆时，你可以将之置于牌堆顶。",
	["ikmiyao"] = "秘药",
	[":ikmiyao"] = "一名其他角色的回合结束时，你可以将手牌补至或弃置至等同于其手牌数的张数。若你以此法获得了两张或更多的牌，你失去1点体力；若你以此法弃置了两张或更多的牌，你回复1点体力。",
	
	["#snow034"] = "恶趣的女仆",
	["snow034"] = "汐王寺茉莉花",--雪 - 空 - 4血
	["ikshidao"] = "侍道",
	[":ikshidao"] = "当你需要使用一张基本牌时，你可以将两张不同类别的牌交给一名其他角色，视为你使用了一张该名称的基本牌。<br />※操作提示：给牌的目标在点击“确定”后才选择，即“确定”确定前只需要选择此基本牌的目标",
	["ikshidao_saveself"] = "侍道自救",
	["ikshidao_slash"] = "侍道出杀",
	["@ikshidao"] = "请将这两张牌交给一名其他角色",
	
	["#snow035"] = "伊甸的黄昏",
	["snow035"] = "诗音",--雪 - 空 - 4血
	["ikshenshu"] = "神赎",
	[":ikshenshu"] = "出牌阶段，你可以将一张【桃】当【神惠雨降】使用。",
	["ikmingwang"] = "溟惘",
	[":ikmingwang"] = "出牌阶段限一次，你可以将一张黑桃牌当【罔两空界】使用。",
	["ikqiyi"] = "凄翼",
	[":ikqiyi"] = "在你的回合，每当一名角色回复1点体力后，你可以摸一张牌或弃置该角色的一张牌。",
	["ikqiyi:draw"] = "摸一张牌",
	["ikqiyi:discard"] = "弃置该角色的一张牌",
	
	["#snow036"] = "天赐的艺术家",
	["snow036"] = "椎名真白",--雪 - 空 - 3血
	--ikmitu
	["iklinghui"] = "灵绘",
	[":iklinghui"] = "出牌阶段限一次，你可令一名其他角色摸一张牌，然后弃置一张手牌，你可弃置任意张与该牌颜色相同的手牌，并令等量的角色依次摸两张牌。",
	["@iklinghui-discard"] = "请弃置一张手牌",
	["@iklinghui"] = "你可以弃置任意张颜色相同的手牌，并令等量的角色依次摸两张牌",
	["~iklinghui"] = "选择任意张颜色相同的手牌→选择等量的角色→点击确定",

	["#snow045"] = "虚电的妖精",
	["snow045"] = "星野琉璃",--雪 - 空 - 4血
	["iklvyan"] = "律衍",
	[":iklvyan"] = "出牌阶段，你可以将至少两张手牌当【杀】使用，若你以此法使用的【杀】被【闪】抵消时，你可以摸等量的牌。",
	["ikhuaiji"] = "怀计",
	[":ikhuaiji"] = "锁定技，结束阶段开始时，若你有手牌，你须将手牌补至或弃置至两张。",

	["#snow046"] = "纯恋的白华",
	["snow046"] = "濑名爱理",--雪 - 空 - 4血
	["iklianxiao"] = "恋咲",
	[":iklianxiao"] = "你距离1以内的角色的出牌阶段结束时，若其未于此阶段内对你使用过牌，你可以选择一项：令该角色摸一张牌；或令该角色手牌上限-1，直到回合结束。",
	["iklianxiao:draw"] = "令其摸一张牌",
	["iklianxiao:max"] = "令其手牌上限-1",

	["#snow048"] = "喧哄的问题儿",
	["snow048"] = "三枝叶留佳",--雪 - 空 - 4血
	["ikqile"] = "气乐",
	[":ikqile"] = "出牌阶段开始时，你可以弃置一张基本牌并摸两张牌，若如此做，你不能使用【杀】，直到回合结束。",
	["@ikqile"] = "你可以发动“气乐”",
	["iksaoxiao"] = "骚嚣",
	[":iksaoxiao"] = "每当你受到一次伤害，在结算后，你可以令伤害来源选择一种花色，然后你可以选择一张花色不同的手牌，并令一名角色使用之。",
	["#IkSaoxiaoChoice"] = "%from 选择了 %arg",
	["@iksaoxiao"] = "请选择一张花色不同的手牌",
	["@iksaoxiao-choose"] = "你可以令一名角色使用此牌",
	["@iksaoxiao-use"] = "请使用该牌",
	["~iksaoxiao"] = "选择该牌的目标→点击确定",

	["#snow049"] = "掌中萌虎",
	["snow049"] = "逢坂大河",--雪 - 空 - 4血
	["ikxiaowu"] = "虓武",
	[":ikxiaowu"] = "出牌阶段限一次，你可以将一张黑色非锦囊牌当【枯羽华庭】或方块牌当【春雪幻梦】无视合法性对你使用，然后摸一张牌并选择一名其他角色，若如此做，你无视与该角色的距离，直到回合结束，且视为你对其使用一张【杀】（此【杀】无视其防具，且不计入每阶段的使用限制）。",
	["#ikxiaowu"]="虓武（后续结算）",

	["#snow051"] = "九狱的大妖",
	["snow051"] = "羽衣狐",--雪 - 空 - 3血
	["ikwanmi"] = "万祢",
	[":ikwanmi"] = "游戏开始后，每当你的区域内置入一次牌时，你可以将这些牌当一张无视距离且不计入每阶段次数限制的基本牌使用，每回合限两次。",
	["ikwanmi_invoke:invoke"] = "你的 %arg 置入了新的牌，是否发动“万祢”",
	["ikwanmi_h"]="手牌",
	["ikwanmi_e"]="装备区",
	["ikwanmi_j"]="判定区",
	["@ikwanmi"] = "请发动“万祢”使用此牌",
	["~ikwanmi"] = "选择此牌的合法目标（若有）→点击确定",
	["ikguichan"] = "鬼缠",
	[":ikguichan"] = "你死亡时，你可以声明一种花色，若如此做，所有其他角色无法使用或打出该花色的牌直到其下一个回合结束。",

--luna
	["#luna030"] = "病娇的白蔷薇",
	["luna030"] = "雪华绮晶",--月 - 空 - 4血
	["iklingcu"] = "凌簇",
	[":iklingcu"] = "你可以跳过出牌阶段并将你的人物牌翻面，然后进行一次判定，判定牌生效后将其置于你的人物牌上。你须重复从判定开始的流程，直到你的人物牌上出现花色相同的牌为止，然后你的人物牌上每有一张牌，视为你对你攻击范围内的一名角色使用一张【杀】，在结算后，将你的人物牌上的全部的牌置入弃牌堆。",

	["#luna031"] = "灵心的炼金师",
	["luna031"] = "托托莉雅•赫尔默德",--月 - 空 - 3血
	["&luna031"] = "托托莉雅",
	["ikqisi"] = "奇思",
	[":ikqisi"] = "当一名手牌数不小于你的其他角色使用一张非延时类锦囊牌生效前，若你的手牌数为奇数，你可以摸一张牌，若如此做，视为你使用了一张【三粒天滴】。",
	["ikmiaoxiang"] = "妙想",
	[":ikmiaoxiang"] = "当一名手牌数不大于你的其他角色受到一次伤害后，若你的手牌数为偶数，你可以弃置一张非基本牌，然后令其回复1点体力。",
	["@ikmiaoxiang"] = "你可以发动“妙想”",

	["#luna032"] = "星咏的歌姬",
	["luna032"] = "初音未来",--月 - 幻 - 3血
	["ikjichang"] = "激唱",
	[":ikjichang"] = "准备阶段开始时，若你没有手牌，你可以摸四张牌；否则你可以展示你全部的手牌，每少一种花色，你摸一张牌。若你于本阶段获得了两张或更多的牌，跳过你的判定阶段和摸牌阶段。",
	["ikmanwu"] = "曼舞",
	[":ikmanwu"] = "出牌阶段限一次，你可与一名其他角色拼点，拼点赢的角色将手牌补至等同于其体力上限的张数。",

	["#luna033"] = "赤猫的琴手",
	["luna033"] = "中野梓",--月 - 空 - 4血
	["ikxianlv"] = "弦律",
	[":ikxianlv"] = "准备阶段或结束阶段开始时，你可以进行一次判定：若结果与你人物牌上任意一张牌花色均不同，你须将判定牌置于你的人物牌上，称为“乐”。一名角色的摸牌阶段开始时，你可以选择任意张“乐”并令其获得之，然后你摸一张牌，若如此做，该角色摸牌时须少摸一张牌。",
	["@ikxianlv"] = "你可以发动“弦律”交给其任意张“乐”",
	["~ikxianlv"] = "选择任意张“乐”→选择目标角色→点击确定",
	["music"] = "乐",

	["#luna036"] = "水风的皇裔",
	["luna036"] = "练红玉",--月 - 空 - 4血
	["iklianwu"] = "涟巫",
	[":iklianwu"] = "准备阶段开始时，你可以失去1点体力或弃置一张非基本牌，然后弃置一名其他角色的一张牌。若此牌为锦囊牌，你可以令至多X名角色摸一张牌；若此牌为基本牌，你计算与其他角色的距离时，始终-X，直到回合结束；若此牌为装备牌，你于此回合的出牌阶段可额外使用X张【杀】（X为你已损失的体力值，且至少为1）。",
	["@iklianwu0"] = "你可以失去1点体力或弃置一张装备牌发动“涟巫”",
	["@iklianwu1"] = "你可以令至多X名角色摸一张牌",
	["~iklianwu"] = "选择目标角色→点击确定",
	["~iklianwu1"] = "选择至多X名角色→点击确定",

	["#luna037"] = "天仪的孤风",
	["luna037"] = "岛风",--月 - 空 - 4血
	["ikmoshan"] = "魔闪",
	[":ikmoshan"] = "锁定技，若你拥有技能“衍梦”，你的【闪】均视为【净琉璃镜】。每当你即将失去装备区的【净琉璃镜】时，须将装备区的防具牌交给一名其他角色，然后弃置一名其他角色的一张牌。",
	["@ikmoshan"] = "请将装备区的防具牌交给一名其他角色",
	--thyanmeng

	["#luna045"] = "赤色之绊",
	["luna045"] = "千堂瑛里华",--月 - 空 - 4血
	["ikxieke"] = "血刻",
	[":ikxieke"] = "出牌阶段，若你的人物牌竖置，你可以视为你使用一张基本牌或【三粒天滴】，然后你将人物牌横置（存活角色的数量为2时，你只能以此法视为使用一张【杀】）。",
	["ikxieke_saveself"] = "血刻自救",
	["ikxieke_slash"] = "血刻出杀",
	["#ikxieke"] = "血刻（横置）",
	["ikyunmai"] = "运脉",
	[":ikyunmai"] = "回合结束时，若人物牌横置的角色数不少于X，你可以重置一名角色的人物牌；若人物牌竖置的角色数不少于X，你可以横置一名角色的人物牌（X为存活角色的数量的一半）。",
	["@ikyunmai"] = "你可以发动“运脉”",

	["#luna046"] = "物静之恋",
	["luna046"] = "小野寺小咲",--月 - 空 - 3血
	["iklunyao"] = "轮钥",
	[":iklunyao"] = "锁定技，你的牌即将被其他角色弃置或获得时，须以牌堆顶等量的牌替换之。",
	["ikqimu"] = "绮慕",
	[":ikqimu"] = "锁定技，你于一名角色的回合内使用的第一张牌，在结算后置入弃牌堆时，你须将其交给一名其他角色。",

	["#luna047"] = "银破的棺姬",
	["luna047"] = "嘉依卡•托拉庞特",--月 - 空 - 4血
	["&luna047"] = "嘉依卡",
	["ikyuanji"] = "元姬",
	[":ikyuanji"] = "锁定技，游戏开始时，你获得1枚“枢灵”标记。拥有该标记的角色的摸牌阶段须额外摸一张牌，且出牌阶段使用的第一张【杀】不计入每阶段的使用限制。拥有该标记的角色死亡时，你获得之；你死亡时，弃置场上的该标记。",
	["@shuling"] = "枢灵",
	["ikshuluo"] = "枢络",
	[":ikshuluo"] = "当一名角色对拥有“枢灵”标记的另一名角色造成一次伤害后，可以弃置两张牌并获得该标记（你为伤害来源时不需弃置牌）。",
	["@ikshuluo"] = "你可以弃置两张牌获得“枢灵”标记",
	
	["#luna048"] = "幻银的月华",
	["luna048"] = "月野兔",--月 - 空 - 4血
	["ikzhiwang"] = "织望",
	[":ikzhiwang"] = "锁定技，出牌阶段开始时，你须选择一项：视为对你攻击范围内的一名角色使用一张【杀】；或视为无视合法性对你使用一张【杀】。",
	["iklianlong"] = "帘胧",
	[":iklianlong"] = "锁定技，若你的手牌数为奇数，你计算与其他角色的距离+1；否则你计算与其他角色的距离-1（每当你使用牌，在结算结束前，你的手牌数视为等同于即将使用该牌时）。",

	["#luna049"] = "末广之歌",
	["luna049"] = "南小鸟",--月 - 空 - 3血
	["ikhuanxian"] = "环弦",
	[":ikhuanxian"] = "每当你需要使用一张【闪】时，所有其他角色可以依次交给你一张牌，若没有角色交给你牌，视为你使用了一张【闪】。",
	["@ikhuanxian-give"] = "%src 发动了“环弦”，你可以交给其一张牌",
	["ikwuyu"] = "舞雩",
	[":ikwuyu"] = "弃牌阶段结束时，你攻击范围内的角色可以依次获得此阶段内进入弃牌堆的一张牌，每有一名角色以此法获得一张红色牌，你回复1点体力。",
	["ikwuyu_get"] = "舞雩",
	
	["#luna051"] = "紫芍的缘业",
	["luna051"] = "藤林杏",--月 - 空 - 4血
	["ikkezhan"] = "恪瞻",
	[":ikkezhan"] = "锁定技，回合结束时，每达成以下一项，你便摸一张牌；若均不达成，你失去1点体力：\
1.你于此回合内造成过伤害\
2.你展示一张手牌，若为【杀】\
3.有至少一名内奸角色已死亡",
	["@ikkezhan"] = "由于“恪瞻”的效果，你须展示一张手牌",
}