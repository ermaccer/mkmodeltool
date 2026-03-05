#include "MKModelTool.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <Windows.h>
#include "Utils.h"

void writeChunk(std::ofstream& o, unsigned int id, unsigned int lib, unsigned int sz)
{
	o.write((char*)&id, sizeof(int));
	o.write((char*)&sz, sizeof(int));
	o.write((char*)&lib, sizeof(int));
}

void adjustInt(std::ofstream& o, unsigned int offset, unsigned int v)
{
	unsigned int cur = (unsigned int)o.tellp();
	o.seekp(offset);
	o.write((char*)&v, sizeof(int));
	o.seekp(cur);
}

void adjustShort(std::ofstream& o, unsigned int offset, unsigned short v)
{
	unsigned int cur = (unsigned int)o.tellp();
	o.seekp(offset);
	o.write((char*)&v, sizeof(short));
	o.seekp(cur);
}

XMFLOAT4X4 MatrixFromBone(const float* b)
{
	XMFLOAT4X4 r;
	ZeroMemory(&r, sizeof(r));
	r.m[0][0] = b[0];  r.m[0][1] = b[1];  r.m[0][2] = b[2];
	r.m[1][0] = b[3];  r.m[1][1] = b[4];  r.m[1][2] = b[5];
	r.m[2][0] = b[6];  r.m[2][1] = b[7];  r.m[2][2] = b[8];
	r.m[3][0] = b[9];  r.m[3][1] = b[10]; r.m[3][2] = b[11]; r.m[3][3] = 1;
	return r;
}

XMFLOAT4X4 MatrixInvAffine(const XMFLOAT4X4* a)
{
	XMFLOAT4X4 r;
	ZeroMemory(&r, sizeof(r));
	r.m[0][0] = a->m[0][0]; r.m[0][1] = a->m[1][0]; r.m[0][2] = a->m[2][0];
	r.m[1][0] = a->m[0][1]; r.m[1][1] = a->m[1][1]; r.m[1][2] = a->m[2][1];
	r.m[2][0] = a->m[0][2]; r.m[2][1] = a->m[1][2]; r.m[2][2] = a->m[2][2];
	float tx = a->m[3][0], ty = a->m[3][1], tz = a->m[3][2];
	r.m[3][0] = -(r.m[0][0] * tx + r.m[1][0] * ty + r.m[2][0] * tz);
	r.m[3][1] = -(r.m[0][1] * tx + r.m[1][1] * ty + r.m[2][1] * tz);
	r.m[3][2] = -(r.m[0][2] * tx + r.m[1][2] * ty + r.m[2][2] * tz);
	r.m[3][3] = 1;
	return r;
}

static void writeRWMatrix(std::ofstream& o, const XMFLOAT4X4* m)
{
	o.write((char*)&m->m[0][0], sizeof(float)); o.write((char*)&m->m[0][1], sizeof(float)); o.write((char*)&m->m[0][2], sizeof(float));
	o.write((char*)&m->m[1][0], sizeof(float)); o.write((char*)&m->m[1][1], sizeof(float)); o.write((char*)&m->m[1][2], sizeof(float));
	o.write((char*)&m->m[2][0], sizeof(float)); o.write((char*)&m->m[2][1], sizeof(float)); o.write((char*)&m->m[2][2], sizeof(float));
	o.write((char*)&m->m[3][0], sizeof(float)); o.write((char*)&m->m[3][1], sizeof(float)); o.write((char*)&m->m[3][2], sizeof(float));
}
static int CountWeights(MKVert& v)
{
	int n = 0;
	for (int i = 0; i < 4; i++)
		if (v.bw[i] > 0.f) n++;
	return n;
}



bool MKModelTool::LoadDFF(std::string path)
{
	pFile.open(path, std::ifstream::binary);

	int texStart;
	pFile.read((char*)&texStart, sizeof(int));
	int texEnd;
	pFile.read((char*)&texEnd, sizeof(int));

	RWChunk chunkHeader;
	pFile.read((char*)&chunkHeader, sizeof(RWChunk));
	if (chunkHeader.rwSectionID != 0x10)
	{
		std::cout << "ERROR: Not a valid MK DFF file!" << std::endl;
		return false;
	}

	RWChunk clumpHeader;
	pFile.read((char*)&clumpHeader, sizeof(RWChunk));
	pFile.read((char*)&numAtomics, sizeof(int));

	unsigned int rwVersion = UnpackRWVersion(clumpHeader.rwLibraryID);
	if (rwVersion >= 0x33000)
	{
		pFile.read((char*)&numLights, sizeof(int));
		pFile.read((char*)&numCameras, sizeof(int));
	}

	RWChunk frameListHeader;
	pFile.read((char*)&frameListHeader, sizeof(RWChunk));
	unsigned int donePos = (unsigned int)pFile.tellg() + frameListHeader.size;

	ZeroMemory(&frameList, sizeof(MKFrameList));
	ReadFrameList(&frameList);
	pFile.seekg(donePos, pFile.beg);

	ReadBones();

	// GeometryList
	RWChunk geometryListHeader;
	pFile.read((char*)&geometryListHeader, sizeof(RWChunk));
	RWChunk geometryListStruct;
	pFile.read((char*)&geometryListStruct, sizeof(RWChunk));

	pFile.read((char*)&geometryCount, sizeof(int));
	unsigned int geometryStartOffset = (unsigned int)pFile.tellg();
	unsigned int geoListEnd = geometryStartOffset - 12 - geometryListStruct.size + geometryListHeader.size;
	pFile.seekg(geoListEnd, pFile.beg);
	printf_s("geometryListHeader id=0x%X size=%d\n", geometryListHeader.rwSectionID, geometryListHeader.size);
	printf_s("geometryListStruct id=0x%X size=%d\n", geometryListStruct.rwSectionID, geometryListStruct.size);
	printf_s("geometryCount=%d pos=%d\n", geometryCount, (unsigned int)pFile.tellg());
	ReadAtomics();
	pFile.seekg(geometryStartOffset, pFile.beg);

	for (int i = 0; i < geometryCount; i++)
	{
		RWChunk geometryHeader;
		pFile.read((char*)&geometryHeader, sizeof(RWChunk));
		unsigned int endPos = (unsigned int)pFile.tellg() + geometryHeader.size;
		ReadGeometry(i);
		pFile.seekg(endPos, pFile.beg);
	}

	pFile.close();
	return true;
}

bool MKModelTool::LoadMesh(std::string path)
{
	pFile.open(path, std::ifstream::binary);

	MKModelHeader model;

	pFile.read((char*)&model, sizeof(MKModelHeader));

	if (!(model.header == 'MESH'))
	{
		std::cout << "ERROR: Not a valid mk mesh file!" << std::endl;
		return false;
	}

	meshFlags = model.flags;

	for (int i = 0; i < model.numBones; i++)
	{
		MKModelBone bone;
		pFile.read((char*)&bone, sizeof(MKModelBone));
		bones.push_back(bone);
	}

	for (int i = 0; i < model.numMaterials; i++)
	{
		MKMaterial mat;
		pFile.read((char*)&mat, sizeof(MKMaterial));
		mats.push_back(mat);
	}

	for (int i = 0; i < model.numVertexes; i++)
	{
		MKVert v = {};
		pFile.read((char*)&v.pos, sizeof(float) * 3);

		if (meshFlags & EMF_Normals)
			pFile.read((char*)v.nrm, sizeof(v.nrm));
		if (meshFlags & EMF_UV)
			pFile.read((char*)v.uv, sizeof(v.uv));
		if (meshFlags & EMF_VertexColor)
			pFile.read((char*)v.col, sizeof(v.col));
		if (meshFlags & EMF_Skinned)
		{
			pFile.read((char*)v.bi, sizeof(v.bi));
			pFile.read((char*)v.bw, sizeof(v.bw));
		}

		verts.push_back(v);
	}

	for (int i = 0; i < model.numGroups; i++)
	{
		MKMeshGroup sub;
		pFile.read((char*)&sub, sizeof(MKMeshGroup));
		groups.push_back(sub);
	}

	for (int i = 0; i < model.numFaces; i++)
	{
		MKFace face;
		pFile.read((char*)&face, sizeof(MKFace));
		faces.push_back(face);
	}


	pFile.read((char*)&hanimCount, sizeof(int));

	for (int i = 0; i < hanimCount; i++)
	{
		pFile.read((char*)&hanimBoneID[i], sizeof(int));
		pFile.read((char*)&hanimSkinBoneID[i], sizeof(int));
		pFile.read((char*)&hanimBoneType[i], sizeof(int));
	}



	pFile.close();
	return true;
}

void MKModelTool::ReadFrameList(MKFrameList* fl)
{
	RWChunk frameListStruct;
	pFile.read((char*)&frameListStruct, sizeof(RWChunk));
	int frameCount = 0;
	pFile.read((char*)&frameCount, sizeof(int));
	fl->frameCount = frameCount;

	for (int i = 0; i < fl->frameCount; i++)
	{
		MKFrameInfo& frame = fl->frames[i];
		float rawMat[12]; // 3x3 rotation + xyz position
		pFile.read((char*)rawMat, 48);

		ZeroMemory(&frame.matrix, sizeof(XMFLOAT4X4));
		frame.matrix.m[0][0] = rawMat[0];
		frame.matrix.m[0][1] = rawMat[3];
		frame.matrix.m[0][2] = rawMat[6];
		frame.matrix.m[0][3] = rawMat[9];
		frame.matrix.m[1][0] = rawMat[1];
		frame.matrix.m[1][1] = rawMat[4];
		frame.matrix.m[1][2] = rawMat[7];
		frame.matrix.m[1][3] = rawMat[10];
		frame.matrix.m[2][0] = rawMat[2];
		frame.matrix.m[2][1] = rawMat[5];
		frame.matrix.m[2][2] = rawMat[8];
		frame.matrix.m[2][3] = rawMat[11];
		frame.matrix.m[3][3] = 1.0f;
		pFile.read((char*)&frame.parentID, sizeof(int));
		int pad;
		pFile.read((char*)&pad, sizeof(int));
		frame.animBoneID = -1;
		frame.skinBoneID = -1;
		sprintf_s(frame.name, "Bone%d", i);
	}

	// convert local matrix to world matrix
	for (int i = 0; i < fl->frameCount; i++)
	{
		int pi = fl->frames[i].parentID;
		if (pi > -1 && pi < fl->frameCount)
			fl->worldMat[i] = mat4_mul(&fl->frames[i].matrix, &fl->worldMat[pi]);
		else
			fl->worldMat[i] = fl->frames[i].matrix;
	}

	// rwFrameExtension list
	for (int i = 0; i < fl->frameCount; i++)
	{
		MKFrameInfo& frame = fl->frames[i];
		RWChunk frameExtensionHeader;
		pFile.read((char*)&frameExtensionHeader, sizeof(RWChunk));
		unsigned int donePos = (unsigned int)pFile.tellg() + frameExtensionHeader.size;

		if ((int)frameExtensionHeader.size <= 0)
		{
			frame.animBoneID = -1;
			sprintf_s(frame.name, "Frame%d", i);
			continue;
		}

		while ((unsigned int)pFile.tellg() < donePos)
		{
			RWChunk ch;
			pFile.read((char*)&ch, sizeof(RWChunk));
			if (ch.rwSectionID == RW_HANIM)
			{

				int version;
				pFile.read((char*)&version, sizeof(int));
				pFile.read((char*)&frame.animBoneID, sizeof(int));
				int boneCount = 0;
				pFile.read((char*)&boneCount, sizeof(int));
				printf_s("  frame %d: HAnim found, animBoneID=%d boneCount=%d\n", i, frame.animBoneID, boneCount);
				if (boneCount <= 0)
					continue;

				fl->hasHAnim = true;
				fl->hanimCount = boneCount;
				int flags;
				pFile.read((char*)&flags, sizeof(int));
				int keyFrameSize;
				pFile.read((char*)&keyFrameSize, sizeof(int));
				for (int a = 0; a < boneCount; a++)
				{
					pFile.read((char*)&fl->hAnimBoneID[a], sizeof(int));
					pFile.read((char*)&fl->hSkinBoneID[a], sizeof(int));
					pFile.read((char*)&fl->hAnimBoneType[a], sizeof(int));
				}
			}
			else
			{
				pFile.seekg(ch.size, pFile.cur);
			}
		}
		pFile.seekg(donePos, pFile.beg);

		if (frame.animBoneID >= 0)
			sprintf_s(frame.name, "Bone%d", frame.animBoneID);
		else
			sprintf_s(frame.name, "Frame%d", i);
	}

	if (fl->hasHAnim)
	{
		// Map skinBoneID to frames
		for (int i = 0; i < fl->frameCount; i++)
		{
			for (int a = 0; a < fl->hanimCount; a++)
			{
				if ((fl->hAnimBoneID[a] & 0xFFFF) == fl->frames[i].animBoneID)
				{
					fl->frames[i].skinBoneID = fl->hSkinBoneID[a];
					break;
				}
			}
		}

		// hAnimBoneMap
		fl->hAnimBoneMapCount = 0;
		for (int i = 0; i < fl->hanimCount; i++)
		{
			int hab = (fl->hAnimBoneID[i] & 0xFFFF);
			for (int a = 0; a < fl->frameCount; a++)
			{
				if (fl->frames[a].animBoneID == hab)
				{
					if (fl->hAnimBoneMapCount < MAX_HANIM)
						fl->hAnimBoneMap[fl->hAnimBoneMapCount++] = fl->frames[a].skinBoneID;
					break;
				}
				if ((hab & 0x8000) && (hab & 0x7FFF) == fl->frames[a].animBoneID)
				{
					if (fl->hAnimBoneMapCount < MAX_HANIM)
						fl->hAnimBoneMap[fl->hAnimBoneMapCount++] = fl->frames[a].skinBoneID;
					break;
				}
			}
		}

		// hanimToFrame
		fl->vgCount = 0;
		for (int i = 0; i < fl->hanimCount; i++)
		{
			int boneID = fl->hAnimBoneID[i] & 0x7FFF;
			fl->hanimToFrame[i] = -1;
			for (int a = 0; a < fl->frameCount; a++)
			{
				if (fl->frames[a].animBoneID == boneID)
				{
					if (fl->vgCount < MAX_HANIM)
						fl->vgBoneMap[fl->vgCount++] = fl->frames[a].skinBoneID;
					fl->hanimToFrame[i] = a;
					break;
				}
			}
		}
	}
}

