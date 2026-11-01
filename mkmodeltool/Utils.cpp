#include "Utils.h"
#include <algorithm>
#include <DirectXMath.h>
#include <Windows.h>

XMFLOAT4X4 mat4_mul(XMFLOAT4X4* a, XMFLOAT4X4* b)
{
	XMFLOAT4X4 r;
	ZeroMemory(&r, sizeof(r));
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				r.m[i][j] += a->m[i][k] * b->m[k][j];
	return r;
}


XMFLOAT4X4 mat4_identity()
{
	XMFLOAT4X4 r;
	ZeroMemory(&r, sizeof(r));
	r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0f;
	return r;
}

Vec3 mat3_transform(XMFLOAT4X4* m, Vec3 v)
{
	Vec3 r;
	r.x = m->m[0][0] * v.x + m->m[1][0] * v.y + m->m[2][0] * v.z + m->m[0][3];
	r.y = m->m[0][1] * v.x + m->m[1][1] * v.y + m->m[2][1] * v.z + m->m[1][3];
	r.z = m->m[0][2] * v.x + m->m[1][2] * v.y + m->m[2][2] * v.z + m->m[2][3];
	return r;
}

XMFLOAT4X4 mat3_inv_transpose(XMFLOAT4X4* m)
{
	float a[3][3] =
	{
		{m->m[0][0], m->m[0][1], m->m[0][2]},
		{m->m[1][0], m->m[1][1], m->m[1][2]},
		{m->m[2][0], m->m[2][1], m->m[2][2]}
	};
	float det = a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1])
		- a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
		+ a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
	if (fabsf(det) < 1e-10f) det = 1.0f;
	float id = 1.0f / det;
	XMFLOAT4X4 r;
	ZeroMemory(&r, sizeof(r));
	r.m[0][0] = (a[1][1] * a[2][2] - a[1][2] * a[2][1]) * id;
	r.m[0][1] = (a[1][2] * a[2][0] - a[1][0] * a[2][2]) * id;
	r.m[0][2] = (a[1][0] * a[2][1] - a[1][1] * a[2][0]) * id;
	r.m[1][0] = (a[0][2] * a[2][1] - a[0][1] * a[2][2]) * id;
	r.m[1][1] = (a[0][0] * a[2][2] - a[0][2] * a[2][0]) * id;
	r.m[1][2] = (a[0][1] * a[2][0] - a[0][0] * a[2][1]) * id;
	r.m[2][0] = (a[0][1] * a[1][2] - a[0][2] * a[1][1]) * id;
	r.m[2][1] = (a[0][2] * a[1][0] - a[0][0] * a[1][2]) * id;
	r.m[2][2] = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) * id;
	r.m[3][3] = 1.0f;
	return r;
}

MKSkinData parse_skin_nn(std::vector<char>& d, int nv, unsigned int ver)
{
	MKSkinData sk;
	int pos = 0;
	sk.boneCount = (unsigned char)d[pos++];
	sk.usedBoneCount = (unsigned char)d[pos++];
	sk.maxWeights = (unsigned char)d[pos++];
	pos++; // pad
	pos += sk.usedBoneCount;
	sk.numVert = nv;
	sk.boneID.resize(nv * 4);
	memcpy(sk.boneID.data(), d.data() + pos, nv * 4); pos += nv * 4;
	sk.boneWgt.resize(nv * 16);
	memcpy(sk.boneWgt.data(), d.data() + pos, nv * 16); pos += nv * 16;

	sk.usedBoneList.resize(sk.boneCount);
	if (ver < 0x34000 && sk.maxWeights == 0 && sk.boneCount > 0)
	{
		int trailerStart = pos;
		int nzLow = 0, nzHigh = 0;
		for (int i = 0; i < sk.boneCount; i++)
		{
			unsigned char lo = (unsigned char)d[trailerStart + i * 68 + 0];
			unsigned char hi = (unsigned char)d[trailerStart + i * 68 + 3];
			if (lo) nzLow++;
			if (hi) nzHigh++;
		}
		bool useHigh = (nzHigh > nzLow);

		for (int i = 0; i < sk.boneCount; i++)
		{
			int f;
			memcpy(&f, d.data() + pos, 4);
			sk.usedBoneList[i] = useHigh ? ((f >> 24) & 0xFF) : (f & 0xFF);
			pos += 4;
			pos += sizeof(XMFLOAT4X4);
		}
	}
	else
	{
		for (int i = 0; i < sk.boneCount; i++)
		{
			sk.usedBoneList[i] = i;
			pos += sizeof(XMFLOAT4X4);
		}
	}
	return sk;
}
MKSkinData parse_skin_native(std::vector<char>& d)
{
	MKSkinData sk;
	int pos = sizeof(RWChunk); // ignore header
	unsigned int platform;
	memcpy(&platform, d.data() + pos, 4);
	pos += 4;
	sk.boneCount = (unsigned char)d[pos++];
	sk.usedBoneCount = (unsigned char)d[pos++];
	sk.maxWeights = (unsigned char)d[pos++];
	pos++; // pad
	sk.usedBoneList.resize(256, 0);
	if (platform == PLATFORM_PS2)
	{
		for (int i = 0; i < sk.usedBoneCount; i++)
			sk.usedBoneList[i] = (unsigned char)d[pos++];
	}
	return sk;
}

