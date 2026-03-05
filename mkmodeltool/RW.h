#pragma once

#define RW_STRUCT   0x1
#define RW_STRING   0x2
#define RW_EXT      0x3
#define RW_TEX      0x6
#define RW_MAT      0x7
#define RW_MATL     0x8
#define RW_FRL      0xE
#define RW_GEO      0xF
#define RW_CLUMP    0x10
#define RW_ATOMIC   0x14
#define RW_GEOL     0x1A
#define RW_R2R      0x1F
#define RW_SMIP     0x110
#define RW_SKIN     0x116
#define RW_HANIM    0x11E
#define RW_BMESH    0x50E
#define RW_MKSOBJ   0x895302
#define RW_MKMAT    0x895303



struct RWChunk 
{
	unsigned int rwSectionID;
	unsigned int size;
	unsigned int rwLibraryID; // rw ver
};

struct HAnimEntry
{
	int boneID;
	int skinBoneID;
	int boneType;
	int groupID;
	int frameID;
};