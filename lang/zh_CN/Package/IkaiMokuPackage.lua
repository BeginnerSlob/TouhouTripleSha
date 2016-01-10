-- translation for IkaiKiPackage

return {
	["ikai-moku"] = "异世界的木阴",
	
--wind
	["#wind008"] = "风华绝代的圣者",
	["wind008"] = "桔梗",--风 - 空 - 4血
	["designer:wind008"] = "游卡桌游",
	["illustrator:wind008"] = "IHET",
	["ikliegong"] = "烈弓",
	[":ikliegong"] = "出牌阶段，当你使用【杀】指定一名角色为目标后，若其手牌数不小于你的体力值或者不大于你的攻击范围，你可以令此【杀】不可以被【闪】响应，然后令该角色的非锁定技无效，直到回合结束。",
	["ikhuanghun"] = "荒魂",
	[":ikhuanghun"] = "出牌阶段，你可以重铸一张锦囊牌。",

	["#wind009"] = "血风的旁观者",
	["wind009"] = "蕾琪尔•阿尔卡德",--风 - 空 - 4血
	["&wind009"] = "蕾琪尔",
	["designer:wind009"] = "游卡桌游",
	["illustrator:wind009"] = "daiaru",
	["ikkuanggu"] = "狂骨",
	[":ikkuanggu"] = "锁定技，每当你对距离1以内的一名其他角色造成1点伤害后，你回复1点体力或摸两张牌（若为【杀】造成的伤害，你不可以选择摸牌）。",
	["ikkuanggu:recover"] = "回复1点体力",
	["ikkuanggu:draw"] = "摸两张牌",

	["#wind010"] = "神风的不死鸟",
	["wind010"] = "响",--风 - 空 - 3血
	["designer:wind010"] = "游卡桌游",
	["illustrator:wind010"] = "八城惺架",
	["ikfuhua"] = "缚华",
	[":ikfuhua"] = "出牌阶段，你可以将一张梅花牌当【赤雾锁魂】使用或重铸。",
	["iksuinie"] = "碎涅",
	[":iksuinie"] = "限定技，当你处于濒死状态时，你可以将你判定区里全部的牌置入弃牌堆，然后将你的人物牌翻至正面朝上，并重置之，再摸三张牌且体力回复至3点。",

	["#wind011"] = "灼烂的炎魔",
	["wind011"] = "五河琴里",--风 - 空 - 3血
	["designer:wind011"] = "游卡桌游",
	["illustrator:wind011"] = "つなこ",
	["ikjingnie"] = "净涅",
	[":ikjingnie"] = "锁定技，专属技，若你的装备区没有防具牌，视为你装备着【净琉璃镜】。",
	["ikjianyan"] = "歼焰",
	[":ikjianyan"] = "出牌阶段，你可以将一张红色牌当【灼狱业焰】使用。",
	["ikxuanying"] = "绚影",
	[":ikxuanying"] = "你可以将一张黑色牌当【三粒天滴】使用。",

	["#wind012"] = "堕天的圣黑猫",
	["wind012"] = "五更瑠璃",--风 - 空 - 4血
	["designer:wind012"] = "游卡桌游",
	["illustrator:wind012"] = "キムラダイスケ",
	["iktiaoxin"] = "挑衅",
	[":iktiaoxin"] = "出牌阶段限一次，你可以指定一名你在其攻击范围内的其他角色，该角色选择一项：对你使用一张【杀】，或令你弃置其一张牌。",
	["@iktiaoxin-slash"] = "%src 对你发动“挑衅”，请对其使用一张【杀】",
	["ikshengtian"] = "圣天",
	[":ikshengtian"] = "觉醒技，准备阶段开始时，若你没有手牌，你须回复1点体力或摸两张牌，然后减少1点体力上限，并获得技能“玄舞”和“墨华”（锁定技，你的方块牌均视为梅花牌。）。",
	["ikshengtian:draw"] = "摸两张牌",
	["ikshengtian:recover"] = "回复1点体力",
	["#IkShengtianWake"] = "%from 没有手牌，触发“%arg”觉醒",
	["ikmohua"] = "墨华",
	[":ikmohua"] = "锁定技，你的方块牌均视为梅花牌。",

	["#wind013"] = "捣蛋万岁",
	["wind013"] = "岁纳京子",--风 - 空 - 4血
	["designer:wind013"] = "游卡桌游",
	["illustrator:wind013"] = "みわべさくら",
	["ikhuoshou"] = "祸首",
	[":ikhuoshou"] = "锁定技，专属技，【百鬼夜行】对你无效；你是任何【百鬼夜行】造成伤害的来源。",
	["#sa_avoid_ikhuoshou"] = "祸首（无效）",
	["ikzailuan"] = "再乱",
	[":ikzailuan"] = "摸牌阶段开始时，若你已受伤，你可放弃摸牌，改为展示牌堆顶的X张牌（X为你已损失的体力值），其中每有一张红桃牌，你回复1点体力或摸两张牌，然后弃置这些红桃牌，并获得其余的牌。",
	["ikzailuan:recover"] = "回复1点体力",
	["ikzailuan:draw"] = "摸两张牌",

	["#wind014"] = "宙回之零",
	["wind014"] = "楯山文乃",--风 - 空 - 3血
	["designer:wind014"] = "游卡桌游",
	["illustrator:wind014"] = "V-hu_愁音",
	["ikchihu"] = "赤帍",
	[":ikchihu"] = "锁定技，当其他角色使用【杀】指定你为目标时，需额外弃置一张基本牌，否则该【杀】对你无效。",
	["@ikchihu-discard"] = "你须再弃置一张基本牌使此【杀】生效",
	["ikyouxing"] = "攸幸",
	[":ikyouxing"] = "专属技，你可以跳过你的出牌阶段，然后在回合结束时可以弃置一张手牌令一名其他角色进行一个额外的回合。",
	["@ikyouxing-give"] = "你可以弃置一张手牌令一名其他角色进行一个额外的回合",
	["~ikyouxing"] = "选择一张手牌→选择一名其他角色→点击确定",
	["#IkYouxing"] = "%to 将进行一个额外的回合",
	["ikmugua"] = "目挂",
	[":ikmugua"] = "君主技，觉醒技，准备阶段开始时，若你的体力值是全场最少的（或之一），你须增加1点体力上限，回复1点体力，并获得技能“心契”。",
	["#IkChihu"] = "%from 受到“%arg”的影响，需弃置一张基本牌才能令此【杀】对 %to 生效。",
	["#IkMuguaWake"] = "%from 的体力值 %arg 为场上最少，触发“%arg2”觉醒",

	["#wind015"] = "绯色菖蒲",
	["wind015"] = "星伽白雪",--风 - 幻 - 4血
	["designer:wind015"] = "游卡桌游",
	["illustrator:wind015"] = "暴力にゃ長",
	["ikjugui"] = "巨鬼",
	[":ikjugui"] = "锁定技，【百鬼夜行】对你无效；若其他角色使用的【百鬼夜行】，在结算后置入弃牌堆时，你获得之。",
	["#sa_avoid_ikjugui"] = "巨鬼（无效）",
	["iklieren"] = "烈刃",
	[":iklieren"] = "每当你使用【杀】或【碎月绮斗】对目标角色造成一次伤害后，可与其拼点，若你赢，你获得该角色的一张牌。",

	["#wind029"] = "北斗之蛟",
	["wind029"] = "土御门夏目",--风 - 空 - 3血
	["designer:wind029"] = "游卡桌游",
	["illustrator:wind029"] = "Anmi",
	["ikqiyao"] = "七曜",
	[":ikqiyao"] = "分发起始手牌时，共发你十一张牌，你将其中七张牌面朝下置于你的人物牌上，称为“曜”；摸牌阶段结束时，你可以用任意数量的手牌替换等量的“曜”。",
	["#ikqiyao"] = "七曜",
	["stars"] = "曜",
	["@ikqiyao-exchange"] = "请选择 %arg 张手牌用于交换",
	["#IkQiyaoExchange"] = "%from 发动了“%arg2”，交换了 %arg 张手牌",
	["ikliefeng"] = "烈风",
	[":ikliefeng"] = "结束阶段开始时，你可以弃置一张“曜”，并选择一名角色，若如此做，每当其受到的火焰伤害结算开始时，此伤害+1，直到你的下回合开始。",
	["#ikliefeng"] = "烈风",
	["@liefeng"] = "烈风",
	["@ikliefeng-card"] = "你可以发动“烈风”",
	["~ikliefeng"] = "选择一名角色→点击确定→然后在窗口中选择一张牌",
	["#IkLiefengPower"] = "“<font color=\"yellow\"><b>烈风</b></font>”效果被触发，%from 的火焰伤害从 %arg 点增加至 %arg2 点",
	["ikmiaowu"] = "渺雾",
	[":ikmiaowu"] = "结束阶段开始时，你可以弃置X张“曜”，并选择X名角色，若如此做，每当这些角色受到的非雷电伤害结算开始时，防止此伤害，直到你的下回合开始。",
	["@miaowu"] = "渺雾",
	["@ikmiaowu-card"] = "你可以发动“渺雾”",
	["~ikmiaowu"] = "选择若干名角色→点击确定→然后在窗口中选择相应数量的牌",
	["#IkMiaowuProtect"] = "%from 的“<font color=\"yellow\"><b>渺雾</b></font>”效果被触发，防止了 %arg 点伤害[%arg2]",

	["#wind030"] = "冷夜的花火",
	["wind030"] = "灰原哀",--风 - 空 - 2血
	["designer:wind030"] = "游卡桌游",
	["illustrator:wind030"] = "栗川鮫弥",
	["ikjuejing"] = "绝境",
	[":ikjuejing"] = "锁定技，摸牌阶段，你摸牌的数量改为你已损失的体力值+2；你的手牌上限+2。",
	["ikzhihun"] = "智魂",
	[":ikzhihun"] = "你可以将同花色的X张牌按下列规则使用或打出：红桃当【桃】，黑桃当具火焰伤害的【杀】，梅花当【闪】，方块当【三粒天滴】（X为你当前的体力值且至少为1）。",

--bloom
	["#bloom008"] = "黑扬羽蝶",
	["bloom008"] = "黑雪姬",--花 - 空 - 4血
	["designer:bloom008"] = "游卡桌游",
	["illustrator:bloom008"] = "八神",
	["ikxunyu"] = "迅羽",
	[":ikxunyu"] = "你可以选择一至两项：<br />1.跳过你此回合的判定阶段和摸牌阶段<br />2.跳过你此回合出牌阶段并弃置一张非锦囊牌<br />你每选择一项，视为对一名其他角色使用一张无视距离的【杀】。",
	["@ikxunyu1"] = "你可以跳过判定阶段和摸牌阶段发动“迅羽”",
	["@ikxunyu2"] = "你可以跳过出牌阶段并弃置一张非锦囊牌发动“迅羽”",
	["~ikxunyu1"] = "选择【杀】的目标角色→点击确定",
	["~ikxunyu2"] = "选择一张非锦囊牌→选择【杀】的目标角色→点击确定",

	["#bloom009"] = "白樱之华",
	["bloom009"] = "桂雏菊",--花 - 空 - 4血
	["designer:bloom009"] = "游卡桌游",
	["illustrator:bloom009"] = "H2SO4",
	["ikmancai"] = "曼才",
	[":ikmancai"] = "你可以弃置一张手牌跳过你的一个阶段（准备阶段和结束阶段除外），若以此法跳过摸牌阶段，你获得其他一至两名角色各一张手牌；若以此法跳过出牌阶段，你可以将场上的一张牌移动到另一名角色区域内的相应位置。",
	["@ikmancai-2"] = "你可以依次获得一至两名其他角色的各一张手牌",
	["@ikmancai-3"] = "你可以将场上的一张牌移动至另一名角色相应的区域内",
	["#ikmancai-1"] = "你可以弃置 %arg 张手牌跳过判定阶段",
	["#ikmancai-2"] = "您可以弃置 %arg 张手牌跳过摸牌阶段",
	["#ikmancai-3"] = "您可以弃置 %arg 张手牌跳过出牌阶段",
	["#ikmancai-4"] = "您可以弃置 %arg 张手牌跳过弃牌阶段",
	["~ikmancai2"] = "选择 1-2 名其他角色→点击确定",
	["~ikmancai3"] = "选择一名角色→点击确定",
	["@ikmancai-to"] = "请选择移动【%arg】的目标角色",

	["#bloom010"] = "汲血的死徒",
	["bloom010"] = "弓塚五月",--花 - 空 - 4血
	["designer:bloom010"] = "游卡桌游",
	["illustrator:bloom010"] = "白森ゆせ",
	["ikkujie"] = "枯界",
	[":ikkujie"] = "出牌阶段，你可以将一张黑色非锦囊牌当【枯羽华庭】使用；你可以对距离2以内的一名其他角色使用【枯羽华庭】。",

	["#bloom011"] = "轻国的帝王",
	["bloom011"] = "泉此方",--花 - 空 - 4血
	["designer:bloom011"] = "Danny",
	["illustrator:bloom011"] = "虎顎かずや",
	["ikzhaihun"] = "宅魂",
	[":ikzhaihun"] = "结束阶段开始时，你可以摸2+X张牌，然后将你的人物牌翻面。若如此做，你的下一个摸牌阶段开始时，你须弃置X张牌（X为场上武器牌的数量）。",
	["ikfojiao"] = "佛脚",
	[":ikfojiao"] = "若你的手牌数大于你的体力值，你可以将你装备区里的装备牌当【三粒天滴】使用。",
	["#IkZhaihunDiscard"] = "%from 的“%arg2”效果被触发，须弃置 %arg 张牌";

	["#bloom012"] = "金色之暗",
	["bloom012"] = "伊芙",--花 - 空 - 4血
	["designer:bloom012"] = "游卡桌游",
	["illustrator:bloom012"] = "syokuyou-mogura",
	["ikqiangxi"] = "强袭",
	[":ikqiangxi"] = "出牌阶段限一次，你可以失去1点体力或弃置一张武器牌，并对一名其他角色造成1点伤害。",

	["#bloom013"] = "云裳的巫女",
	["bloom013"] = "姬宫千歌音",--花 - 空 - 3血
	["designer:bloom013"] = "游卡桌游",
	["illustrator:bloom013"] = "たくなま",
	["ikyushen"] = "御神",
	[":ikyushen"] = "出牌阶段限一次，你可以与一名体力比你多的角色拼点，若你赢，则该角色对其攻击范围内由你指定的一名角色造成1点伤害；若你没赢，则其对你造成1点伤害。",
	["@ikyushen-damage"] = "请选择 %src 攻击范围内的一名角色",
	["#IkYushenNoWolf"] = "%from “<font color=\"yellow\"><b>御神</b></font>”拼点赢，由于 %to 攻击范围内没有角色，结算中止",
	["ikjieming"] = "节命",
	[":ikjieming"] = "每当你受到一次伤害后，可令一名角色将手牌补至等同于其体力上限的张数（至多补至五张）。",
	["ikjieming-invoke"] = "你可以发动“节命”<br/> <b>操作提示</b>: 选择一名角色→点击确定<br/>",

	["#bloom014"] = "世界之心",
	["bloom014"] = "凉宫春日",--花 - 空 - 3血
	["designer:bloom014"] = "游卡桌游",
	["illustrator:bloom014"] = "cuteg",
	["iktanwan"] = "叹惋",
	[":iktanwan"] = "你可以获得死亡角色的所有牌。",
	["ikbisuo"] = "闭锁",
	[":ikbisuo"] = "每当你受到一次伤害后，可以令一名其他角色摸X张牌（X为你已损失的体力值），然后该角色将其人物牌翻面。",
	["ikbisuo-invoke"] = "你可以发动“闭锁”<br/> <b>操作提示</b>: 选择一名其他角色→点击确定<br/>",
	["iksongwei"] = "颂威",
	[":iksongwei"] = "君主技，其他花势力角色的判定牌为黑色且生效后，可以令你摸一张牌。",

	["#bloom015"] = "银月的舞姬",
	["bloom015"] = "丽夏麻绪",--花 - 空 - 4血
	["designer:bloom015"] = "游卡桌游",
	["illustrator:bloom015"] = "さとみ",
	["ikyindie"] = "隠蝶",
	[":ikyindie"] = "你的回合外，每当失去牌时，可以进行一次判定，若不为红桃，将该判定牌置于你的人物牌上，称为“隐”；每有一张“隐”，你计算与其他角色的距离-1。",
	["#ikyindie-move"] = "隠蝶（获得判定牌）",
	["#ikyindie-dist"] = "隠蝶",
	["ikyindiepile"] = "隐",
	["ikguiyue"] = "鬼月",
	[":ikguiyue"] = "觉醒技，准备阶段开始时，若你的“隐”的数量为三张或更多时，你须减少1点体力上限，并获得技能“幻舞”（出牌阶段，你可以将一张“隐”当【妙手探云】使用）。",
	["#IkGuiyueWake"] = "%from 的“隐”为 %arg 张，触发“%arg2”觉醒",
	["ikhuanwu"] = "幻舞",
	[":ikhuanwu"] = "出牌阶段，你可以将一张“隐”当【妙手探云】使用。",
	
	["#bloom029"] = "军火巨枭",
	["bloom029"] = "蔻蔻•海克梅迪亚",--花 - 空 - 3血
	["designer:bloom029"] = "游卡桌游",
	["illustrator:bloom029"] = "硯",
	["&bloom029"] = "蔻蔻",
	["ikyihuo"] = "易货",
	[":ikyihuo"] = "其他角色的出牌阶段限一次，若你的装备区有装备牌或人物牌背面朝上，该角色可以选择一张手牌并令你观看之，你可以交给其一张装备牌，然后获得此牌并摸一张牌。",
	["ikyihuov"] = "易货",
	[":ikyihuov"] = "出牌阶段限一次，若蔻蔻的装备区有装备牌或人物牌背面朝上，你可以选择一张手牌并令蔻蔻观看之，蔻蔻可以交给你一张装备牌，然后蔻蔻获得之并摸一张牌。",
	["@ikyihuo-equip"] = "你可以交给 %src 一张装备牌，然后获得该牌并摸一张牌",
	["ikguixin"] = "归心",
	[":ikguixin"] = "每当你受到一次伤害后，若场上存活的角色数小于4或你的人物牌正面朝上，你可分别获得所有其他角色区域的一张牌，然后你将人物牌翻面。",
	
	["#bloom030"] = "寡言的观察者",
	["bloom030"] = "长门有希",--花 - 空 - 4血
	["designer:bloom030"] = "游卡桌游",
	["illustrator:bloom030"] = "poはるのいぶきkiki",
	["ikrenjia"] = "忍枷",
	[":ikrenjia"] = "锁定技，当你受到伤害后或于弃牌阶段弃置手牌时，获得等同于受到伤害或弃置手牌数量的“天枷”标记。",
	["@tianjia"] = "天枷",
	["iktiangai"] = "天盖",
	[":iktiangai"] ="觉醒技，准备阶段开始时，若你拥有4枚或更多的“天枷”标记，须减少1点体力上限并获得技能“极略”（弃置1枚“天枷”标记以发动下列一项技能：“预悉”、“慧泉”、“死噬”或“隙境”）。",
	["#IkTiangaiWake"] = "%from 的“天枷”为 %arg 个，触发“<font color=\"yellow\"><b>天盖</b></font>”觉醒",
	["ikjilve"] = "极略",
	[":ikjilve"] = "弃置1枚“天枷”标记以发动下列一项技能：“预悉”、“慧泉”、“死噬”或“隙境”。",

--snow
	["#snow009"] = "地狱之蝶",
	["snow009"] = "阎魔爱",--雪 - 空 - 4血
	["designer:snow009"] = "游卡桌游",
	["illustrator:snow009"] = "有河サトル",
	["ikliangban"] = "良坂",
	[":ikliangban"] = "准备阶段开始时，你可以选择一项：令一名其他角色摸X张牌，然后弃置一张牌；或令一名其他角色摸一张牌，然后弃置X张牌（X为你已损失的体力值且至少为1）。",
	["ikliangban-invoke"] = "你可以发动“良坂”<br/> <b>操作提示</b>: 选择一名其他角色→点击确定<br/>",
	["ikliangban:d1tx"] = "摸一张牌，然后弃置X张牌",
	["ikliangban:dxt1"] = "摸X张牌，然后弃置一张牌",

	["#snow010"] = "自慢的空圣",
	["snow010"] = "姬木千冬",--雪 - 空 - 3血
	["designer:snow010"] = "游卡桌游",
	["illustrator:snow010"] = "Dhiea",
	["ikzhuoshi"] = "卓始",
	[":ikzhuoshi"] = "摸牌阶段摸牌时，你可额外摸两张牌，若此时你的手牌多于五张，将一半（向下取整）的手牌交给场上手牌数最少（或之一）的一名其他角色。",
	["#ikzhuoshi-give"] = "卓始（后续结算）",
	["@ikzhuoshi"] = "请选择“卓始”的目标，将一半手牌（向下取整）交给该角色",
	["~ikzhuoshi"] = "选择需要给出的手牌→选择一名其他角色→点击确定",
	["ikyijing"] = "易境",
	[":ikyijing"] = "出牌阶段限一次，你可以选择两名手牌数差不大于三的其他角色，并弃置等同于这两名角色手牌数差的牌，然后交换她们的手牌。",
	["#IkYijing"] = "%from (原来 %arg 手牌) 与 %to (原来 %arg2 手牌) 交换了手牌",

	["#snow011"] = "司穰的贤狼",
	["snow011"] = "赫萝",--雪 - 幻 - 3血
    ["designer:snow011"] = "游卡桌游",
	["illustrator:snow011"] = "RYO",
    ["ikzhihui"] = "知惠",
	[":ikzhihui"] = "每当你受到伤害时，你可弃置一张红桃手牌并将此伤害转移给一名其他角色，若如此做，该角色在伤害结算后摸X张牌（X为该角色已损失的体力值）。",
    ["#ikzhihui"] = "知惠（摸牌）",
	["@ikzhihui-card"] = "请选择“知惠”的目标",
	["~ikzhihui"] = "选择一张<font color=\"red\">♥</font>手牌→选择一名其他角色→点击确定",
	["ikchiqiu"] = "赤秋",
	[":ikchiqiu"] = "锁定技，你的黑桃牌均视为红桃牌。",

	["#snow012"] = "天穹的女皇",
	["snow012"] = "伊卡洛斯",--雪 - 空 - 4血
	["designer:snow012"] = "游卡桌游",
	["illustrator:snow012"] = "和泉つばす",
	["ikjianlve"] = "歼略",
	[":ikjianlve"] = "出牌阶段限一次，你可以与一名其他角色拼点。若你赢，你获得技能“空殒”直到回合结束（你于出牌阶段可以额外使用一张【杀】；当你使用【杀】时可以额外指定一个目标，且无距离限制。）；若你没赢，你不能使用【杀】直到回合结束。",
	["ikkongyun"] = "空殒",
	[":ikkongyun"] = "你于出牌阶段可以额外使用一张【杀】；当你使用【杀】时可以额外指定一个目标，且无距离限制。",
	
	["#snow013"] = "紫阳之吻",
	["snow013"] = "散华礼弥",--雪 - 空 - 4血
	["designer:snow013"] = "游卡桌游",
	["illustrator:snow013"] = "秋の回忆亚",
	["iksusheng"] = "苏生",
	[":iksusheng"] = "锁定技，每当你处于濒死状态时，你将牌堆顶的一张牌置于人物牌上，称为“芳”，若该牌与你人物牌上的其他牌点数均不同，你将体力回复至1点，否则你将此牌置入弃牌堆。若你的人物牌上有牌，你的手牌上限等同于你的“芳”的数量。",
	["iksushengpile"] = "芳",
	["ikhuapan"] = "花磐",
	[":ikhuapan"] = "每当一名角色因另一名角色的弃置或获得而失去手牌后，你可以失去1点体力，令该失去手牌的角色摸两张牌。",

	["#snow014"] = "远方的苇莺",
	["snow014"] = "远野秋叶",--雪 - 空 - 4血
	["designer:snow014"] = "游卡桌游",
	["illustrator:snow014"] = "ここのび",
	["ikheyi"] = "赫訳",
	[":ikheyi"] = "每当你使用（指定目标后）或被使用（成为目标后）一张【碎月绮斗】或红色的【杀】时，你可以摸一张牌。",
	["ikchizhu"] = "赤主",
	[":ikchizhu"] = "觉醒技，准备阶段开始时，若你的体力值为1，你须减少1点体力上限，并回复1点体力或摸两张牌，然后获得技能“沉红”和“良坂”。",
	["#IkChizhuWake"] = "%from 的体力值为 %arg2，触发“%arg”觉醒",
	["ikchizhu:recover"] = "回复1点体力",
	["ikchizhu:draw"] = "摸两张牌",
	["ikbiansheng"] = "遍生",
	[":ikbiansheng"] = "君主技，其他雪势力角色的出牌阶段限一次，可以与你拼点，若该角色没赢，你可以获得双方拼点的牌；“赤主”发动后，你可以拒绝此拼点。",
	["ikbiansheng:pindian"] = "你可以获得双方的拼点牌",
	["ikbiansheng_pindian"] = "遍生",
	[":ikbiansheng_pindian"] = "出牌阶段限一次，你可以与君主拼点，若你没赢，君主可获得双方拼点的牌；“赤主”发动后，君主可以拒绝此拼点。",
	["ikbiansheng_pindian:accept"] = "接受",
	["ikbiansheng_pindian:reject"] = "拒绝",
	["#IkBianshengReject"] = "%from 拒绝 %to 发动“%arg”",

	["#snow015"] = "多舛的双子",
	["snow015"] = "琥珀＆翡翠",--雪 - 空 - 3血
	["designer:snow015"] = "游卡桌游",
	["illustrator:snow015"] = "森井しづき",
	["ikjiban"] = "羁绊",
	[":ikjiban"] = "出牌阶段，你可以将手牌中的一张装备牌置于一名其他角色的装备区里，然后摸一张牌。",
	["$IkJibanEquip"] = "%from 被装备了 %card",
	["ikjizhou"] = "箕箒",
	[":ikjizhou"] = "其他角色的弃牌阶段结束时，你可以将弃牌堆里该角色于此阶段因弃置而失去的一张手牌交给该角色，若如此做，你可以获得弃牌堆里其余于此阶段内弃置的牌。",

	["#snow029"] = "降乩引魂的市子",
	["snow029"] = "恐山安娜",--雪 - 空 - 3血
	["designer:snow029"] = "游卡桌游",
	["illustrator:snow029"] = "映日紅",
	["iklvejue"] = "略决",
	[":iklvejue"] = "摸牌阶段开始时，你可以放弃摸牌，改为从牌堆顶亮出五张牌，你获得不同花色的牌各一张，将其余的牌置入弃牌堆。",
	["iklingshi"] = "灵视",
	[":iklingshi"] = "出牌阶段限一次，你可以观看一名其他角色的手牌，并可以展示其中一张红桃牌，然后将其弃置或置于牌堆顶。",
	["iklingshi:discard"] = "弃置",
	["iklingshi:put"] = "置于牌堆顶",

	["#snow030"] = "圣炎的龙姬",
	["snow030"] = "萝丝",--雪 - 空 - 4血
	["designer:snow030"] = "游卡桌游",
	["illustrator:snow030"] = "亜方逸樹",
	["iklongxi"] = "龙息",
	[":iklongxi"] = "弃牌阶段结束时，若你于此阶段弃置了两张或更多的手牌，你可以令所有角色各回复1点体力或各失去1点体力。",
	["iklongxi:up"] = "所有角色各回复1点体力",
	["iklongxi:down"] = "所有角色各失去1点体力",
	["ikyeyan"] = "业焰",
	[":ikyeyan"] = "限定技，出牌阶段，你可以指定一至三名角色，然后分别对她们造成至多共3点火焰伤害，若你以此法对一名角色造成了2点或更多的火焰伤害，你须先弃置四张不同花色的手牌并失去3点体力。<br />◇多次点击以分配多点伤害",
	["greatikyeyan"] = "业焰",
	["smallikyeyan"] = "业焰",
	
--luna
	["#luna001"] = "孢子花的挽歌",
	["luna001"] = "沙耶",--月 - 空 - 8血
	["designer:luna001"] = "游卡桌游",
	["illustrator:luna001"] = "星屑七号",
	["ikfusheng"] = "腐生",
	[":ikfusheng"] = "你可以将一张黑桃手牌当【酒】使用。",
	["ikhuanbei"] = "幻呗",
	[":ikhuanbei"] = "锁定技，你对幻属性角色、幻属性角色对你使用【杀】时，需连续使用两张【闪】才能抵消。",
	["ikbenghuai"] = "崩坏",
	[":ikbenghuai"] ="锁定技，结束阶段开始时，若你的体力值不是全场最少的（或之一），你须失去1点体力或减少1点体力上限。",
	["ikbenghuai:hp"] = "失去1点体力",
	["ikbenghuai:maxhp"] = "减少1点体力上限",
	["ikwuhua"] = "舞华",
	[":ikwuhua"] = "君主技，每当其他月势力角色造成一次伤害后，可以进行一次判定，若为黑桃，你回复1点体力。",

	["#luna004"] = "辉煌的烈阳",
	["luna004"] = "高町奈叶",--月 - 空 - 4血
	["designer:luna004"] = "游卡桌游",
	["illustrator:luna004"] = "八城惺架",
	["ikxinghuang"] = "星煌",
	[":ikxinghuang"] = "出牌阶段，你可以将两张相同花色的手牌当【魔闪花火】使用。",
	["ikxuzhao"] = "欻照",
	[":ikxuzhao"] = "出牌阶段限一次，你可以失去1点体力并将一张手牌当【罔两空界】使用。",

	["#luna005"] = "印章的白与黑",
	["luna005"] = "玛戈特•奈特＆玛伽•成濑",--月 - 空 - 4血
	["&luna005"] = "玛戈特＆玛伽",
	["designer:luna005"] = "游卡桌游",
	["illustrator:luna005"] = "白鷺六羽",
	["ikjingfa"] = "境法",
	[":ikjingfa"] = "出牌阶段结束时，若你于此阶段没有造成伤害，你可以摸一张牌或弃置场上的一张牌。",
	["@ikjingfa"] = "你可以弃置场上的一张牌，或点“取消”摸一张牌",
	["ikqiyu"] = "契羽",
	[":ikqiyu"] = "摸牌阶段开始时，你可以放弃摸牌，改为进行一次判定，你获得此判定牌，且此回合你的每张与该判定牌不同颜色的手牌均可以当【碎月绮斗】使用。",
	["#ikqiyu"] = "契羽（获得判定牌）",

	["#luna007"] = "凄美的黑蔷薇",
	["luna007"] = "水银灯",--月 - 空 - 3血
	["designer:luna007"] = "游卡桌游",
	["illustrator:luna007"] = "NAbyssor",
	["iksishideng"] = "死噬",
	[":iksishideng"] = "锁定技，在你的回合，除你以外，只有处于濒死状态的角色才能使用【桃】。",
	["#IkSishidengOne"] = "%from 的“%arg”被触发，只能 %from 自救",
	["#IkSishidengTwo"] = "%from 的“%arg”被触发，只有 %from 和 %to 才能救 %to",
	["ikwenle"] = "文乐",
	[":ikwenle"] = "限定技，出牌阶段，可令所有其他角色各选择一项：对其与之距离最近的另一名角色使用一张【杀】，或失去1点体力。",
	["@ikwenle-slash"] = "请使用一张【杀】响应“文乐”",
	["ikmoyu"] = "墨羽",
	[":ikmoyu"] = "锁定技，你不能成为黑色锦囊牌的目标。",

	["#luna008"] = "天翔的银狼",
	["luna008"] = "尤丽叶•希格图娜",--月 - 空 - 4血
	["&luna008"] = "尤丽叶",
	--thjibu
	["designer:luna008"] = "游卡桌游",
	["illustrator:luna008"] = "浅葉ゆう",
	["ikkongsa"] = "空飒",
	[":ikkongsa"] = "当你使用的【杀】被目标角色的【闪】抵消时，你可以弃置其一张牌；当你使用红色【杀】对目标角色造成一次伤害后，你可以摸一张牌。",

	["#luna009"] = "魔法使之契",
	["luna009"] = "木之本樱",--月 - 空 - 3血
	["designer:luna009"] = "游卡桌游",
	["illustrator:luna009"] = "pun2",
	["ikhuanshen"] = "幻身",
	[":ikhuanshen"] = "所有人都展示人物牌后，你随机获得两张未加入游戏的人物牌，称为“幻身牌”，将其中一张置于你的人物牌上并声明该人物的一项技能，你获得该技能且将特殊属性和势力属性变成与该人物相同直到失去人物牌上的牌。回合开始时和结束后，你可以获得你人物牌上的牌，然后将一张“幻身牌”置于你的人物牌上并声明该人物的一项技能（你不可声明限定技、觉醒技、君主技或专属技）。",
	["#ikhuanshen-start"] = "幻身（游戏开始）",
	["#GetIkHuanshen"] = "%from 获得了 %arg 张“幻身牌”，现在共有 %arg2 张“幻身牌”",
	["#GetIkHuanshenDetail"] = "%from 获得了“幻身牌” %arg",
	["iklingqi"] = "灵契",
	[":iklingqi"] = "专属技，每当你受到1点伤害后，你可以获得一张“幻身牌”。",

	["#luna011"] = "诡变的欺诈师",
	["luna011"] = "塞蕾丝提雅•罗登贝克",--月 - 空 - 4血
	["&luna011"] = "塞蕾丝提雅",
	["designer:luna011"] = "游卡桌游",
	["illustrator:luna011"] = "ちまり",
	["ikguihuo"] = "诡惑",
	[":ikguihuo"] = "你可以选择任何一种基本牌或非延时类锦囊牌，并正面朝下使用或打出一张手牌。若无人质疑，则该牌按你所述之牌结算。若有人质疑则亮出验明：若为真，质疑者各失去1点体力；若为假，质疑者各摸1张牌。除非被质疑的牌为红桃且为真时，该牌仍然可进行结算，否则无论真假，将该牌置入弃牌堆。",
	["question"] = "质疑",
	["noquestion"] = "不质疑",
	["ikguihuo_saveself"] = "“诡惑”【桃】或【酒】",
	["ikguihuo_slash"] = "“诡惑”【杀】",
	["normal_slash"] = "普通杀",
	["#IkGuihuo"] = "%from 发动了“%arg2”，声明此牌为 【%arg】，指定的目标为 %to",
	["#IkGuihuoNoTarget"] = "%from 发动了“%arg2”，声明此牌为 【%arg】",
	["#IkGuihuoCannotQuestion"] = "%from 当前体力值为 %arg，无法质疑",
	["#IkGuihuoQuery"] = "%from 表示 %arg",
	["$IkGuihuoResult"] = "%from 的“<font color=\"yellow\"><b>诡惑</b></font>”牌是 %card",

	["#luna012"] = "宝石魔术的血脉",
	["luna012"] = "远坂凛",--月 - 幻 - 3血
	["designer:luna012"] = "游卡桌游",
	["illustrator:luna012"] = "しらび",
	["ikhuiyao"] = "辉耀",
	[":ikhuiyao"] = "每当一名角色受到【杀】造成的一次伤害后，你可以弃置一张牌，并令其进行一次判定：若结果为红桃，该角色回复1点体力；若结果为方块，该角色摸两张牌；若结果为梅花，伤害来源弃置两张牌；若结果为黑桃，伤害来源将其人物牌翻面。",
	["@ikhuiyao"] = "你可以弃置一张牌发动“辉耀”",
	["ikqihuang"] = "淒煌",
	[":ikqihuang"] = "锁定技，当你死亡时，你令杀死你的角色失去所有的人物技能。",
	["#IkQihuangLoseSkills"] = "%from 的“%arg”被触发， %to 失去所有人物技能",
	["@qihuang"] = "淒煌",

	["#luna014"] = "超电磁炮",
	["luna014"] = "御坂美琴",--月 - 空 - 3血
	["designer:luna014"] = "游卡桌游",
	["illustrator:luna014"] = "三嶋くろね",
	["ikleiji"] = "雷击",
	[":ikleiji"] = "每当你使用或打出一张【闪】时，可以令一名角色进行一次判定，若结果为黑桃，你对该角色造成2点雷电伤害；若结果为梅花，你对该角色造成1点雷电伤害，然后你回复1点体力。",
	["ikleiji-invoke"] = "你可以发动“雷击”<br/> <b>操作提示</b>: 选择一名角色→点击确定<br/>",
	["iktianshi"] = "天势",
	[":iktianshi"] = "在一名角色的判定牌生效前，你可以用一张黑色牌替换之。",
	["@iktianshi-card"] = CommonTranslationTable["@askforretrial"],
	["ikyuji"] = "御姬",
	[":ikyuji"] = "君主技，其他月势力角色的出牌阶段限一次，该角色可以交给你一张【闪】或【净琉璃镜】。",
	["ikyujiv"] = "御姬",
	[":ikyujiv"] = "出牌阶段限一次，你可以交给君主一张【闪】或【净琉璃镜】。",

	["#luna029"] = "无双的剑姬",
	["luna029"] = "阿斯特莱雅",--月 - 空 - 5血
	["designer:luna029"] = "游卡桌游",
	["illustrator:luna029"] = "渡邊義弘",
	["@mailun"] = "脉轮",
	["ikzhuohuo"] = "拙火",
	[":ikzhuohuo"] = "锁定技，游戏开始时，你获得2枚“脉轮”标记，每当你造成或受到1点伤害后，获得1枚“脉轮”标记。",
	["#@mailun-2"] = "拙火",
	["#IkZhuohuoDamage"] = "%from 的“%arg2”被触发，造成 %arg 点伤害获得 %arg 枚“脉轮”标记",
	["#IkZhuohuoDamaged"] = "%from 的“%arg2”被触发，受到 %arg 点伤害获得 %arg 枚“脉轮”标记",
	["ikwumou"] = "无谋",
	[":ikwumou"] = "锁定技，当你使用一张非延时类锦囊牌时，你须弃置1枚“脉轮”标记或失去1点体力。",
	["ikwumou:discard"] = "弃置1枚“脉轮”标记",
	["ikwumou:losehp"] = "失去1点体力",
	["iksuikong"] = "碎空",
	[":iksuikong"] = "出牌阶段，你可以弃置2枚“脉轮”标记并选择一名其他角色，该角色的防具无效且你获得技能“无双”，直到回合结束。",
	["iktianwu"] = "天舞",
	[":iktianwu"] = "出牌阶段限一次，你可以弃置6枚“脉轮”标记，然后对所有其他角色各造成1点伤害，所有其他角色弃置各自装备区的牌，再弃置四张手牌，然后将你的人物牌翻面。",
}