void MKModelTool::ReadBones()
{
	for (int i = 0; i < frameList.frameCount; i++)
	{
		MKFrameInfo& frame = frameList.frames[i];
		MKModelBone b = {};
		XMFLOAT4X4* m = &frameList.worldMat[i];
		b.mat[0] = m->m[0][0]; b.mat[1] = m->m[0][1]; b.mat[2] = m->m[0][2];
		b.mat[3] = m->m[1][0]; b.mat[4] = m->m[1][1]; b.mat[5] = m->m[1][2];
		b.mat[6] = m->m[2][0]; b.mat[7] = m->m[2][1]; b.mat[8] = m->m[2][2];
		b.mat[9] = m->m[0][3]; b.mat[10] = m->m[1][3]; b.mat[11] = m->m[2][3];
		b.parent = frame.parentID;
		b.id = frame.animBoneID;
		b.skinBoneID = frame.skinBoneID;
		b.boneType = 0;
		b.groupID = 0;
		if (frameList.hasHAnim)
		{
			for (int j = 0; j < frameList.hanimCount; j++)
			{
				if ((int)(frameList.hAnimBoneID[j] & 0xFFFF) == frame.animBoneID)
				{
					b.boneType = frameList.hAnimBoneType[j];
					b.groupID = (int)((frameList.hAnimBoneID[j] >> 16) & 0xFFFF);
					break;
				}
			}
		}
		strncpy_s(b.name, frameList.frames[i].name, 63);
		bones.push_back(b);
	}

	hanimCount = frameList.hanimCount;
	for (int j = 0; j < frameList.hanimCount; j++)
	{
		hanimBoneID[j] = frameList.hAnimBoneID[j];
		hanimSkinBoneID[j] = frameList.hSkinBoneID[j];
		hanimBoneType[j] = frameList.hAnimBoneType[j];
	}
}

void MKModelTool::ReadAtomics()
{
	atoms.resize(numAtomics);
	vtm.resize((std::max)(geometryCount, numAtomics));

	for (auto& m : vtm)
		m = mat4_identity();

	for (int i = 0; i < numAtomics; i++)
	{
		RWChunk atomicHeader;
		pFile.read((char*)&atomicHeader, sizeof(RWChunk));
		RWChunk atomicStruct;
		pFile.read((char*)&atomicStruct, sizeof(RWChunk));
		pFile.read((char*)&atoms[i].frameID, sizeof(int));
		pFile.read((char*)&atoms[i].geoID, sizeof(int));
		int pad;
		pFile.read((char*)&pad, sizeof(int));
		pFile.read((char*)&pad, sizeof(int));

		RWChunk ext;
		pFile.read((char*)&ext, sizeof(RWChunk));
		unsigned int extEnd = (unsigned int)pFile.tellg() + ext.size;

		while ((unsigned int)pFile.tellg() < extEnd)
		{
			RWChunk ec;
			pFile.read((char*)&ec, sizeof(RWChunk));
			if (ec.rwSectionID == RW_MKSOBJ && ec.size >= 16)
			{
				unsigned int ecEnd = (unsigned int)pFile.tellg() + ec.size;
				int pad;
				pFile.read((char*)&pad, sizeof(int));
				int pad2;
				pFile.read((char*)&pad2, sizeof(int));
				float pad3;
				pFile.read((char*)&pad3, sizeof(float));
				pFile.read((char*)&atomicMeshType, sizeof(int));
				pFile.seekg(ecEnd, pFile.beg);
			}
			else
			{
				pFile.seekg(ec.size, pFile.cur);
			}
		}
		pFile.seekg(extEnd, pFile.beg);
	}

	for (int i = 0; i < numAtomics; i++)
	{
		if (atoms[i].geoID < geometryCount && atoms[i].frameID < frameList.frameCount)
			vtm[atoms[i].geoID] = frameList.worldMat[atoms[i].frameID];
	}
}

void MKModelTool::ReadMaterial(MKMaterial& om)
{
	RWChunk hdr;
	pFile.read((char*)&hdr, sizeof(RWChunk));
	int unused;
	pFile.read((char*)&unused, sizeof(int));
	pFile.read((char*)&om.r, sizeof(unsigned char)); pFile.read((char*)&om.g, sizeof(unsigned char));
	pFile.read((char*)&om.b, sizeof(unsigned char)); pFile.read((char*)&om.a, sizeof(unsigned char));
	int unused2;
	pFile.read((char*)&unused2, sizeof(int));
	pFile.read((char*)&om.hasTexture, sizeof(int));
	pFile.read((char*)&om.rw_ambient, sizeof(float));
	pFile.read((char*)&om.rw_specular, sizeof(float));
	pFile.read((char*)&om.rw_diffuse, sizeof(float));

	ZeroMemory(om.tex, 64); ZeroMemory(om.alphaTex, 64);
	om.texFilter = 0; om.uAddressing = 0; om.vAddressing = 0;
	om.useMipLevel = 0; om.hasSkyMipmap = 0;
	om.skyMipmapK = 0; om.skyMipmapL = 0;
	om.mkFlag = 0; om.mkType = 0; om.mkFlagsUpper = 0;
	om.mkUnk1 = 0; om.mkUnk2 = 0; om.mkUnk3 = 0;
	om.mkUseBonePalette = 0; om.mkBonePaletteCount = 0;
	om.mkSpecR = 127; om.mkSpecG = 127; om.mkSpecB = 127; om.mkSpecA = 255;
	om.mkUnk5 = 0; om.mkUnk6 = 0;
	om.mkUVScroll = 0; om.mkScrollX = 0; om.mkScrollY = 0; om.mkScrollZ = 0; om.mkScrollPad = 0;
	om.mkMeshType = 0;

	if (om.hasTexture)
	{
		RWChunk th;
		pFile.read((char*)&th, sizeof(RWChunk));
		RWChunk ts;
		pFile.read((char*)&ts, sizeof(RWChunk));
		pFile.read((char*)&om.texFilter, sizeof(unsigned char));
		unsigned char uvAddr = 0;
		pFile.read((char*)&uvAddr, sizeof(unsigned char));
		om.uAddressing = uvAddr >> 4;
		om.vAddressing = uvAddr & 0xF;
		pFile.read((char*)&om.useMipLevel, sizeof(unsigned short));

		RWChunk nc;
		pFile.read((char*)&nc, sizeof(RWChunk));
		if (nc.size > 0 && nc.size < 64)
			pFile.read(om.tex, nc.size);
		else
			pFile.seekg(nc.size, pFile.cur);

		RWChunk anc;
		pFile.read((char*)&anc, sizeof(RWChunk));
		if (anc.size > 0 && anc.size < 64)
			pFile.read(om.alphaTex, anc.size);
		else
			pFile.seekg(anc.size, pFile.cur);

		RWChunk te;
		pFile.read((char*)&te, sizeof(RWChunk));
		unsigned int texExtEnd = (unsigned int)pFile.tellg() + te.size;
		if (te.size)
		{
			RWChunk texExtChunk;
			pFile.read((char*)&texExtChunk, sizeof(RWChunk));
			if (texExtChunk.rwSectionID == RW_SMIP)
			{
				om.hasSkyMipmap = 1;
				int val = 0;
				pFile.read((char*)&val, sizeof(int));
				om.skyMipmapK = val & 0xFFF;
				om.skyMipmapL = (val >> 12) & 3;
			}
			pFile.seekg(texExtEnd, pFile.beg);
		}
	}

	// Material extension
	RWChunk me;
	pFile.read((char*)&me, sizeof(RWChunk));
	unsigned int mend = (unsigned int)pFile.tellg() + me.size;
	while ((unsigned int)pFile.tellg() < mend)
	{
		RWChunk ch;
		pFile.read((char*)&ch, sizeof(RWChunk));
		if (ch.rwSectionID == RW_MKMAT)
		{
			int flag = 0;
			pFile.read((char*)&flag, sizeof(int));
			om.mkFlag = flag;
			om.mkType = flag & 0xFFFF;
			om.mkFlagsUpper = flag >> 16;
			int first = flag & 0xFFFF, second = flag >> 16;
			pFile.read((char*)&om.mkUnk1, sizeof(short));
			pFile.read((char*)&om.mkUnk2, sizeof(short));
			pFile.read((char*)&om.mkUnk3, sizeof(float));
			if (first >= 2)
			{
				if (flag & 0x40000000)
				{
					om.mkUseBonePalette = 1;
					pFile.read((char*)&om.mkBonePaletteCount, sizeof(int));
					for (int i = 0; i < om.mkBonePaletteCount && i < 64; i++)
						pFile.read((char*)&om.mkBonePalette[i], sizeof(int));
				}
				else
				{
					om.mkUseBonePalette = 0;
					om.mkBonePaletteCount = 3;
					om.mkBonePalette[0] = om.mkBonePalette[1] = om.mkBonePalette[2] = 0;
				}
			}
			if (first >= 3)
			{
				pFile.read((char*)&om.mkSpecR, sizeof(unsigned char)); pFile.read((char*)&om.mkSpecG, sizeof(unsigned char));
				pFile.read((char*)&om.mkSpecB, sizeof(unsigned char)); pFile.read((char*)&om.mkSpecA, sizeof(unsigned char));
			}
			if (first >= 4)
			{
				pFile.read((char*)&om.mkUnk5, sizeof(float));
				pFile.read((char*)&om.mkUnk6, sizeof(float));
				if (second & 0x8000)
				{
					om.mkUVScroll = 1;
					pFile.read((char*)&om.mkScrollX, sizeof(float)); pFile.read((char*)&om.mkScrollY, sizeof(float));
					pFile.read((char*)&om.mkScrollZ, sizeof(float)); pFile.read((char*)&om.mkScrollPad, sizeof(float));
				}
			}
			if (first >= 5)
				pFile.read((char*)&om.mkMeshType, sizeof(int));
		}
		else
		{
			pFile.seekg(ch.size, pFile.cur);
		}
	}
	pFile.seekg(mend, pFile.beg);
}