MKBinMeshPLG parse_binmesh(std::vector<char>& d, int native)
{
	MKBinMeshPLG bm;
	int pos = 0;
	memcpy(&bm.faceType, d.data() + pos, 4); pos += 4;
	memcpy(&bm.numSplits, d.data() + pos, 4); pos += 4;
	memcpy(&bm.idxCount, d.data() + pos, 4); pos += 4;

	bm.matIDs.resize(bm.numSplits);
	bm.matNumFaces.resize(bm.numSplits);
	bm.splitIdx.resize(bm.numSplits);

	for (int i = 0; i < bm.numSplits; i++)
	{
		unsigned int nfi; memcpy(&nfi, d.data() + pos, 4); pos += 4;
		unsigned int mid; memcpy(&mid, d.data() + pos, 4); pos += 4;
		bm.matIDs[i] = mid;
		bm.matNumFaces[i] = nfi;
		if (native != 1)
		{
			bm.splitIdx[i].resize(nfi);
			for (unsigned int j = 0; j < nfi; j++)
			{
				memcpy(&bm.splitIdx[i][j], d.data() + pos, 4); pos += 4;
			}
			if (bm.faceType == 1)
			{
				for (unsigned int k = 2; k < nfi; k++)
				{
					unsigned int f1 = bm.splitIdx[i][k - 2];
					unsigned int f2 = bm.splitIdx[i][k - 1];
					unsigned int f3 = bm.splitIdx[i][k];
					if (f1 != f2 && f2 != f3 && f1 != f3)
					{
						MKFace fc;
						if (k % 2 == 0)
						{
							fc.v[0] = f1;
							fc.v[1] = f2;
							fc.v[2] = f3;
						}
						else
						{
							fc.v[0] = f2;
							fc.v[1] = f1;
							fc.v[2] = f3;
						}

						bm.faces.push_back(fc);
						bm.faceMatID.push_back(mid);
					}
				}
			}
		}
	}
	return bm;
}

std::vector<float> decode_normals(char* data, int nv)
{
	std::vector<float> out(nv * 3);
	for (int i = 0; i < nv; i++)
	{
		out[i * 3 + 0] = (char)data[i * 3 + 0] / 128.0f;
		out[i * 3 + 1] = (char)data[i * 3 + 1] / 128.0f;
		out[i * 3 + 2] = (char)data[i * 3 + 2] / 128.0f;
	}
	return out;
}

std::vector<float> decode_uv16(char* data, int nv)
{
	std::vector<float> out(nv * 2);
	for (int i = 0; i < nv; i++)
	{
		short u, v;
		memcpy(&u, data + i * 4, 2);
		memcpy(&v, data + i * 4 + 2, 2);
		out[i * 2 + 0] = u / 4096.0f;
		out[i * 2 + 1] = v / 4096.0f;
	}
	return out;
}

