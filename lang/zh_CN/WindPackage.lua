-- translation for WindPackage

return {
	["wind"] = "风包",

	["#caoren"] = "大将军",
	["caoren"] = "曹仁",
	["jushou"] = "据守",
	[":jushou"] = "回合结束阶段开始时，你可以摸三张牌，然后将你的武将牌翻面。",
	["jushou:yes"] = "摸三张牌，然后将你的武将翻面",

	["#yuji"] = "太平道人",
	--["yuji"] = "于吉",
	["illustrator:yuji"] = "LiuHeng",
	["guhuo"] = "蛊惑",
	[":guhuo"] = "你可以说出一张基本牌或非延时类锦囊牌的名称，并背面朝上使用或打出一张手牌。若无其他角色质疑，则亮出此牌并按你所述之牌结算。若有其他角色质疑则亮出验明：若为真，质疑者各失去1点体力；若为假，质疑者各摸一张牌。除非被质疑的牌为红桃且为真，此牌仍然进行结算，否则无论真假，将此牌置入弃牌堆。\
◆当前的体力值为0的角色不能质疑。",
	["guhuo_pile"] = "蛊惑牌堆",
	["#Guhuo"] = "%from 发动【%arg2】，据说这张牌是 %arg，选择的目标为 %to",
	["#GuhuoNoTarget"] = "%from 发动【%arg2】，据说这张牌是 %arg",
	["#GuhuoCannotQuestion"] = "%from 体力不支(当前体力值=%arg)，无法质疑",
	["guhuo:question"] = "质疑",
	["guhuo:noquestion"] = "不质疑",
	["question"] = "质疑",
	["noquestion"] = "不质疑",
	["#GuhuoQuery"] = "%from 表示 %arg",
	["$GuhuoResult"] = "%from 用来蛊惑的牌是 %card",
	["guhuo-saveself"] = "自救蛊惑",
	["guhuo-saveself:peach"] = "桃子",
	["guhuo-saveself:analeptic"] = "酒",

-- guhuo dialog
	["single_target"] = "单目标锦囊",
	["multiple_targets"] = "多目标锦囊",
}