VIFList MKModelTool::UnpackVIFCode(const char* data, int size)
{
	VIFList vl;
	int pos = 0;
	while (pos < size)
	{
		unsigned int cmd;
		memcpy(&cmd, data + pos, 4);
		pos += 4;
		int vif_cmd = (cmd >> 24) & 0x7F;
		if (vif_cmd == VIF_STMASK)
		{
			//VIFn_MASK
			pos += 4;
		}
		else if (vif_cmd == VIF_STROW)
		{
			//VIFn_R0_R3
			pos += 16;
		}
		else if (vif_cmd == VIF_STCOL)
		{
			//VIFn_C0_C3
			pos += 16;
		}
		else if (vif_cmd & VIF_UNPACK)
		{
			// VU Mem address in transfer destination(address divided by 16.)
			VIFUnpack u;
			u.num = (cmd >> 16) & 0xFF;
			int vl_bits = (cmd >> 24) & 3, vn = (cmd >> 26) & 3;
			u.cmd = (cmd >> 24) & 0xF;
			u.numElems = vn + 1;
			if (vl_bits == 0) u.elemBits = 32;
			else if (vl_bits == 1) u.elemBits = 16;
			else if (vl_bits == 2) u.elemBits = 8;
			else u.elemBits = 5;
			if (u.num)
			{
				u.data.resize(u.num * u.numElems * (u.elemBits / 8));
				if (u.cmd == 0xF) u.data.resize(u.num * 2);
				memcpy(u.data.data(), data + pos, u.data.size());
				pos += (int)u.data.size();
				if (u.data.size() % 4) pos += 4 - (u.data.size() % 4);
				vl.items.push_back(u);
			}
		}
	}
	return vl;
}