VertexIDList decode_vidlist_skin(char* data, int dataSize, int useBP)
{
	VertexIDList vl;
	int nv = dataSize / 2;
	vl.ids.resize(nv);
	vl.skipFlags.resize(nv);
	for (int i = 0; i < nv; i++)
	{
		unsigned short val; memcpy(&val, data + i * 2, 2);
		int skip = (val & 0x8000) ? 1 : 0;
		int v15 = val & 0x7FFF, vid;
		if (useBP) vid = (v15 < 488) ? ((v15 - 130) / 4) : ((v15 - 488) / 4);
		else       vid = (v15 < 634) ? ((v15 - 191) / 3) : ((v15 - 634) / 3);
		vl.ids[i] = vid;
		vl.skipFlags[i] = skip;
	}
	return vl;
}

VertexIDListShared decode_shared_vidlist_skin(char* data, int dataSize, int useBP)
{
	int nv = dataSize / 2, np = nv / 2;
	VertexIDListShared sv;
	sv.prevIDs.resize(np); sv.curIDs.resize(np);
	sv.skipPrev.resize(np); sv.skipCur.resize(np);
	for (int i = 0; i < nv; i++)
	{
		unsigned short val;
		memcpy(&val, data + i * 2, 2);

		int skip = (val & 0x8000) ? 1 : 0;

		int v15 = val & 0x7FFF, vid;

		if (useBP)
			vid = (v15 < 488) ? ((v15 - 130) / 4) : ((v15 - 488) / 4);
		else
			vid = (v15 < 634) ? ((v15 - 191) / 3) : ((v15 - 634) / 3);

		if (i % 2 == 0)
		{
			sv.prevIDs[i / 2] = vid;
			sv.skipPrev[i / 2] = skip;
		}
		else
		{
			sv.curIDs[i / 2] = vid;
			sv.skipCur[i / 2] = skip;
		}
	}
	return sv;
}

VertexIDList decode_vidlist_map(char* data, int dataSize, int* out_interval)
{
	int nv = dataSize / 2;
	VertexIDList vl;
	vl.ids.resize(nv);
	vl.skipFlags.resize(nv);
	std::vector<int> vals(nv), sorted(nv);
	for (int i = 0; i < nv; i++)
	{
		unsigned short v; memcpy(&v, data + i * 2, 2);
		vl.skipFlags[i] = (v & 0x8000) ? 1 : 0;
		vals[i] = v & 0x7FFF;
		sorted[i] = v & 0x7FFF;
	}
	std::sort(sorted.begin(), sorted.end());
	int interval = 999;
	for (int i = 1; i < nv; i++)
	{
		int d = sorted[i] - sorted[i - 1];
		if (d > 0 && d < interval) interval = d;
	}
	*out_interval = interval;

	for (int i = 0; i < nv; i++)
	{
		int value = vals[i], vid = 0;
		if (interval % 3 == 0)
		{
			if (value < 622 && value >= 1)
			{
				if (value % 3 == 1) vid = (value >= 238) ? ((value - 238) / 3) : ((value - 1) / 3);
				else if (value % 3 == 0) vid = (value - 147) / 3;
			}
			else if (value >= 622)
			{
				vid = ((value - 622) % 3 > 0) ? ((value - 723) / 3) : ((value - 622) / 3);
			}
		}
		else if (interval % 5 == 0)
		{
			if (value < 671 && value >= 193)
			{
				vid = ((value - 193) % 5 > 0) ? ((value - 201) / 5) : ((value - 193) / 5);
			}
			else if (value >= 671)
			{
				vid = ((value - 671) % 5 > 0) ? ((value - 679) / 5) : ((value - 671) / 5);
			}
		}
		vl.ids[i] = vid;
	}
	return vl;
}

