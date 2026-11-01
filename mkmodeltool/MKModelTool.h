#pragma once
#include "MKModel.h"
#include <vector>
#include <string>
#include "RW.h"
#include <fstream>
#include "VIFCodes.h"
#include "NvTriStrip.h"
#include "MKArchive.h"

enum EExportType
{
	Export_SkinnedPalette = 0,
	Export_Skinned = 1,
	Export_SkinnedNoUV = 2,
	Export_Generic = 3
};

struct NvTriStripGeneratedStrips
{
	std::vector<int> strip;
	std::vector<int> skip;
};


class MKModelTool
{
private:
	std::ifstream pFile;
	MKFrameList frameList;

	std::vector<MKModelBone> bones;
	std::vector<MKAtomic> atoms;
	std::vector<XMFLOAT4X4> vtm;
	std::vector<MKVert> verts;
	std::vector<MKFace> faces;
	std::vector<MKMaterial> mats;
	std::vector<MKMeshGroup> groups;

	// RW stuff
	int numAtomics = 0;
	int numLights = 0;
	int numCameras = 0;
	int geometryCount = 0;
	int hanimCount = 0;
	int hanimBoneID[MAX_HANIM];
	int hanimSkinBoneID[MAX_HANIM];
	int hanimBoneType[MAX_HANIM];
	int atomicMeshType = 0;
	int meshFlags = 0;


	bool isPSP = false;
public:
	bool LoadDFF(std::string path);
	bool LoadMesh(std::string path);

	void ReadFrameList(MKFrameList* fl);
	void ReadBones();
	void ReadAtomics();
	void ReadMaterial(MKMaterial& om);
	void ReadGeometry(int geoIndex);

	void DumpMesh(std::string path);
	void DumpDFF(std::string path, EExportType et);
	int GetHAnimFromFrame(int frameID, std::vector<HAnimEntry>& haList);
	void RemapPalette(MKVert& v, int* pal, int pc, int* bi, int* bn, std::vector<HAnimEntry>& haList);
	void GetHAnimBoneIndices(MKVert& v, int* bi, int* bn, std::vector<HAnimEntry>& haList);
	int BuildPalette(MKMeshGroup& sub, int* pal, std::vector<HAnimEntry>& haList);

	unsigned char FindBone(unsigned int boneData, int* bonePalette);
	VIFList UnpackVIFCode(const char* data, int size);
	unsigned int UnpackRWVersion(unsigned int packedVersion);
};
