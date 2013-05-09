-- translation for Kami Package

return {
	["kami"] = "神",

	["#kami001"]="往昔的圣迹",
	["kami001"]="东风谷早苗",--神 - 空 - 3血
	["designer:kami001"]="幻桜落 | Codeby:Slob",
	["illustrator:kami001"]="正体不明",
	["cv:kami001"]="暂无",
	["thkexing"]="客星",
	[":thkexing"]="回合开始阶段开始时，你可以从牌堆顶亮出三张牌，将其中任意数量的非锦囊牌以任意顺序置于牌堆底，然后将其余的牌置入弃牌堆。",
	["$ThKexing"]="%from 使用“%arg”置于牌堆底的牌为 %card",
	["thshenfeng"]="神风",
	[":thshenfeng"]="出牌阶段，你可以将两张相同花色的牌交给一名体力值比你多的角色，然后该角色须对其距离为1的由你指定的一名角色造成1点伤害，每阶段限一次。",
	["thkaihai"]="开海",
	[":thkaihai"]="每当你失去最后的手牌时，可从牌堆底摸一张牌。",
	
	["#kami002"]="天道是非",
	["kami002"]="比那名居天子",--神 - 空 - 4血
	["designer:kami002"]="幻桜落 | Codeby:Slob",
	["illustrator:kami002"]="正体不明",
	["cv:kami002"]="暂无",
	["thjiguang"]="极光",
	[":thjiguang"]="回合开始阶段开始时，你可选择一项“气象”，效果持续到你的下回合开始。你不能选择你上回合所选择的“气象”。\
“烈日”：所有角色每次受到的火焰伤害+1。\
“风雨”：所有角色计算与其他角色的距离时，始终-1。\
“黄砂”：所有角色每次受到多于1点的伤害时，防止多余的伤害。\
“昙天”：所有角色可将红桃花色的【闪】当【桃】使用。\
“浓雾”：所有角色每使用【杀】对与其距离1以内的角色造成一次伤害后，回复1点体力。",
	["@jglieri"]="烈日",
	["@jgfengyu"]="风雨",
	["@jghuangsha"]="黄砂",
	["@jgtantian"]="昙天",
	["@jgnongwu"]="浓雾",
	["jglieri"]="烈日",
	["jgfengyu"]="风雨",
	["jghuangsha"]="黄砂",
	["jgtantian"]="昙天",
	["jgnongwu"]="浓雾",
	["thjiguangdis"]="极光(风雨)", 
	["thjiguanggivenskill"]="极光(昙天)", 
	[":thjiguanggivenskill"]="你可将红桃花色的【闪】当【桃】使用。", 
	
	["#kami003"]="七色莲华之蝶",
	["kami003"]="博丽灵梦",--神 - 空 - 3血
	["designer:kami003"]="幻桜落 | Codeby:Slob",
	["illustrator:kami003"]="正体不明",
	["cv:kami003"]="暂无",
	["thmengsheng"]="梦生",
	[":thmengsheng"]="每当你受到一次伤害后，你可获得伤害来源的一张手牌，然后该角色失去当前的所有武将技能直到你的下一个回合结束。",
	["@mengsheng"]="梦生",
	["thqixiang"]="绮想",
	[":thqixiang"]="你攻击范围内的其他角色的弃牌阶段结束时，你可弃置一张手牌，令其摸一张牌或弃置一张手牌。",
	["@thqixiang"]="你可以弃置一张手牌发动“绮想”",
	["thqixiang:draw"]="摸一张牌",
	["thqixiang:discard"]="弃置一张牌",
	
	["#kami004"]="子夜的女皇",
	["kami004"]="蕾米莉亚•斯卡雷特",--神 - 空 - 4血
	["designer:kami004"]="幻桜落 | Codeby:Slob",
	["illustrator:kami004"]="正体不明",
	["cv:kami004"]="暂无",
	["thgugao"]="孤高",
	[":thgugao"]="出牌阶段，你可与一名其他角色拼点，若你赢，你对其造成1点伤害，否则受到其对你造成的1点伤害，每阶段限一次。“<b>千狱</b>”发动后，若双方拼点的牌花色不同，防止你因没有赢受到的伤害；“<b>皇仪</b>”发动后，若你赢，视为你此阶段没有发动“<b>孤高</b>”。",
	["thqianyu"]="千狱",
	[":thqianyu"]="<b>觉醒技</b>，一名角色的回合结束后，若你的体力值为1，你须将体力上限减少至1点，并获得技能“<b>狂魔</b>”（<b>锁定技</b>，若你拥有技能“<b>孤高</b>”，每当你对一名其他角色造成除【杀】以外的1点伤害后，你增加1点体力上限，然后回复1点体力。），然后进行一个额外的回合。",
	["#ThQianyu"]="%from 的体力值为 %arg2 ，触发“%arg”，将进行一个额外的回合",
	["@qianyu"]="千狱",
	["thkuangmo"]="狂魔",
	[":thkuangmo"]="<b>锁定技</b>，若你拥有技能“<b>孤高</b>”，每当你对一名角色造成除【杀】以外的1点伤害后，你增加1点体力上限，然后回复1点体力。",
	["thhuangyi"]="皇仪",
	[":thhuangyi"]="<b>觉醒技</b>，“<b>千狱</b>”发动后，回合开始阶段开始时，若你的体力上限大于X，你须将体力上限减少至X并摸等同于减少数量的牌，然后回复1点体力并失去技能“<b>狂魔</b>”（X为你游戏开始时的体力上限）。",
	["@huangyi"]="皇仪",
	
	["#kami007"]="华胥之梦",
	["kami007"]="西行寺幽幽子",--神 - 幻 - 2血
	["designer:kami007"]="幻桜落 | Codeby:Slob",
	["illustrator:kami007"]="正体不明",
	["cv:kami007"]="暂无",
	["thfanhun"]="返魂",
	[":thfanhun"]="若你拥有技能“<b>满咲</b>”，当你处于濒死状态时，你可获得1枚“桜咲”标记且体力回复至1点，然后将你的武将牌翻至正面朝上并重置之。",
	["thyoushang"]="诱殇",
	[":thyoushang"]="若你拥有技能“<b>满咲</b>”，每当你的红色牌对目标角色造成伤害时，你可以防止此伤害，并令该角色减少1点体力上限，然后获得1枚“桜咲”标记。",
	["thyouya"]="幽雅",
	[":thyouya"]="每当你受到伤害时，你可以弃置一张牌并令至多X名角色选择一项：打出一张【闪】；或令你获得其一张牌。（X为你的“桜咲”标记的数量）",
	["@thyouya"]="你可以使用技能【幽雅】",
	["~thyouya"]="选择一张牌→选择至多X名角色→点击确定",
	["@thyouya-jink"]="请打出一张【闪】或点“取消”令 %src 摸一张牌",
	["thmanxiao"]="满咲",
	[":thmanxiao"]="<b>锁定技</b>，你每拥有1枚“桜咲”标记，你的手牌上限便+1。当你拥有4枚“桜咲”标记时，你立即死亡。",
	["@yingxiao"]="桜咲",
	
	["#kami008"]="夫众生者悉皆杀",
	["kami008"]="芙兰朵露•斯卡雷特",--神 - 空 - 4血
	["designer:kami008"]="幻桜落 | Codeby:Slob",
	["illustrator:kami008"]="正体不明",
	["cv:kami008"]="暂无",
	["thjinlu"]="尽戮",
	[":thjinlu"]="出牌阶段，你可弃置一张牌并指定一名其他角色，获得其全部的牌。此出牌阶段结束时，你须交给该角色等同于其体力值数量的牌，且回合结束阶段开始时将你的武将牌翻面。每阶段限一次。",
	["ThJinluGoBack"] = "请交给目标角色 %arg 张牌",
	["thkuangli"]="狂戾",
	[":thkuangli"]="每当你的武将牌翻面、横置或重置时，你可以摸一张牌。",
	
	["#kami009"]="心华之殇",
	["kami009"]="古明地觉",--神 - 空 - 4血
	["designer:kami009"]="幻桜落 | Codeby:Slob",
	["illustrator:kami009"]="正体不明",
	["cv:kami009"]="暂无",
	["thyuxin"]="愈心",
	[":thyuxin"]="摸牌阶段，你可弃置一张牌，并放弃摸牌，改为从牌堆顶亮出两张牌并获得之，若这两张牌均为红桃，你可以指定一名角色并令其回复1点体力。",
	["@thyuxin"]="你可以弃置一张牌发动技能“愈心”",
	["thchuangxin"]="疮心",
	[":thchuangxin"]="出牌阶段开始时，你可弃置X张牌，然后选择X项：\
1. 获得技能“<b>灵视</b>”，直到回合结束 \
2. 获得技能“<b>捉月</b>”，直到回合结束",
	["@thchuangxin"]="是否发动技能“疮心”",
	["~thchuangxin"]="选择1~2张牌→点击确定",
	["thtianxin"]="天心",
	[":thtianxin"]="你的回合开始时，你可弃置X张牌，然后选择X项：\
1. 获得技能“<b>预悉</b>”，直到回合结束 \
2. 获得技能“<b>天妒</b>”，直到回合结束",
	["@thtianxin"]="是否发动技能“天心”",
	["~thtianxin"]="选择1~2张牌→点击确定",
	
	["#kami010"]="星煌的祈者",
	["kami010"]="霍青娥",--神 - 幻 - 3血
	["designer:kami010"]="幻桜落 | Codeby:Slob",
	["illustrator:kami010"]="正体不明",
	["cv:kami010"]="暂无",
	["thrangdeng"]="禳灯",
	[":thrangdeng"]="<b>锁定技</b>，出牌阶段开始时，你须选择一项：将至多三张与你武将牌上的任何一张牌点数都不相同的手牌面朝上置于你的武将牌上，称为“灯”；或弃置一张手牌。你的武将牌上每有一种花色的“灯”，你获得相应的技能：红桃“<b>捉月</b>”；黑桃“<b>濛雾</b>”；方片“<b>沉红</b>”；梅花“<b>疾步</b>”。",
	["thrangdengpile"]="灯",
	["@ThRangdeng"]="你可以发动“禳灯”将一张手牌置于武将牌上",
	["thbaihun"]="拜魂",
	[":thbaihun"]="出牌阶段，你可以将十三张“灯”置入弃牌堆，然后令一名其他角色立即死亡。",
	
	["#kami012"]="竹取飞翔",
	["kami012"]="蓬莱山辉夜",--神 - 空 - 4血
	["designer:kami012"]="幻桜落 | Codeby:Slob",
	["illustrator:kami012"]="正体不明",
	["cv:kami012"]="暂无",
	["thwunan"]="五难",
	[":thwunan"]="每当一名其他角色：\
1. 使用一张【桃园结义】或【五谷丰登】时\
2. 于濒死状态回复体力后，若其体力值不小于1\
3. 受到一次火焰伤害后\
4. 发动【净琉璃镜】使用一张【闪】时\
5. 使用【杀】对没有手牌的角色造成伤害时\
你可以弃置一张手牌，然后回复1点体力并弃置该角色的一张手牌。",
	["@thwunan"]="你可以弃置一张手牌发动“五难”",

	["#kami013"]="鸣蛙不输风雨",
	["kami013"]="洩矢诹访子",--神 - 幻 - 1血
	["designer:kami013"]="幻桜落 | Codeby:Slob",
	["illustrator:kami013"]="正体不明",
	["cv:kami013"]="暂无",
	["thsanling"]="散灵",
	[":thsanling"]="<b>锁定技</b>，一名角色的回合结束后，若你没有手牌，你立即死亡。",
	["thbingzhang"]="氷障",
	[":thbingzhang"]="每当你受到伤害结算开始时或即将失去体力时，你可以弃置两张牌，然后防止此伤害或此次失去体力。",
	["thjiwu"]="极武",
	[":thjiwu"]="<b>锁定技</b>，你的【桃】和【酒】均视为【闪】。弃牌阶段外，每当你的【闪】、【桃】或【酒】置入弃牌堆或其他角色每获得你的一张手牌时，你摸一张牌。",
	["thsisui"]="祀祟",
	[":thsisui"]=" 回合开始阶段开始时，你可以令所有其他角色各弃置一张手牌，然后你从弃牌堆获得这些牌中的至多三张牌，并依次交给一名角色。",
	["@thsisui"]="受“祀祟”影响，你须弃置一张手牌",
	["thzhanying"]="湛樱",
	[":thzhanying"]="<b>锁定技</b>，你始终跳过你的摸牌阶段；你的手牌上限+4。",
	
	["#kami014"]="诞妄的妖姬",
	["kami014"]="封兽鵺",--神 - 幻 - 3血
	["designer:kami014"]="幻桜落 | Codeby:Slob",
	["illustrator:kami014"]="正体不明",
	["cv:kami014"]="暂无",
	["thluli"]="陆离",
	[":thluli"]="当你对体力值不小于你的一名其他角色造成伤害，或一名体力值不小于你的其他角色对你造成伤害时，你可以弃置一张黑色手牌令你造成的伤害+1；或弃置一张红色手牌令你受到的伤害-1。",
	["@ThLuliIncrease"] = "你可以弃置一张黑色手牌：若如此做，此伤害+1",
	["@ThLuliDecrease"] = "你可以弃置一张红色手牌：若如此做，此伤害-1",
	["#ThLuliIncrease"] = "%from 发动了“<font color=\"yellow\"><b>陆离</b></font>”，伤害点数从 %arg 点增加至 %arg2 点",
	["#ThLuliDecrease"] = "%from 发动了“<font color=\"yellow\"><b>陆离</b></font>”，伤害点数从 %arg 点减少至 %arg2 点",
	["thguihuan"]="诡幻",
	[":thguihuan"]="<b>限定技</b>，若你的身份不是主公，当你杀死一名身份不是主公的其他角色，在其翻开身份牌之前，你可以与该角色交换身份牌。",
	["@guihuan"]="诡幻(未发动)",
	["@guihuanused"]="诡幻(已发动)",
	
	["#kami015"]="冰魄的女仙",
	["kami015"]="琪露诺",--神 - 空 - 3血
	["designer:kami015"]="幻桜落 | Codeby:Slob",
	["illustrator:kami015"]="正体不明",
	["cv:kami015"]="暂无",
	["thzhizun"]="至尊",
	[":thzhizun"]="回合结束阶段开始时，你可以选择一项：改变一名其他角色的势力属性；或获得一项技能（你只可以以此法获得“<b>励气</b>”、“<b>唤卫</b>”、“<b>济援</b>”、“<b>御姬</b>”、“<b>祈愿</b>”、“<b>颂威</b>”、“<b>春度</b>”或“<b>舞华</b>”，你不能以此法获得场上其他存活角色拥有的技能）。",
	["thzhizun:modify"] = "改变一名其他角色的势力属性",
	["thzhizun:obtain"] = "获得一项未加入游戏或已死亡角色的主公技",
	["thfeiying"]="飞影",
	[":thfeiying"]="<b>锁定技</b>，当其他角色计算与你的距离时，始终+1。",
	
	["#kami016"]="最古妖王的宝库",
	["kami016"]="因幡帝",--神 - 幻 - 3血
	["designer:kami016"]="幻桜落 | Codeby:Slob",
	["illustrator:kami016"]="正体不明",
	["cv:kami016"]="暂无",
	["thlijian"]="离剑",
	[":thlijian"]="<b>限定技</b>，回合开始阶段开始时，你可以展示一张手牌，所有其他角色须各选择一项：弃置一张与此牌类型相同的牌；或受到你对其造成的1点伤害。你的下一个回合开始阶段开始时，须重复一次此流程。",
	["thsiqiang"]="死枪",
	[":thsiqiang"]="<b>限定技</b>，出牌阶段，你可以选择一名其他角色，若如此做，该角色不能使用或打出【杀】、【闪】或【无懈可击】，直到回合结束。",
	["thjiefu"]="戒符",
	[":thjiefu"]="<b>限定技</b>，出牌阶段，你可弃置一名其他角色装配区的所有牌，然后令其失去全部的武将技能，直到回合结束。",
	["thhuanxiang"]="幻乡",
	[":thhuanxiang"]="<b>限定技</b>，当你受到伤害结算开始时，你可以防止此伤害，且将你的体力回复至３点；然后你不能成为其他角色的牌的目标，且每当你受到伤害结算开始时，防止该伤害，直到你的下回合开始。",
	
}