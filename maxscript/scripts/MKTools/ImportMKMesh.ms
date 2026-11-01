
	-- from ActorXImporter.ms by Gildor
fn readFixedString bstream fixedLen =
	(
		local str = ""
		local length = 0
		local finished = false
		for i = 1 to fixedLen do
		(
			local c = ReadByte bstream #unsigned
			if c == 0 then finished = true	-- end of line char
			if not finished then 			-- has end of line before - skip remaining chars
			(
				-- not "finished" string
				str += bit.intAsChar(c)		-- append a character
				if c != 32 then length = i	-- position of last non-space char
			)
		)
		substring str 1 length				-- return first "length" chars
	)



fn MKMeshImp filePath = (
		local f = fopen filePath "rb"
		if f == undefined do (
			messageBox ("Cannot open file: " + filePath)
			return false
		)


-- header
		local magic      = readLong f #unsigned
		local version    = readLong f #unsigned
		local numVerts   = readLong f #unsigned
		local numFaces   = readLong f #unsigned
		local numBones   = readLong f #unsigned
		local numMats    = readLong f #unsigned
		local flags      = readLong f #unsigned
		local numSubs    = readLong f #unsigned
		
		local numAtomics = 1
		local numGeometries = 1
		if version >= 5 do (
			numAtomics = readLong f #unsigned
			numGeometries = readLong f #unsigned
		)

		if magic != 0x4D455348 do (
			fclose f
			messageBox "Not a valid .mesh file (bad magic)"
			return false
		)

		local hasNormals = (bit.and flags 0x01) != 0
		local hasUVs     = (bit.and flags 0x02) != 0
		local hasColors  = (bit.and flags 0x04) != 0
		local hasSkin    = (bit.and flags 0x08) != 0


-- bones
		local boneNames    = #()
		local boneParents  = #()
		local boneIDs      = #()
		local boneTypes    = #()
		local boneSkinIDs  = #()
		local boneGroupIDs = #()
		local boneMats     = #()

		for i = 1 to numBones do (
			local mat = #()
			for j = 1 to 12 do append mat (readFloat f)
			local parentIdx  = readLong f 
			local boneID     = readLong f
			local boneType   = readLong f 
			local skinBoneID = readLong f 
			local groupID    = 0
			if version >= 4 do groupID = readLong f 
			local boneName   = readFixedString f 64

			if boneName == "" do boneName = ("Bone" + (i as string))

			append boneNames boneName
			append boneParents (parentIdx + 1) 
			append boneIDs boneID
			append boneTypes boneType
			append boneSkinIDs skinBoneID
			append boneGroupIDs groupID


			local m = matrix3 \
				[mat[1], mat[2], mat[3]] \
				[mat[4], mat[5], mat[6]] \
				[mat[7], mat[8], mat[9]] \
				[mat[10], mat[11], mat[12]]
			append boneMats m
		)

--- mats
		local matArray = #()
		local meshDir = getFilenamePath filePath

		for i = 1 to numMats do (
			local colR = readByte f #unsigned
			local colG = readByte f #unsigned
			local colB = readByte f #unsigned
			local colA = readByte f #unsigned
			local rwAmbient  = readFloat f
			local rwSpecular = readFloat f
			local rwDiffuse  = readFloat f
			local hasTex = readLong f
			local texName = readFixedString f 64
			local alphaTexName = readFixedString f 64
			local texFilter = readByte f #unsigned
			local uAddr     = readByte f #unsigned
			local vAddr     = readByte f #unsigned
			local _pad0     = readByte f #unsigned
			local useMipLvl    = readShort f #unsigned
			local hasSkyMipmap = readByte f #unsigned
			local _pad1        = readByte f #unsigned
			local skyK = readLong f
			local skyL = readLong f

			local mkFlag       = readLong f
			local mkType       = readLong f
			local mkFlagsUpper = readLong f
			local mkUnk1       = readShort f
			local mkUnk2       = readShort f
			local mkUnk3       = readFloat f
			local mkUseBP      = readLong f
			local mkBPCount    = readLong f
			local mkBonePal    = #()
			for j = 1 to 64 do append mkBonePal (readLong f)
			local mkSpecR = readByte f #unsigned
			local mkSpecG = readByte f #unsigned
			local mkSpecB = readByte f #unsigned
			local mkSpecA = readByte f #unsigned
			local mkUnk5  = readFloat f
			local mkUnk6  = readFloat f
			local mkUVScroll = readLong f
			local mkScrollX  = readFloat f
			local mkScrollY  = readFloat f
			local mkScrollZ  = readFloat f
			local mkScrollPad = readFloat f
			local mkMeshType = readLong f

			local matName = if texName != "" then texName else ("mtl_" + (i as string))
			local mkMat = MK_Mtl()
			mkMat.name = matName

			mkMat.matColor = color colR colG colB
			mkMat.matAlpha = colA / 255.0
			mkMat.rw_ambient = rwAmbient
			mkMat.rw_diffuse = rwDiffuse
			mkMat.rw_specular = rwSpecular
			mkMat.texture_name = texName
			mkMat.texture_filter = texFilter
			mkMat.uv_addressing = (bit.or (bit.shift uAddr 4) vAddr)
			mkMat.has_skymipmap = (hasSkyMipmap != 0)
			mkMat.skymipmap_K = skyK
			mkMat.skymipmap_L = skyL
			
			if hasTex != 0 and texName != "" do (
				local texPath = undefined
				for ext in #(".bmp", ".tga", ".png", ".dds") do (
					local tryPath = meshDir + texName + ext
					if doesFileExist tryPath do ( texPath = tryPath; exit )
				)
				if texPath != undefined do (
					local bm = bitmaptexture filename:texPath
					mkMat.texmap_diffuse = bm
				)
			)

			mkMat.mk_type = mkType
			mkMat.mk_meshType = mkMeshType
			mkMat.mk_unk1 = mkUnk1
			mkMat.mk_unk2 = mkUnk2
			mkMat.mk_unk3 = mkUnk3
			mkMat.mk_spec_r = mkSpecR
			mkMat.mk_spec_g = mkSpecG
			mkMat.mk_spec_b = mkSpecB
			mkMat.mk_spec_a = mkSpecA
			mkMat.mk_unk5 = mkUnk5
			mkMat.mk_unk6 = mkUnk6
			mkMat.mk_uv_scroll = (mkUVScroll != 0)
			mkMat.mk_uv_scroll_x = mkScrollX
			mkMat.mk_uv_scroll_y = mkScrollY
			mkMat.mk_uv_scroll_z = mkScrollZ
			mkMat.mk_useBonePal = (mkUseBP != 0)

			--hack: store original bonepalette per material
			--practically unviable to do models from scratch anyway
			if mkUseBP != 0 and mkBPCount > 0 do (
				local bpStr = ""
				local cnt = if mkBPCount > 64 then 64 else mkBPCount
				for j = 1 to cnt do (
					if j > 1 do bpStr += ","
					bpStr += (mkBonePal[j] as string)
				)
				mkMat.mk_bonePalette = bpStr
			)

			append matArray mkMat
		)

