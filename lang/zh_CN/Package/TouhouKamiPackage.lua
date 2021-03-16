-- translation for Kami Package

return {
	["touhou-kami"] = "东方的神祝",
	["touhou-kami_cards"] = "神魂魄妖梦专属卡牌",

	["#kami001"] = "往昔的圣迹",
	["kami001"] = "神东风谷早苗",--神 - 空 - 3血
	["&kami001"] = "神早苗",
	["designer:kami001"] = "幻桜落",
	["illustrator:kami001"] = "雛咲",
	["cv:kami001"] = "暂无",
	["thkexing"] = "客星",
	[":thkexing"] = "准备阶段开始时，你可以从牌堆顶亮出三张牌，将其中任意数量的非锦囊牌以任意顺序置于牌堆底，然后将其余的牌置入弃牌堆。<br />※双击置入弃牌堆的牌，然后点击确定。",
	["thshenfeng"] = "神风",
	[":thshenfeng"] = "阶段技。你可将两张相同颜色的牌交给一名体力值比你多的角色，然后该角色须对其距离为1的由你指定的一名角色造成1点伤害。",
	["thkaihai"] = "开海",
	[":thkaihai"] = "每当你失去最后的手牌时，可从牌堆底摸一张牌。",
	
	["#kami002"] = "天道是非",
	["kami002"] = "神比那名居天子",--神 - 空 - 4血
	["&kami002"] = "神天子",
	["designer:kami002"] = "幻桜落",
	["illustrator:kami002"] = "亜音",
	["cv:kami002"] = "暂无",
	["thtianbao"] = "天宝",
	[":thtianbao"] = "专属技。锁定技。游戏、准备阶段和结束阶段开始时，你须弃置你人物牌上的“灵”，然后将一张随机灵宝牌置于你的人物牌上，称为“灵”，并获得相应技能直到失去该“灵”。",
	--thyanmeng

	["#kami003"] = "混沌的巫女",
	["kami003"] = "神博丽灵梦",--神 - 空 - 8血
	["&kami003"] = "神灵梦",
	["designer:kami003"] = "幻桜落",
	["illustrator:kami003"] = "明星かがよ",
	["cv:kami003"] = "暂无",
	["thwudao"] = "巫道",
	[":thwudao"] = "锁定技。游戏开始时，你失去4点体力。准备阶段开始时，你失去或回复体力至X点（X为你已损失的体力值）。",
	["thhuanjun"] = "幻君",
	[":thhuanjun"] = "锁定技。你的方块【闪】均视为【碎月绮斗】，你手牌中的防具牌均视为【酒】，你获得即将进入你装备区的防具牌。",
	--thyanmeng
	
	["#kami004"] = "子夜的女皇",
	["kami004"] = "神蕾米莉娅•斯卡雷特",--神 - 空 - 4血
	["&kami004"] = "神蕾米",
	["designer:kami004"] = "昂翼天使",
	["illustrator:kami004"] = "墨洲",
	["cv:kami004"] = "暂无",
	["thgugao"] = "孤高",
	[":thgugao"] = "阶段技。你可以与一名其他角色拼点，赢的角色对没有赢的角色造成1点伤害；“千狱”发动后，防止你因没有赢而受到的伤害；“皇仪”发动后，若你赢，视为你此阶段没有发动“孤高”。",
	["thqianyu"] = "千狱",
	[":thqianyu"] = "觉醒技。一名角色的回合结束后，若你的体力值为1，你须将体力上限减少至1点，并获得技能“狂魔”（锁定技。专属技。每当你对一名其他角色造成除【杀】以外的1点伤害后，你增加1点体力上限，然后回复1点体力。），然后进行一个额外的回合。",
	["#ThQianyu"] = "%from 的体力值为 %arg2 ，触发“%arg”觉醒",
	["thkuangmo"] = "狂魔",
	[":thkuangmo"] = "锁定技。专属技。每当你对一名角色造成除【杀】以外的1点伤害后，你增加1点体力上限，然后回复1点体力。",
	["thhuangyi"] = "皇仪",
	[":thhuangyi"] = "觉醒技。“千狱”发动后，准备阶段开始时，若你的体力上限大于4点，你须减少至4点，并摸等同于减少数量的牌，然后回复1点体力，并失去技能“狂魔”。",
	["#ThHuangyi"] = "%from 的体力上限为 %arg2 ，触发“%arg”觉醒",
	
	["#kami005"] = "丑三时的幻之舞",
	["kami005"] = "神秦心",--神 - 幻 - 3血
	["&kami005"] = "神心",
	["designer:kami005"] = "昂翼天使",
	["illustrator:kami005"] = "Takibi",
	["cv:kami005"] = "暂无",
	["thsuhu"] = "肃狐",
	[":thsuhu"] = "摸牌阶段开始时，你可放弃摸牌，改为将牌堆顶的一张牌面朝上置于你的人物牌上，称为“面”，然后你可以使用一张手牌。",
	["@thsuhu"] = "请使用一张手牌",
	["thfenlang"] = "忿狼",
	[":thfenlang"] = "锁定技。专属技。每当你因受到伤害而扣减1点体力后，回复1点体力并摸两张牌。",
	["thleshi"] = "乐狮",
	[":thleshi"] = "阶段技。你可以选择一种牌的类别并摸四张牌，视为你此阶段没有使用过此类别的牌。",
	["#ThLeshi"] = "%from 选择了 %arg",
	["thyouli"] = "忧狸",
	[":thyouli"] = "锁定技。你的手牌上限始终为4。每当你的手牌数变化后，若大于四张，须将多余的作为“面”置于你的人物牌上。每当你的“面”的数量不小于两张时，须弃置两张“面”并失去1点体力。",
	["@thyouli-hand"] = "收“忧狸”影响，请通过将多余牌置于角色牌上的方法把手牌调整至四张",
	["thjingyuan"] = "惊猿",
	[":thjingyuan"] = "锁定技。出牌阶段，每种类别的牌你只能使用一张。",
	["mask"] = "面",

	["#kami006"] = "千宫神羽映华",
	["kami006"] = "神四季映姬",--神 - 空 - 3血
	["&kami006"] = "神映姬",
	["designer:kami006"] = "浪人兵法家",
	["illustrator:kami006"] = "DomotoLain",
	["cv:kami006"] = "暂无",
	["thpanghun"] = "彷魂",
	[":thpanghun"] = "摸牌阶段结束时，你可以弃置一张牌，然后你计算与所有其他角色的距离始终为1，且你的【杀】均视为【赤雾锁魂】，直到回合结束。",
	["@thpanghun"] = "你可以发动“彷魂”",
	["#thpanghun-fil"] = "彷魂",
	["thjingwu"] = "镜悟",
	[":thjingwu"] = "阶段技。你可以展示一张黑色锦囊牌，然后弃置场上的一张牌。",
	["thlunyu"] = "轮狱",
	[":thlunyu"] = "连舞技。当一张你不为其使用的目标（或之一）的牌置入弃牌堆时，你将这张牌置于牌堆顶，然后此阶段结束时你从牌堆底摸两张牌。",
	["thlunyu:prompt"] = "你可以发动“轮狱”，将 %arg[%arg2] 置于牌堆顶",
	["#ThLunyuPut"] = "%from 把 %card 置于牌堆顶",
	
--连舞技：每当你的一个技能发动或触发时，若此时你所有的除了连舞技外的游戏开始时便拥有的技能都已在当前回合内发动或触发，则直到回合结束，你可以发动一次连舞技。连舞技无法被其他角色获得。

	["#kami007"] = "华胥之梦",
	["kami007"] = "神西行寺幽幽子",--神 - 幻 - 2血
	["&kami007"] = "神幽幽子",
	["designer:kami007"] = "幻桜落",
	["illustrator:kami007"] = "siro",
	["cv:kami007"] = "暂无",
	["thfanhun"] = "返魂",
	[":thfanhun"] = "专属技。当你处于濒死状态时，你可获得1枚“咲”标记且体力回复至1点，然后将你的人物牌翻至正面朝上并重置之。",
	["thyoushang"] = "诱殇",
	[":thyoushang"] = "专属技。每当你的非黑桃牌对目标角色造成伤害时，你可以防止此伤害，并令该角色减少1点体力上限，然后获得1枚“咲”标记。",
	["thyouya"] = "幽雅",
	[":thyouya"] = "每当你受到伤害时，你可以弃置一张牌并令一至X名角色选择一项：打出一张【闪】；或令你获得其一张牌。（X为你的“咲”标记的数量）",
	["@thyouya"] = "你可以发动“幽雅”",
	["~thyouya"] = "选择一张牌→选择一至X名角色→点击确定",
	["@thyouya-jink"] = "请打出一张【闪】或点“取消”令 %src 获得一张牌",
	["thmanxiao"] = "满咲",
	[":thmanxiao"] = "锁定技。你每拥有1枚“咲”标记，你的手牌上限便+1。当你拥有4枚“咲”标记时，你立即死亡。",
	["@bloom"] = "咲",
	
	["#kami008"] = "夫众生者悉皆杀",
	["kami008"] = "神芙兰朵露•斯卡雷特",--神 - 幻 - 3血
	["&kami008"] = "神芙兰",
	["designer:kami008"] = "Danny",
	["illustrator:kami008"] = "Dhiea",
	["cv:kami008"] = "暂无",
	["thjinlu"] = "尽戮",
	[":thjinlu"] = "阶段技。你可以弃置一张手牌并指定一名其他角色，获得其全部的牌。此出牌阶段结束时，你须交给该角色等同于其体力值数量的牌，且结束阶段开始时将你的人物牌翻面。",
	["#thjinlu"] = "尽戮（后续结算）",
	["ThJinluGoBack"] = "请交给目标角色 %arg 张牌",
	["thkuangli"] = "狂戾",
	[":thkuangli"] = "每当你的人物牌翻面时，你可以摸一张牌。",
	
	["#kami009"] = "心华之殇",
	["kami009"] = "神古明地觉",--神 - 空 - 4血
	["&kami009"] = "神觉",
	["designer:kami009"] = "幻桜落",
	["illustrator:kami009"] = "ファルまろ",
	["cv:kami009"] = "暂无",
	["thyuxin"] = "愈心",
	[":thyuxin"] = "摸牌阶段开始时，你可以放弃摸牌，改为从牌堆顶亮出两张牌并获得之，若这两张牌均为红色，你可以指定一名角色，令其回复1点体力或摸一张牌。",
	["@thyuxin"] = "你可以发动“愈心”",
	["thyuxin-target"] = "你可以指定一名角色，令其回复1点体力或摸一张牌",
	["thyuxin:recover"] = "回复1点体力",
	["thyuxin:draw"] = "摸一张牌",
	["thchuangxin"] = "疮心",
	[":thchuangxin"] = "出牌阶段开始时，你可弃置X张牌，然后选择X项：<br />1. 获得技能“灵视”，直到回合结束<br />2. 获得技能“闭月”，直到回合结束",
	["@thchuangxin"] = "你可以发动“疮心”",
	["~thchuangxin"] = "选择1~2张牌→点击确定",
	["thtianxin"] = "天心",
	[":thtianxin"] = "你的回合开始时，你可弃置X张牌，然后选择X项：<br />1. 获得技能“虚视”，直到回合结束<br />2. 获得技能“天妒”，直到回合结束",
	["@thtianxin"] = "你可以发动“天心”",
	["~thtianxin"] = "选择1~2张牌→点击确定",
	
	["#kami010"] = "星煌的祈者",
	["kami010"] = "神霍青娥",--神 - 幻 - 3血
	["&kami010"] = "神青娥",
	["designer:kami010"] = "昂翼天使",
	["illustrator:kami010"] = "c7肘",
	["cv:kami010"] = "暂无",
	["thrangdeng"] = "禳灯",
	[":thrangdeng"] = "锁定技。出牌阶段开始时，你须选择一项：依次将至多三张与你人物牌上的任何一张牌点数都不相同的手牌面朝上置于你的人物牌上，称为“灯”；或弃置一张手牌。你的人物牌上每有一种花色的“灯”，你获得相应的技能：红桃“闭月”；黑桃“飞影”；方块“沉红”；梅花“霁风”。",
	["lantern"] = "灯",
	["@thrangdeng"] = "请将一张手牌置于人物牌上",
	["thbaihun"] = "拜魂",
	[":thbaihun"] = "出牌阶段，你可以将十三张“灯”置入弃牌堆，然后令一名其他角色立即死亡。",
	
	["#kami011"] = "创刻的幻界",
	["kami011"] = "神八云紫",--神 - 空 - 4血
	["&kami011"] = "神紫",
	["designer:kami011"] = "幻桜落",
	["illustrator:kami011"] = "DomotoLain",
	["cv:kami011"] = "暂无",
	["thxujing"] = "虚境",
	[":thxujing"] = "若你拥有技能“灵殒”，你的回合外，每当你成为以下牌的目标后，你可以失去一项人物技能并摸一张牌，然后令此牌的使用者获得相应技能，直到你的下一个回合结束：1.黑色非延时类锦囊牌-“幻葬”和“暗月”；2.你的【桃】或【酒】-“霁风”和“隙境”。",
	["thlingyun"] = "灵殒",
	[":thlingyun"] = "出牌阶段，若你不拥有相应技能，你可以弃置一张牌，并获得该技能，视为使用以下一张牌：<br />1.【绯想镜诗】－“崩坏”；<br />2.【灼狱业焰】－“散灵”；<br />3.【心网密葬】－“心殇”；<br />4.【赤雾锁魂】－“禁恋”。",
	["thzhaoai"] = "朝霭",
	[":thzhaoai"] = "锁定技。当你因其他角色对你造成的伤害死亡时，你令一名其他角色获得技能“神宝”和“浴火”，并摸X张牌（X为你的技能数量）。",
	
	["#kami012"] = "竹取飞翔",
	["kami012"] = "神蓬莱山辉夜",--神 - 空 - 4血
	["&kami012"] = "神辉夜",
	["designer:kami012"] = "幻桜落",
	["illustrator:kami012"] = "わたあめ",
	["cv:kami012"] = "暂无",
	["thwunan"] = "五难",
	[":thwunan"] = "当一名其他角色使用【神惠雨降】或【竹取谜宝】时，或当一名其他角色于濒死状态回复体力后（若其体力值不小于1），或当一名其他角色受到火属性伤害后，或当一名其他角色使用【净琉璃镜】的效果而使用或打出【闪】时，或当一名其他角色使用【杀】对没有手牌的角色造成伤害时，你可以选择一项：1.摸一张牌；2.弃置该角色的一张牌。然后若你是体力值最小的角色，你可以回复1点体力。每回合限一次。",
	["thwunan:draw"] = "摸一张牌",
	["thwunan:throw"] = "弃置该角色的一张牌",
	["thwunan_recover:yes"] = "你可以回复1点体力",

	["#kami013"] = "鸣蛙不输风雨",
	["kami013"] = "神洩矢诹访子",--神 - 幻 - 1血
	["&kami013"] = "神诹访子",
	["designer:kami013"] = "幻桜落",
	["illustrator:kami013"] = "6U",
	["cv:kami013"] = "暂无",
	["thsanling"] = "散灵",
	[":thsanling"] = "锁定技。一名角色的回合结束后，若你没有手牌，你立即死亡。",
	["thbingzhang"] = "氷障",
	[":thbingzhang"] = "每当你受到伤害结算开始时或即将失去体力时，你可以弃置两张牌，然后防止此伤害或此次失去体力。",
	["@thbingzhang"] = "你可以发动“氷障”",
	["~thbingzhang"] = "选择两张牌→点击“确定”",
	["thjiwu"] = "极武",
	[":thjiwu"] = "锁定技。你的【桃】和【酒】均视为【闪】。你的回合外，每当你的【闪】、【桃】或【酒】置入弃牌堆或其他角色每获得你的一张手牌时，你摸一张牌。",
	["thsisui"] = "祀祟",
	[":thsisui"] = "专属技。准备阶段开始时，你可以令所有其他角色各弃置一张手牌，你从弃牌堆获得这些牌中的至多两张牌，然后你可以依次交给一名其他角色。",
	["@thsisui"] = "受“祀祟”影响，你须弃置一张手牌",
	["thzhanying"] = "湛樱",
	[":thzhanying"] = "锁定技。你始终跳过你的摸牌阶段；你的手牌上限+4。",
	
	["#kami014"] = "诞妄的妖姬",
	["kami014"] = "神封兽鵺",--神 - 幻 - 3血
	["&kami014"] = "神鵺",
	["designer:kami014"] = "韩旭",
	["illustrator:kami014"] = "目薬",
	["cv:kami014"] = "暂无",
	["thluli"] = "陆离",
	[":thluli"] = "当你对体力值不小于你的一名其他角色造成伤害，或一名体力值不小于你的其他角色对你造成伤害时，你可以弃置一张黑色手牌令你造成的伤害+1；或弃置一张红色手牌令你受到的伤害-1。",
	["@thluli-increase"] = "你可以弃置一张黑色手牌令 %src 受到的伤害+1",
	["@thluli-decrease"] = "你可以弃置一张红色手牌令 %src 造成的伤害-1",
	["#ThLuliIncrease"] = "%from 发动了“<font color=\"yellow\"><b>陆离</b></font>”，伤害点数从 %arg 点增加至 %arg2 点",
	["#ThLuliDecrease"] = "%from 发动了“<font color=\"yellow\"><b>陆离</b></font>”，伤害点数从 %arg 点减少至 %arg2 点",
	["thguihuan"] = "诡幻",
	[":thguihuan"] = "限定技。若你的身份不是君主，当你杀死一名身份不是君主的其他角色，在其翻开身份牌之前，你可以与该角色交换身份牌。",
	
	["#kami015"] = "冰魄的女仙",
	["kami015"] = "神琪露诺",--神 - 空 - 4血
	["&kami015"] = "神琪露诺",
	["designer:kami015"] = "游卡桌游",
	["illustrator:kami015"] = "ヨシュア",
	["cv:kami015"] = "暂无",
	["thzhizun"] = "至尊",
	[":thzhizun"] = "准备阶段和结束阶段开始时，你可以改变一名其他角色的势力属性，然后可以获得一项技能（你只能以此法获得“心契”、“唤卫”、“济援”、“御姬”、“华祇”、“颂威”、“春度”或“舞华”，且无法获得场上其他存活角色拥有的以上技能）。",
	["@thzhizun"] = "你可以发动“至尊”改变一名其他角色的势力属性",
	["thzhizun_kingdom"] = "至尊",
	["thzhizun_lordskills"] = "至尊",
	["thfeiying"] = "飞影",
	[":thfeiying"] = "锁定技。当其他角色计算与你的距离时，始终+1。",
	
	["#kami016"] = "最古妖王的宝库",
	["kami016"] = "神因幡帝",--神 - 幻 - 3血
	["&kami016"] = "神帝",
	["designer:kami016"] = "幻桜落",
	["illustrator:kami016"] = "ke-ta",
	["cv:kami016"] = "暂无",
	["thlijian"] = "离剑",
	[":thlijian"] = "限定技。准备阶段开始时，你可以展示一张手牌，所有其他角色须依次选择一项：交给你一张与此牌类别相同的牌；或受到你对其造成的1点伤害。",
	["@thlijian-show"] = "请展示一张手牌",
	["@thlijian-give"] = "请交给 %src 一张类别相同的牌",
	["thsiqiang"] = "死枪",
	[":thsiqiang"] = "限定技。出牌阶段，你可以选择一名其他角色，令其不能使用或打出牌，直到回合结束。",
	["thjiefu"] = "戒符",
	[":thjiefu"] = "限定技。出牌阶段，你可弃置一名其他角色装备区的所有牌，然后令其全部的人物技能无效，直到回合结束。",
	["thhuanxiang"] = "幻乡",
	[":thhuanxiang"] = "限定技。当你受到伤害结算开始时，你可以防止此伤害，且将你的体力回复至3点；然后你不是其他角色使用牌的合法目标，且每当你受到伤害结算开始时，防止该伤害，直到你的下回合开始。",

	["#kami017"] = "诹访明神",
	["kami017"] = "神八坂神奈子",--神 - 空 - 4血
	["&kami017"] = "神神奈子",
	["designer:kami017"] = "桀",
	["illustrator:kami017"] = "はねぽち",
	["cv:kami017"] = "暂无",
	["thshuangfeng"] = "霜风",
	[":thshuangfeng"] = "阶段技。与“霰湖”互斥。你可以亮出牌堆顶的四张牌，将其中一个类别的牌置入弃牌堆，其余以任意顺序置于牌堆底。",
	["thxianhu"] = "霰湖",
	[":thxianhu"] = "阶段技。与“霜风”互斥。你可以随机将弃牌堆的四张牌置入处理区，将其中一个花色的牌置入手牌，其余以任意顺序置于牌堆底。",
	["thxingyong"] = "星涌",
	[":thxingyong"] = "每当你受到1点伤害后，你可选择令“霜风”亮出牌堆顶或“霰湖”置入处理区的牌数+1。",
	["thzaishen"] = "在神",
	[":thzaishen"] = "限定技。一名角色的回合结束时，若牌堆仅剩你置于牌堆底的牌，你可选择一个牌的名称，对其使用牌堆所有该名称的牌（这些牌对其他角色无效）。牌堆洗切时，重置此技能。",
	["@mountain"] = "霜风",
	["@lake"] = "霰湖",

	["#kami018"] = "永萃异梦",
	["kami018"] = "神赫卡缇雅•拉碧丝拉祖利",--神 - 空 - 3血
	["&kami018"] = "神赫卡缇雅",
	["designer:kami018"] = "永恒の须臾",
	["illustrator:kami018"] = "60枚",
	["cv:kami018"] = "暂无",
	["thsanjie"] = "三界",
	[":thsanjie"] = "专属技。锁定技。游戏开始时，你处于“地界”状态。你根据不同的状态拥有不同的效果。<br />“地界”：每当其他角色回复一次体力后，你摸一张牌；每当你使用红色基本牌后，你弃置一名其他角色一张牌。当你弃置牌后，切换至“月界”；当你获得其他角色的牌后，切换至“异界”。<br />“月界”：每当你使用黑色基本牌后，你摸一张牌；每当你使用一张非延时锦囊牌后，你弃置一名其他角色的一张牌。当你受到伤害后，切换至“异界”；当其他角色回复一次体力后，切换至“地界”。<br />“异界”：每当你受到一次伤害后，你摸一张牌；每当你弃置一次牌后，你弃置一名其他角色的一张牌。当你使用黑色基本牌后，切换至“地界”；当你使用非基本牌后，切换至“月界”。",
	["@thsanjie"] = "请弃置一名其他角色的一张牌",

	["#kami019"] = "苍天的庭师",
	["kami019"] = "神魂魄妖梦",--神 - 空 - 4血
	["&kami019"] = "神妖梦",
	["designer:kami019"] = "Slob",
	["illustrator:kami019"] = "ファルケン",
	["cv:kami019"] = "暂无",
	["thjiesha"] = "劫杀",
	[":thjiesha"] = "阶段技。你可以弃置你装备区所有的牌并选择包括你在内的等量角色，这些角色各摸1张牌，然后你对这些角色各造成1点伤害。",
	["thmingren"] = "冥刃",
	[":thmingren"] = "出牌阶段，若你的武器栏没有牌，你可以将黑色手牌当【楼观剑】，红色手牌当【空观剑】使用。",
	["thchuntie"] = "錞铁",
	[":thchuntie"] = "每名角色的回合限一次，当你失去装备区里的武器牌前，你可以将之交给一名其他角色，然后你摸一张牌。",
	["@thchuntie"] = "你可以发动“錞铁”",

	["kuukanken"] = "空观剑",
	[":kuukanken"] = "装备牌·武器<br /><b>攻击范围</b>：２～２<br /><b>武器技能</b>：锁定技。你使用【杀】的额定目标上限+1。当你使用的无属性【杀】结算结束后，若此【杀】的目标数为1，你于此回合内使用【杀】的次数上限便+1。当装备区的【空观剑】进入非装备区的区域时，此牌的全部效果失效。",

	["#kami020"] = "隽智兼思",
	["kami020"] = "神八意永琳",--神 - 空 - 3血
	["&kami020"] = "神永琳",
	["designer:kami020"] = "戴耳机的妖精",
	["illustrator:kami020"] = "鏡 Area",
	["cv:kami020"] = "暂无",
	["thyuexiang"] = "月相",
	[":thyuexiang"] = "专属技。锁定技。游戏开始时，你拥有处于“既朔”状态的“月相”。每当一个回合结束后，你将月相按“既朔—上弦—既望—下弦—既朔”的顺序调整至下个状态。当前回合角色的出牌阶段开始时，其根据月相发动对应的效果。<br />既朔：当前回合角色使用【杀】仅能指定距离最近的其他角色为目标。<br />上弦：当前回合角色可移动场上的一张牌。<br />既望：当前回合角色明置所有手牌；使用【杀】无距离限制。<br />下弦：当前回合角色于此阶段结束后额外执行一个出牌阶段（额外阶段不受任何月相影响）。",
	["@thyuexiang-move"] = "受到“月相”影响，你可以移动场上的一张牌",
	["@thyuexiang-to"] = "请选择移动【%arg】的目标角色",
	["thmishu"] = "秘术",
	[":thmishu"] = "每轮限一次。一名角色的准备阶段开始时，你可将月相调整至“大晦”。“大晦”月相的回合结束后，将月相调整至“既朔”。<br />大晦：当前回合角色跳过弃牌阶段；所有角色均不是【杀】的合法目标。",
}