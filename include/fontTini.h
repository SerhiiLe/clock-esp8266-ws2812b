// крошечные и уродливые шрифты, зато можно выводить не 5.5 символа, а 8.5 ...
const uint8_t fontTiny[][3] PROGMEM = {
	 //a-z
	{0, 0, 0},		// space 0 0x20
	{0x1E, 0x09, 0x1F},	// a 1 0x41
	{0x1F, 0x15, 0x0A},	// b 2
	{0x1F, 0x11, 0x11},	// c 3
	{0x1F, 0x11, 0x0E},	// d 4
	{0x1F, 0x15, 0x11},	// e 5
	{0x1F, 0x05, 0x01},	// f 6
	{0x1F, 0x11, 0x1D},	// g 7
	{0x1F, 0x04, 0x1F},	// h 8
	{0x11, 0x1F, 0x11},	// i 9
	{0x18, 0x10, 0x1F},	// j 10
	{0x1F, 0x04, 0x1B},	// k 11
	{0x1F, 0x10, 0x10},	// l 12
	{0x1F, 0x02, 0x1F},	// m 13
	{0x1F, 0x01, 0x1E},	// n 14
	{0x1F, 0x11, 0x1F},	// o 15
	{0x1F, 0x05, 0x07},	// p 16
	{0x07, 0x05, 0x1F},	// q 17
	{0x1F, 0x0D, 0x17},	// r 18
	{0x17, 0x15, 0x1D},	// s 19
	{0x01, 0x1F, 0x01},	// t 20
	{0x1F, 0x10, 0x1F},	// u 21
	{0x0F, 0x10, 0x0F},	// v 22
	{0x1F, 0x08, 0x1F},	// w 23
	{0x1B, 0x04, 0x1B},	// x 24
	{0x07, 0x1C, 0x07},	// y 25
	{0x19, 0x15, 0x13},	// z 26 0x5a

	{0x17, 0x00, 0x00}, // ! 27 0x21
	{0x03, 0x00, 0x03}, // " 28
	{0x0A, 0x1F, 0x0A}, // # 29
	{0x04, 0x0A, 0x04}, // $ 30
	{0x09, 0x04, 0x12}, // % 31
	{0x0F, 0x1F, 0x08}, // & 32
	{0x03, 0x00, 0x00}, // ' 33
	{0x0E, 0x11, 0x00}, // ( 34
	{0x11, 0x0E, 0x00}, // ) 35
	{0x0A, 0x04, 0x0A}, // * 36
	{0x04, 0x0E, 0x04}, // + 37
	{0x18, 0x00, 0x00}, // , 38
	{0x04, 0x04, 0x04}, // - 39
	{0x10, 0x00, 0x00},	// . 40
	{0x18, 0x04, 0x03}, // / 41 0x2f

	{0x1F, 0x11, 0x1F},	// 0 42 0x30
	{0x00, 0x1F, 0x00},	// 1 43
	{0x1D, 0x15, 0x17},	// 2 44
	{0x11, 0x15, 0x1F},	// 3 45
	{0x07, 0x04, 0x1F},	// 4 46
	{0x17, 0x15, 0x1D},	// 5 47
	{0x1F, 0x15, 0x1D},	// 6 48
	{0x01, 0x01, 0x1F},	// 7 49
	{0x1F, 0x15, 0x1F},	// 8 50
	{0x17, 0x15, 0x1F},	// 9 51 0x39

	{0x0A, 0x00, 0x00},	// : 52 0x3a
	{0x1A, 0x00, 0x00},	// ; 53
	{0x04, 0x0A, 0x11},	// < 54
	{0x0A, 0x0A, 0x0A}, // = 55
	{0x11, 0x0A, 0x04}, // > 56
	{0x01, 0x15, 0x07},	// ? 57
	{0x1F, 0x17, 0x17}, // @ 58 0x40
	
	{0x1E, 0x09, 0x1F}, // А 59 0xd090
	{0x1F, 0x15, 0x1D}, // Б 60
	{0x1F, 0x15, 0x0A},	// В 61
	{0x1F, 0x01, 0x01}, // Г 62
	{0x18, 0x0F, 0x1F}, // Д 63
	{0x1F, 0x15, 0x11}, // Е 64
	{0x1B, 0x0E, 0x1B}, // Ж 65
	{0x11, 0x15, 0x1F},	// З 66
	{0x0E, 0x10, 0x1E}, // И 67
	{0x0C, 0x11, 0x1C}, // Й 68
	{0x1F, 0x04, 0x1B},	// К 69
	{0x1C, 0x02, 0x1F}, // Л 70
	{0x1F, 0x02, 0x1F}, // М 71
	{0x1F, 0x04, 0x1F}, // Н 72
	{0x1F, 0x11, 0x1F},	// О 73
	{0x1F, 0x01, 0x1F}, // П 74
	{0x1F, 0x05, 0x07},	// Р 75
	{0x1F, 0x11, 0x11},	// С 76
	{0x01, 0x1F, 0x01},	// Т 77
	{0x17, 0x14, 0x1F},	// У 78
	{0x0E, 0x1F, 0x0E},	// Ф 79
	{0x1B, 0x04, 0x1B},	// Х 80
	{0x0F, 0x08, 0x1F},	// Ц 81
	{0x07, 0x04, 0x1F},	// Ч 82
	{0x1F, 0x1E, 0x1F},	// Ш 83
	{0x0F, 0x0E, 0x1F},	// Щ 84
	{0x01, 0x1F, 0x1C},	// Ъ 85
	{0x1F, 0x14, 0x1F},	// Ы 86
	{0x1F, 0x14, 0x1C}, // Ь 87
	{0x15, 0x15, 0x1F},	// Э 88
	{0x1F, 0x0E, 0x11},	// Ю 89
	{0x17, 0x0D, 0x1F},	// Я 90 0xd0af

	{0x0E, 0x15, 0x11},	// Є 91 0xd084
	{0x11, 0x1E, 0x11},	// Ї 92 0xd087
	{0x1E, 0x02, 0x03}, // Ґ 93 0xd290

	{0x07, 0x05, 0x07}, // ° 94 0xb0 171
};

// костыль для отрисовки буквы Ю, единственной которая ну совсем не лезет в три пикселя ширины
const uint8_t fontTinyU[] PROGMEM = {0x0e};