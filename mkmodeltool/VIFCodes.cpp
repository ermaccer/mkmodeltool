#include "VIFCodes.h"
#include <cstring>

unsigned int VIFEnc(int num, int addr, int fmt, int mBit, int unsignd)
{
	int vl = fmt & 3, vn = (fmt >> 2) & 3;
	unsigned int v = (addr & 0xFFFF) | (unsignd << 14) | (1 << 15);
	v |= (num & 0xFF) << 16;
	v |= ((unsigned int)(vl | (vn << 2) | (mBit << 4) | (3 << 5))) << 24;
	return v;
}