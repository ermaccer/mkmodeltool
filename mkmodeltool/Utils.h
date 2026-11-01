#pragma once
#include <vector>
#include "RW.h"
#include "VIFCodes.h"
#include <DirectXMath.h>

// https://github.com/aap/librw/blob/master/src/rwbase.h

enum Platform
{
	PLATFORM_NULL = 0,
	// D3D7
	PLATFORM_GL = 2,
	// MAC
	PLATFORM_PS2 = 4,
	PLATFORM_XBOX = 5,
	// GAMECUBE
	// SOFTRAS
	PLATFORM_D3D8 = 8,
	PLATFORM_D3D9 = 9,
	PLATFORM_PSP = 10,
	// non-stock-RW platforms

	PLATFORM_WDGL = 11,	// WarDrum OpenGL

	NUM_PLATFORMS,

	FOURCC_PS2 = 0x00325350		// 'PS2\0'
};
using namespace DirectX;



// math helpers
XMFLOAT4X4 mat4_mul(XMFLOAT4X4* a, XMFLOAT4X4* b);
XMFLOAT4X4 mat4_identity();
Vec3 mat3_transform(XMFLOAT4X4* m, Vec3 v);
XMFLOAT4X4 mat3_inv_transpose(XMFLOAT4X4* m);

// reading helper functions

MKSkinData parse_skin_nn(std::vector<char>& d, int nv, unsigned int ver);
MKSkinData parse_skin_native(std::vector<char>& d);
MKBinMeshPLG parse_binmesh(std::vector<char>& d, int native);
std::vector<float> decode_normals(char* data, int nv);
std::vector<float> decode_uv16(char* data, int nv);
VertexIDList decode_vidlist_skin(char* data, int dataSize, int useBP);
VertexIDListShared decode_shared_vidlist_skin(char* data, int dataSize, int useBP);
VertexIDList decode_vidlist_map(char* data, int dataSize, int* out_interval);
VertexIDListShared decode_shared_vidlist_map(char* data, int dataSize, int interval);
void create_trilist(std::vector<int>& skipList, int nv, std::vector<MKFace>& outFaces, unsigned int baseIdx);
void create_tristrip_simple(std::vector<float>& verts, int nv, std::vector<MKFace>& outFaces, unsigned int baseIdx);
MKFaceSetNoUV create_trilist_nouv(char* data, int dataSize, int* bonePalette, MKFrameList* fl);
void transform_verts_ps2(std::vector<float>& verts, int nv, XMFLOAT4X4* mat, int skinFlag);
void transform_normals_ps2(std::vector<float>& norms, int nv, XMFLOAT4X4* mat);
void fix_face_normals(std::vector<MKFace>& faces, int startFace, std::vector<float>& verts, std::vector<float>& norms, int vbase);