void MKModelTool::ReadGeometry(int geoIndex)
{

	XMFLOAT4X4* vertMat = &vtm[geoIndex];

	RWChunk geoStruct;
	pFile.read((char*)&geoStruct, sizeof(RWChunk));
	unsigned int rwVer = UnpackRWVersion(geoStruct.rwLibraryID);

	unsigned short fmtFlags = 0;
	pFile.read((char*)&fmtFlags, sizeof(unsigned short));
	char numUV;
	pFile.read((char*)&numUV, sizeof(char));
	char nativeFlags = 0;
	pFile.read((char*)&nativeFlags, sizeof(char));
	unsigned int numFace = 0;
	pFile.read((char*)&numFace, sizeof(unsigned int));
	unsigned int numVert = 0;
	pFile.read((char*)&numVert, sizeof(unsigned int));
	unsigned int numMorph = 0;
	pFile.read((char*)&numMorph, sizeof(unsigned int));
	printf_s("ReadGeometry(%d) nativeFlags=%d\n", geoIndex, nativeFlags);
	int Prelit = (fmtFlags >> 3) & 1;
	int Textured = (fmtFlags >> 2) & 1;
	int Meshes = (fmtFlags >> 1) & 1;
	int Textured2 = (fmtFlags >> 7) & 1;
	int MKSkinFlag = (fmtFlags & 0x100) ? 1 : 0;

	unsigned int vertBase = (unsigned int)verts.size();

	std::vector<float> uvData;
	std::vector<unsigned char> colorData;

	if (nativeFlags != 1)
	{
		if (rwVer < 0x34000)
			pFile.seekg(12, pFile.cur);
		if (Prelit)
		{
			colorData.resize(numVert * 4);
			pFile.read((char*)colorData.data(), numVert * 4);
			meshFlags |= EMF_VertexColor;
		}
		if (Textured || Textured2)
		{
			uvData.resize(numVert * 2);
			pFile.read((char*)uvData.data(), numVert * 8);
			meshFlags |= EMF_UV;
		}
		if (Textured2)
			pFile.seekg(numVert * 8, pFile.cur);
		if (Meshes)
			pFile.seekg(numFace * 8, pFile.cur);
	}

	// MORPHS: Only read first morph as a static model
	// mkda uses morphs for stage animations eg wu shi banners
	std::vector<float> vertPos;
	std::vector<float> vertNrm;

	for (unsigned int m = 0; m < numMorph; m++)
	{
		pFile.seekg(12, pFile.cur);
		float radius;
		pFile.read((char*)&radius, sizeof(float));
		unsigned int posF = 0;
		pFile.read((char*)&posF, sizeof(unsigned int));
		unsigned int nrmF = 0;
		pFile.read((char*)&nrmF, sizeof(unsigned int));
		if (nativeFlags != 1)
		{
			if (posF)
			{
				if (rwVer >= 0x34000)
				{
					unsigned int cur = (unsigned int)pFile.tellg();
					unsigned int pad1 = ((cur + 15) & ~15u) - cur;
					unsigned int pad2 = 16 - pad1;
					pFile.seekg(pad1, pFile.cur);

					if (m == 0)
					{
						vertPos.resize(numVert * 3);
						pFile.read((char*)vertPos.data(), numVert * 12);
					}
					else
						pFile.seekg(numVert * 12, pFile.cur);

					pFile.seekg(pad2, pFile.cur);
				}
				else
				{
					if (m == 0)
					{
						vertPos.resize(numVert * 3);
						pFile.read((char*)vertPos.data(), numVert * 12);
					}
					else
						pFile.seekg(numVert * 12, pFile.cur);
				}
			}
			if (nrmF)
			{
				if (rwVer >= 0x34000)
				{
					unsigned int cur = (unsigned int)pFile.tellg();
					unsigned int pad1 = ((cur + 15) & ~15u) - cur;
					unsigned int pad2 = 16 - pad1;
					pFile.seekg(pad1, pFile.cur);
					if (m == 0)
					{
						vertNrm.resize(numVert * 3);
						pFile.read((char*)vertNrm.data(), numVert * 12);
						meshFlags |= EMF_Normals;
					}
					else pFile.seekg(numVert * 12, pFile.cur);
					pFile.seekg(pad2, pFile.cur);
				}
				else
				{
					if (m == 0)
					{
						vertNrm.resize(numVert * 3);
						pFile.read((char*)vertNrm.data(), numVert * 12);
						meshFlags |= EMF_Normals;
					}
					else pFile.seekg(numVert * 12, pFile.cur);
				}
			}
		}
	}

	// material list

	RWChunk mlH;
	pFile.read((char*)&mlH, sizeof(RWChunk));
	unsigned int mlEnd = (unsigned int)pFile.tellg() + mlH.size;
	RWChunk mlStruct;
	pFile.read((char*)&mlStruct, sizeof(RWChunk));
	unsigned int matCount = 0;
	pFile.read((char*)&matCount, sizeof(unsigned int));
	pFile.seekg(matCount * 4, pFile.cur);

	int matBase = (int)mats.size();
	std::vector<MKMaterial> pmats(matCount);

	for (unsigned int i = 0; i < matCount; i++)
		ZeroMemory(&pmats[i], sizeof(MKMaterial));

	for (unsigned int i = 0; i < matCount; i++)
	{
		RWChunk mh;
		pFile.read((char*)&mh, sizeof(RWChunk));
		unsigned int me = (unsigned int)pFile.tellg() + mh.size;
		ReadMaterial(pmats[i]);
		pFile.seekg(me, pFile.beg);
		mats.push_back(pmats[i]);
	}
	pFile.seekg(mlEnd, pFile.beg);

	// geometry extension
	RWChunk geH;
	pFile.read((char*)&geH, sizeof(RWChunk));
	unsigned int geEnd = (unsigned int)pFile.tellg() + geH.size;
	bool isSkin = false;
	bool isBM = false;
	std::vector<char> skinD, bmD;

	while ((unsigned int)pFile.tellg() < geEnd)
	{
		RWChunk h;
		pFile.read((char*)&h, sizeof(RWChunk));
		if (h.rwSectionID == RW_BMESH)
		{
			isBM = true;
			bmD.resize(h.size);
			pFile.read(bmD.data(), h.size);
		}
		else if (h.rwSectionID == RW_SKIN)
		{
			isSkin = true;
			skinD.resize(h.size);
			pFile.read(skinD.data(), h.size);
		}
		else
		{
			pFile.seekg(h.size, pFile.cur);
		}
	}
	pFile.seekg(geEnd, pFile.beg);

	MKSkinData skin;
	MKBinMeshPLG bm;
	bool hasSkin = false;

	if (isSkin && nativeFlags == 0)
	{
		skin = parse_skin_nn(skinD, numVert, rwVer);
		hasSkin = true;
	}

	if (isBM)
		bm = parse_binmesh(bmD, nativeFlags);

	// standard geometry (mkda stages, probably other mkda models as well)
	if (nativeFlags != 1 && !vertPos.empty())
	{
		for (unsigned int i = 0; i < numVert; i++)
		{
			MKVert v = {};
			Vec3 pos; memcpy(&pos, &vertPos[i * 3], 12);
			pos = mat3_transform(vertMat, pos);

			v.pos[0] = pos.x;
			v.pos[1] = pos.y;
			v.pos[2] = pos.z;

			if (!vertNrm.empty())
			{
				Vec3 n;
				memcpy(&n, &vertNrm[i * 3], 12);
				XMFLOAT4X4 itn = mat3_inv_transpose(vertMat);

				n = mat3_transform(&itn, n);
				n.Normalise();

				v.nrm[0] = n.x;
				v.nrm[1] = n.y;
				v.nrm[2] = n.z;
			}

			if (!uvData.empty())
			{
				v.uv[0] = uvData[i * 2];
				v.uv[1] = 1.0f - uvData[i * 2 + 1];
			}

			if (!colorData.empty())
			{
				v.col[0] = colorData[i * 4];
				v.col[1] = colorData[i * 4 + 1];
				v.col[2] = colorData[i * 4 + 2];
				v.col[3] = colorData[i * 4 + 3];
			}

			if (hasSkin)
			{
				meshFlags |= EMF_Skinned;
				float wts[4];
				memcpy(wts, skin.boneWgt.data() + i * 16, 16);
				unsigned char ID[4];
				memcpy(ID, skin.boneID.data() + i * 4, 4);

				for (int w = 0; w < 4; w++)
				{
					if (wts[w] > 0 && rwVer < 0x34000 && skin.maxWeights == 0)
						ID[w] = (unsigned char)skin.usedBoneList[ID[w]];
				}
				memcpy(v.bi, ID, 4);
				memcpy(v.bw, wts, 16);
			}
			verts.push_back(v);
		}
		if (isBM)
		{
			for (int sp = 0; sp < bm.numSplits; sp++)
			{
				int matID = bm.matIDs[sp];
				unsigned int ff = (unsigned int)faces.size();
				unsigned int fc_count = 0;
				for (int f = 0; f < (int)bm.faces.size(); f++)
				{
					if (bm.faceMatID[f] == matID)
					{
						MKFace df;
						df.v[0] = bm.faces[f].v[0] + vertBase;
						df.v[1] = bm.faces[f].v[1] + vertBase;
						df.v[2] = bm.faces[f].v[2] + vertBase;
						faces.push_back(df);
						fc_count++;
					}
				}
				if (fc_count)
				{
					MKMeshGroup sub;
					sub.matID = matBase + matID;
					sub.firstFace = ff;
					sub.numFaces = fc_count;
					groups.push_back(sub);
				}
			}
		}
	}

	// mkd/mka
	if (nativeFlags == 1 && geoStruct.size > 40 && isBM)
	{
		int splitCount = bm.numSplits;

		unsigned int nativeChunkStart = (unsigned int)pFile.tellg();
		unsigned int nativeChunkSize = 0;
		for (int i = 0; i < splitCount; i++)
		{
			unsigned int dsize = 0;
			pFile.read((char*)&dsize, sizeof(unsigned int));
			unsigned int mtype;
			pFile.read((char*)&mtype, sizeof(unsigned int));
			pFile.seekg(dsize, pFile.cur);
			nativeChunkSize += dsize + 8;
		}

		MKSkinData nativeSkin;
		bool hasNativeSkin = false;
		if (MKSkinFlag)
		{
			int skinDataSz = (int)(geoStruct.size - 40 - nativeChunkSize);
			if (skinDataSz > 0 && skinDataSz < (int)geoStruct.size)
			{
				std::vector<char> nsd(skinDataSz);
				pFile.read(nsd.data(), skinDataSz);
				nativeSkin = parse_skin_native(nsd);
				hasNativeSkin = true;
			}
		}
		unsigned int afterSkin = (unsigned int)pFile.tellg();

		pFile.seekg(nativeChunkStart, pFile.beg);
		meshFlags |= EMF_Normals;

		for (int sp = 0; sp < splitCount; sp++)
		{
			unsigned int dataSize = 0;
			pFile.read((char*)&dataSize, sizeof(unsigned int));
			unsigned int meshType = 0;
			pFile.read((char*)&meshType, sizeof(unsigned int));
			unsigned int endOfs = (unsigned int)pFile.tellg() + dataSize;

			// ssf type padding (128, PAD128)
			unsigned int cur = (unsigned int)pFile.tellg();
			unsigned int padLen1 = ((cur + 127) & ~127u) - cur;
			pFile.seekg(padLen1, pFile.cur);
			int vifSize = dataSize - 128;
			std::vector<char> vifData(vifSize);
			pFile.read(vifData.data(), vifSize);
			pFile.seekg(endOfs - (unsigned int)pFile.tellg(), pFile.cur);

			VIFList vifs = UnpackVIFCode(vifData.data(), vifSize);

			int matID = bm.matIDs[sp];
			int skinFlag = MKSkinFlag;
			int* bonePal = nullptr;
			int useBP = 0;
			int count0x71 = 4;

			int mtype = pmats[matID].mkMeshType;
			// 128 - most skinned models, 129 - uv map scroll active? doesnt work without 129
			if (mtype == 128 || mtype == 129)
			{
				count0x71 = 4;
				skinFlag = 1;
				useBP = pmats[matID].mkUseBonePalette;
				if (useBP) bonePal = pmats[matID].mkBonePalette;
			}
			// skinned model with no uv, reflection models, supports vertex colors
			else if (mtype == 130)
			{
				skinFlag = 1;
				useBP = pmats[matID].mkUseBonePalette;
				if (useBP)
					bonePal = pmats[matID].mkBonePalette;
			}
			// skinned model with no bone palette
			else if (mtype == 131)
			{
				count0x71 = 4;
				skinFlag = 1;
				bonePal = (int*)frameList.hSkinBoneID;
			}
			// generic model, stages etc
			else if (mtype == 132)
			{
				count0x71 = 0;
				skinFlag = 0;
			}
			// generic model skinned
			else if (mtype == 137)
			{
				count0x71 = 2;
				skinFlag = 0;
			}

			std::vector<TriStripReference> vertDatas, skinChunks, nouv6E;
			std::vector<std::vector<float>> uvDatasV, normDatasV;
			std::vector<TriStripReference> colorDatasN;
			std::vector<VertexIDList> vidList1, vidList2;
			std::vector<VertexIDListShared> sharedVids;
			std::vector<int> sharedTargetStrip;
			std::vector<int> realNumVerts;
			int mapInterval = 0;
			int local0x71 = count0x71;
			int vertStripID = -1;

			for (int vi = 0; vi < (int)vifs.items.size(); vi++)
			{
				VIFUnpack& vifUp = vifs.items[vi];
				int vifUpSize = (int)vifUp.data.size();
				char* vifUpBuff = (char*)vifUp.data.data();

				if (vifUp.numElems == 3 && vifUp.elemBits == 32)  // 0x68 vertex
				{
					vertStripID++;
					vertDatas.push_back({ vifUpBuff, vifUpSize });
				}
				else if (vifUp.numElems == 2 && vifUp.elemBits == 16) // 0x65 UV short
				{
					int nv2 = vifUpSize / 4;
					uvDatasV.push_back(decode_uv16(vifUpBuff, nv2));
					meshFlags |= EMF_UV;
				}
				else if (vifUp.numElems == 2 && vifUp.elemBits == 32) // 0x64 UV float
				{
					int nv2 = vifUpSize / 8;
					std::vector<float> uv(nv2 * 2);
					memcpy(uv.data(), vifUpBuff, nv2 * 8);
					uvDatasV.push_back(std::move(uv));
					meshFlags |= EMF_UV;
				}
				else if (vifUp.numElems == 4 && vifUp.elemBits == 32) // 0x6C uv + uv2
				{
					if (vifUpSize > 16)
					{
						int nv2 = vifUpSize / 16;
						std::vector<float> uv(nv2 * 2);
						for (int u = 0; u < nv2; u++) memcpy(&uv[u * 2], vifUpBuff + u * 16, 8);
						uvDatasV.push_back(std::move(uv));
						meshFlags |= EMF_UV;
					}
				}
				else if (vifUp.numElems == 3 && vifUp.elemBits == 8) // 0x6A normals
				{
					if ((int)normDatasV.size() < (int)vertDatas.size())
					{
						int nv2 = vifUpSize / 3;
						normDatasV.push_back(decode_normals(vifUpBuff, nv2));
					}
				}
				else if (vifUp.numElems == 1 && vifUp.elemBits == 16)   // 0x71
				{
					if (skinFlag)
					{
						if (local0x71 % 4 == 0)
							vidList1.push_back(decode_vidlist_skin(vifUpBuff, vifUpSize, useBP));
						if (local0x71 % 4 == 3)
							vidList2.push_back(decode_vidlist_skin(vifUpBuff, vifUpSize, useBP));
						if (local0x71 % 4 == 1 || local0x71 % 4 == 2)
							skinChunks.push_back({ vifUpBuff, vifUpSize });
					}
					else
					{
						if (local0x71 % 2 == 0)
							vidList1.push_back(decode_vidlist_map(vifUpBuff, vifUpSize, &mapInterval));
						if (local0x71 % 2 == 1)
							vidList2.push_back(decode_vidlist_map(vifUpBuff, vifUpSize, &mapInterval));
					}
					local0x71++;
				}
				else if (vifUp.numElems == 4 && vifUp.elemBits == 8) // 0x6E
				{
					if (skinFlag)
					{
						if (vifUpSize > 8) nouv6E.push_back({ vifUpBuff, vifUpSize });
						else realNumVerts.push_back((unsigned char)vifUpBuff[3]);
					}
					else if (vifUpSize > 4)
					{
						colorDatasN.push_back({ vifUpBuff, vifUpSize / 4 * 4 });
						meshFlags |= EMF_VertexColor;
					}
					else if (vifUpSize == 4)
					{
						realNumVerts.push_back((unsigned char)vifUpBuff[3]);
					}
				}
				else if (vifUp.numElems == 4 && vifUp.elemBits == 16) // 0x6D shared
				{
					if (skinFlag)
						sharedVids.push_back(decode_shared_vidlist_skin(vifUpBuff, vifUpSize, useBP));
					else
						sharedVids.push_back(decode_shared_vidlist_map(vifUpBuff, vifUpSize, mapInterval));
					sharedTargetStrip.push_back(vertStripID + 1);
				}
			}


			int numVB = (int)vertDatas.size();
			std::vector<std::vector<unsigned char>> blockBoneIDs;
			std::vector<std::vector<float>> blockWeights;

			if (!vidList1.empty() && skinFlag)
			{
				blockBoneIDs.resize(numVB);
				for (int b = 0; b < numVB; b++)
				{
					int nv2 = vertDatas[b].size / 12;
					blockBoneIDs[b].resize(nv2 * 3);
					int* pal = bonePal ? bonePal : (int*)frameList.hSkinBoneID;
					for (int j = 0; j < nv2; j++)
					{
						unsigned int bx, by, bz;
						memcpy(&bx, vertDatas[b].data + j * 12, 4);
						memcpy(&by, vertDatas[b].data + j * 12 + 4, 4);
						memcpy(&bz, vertDatas[b].data + j * 12 + 8, 4);
						bx &= 0x3FF; by &= 0x3FF; bz &= 0x3FF;

						blockBoneIDs[b][j * 3 + 0] = FindBone(bx, pal);
						blockBoneIDs[b][j * 3 + 1] = FindBone(by, pal);
						blockBoneIDs[b][j * 3 + 2] = FindBone(bz, pal);
					}
				}

				int numWBlocks = (int)skinChunks.size() / 2;
				blockWeights.resize(numVB);
				for (int b = 0; b < numWBlocks && b < numVB; b++)
				{
					int nv2 = skinChunks[b * 2].size / 2;
					blockWeights[b].resize(nv2 * 3);
					for (int j = 0; j < nv2; j++)
					{
						short w1, w2;
						memcpy(&w1, skinChunks[b * 2].data + j * 2, 2);
						memcpy(&w2, skinChunks[b * 2 + 1].data + j * 2, 2);
						float fw1 = w1 / 4096.0f, fw2 = w2 / 4096.0f, fw3 = 0;
						if (w1 + w2 < 4095) fw3 = 1.0f - fw1 - fw2;
						blockWeights[b][j * 3] = fw1; blockWeights[b][j * 3 + 1] = fw2; blockWeights[b][j * 3 + 2] = fw3;
					}
				}
				meshFlags |= EMF_Skinned;
			}

			unsigned int splitFirstFace = (unsigned int)faces.size();
			unsigned int splitFaceCount = 0;


			std::vector<std::vector<float>> prevVerts(numVB), prevNorms(numVB), prevUVs(numVB), prevWgts(numVB);
			std::vector<std::vector<unsigned char>> prevBIDs(numVB);

			for (int v = 0; v < numVB; v++)
			{
				int rawNV = vertDatas[v].size / 12;
				if (rawNV <= 2)
					continue;

				std::vector<float> rawVerts(rawNV * 3);
				memcpy(rawVerts.data(), vertDatas[v].data, rawNV * 12);
				std::vector<float> rawNorms2;
				if (v < (int)normDatasV.size()) rawNorms2 = normDatasV[v];
				std::vector<float> rawUVs;
				if (v < (int)uvDatasV.size()) rawUVs = uvDatasV[v];

				std::vector<float> finalVerts, finalNorms_f, finalUVs_f, finalWgts;
				std::vector<unsigned char> finalBIDs, finalCols;
				std::vector<int> skipList;
				int finalNV;

				if (!vidList1.empty() && v < (int)vidList1.size())
				{
					int vc = (v < (int)realNumVerts.size()) ? realNumVerts[v] : rawNV;
					if (!skinFlag)
					{
						int maxID = 0;
						for (int j = 0; j < (int)vidList1[v].ids.size(); j++)
						{
							if (vidList1[v].ids[j] > maxID)
								maxID = vidList1[v].ids[j];
						}

						if (v < (int)vidList2.size())
						{
							for (int j = 0; j < (int)vidList2[v].ids.size(); j++)
							{
								if (vidList2[v].ids[j] > maxID)
								{
									maxID = vidList2[v].ids[j];
								}
							}
						}
						if (v > 0)
						{
							for (int si = 0; si < (int)sharedVids.size(); si++)
							{
								if (sharedTargetStrip[si] == v)
								{
									for (int j = 0; j < (int)sharedVids[si].curIDs.size(); j++)
									{
										if (sharedVids[si].curIDs[j] > maxID)
											maxID = sharedVids[si].curIDs[j];
									}
								}
							}
						}
						vc = maxID + 1;
					}

					finalNV = vc;
					finalVerts.resize(vc * 3, 0);
					finalNorms_f.resize(vc * 3, 0);
					finalUVs_f.resize(vc * 2, 0);
					skipList.resize(vc, 1);

					if (!blockBoneIDs.empty())
					{
						finalBIDs.resize(vc * 3, 0);
						finalWgts.resize(vc * 3, 0);
					}

					if (v < (int)colorDatasN.size())
						finalCols.resize(vc * 4, 0);

					int normNV = (v < (int)normDatasV.size()) ? (int)normDatasV[v].size() / 3 : 0;
					int uvNV = (v < (int)uvDatasV.size()) ? (int)uvDatasV[v].size() / 2 : 0;
					int colNV = (v < (int)colorDatasN.size()) ? colorDatasN[v].size / 4 : 0;

					// TODO: seperate function somehow

					for (int j = 0; j < (int)vidList1[v].ids.size() && j < rawNV; j++)
					{
						int vid = vidList1[v].ids[j];
						if (vid >= 0 && vid < vc)
						{
							memcpy(&finalVerts[vid * 3], &rawVerts[j * 3], 12);
							if (!rawNorms2.empty() && j < normNV)
								memcpy(&finalNorms_f[vid * 3], &rawNorms2[j * 3], 12);

							if (!rawUVs.empty() && j < uvNV)
								memcpy(&finalUVs_f[vid * 2], &rawUVs[j * 2], 8);
							skipList[vid] = vidList1[v].skipFlags[j];

							if (!finalBIDs.empty() && v < (int)blockBoneIDs.size() && !blockBoneIDs[v].empty())
								memcpy(&finalBIDs[vid * 3], &blockBoneIDs[v][j * 3], 3);
							if (!finalWgts.empty() && v < (int)blockWeights.size() && !blockWeights[v].empty())
								memcpy(&finalWgts[vid * 3], &blockWeights[v][j * 3], 12);
							if (!finalCols.empty() && j < colNV)
								memcpy(&finalCols[vid * 4], colorDatasN[v].data + j * 4, 4);
						}
					}
					if (v < (int)vidList2.size())
					{
						for (int j = 0; j < (int)vidList2[v].ids.size() && j < rawNV; j++)
						{
							int vid = vidList2[v].ids[j];
							if (vid >= 0 && vid < vc)
							{
								memcpy(&finalVerts[vid * 3], &rawVerts[j * 3], 12);
								if (!rawNorms2.empty() && j < normNV)
									memcpy(&finalNorms_f[vid * 3], &rawNorms2[j * 3], 12);

								if (!rawUVs.empty() && j < uvNV)
									memcpy(&finalUVs_f[vid * 2], &rawUVs[j * 2], 8);
								skipList[vid] = vidList2[v].skipFlags[j];

								if (!finalBIDs.empty() && v < (int)blockBoneIDs.size() && !blockBoneIDs[v].empty())
									memcpy(&finalBIDs[vid * 3], &blockBoneIDs[v][j * 3], 3);
								if (!finalWgts.empty() && v < (int)blockWeights.size() && !blockWeights[v].empty())
									memcpy(&finalWgts[vid * 3], &blockWeights[v][j * 3], 12);
								if (!finalCols.empty() && j < colNV)
									memcpy(&finalCols[vid * 4], colorDatasN[v].data + j * 4, 4);
							}
						}
					}

					if (v > 0)
					{
						for (int si = 0; si < (int)sharedVids.size(); si++)
						{
							if (sharedTargetStrip[si] == v && !prevVerts[v - 1].empty())
							{
								auto& sv = sharedVids[si];
								for (int j = 0; j < (int)sv.prevIDs.size(); j++)
								{
									int prevVid = sv.prevIDs[j], curVid = sv.curIDs[j];
									if (curVid >= 0 && curVid < vc && prevVid >= 0)
									{
										memcpy(&finalVerts[curVid * 3], &prevVerts[v - 1][prevVid * 3], 12);

										if (!prevNorms[v - 1].empty())
											memcpy(&finalNorms_f[curVid * 3], &prevNorms[v - 1][prevVid * 3], 12);

										if (!prevUVs[v - 1].empty())
											memcpy(&finalUVs_f[curVid * 2], &prevUVs[v - 1][prevVid * 2], 8);

										skipList[curVid] = sv.skipPrev[j];

										if (!prevBIDs[v - 1].empty())
											memcpy(&finalBIDs[curVid * 3], &prevBIDs[v - 1][prevVid * 3], 3);

										if (!prevWgts[v - 1].empty())
											memcpy(&finalWgts[curVid * 3], &prevWgts[v - 1][prevVid * 3], 12);
									}
								}
							}
						}
					}

					prevVerts[v] = finalVerts;
					prevNorms[v] = finalNorms_f;

					if (!rawUVs.empty())
						prevUVs[v] = finalUVs_f;
					if (!finalBIDs.empty())
						prevBIDs[v] = finalBIDs;
					if (!finalWgts.empty())
						prevWgts[v] = finalWgts;

				}
				else if (!nouv6E.empty() && v < (int)nouv6E.size())
				{
					MKFaceSetNoUV nr = create_trilist_nouv(nouv6E[v].data, nouv6E[v].size, bonePal ? bonePal : (int*)frameList.hSkinBoneID, &frameList);
					finalVerts = std::move(rawVerts);
					finalNorms_f = std::move(rawNorms2);
					finalNV = rawNV;

					transform_verts_ps2(finalVerts, finalNV, vertMat, skinFlag);
					if (!finalNorms_f.empty())
						transform_normals_ps2(finalNorms_f, finalNV, vertMat);
					unsigned int vbase = (unsigned int)verts.size();
					for (int j = 0; j < finalNV; j++)
					{
						MKVert ov = {};
						ov.pos[0] = finalVerts[j * 3];
						ov.pos[1] = finalVerts[j * 3 + 1];
						ov.pos[2] = finalVerts[j * 3 + 2];

						if (!finalNorms_f.empty())
						{
							ov.nrm[0] = finalNorms_f[j * 3];
							ov.nrm[1] = finalNorms_f[j * 3 + 1];
							ov.nrm[2] = finalNorms_f[j * 3 + 2];
						}
						ov.bi[0] = (unsigned char)nr.boneIDs[j];
						ov.bw[0] = 1.0f;
						meshFlags |= EMF_Skinned;
						verts.push_back(ov);
					}
					for (int j = 0; j < (int)nr.faces.size(); j++)
					{
						MKFace df;
						df.v[0] = nr.faces[j].v[0] + vbase;
						df.v[1] = nr.faces[j].v[1] + vbase;
						df.v[2] = nr.faces[j].v[2] + vbase;
						faces.push_back(df);
						splitFaceCount++;
					}
					continue;

				}
				else
				{
					finalVerts = std::move(rawVerts);
					finalNorms_f = std::move(rawNorms2);
					finalUVs_f = std::move(rawUVs);
					finalNV = rawNV;
				}

				transform_verts_ps2(finalVerts, finalNV, vertMat, skinFlag);
				if (!finalNorms_f.empty())
					transform_normals_ps2(finalNorms_f, finalNV, vertMat);

				unsigned int vbase = (unsigned int)verts.size();
				for (int j = 0; j < finalNV; j++)
				{
					MKVert ov = {};
					ov.pos[0] = finalVerts[j * 3];
					ov.pos[1] = finalVerts[j * 3 + 1];
					ov.pos[2] = finalVerts[j * 3 + 2];
					if (!finalNorms_f.empty())
					{
						ov.nrm[0] = finalNorms_f[j * 3];
						ov.nrm[1] = finalNorms_f[j * 3 + 1];
						ov.nrm[2] = finalNorms_f[j * 3 + 2];
					}
					if (!finalUVs_f.empty())
					{
						ov.uv[0] = finalUVs_f[j * 2];
						ov.uv[1] = 1.0f - finalUVs_f[j * 2 + 1];
					}
					if (!finalCols.empty())
					{
						ov.col[0] = finalCols[j * 4];
						ov.col[1] = finalCols[j * 4 + 1];
						ov.col[2] = finalCols[j * 4 + 2];
						ov.col[3] = finalCols[j * 4 + 3];
					}
					if (!finalBIDs.empty())
					{
						ov.bi[0] = finalBIDs[j * 3]; ov.bi[1] = finalBIDs[j * 3 + 1]; ov.bi[2] = finalBIDs[j * 3 + 2];

						if (!finalWgts.empty())
						{
							ov.bw[0] = finalWgts[j * 3];
							ov.bw[1] = finalWgts[j * 3 + 1];
							ov.bw[2] = finalWgts[j * 3 + 2];
						}
					}
					verts.push_back(ov);
				}

				int faceStart = (int)faces.size();
				if (!skipList.empty())
				{
					create_trilist(skipList, finalNV, faces, vbase);
				}
				else
				{
					create_tristrip_simple(finalVerts, finalNV, faces, vbase);
				}
				if (!finalNorms_f.empty() && (int)faces.size() > faceStart)
				{
					fix_face_normals(faces, faceStart, finalVerts, finalNorms_f, vbase);
				}
				splitFaceCount += (unsigned int)faces.size() - faceStart;
			}

			if (splitFaceCount > 0)
			{
				MKMeshGroup sub;
				sub.matID = matBase + matID;
				sub.firstFace = splitFirstFace;
				sub.numFaces = splitFaceCount;
				groups.push_back(sub);
			}
		}
		pFile.seekg(afterSkin, pFile.beg);
	}
}

