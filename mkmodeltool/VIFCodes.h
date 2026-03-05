#pragma once
#include <vector>
#include "MKModel.h"

#define VIF_STMASK 0x20
#define VIF_STROW 0x30
#define VIF_STCOL 0x31
#define VIF_UNPACK 0x60


struct VIFUnpack {
	int numElems = 0;
	int elemBits = 0;
	int num = 0;
	int cmd = 0;
	std::vector<char> data;
};

struct VIFList {
	std::vector<VIFUnpack> items;
};


unsigned int VIFEnc(int num, int addr, int fmt, int mBit, int unsignd);