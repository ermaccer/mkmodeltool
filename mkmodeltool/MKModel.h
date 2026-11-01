#pragma once

#include <vector>
#include "Vector.h"
#include <DirectXMath.h>
using namespace DirectX;

#define MAX_FRAMES 256
#define MAX_HANIM 256
#define MAX_STRIPS 64
#define MAX_PALETTE 64

enum EMeshFlags
{
	EMF_Normals = 1,
	EMF_UV = 2,
	EMF_VertexColor = 4,
	EMF_Skinned = 8
};


struct MKVert
{
	float pos[3];
	float nrm[3];
	float uv[2];
	unsigned char col[4];
	unsigned char bi[4];
	float bw[4];
};


struct PSPVertexFormat
{
	int uvFormat;
	int colorFormat;
	int normalFormat;
	int positionFormat;
	int weightFormat;
	int indexFormat;
	int numWeights;
	int numVertices;
	int coordType;
};

struct PSPSkinData
{
	int boneCount;
	int numUsedBone;
	int maxWeights;
	std::vector<int> usedBoneIndexList;
	std::vector<std::vector<int>> bonePalettes;
};

struct MKFace
{
	unsigned int v[3];
};

#pragma pack (push,1)
struct MKMaterial
{
	// RenderWare base
	unsigned char  r, g, b, a;
	float    rw_ambient;
	float    rw_specular;
	float    rw_diffuse;
	int  hasTexture;
	char     tex[64];
	char     alphaTex[64];
	unsigned char  texFilter;
	unsigned char  uAddressing;
	unsigned char  vAddressing;
	unsigned char  pad0;
	unsigned short useMipLevel;
	unsigned char  hasSkyMipmap;
	unsigned char  pad1;
	int  skyMipmapK;
	int  skyMipmapL;


	// MKMaterial
	int  mkFlag;
	int  mkType;
	int  mkFlagsUpper;
	short  mkUnk1;
	short  mkUnk2;
	float    mkUnk3;
	int  mkUseBonePalette;
	int  mkBonePaletteCount;
	int  mkBonePalette[64];
	unsigned char  mkSpecR, mkSpecG, mkSpecB, mkSpecA;
	float    mkUnk5;
	float    mkUnk6;
	int  mkUVScroll;
	float    mkScrollX, mkScrollY, mkScrollZ, mkScrollPad;
	int  mkMeshType;
};
#pragma pack(pop)

struct MKMeshGroup
{
	unsigned int matID;
	unsigned int firstFace;
	unsigned int numFaces;
};

struct MKModelHeader
{
	int header;
	int version;
	int numVertexes;
	int numFaces;
	int numBones;
	int numMaterials;
	int flags;
	int numGroups;
};

#pragma pack (push,1)
struct MKModelBone
{
	float mat[12]; // 3x3 rotation, xyz translation
	int parent;
	int id;
	int boneType;
	int skinBoneID;
	int groupID;
	char name[64];
};
#pragma pack(pop)

struct MKFrameInfo
{
	XMFLOAT4X4 matrix;
	int parentID;
	int animBoneID;
	int skinBoneID;
	char name[64];
};

struct MKFrameList
{
	int frameCount;
	MKFrameInfo frames[MAX_FRAMES];
	XMFLOAT4X4 worldMat[MAX_FRAMES];
	int hasHAnim, hanimCount;
	int hAnimBoneID[MAX_HANIM];
	int hSkinBoneID[MAX_HANIM];
	int hAnimBoneType[MAX_HANIM];
	int hAnimBoneMap[MAX_HANIM];
	int hAnimBoneMapCount;
	int vgBoneMap[MAX_HANIM];
	int vgCount;
	int hanimToFrame[MAX_HANIM];
};


struct MKAtomic
{
	int frameID;
	int geoID;
};

struct TriStripReference
{
	char* data;
	int size;
};

struct VertexIDList
{
	std::vector<int> ids;
	std::vector<int> skipFlags;
};

struct VertexIDListShared
{
	std::vector<int> prevIDs;
	std::vector<int> curIDs;
	std::vector<int> skipPrev;
	std::vector<int> skipCur;
};

struct MKFaceSetNoUV
{
	std::vector<MKFace> faces;
	std::vector<int> boneIDs;
	std::vector<float> weights;
	int nv;
};

struct MKSkinData
{
	int boneCount = 0;
	int usedBoneCount = 0;
	int maxWeights = 0;
	int numVert = 0;
	std::vector<unsigned char> boneID;
	std::vector<unsigned char> boneWgt;
	std::vector<int> usedBoneList;
};


struct MKBinMeshPLG
{
	int faceType = 0;
	int numSplits = 0;
	int idxCount = 0;
	std::vector<int> matIDs;
	std::vector<int> matNumFaces;
	std::vector<std::vector<unsigned int>> splitIdx;
	std::vector<MKFace> faces;
	std::vector<int> faceMatID;
};