void MKModelTool::DumpMesh(std::string path)
{
	std::ofstream oFile(path, std::ofstream::binary);

	MKModelHeader model;
	ZeroMemory(&model, sizeof(MKModelHeader));
	model.header = 'MESH';
	model.version = 4;
	model.numVertexes = (unsigned int)verts.size();
	model.numFaces = (unsigned int)faces.size();
	model.numBones = (unsigned int)bones.size();
	model.numMaterials = (unsigned int)mats.size();
	model.flags = meshFlags;
	model.numGroups = (unsigned int)groups.size();

	oFile.write((char*)&model, sizeof(MKModelHeader));

	for (auto& b : bones)
	{
		oFile.write((char*)&b, sizeof(MKModelBone));
	}
	for (auto& mat : mats)
	{
		oFile.write((char*)&mat, sizeof(MKMaterial));
	}

	for (unsigned int i = 0; i < verts.size(); i++)
	{
		MKVert& v = verts[i];
		oFile.write((char*)v.pos, 12);
		if (meshFlags & EMF_Normals)
			oFile.write((char*)v.nrm, sizeof(v.nrm));
		if (meshFlags & EMF_UV)
			oFile.write((char*)v.uv, sizeof(v.uv));
		if (meshFlags & EMF_VertexColor)
			oFile.write((char*)v.col, sizeof(v.col));
		if (meshFlags & EMF_Skinned)
		{
			oFile.write((char*)v.bi, sizeof(v.bi));
			oFile.write((char*)v.bw, sizeof(v.bw));
		}
	}
	for (auto& sub : groups)
	{
		oFile.write((char*)&sub, sizeof(MKMeshGroup));
	}
	for (auto& f : faces)
	{
		oFile.write((char*)&f, sizeof(MKFace));
	}


	unsigned int hc = (unsigned int)hanimCount;
	oFile.write((char*)&hc, 4);

	for (int i = 0; i < hanimCount; i++)
	{
		oFile.write((char*)&hanimBoneID[i], 4);
		oFile.write((char*)&hanimSkinBoneID[i], 4);
		oFile.write((char*)&hanimBoneType[i], 4);
	}

	oFile.close();
}