VertexIDListShared decode_shared_vidlist_map(char* data, int dataSize, int interval)
{
	int nv = dataSize / 2, np = nv / 2;
	VertexIDListShared sv;
	sv.prevIDs.resize(np); sv.curIDs.resize(np);
	sv.skipPrev.resize(np); sv.skipCur.resize(np);
	for (int i = 0; i < nv; i++)
	{
		unsigned short val; memcpy(&val, data + i * 2, 2);
		int v1 = val & 0x7FFF, skip = (val & 0x8000) ? 1 : 0, vid = 0;
		if (interval % 3 == 0)
		{
			if (v1 >= 147 && v1 < 622) vid = ((v1 - 147) % 3 > 0) ? ((v1 - 238) / 3) : ((v1 - 147) / 3);
			else if (v1 >= 622) vid = ((v1 - 622) % 3 > 0) ? ((v1 - 723) / 3) : ((v1 - 622) / 3);
		}
		else if (interval % 5 == 0)
		{
			if (v1 >= 193 && v1 < 671) vid = ((v1 - 193) % 5 > 0) ? ((v1 - 201) / 5) : ((v1 - 193) / 5);
			else if (v1 >= 671) vid = ((v1 - 671) % 5 > 0) ? ((v1 - 679) / 5) : ((v1 - 671) / 5);
		}
		if (i % 2 == 0)
		{
			sv.prevIDs[i / 2] = vid;
			sv.skipPrev[i / 2] = skip;
		}
		else
		{
			sv.curIDs[i / 2] = vid;
			sv.skipCur[i / 2] = skip;
		}
	}
	return sv;
}

void create_trilist(std::vector<int>& skipList, int nv, std::vector<MKFace>& outFaces, unsigned int baseIdx)
{
	int faceDir = -1;
	int f1 = 0;
	int f2 = 1;
	for (int i = 0; i < nv; i++)
	{
		int f3 = i;
		if (i > 1)
		{
			faceDir *= -1;
			if (!skipList[i])
			{
				MKFace fc;
				if (faceDir > 0)
				{
					fc.v[0] = f1 + baseIdx;
					fc.v[1] = f2 + baseIdx;
					fc.v[2] = f3 + baseIdx;
				}
				else
				{
					fc.v[0] = f2 + baseIdx;
					fc.v[1] = f1 + baseIdx;
					fc.v[2] = f3 + baseIdx;
				}
				outFaces.push_back(fc);
			}
			f1 = f2; f2 = f3;
		}
	}
}

void create_tristrip_simple(std::vector<float>& verts, int nv, std::vector<MKFace>& outFaces, unsigned int baseIdx)
{
	int dir = 1;
	for (int j = 0; j < nv - 2; j++)
	{
		float* v1 = &verts[j * 3], * v2 = &verts[(j + 1) * 3], * v3 = &verts[(j + 2) * 3];
		bool same12 = (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2]);
		bool same23 = (v2[0] == v3[0] && v2[1] == v3[1] && v2[2] == v3[2]);
		bool same13 = (v1[0] == v3[0] && v1[1] == v3[1] && v1[2] == v3[2]);
		if (!same12 && !same23 && !same13)
		{
			MKFace fc;
			if (dir > 0)
			{
				fc.v[0] = j + baseIdx;
				fc.v[1] = j + 1 + baseIdx;
				fc.v[2] = j + 2 + baseIdx;
			}
			else
			{
				fc.v[0] = j + 1 + baseIdx;
				fc.v[1] = j + baseIdx;
				fc.v[2] = j + 2 + baseIdx;
			}
			outFaces.push_back(fc);
		}
		dir *= -1;
	}
}



