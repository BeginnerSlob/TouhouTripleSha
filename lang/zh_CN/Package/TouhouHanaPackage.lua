-- translation for TouhouHanaPackage

return {
	["touhou-hana"] = "东方的花火",

	["#hana001"]="四时的花之主",
	["hana001"]="风见幽香",--花 - 空 - 4血
	["designer:hana001"]="幻桜落",
	["illustrator:hana001"]="An2A",
	["cv:hana001"]="暂无",
	["thhuaji"]="花祭",
	[":thhuaji"]="在你的回合，当其他角色使用一张基本牌或非延时类锦囊牌时，你可弃置一张黑色牌，则该角色需弃置一张与其之前使用的牌名称相同的牌，否则该牌无效。",
	["@thhuajiuse"]="你可以弃置一张黑色牌发动“花祭”",
	["@thhuaji"]="请弃置一张【%arg】使“花祭”失效",
	["thfeizhan"]="菲绽",
	[":thfeizhan"]="君主技，锁定技，你的攻击范围和你的手牌上限+X（X为存活的其他花势力角色的数量）。",
	
	["#hana002"]="普通的魔法使",
	["hana002"]="雾雨魔理沙",--花 - 空 - 4血
	["designer:hana002"]="幻桜落",
	["illustrator:hana002"]="えふぇ",
	["cv:hana002"]="暂无",
	["thjiewu"]="借物",
	[":thjiewu"]="出牌阶段限一次，你可获得一名你攻击范围内的角色的一张牌，视为该角色对你使用一张无视距离的【杀】，且此【杀】无视你的防具。",
	["thgenxing"]="根性",
	[":thgenxing"]="觉醒技，准备阶段开始时，若你的体力值为1，你须回复1点体力或摸两张牌，然后减少1点体力上限并获得技能“魔炮”（每当你使用或打出一张【闪】时，你可以令一名其他角色摸一张牌，然后你对其造成1点火焰伤害。）。",
	["thgenxing:recover"]="回复1点体力",
	["thgenxing:draw"]="摸两张牌",
	["@genxing"]="根性",
	["#ThGenxing"]="%from 的体力值为 %arg2，触发“%arg”觉醒",
	["thmopao"]="魔炮",
	[":thmopao"]="每当你使用或打出一张【闪】时，你可以令一名其他角色摸一张牌，然后你对其造成1点火焰伤害。",
	["@thmopao"]="你可以发动“魔炮”",
	
	["#hana003"]="三途川的引路人",
	["hana003"]="小野塚小町",--花  - 幻 - 3血
	["designer:hana003"]="幻桜落",
	["illustrator:hana003"]="G.H",
	["cv:hana003"]="飞鸟",
	["~hana003"]="死神，亦有归去之时...",
	["thbian"]="彼岸",
	[":thbian"]="当一名处于濒死状态的角色成为【桃】的目标时，你可弃置一张锦囊牌，令此【桃】对其无效。",
	["@thbian"]="你可以弃置一张锦囊牌对 %src 发动“彼岸”",
	["$thbian1"]="余命无几，且让咱送汝一程。",
	["$thbian2"]="不惜身命，悔之晚矣。",
	["thguihang"]="归航",
	[":thguihang"]="当一名角色进入濒死状态时，你可展示其一张手牌，若为红色，该角色须弃置之并回复1点体力。",
	["@thguihang"]="请展示一张手牌",
	["$thguihang1"]="阳寿未尽，怎可渡汝？",
	["$thguihang2"]="可惜身命，乃可为哉。",
	["thwujian"]="无间",
	[":thwujian"]="出牌阶段限一次，你可弃置一张牌并指定你攻击范围内的一名角色，直到你的下回合开始，该角色计算与除其以外的角色的距离时，始终+1。",
	["@wujian"]="无间",
	["#thwujian-distance"]="无间",
	["$thwujian1"]="碧落黄泉，咫尺亦是天涯。",
	["$thwujian2"]="近在眼前，远在天边。",
	
	["#hana004"]="小小甘蜜毒药",
	["hana004"]="梅蒂欣•梅兰可莉",--花 - 空 - 3血
	["&hana004"]="梅蒂欣",
	["designer:hana004"]="幻桜落",
	["illustrator:hana004"]="亜音",
	["cv:hana004"]="暂无",
	["thxuelan"]="血兰",
	[":thxuelan"]="你可以弃置一张红色牌并抵消一张【桃】对一名角色的效果，然后若该角色的体力上限不大于其游戏开始时的体力上限，则该角色须增加1点体力上限。",
	["@thxuelan"]="你可以弃置一张红色牌发动“血兰”",
	["thxinwang"]="心妄",
	[":thxinwang"]="你的回合外，每当你使用、打出一张红桃牌时，或因弃置而失去一张红桃牌后，你可以摸一张牌或令一名其他角色回复1点体力。",
	["@thxinwang"]="你可以令一名其他角色回复1点体力或点“取消”摸一张牌",
	["thjuedu"]="绝毒",
	[":thjuedu"]="锁定技，杀死你的角色获得技能“崩坏”。",
	
	["#hana005"]="暗海的绯之衣",
	["hana005"]="永江衣玖",--花 - 空 - 4血
	["designer:hana005"]="幻桜落",
	["illustrator:hana005"]="DomotoLain",
	["cv:hana005"]="暂无",
	["thtingwu"]="霆舞",
	[":thtingwu"]="出牌阶段，每当你对一名人物牌竖置的其他角色造成雷电伤害，在伤害结算后，你可以进行一次判定，若结果不为红桃，你对该角色下家造成1点雷电伤害。",
	["thyuchang"]="羽裳",
	[":thyuchang"]="锁定技，你使用具雷电伤害的【杀】时无距离限制；你的梅花【杀】均视为具雷电伤害的【杀】。",
	
	["#hana006"]="佐渡的狸妖",
	["hana006"]="二岩猯藏",--花 - 空 - 4血
	["designer:hana006"]="昂翼天使",
	["illustrator:hana006"]="G.H",
	["cv:hana006"]="暂无",
	["thxihua"]="戏画",
	[":thxihua"]="准备阶段开始时，你可以将一张手牌面朝下置于你的人物牌上，称为“戏”，视为对一名其他角色使用一张无视距离的【杀】或【碎月绮斗】。此【杀】或【碎月绮斗】即将造成伤害时，亮出“戏”，若不为【杀】，防止此伤害；否则你弃置其一张手牌。在结算后，将“戏”置入弃牌堆。",
	["xihuapile"]="戏",
	["@thxihua"]="你可以发动“戏画”",
	["~thxihua"]="选择一张手牌→点击确定",
	
	["#hana007"]="平安京的妖云",
	["hana007"]="封兽鵺",--花 - 幻 - 3血
	["designer:hana007"]="星野梦美&幻桜落 | Codeby:Ellis",
	["illustrator:hana007"]="にろ",
	["cv:hana007"]="子覃",
	["~hana007"]="这...这张弓是...？！",
	["thmimeng"]="迷蒙",
	[":thmimeng"]="你可将你最后一张手牌当除【绯想镜诗】、【竹曲谜宝】、【妙手探云】、【神惠雨降】、【魔闪花火】、【荒波暴流】、【血凤涅烟】、【罔两空界】和【幽瞳占略】外的一张基本牌或非延时类锦囊牌使用或打出。",
	["$thmimeng1"]="哈哈哈...真遗憾...",
	["$thmimeng2"]="你·猜·错·了~！",
	["thmimeng_skill_saveself"] = "迷蒙自救",
	["thmimeng_skill_slash"] = "迷蒙出杀",
	["thanyun"]="暗云",
	[":thanyun"]="摸牌阶段开始时，你可以放弃摸牌，若如此做，此回合的结束阶段开始时，你摸两张牌。",
	["$thanyun1"]="今夜，你们的眼睛将会欺骗你们的心灵~！",
	["$thanyun2"]="今夜，一切都将变得虚实难辨~！",
	["$thanyun3"]="演出结束~！",
	
	["#hana008"]="独臂有角的仙人",
	["hana008"]="茨木华扇",--花 - 空 - 3血
	["designer:hana008"]="桃花僧",
	["illustrator:hana008"]="十誤一会",
	["cv:hana008"]="暂无",
	["thquanshan"]="劝善",
	[":thquanshan"]="出牌阶段限一次，你可以令一名有手牌的其他角色将至少一张手牌交给另一名除你以外的角色，若这些牌均为同一类别，你摸一张牌。",
	["thquanshangive"]="劝善",
	["@thquanshan"]="请将至少一张手牌交给除使用者外的其他角色",
	["~thquanshangive"]="选择任意张手牌→选择一名除使用者外的其他角色→点击确定",
	["thxiangang"]="仙罡",
	[":thxiangang"]="每当你受到伤害时，可以进行一次判定，若结果为梅花，防止此伤害。",
	["#thxiangang"]="%from 发动“%arg”防止了此伤害",
	
	["#hana009"]="地狱的裁判长",
	["hana009"]="四季映姬",--花 - 空 - 4血
	["designer:hana009"]="笔枔",
	["illustrator:hana009"]="珠洲城くるみ",
	["cv:hana009"]="暂无",
	["thduanzui"]="断罪",
	[":thduanzui"]="出牌阶段限一次，你可以展示一名其他角色的一张手牌，若为【杀】，视为你对该角色使用一张【碎月绮斗】，此【碎月绮斗】不能被【三粒天滴】响应；若为【闪】，视为你对该角色使用一张无视距离且不计入使用限制的【杀】。",
	
	["#hana010"]="地上的彗星",
	["hana010"]="莉格露•奈特巴格",--花 - 空 - 4血
	["&hana010"]="莉格露",
	["designer:hana010"]="冬天吃雪糕",
	["illustrator:hana010"]="くまだ",
	["cv:hana010"]="暂无",
	["thyingdeng"]="萤灯",
	[":thyingdeng"]="准备阶段开始时，你可以弃置一张牌，其他角色不能使用或打出与该牌花色相同的牌，直到回合结束。",
	["@thyingdeng"]="你可以弃置一张牌来发动“萤灯”",
	["thzheyin"]="蛰隐",
	[":thzheyin"]="出牌阶段限一次，你可以摸一张牌，然后将一张牌面朝下置于你的人物牌上，称为“蛰”。每当你成为一张牌的目标时，你可以展示“蛰”，若该牌与你的“蛰”花色相同，该牌对你无效；你的回合开始时，你须弃置这张“蛰”。",
	["thzheyinpile"]="蛰",
	["@thzheyin"]="请将一张牌面朝下置于你的人物牌上",
	
	["#hana011"]="浅春的妖精",
	["hana011"]="莉莉•白",--花 - 空 - 3血
	["&hana011"]="莉莉",
	["designer:hana011"]="妒天のPAD",
	["illustrator:hana011"]="ゆたまろ",
	["cv:hana011"]="暂无",
	["thyachui"]="芽吹",
	[":thyachui"]="摸牌阶段开始时，你可放弃摸牌，改为将至多X张红色手牌交给一名其他角色，然后摸等量的牌（X为该角色已损失的体力值）。",
	["@thyachui"]="你可以发动“芽吹”",
	["~thyachui"]="选择若干张红色手牌→选择一名其他角色→点击确定",
	["thchunhen"]="春痕",
	[":thchunhen"]="每当一名角色因弃置而失去牌时，若其中有方块牌，你可以摸一张牌。",
	
	["#hana012"]="欢愉的遗忘之伞",
	["hana012"]="多多良小伞",--花 - 空 - 4血
	["designer:hana012"]="幻桜落",
	["illustrator:hana012"]="紅緒",
	["cv:hana012"]="暂无",
	["thxiagong"] = "遐攻",
	[":thxiagong"] = "若你的装备区没有武器牌，你可以对与你距离2以内的角色使用【杀】。",
	["thguaitan"]="怪谈",
	[":thguaitan"]="每当你造成伤害，在结算后，你可以选择一种牌的类别，受到伤害的角色不能使用或打出该类别的牌，直到其再次受到一次伤害后，或其下一个回合的回合结束。",
	["@guaitan_basic"]="怪谈(基本牌)",
	["@guaitan_equip"]="怪谈(装备牌)",
	["@guaitan_trick"]="怪谈(锦囊牌)",
	["#ThGuaitan"]="%from 选择了 %arg，%to 的下个回合无法使用或打出该类别的牌",
	["#ThGuaitanTrigger"]="%from 受“%arg2”影响，本回合无法使用或打出 %arg",
	
	["#hana013"]="忠实的死体",
	["hana013"]="宫古芳香",--花 - 空 - 3血
	["designer:hana013"]="昂翼天使",
	["illustrator:hana013"]="KS",
	["cv:hana013"]="暂无",
	["thhouzhi"]="后知",
	[":thhouzhi"]="锁定技，若你拥有技能“歃愈”，当你受到伤害时，你防止之，然后获得等量的“坚韧”标记。结束阶段开始时，你弃置全部的该标记并失去等量体力。",
	["thshayu"]="歃愈",
	[":thshayu"]="锁定技，每当你于出牌阶段造成一次伤害后，你须弃置1枚“坚韧”标记。",
	["@thshayu"]="你可以弃置一张牌发动“疗愈”",
	["thdujia"]="毒稼",
	[":thdujia"]="出牌阶段限一次，你可以弃置一张基本牌，并失去1点体力，然后摸三张牌。",
	["@jianren"]="坚韧",
	
	["#hana014"]="无理非道的仙人",
	["hana014"]="霍青娥",--花 - 幻 - 4血
	["designer:hana014"]="幻桜落",
	["illustrator:hana014"]="ぬぬっこ",
	["cv:hana014"]="暂无",
	["thxianfa"]="仙法",
	[":thxianfa"]="出牌阶段限一次，你可以失去1点体力或弃置一张牌，然后选择一名角色，并选择一个除出牌阶段外的阶段。若如此做，该角色于你此回合的弃牌阶段后额外进行一个该阶段。",
	["#ThXianfaChoose"]="%from 选择了 %arg 阶段",
	["#ThXianfaDo"]="%from 受到了“%arg”的影响，执行了一个额外的 %arg2 阶段",
	["thwendao"]="问道",
	[":thwendao"]="觉醒技，准备阶段开始时，若你没有手牌，你须回复1点体力或摸两张牌，然后减少1点体力上限，并获得技能“迷途”。",
	["#ThWendao"]="%from 没有手牌，触发“%arg”觉醒",
	["@wendao"]="问道",
	
	["#hana015"]="神明末裔的亡灵",
	["hana015"]="苏我屠自古",--花 - 空 - 3血
	["designer:hana015"]="幻桜落",
	["illustrator:hana015"]="いちやん",
	["cv:hana015"]="暂无",
	["thleishi"]="雷矢",
	[":thleishi"]="每当你对一名角色造成伤害，在结算后，若其体力值不大于1，你可以指定该角色与之距离最近的另一名角色，对其造成1点雷电伤害。",
	["@thleishi"]="你可以发动“雷矢”",
	["thshanling"]="闪灵",
	[":thshanling"]="准备阶段开始时，若你的体力值是全场最少的（或之一），你可以对你攻击范围内的一名角色造成1点雷电伤害。",
	["@thshanling"]="你可以发动“闪灵”",
	
	["#hana016"]="阴阳圣童女",
	["hana016"]="物部布都",--花 - 空 - 3血
	["designer:hana016"]="昂翼天使",
	["illustrator:hana016"]="ファルまろ",
	["cv:hana016"]="暂无",
	["thshijie"]="尸解",
	[":thshijie"]="每当你回复1点体力后，可以从牌堆顶亮出一张牌置于你的人物牌上，称为“皿”。你可以将一张“皿”当【三粒天滴】使用。",
	["shijiepile"]="皿",
	["thshengzhi"]="圣贽",
	[":thshengzhi"]="其他角色的回合开始时，你可弃置一张黑色手牌或“皿”并令该角色跳过此回合的一个阶段。若以此法跳过摸牌阶段，你失去1点体力；若以此法跳过出牌阶段，该角色回复1点体力。",
	["@thshengzhi"]="你可以发动“圣贽",
	["~thshengzhi"]="选择一张黑色手牌或“皿”→点击确定",
	
	["#hana017"]="圣德道士",
	["hana017"]="丰聪耳神子",--花 - 空 - 7血
	["designer:hana017"]="幻桜落",
	["illustrator:hana017"]="三日月沙羅",
	["cv:hana017"]="凉子",
	["~hana017"]="该来的，还是躲不...过...",
	["thzhaoyu"]="诏谕",
	[":thzhaoyu"]="一名角色的准备阶段开始时，你可以将一张牌置于牌堆顶。然后若其判定区有牌或装备区有武器牌，你可以从牌堆底摸一张牌。",
	["thzhaoyu-draw:draw"]="你可以从牌堆底摸一张牌",
	["@thzhaoyu"]="你可以发动“诏谕”",
	["$thzhaoyu1"]="奉天承运，天皇诏曰。",
	["$thzhaoyu2"]="还不速速领旨？",
	["thwuwu"]="无忤",
	[":thwuwu"]="锁定技，弃牌阶段开始时，你须弃置一张手牌。",
	["$thwuwu1"]="以和为贵，以和为贵呀。",
	["$thwuwu2"]="居庙堂者，当以无忤为宗。",
	["thrudao"]="入道",
	[":thrudao"]="觉醒技，准备阶段开始时，若你没有手牌，你须摸两张牌，然后减少3点体力上限，并失去技能“无忤”。",
	["#ThRudao"]="%from 没有手牌，触发“%arg”觉醒",
	["@rudao"]="入道",
	["$thrudao"]="长生，亦我所愿也。",
	
	["#hana018"]="非想非非想之女",
	["hana018"]="比那名居天子",--花 - 空 - 4血
	["designer:hana018"]="晓ャ绝对",
	["illustrator:hana018"]="国府田ハヤト",
	["cv:hana018"]="暂无",
	--["~hana018"]="铭志，天地...不仁...",
	["thliuzhen"]="六震",
	[":thliuzhen"]="出牌阶段限一次，当你使用【杀】指定目标后，可以选择任意名不为该【杀】目标的其他角色，该【杀】在结算后置入弃牌堆时，你无视距离对你所选的角色使用，其中每有一名角色使用【闪】抵消了该【杀】的效果，你须弃置一张牌或失去1点体力。",
	["@thliuzhen"]="你可以发动“六震”",
	["~thliuzhen"]="选择任意名不为该【杀】目标的其他角色→点击确定",
	["#ThLiuzhenMiss"]="%to 抵消了 %from 的【<font color=yellow><b>杀</b></font>】，触发“%arg”",
	["$thliuzhen1"]="铭志，念地运遐迩，肃诸天，非所及也。",
	["$thliuzhen2"]="铭志，相地命万千，含六合，其道穷也。",
	["thtianchan"]="天禅",
	[":thtianchan"]="君主技，当你处于濒死状态时，其他花势力的角色可以将黑桃牌当【桃】使用。",
	["thtianchanv"]="天禅",
	[":thtianchanv"]="君主处于濒死状态时，若你是花势力角色，你可以将黑桃牌当【桃】使用。",
	["$thtianchan1"]="铭志，囚封天之道，众生需渡无量劫。",
	["$thtianchan2"]="铭志，锁亡天之运，众生不得真道，常沉苦海。",
}