void MKModelTool::DumpDFF(std::string path, EExportType et)
{
	std::ofstream oFile(path, std::ofstream::binary);

	unsigned char pad = 0x00;

	unsigned int rwVer = 0x1803FFFF;
	int skin = (meshFlags & EMF_Skinned) ? 1 : 0;
	int mt = 137;

	if (et == Export_SkinnedPalette)
		mt = skin ? 128 : 137;
	else if (et == Export_Skinned)
		mt = skin ? 131 : 137;
	else if (et == Export_SkinnedNoUV)
		mt = 130;
	if (et == Export_Generic)
		skin = 0;

	std::vector<HAnimEntry> haList;
	if (hanimCount > 0)
	{
		for (int i = 0; i < hanimCount; i++)
		{
			HAnimEntry h;
			unsigned int packed = (unsigned int)hanimBoneID[i];
			h.boneID = (int)(packed & 0xFFFF);
			h.groupID = (int)((packed >> 16) & 0xFFFF);
			h.skinBoneID = hanimSkinBoneID[i];
			h.boneType = hanimBoneType[i];
			h.frameID = -1;

			int lookupID = h.boneID;
			if (lookupID >= 0x8000)
				lookupID -= 0x8000;

			for (int fi = 0; fi < (int)bones.size(); fi++)
			{
				if (bones[fi].id == lookupID)
				{
					h.frameID = fi;
					break;
				}
			}

			haList.push_back(h);
		}
	}
	else
	{
		for (unsigned int i = 0; i < bones.size(); i++)
		{
			if (bones[i].id >= 0)
			{
				HAnimEntry h;
				h.boneID = bones[i].id;
				h.skinBoneID = bones[i].skinBoneID;
				h.boneType = bones[i].boneType;
				h.groupID = bones[i].groupID;
				h.frameID = i;
				haList.push_back(h);
			}
		}
	}
	unsigned int haCnt = haList.size();

	bool hasHAnim = false;
	for (unsigned int i = 0; i < bones.size(); i++)
	{
		if (bones[i].id >= 0)
		{
			hasHAnim = true;
			break;
		}
	}

	unsigned int frameCount = bones.size();

	int hAnimRoot = -1;
	for (int i = 0; i < (int)bones.size(); i++)
	{
		if (bones[i].parent == 0 && bones[i].id == -1 && i > 0)
		{
			hAnimRoot = i;
			break;
		}
	}
	if (hAnimRoot < 0 && hasHAnim && bones.size() > 1)
		hAnimRoot = 1;

	int nTex = 0;
	for (int i = 0; i < (int)mats.size(); i++)
	{
		if (mats[i].hasTexture)
			nTex++;
	}

	// tex start/end
	int tmp = 0;
	oFile.write((char*)&tmp, sizeof(int));
	tmp = nTex > 0 ? nTex - 1 : 0;
	oFile.write((char*)&tmp, sizeof(int));


	unsigned int clO = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_CLUMP, rwVer, 0);
	unsigned int clD = (unsigned int)oFile.tellp();

	unsigned int utmp;
	writeChunk(oFile, RW_STRUCT, rwVer, 12);
	utmp = 1;
	oFile.write((char*)&utmp, sizeof(int));
	utmp = 0;
	oFile.write((char*)&utmp, sizeof(int));
	oFile.write((char*)&utmp, sizeof(int));

	unsigned int flO = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_FRL, rwVer, 0);
	unsigned int flD = (unsigned int)oFile.tellp();

	unsigned int flS = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_STRUCT, rwVer, 0);
	oFile.write((char*)&frameCount, sizeof(int));

	std::vector<XMFLOAT4X4> wm;
	wm.resize(bones.size());

	for (int i = 0; i < (int)bones.size(); i++)
		wm[i] = MatrixFromBone(bones[i].mat);

	for (int i = 0; i < (int)bones.size(); i++)
	{
		int par = bones[i].parent;
		if (par >= 0 && par < (int)bones.size())
		{
			XMFLOAT4X4 pi = MatrixInvAffine(&wm[par]);
			XMFLOAT4X4 loc = mat4_mul(&pi, &wm[i]);
			writeRWMatrix(oFile, &loc);
		}
		else
		{
			writeRWMatrix(oFile, &wm[i]);
		}
		oFile.write((char*)&par, sizeof(int));
		utmp = 3;
		oFile.write((char*)&utmp, sizeof(int));
	}
	adjustInt(oFile, flS + 4, (unsigned int)((unsigned int)oFile.tellp() - flS - 12));

	for (int i = 0; i < (int)bones.size(); i++)
	{
		if (hasHAnim && i == hAnimRoot)
		{
			unsigned int eO = (unsigned int)oFile.tellp();
			writeChunk(oFile, RW_EXT, rwVer, 0);
			unsigned int eD = (unsigned int)oFile.tellp();

			writeChunk(oFile, RW_HANIM, rwVer, 20 + 12 * haCnt);
			utmp = 256;
			oFile.write((char*)&utmp, sizeof(int));
			tmp = -1;
			oFile.write((char*)&tmp, sizeof(int));
			oFile.write((char*)&haCnt, sizeof(int));
			utmp = 0;
			oFile.write((char*)&utmp, sizeof(int));
			utmp = 36;
			oFile.write((char*)&utmp, sizeof(int));

			for (unsigned int h = 0; h < haCnt; h++)
			{
				unsigned short bid = (unsigned short)(haList[h].boneID & 0xFFFF);
				unsigned short gid = (unsigned short)(haList[h].groupID & 0xFFFF);
				oFile.write((char*)&bid, sizeof(short));
				oFile.write((char*)&gid, sizeof(short));
				oFile.write((char*)&haList[h].skinBoneID, sizeof(int));
				oFile.write((char*)&haList[h].boneType, sizeof(int));
			}
			adjustInt(oFile, eO + 4, (unsigned int)((unsigned int)oFile.tellp() - eD));
		}
		else if (hasHAnim && bones[i].id >= 0)
		{
			writeChunk(oFile, RW_EXT, rwVer, 24);
			writeChunk(oFile, RW_HANIM, rwVer, 12);
			utmp = 256;
			oFile.write((char*)&utmp, sizeof(int));
			oFile.write((char*)&bones[i].id, sizeof(int));
			utmp = 0;
			oFile.write((char*)&utmp, sizeof(int));
		}
		else
		{
			writeChunk(oFile, RW_EXT, rwVer, 0);
		}
	}
	adjustInt(oFile, flO + 4, (unsigned int)((unsigned int)oFile.tellp() - flD));

	unsigned int glO = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_GEOL, rwVer, 0);
	unsigned int glD = (unsigned int)oFile.tellp();

	writeChunk(oFile, RW_STRUCT, rwVer, sizeof(int));
	utmp = 1;
	oFile.write((char*)&utmp, sizeof(int));

	unsigned int gO = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_GEO, rwVer, 0);
	unsigned int gD = (unsigned int)oFile.tellp();

	unsigned int gS = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_STRUCT, rwVer, 0);

	unsigned short gfl = 0x01 | 0x02 | 0x10 | 0x20;

	if ((meshFlags & EMF_UV) && mt != 130)
		gfl |= 0x04;
	if (skin)
		gfl |= 0x0100;

	unsigned char numUV = 1, nativeF = 1;
	oFile.write((char*)&gfl, sizeof(short));
	oFile.write((char*)&numUV, sizeof(char));
	oFile.write((char*)&nativeF, sizeof(char));
	utmp = (unsigned int)faces.size();
	oFile.write((char*)&utmp, sizeof(int));
	utmp = (unsigned int)verts.size();
	oFile.write((char*)&utmp, sizeof(int));
	tmp = 1;
	oFile.write((char*)&tmp, sizeof(int));

	{
		float mnx = FLT_MAX, mny = FLT_MAX, mnz = FLT_MAX;
		float mxx = -FLT_MAX, mxy = -FLT_MAX, mxz = -FLT_MAX;
		for (int i = 0; i < (int)verts.size(); i++)
		{
			float* p = verts[i].pos;
			if (p[0] < mnx) mnx = p[0];
			if (p[0] > mxx) mxx = p[0];
			if (p[1] < mny) mny = p[1];
			if (p[1] > mxy) mxy = p[1];
			if (p[2] < mnz) mnz = p[2];
			if (p[2] > mxz) mxz = p[2];
		}
		float cx = (mxx + mnx) * 0.5f;
		float cy = (mxy + mny) * 0.5f;
		float cz = (mxz + mnz) * 0.5f;
		float dx = mxx - mnx, dy = mxy - mny, dz = mxz - mnz;
		float rad = sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f;
		oFile.write((char*)&cx, sizeof(float));
		oFile.write((char*)&cy, sizeof(float));
		oFile.write((char*)&cz, sizeof(float));
		oFile.write((char*)&rad, sizeof(float));
	}
	tmp = 0;
	oFile.write((char*)&tmp, sizeof(int));
	oFile.write((char*)&tmp, sizeof(int));

	unsigned int matCount = (unsigned int)mats.size();
	unsigned int mlO = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_MATL, rwVer, 0);
	unsigned int mlD = (unsigned int)oFile.tellp();

	writeChunk(oFile, RW_STRUCT, rwVer, 4 + matCount * 4);
	oFile.write((char*)&matCount, sizeof(int));
	tmp = -1;

	for (unsigned int i = 0; i < matCount; i++)
		oFile.write((char*)&tmp, sizeof(int));

	for (int mi = 0; mi < (int)mats.size(); mi++)
	{
		MKMaterial& m = mats[mi];

		unsigned int mO = (unsigned int)oFile.tellp();
		writeChunk(oFile, RW_MAT, rwVer, 0);
		unsigned int mD = (unsigned int)oFile.tellp();

		writeChunk(oFile, RW_STRUCT, rwVer, 28);
		tmp = 0;
		oFile.write((char*)&tmp, sizeof(int));
		tmp = -1;
		oFile.write((char*)&tmp, sizeof(int));
		tmp = 0;
		oFile.write((char*)&tmp, sizeof(int));
		oFile.write((char*)&m.hasTexture, sizeof(int));
		oFile.write((char*)&m.rw_ambient, sizeof(float));
		oFile.write((char*)&m.rw_specular, sizeof(float));
		oFile.write((char*)&m.rw_diffuse, sizeof(float));

		if (m.hasTexture)
		{
			unsigned int tO = (unsigned int)oFile.tellp();
			writeChunk(oFile, RW_TEX, rwVer, 0);
			unsigned int tD = (unsigned int)oFile.tellp();

			writeChunk(oFile, RW_STRUCT, rwVer, 4);
			tmp = m.texFilter | ((m.vAddressing << 4 | m.uAddressing) << 8) | (1 << 16);
			oFile.write((char*)&tmp, sizeof(int));

			int nl = (int)strlen(m.tex);
			int np = (4 - ((nl + 1) % 4)) % 4;
			writeChunk(oFile, RW_STRING, rwVer, nl + 1 + np);
			oFile.write(m.tex, nl);

			for (int p = 0; p <= np; p++)
				oFile.write((char*)&pad, sizeof(char));

			writeChunk(oFile, RW_STRING, rwVer, 4);
			utmp = 0;
			oFile.write((char*)&utmp, sizeof(int));

			writeChunk(oFile, RW_EXT, rwVer, 16);
			writeChunk(oFile, RW_SMIP, rwVer, 4);
			utmp = (m.skyMipmapL << 12) | (m.skyMipmapK & 0xFFF);
			oFile.write((char*)&utmp, sizeof(int));

			adjustInt(oFile, tO + 4, (unsigned int)((unsigned int)oFile.tellp() - tD));
		}

		unsigned int eO = (unsigned int)oFile.tellp();
		writeChunk(oFile, RW_EXT, rwVer, 0);
		unsigned int eD = (unsigned int)oFile.tellp();

		unsigned int kO = (unsigned int)oFile.tellp();
		writeChunk(oFile, RW_MKMAT, rwVer, 0);
		unsigned int kD = (unsigned int)oFile.tellp();

		int mkf = m.mkType;
		if (skin && (et == Export_SkinnedPalette || et == Export_SkinnedNoUV))
			mkf |= 0x60000000;
		if (m.mkUVScroll)
			mkf |= (int)0x80000000;

		oFile.write((char*)&mkf, sizeof(int));
		oFile.write((char*)&m.mkUnk1, sizeof(short));
		oFile.write((char*)&m.mkUnk2, sizeof(short));
		oFile.write((char*)&m.mkUnk3, sizeof(float));

		if ((mkf & 0x40000000) && m.mkType >= 2)
		{
			int pl[MAX_PALETTE];
			int plc = 0;

			if (m.mkBonePaletteCount > 0)
			{
				plc = m.mkBonePaletteCount;
				if (plc > MAX_PALETTE)
					plc = MAX_PALETTE;
				for (int p = 0; p < plc; p++)
					pl[p] = m.mkBonePalette[p];
			}
			else
			{
				for (int si = 0; si < (int)groups.size(); si++)
				{
					if (groups[si].matID == (unsigned int)mi)
					{
						plc = BuildPalette(groups[si], pl, haList);
						break;
					}
				}
			}

			oFile.write((char*)&plc, sizeof(int));
			for (int p = 0; p < plc; p++)
				oFile.write((char*)&pl[p], sizeof(int));
		}

		if (m.mkType >= 3)
		{
			oFile.write((char*)&m.mkSpecR, sizeof(char));
			oFile.write((char*)&m.mkSpecG, sizeof(char));
			oFile.write((char*)&m.mkSpecB, sizeof(char));
			oFile.write((char*)&m.mkSpecA, sizeof(char));
		}

		if (m.mkType >= 4)
		{
			oFile.write((char*)&m.mkUnk5, sizeof(float));
			oFile.write((char*)&m.mkUnk6, sizeof(float));
			if (mkf & (int)0x80000000)
			{
				float zf = 0;
				oFile.write((char*)&m.mkScrollX, sizeof(float));
				oFile.write((char*)&m.mkScrollY, sizeof(float));
				oFile.write((char*)&m.mkScrollZ, sizeof(float));
				oFile.write((char*)&zf, sizeof(float));
			}
		}

		if (m.mkType >= 5)
		{
			int mmt = m.mkMeshType > 0 ? m.mkMeshType : mt;
			oFile.write((char*)&mmt, sizeof(int));
		}

		adjustInt(oFile, kO + 4, (unsigned int)((unsigned int)oFile.tellp() - kD));
		adjustInt(oFile, eO + 4, (unsigned int)((unsigned int)oFile.tellp() - eD));
		adjustInt(oFile, mO + 4, (unsigned int)((unsigned int)oFile.tellp() - mD));
	}
	adjustInt(oFile, mlO + 4, (unsigned int)((unsigned int)oFile.tellp() - mlD));

	unsigned int geO = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_EXT, rwVer, 0);
	unsigned int geD = (unsigned int)oFile.tellp();


	//NvTriStrip conversion here
	std::vector<NvTriStripGeneratedStrips> strips;
	strips.resize(groups.size());
	int totalID = 0;
	int splitN = 34;

	SetCacheSize(CACHESIZE_GEFORCE1_2);
	SetStitchStrips(true);
	SetMinStripSize(0);
	SetListsOnly(false);

	for (int si = 0; si < (int)groups.size(); si++)
	{
		MKMeshGroup& sub = groups[si];
		std::vector<unsigned short> indices(sub.numFaces * 3);

		for (unsigned int fi = 0; fi < sub.numFaces; fi++)
		{
			indices[fi * 3 + 0] = (unsigned short)faces[sub.firstFace + fi].v[0];
			indices[fi * 3 + 1] = (unsigned short)faces[sub.firstFace + fi].v[1];
			indices[fi * 3 + 2] = (unsigned short)faces[sub.firstFace + fi].v[2];
		}

		PrimitiveGroup* primGroups = nullptr;
		unsigned short numGroups = 0;
		GenerateStrips(indices.data(), (unsigned int)indices.size(), &primGroups, &numGroups);

		if (numGroups > 0 && primGroups[0].type == PT_STRIP)
		{
			strips[si].strip.resize(primGroups[0].numIndices);
			for (unsigned int j = 0; j < primGroups[0].numIndices; j++)
				strips[si].strip[j] = primGroups[0].indices[j];
		}
		else
		{
			strips[si].strip.resize(indices.size());
			for (int j = 0; j < (int)indices.size(); j++)
				strips[si].strip[j] = indices[j];
		}
		delete[] primGroups;

		int sn = (int)strips[si].strip.size();
		strips[si].skip.resize(sn, 0);
		for (int i = 2; i < sn; i++)
		{
			int a = strips[si].strip[i - 2];
			int b2 = strips[si].strip[i - 1];
			int c = strips[si].strip[i];
			strips[si].skip[i] = (a == b2 || b2 == c || c == a) ? 1 : 0;
		}
		totalID += sn;
	}


	unsigned int bmO = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_BMESH, rwVer, 0);
	unsigned int bmD = (unsigned int)oFile.tellp();

	tmp = 1;
	oFile.write((char*)&tmp, sizeof(int));
	utmp = (unsigned int)groups.size();
	oFile.write((char*)&utmp, sizeof(int));
	oFile.write((char*)&totalID, sizeof(int));

	for (int si = 0; si < (int)groups.size(); si++)
	{
		utmp = (unsigned int)strips[si].strip.size();
		oFile.write((char*)&utmp, sizeof(int));
		oFile.write((char*)&groups[si].matID, sizeof(int));
	}
	adjustInt(oFile, bmO + 4, (unsigned int)((unsigned int)oFile.tellp() - bmD));
	adjustInt(oFile, geO + 4, (unsigned int)((unsigned int)oFile.tellp() - geD));

	unsigned int natO = (unsigned int)oFile.tellp();

	for (int si = 0; si < (int)groups.size(); si++)
	{
		MKMeshGroup& sub = groups[si];
		int* str = strips[si].strip.data();
		int sc = (int)strips[si].strip.size();
		int* skl = strips[si].skip.data();

		int bp[MAX_PALETTE];
		int bpc = 0;
		if (skin && et == Export_SkinnedPalette)
		{
			unsigned int matID = sub.matID;
			if (matID < mats.size() && mats[matID].mkBonePaletteCount > 0)
			{
				bpc = mats[matID].mkBonePaletteCount;
				if (bpc > MAX_PALETTE)
					bpc = MAX_PALETTE;
				for (int p = 0; p < bpc; p++)
					bp[p] = mats[matID].mkBonePalette[p];
			}
			else
			{
				bpc = BuildPalette(sub, bp, haList);
			}
		}

		int nch = (sc <= splitN) ? 1 : 1 + ((sc - splitN + splitN - 1) / splitN);


		unsigned int ssO = (unsigned int)oFile.tellp();
		utmp = 0;
		oFile.write((char*)&utmp, sizeof(int));
		utmp = 1;
		oFile.write((char*)&utmp, sizeof(int));
		unsigned int ssD = (unsigned int)oFile.tellp();


		unsigned int p1 = (unsigned int)oFile.tellp() % SEC_FILEPADSIZE;
		if (p1) p1 = SEC_FILEPADSIZE - p1;
		unsigned int p2 = SEC_FILEPADSIZE - p1;
		oFile.write((char*)gDummyData, p1);

		unsigned int dO = (unsigned int)oFile.tellp();
		unsigned short stmp = 0;
		oFile.write((char*)&stmp, sizeof(short));
		stmp = 0x6000;
		oFile.write((char*)&stmp, sizeof(short));
		utmp = 0;
		oFile.write((char*)&utmp, sizeof(int));
		utmp = 0x05000000;
		oFile.write((char*)&utmp, sizeof(int));
		unsigned int dP = (unsigned int)oFile.tellp() + 4;

		int avc = 0;
		for (int ch = 0; ch < nch; ch++)
		{
			int start, csz;
			if (ch == 0)
			{
				start = 0;
				csz = (sc > splitN) ? splitN : sc;
			}
			else
			{
				start = ch * splitN - 2;
				csz = sc - start;
				if (csz > splitN + 2)
					csz = splitN + 2;
			}
			if (csz <= 0)
				break;


			std::vector<int> cv(csz);
			std::vector<int> cs(csz);

			std::copy(str + start, str + start + csz, cv.begin());
			std::copy(skl + start, skl + start + csz, cs.begin());

			std::vector<unsigned short> vidPre;
			vidPre.resize(csz);

			for (int vi = 0; vi < csz; vi++)
			{
				unsigned short ID;
				if (skin)
				{
					if (mt == 128)
						ID = !(ch & 1) ? (vi * 4 + 130) : (vi * 4 + 488);
					else if (mt == 131)
						ID = !(ch & 1) ? (vi * 3 + 191) : (vi * 3 + 634);
					else
						ID = !(ch & 1) ? (vi * 5 + 193) : (vi * 5 + 671);
				}
				else
				{
					ID = !(ch & 1) ? (vi * 5 + 193) : (vi * 5 + 671);
				}

				if (cs[vi])
					ID |= 0x8000;

				vidPre[vi] = ID;
			}


			if (skin && (et == Export_SkinnedPalette || et == Export_Skinned))
			{
				std::vector<int> tv;
				std::vector<int> ts;

				tv.resize(csz);
				ts.resize(csz);

				std::vector<unsigned short> ti;
				ti.resize(csz);

				int ID2 = 0;

				for (int wc = 1; wc <= 3; wc++)
				{
					for (int i = 0; i < csz; i++)
					{
						if (CountWeights(verts[cv[i]]) == wc)
						{
							tv[ID2] = cv[i];
							ts[ID2] = cs[i];
							ti[ID2] = vidPre[i];
							ID2++;
						}
					}
				}
				for (int i = 0; i < csz; i++)
				{
					int nw = CountWeights(verts[cv[i]]);
					if (nw < 1 || nw > 3)
					{
						tv[ID2] = cv[i];
						ts[ID2] = cs[i];
						ti[ID2] = vidPre[i];
						ID2++;
					}
				}
				cv = tv;
				cs = ts;
				vidPre = ti;
			}

			if (et != Export_SkinnedNoUV)
			{
				utmp = 0x01000101;
				oFile.write((char*)&utmp, sizeof(int));

				if (skin)
				{
					int c1 = 0, c2 = 0, c3 = 0;
					for (int i2 = 0; i2 < csz; i2++)
					{
						int nw = CountWeights(verts[cv[i2]]);
						if (nw == 1) c1++;
						else if (nw == 2) c2++;
						else if (nw >= 3) c3++;
					}
					utmp = 0x6E02C000;
					oFile.write((char*)&utmp, sizeof(int));
					unsigned char uc;
					uc = c1;
					oFile.write((char*)&uc, sizeof(char));
					uc = c2;
					oFile.write((char*)&uc, sizeof(char));
					uc = c3;
					oFile.write((char*)&uc, sizeof(char));
					uc = csz;
					oFile.write((char*)&uc, sizeof(char));
					oFile.write((char*)&pad, sizeof(char));
					oFile.write((char*)&pad, sizeof(char));
					oFile.write((char*)&pad, sizeof(char));
					oFile.write((char*)&pad, sizeof(char));
				}
				else
				{
					utmp = 0x6C018000;
					oFile.write((char*)&utmp, sizeof(int));

					float mnx = FLT_MAX, mny = FLT_MAX, mnz = FLT_MAX;
					float mxx = -FLT_MAX, mxy = -FLT_MAX, mxz = -FLT_MAX;
					for (int i = 0; i < csz; i++)
					{
						float* p = verts[cv[i]].pos;
						if (p[0] < mnx) mnx = p[0];
						if (p[0] > mxx) mxx = p[0];
						if (p[1] < mny) mny = p[1];
						if (p[1] > mxy) mxy = p[1];
						if (p[2] < mnz) mnz = p[2];
						if (p[2] > mxz) mxz = p[2];
					}
					float cx = (mxx + mnx) * 0.5f;
					float cy = (mxy + mny) * 0.5f;
					float cz = (mxz + mnz) * 0.5f;
					float dx = mxx - mnx, dy = mxy - mny, dz = mxz - mnz;
					float rad = sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f;
					oFile.write((char*)&cx, sizeof(float));
					oFile.write((char*)&cy, sizeof(float));
					oFile.write((char*)&cz, sizeof(float));
					oFile.write((char*)&rad, sizeof(float));

					utmp = 0x6E01C001;
					oFile.write((char*)&utmp, sizeof(int));
					unsigned char uc;
					uc = csz;
					oFile.write((char*)&uc, sizeof(char));
					oFile.write((char*)&pad, sizeof(char));
					oFile.write((char*)&pad, sizeof(char));
					uc = csz;
					oFile.write((char*)&uc, sizeof(char));
				}
			}

			utmp = 0x01000103;
			oFile.write((char*)&utmp, sizeof(int));

			int addr = 0;
			if (mt == 128) addr = 15;
			else if (mt == 137) addr = 17;
			else if (mt == 131) addr = 22;

			utmp = VIFEnc(csz, addr, 0x8, 0, 0);
			oFile.write((char*)&utmp, sizeof(int));

			for (int i = 0; i < csz; i++)
			{
				MKVert& v = verts[cv[i]];
				unsigned int vx, vy, vz;
				memcpy(&vx, &v.pos[0], sizeof(int));
				memcpy(&vy, &v.pos[1], sizeof(int));
				memcpy(&vz, &v.pos[2], sizeof(int));

				if (skin && (mt == 128 || mt == 131))
				{
					vx &= 0xFFFFFC00;
					vy &= 0xFFFFFC00;
					vz &= 0xFFFFFC00;

					int bi[4] = { 0 }, bn = 0;
					if (mt == 128 && bpc > 0)
						RemapPalette(v, bp, bpc, bi, &bn, haList);
					else
						GetHAnimBoneIndices(v, bi, &bn, haList);

					unsigned int b1 = (bn > 0) ? ((bi[0] + 1) * 4) & 0x3FF : 0;
					unsigned int b2 = (bn > 1) ? ((bi[1] + 1) * 4) & 0x3FF : 0;
					unsigned int b3 = (bn > 2) ? ((bi[2] + 1) * 4) & 0x3FF : 0;
					vx |= b1;
					vy |= b2;
					vz |= b3;
				}
				oFile.write((char*)&vx, sizeof(int));
				oFile.write((char*)&vy, sizeof(int));
				oFile.write((char*)&vz, sizeof(int));
			}

			if (et == Export_SkinnedNoUV && skin)
			{
				utmp = VIFEnc(csz, 1, 0xE, 0, 1);
				oFile.write((char*)&utmp, sizeof(int));

				for (int i = 0; i < csz; i++)
				{
					MKVert& v = verts[cv[i]];
					float mw = 0;
					int mbi = 0;
					int bi[4] = { 0 }, bn = 0;
					GetHAnimBoneIndices(v, bi, &bn, haList);

					for (int w = 0; w < bn; w++)
					{
						if (v.bw[w] > mw)
						{
							mw = v.bw[w];
							mbi = bi[w];
						}
					}
					unsigned char uc = (mbi + 1) * 4;
					oFile.write((char*)&uc, sizeof(char));
					oFile.write((char*)&pad, sizeof(char));
					oFile.write((char*)&pad, sizeof(char));
					uc = cs[i];
					oFile.write((char*)&uc, sizeof(char));
				}
			}

			addr = 0;
			if (mt == 128) addr = 16;
			else if (mt == 137) addr = 18;
			else if (mt == 131) addr = 23;
			else if (mt == 130) addr = 2;

			utmp = VIFEnc(csz, addr, 0xA, 0, 0);
			oFile.write((char*)&utmp, sizeof(int));

			for (int i = 0; i < csz; i++)
			{
				MKVert& v = verts[cv[i]];
				char nx = (char)(v.nrm[0] * 127.0f);
				char ny = (char)(v.nrm[1] * 127.0f);
				char nz = (char)(v.nrm[2] * 127.0f);
				oFile.write(&nx, sizeof(char));
				oFile.write(&ny, sizeof(char));
				oFile.write(&nz, sizeof(char));
			}

			while ((unsigned int)oFile.tellp() % 4)
				oFile.write((char*)&pad, sizeof(char));

			if ((meshFlags & EMF_UV) && (mt == 128 || mt == 131))
			{
				addr = (mt == 128) ? 17 : 24;
				utmp = VIFEnc(csz, addr, 0x5, 0, 0);
				oFile.write((char*)&utmp, sizeof(int));
				for (int i = 0; i < csz; i++)
				{
					MKVert& v = verts[cv[i]];
					short su = (short)(v.uv[0] * 4096.0f);
					short sv = (short)((1.0f - v.uv[1]) * 4096.0f);
					oFile.write((char*)&su, sizeof(short));
					oFile.write((char*)&sv, sizeof(short));
				}
			}

			if ((meshFlags & EMF_UV) && mt == 137)
			{
				utmp = VIFEnc(csz, 19, 0x4, 0, 0);
				oFile.write((char*)&utmp, sizeof(int));
				for (int i = 0; i < csz; i++)
				{
					MKVert& v = verts[cv[i]];
					float fu = v.uv[0];
					float fv = 1.0f - v.uv[1];
					oFile.write((char*)&fu, sizeof(float));
					oFile.write((char*)&fv, sizeof(float));
				}
			}

			if (mt == 137)
			{
				utmp = VIFEnc(csz, 20, 0xE, 0, 1);
				oFile.write((char*)&utmp, sizeof(int));
				for (int i = 0; i < csz; i++)
				{
					if (meshFlags & EMF_VertexColor)
					{
						MKVert& v = verts[cv[i]];
						oFile.write((char*)&v.col[0], sizeof(char));
						oFile.write((char*)&v.col[1], sizeof(char));
						oFile.write((char*)&v.col[2], sizeof(char));
						oFile.write((char*)&v.col[3], sizeof(char));
					}
					else
					{
						oFile.write((char*)&pad, sizeof(char));
						oFile.write((char*)&pad, sizeof(char));
						oFile.write((char*)&pad, sizeof(char));
						unsigned char ff = 255;
						oFile.write((char*)&ff, sizeof(char));
					}
				}
			}

			utmp = 0x20000000;
			oFile.write((char*)&utmp, sizeof(int));
			utmp = 0xCFCFCFCF;
			oFile.write((char*)&utmp, sizeof(int));

			if (mt == 128 || mt == 131 || mt == 137)
			{
				addr = (mt == 131) ? 24 : 17;
				utmp = VIFEnc(csz, addr, 0x1, 1, 0);
				oFile.write((char*)&utmp, sizeof(int));

				for (int i = 0; i < csz; i++)
					oFile.write((char*)&vidPre[i], sizeof(short));

				if ((csz * 2) % 4)
					oFile.write((char*)&pad, sizeof(short));

				utmp = 0x20000000;
				oFile.write((char*)&utmp, sizeof(int));
				utmp = 0x3F3F3F3F;
				oFile.write((char*)&utmp, sizeof(int));

				if (skin && et != Export_Generic)
				{
					addr = 0;
					if (mt == 128) addr = 15;
					else if (mt == 131) addr = 22;

					utmp = VIFEnc(csz, addr, 0x1, 1, 0);
					oFile.write((char*)&utmp, sizeof(int));

					for (int i = 0; i < csz; i++)
					{
						MKVert& v = verts[cv[i]];
						int nw = CountWeights(v);
						short w1 = (nw == 1) ? 4096 : (nw > 1) ? (short)(v.bw[0] * 4096.0f) : 4096;
						oFile.write((char*)&w1, sizeof(short));
					}
					if ((csz * 2) % 4)
						oFile.write((char*)&pad, sizeof(short));

					addr = 0;
					if (mt == 128) addr = 16;
					else if (mt == 131) addr = 23;

					utmp = VIFEnc(csz, addr, 0x1, 1, 0);
					oFile.write((char*)&utmp, sizeof(int));

					for (int i = 0; i < csz; i++)
					{
						MKVert& v = verts[cv[i]];
						short w2 = (CountWeights(v) >= 2) ? (short)(v.bw[1] * 4096.0f) : 0;
						oFile.write((char*)&w2, sizeof(short));
					}

					if ((csz * 2) % 4)
						oFile.write((char*)&pad, sizeof(short));
				}


				addr = 0;
				if (mt == 128) addr = 17;
				else if (mt == 137) addr = 18;
				else if (mt == 131) addr = 24;

				utmp = VIFEnc(csz, addr, 0x1, 1, 0);

				oFile.write((char*)&utmp, sizeof(int));

				for (int i = 0; i < csz; i++)
					oFile.write((char*)&vidPre[i], sizeof(short));

				if ((csz * 2) % 4)
					oFile.write((char*)&pad, sizeof(short));
			}

			utmp = 0x01000101;
			oFile.write((char*)&utmp, sizeof(int));
			utmp = 0x20000000;
			oFile.write((char*)&utmp, sizeof(int));

			utmp = 0; oFile.write((char*)&utmp, sizeof(int));
			avc += csz;

			if (mt == 130)
				utmp = 0x04000000 | csz;
			else if (ch == 0)
				utmp = 0x04000000;
			else
				utmp = 0x04000000 | (avc & 1);

			oFile.write((char*)&utmp, sizeof(int));

			utmp = (ch == 0) ? 0x14000000 : 0x17000000;
			oFile.write((char*)&utmp, sizeof(int));
		}

		utmp = 0x11000000;
		oFile.write((char*)&utmp, sizeof(int));;

		while ((unsigned int)oFile.tellp() % 16)
			oFile.write((char*)&pad, sizeof(char));

		adjustShort(oFile, dO, (unsigned short)(((unsigned int)oFile.tellp() - dP) / 16));

		oFile.write((char*)gDummyData, p2);

		adjustInt(oFile, ssO, (unsigned int)((unsigned int)oFile.tellp() - ssD));
	}

	if (skin && (mt == 128 || mt == 131 || mt == 130))
	{
		writeChunk(oFile, RW_STRUCT, rwVer, 0xFFFFFFF4);
		tmp = 4;
		oFile.write((char*)&tmp, sizeof(int));
		unsigned char uc = haCnt;
		oFile.write((char*)&uc, sizeof(char));

		int nu = 0, mxw = 0;
		std::vector<int> ua;;
		ua.resize(MAX_HANIM);

		for (int vi = 0; vi < MAX_HANIM; vi++)
			ua[vi] = 0;

		for (int vi = 0; vi < (int)verts.size(); vi++)
		{
			int nw = CountWeights(verts[vi]);
			if (nw > mxw)
				mxw = nw;
			for (int w = 0; w < nw; w++)
			{
				int hi = GetHAnimFromFrame(verts[vi].bi[w], haList);
				if (!ua[hi])
				{
					ua[hi] = 1;
					nu++;
				}
			}
		}
		uc = nu;
		oFile.write((char*)&uc, sizeof(char));
		uc = mxw;
		oFile.write((char*)&uc, sizeof(char));
		uc = 0;
		oFile.write((char*)&uc, sizeof(char));

		if (mt == 130 || mt == 131)
		{
			int cnt = 0;
			for (unsigned int h = 0; h < haCnt && cnt < nu; h++)
			{
				if (ua[h])
				{
					uc = h;
					oFile.write((char*)&uc, sizeof(char));
					cnt++;
				}
			}
		}
		else
		{
			for (int i = 0; i < nu; i++)
			{
				uc = i;
				oFile.write((char*)&uc, sizeof(char));
			}
		}

		XMFLOAT4X4 inv0 = MatrixInvAffine(&wm[haList[0].frameID]);
		for (unsigned int h = 0; h < haCnt; h++)
		{
			XMFLOAT4X4 r = mat4_mul(&inv0, &wm[haList[h].frameID]);
			for (int row = 0; row < 3; row++)
				for (int col = 0; col < 4; col++)
					oFile.write((char*)&r.m[row][col], sizeof(float));

			float neg;
			neg = -r.m[3][0];
			oFile.write((char*)&neg, sizeof(float));
			neg = -r.m[3][1];
			oFile.write((char*)&neg, sizeof(float));
			neg = -r.m[3][2];
			oFile.write((char*)&neg, sizeof(float));
			float one = 1.0f;
			oFile.write((char*)&one, sizeof(float));
		}

		tmp = 0;
		oFile.write((char*)&tmp, sizeof(int));
		oFile.write((char*)&tmp, sizeof(int));
		oFile.write((char*)&tmp, sizeof(int));
		oFile.write((char*)&tmp, sizeof(int));
	}

	unsigned int natE = (unsigned int)oFile.tellp();
	utmp = (unsigned int)(0x28 + (natE - natO));
	adjustInt(oFile, gS + 4, utmp);
	adjustInt(oFile, gO + 4, (unsigned int)((unsigned int)oFile.tellp() - gD));
	adjustInt(oFile, glO + 4, (unsigned int)((unsigned int)oFile.tellp() - glD));

	unsigned int aO = (unsigned int)oFile.tellp();
	writeChunk(oFile, RW_ATOMIC, rwVer, 0);
	unsigned int aD = (unsigned int)oFile.tellp();

	writeChunk(oFile, RW_STRUCT, rwVer, 16);
	utmp = 0;
	oFile.write((char*)&utmp, sizeof(int));
	oFile.write((char*)&utmp, sizeof(int));
	utmp = 5;
	oFile.write((char*)&utmp, sizeof(int));
	utmp = 0;
	oFile.write((char*)&utmp, sizeof(int));

	writeChunk(oFile, RW_EXT, rwVer, 48);
	writeChunk(oFile, RW_R2R, rwVer, 8);
	tmp = 278;
	oFile.write((char*)&tmp, sizeof(int));
	tmp = 1;
	oFile.write((char*)&tmp, sizeof(int));

	writeChunk(oFile, RW_MKSOBJ, rwVer, 16);
	utmp = 3;
	oFile.write((char*)&utmp, sizeof(int));
	utmp = 0;
	oFile.write((char*)&utmp, sizeof(int));
	float zf = 0;
	oFile.write((char*)&zf, sizeof(float));
	oFile.write((char*)&mt, sizeof(int));

	adjustInt(oFile, aO + 4, (unsigned int)((unsigned int)oFile.tellp() - aD));

	writeChunk(oFile, RW_EXT, rwVer, 0);
	adjustInt(oFile, clO + 4, (unsigned int)((unsigned int)oFile.tellp() - clD));

	unsigned int rem = (unsigned int)oFile.tellp() % SEC_FILEPADSIZE;
	if (rem)
		oFile.write((char*)gDummyData, SEC_FILEPADSIZE - rem);

	unsigned int finalSize = (unsigned int)oFile.tellp();
	oFile.close();
}

