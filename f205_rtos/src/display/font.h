/**
 *****************************************************************************
 ** Kommunik�ci�s m�r�s - font.h
 ** Karakterek k�pe a grafikus LCD-n
 *****************************************************************************
 */

#ifndef FONT_H_
#define FONT_H_

//Size: 5x7
char fontdata[] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, // SPACE	0
	0x00, 0x00, 0x5F, 0x00, 0x00, // !		1
	0x00, 0x03, 0x00, 0x03, 0x00, // "		2
	0x14, 0x3E, 0x14, 0x3E, 0x14, // #		3
	0x24, 0x2A, 0x7F, 0x2A, 0x12, // $		4
	0x43, 0x33, 0x08, 0x66, 0x61, // %		5
	0x36, 0x49, 0x55, 0x22, 0x50, // &		6
	0x00, 0x05, 0x03, 0x00, 0x00, // '		7
	0x00, 0x1C, 0x22, 0x41, 0x00, // (		8
	0x00, 0x41, 0x22, 0x1C, 0x00, // )		9
	0x14, 0x08, 0x3E, 0x08, 0x14, // *		10
	0x08, 0x08, 0x3E, 0x08, 0x08, // +		11
	0x00, 0x50, 0x30, 0x00, 0x00, // ,		12
	0x08, 0x08, 0x08, 0x08, 0x08, // -		13
	0x00, 0x60, 0x60, 0x00, 0x00, // .		14
	0x20, 0x10, 0x08, 0x04, 0x02, // /		15
	0x3E, 0x51, 0x49, 0x45, 0x3E, // 0		16
	0x00, 0x04, 0x02, 0x7F, 0x00, // 1		17
	0x42, 0x61, 0x51, 0x49, 0x46, // 2		18
	0x22, 0x41, 0x49, 0x49, 0x36, // 3		19
	0x18, 0x14, 0x12, 0x7F, 0x10, // 4		20
	0x27, 0x45, 0x45, 0x45, 0x39, // 5		21
	0x3E, 0x49, 0x49, 0x49, 0x32, // 6		22
	0x01, 0x01, 0x71, 0x09, 0x07, // 7		23
	0x36, 0x49, 0x49, 0x49, 0x36, // 8		24
	0x26, 0x49, 0x49, 0x49, 0x3E, // 9		25
	0x00, 0x36, 0x36, 0x00, 0x00, // :		26
	0x00, 0x56, 0x36, 0x00, 0x00, // ;		27
	//0x08, 0x14, 0x22, 0x41, 0x00, // <		28
	0x08, 0x1C, 0x3E, 0x7F, 0x00, // <		28
	0x14, 0x14, 0x14, 0x14, 0x14, // =		29
	//0x00, 0x41, 0x22, 0x14, 0x08, // >		30
	0x00, 0x7F, 0x3E, 0x1C, 0x08, // >		30
	0x02, 0x01, 0x51, 0x09, 0x06, // ?		31
	0x3E, 0x41, 0x59, 0x55, 0x5E, // @		32
	0x7E, 0x09, 0x09, 0x09, 0x7E, // A		33
	0x7F, 0x49, 0x49, 0x49, 0x36, // B		34
	0x3E, 0x41, 0x41, 0x41, 0x22, // C		35
	0x7F, 0x41, 0x41, 0x41, 0x3E, // D		36
	0x7F, 0x49, 0x49, 0x49, 0x41, // E		37
	0x7F, 0x09, 0x09, 0x09, 0x01, // F		38
	0x3E, 0x41, 0x41, 0x49, 0x3A, // G		39
	0x7F, 0x08, 0x08, 0x08, 0x7F, // H		40
	0x00, 0x41, 0x7F, 0x41, 0x00, // I		41
	0x30, 0x40, 0x40, 0x40, 0x3F, // J		42
	0x7F, 0x08, 0x14, 0x22, 0x41, // K		43
	0x7F, 0x40, 0x40, 0x40, 0x40, // L		44
	0x7F, 0x02, 0x0C, 0x02, 0x7F, // M		45
	0x7F, 0x02, 0x04, 0x08, 0x7F, // N		46
	0x3E, 0x41, 0x41, 0x41, 0x3E, // O		47
	0x7F, 0x09, 0x09, 0x09, 0x06, // P		48
	0x1E, 0x21, 0x21, 0x21, 0x5E, // Q		49
	0x7F, 0x09, 0x09, 0x09, 0x76, // R		50
	0x26, 0x49, 0x49, 0x49, 0x32, // S		51
	0x01, 0x01, 0x7F, 0x01, 0x01, // T		52
	0x3F, 0x40, 0x40, 0x40, 0x3F, // U		53
	0x1F, 0x20, 0x40, 0x20, 0x1F, // V		54
	0x7F, 0x20, 0x10, 0x20, 0x7F, // W		55
	0x41, 0x22, 0x1C, 0x22, 0x41, // X		56
	0x07, 0x08, 0x70, 0x08, 0x07, // Y		57
	0x61, 0x51, 0x49, 0x45, 0x43, // Z		58
	0x00, 0x7F, 0x41, 0x00, 0x00, // [		59
	0x02, 0x04, 0x08, 0x10, 0x20, // slash 60
	0x00, 0x00, 0x41, 0x7F, 0x00, // ]		61
	0x04, 0x02, 0x01, 0x02, 0x04, // ^		62
	0x40, 0x40, 0x40, 0x40, 0x40, // _		63
	0x00, 0x01, 0x02, 0x04, 0x00, // `		64
	0x20, 0x54, 0x54, 0x54, 0x78, // a		65
	0x7F, 0x44, 0x44, 0x44, 0x38, // b		66
	0x38, 0x44, 0x44, 0x44, 0x44, // c		67
	0x38, 0x44, 0x44, 0x44, 0x7F, // d		68
	0x38, 0x54, 0x54, 0x54, 0x18, // e		69
	0x04, 0x04, 0x7E, 0x05, 0x05, // f		70
	0x08, 0x54, 0x54, 0x54, 0x3C, // g		71
	0x7F, 0x08, 0x04, 0x04, 0x78, // h		72
	0x00, 0x44, 0x7D, 0x40, 0x00, // i		73
	0x20, 0x40, 0x44, 0x3D, 0x00, // j		74
	0x7F, 0x10, 0x28, 0x44, 0x00, // k		75
	0x00, 0x41, 0x7F, 0x40, 0x00, // l		76
	0x7C, 0x04, 0x78, 0x04, 0x78, // m		77
	0x7C, 0x08, 0x04, 0x04, 0x78, // n		78
	0x38, 0x44, 0x44, 0x44, 0x38, // o		79
	0x7C, 0x14, 0x14, 0x14, 0x08, // p		80
	0x08, 0x14, 0x14, 0x14, 0x7C, // q		81
	0x00, 0x7C, 0x08, 0x04, 0x04, // r		82
	0x48, 0x54, 0x54, 0x54, 0x20, // s		83
	0x04, 0x04, 0x3F, 0x44, 0x44, // t		84
	0x3C, 0x40, 0x40, 0x20, 0x7C, // u		85
	0x1C, 0x20, 0x40, 0x20, 0x1C, // v		86
	0x3C, 0x40, 0x30, 0x40, 0x3C, // w		87
	0x44, 0x28, 0x10, 0x28, 0x44, // x		88
	0x0C, 0x50, 0x50, 0x50, 0x3C, // y		89
	0x44, 0x64, 0x54, 0x4C, 0x44, // z		90
	0x00, 0x08, 0x36, 0x41, 0x41, // {		91
	0x00, 0x00, 0x7F, 0x00, 0x00, // |		92
	0x41, 0x41, 0x36, 0x08, 0x00, // }		93
	0x02, 0x01, 0x02, 0x04, 0x02, // ~		94
	0x38, 0x54, 0x56, 0x55, 0x18, // �		95
	0x38, 0x44, 0x46, 0x45, 0x38, // �		96
	0x38, 0x42, 0x41, 0x42, 0x39, // �		97
	0x1C, 0x30, 0x79, 0x2E, 0x10, // tuz	98
	0x44, 0x24, 0x1E, 0x24, 0x44, // ember	99
	0x3E, 0x55, 0x51, 0x55, 0x3E, // arc	100
	0x00, 0x06, 0x09, 0x09, 0x06 // fok	101
};