MKFaceSetNoUV create_trilist_nouv(char* data, int dataSize, int* bonePalette, MKFrameList* fl)
{
	MKFaceSetNoUV res;
	int nv = dataSize / 4;
	res.nv = nv;
	res.boneIDs.resize(nv);
	res.weights.resize(nv);
	int pos = 0;
	int faceDir = 1, f1 = 0, f2 = 1;
	for (int i = 0; i < nv; i++)
	{
		unsigned char bid1 = (unsigned char)data[pos] / 4; pos++;
		pos += 2;
		unsigned char bitFlag = (unsigned char)data[pos++];
		int hanimIdx = bonePalette[bid1 - 1];
		int frameIdx = (hanimIdx >= 0 && hanimIdx < fl->hanimCount && fl->hanimToFrame[hanimIdx] >= 0)
			? fl->hanimToFrame[hanimIdx] : 0;
		res.boneIDs[i] = frameIdx;
		res.weights[i] = 1.0f;
		int f3 = i, skip = bitFlag & 1;
		if (!skip && f1 != f2 && f2 != f3 && f3 != f1)
		{
			MKFace fc;
			if (faceDir > 0) { fc.v[0] = f1; fc.v[1] = f2; fc.v[2] = f3; }
			else { fc.v[0] = f2; fc.v[1] = f1; fc.v[2] = f3; }
			res.faces.push_back(fc);
		}
		faceDir *= -1; f1 = f2; f2 = f3;
	}
	return res;
}


void transform_verts_ps2(std::vector<float>& verts, int nv, XMFLOAT4X4* mat, int skinFlag)
{
	for (int i = 0; i < nv; i++)
	{
		Vec3 v;
		if (skinFlag)
		{
			unsigned int ix, iy, iz;
			memcpy(&ix, &verts[i * 3 + 0], 4); memcpy(&iy, &verts[i * 3 + 1], 4); memcpy(&iz, &verts[i * 3 + 2], 4);
			ix &= 0xFFFFFC00; iy &= 0xFFFFFC00; iz &= 0xFFFFFC00;
			memcpy(&v.x, &ix, 4); memcpy(&v.y, &iy, 4); memcpy(&v.z, &iz, 4);
		}
		else
		{
			v.x = verts[i * 3]; v.y = verts[i * 3 + 1]; v.z = verts[i * 3 + 2];
		}
		v = mat3_transform(mat, v);
		verts[i * 3] = v.x; verts[i * 3 + 1] = v.y; verts[i * 3 + 2] = v.z;
	}
}

void transform_normals_ps2(std::vector<float>& norms, int nv, XMFLOAT4X4* mat)
{
	XMFLOAT4X4 itn = mat3_inv_transpose(mat);
	for (int i = 0; i < nv; i++)
	{
		Vec3 n = { norms[i * 3], norms[i * 3 + 1], norms[i * 3 + 2] };
		n = mat3_transform(&itn, n);
		n.Normalise();
		norms[i * 3] = n.x;
		norms[i * 3 + 1] = n.y;
		norms[i * 3 + 2] = n.z;
	}
}

void fix_face_normals(std::vector<MKFace>& faces, int startFace, std::vector<float>& verts, std::vector<float>& norms, int vbase)
{
	for (int f = startFace; f < (int)faces.size(); f++)
	{
		MKFace& fc = faces[f];
		unsigned int i0 = fc.v[0] - vbase, i1 = fc.v[1] - vbase, i2 = fc.v[2] - vbase;
		Vec3 p0 = { verts[i0 * 3], verts[i0 * 3 + 1], verts[i0 * 3 + 2] };
		Vec3 p1 = { verts[i1 * 3], verts[i1 * 3 + 1], verts[i1 * 3 + 2] };
		Vec3 p2 = { verts[i2 * 3], verts[i2 * 3 + 1], verts[i2 * 3 + 2] };
		Vec3 e1 = p1 - p0;
		Vec3 e2 = p2 - p0;
		Vec3 fn = CrossProduct(e1, e2);
		Vec3 n0 = { norms[i0 * 3], norms[i0 * 3 + 1], norms[i0 * 3 + 2] };
		Vec3 n1 = { norms[i1 * 3], norms[i1 * 3 + 1], norms[i1 * 3 + 2] };
		Vec3 n2 = { norms[i2 * 3], norms[i2 * 3 + 1], norms[i2 * 3 + 2] };

		if (DotProduct(fn, n0) < 0 && DotProduct(fn, n1) < 0 && DotProduct(fn, n2) < 0)
		{
			unsigned int t = fc.v[1];
			fc.v[1] = fc.v[2];
			fc.v[2] = t;
		}
	}
}