int MKModelTool::GetHAnimFromFrame(int frameID, std::vector<HAnimEntry>& haList)
{
	for (unsigned int i = 0; i < haList.size(); i++)
	{
		if (haList[i].frameID == frameID)
			return i;
	}

	return 0;
}

void MKModelTool::RemapPalette(MKVert& v, int* pal, int pc, int* bi, int* bn, std::vector<HAnimEntry>& haList)
{
	*bn = 0;
	int haCnt = (int)haList.size();
	for (int w = 0; w < 4 && v.bw[w] > 0.f; w++)
	{
		int fi = v.bi[w]; int pi = 0;
		for (int p = 0; p < pc; p++)
		{
			int hi = pal[p];
			if (hi >= 0 && hi < haCnt && haList[hi].frameID == fi) { pi = p; break; }
		}
		bi[(*bn)++] = pi;
	}
}

void MKModelTool::GetHAnimBoneIndices(MKVert& v, int* bi, int* bn, std::vector<HAnimEntry>& haList)
{
	*bn = 0;
	for (int w = 0; w < 4 && v.bw[w] > 0.f; w++)
	{
		bi[(*bn)++] = GetHAnimFromFrame(v.bi[w], haList);
	}

}

int MKModelTool::BuildPalette(MKMeshGroup& sub, int* pal, std::vector<HAnimEntry>& haList)
{
	int pc = 0;
	for (unsigned int fi = sub.firstFace; fi < sub.firstFace + sub.numFaces; fi++)
	{
		for (int vi = 0; vi < 3; vi++)
		{
			MKVert& v = verts[faces[fi].v[vi]];
			for (int w = 0; w < 4 && v.bw[w] > 0.f; w++)
			{
				int hi = GetHAnimFromFrame(v.bi[w], haList);
				bool found = false;
				for (int p = 0; p < pc; p++)
				{
					if (pal[p] == hi)
					{
						found = true;
						break;
					}
				}
				if (!found && pc < MAX_PALETTE)
					pal[pc++] = hi;
			}
		}
	}

	return pc;
}


unsigned char MKModelTool::FindBone(unsigned int boneFlags, int* bonePalette)
{
	if (boneFlags == 0)
		return 0;

	int palID = boneFlags / 4 - 1;

	if (palID < 0)
		return 0;

	int hAnimID = bonePalette[palID];
	return (hAnimID >= 0 && hAnimID < frameList.hanimCount && frameList.hanimToFrame[hAnimID] >= 0) ? (unsigned char)frameList.hanimToFrame[hAnimID] : 0;
}

unsigned int MKModelTool::UnpackRWVersion(unsigned int packedVersion)
{
	unsigned int rwVer;
	unsigned int tmp = packedVersion;
	if (tmp & 0xFFFF0000)
		rwVer = (tmp >> 14 & 0x3FF00) + 0x30000 | (tmp >> 16 & 0x3F);
	else
		rwVer = tmp << 8;
	return rwVer;
}