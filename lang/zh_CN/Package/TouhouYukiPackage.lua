-- translation for TouhouYukiPackage

return {
	["touhou-yuki"] = "东方的雪园",

	["#yuki001"] = "乐园的巫女",
	["yuki001"] = "博丽灵梦",--雪 - 空 - 4血
	["&yuki001"] = "灵梦",
	["designer:yuki001"] = "幻桜落",
	["illustrator:yuki001"] = "えふぇ",
	["cv:yuki001"] = "暂无",
	["thjianmo"] = "缄魔",
	[":thjianmo"] = "其他角色的出牌阶段开始时，若其手牌数不小于体力上限，你可令其选择一项：摸一张牌，且此阶段不能使用或打出【杀】；或此阶段使用【杀】时需弃置一张牌，否则此【杀】无效。",
	["#thjianmo"] = "缄魔（弃牌）",
	["thjianmo:jian"] = "摸一张牌，且此阶段不能使用或打出【杀】",
	["thjianmo:mo"] = "此阶段使用【杀】时需弃置一张牌，否则此【杀】无效。",
	["#thjianmochoose1"] = "%from 选择了第 %arg 项，此阶段无法使用或打出【<font color=yellow><b>杀</b></font>】",
	["#thjianmochoose2"] = "%from 选择了第 %arg 项，此阶段使用【<font color=yellow><b>杀</b></font>】时须弃一张牌",
	["@imprison"] = "缚",
	["#ThJianmo"] = "受“%arg”影响，%from 需弃置一张牌，否则此【%arg2】无效",
	["@thjianmo"] = "请弃置一张牌使该【杀】生效",
	["therchong"] = "二重",
	[":therchong"] = "觉醒技。若你回合外的两个连续的回合内，当前回合角色均未使用【杀】，且第二个回合的回合结束时，若你已受伤，你须减少1点体力上限，并获得技能“幻法”和“祝祭”。",
	["thhuanfa"] = "幻法",
	[":thhuanfa"] = "阶段技。你可以将一张红桃手牌交给一名其他角色，然后你获得该角色的一张牌并将该牌交给除该角色外的一名角色。",
	["@thhuanfa-give"] = "你可以将此牌交给除 %src 外的一名角色",
	["@layer"] = "重",
	["#ThErchong"] = "%from 已受伤，触发“%arg”觉醒",
	["thchundu"] = "春度",
	[":thchundu"] = "君主技。每当其他雪势力角色使用的红桃基本牌结算后置入弃牌堆时，你可弃置一张手牌获得之。",
	["@thchundu"] = "你可以弃置一张手牌发动“春度”",

	["#yuki002"] = "疏雨的百鬼夜行",
	["yuki002"] = "伊吹萃香",--雪 - 空 - 4血
	["&yuki002"] = "萃香",
	["designer:yuki002"] = "数据流突袭者",
	["illustrator:yuki002"] = "九十i",
	["cv:yuki002"] = "暂无",
	["thzuishang"] = "醉觞",
	[":thzuishang"] = "你的回合内，所有角色可以将两张牌当【酒】使用（你的回合内，所有角色使用的【酒】不计入使用限制）。",
	["thzuishangv"] = "醉觞",
	[":thzuishangv"] = "你可以将两张牌当【酒】使用。",
	["thxugu"] = "湑酤",
	[":thxugu"] = "出牌阶段，每当你使用一张【酒】，在结算后，你可以选择一名其他角色，该角色需使用一张【酒】；否则你“湑酤”无效，直到回合结束，且视为你对其使用一张无视距离且不计入使用限制的【杀】。",
	["@thxugu"] = "你可以发动“湑酤”",
	["@thxugu-use"] = "请使用一张【酒】",

	["#yuki003"] = "半分幻的庭师",
	["yuki003"] = "魂魄妖梦",--雪 - 空 - 4血
	["&yuki003"] = "妖梦",
	["designer:yuki003"] = "名和行年",
	["illustrator:yuki003"] = "橙狼",
	["cv:yuki003"] = "暂无",
	["thshenzhan"] = "榊斩",
	[":thshenzhan"] = "锁定技。当你于出牌阶段内使用非基本牌对其他角色造成伤害后，你视为于此阶段内没有使用过【杀】。",
	["thhunqie"] = "魂切",
	[":thhunqie"] = "出牌阶段结束时，若你于此阶段内未使用过【杀】，你可以将一张黑色牌当【杀】使用。",
	["@thhunqie"] = "你可以发动“魂切”",
	["~thhunqie"] = "选择一张黑色牌→选择目标→点击确定",
	["thdaojian"] = "道剑",
	[":thdaojian"] = "专属技。锁定技。若你的装备区里没有武器牌，你视为装备着【楼观剑】。",

	["#yuki004"] = "七色的人形使",
	["yuki004"] = "爱丽丝•玛嘉特洛伊德",--雪 - 空 - 4血
	["&yuki004"] = "爱丽丝",
	["designer:yuki004"] = "CoffeeNO加糖",
	["illustrator:yuki004"] = "spirtie",
	["cv:yuki004"] = "暂无",
	["thzhancao"] = "战操",
	[":thzhancao"] = "你的回合外，当你或你的攻击范围内的一名角色成为【杀】的目标时，你可选择一项使该【杀】对其无效：失去1点体力，且该【杀】在结算后置入弃牌堆时，你获得之；或弃置一张非基本牌。",
	["@thzhancao"] = "请弃置一张非基本牌，或点“取消”失去1点体力",

	["#yuki005"] = "幻想乡的记忆",
	["yuki005"] = "稗田阿求",--雪 - 幻 - 3血
	["&yuki005"] = "阿求",
	["designer:yuki005"] = "江山英魂",
	["illustrator:yuki005"] = "DomotoLain",
	["cv:yuki005"] = "暂无",
	["thmoji"] = "墨迹",
	[":thmoji"] = "当你需要使用或打出一张【杀】、【闪】或【酒】时，你可以将等同于你体力值数量的牌以任意顺序置于牌堆顶（至多两张），视为你使用或打出一张【杀】、【闪】或【酒】。<br/><font color=blue>★你选择的牌将以你选择的顺序的逆序置于牌堆顶</font>",
	["thyuanqi"] = "缘起",
	[":thyuanqi"] = "阶段技。你可以选择一种花色并亮出牌堆顶的一张牌，若花色相同，你可以摸两张牌；若颜色相同且花色不同，你可以将该牌交给一名角色。",
	["thyuanqi_draw:yes"] = "你可以摸两张牌",
	["@thyuanqi"] = "你可以将此牌交给一名角色，或者点“取消”将之置入弃牌堆",

	["#yuki006"] = "凶兆的黑猫",
	["yuki006"] = "橙",--雪 - 空 - 4血
	["designer:yuki006"] = "同声异谱",
	["illustrator:yuki006"] = "橙狼",
	["cv:yuki006"] = "暂无",
	["thdunjia"] = "遁甲",
	[":thdunjia"] = "当你成为一名其他角色使用牌的唯一目标后，若其装备区的牌比你少，你可以摸一张牌。",
	["@thdunjia"] = "你可以发动“遁甲”",
	["thqingming"] = "晴明",
	[":thqingming"] = "当你使用【杀】指定一名其他角色后，若其装备区的牌比你多，你可以令此【杀】对其无效并视对其使用一张【木隐妖岚】、【妙手探云】、【心网密葬】或【碎月绮斗】。",

	["#yuki007"] = "策士的九尾",
	["yuki007"] = "八云蓝",--雪 - 空 - 3血
	["&yuki007"] = "蓝",
	["designer:yuki007"] = "淬毒",
	["illustrator:yuki007"] = "橙狼",
	["cv:yuki007"] = "暂无",
	["thchouce"] = "筹策",
	[":thchouce"] = "你于出牌阶段使用的第一张牌，或点数比你此阶段使用的前一张牌大的牌，可以无视合法性指定一名角色为目标。<br />" ..
				  "★使用方法：点击技能按钮→选择要使用的牌→选择一名角色→点击确定",
	["@augury"] = "占",
	["@thchouce"] = "你选择发动“筹策”",
	["~thchouce"] = "选择要使用的牌→选择一名角色→点击确定",
	["$ThChouce"] = "%from 发动了“%arg”将 %to 指定为了 %card 的目标",
	["thzhanshi"] = "占筮",
	[":thzhanshi"] = "锁定技。出牌阶段结束时，若你于此阶段使用的牌不少于三张，且全部发动了“筹策”，你在此回合结束后进行一个额外的回合，且于额外回合的回合开始时获得技能“幻葬”直到回合结束（锁定技。出牌阶段结束时，你进行一次判定，若结果为黑色，你失去1点体力）。",
	["thhuanzang"] = "幻葬",
	[":thhuanzang"] = "锁定技。出牌阶段结束时，你进行一次判定，若结果为黑色，你失去1点体力。",

	["#yuki008"] = "法界之火",
	["yuki008"] = "圣白莲",--雪 - 空 - 3血
	["&yuki008"] = "白莲",
	["designer:yuki008"] = "幻桜落",
	["illustrator:yuki008"] = "えふぇ",
	["cv:yuki008"] = "暂无",
	["thziyun"] = "紫云",
	[":thziyun"] = "锁定技。你不是【玄海仙鸣】或【枯羽华庭】的合法目标。",
	["thchuiji"] = "垂迹",
	[":thchuiji"] = "弃牌阶段结束时，若你于此阶段弃置了两张或更多的手牌，或每当你于回合外失去牌时，你可进行一次判定，若为红色，令一名角色回复1点体力；若为黑色，弃置一名其他角色的一张牌。",

	["#yuki009"] = "幻想的境界",
	["yuki009"] = "八云紫",--雪 - 空 - 4血
	["&yuki009"] = "紫",
	["designer:yuki009"] = "昂翼天使",
	["illustrator:yuki009"] = "DomotoLain",
	["cv:yuki009"] = "暂无",
	["thlingya"] = "灵压",
	[":thlingya"] = "每当其他角色于你的回合使用红色牌，在结算后你可以令其选择一项：令你摸一张牌；或令你弃置其一张牌。",
	["thlingya:letdraw"] = "令其摸一张牌",
	["thlingya:discard"] = "令其弃置你一张牌",
	["thheimu"] = "黑幕",
	[":thheimu"] = "当你于出牌阶段使用一张牌指定目标时，你可以令一名其他角色成为此牌的使用者，每阶段限一次。",
	["@thheimu"] = "你可以发动“黑幕”",

	["#yuki010"] = "湖上的冰精",
	["yuki010"] = "琪露诺",--雪 - 空 - 4血
	["designer:yuki010"] = "昂翼天使",
	["illustrator:yuki010"] = "エクレア",
	["cv:yuki010"] = "暂无",
	["thhanpo"] = "寒魄",
	[":thhanpo"] = "锁定技。防止你对其他角色造成的火焰伤害，防止你受到的火焰伤害。",
	["thjidong"] = "急冻",
	[":thjidong"] = "出牌阶段开始时，你可以令一名其他角色摸等同于场上方块牌数的牌，然后弃置等量的牌。若如此做，此阶段结束时，若其于此阶段内失去过至少两张牌，其将人物牌翻面，然后摸一张牌。",
	["@thjidong"] = "你可以发动“急冻”",
	["#thjidong"] = "急冻（翻面）",
	["thbingpu"] = "氷瀑",
	[":thbingpu"] = "限定技。出牌阶段，你可以令所有其他有牌的角色做出选择：打出一张【闪】，或令你依次弃置其两张牌。",
	["@thbingpu"] = "%src 使用了“氷瀑”，请打出一张【闪】或点“取消”令其弃置你的两张牌",

	["#yuki011"] = "冬的忘却之物",
	["yuki011"] = "蕾蒂•怀特罗克",--雪 - 空 - 3血
	["&yuki011"] = "蕾蒂",
	["designer:yuki011"] = "幻桜落",
	["illustrator:yuki011"] = "粟",
	["cv:yuki011"] = "暂无",
	["thdongmo"] = "冬末",
	[":thdongmo"] = "结束阶段开始时，你可以指定一至X名其他角色（X为你已损失的体力值），你和这些角色将各自的人物牌翻面，并各摸一张牌。",
	["@thdongmo"] = "你可以发动“冬末”",
	["~thdongmo"] = "选择X名角色→点击确定",
	["thlinhan"] = "凛寒",
	[":thlinhan"] = "当你使用或者打出一张【闪】时，你可以摸一张牌。",

	["#yuki012"] = "平凡陈腐的山彦",
	["yuki012"] = "幽谷响子",--雪 - 空 - 4血
	["&yuki012"] = "响子",
	["designer:yuki012"] = "永恒的须臾",
	["illustrator:yuki012"] = "CUBE",
	["cv:yuki012"] = "暂无",
	["thfusheng"] = "复声",
	[":thfusheng"] = "你可以跳过摸牌阶段，然后依次令其他角色选择令你摸一张牌或令你弃一张牌。若如此做，弃牌阶段开始时，若你的手牌数大于体力值，你依次交给令你摸一张牌的角色一张牌；若你的手牌数小于体力值，令你弃一张牌的角色依次交给你一张牌。",
	["thfusheng:draw"] = "摸一张牌",
	["thfusheng:discard"] = "弃置一张牌",
	["#ThFusheng"] = "%from 选择了 %arg",
	["#thfusheng"] = "复声（给牌）",
	["@thfusheng-give"] = "请将一张牌交给 %src",

	["#yuki013"] = "骚灵乐团",
	["yuki013"] = "普莉茲姆利巴三姐妹",--雪 - 空 - 3血
	["&yuki013"] = "三姐妹",
	["designer:yuki013"] = "绝顶油条",
	["illustrator:yuki013"] = "ke-ta",
	["cv:yuki013"] = "暂无",
	["thsaozang"] = "骚葬",
	[":thsaozang"] = "弃牌阶段结束时，你每于此阶段弃置了一种类别的牌，你可以依次弃置一名其他角色的一张手牌。",
	["@thsaozang"] = "你可以发动“骚葬”",
	["thxuqu"] = "絮曲",
	[":thxuqu"] = "你的回合外，当你失去手牌时，你可以令一名其他角色摸一张牌。",
	["@thxuqu"] = "你可以发动“絮曲”",

	["#yuki014"] = "小小贤将",
	["yuki014"] = "娜兹玲",--雪 - 空 - 4血
	["designer:yuki014"] = "xyzbilliu",
	["illustrator:yuki014"] = "DomotoLain",
	["cv:yuki014"] = "暂无",
	["thqiebao"] = "窃宝",
	[":thqiebao"] = "一名角色对另一名角色使用【桃】时，你可以弃置一张【杀】并令此【桃】对目标角色无效，若此【杀】为红色，你获得此【桃】；一名角色获得另一名角色的牌时，你可弃置一张【杀】，然后获得这些牌。",
	["@thqiebao"] = "你可以弃置一张【杀】发动“窃宝”",
	["@thqiebaomove"] = "%dest 将要得到 %src 的 %arg 张牌，你可以弃置一张【杀】发动“窃宝”",

	["#yuki015"] = "虎纹的毘沙门天",
	["yuki015"] = "寅丸星",--雪 - 空 - 4血
	["&yuki015"] = "星",
	["designer:yuki015"] = "幻桜落",
	["illustrator:yuki015"] = "山口真之介",
	["cv:yuki015"] = "暂无",
	["thlingta"] = "灵塔",
	[":thlingta"] = "你可以选择一至两项：<br />1.跳过你此回合的摸牌阶段；<br />2.跳过你此回合的出牌阶段。<br />你每选择一项，获得1枚“炜”标记。",
	["thlingta:3"] = "你可以发动“灵塔”跳过摸牌阶段",
	["thlingta:4"] = "你可以发动“灵塔”跳过出牌阶段",
	["thweiguang"] = "威光",
	[":thweiguang"] = "你可以选择一至四项：<br />1.跳过你此回合的判定阶段；<br />2.跳过你此回合的弃牌阶段；<br />3.在你此回合的摸牌阶段后额外进行一个摸牌阶段；<br />4.在你此回合的出牌阶段后额外进行一个出牌阶段。<br />你每选择一项，须先弃置1枚“炜”标记，每项每回合限一次。",
	["thweiguang:2"] = "你可以发动“威光”跳过判定阶段",
	["thweiguang:5"] = "你可以发动“威光”跳过弃牌阶段",
	["#thweiguang-skip"] = "威光（跳过阶段）",
	["thweiguang:3"] = "你可以发动“威光”进行一个额外的摸牌阶段",
	["thweiguang:4"] = "你可以发动“威光”进行一个额外的出牌阶段",
	["thchuhui"] = "初辉",
	[":thchuhui"] = "锁定技。游戏开始时，你获得1枚“炜”标记。",
	["@bright"] = "炜",
	
	["#yuki016"] = "宝船之御守",
	["yuki016"] = "云居一轮",--雪 - 空 - 4血
	["&yuki016"] = "一轮",
	["designer:yuki016"] = "香蒲神殇",
	["illustrator:yuki016"] = "閏月戈",
	["cv:yuki016"] = "暂无",
	["thkujie"] = "苦戒",
	[":thkujie"] = "其他角色的出牌阶段限一次，该角色可以弃置一张红色基本牌，然后令你失去1点体力，若如此做，此回合结束后，你回复2点体力。",
	["thkujiev"] = "苦戒",
	[":thkujiev"] = "阶段技。你可以弃置一张红色基本牌，然后令云居一轮失去1点体力，若如此做，此回合结束后，云居一轮回复2点体力。",
	["#thkujie-recover"] = "苦戒（回复体力）",
	["thyinbi"] = "廕庇",
	[":thyinbi"] = "专属技。当一名体力值不大于你的其他角色因伤害而扣减体力后，你可以令其回复等量的体力，然后伤害来源对你造成等量的同属性伤害。",

	["#yuki017"] = "惨憺的大海原",
	["yuki017"] = "村纱水蜜",--雪 - 空 - 4血
	["&yuki017"] = "水蜜",
	["designer:yuki017"] = "幻桜落",
	["illustrator:yuki017"] = "ひそな",
	["cv:yuki017"] = "暂无",
	["thmingling"] = "溟灵",
	[":thmingling"] = "锁定技。防止你受到的火焰伤害，你每次受到的雷电伤害+1。",
	["thchuanshang"] = "船殇",
	[":thchuanshang"] = "阶段技。你可以令你攻击范围内的一名角色获得1枚“溺”标记，若其已拥有该标记，你失去1点体力。其他角色每拥有1枚该标记，其手牌上限便-1。其他角色的结束阶段开始时，若其拥有1枚或更多的“溺”标记，须进行一次判定，若为红桃，弃置2枚该标记；若为黑色，获得1枚该标记。",
	["@drowning"] = "溺",
	["#ThChuanshang"] = "%from 受到了“%arg”的影响",

	["#yuki018"] = "天衣无缝的亡灵",
	["yuki018"] = "西行寺幽幽子",--雪 - 幻 - 3血
	["&yuki018"] = "幽幽子",
	["designer:yuki018"] = "幻桜落",
	["illustrator:yuki018"] = "6U",
	["cv:yuki018"] = "暂无",
	["thlingdie"] = "灵蝶",
	[":thlingdie"] = "出牌阶段限两次，你可将一张手牌交给一名其他角色，然后令该角色观看由其选择的另一名角色的手牌。若你以此法交给的手牌为红桃，该角色摸一张牌；若不为红桃，你此阶段不可以再发动“灵蝶”",
	["@thlingdie"] = "请选择观看手牌的角色",
	["thwushou"] = "无寿",
	[":thwushou"] = "每当你受到一次伤害后，你可以将你的手牌补至四张。",
	["thfuyue"] = "浮月",
	[":thfuyue"] = "君主技。其他雪势力角色的出牌阶段限一次，若你的体力值为1，可以与你拼点，若该角色没有赢，你回复1点体力。你可以拒绝此拼点。",
	["thfuyuev"] = "浮月",
	[":thfuyuev"] = "阶段技。若你为雪势力且君主的体力值为1，你可以与君主拼点，若你没赢，君主回复1点体力。君主可以拒绝此拼点。",
	["thfuyue:accept"] = "同意此次拼点",
	["thfuyue:reject"] = "拒绝此次拼点",
}