-- this script to store the basic configuration for game program itself
-- and it is a little different from config.ini

config = {
	version = "v8.9.0",
	resource_version = "v8.9.0.0",
	version_name = "0401",
	mod_name = "TouhouTripleSha",
	big_font = 56,
	small_font = 27,
	tiny_font = 18,
	kingdoms = {"kaze", "hana", "yuki", "tsuki", "kami"},
	kingdom_colors = {
		yuki = "#547998",
		hana = "#D0796C",
		kaze = "#4DB873",
		tsuki = "#8A807A",
		kami = "#96943D",
		rei = "#DB8EEB",
	},

	skill_type_colors = {
		compulsoryskill = "#0000FF",
		limitedskill = "#FF0000",
		wakeskill = "#800080",
		lordskill = "#FFA500",
		oppphskill = "#008000",
		lianwuskill = "#91ACD5",
		onlyskill = "#9ED600",
	},

	package_names = {
		"StandardCard",
		"Maneuvering",
		"Fantasy",
		--"GreenHandCard",
		--"New3v3Card",
		--"New3v3_2013Card",
		--"New1v1Card",

		"TouhouKaze",
		"TouhouHana",
		"TouhouYuki",
		"TouhouTsuki",
		"TouhouShin",
		"TouhouSP",
		"TouhouBangai",
		"TouhouKami",
		"TenshiReihou",
		"IkaiDo",
		"IkaiMoku",
		"IkaiSui",
		"IkaiKin",
		"IkaiKa",
		"Hulaopass",

		"Achievement"
	},

	hulao_generals = {
		"package:touhou-kaze",
		"package:touhou-hana",
		"package:touhou-yuki",
		"package:touhou-tsuki",
		"package:touhou-shin",
		"package:touhou-sp",
		"package:touhou-bangai",
		"package:touhou-kami",
		"package:ikai-do",
		"package:ikai-moku",
		"package:ikai-sui",
		"package:ikai-kin",
		"package:ikai-ka"
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
		"哦，看看这将面，我们死定了。",
		"已经是一具尸体。",
		"闪电即将进行一波输出。",
		"简直毫无灵性。",
		"牛批牛批，这波可以上电视。",
		"断杀美如画。",
		"这锅我背了。",
		"要是刘三刀同志在你这操作怕是又要被批判一番。",
		"这波稳了。",
		"辣鸡游戏，心态崩了。"
	},

	roles_ban = {
	},

	kof_ban = {
		"bangai007", "bangai008", "bangai009", "bloom001", "bloom007", "bloom008", "bloom013", "bloom014", "bloom015", "bloom016", "bloom019", "bloom020", "bloom021", "bloom022", "bloom024", "bloom029", "bloom030", "bloom032", "bloom033", "bloom036", "bloom041", "bloom043", "bloom048", "bloom050", "bloom052", "hana003", "hana007", "hana008", "hana011", "hana014", "hana018", "hana016", "kami001", "kami002", "kami003", "kami004", "kami005", "kami006", "kami007", "kami008", "kami009", "kami010", "kami011", "kami012", "kami013", "kami014", "kami015", "kami016", "kaze002", "kaze003", "kaze008", "kaze011", "kaze012", "kaze014", "kaze017", "shin003", "shin004", "shin007", "luna001", "luna004", "luna005", "luna007", "luna009", "luna013", "luna015", "luna019", "luna021", "luna024", "luna025", "luna026", "luna027", "luna028", "luna029", "luna031", "luna032", "luna033", "luna034", "luna040", "luna041", "luna043", "luna046", "luna049", "snow001", "snow003", "snow005", "snow009", "snow010", "snow011", "snow012", "snow014", "snow015", "snow017", "snow020", "snow021", "snow022", "snow023", "snow024", "snow026", "snow027", "snow028", "snow029", "snow030", "snow036", "snow037", "snow040", "snow041", "snow048", "snow051", "wind010", "wind012", "wind014", "wind016", "wind018", "wind022", "wind023", "wind024", "wind026", "wind029", "wind030", "wind034", "wind036", "wind041", "wind043", "wind046", "wind047", "wind048", "wind052", "wind053", "wind055", "shin014", "story002", "story003", "wind001", "wind005", "snow052", "snow058", sp002, sp008, sp010, sp011, sp016, "tsuki002", "tsuki003", "tsuki005", "tsuki006", "tsuki007", "tsuki010", "yuki009", "tsuki015", "tsuki016", "tsuki017", "tsuki018", "yuki010", "yuki011", "yuki013", "yuki015", "yuki016"
	},

	basara_ban = {
--[[		"dongzhuo",
		"zuoci",
		"shenzhugeliang",
		"shenlvbu",
		"bgm_lvmeng"]]
	},

	hegemony_ban = {
	},

	pairs_ban = {
--[[		"huatuo", "zuoci", "bgm_pangtong", "kof_nos_huatuo", "nos_huatuo",
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
		"nos_zhangchunhua+heg_luxun", "nos_zhangchunhua+nos_guanxingzhangbao",]]
	},
	
	cards_ban = {
		"KOF_Only,PurpleSong", "KOF_Only,FeintAttack"
	},
	
	couple_lord = "hana002",
	couple_couples = {
		"kaze008+kaze009",
		"kaze018+kaze017",
		"yuki009+yuki018",
		"yuki004+tsuki015",
		"hana006+hana007",
		"hana009+hana003",
		"hana013+hana014",
		"tsuki005+yuki005",
		"tsuki003+tsuki004",
		"luna021+wind015",
		"wind001+wind007",
		"bloom039+bloom007",
		"wind033+wind023",
		"wind006+wind044",
		"luna049+luna032",
		"bloom048+wind040",
		"snow007+luna012",
		"bloom004+bloom045",
		"wind026+snow037",
		"luna014+snow017",
		"luna004+bloom022",
		"snow036+snow020",
		"wind011+luna043",
		"luna007+snow008",
		"snow051+luna003",
		"snow049+bloom018",
		"wind018+snow047"
	},

	convert_pairs = {
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
--[[		"boss_chi+boss_mei+boss_wang+boss_liang",
		"boss_niutou+boss_mamian",
		"boss_heiwuchang+boss_baiwuchang",
		"boss_luocha+boss_yecha"]]
	},

	bossmode_endless_skills = {
--[[		"bossguimei", "bossdidong", "nosenyuan", "bossshanbeng+bossbeiming+huilei+bossmingbao",
		"bossluolei", "bossguihuo", "bossbaolian", "mengjin", "bossmanjia+bazhen",
		"bossxiaoshou", "bossguiji", "fankui", "bosslianyu", "nosjuece",
		"bosstaiping+shenwei", "bosssuoming", "bossxixing", "bossqiangzheng",
		"bosszuijiu", "bossmodao", "bossqushou", "yizhong", "kuanggu",
		"bossmojian", "bossdanshu", "shenji", "wushuang", "wansha"]]
	},

	bossmode_exp_skills = {
--[[		"mashu:15",
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
		"zhiheng:90",]]
	},

	jiange_defense_kingdoms = {
--[[		loyalist = "shu",
		rebel = "wei",]]
	},

	jiange_defense_machine = {
--[[		wei = "jg_machine_tuntianchiwen+jg_machine_shihuosuanni+jg_machine_fudibian",
		shu = "jg_machine_yunpingqinglong+jg_machine_jileibaihu",]]
	},

	jiange_defense_soul = {
--[[		wei = "jg_soul_caozhen+jg_soul_simayi",
		shu = "jg_soul_liubei+jg_soul_zhugeliang",]]
	},

	members = {
		"&nbsp;&nbsp;幻桜落(小巫女莎莎、岚兮雨汐)<br/>"..
		"&nbsp;&nbsp;昂翼天使(heerowww)<br/>"..
		"&nbsp;&nbsp;女王受·虫(Slob)<br/>"..
		"&nbsp;&nbsp;狈耳萌特(萌特)<br/>"..
		"&nbsp;&nbsp;广寒冰焰(IceFlame)<br/>"..
		"&nbsp;&nbsp;nonescarlet(寒极)<br/>"..
		"&nbsp;&nbsp;Leo<br/>"..
		"&nbsp;&nbsp;莎墨<br/>"..
		"&nbsp;&nbsp;同声异谱<br/>"..
		"&nbsp;&nbsp;名和行年"
	}
}