--verts 
		local positions = #()
		local normals   = #()
		local uvCoords  = #()
		local vcolors   = #()
		local skinBI    = #() 
		local skinBW    = #()

		for i = 1 to numVerts do (
			local px = readFloat f
			local py = readFloat f
			local pz = readFloat f
			append positions [px, py, pz]

			if hasNormals do (
				local nx = readFloat f
				local ny = readFloat f
				local nz = readFloat f
				append normals [nx, ny, nz]
			)
			if hasUVs do (
				local u = readFloat f
				local v = readFloat f
				append uvCoords [u, v, 0]
			)
			if hasColors do (
				local cr = readByte f #unsigned
				local cg = readByte f #unsigned
				local cb = readByte f #unsigned
				local ca = readByte f #unsigned
				append vcolors (color cr cg cb ca)
			)
			if hasSkin do (
				local bi = #()
				for j = 1 to 4 do append bi (readByte f #unsigned)
				append skinBI bi
				local bw = #()
				for j = 1 to 4 do append bw (readFloat f)
				append skinBW bw
			)
		)

-- groups
		local subMatIdx   = #()
		local subFirstFace = #()
		local subNumFaces  = #()

		for i = 1 to numSubs do (
			append subMatIdx   ((readLong f #unsigned) + 1) 
			append subFirstFace ((readLong f #unsigned) + 1)
			append subNumFaces  (readLong f #unsigned)
		)

-- faces
		local faceVerts = #()

		for i = 1 to numFaces do (
			local v1 = (readLong f #unsigned) + 1  
			local v2 = (readLong f #unsigned) + 1
			local v3 = (readLong f #unsigned) + 1
			append faceVerts [v1, v2, v3]
		)

		-- hack: just keep everything in model
		local hanimListStr = ""
		local hanimCount = readLong f #unsigned
			for i = 1 to hanimCount do (
				local packed = readLong f #unsigned
				local hSkinBoneID = readLong f
				local hBoneType = readLong f
				if i > 1 do hanimListStr += ";"
				hanimListStr += (packed as string) + "," + (hSkinBoneID as string) + "," + (hBoneType as string)
			)

		fclose f
		local meshName = getFilenameFile filePath
		local theMesh = mesh vertices:positions faces:faceVerts name:meshName

		if hanimListStr != "" do (
			setUserProp theMesh "HAnimList" hanimListStr
		)

		if numMats > 0 do (
			if numMats == 1 then (
				theMesh.material = matArray[1]
			) else (
				local multiMat = multiMaterial numsubs:numMats
				for i = 1 to numMats do (
					multiMat[i] = matArray[i]
					multiMat.materialIDList[i] = i
				)
				theMesh.material = multiMat
			)

			for s = 1 to numSubs do (
				local matID = subMatIdx[s]
				local ff = subFirstFace[s]
				local nf = subNumFaces[s]
				for fi = ff to (ff + nf - 1) do (
					if fi >= 1 and fi <= numFaces do (
						setFaceMatID theMesh fi matID
					)
				)
			)
		)


		if hasUVs and uvCoords.count == numVerts do (
			meshOp.setNumMaps theMesh 2 
			meshOp.setMapSupport theMesh 1 true
			meshOp.setNumMapVerts theMesh 1 numVerts
			meshOp.setNumMapFaces theMesh 1 numFaces

			for i = 1 to numVerts do (
				meshOp.setMapVert theMesh 1 i uvCoords[i]
			)
			for i = 1 to numFaces do (
				meshOp.setMapFace theMesh 1 i faceVerts[i]
			)
		)


		if hasNormals and normals.count == numVerts do (
			for fi = 1 to numFaces do setFaceSmoothGroup theMesh fi 1
			for vi = 1 to numVerts do setNormal theMesh vi normals[vi]
		)

		if hasColors and vcolors.count == numVerts do (
			meshOp.setNumMaps theMesh (meshOp.getNumMaps theMesh + 1)
			local vcChan = 0 
			meshOp.setMapSupport theMesh vcChan true
			meshOp.setNumMapVerts theMesh vcChan numVerts
			meshOp.setNumMapFaces theMesh vcChan numFaces

			for i = 1 to numVerts do (
				local c = vcolors[i]
				meshOp.setMapVert theMesh vcChan i [c.r / 255.0, c.g / 255.0, c.b / 255.0]
			)
			for i = 1 to numFaces do (
				meshOp.setMapFace theMesh 1 i faceVerts[i]
			)
			theMesh.showVertexColors = true
		)

		local boneObjs = #()
		if numBones > 0 do (
			for i = 1 to numBones do (
				local tfm = boneMats[i]
				local bn = dummy boxsize:[0,0,0] wirecolor:yellow
				bn.name = boneNames[i]
				bn.transform = tfm
				bn.showLinks = true
				bn.showLinksOnly = true
				setUserProp bn "BoneIndex" (i - 1)
				setUserProp bn "BoneID" boneIDs[i]
				setUserProp bn "BoneType" ("0x" + bit.intAsHex boneTypes[i])
				setUserProp bn "SkinBoneID" boneSkinIDs[i]
				setUserProp bn "GroupID" boneGroupIDs[i]
				append boneObjs bn
			)

			for i = 1 to numBones do (
				local pi = boneParents[i]
				if pi >= 1 and pi <= numBones do (
					boneObjs[i].parent = boneObjs[pi]
				)
			)

			if hasSkin and skinBI.count == numVerts do (
				local boneIDMap = #()
				local maxBoneIdx = 0
				for vi = 1 to numVerts do (
					for j = 1 to 4 do (
						local idx = skinBI[vi][j]
						if idx > maxBoneIdx do maxBoneIdx = idx
					)
				)

				local useDirectIndex = (maxBoneIdx < numBones)

				if not useDirectIndex do (
					format "using boneID mapping\n" maxBoneIdx numBones
				)

				max modify mode
				select theMesh

				local skinMod = skin()
				addModifier theMesh skinMod
				modPanel.setCurrentObject skinMod

				for i = 1 to numBones do (
					skinOps.addBone skinMod boneObjs[i] 0
				)
				redrawViews()

				for vi = 1 to numVerts do (
					local bi = skinBI[vi]
					local bw = skinBW[vi]

					local boneIdxArr = #()
					local boneWgtArr = #()
					for j = 1 to 4 do (
						if bw[j] > 0.001 do (
							local rawIdx = bi[j]
							local skinIdx = 0

							if useDirectIndex then (
								skinIdx = rawIdx + 1
							) else (
								local found = false
								for b = 1 to numBones while not found do (
									if boneIDs[b] == rawIdx do (
										skinIdx = b
										found = true
									)
								)
								if skinIdx == 0 and rawIdx < numBones do skinIdx = rawIdx + 1
								if skinIdx == 0 do skinIdx = 1
							)

							if skinIdx >= 1 and skinIdx <= numBones do (
								append boneIdxArr skinIdx
								append boneWgtArr bw[j]
							)
						)
					)

					if boneIdxArr.count > 0 do (
						try ( skinOps.ReplaceVertexWeights skinMod vi boneIdxArr boneWgtArr ) catch ()
					)
				)
			)
		)

		update theMesh
		select theMesh
		max zoomext sel all

		format "Import complete: %\n" meshName
		true
	)