uint16_t twoline_nums[] =
{
	0xF80F, 0xFE3F, 0x0770, 0x0360, 0x0360, 0x0360, 0x0770, 0xFE3F, 0xF80F, 	// 0
	0x0000, 0x0000, 0x1800, 0x1800, 0x0C00, 0xFF7F, 0xFF7F, 0x0000, 0x0000, 	// 1
	0x0860, 0x0E70, 0x0778, 0x036C, 0x0366, 0x0363, 0x8361, 0xFE60, 0x3C60, 	// 2
	0x0C18, 0x0E38, 0x0770, 0x0360, 0xC360, 0xE760, 0xBE71, 0xBC3F, 0x001F, 	// 3
	0x0006, 0x8007, 0xC007, 0x7006, 0x3806, 0x0E06, 0xFF7F, 0xFF7F, 0x0006, 	// 4
	0xF018, 0xFF38, 0x4F70, 0x6360, 0x6360, 0x6360, 0xE330, 0xC33F, 0x800F, 	// 5
	0xF00F, 0xFC3F, 0xCE30, 0x6360, 0x6360, 0x6360, 0xE770, 0xCE3F, 0x8C0F, 	// 6
	0x0300, 0x0300, 0x0378, 0x037F, 0xC307, 0xF300, 0x3B00, 0x0F00, 0x0300, 	// 7
	0x3C1F, 0xFE3F, 0xE771, 0xC360, 0xC360, 0xC360, 0xE771, 0xFE3F, 0x3C1F, 	// 8
	0xF818, 0xFE39, 0x8773, 0x0363, 0x0363, 0x0363, 0x8631, 0xFE3F, 0xF80F, 	// 9
	0x0060, 0x0060,																				// .
	0x3060, 0x3060																					// :
};

#endif /* FONT_H_ */
