-- this script to store the basic configuration for game program itself
-- and it is a little different from config.ini

config = {
	version = "20140601",
	version_name = "V2",
	mod_name = "Para",
	big_font = 56,
	small_font = 27,
	tiny_font = 18,
	kingdoms = { "kaze", "hana", "yuki", "tsuki", "kami"},
	kingdom_colors = {
		yuki = "#547998",
		hana = "#D0796C",
		kaze = "#4DB873",
		tsuki = "#8A807A",
		kami = "#96943D",
	},

	skill_type_colors = {
		compulsoryskill = "#0000FF",
		limitedskill = "#FF0000",
		wakeskill = "#800080",
		lordskill = "#FFA500",
		oppphskill = "#008000",
	},

	package_names = {
		"StandardCard",
		"StandardExCard",
		"Maneuvering",
		"LimitationBroken",
		"SPCard",
		"Nostalgia",
		"GreenHandCard",
		"New3v3Card",
		"New3v3_2013Card",
		"New1v1Card",

		"TouhouKaze",
		"TouhouHana",
		"TouhouYuki",
		"TouhouTsuki",
		"TouhouKishin",
		"TouhouSP",
		"TouhouBangai",
		"TouhouKami",
		"IkaiTsuchi",
		--"IkaiHi",
		--"IkaiKin",
		--"IkaiSui",
		--"IkaiKi",
		"Standard",
		"Wind",
		"Fire",
		"Thicket",
		"Mountain",
		"God",
		"YJCM",
		"YJCM2012",
		"YJCM2013",
		"YJCM2014",
		"Assassins",
		"Special3v3",
		"Special3v3Ext",
		"Special1v1",
		"Special1v1Ext",
		"SP",
		"OL",
		"TaiwanSP",
		"Miscellaneous",
		"BGM",
		"BGMDIY",
		"Ling",
		"Hegemony",
		"HFormation",
		"HMomentum",
		"HegemonySP",
		"NostalStandard",
		"NostalWind",
		"NostalYJCM",
		"NostalYJCM2012",
		"NostalYJCM2013",
		"NostalGeneral",
		"Test"
	},

	hulao_generals = {
		"package:nostal_standard",
		"package:wind",
		"package:nostal_wind",
		"zhenji", "zhugeliang", "sunquan", "sunshangxiang",
		"-zhangjiao", "-zhoutai", "-caoren", "-yuji",
		"-nos_yuji"
	},

	xmode_generals = {
		"package:nostal_standard",
		"package:wind",
		"package:fire",
		"package:nostal_wind",
		"zhenji", "zhugeliang", "sunquan", "sunshangxiang",
		"-nos_huatuo",
		"-zhangjiao", "-zhoutai", "-caoren", "-yuji",
		"-nos_zhangjiao", "-nos_yuji"
	},

	easy_text = {
		"太慢了，做两个俯卧撑吧！",
		"快点吧，我等的花儿都谢了！",
		"高，实在是高！",
		"好手段，可真不一般啊！",
		"哦，太菜了。水平有待提高。",
		"你会不会玩啊？！",
		"嘿，一般人，我不使这招。",
		"呵，好牌就是这么打地！",
		"杀！神挡杀神！佛挡杀佛！",
		"你也忒坏了吧？！"
	},

	roles_ban = {
	},

	kof_ban = {
		"sunquan",
	},

	basara_ban = {
		"dongzhuo",
		"zuoci",
		"shenzhugeliang",
		"shenlvbu",
		"bgm_lvmeng"
	},

	pairs_ban = {
		"huatuo", "zuoci", "bgm_pangtong", "kof_nos_huatuo", "nos_huatuo",
		"simayi+zhenji", "simayi+dengai",
		"xiahoudun+luxun", "xiahoudun+zhurong", "xiahoudun+zhangchunhua", "xiahoudun+nos_luxun", "xiahoudun+nos_zhangchunhua",
		"caoren+shenlvbu", "caoren+caozhi", "caoren+bgm_diaochan", "caoren+bgm_caoren", "caoren+nos_caoren",
		"guojia+dengai",
		"zhenji+zhangjiao", "zhenji+shensimayi", "zhenji+zhugejin", "zhenji+nos_simayi", "zhenji+nos_zhangjiao", "zhenji+nos_wangyi",
		"zhanghe+yuanshu",
		"dianwei+weiyan",
		"dengai+zhangjiao", "dengai+shensimayi", "dengai+zhugejin", "dengai+nos_simayi", "dengai+nos_guojia", "dengai+nos_zhangjiao",
		"zhangfei+zhangchunhua", "zhangfei+nos_huanggai", "zhangfei+nos_zhangchunhua",
		"zhugeliang+xushu", "zhugeliang+nos_xushu",
		"huangyueying+wolong", "huangyueying+ganning", "huangyueying+yuanshao", "huangyueying+yanliangwenchou", "huangyueying+nos_huanggai",
		"huangzhong+xusheng",
		"weiyan+nos_huanggai",
		"wolong+luxun", "wolong+zhangchunhua", "wolong+nos_huangyueying", "wolong+nos_luxun", "wolong+nos_zhangchunhua",
		"menghuo+dongzhuo", "menghuo+zhugedan", "menghuo+heg_dongzhuo",
		"sunquan+sunshangxiang",
		"ganning+nos_huangyueying",
		"lvmeng+yuanshu",
		"huanggai+nos_huanggai",
		"luxun+yanliangwenchou", "luxun+guanxingzhangbao", "luxun+guanping", "luxun+guyong",
		    "luxun+nos_liubei", "luxun+nos_yuji", "luxun+nos_guanxingzhangbao",
		"sunshangxiang+shensimayi", "sunshangxiang+heg_luxun", "sunshangxiang+nos_huanggai",
		"sunce+guanxingzhangbao", "sunce+nos_guanxingzhangbao",
		"xiaoqiao+zhangchunhua", "xiaoqiao+nos_zhangchunhua",
		"yuanshao+nos_huangyueying", "yuanshao+nos_huanggai",
		"yanliangwenchou+zhangchunhua", "yanliangwenchou+nos_huangyueying", "yanliangwenchou+nos_huanggai", "yanliangwenchou+nos_luxun",
		    "yanliangwenchou+nos_zhangchunhua",
		"dongzhuo+shenzhaoyun", "dongzhuo+wangyi", "dongzhuo+diy_wangyuanji", "dongzhuo+nos_huanggai", "dongzhuo+nos_zhangchunhua", "dongzhuo+nos_wangyi",
		"st_huaxiong+nos_huanggai",
		"shencaocao+caozhi",
		"shenlvbu+caozhi", "shenlvbu+liaohua", "shenlvbu+bgm_diaochan", "shenlvbu+bgm_caoren", "shenlvbu+nos_caoren",
		"shenzhaoyun+huaxiong", "shenzhaoyun+zhugedan", "shenzhaoyun+heg_dongzhuo",
		"caozhi+bgm_diaochan", "caozhi+bgm_caoren", "caozhi+nos_caoren",
		"gaoshun+zhangchunhua", "gaoshun+nos_zhangchunhua",
		"wuguotai+zhangchunhua", "wuguotai+caochong", "wuguotai+nos_huanggai", "wuguotai+nos_zhangchunhua", "wuguotai+nos_caochong",
		"zhangchunhua+guanxingzhangbao", "zhangchunhua+guanping", "zhangchunhua+guyong", "zhangchunhua+xiahouba", "zhangchunhua+zhugeke",
		    "zhangchunhua+heg_luxun", "zhangchunhua+neo_zhangfei", "zhangchunhua+nos_liubei", "zhangchunhua+nos_zhangfei",
		    "zhangchunhua+nos_yuji", "zhangchunhua+nos_guanxingzhangbao",
		"guanxingzhangbao+bgm_zhangfei", "guanxingzhangbao+heg_sunce", "guanxingzhangbao+nos_huanggai", "guanxingzhangbao+nos_luxun", "guanxingzhangbao+nos_zhangchunhua",
		"huaxiong+nos_huanggai",
		"liaohua+bgm_diaochan",
		"wangyi+zhugedan", "wangyi+heg_dongzhuo",
		"guanping+nos_luxun", "guanping+nos_zhangchunhua",
		"guyong+nos_luxun", "guyong+nos_zhangchunhua",
		"yuanshu+nos_lvmeng",
		"xiahouba+nos_huanggai", "xiahouba+nos_zhangchunhua",
		"zhugedan+diy_wangyuanji", "zhugedan+nos_zhangchunhua", "zhugedan+nos_wangyi",
		"zhugeke+nos_zhangchunhua",
		"bgm_diaochan+bgm_caoren", "bgm_diaochan+nos_caoren",
		"bgm_caoren+nos_caoren",
		"bgm_zhangfei+nos_guanxingzhangbao",
		"diy_wangyuanji+heg_dongzhuo",
		"hetaihou+nos_zhuran",
		"heg_sunce+nos_guanxingzhangbao",
		"heg_dongzhuo+nos_zhangchunhua", "heg_dongzhuo+nos_wangyi",
		"neo_zhangfei+nos_huanggai", "neo_zhangfei+nos_zhangchunhua",
		"nos_liubei+nos_luxun", "nos_liubei+nos_zhangchunhua",
		"nos_zhangfei+nos_huanggai", "nos_zhangfei+nos_zhangchunhua",
		"nos_huangyueying+nos_huanggai",
		"nos_huanggai+nos_guanxingzhangbao",
		"nos_luxun+nos_yuji", "nos_luxun+nos_guanxingzhangbao",
		"nos_yuji+nos_zhangchunhua",
		"nos_zhangchunhua+heg_luxun", "nos_zhangchunhua+nos_guanxingzhangbao",
	},
	
	couple_lord = "caocao",
	couple_couples = {
		"caopi|caozhi+zhenji",
		"simayi|shensimayi+zhangchunhua",
		"diy_simazhao+diy_wangyuanji",
		"liubei|bgm_liubei+ganfuren|mifuren|sp_sunshangxiang",
		"liushan+xingcai",
		"zhangfei|bgm_zhangfei+xiahoushi",
		"zhugeliang|wolong|shenzhugeliang+huangyueying",
		"menghuo+zhurong",
		"zhouyu|shenzhouyu+xiaoqiao",
		"lvbu|shenlvbu+diaochan|bgm_diaochan",
		"sunjian+wuguotai",
		"sunce|heg_sunce+daqiao|bgm_daqiao",
		"sunquan+bulianshi",
		"liuxie|diy_liuxie+fuhuanghou",
		"luxun|heg_luxun+sunru",
		"liubiao+caifuren",
	},

	convert_pairs = {
		"caiwenji->sp_caiwenji",
		"caopi->heg_caopi",
		"dingfeng->sp_dingfeng",
		"fazheng->ol_fazheng",
		"guanxingzhangbao->ol_guanxingzhangbao",
		"jiaxu->sp_jiaxu",
		"liubei->tw_liubei",
		"madai->heg_madai|ol_madai",
		"nos_caocao->tw_caocao",
		"nos_daqiao->wz_daqiao|tw_daqiao",
		"nos_diaochan->sp_diaochan|heg_diaochan|tw_diaochan",
		"nos_ganning->tw_ganning",
		"nos_guanyu->tw_guanyu",
		"nos_guojia->tw_guojia",
		"nos_huanggai->tw_huanggai",
		"nos_huangyueying->heg_huangyueying|tw_huangyueying",
		"nos_luxun->tw_luxun",
		"nos_lvbu->heg_lvbu|tw_lvbu",
		"nos_lvmeng->tw_lvmeng",
		"nos_machao->sp_machao|tw_machao",
		"nos_simayi->tw_simayi|pr_nos_simayi",
		"nos_xiahoudun->tw_xiahoudun",
		"nos_xuchu->tw_xuchu",
		"nos_zhangfei->tw_zhangfei",
		"nos_zhangliao->tw_zhangliao",
		"nos_zhaoyun->tw_zhaoyun",
		"nos_zhouyu->heg_zhouyu|sp_heg_zhouyu|tw_zhouyu",
		"panfeng->sp_panfeng",
		"pangde->sp_pangde",
		"shencaocao->pr_shencaocao",
		"shenlvbu->sp_shenlvbu",
		"sunquan->tw_sunquan",
		"sunshangxiang->sp_sunshangxiang|tw_sunshangxiang",
		"wangyi->ol_wangyi",
		"xiaoqiao->wz_xiaoqiao|heg_xiaoqiao|sp_heg_xiaoqiao|tw_xiaoqiao",
		"xushu->ol_xushu",
		"yuanshu->tw_yuanshu",
		"yuejin->sp_yuejin",
		"zhenji->sp_zhenji|heg_zhenji|tw_zhenji",
		"zhugeke->diy_zhugeke",
		"zhugeliang->heg_zhugeliang|tw_zhugeliang",
		"zhugejin->sp_zhugejin"
	},

	removed_hidden_generals = {
	},

	extra_hidden_generals = {
	},

	removed_default_lords = {
	},

	extra_default_lords = {
	},

	bossmode_default_boss = {
		"boss_chi+boss_mei+boss_wang+boss_liang",
		"boss_niutou+boss_mamian",
		"boss_heiwuchang+boss_baiwuchang",
		"boss_luocha+boss_yecha"
	},

	bossmode_endless_skills = {
		"bossguimei", "bossdidong", "nosenyuan", "bossshanbeng+bossbeiming+huilei+bossmingbao",
		"bossluolei", "bossguihuo", "bossbaolian", "mengjin", "bossmanjia+bazhen",
		"bossxiaoshou", "bossguiji", "fankui", "bosslianyu", "nosjuece",
		"bosstaiping+shenwei", "bosssuoming", "bossxixing", "bossqiangzheng",
		"bosszuijiu", "bossmodao", "bossqushou", "yizhong", "kuanggu",
		"bossmojian", "bossdanshu", "shenji", "wushuang", "wansha"
	},

	bossmode_exp_skills = {
		"mashu:15",
		"tannang:25",
		"yicong:25",
		"feiying:30",
		"yingyang:30",
		"zhenwei:40",
		"nosqicai:40",
		"nosyingzi:40",
		"zongshi:40",
		"qicai:45",
		"wangzun:45",
		"yingzi:50",
		"kongcheng:50",
		"nosqianxun:50",
		"weimu:50",
		"jie:50",
		"huoshou:50",
		"hongyuan:55",
		"dangxian:55",
		"xinzhan:55",
		"juxiang:55",
		"wushuang:60",
		"xunxun:60",
		"zishou:60",
		"jingce:60",
		"shengxi:60",
		"zhichi:60",
		"bazhen:60",
		"yizhong:65",
		"jieyuan:70",
		"mingshi:70",
		"tuxi:70",
		"guanxing:70",
		"juejing:75",
		"jiangchi:75",
		"bosszuijiu:80",
		"shelie:80",
		"gongxin:80",
		"fenyong:85",
		"kuanggu:85",
		"yongsi:90",
		"zhiheng:90",
	}
}