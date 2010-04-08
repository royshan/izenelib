#include <am/vsynonym/VTrie.h>

#include <types.h>

NS_IZENELIB_AM_BEGIN

/* Conversion table for VTKEY_NUM numerical code */
uint8_t VTRIE_CODE[ VTKEY_NUM ] = {
//	0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xa 0xb 0xc 0xd 0xe 0xf
	  0,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239, // 0x00
	240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255, // 0x10
	 94, 75, 66,  1, 67, 68, 69, 28, 70, 71, 72, 73, 74, 65, 76, 77, // 0x20
	 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 78, 79, 80, 81, 82, 83, // 0x30
	 84, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, // 0x40
	 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 85, 86, 87, 88, 89, // 0x50
	 95,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, // 0x60
	 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 90, 91, 92, 93, 96, // 0x70
	 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,112, // 0x80
	113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128, // 0x90
	129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144, // 0xa0
	145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160, // 0xb0
	161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176, // 0xc0
	177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192, // 0xd0
	193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208, // 0xe0
	209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224  // 0xf0
};

NS_IZENELIB_AM_END
