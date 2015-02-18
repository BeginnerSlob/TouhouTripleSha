-- translation for ChunxueScenario

return {
	["chunxue"] = "春雪异变之章",
	["#CxWake"] = "%from 触发 %arg 觉醒",

--[[
春雪舞·凛栗
灵　梦：赏桜的时节已经到了啊，为何外面还是大雪天呢？
魔理沙：已经５月了啊，因为太冷了，都没有发觉的说。
咲　夜：没想到今年冬天会持续这许久呢，暖房燃料用完的话，大小姐又要啰啰嗦嗦啦。
爱丽丝：春度被....？那个地方发生了什么吗？
琪露诺：呀哈，本小姐是最强哒！

锁定技，灵梦、魔理沙、咲夜、爱丽丝摸牌阶段摸牌时，少摸一张牌，直到"冥域扉"发生。
锁定技，幽幽子的技能"诱殇"和"满咲"无效，直到"墨染桜"发生。
]]
	["ChunXueWu"] = "春雪舞·凛栗",

	["$ChunXueWu1"] = "灵　梦：\n" .. 
					  "      赏桜的时节已经到了啊，\n" ..
					  "      为何外面还是大雪天呢？",
	["$ChunXueWu2"] = "魔理沙：\n" ..
					  "      已经５月了啊，\n" ..
					  "      因为太冷了，\n" ..
					  "      都没有发觉的说。",
	["$ChunXueWu3"] = "咲　夜：\n" ..
					  "      没想到今年冬天会持续这许久呢，\n" ..
					  "      暖房燃料用完的话，\n" ..
					  "      大小姐又要啰啰嗦嗦啦。",
	["$ChunXueWu4"] = "爱丽丝：\n" ..
					  "      春度被....？\n" ..
					  "      那个地方发生了什么吗？",
	["$ChunXueWu5"] = "琪露诺：\n" ..
					  "      呀哈，本小姐是最强哒！",

	["cxlinli"] = "凛栗",
	[":cxlinli"] = "锁定技，摸牌阶段摸牌时，你少摸一张牌。",
	["#CxLinli"] = "%from 受到 %arg2 的影响，少摸了 %arg 张牌",
	["cxlinli_lord"] = "凛栗",
	[":cxlinli_lord"] = "锁定技，你的技能\"<b>诱殇</b>\"和\"<b>满咲</b>\"无效。",

--[[
偷心盗·七煌
魔理沙：呀嚯，这不是爱丽丝嘛？我在异变退治中的说，爱丽丝也一起来嘛。
爱丽丝：哎？等....给我慢着！

觉醒技，魔理沙对爱丽丝使用【妙手探云】或发动"借物"时，爱丽丝摸一张牌，且身份牌改为反贼。
]]
	["TouXinDao"] = "偷心盗·七煌",

	["$TouXinDao1"] = "魔理沙：\n" .. 
					  "      呀嚯，这不是爱丽丝嘛？\n" .. 
					  "      我在异变退治中的说，\n" .. 
					  "      爱丽丝也一起来嘛。",
	["$TouXinDao2"] = "爱丽丝：\n" .. 
					  "      哎？等....给我慢着！",

	["cxqihuang"] = "七煌",
	[":cxqihuang"] = "觉醒技，魔理沙对你使用【妙手探云】或发动\"<b>借物</b>\"时，你摸一张牌，且身份牌改为反贼。",
	
--[[
冥域扉·絮曲
　橙　：欢迎来到迷途之家喵...呜咕！
咲　夜：黑幕发现，不过真弱啊。
灵　梦：据说这里的东西拿回去能带来好运？掠夺开始咯！
爱丽丝：....啊哈哈。
魔理沙：啊，厉害的结界发现啦！芝麻开门！
魔理沙：算了直接飞过去吧，话说刚才掉下去的三个幽灵一样的东西是什么的说？

觉醒技，若灵梦、魔理沙、咲夜、爱丽丝都使用牌指定过橙为目标后，除幽幽子和妖梦外的所有角色摸一张牌。
]]
	["MingYuFei"] = "冥域扉·絮曲",

	["$MingYuFei1"] = "　橙　：\n" .. 
					  "      欢迎来到迷途之家喵...\n" .. 
					  "      呜咕！",
	["$MingYuFei2"] = "咲　夜：\n" .. 
					  "      黑幕发现，\n" .. 
					  "      不过真弱啊。",
	["$MingYuFei3"] = "灵　梦：\n" .. 
					  "      据说这里的东西\n" .. 
					  "      拿回去能带来好运？\n" .. 
					  "      掠夺开始咯！",
	["$MingYuFei4"] = "爱丽丝：\n" .. 
					  "      ....啊哈哈。",
	["$MingYuFei5"] = "魔理沙：\n" .. 
					  "      啊，\n" .. 
					  "      厉害的结界发现啦！\n" .. 
					  "      芝麻开门！",
	["$MingYuFei6"] = "魔理沙：\n" .. 
					  "      算了直接飞过去吧，\n" .. 
					  "      话说刚才掉下去的\n" .. 
					  "      三个幽灵一样的东西\n" .. 
					  "      是什么的说？",

	["cxxuqu"] = "絮曲",
	[":cxxuqu"] = "觉醒技，若灵梦、魔理沙、咲夜、爱丽丝都使用牌指定过你为目标后，除幽幽子和妖梦外的所有角色摸一张牌。",
	
--[[
幽明法·求闻
妖　梦：真是些不速之客呢。这里是白玉楼，死者的居所里，生者可是会遭遇不幸的啊。
妖　梦：那么，正好把你们收集的春交出来吧！看剑！

限定技，"冥域扉"发生后，妖梦的出牌阶段，妖梦可以令其使用【杀】时无距离限制且无视防具，不计入每阶段的使用限制，且可以额外指定一个目标，直到回合结束。
]]
	["YouMingFa"] = "幽明法·求闻",
	
	["$YouMingFa1"] = "妖　梦：\n" .. 
					  "      真是些不速之客呢。\n" .. 
					  "      这里是白玉楼，\n" .. 
					  "      死者的居所里，\n" .. 
					  "      生者可是会遭遇不幸的啊。",
	["$YouMingFa2"] = "妖　梦：\n" .. 
					  "      那么，\n" .. 
					  "      正好把你们\n" .. 
					  "      收集的春交出来吧！\n" .. 
					  "      看剑！",
	
	["cxqiuwen"] = "求闻",
	[":cxqiuwen"] = "限定技，出牌阶段，你可以令自己使用【杀】时无距离限制且无视防具，不计入每阶段的使用限制，且可以额外指定一个目标，直到回合结束。",

--[[
空观剑·六净
妖　梦：有破绽！看招！空观剑，六根清净斩！

限定技，"冥域扉"发生后，当妖梦受到伤害时，可以将手牌补至四张，并转移此伤害给伤害来源，然后伤害来源将其人物牌翻面。
]]
	["KongGuanJian"] = "空观剑·六净",
	
	["$KongGuanJian1"] = "妖　梦：\n" .. 
					  "      有破绽！看招！\n" .. 
					  "      空观剑，\n" .. 
					  "      六根清净斩！",
	
	["cxliujing"] = "六净",
	[":cxliujing"] = "限定技，当你受到伤害时，可以将手牌补至四张，并转移此伤害给伤害来源，然后伤害来源将其人物牌翻面。",
	
--[[
墨染桜·亡我
幽幽子：快了，还差一点点，西行妖就会开始绽放了，一切尘封的谜都会被揭开。
幽幽子：所以说，亡骸聚集在一起才美丽，就像桜花一样，不是吗，各位？

觉醒技，"幽明法"发生后，幽幽子的准备阶段开始时，弃置全部的"桜咲"标记，然后获得技能"浮月"。
]]
	["MoRanYing"] = "墨染桜·亡我",
	
	["$MoRanYing1"] = "幽幽子：\n" .. 
					  "      快了，还差一点点，\n" .. 
					  "      西行妖就会开始绽放了，\n" .. 
					  "      一切尘封的谜都会被揭开。",
	["$MoRanYing2"] = "幽幽子：\n" .. 
					  "      所以说，\n" .. 
					  "      亡骸聚集在一起才美丽，\n" .. 
					  "      就像桜花一样，\n" .. 
					  "      不是吗，各位？",
					  
	["cxwangwo"] = "亡我",
	[":cxwangwo"] = "觉醒技，准备阶段开始时，你弃置全部的\"桜咲\"标记，然后获得技能\"<b>浮月</b>\"。",

--[[
仙狐宴·现临
　蓝　：作为式神，你离独当一面还差得远啊，橙。
　橙　：蓝大人喵！
　蓝　：你且退后，让我来会会这些无礼之徒罢。

觉醒技，"墨染桜"发生后，当橙进入濒死状态时，其人物改为八云蓝，保留原技能且获得技能"筹策"和"占筮"，手牌补至四张，失去１点体力上限，回复３点体力并重置之。
]]
	["XianHuYan"] = "仙狐宴·现临",

	["$XianHuYan1"] = "　蓝　：\n" .. 
					  "      作为式神，\n" .. 
					  "      你离独当一面\n" .. 
					  "      还差得远啊，橙。",
	["$XianHuYan2"] = "　橙　：\n" .. 
					  "      蓝大人喵！",
	["$XianHuYan3"] = "　蓝　：\n" .. 
					  "      你且退后，\n" .. 
					  "      让我来会会\n" .. 
					  "      这些无礼之徒罢。",
	
	["cxxianlin"] = "现临",
	[":cxxianlin"] = "觉醒技，当你进入濒死状态时，你的人物改为八云蓝，保留原技能且获得技能\"<b>筹策</b>\"和\"<b>占筮</b>\"，手牌补至四张，失去1点体力上限，回复3点体力并重置。",
	
--[[
迷津斩·永劫
妖　梦：怎么能....在这里！还差一点点....！啊啊啊！未来永劫斩！

觉醒技，"墨染桜"发生后，当妖梦进入濒死状态时，回复２点体力，然后获得技能"神杀"。
]]
	["MiJinZhan"] = "迷津斩·永劫",

	["$MiJinZhan1"] = "妖　梦：\n" .. 
					  "      怎么能....在这里！\n" ..
					  "      还差一点点....！\n" ..
					  "      啊啊啊！未来永劫斩！",
	
	["cxyongjie"] = "永劫",
	[":cxyongjie"] = "觉醒技，当你进入濒死状态时，你回复２点体力，然后获得技能\"<b>神杀</b>\"。",
}
