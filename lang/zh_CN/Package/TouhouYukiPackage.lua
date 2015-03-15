-- translation for TouhouYukiPackage

return {
	["touhou-yuki"] = "东方的雪园",

	["#yuki001"]="乐园的巫女",
	["yuki001"]="博丽灵梦",--雪 - 空 - 4血
	["designer:yuki001"]="幻桜落",
	["illustrator:yuki001"]="えふぇ",
	["cv:yuki001"]="暂无",
	["thjianmo"]="缄魔",
	[":thjianmo"]="其他角色的出牌阶段开始时，若其手牌数不小于体力上限，你可令其选择一项：摸一张牌，且此阶段不能使用或打出【杀】；或此阶段使用【杀】时需弃置一张牌，否则此【杀】无效。",
	["#thjianmo"]="缄魔（弃牌）",
	["thjianmo:jian"]="摸一张牌，且此阶段不能使用或打出【杀】",
	["thjianmo:mo"]="此阶段使用【杀】时需弃置一张牌，否则此【杀】无效。",
	["#thjianmochoose1"]="%from 选择了第 %arg 项，此阶段无法使用或打出【<font color=yellow><b>杀</b></font>】",
	["#thjianmochoose2"]="%from 选择了第 %arg 项，此阶段使用【<font color=yellow><b>杀</b></font>】时须弃一张牌",
	["#ThJianmo"]="受“%arg”影响，%from 需弃置一张牌，否则此【%arg2】无效",
	["@thjianmo"]="请弃置一张牌使该【杀】生效",
	["therchong"]="二重",
	[":therchong"]="觉醒技，准备阶段开始时，若你的体力值不大于2，你须弃置两张牌，并减少1点体力上限，然后获得技能“幻法”和“祝祭”。",
	["#ThErchong"]="%from 的体力值为 %arg2 ，触发“%arg”觉醒",
	["thchundu"]="春度",
	[":thchundu"]="君主技，每当其他雪势力角色使用一张红桃基本牌时，可以令你观看牌堆顶的一张牌，然后你须将此牌交给一名其他角色或置入弃牌堆。",

	["#yuki002"]="疏雨的百鬼夜行",
	["yuki002"]="伊吹萃香",--雪 - 空 - 3血
	["designer:yuki002"]="幻桜落",
	["illustrator:yuki002"]="九十i",
	["cv:yuki002"]="暂无",
	["thcuimeng"]="萃梦",
	[":thcuimeng"]="准备阶段开始时，你可进行一次判定，若结果为红色，你获得此牌，你可重复此流程，直到出现黑色的判定结果为止。",
	["#thcuimeng-move"]="萃梦（获得判定牌）",
	["thzuishang"]="醉觞",
	[":thzuishang"]="锁定技，你的红桃基本牌均视为【酒】。当你使用一张【酒】时，你摸一张牌。",
	["#thzuimeng"]="醉梦",
	["thmengwu"]="濛雾",
	[":thmengwu"]="锁定技，你的手牌上限+1。",

	["#yuki003"]="半分幻的庭师",
	["yuki003"]="魂魄妖梦",--雪 - 空 - 4血
	["designer:yuki003"]="Why.",
	["illustrator:yuki003"]="橙狼",
	["cv:yuki003"]="向晚",
	["~yuki003"]="幽幽子大人...！",
	["thcihang"]="慈航",
	[":thcihang"]="当你使用的【杀】被目标角色的【闪】抵消时，你可以选择一项，以令此【杀】依然造成伤害：弃置等同于目标角色已损失的体力值数量的牌（不足则全弃）；或令目标角色摸等同于其体力值数量的牌（至多摸五张）。",
	["thcihang:discard"]="弃置等同于目标角色已损失的体力值数量的牌",
	["thcihang:draw"]="令目标角色摸等同于其体力值数量的牌",
	["$thcihang1"]="断迷剑，迷津慈航斩！",
	["$thcihang2"]="皆斩！",

	["#yuki004"]="七色的人形使",
	["yuki004"]="爱丽丝•玛嘉特洛伊德",--雪 - 空 - 4血
	["&yuki004"]="爱丽丝",
	["designer:yuki004"]="CoffeeNO加糖",
	["illustrator:yuki004"]="spirtie",
	["cv:yuki004"]="暂无",
	["thzhancao"]="战操",
	[":thzhancao"]="你的回合外，当你或你的攻击范围内的一名角色成为【杀】的目标时，你可选择一项使该【杀】对其无效：失去1点体力，且该【杀】在结算后置入弃牌堆时，你获得之；或弃置你装备区内的一张装备牌。",
	["@thzhancao"]="请弃置一张装备区内的装备牌，或点“取消”失去1点体力",

	["#yuki005"]="幻想乡的记忆",
	["yuki005"]="稗田阿求",--雪 - 幻 - 3血
	["designer:yuki005"]="幻桜落",
	["illustrator:yuki005"]="DomotoLain",
	["cv:yuki005"]="暂无",
	["thyuanqi"]="缘起",
	[":thyuanqi"]="出牌阶段限一次，你可以展示一张手牌，然后令一名其他角色选择一项：<br />1.摸一张牌，然后失去1点体力；<br />2.弃置这张牌，并令你获得其一张牌。",
	["thyuanqi:draw"]="摸一张牌，然后失去1点体力",
	["thyuanqi:throw"]="弃置这张牌，并令其获得一张牌",
	["@thyuanqi"]="你可以将一张相同颜色的牌交给一名其他角色",
	["~thyuanqi"]="选择一张牌→选择目标角色→点击确定",
	["thmoji"]="墨迹",
	[":thmoji"]="锁定技，回合结束时或你于回合外手牌数变化后，若你的手牌数小于二，你须将手牌补至两张。",

	["#yuki006"]="凶兆的黑猫",
	["yuki006"]="橙",--雪 - 空 - 4血
	["designer:yuki006"]="幻桜落",
	["illustrator:yuki006"]="橙狼",
	["cv:yuki006"]="暂无",
	["thjibu"]="疾步",
	[":thjibu"]="锁定技，当你计算与其他角色的距离时，始终-1。",
	["thdunjia"]="遁甲",
	[":thdunjia"]="每当你使用【杀】造成一次伤害后，你可以选择一项：弃置受到该伤害的角色X张牌；或摸X张牌（X为你与该角色的装备区里的装备牌数差，且至多为三）。",
	["thdunjia:discard"]="弃置其X张牌",
	["thdunjia:draw"]="摸X张牌",

	["#yuki007"]="策士的九尾",
	["yuki007"]="八云蓝",--雪 - 空 - 3血
	["designer:yuki007"]="淬毒",
	["illustrator:yuki007"]="橙狼",
	["cv:yuki007"]="暂无",
	["thchouce"]="筹策",
	[":thchouce"]="你于出牌阶段使用的第一张牌，或点数比你此阶段使用的前一张牌大的牌，可以无视合法性指定一名角色为目标。<br />" ..
				  "◇<font color=\"red\"><b>注意</b></font>：此技能的两种使用方法：<br />" ..
				  "1. 通过点击“<font color=\"green\"><b>筹策</b></font>”按钮使用；<br />" ..
				  "2. 正常使用一张卡牌（可随意选择目标），若满足“筹策”条件，则会在<font color=\"red\"><b>点击确定后</b></font>询问。",
	["@thchouce"]="你可以发动“筹策”",
	["$ThChouce"]="%from 发动了“%arg”将 %to 指定为了 %card 的目标",
	["thzhanshi"]="占筮",
	[":thzhanshi"]="锁定技，出牌阶段结束时，若你于此阶段使用的牌不少于三张，且全部发动了“筹策”，你在此回合结束后进行一个额外的回合，且于额外回合的回合开始时获得技能“幻葬”直到回合结束（锁定技，出牌阶段结束时，你进行一次判定，若结果为黑色，你失去1点体力）。",
	["@tianji"]="天机",
	["thhuanzang"]="幻葬",
	[":thhuanzang"]="锁定技，出牌阶段结束时，你进行一次判定，若结果为黑色，你失去1点体力。",

	["#yuki008"]="法界之火",
	["yuki008"]="圣白莲",--雪 - 空 - 3血
	["designer:yuki008"]="冬天吃雪糕",
	["illustrator:yuki008"]="えふぇ",
	["cv:yuki008"]="暂无",
	["thziyun"]="紫云",
	[":thziyun"]="锁定技，你不能成为【玄海仙鸣】或【枯羽华庭】的目标。",
	["thchuiji"]="垂迹",
	[":thchuiji"]="弃牌阶段结束时，若你于此阶段弃置了两张或更多的手牌，或每当你于回合外失去牌时，你可进行一次判定，若为红色，令一名角色回复1点体力；若为黑色，弃置一名其他角色的一张牌。",

	["#yuki009"]="幻想的境界",
	["yuki009"]="八云紫",--雪 - 空 - 4血
	["designer:yuki009"]="昂翼天使",
	["illustrator:yuki009"]="DomotoLain",
	["cv:yuki009"]="暂无",
	["thlingya"]="灵压",
	[":thlingya"]="每当其他角色于你的回合使用红色牌，在结算后你可以令其选择一项：令你摸一张牌；或令你弃置其一张牌。",
	["thlingya:letdraw"]="令其摸一张牌",
	["thlingya:discard"]="令其弃置你一张牌",
	["thheimu"]="黑幕",
	[":thheimu"]="出牌阶段限一次，当你使用一张牌指定目标时，你可以令一名其他角色成为此牌的使用者。",
	["@thheimu"]="你可以发动“黑幕”",

	["#yuki010"]="湖上的冰精",
	["yuki010"]="琪露诺",--雪 - 空 - 4血
	["designer:yuki010"]="昂翼天使",
	["illustrator:yuki010"]="橙狼",
	["cv:yuki010"]="暂无",
	["thhanpo"]="寒魄",
	[":thhanpo"]="锁定技，防止你对其他角色造成的火焰伤害，防止你受到的火焰伤害。",
	["thzhengguan"]="争冠",
	[":thzhengguan"]="每当一名其他角色跳过摸牌阶段或出牌阶段后，你可以弃置一张红色牌，并进行一个额外的该阶段。",
	["@thzhengguan"]="你可以弃置一张红色牌发动“争冠”进行一个额外的 %arg 阶段",
	["#ThZhengguan"]="%from 发动了“%arg”，进行了一个额外的 %arg2 阶段",
	["thbingpu"]="氷瀑",
	[":thbingpu"]="限定技，出牌阶段，你可以令所有其他有牌的角色做出选择：打出一张【闪】，或令你依次弃置其两张牌。",
	["@thbingpu"]="%src 使用了“氷瀑”，请打出一张【闪】或点“取消”令其弃置你的两张牌",

	["#yuki011"]="冬的忘却之物",
	["yuki011"]="蕾蒂•怀特罗克",--雪 - 空 - 3血
	["&yuki011"]="蕾蒂",
	["designer:yuki011"]="幻桜落",
	["illustrator:yuki011"]="阿佐ヒナ",
	["cv:yuki011"]="暂无",
	["thdongmo"]="冬末",
	[":thdongmo"]="结束阶段开始时，你可以指定至多X名其他角色（X为你已损失的体力值），你和这些角色将各自的武人物翻面，并各摸一张牌。",
	["@thdongmo"]="你可以发动“冬末”",
	["~thdongmo"]="选择X名角色→点击确定",
	["thlinhan"]="凛寒",
	[":thlinhan"]="当你使用或者打出一张【闪】时，你可以摸一张牌。",

	["#yuki012"]="平凡陈腐的山彦",
	["yuki012"]="幽谷响子",--雪 - 空 - 3血
	["designer:yuki012"]="仲达与孔明",
	["illustrator:yuki012"]="CUBE",
	["cv:yuki012"]="暂无",
	["thfusheng"]="复声",
	[":thfusheng"]="锁定技，其他角色每令你回复1点体力，该角色摸一张牌；每当你受到一次伤害后，伤害来源须交给你一张红桃手牌，否则失去1点体力。",
	["@thfusheng-heart"]="请给出一张红桃手牌",
	["thhuanfa"]="幻法",
	[":thhuanfa"]="出牌阶段限一次，你可以将一张红桃手牌交给一名其他角色，然后你获得该角色的一张牌并将该牌交给除该角色外的一名角色。",
	["@thhuanfa-give"] = "你可以将此牌交给除 %src 外的一名角色",

	["#yuki013"]="骚灵乐团",
	["yuki013"]="普莉茲姆利巴三姐妹",--雪 - 空 - 3血
	["&yuki013"]="骚灵三姐妹",
	["designer:yuki013"]="绝顶油条",
	["illustrator:yuki013"]="ke-ta",
	["cv:yuki013"]="暂无",
	["thsaozang"]="骚葬",
	[":thsaozang"]="弃牌阶段结束时，你每于此阶段弃置了一种类别的牌，你可以依次弃置一名其他角色的一张手牌。",
	["@thsaozang"]="你可以发动“骚葬”",
	["thxuqu"]="絮曲",
	[":thxuqu"]="你的回合外，当你失去手牌时，你可以令一名其他角色摸一张牌。",
	["@thxuqu"]="你可以发动“絮曲”",

	["#yuki014"]="小小贤将",
	["yuki014"]="娜兹玲",--雪 - 空 - 4血
	["designer:yuki014"]="xyzbilliu",
	["illustrator:yuki014"]="DomotoLain",
	["cv:yuki014"]="暂无",
	["thqiebao"]="窃宝",
	[":thqiebao"]="一名角色对另一名角色使用【桃】时，你可以弃置一张【杀】并令此【桃】对目标角色无效，若此【杀】为红色，你获得该【桃】；一名角色获得另一名角色的牌时，你可弃置一张【杀】，若此【杀】为红色，你获得这些牌，否则置入弃牌堆。",
	["@thqiebao"]="你可以弃置一张【杀】发动“窃宝”",

	["#yuki015"]="虎纹的毘沙门天",
	["yuki015"]="寅丸星",--雪 - 空 - 4血
	["designer:yuki015"]="幻桜落",
	["illustrator:yuki015"]="山口真之介",
	["cv:yuki015"]="暂无",
	["thlingta"]="灵塔",
	[":thlingta"]="你可以选择一至两项：<br />1.跳过你此回合的摸牌阶段；<br />2.跳过你此回合的出牌阶段。<br />你每选择一项，获得1枚“法灯”标记。",
	["thlingta:3"]="你可以发动“灵塔”跳过摸牌阶段",
	["thlingta:4"]="你可以发动“灵塔”跳过出牌阶段",
	["thweiguang"]="威光",
	[":thweiguang"]="你可以选择一至四项：<br />1.跳过你此回合的判定阶段；<br />2.跳过你此回合的弃牌阶段；<br />3.在你此回合的摸牌阶段后额外进行一个摸牌阶段；<br />4.在你此回合的出牌阶段后额外进行一个出牌阶段。<br />你每选择一项，须先弃置1枚“法灯”标记，每项每回合限一次。",
	["thweiguang:2"]="你可以发动“威光”跳过判定阶段",
	["thweiguang:5"]="你可以发动“威光”跳过弃牌阶段",
	["#thweiguang-skip"]="威光（跳过阶段）",
	["thweiguang:3"]="你可以发动“威光”进行一个额外的摸牌阶段",
	["thweiguang:4"]="你可以发动“威光”进行一个额外的出牌阶段",
	["thchuhui"]="初辉",
	[":thchuhui"]="锁定技，游戏开始时，你获得1枚“法灯”标记。",
	["@fadeng"]="法灯",
	
	["#yuki016"]="宝船之御守",
	["yuki016"]="云居一轮",--雪 - 空 - 4血
	["designer:yuki016"]="香蒲神殇",
	["illustrator:yuki016"]="閏月戈",
	["cv:yuki016"]="暂无",
	["thkujie"]="苦戒",
	[":thkujie"]="其他角色的出牌阶段限一次，若你在其攻击范围内，该角色可以弃置一张红色基本牌，然后令你失去1点体力，若如此做，此回合结束后，你回复2点体力，并令一名角色摸一张牌。",
	["thkujiev"]="苦戒",
	[":thkujiev"]="出牌阶段限一次，若云居一轮在你攻击范围内，你可以弃置一张红色基本牌，然后令云居一轮失去1点体力，若如此做，此回合结束后，云居一轮回复2点体力。",
	["#thkujie-recover"]="苦戒（回复体力）",
	["thyinbi"]="廕庇",
	[":thyinbi"]="当其他角色因伤害而扣减体力后，若该伤害不小于2点，你可以令其回复等量的体力，然后伤害来源对你造成等量的同属性伤害。",

	["#yuki017"]="惨憺的大海原",
	["yuki017"]="村纱水蜜",--雪 - 空 - 4血
	["designer:yuki017"]="幻桜落",
	["illustrator:yuki017"]="ひそな",
	["cv:yuki017"]="暂无",
	["thmingling"]="溟灵",
	[":thmingling"]="锁定技，防止你受到的火焰伤害，你每次受到的雷电伤害+1。",
	["thchuanshang"]="船殇",
	[":thchuanshang"]="出牌阶段限一次，你可以令你攻击范围内的一名角色获得1枚“溺水”标记，若其已拥有该标记，你失去1点体力。其他角色每拥有1枚该标记，其手牌上限便-1。其他角色的结束阶段开始时，若其拥有1枚或更多的“溺水”标记，须进行一次判定，若为红桃，弃置全部的该标记；若为黑色，获得1枚该标记。",
	["@nishui"]="溺水",
	["#ThChuanshang"]="%from 受到了“%arg”的影响",

	["#yuki018"]="天衣无缝的亡灵",
	["yuki018"]="西行寺幽幽子",--雪 - 幻 - 3血
	["designer:yuki018"]="幻桜落",
	["illustrator:yuki018"]="6U",
	["cv:yuki018"]="暂无",
	["thlingdie"]="灵蝶",
	[":thlingdie"]="出牌阶段限一次，你可以选择一名角色并交给其一张手牌（若为你则不交给），然后该角色观看由其选择的另一名角色的手牌。",
	["thwushou"]="无寿",
	[":thwushou"]="每当你受到一次伤害后，你可以将你的手牌补至四张。",
	["thfuyue"]="浮月",
	[":thfuyue"]="君主技，其他雪势力角色的出牌阶段限一次，若你已受伤，可以与你拼点，若该角色没有赢，你回复1点体力。",
	["thfuyuev"]="浮月",
	[":thfuyuev"]="出牌阶段限一次，若你为雪势力且君主已受伤，你可以与君主拼点，若你没赢，君主回复1点体力。",
}