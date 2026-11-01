fn writeFixedString f str len = (
    local written = 0
    local maxLen = if str.count < len then str.count else len
    for i = 1 to maxLen do (
        writeByte f (bit.charAsInt str[i])
        written += 1
    )
    for i = written + 1 to len do writeByte f 0
)


fn MKMeshExp obj filePath CPV = (
	
		if not isKindOf obj GeometryClass do (
			messageBox "Selected object is not a mesh"
			return false
		)

		local snapMesh = snapshotAsMesh obj

		local origNumVerts = snapMesh.numVerts
		local origNumFaces = snapMesh.numFaces

		format "Exporting: % Export cols: %\n" obj.name CPV
		format "  % vertexes % faces\n" origNumVerts origNumFaces

		local hasSkin = false
		local skinMod = undefined
		local skinBoneObjs = #()
		local numBones = 0

		for m in obj.modifiers do (
			if classOf m == Skin do ( skinMod = m; exit )
		)

		if skinMod != undefined do (
			max modify mode
			select obj
			modPanel.setCurrentObject skinMod
			numBones = skinOps.getNumberBones skinMod
			if numBones > 0 do (
				hasSkin = true
				for i = 1 to numBones do (
					local boneName = skinOps.getBoneName skinMod i 0
					local boneNode = getNodeByName boneName
					append skinBoneObjs boneNode
				)
			)
		)
		format "  % bones skin=%\n" numBones hasSkin

		local matList = #()
		local matLookup = #()

		local objMat = obj.material
		if objMat != undefined do (
			if classOf objMat == MultiMaterial then (
				for i = 1 to objMat.numsubs do append matList objMat[i]
			) else (
				append matList objMat
			)
		)
		local numMats = matList.count
		format "  % materials\n" numMats

		local hasNormals = true
		local hasUVs = (meshOp.getMapSupport snapMesh 1)
		local hasColors = CPV
		-- hack so that white mesh doesnt trigger vertex color export
		-- added button no longer needed
		--if meshOp.getMapSupport snapMesh 0 do (
		--	local allWhite = true
		--	for i = 1 to (meshOp.getNumMapVerts snapMesh 0) while allWhite do (
		--		local vc = meshOp.getMapVert snapMesh 0 i
		--		if vc.x < 0.999 or vc.y < 0.999 or vc.z < 0.999 do allWhite = false
		--	)
		--	hasColors = not allWhite
		--)
		local flags = 0
		if hasNormals do flags = bit.or flags 0x01
		if hasUVs    do flags = bit.or flags 0x02
		if hasColors do flags = bit.or flags 0x04
		if hasSkin   do flags = bit.or flags 0x08

		local faceMatIDs = #()
		for i = 1 to origNumFaces do append faceMatIDs (getFaceMatID snapMesh i)

		local usedMatIDs = #()
		for i = 1 to origNumFaces do (
			local mid = faceMatIDs[i]
			if findItem usedMatIDs mid == 0 do append usedMatIDs mid
		)
		sort usedMatIDs

		struct SubMeshData (matIdx, faceIndices)
		local submeshes = #()
		for mid in usedMatIDs do (
			local sm = SubMeshData matIdx:(mid - 1) faceIndices:#()
			for fi = 1 to origNumFaces do (
				if faceMatIDs[fi] == mid do append sm.faceIndices fi
			)
			append submeshes sm
		)
		local numSubs = submeshes.count

		local orderedFaces = #()
		local subFirstFace = #()
		local subNumFaces = #()
		for s = 1 to numSubs do (
			append subFirstFace orderedFaces.count
			for fi in submeshes[s].faceIndices do append orderedFaces fi
			append subNumFaces submeshes[s].faceIndices.count
		)
		local totalFaces = orderedFaces.count


		local vertNormals = #()
		if hasNormals do (
			local enMod = Edit_Normals()
			addModifier obj enMod
			select obj
			local numEN = enMod.GetNumNormals()
			local nrmAccum = #()
			local nrmCount = #()
			for i = 1 to origNumVerts do (append nrmAccum [0,0,0]; append nrmCount 0)
			for fi = 1 to origNumFaces do (
				for ci = 1 to 3 do (
					local nID = enMod.GetNormalID fi ci
					if nID > 0 and nID <= numEN do (
						local n = enMod.GetNormal nID
						local fv = getFace snapMesh fi
						local vi = if ci == 1 then (fv.x as integer) else if ci == 2 then (fv.y as integer) else (fv.z as integer)
						nrmAccum[vi] = nrmAccum[vi] + n
						nrmCount[vi] = nrmCount[vi] + 1
					)
				)
			)
			for i = 1 to origNumVerts do (
				if nrmCount[i] > 0 then append vertNormals (normalize nrmAccum[i])
				else append vertNormals [0,0,1]
			)
			deleteModifier obj enMod
		)


		local skinBI = #()
		local skinBW = #()
		if hasSkin and skinMod != undefined do (
			modPanel.setCurrentObject skinMod
			for vi = 1 to origNumVerts do (
				local bi = #(0, 0, 0, 0)
				local bw = #(0.0, 0.0, 0.0, 0.0)
				local nw = skinOps.getVertexWeightCount skinMod vi
				local maxW = if nw < 4 then nw else 4
				for w = 1 to maxW do (
					bi[w] = (skinOps.getVertexWeightBoneID skinMod vi w) - 1
					bw[w] = skinOps.getVertexWeight skinMod vi w
				)
				append skinBI bi
				append skinBW bw
			)
		)


		-- regenerate output mesh to account for edits

		local newPositions = #()
		local newNormals = #()
		local newUVs = #()
		local newSkinBI = #()
		local newSkinBW = #()
		local newColors = #()
		local newFaceVerts = #()

		local vertMap = #()

		for i = 1 to totalFaces do (
			local fi = orderedFaces[i]
			local geoFace = getFace snapMesh fi
			local uvFace = if hasUVs then (meshOp.getMapFace snapMesh 1 fi) else geoFace
			local newFace = [0,0,0]

			for ci = 1 to 3 do (
				local gi = if ci == 1 then (geoFace.x as integer) else if ci == 2 then (geoFace.y as integer) else (geoFace.z as integer)
				local ui = if ci == 1 then (uvFace.x as integer) else if ci == 2 then (uvFace.y as integer) else (uvFace.z as integer)

				local key = (gi as string) + "_" + (ui as string)
				local found = findItem vertMap key

				if found == 0 then (
					append newPositions (getVert snapMesh gi)
					if hasNormals do append newNormals vertNormals[gi]
					if hasUVs do (
						local uv = meshOp.getMapVert snapMesh 1 ui
						append newUVs uv
					)
					if hasColors do append newColors (meshOp.getMapVert snapMesh 0 gi)
					if hasSkin do (
						append newSkinBI skinBI[gi]
						append newSkinBW skinBW[gi]
					)
					append vertMap key
					if ci == 1 then newFace.x = vertMap.count
					else if ci == 2 then newFace.y = vertMap.count
					else newFace.z = vertMap.count
				) else (
					if ci == 1 then newFace.x = found
					else if ci == 2 then newFace.y = found
					else newFace.z = found
				)
			)
			append newFaceVerts newFace
		)

		local numVerts = newPositions.count


		local f = fopen filePath "wb"
		if f == undefined do (
			messageBox ("Cannot create file: " + filePath)
			delete snapMesh
			return false
		)

		-- header
		writeLong f 0x4D455348 #unsigned
		writeLong f 4 #unsigned
		writeLong f numVerts #unsigned
		writeLong f totalFaces #unsigned
		writeLong f numBones #unsigned
		writeLong f numMats #unsigned
		writeLong f flags #unsigned
		writeLong f numSubs #unsigned

		for i = 1 to numBones do (
			local boneNode = skinBoneObjs[i]
			local tfm = if boneNode != undefined then boneNode.transform else (matrix3 1)
			writeFloat f tfm.row1.x; writeFloat f tfm.row1.y; writeFloat f tfm.row1.z
			writeFloat f tfm.row2.x; writeFloat f tfm.row2.y; writeFloat f tfm.row2.z
			writeFloat f tfm.row3.x; writeFloat f tfm.row3.y; writeFloat f tfm.row3.z
			writeFloat f tfm.row4.x; writeFloat f tfm.row4.y; writeFloat f tfm.row4.z
			local parentBoneIdx = -1
			if boneNode != undefined and boneNode.parent != undefined do (
				for j = 1 to numBones do (
					if skinBoneObjs[j] == boneNode.parent do (
						parentBoneIdx = j - 1; exit
					)
				)
			)
			writeLong f parentBoneIdx

			local boneID = -1
			if boneNode != undefined do (
				local bidVal = getUserProp boneNode "BoneID"
				if bidVal != undefined do (
					try (
						local bidStr = bidVal as string
						if bidStr.count >= 2 and (substring bidStr 1 2) == "0x" then
							boneID = bit.hexAsInt (substring bidStr 3 -1)
						else
							boneID = bidVal as integer
					) catch ()
				)
			)
			writeLong f boneID
			local boneType = 0
			if boneNode != undefined do (
				local btVal = getUserProp boneNode "BoneType"
				if btVal != undefined do (
					try (
						local btStr = btVal as string
						if btStr.count >= 2 and (substring btStr 1 2) == "0x" then
							boneType = bit.hexAsInt (substring btStr 3 -1)
						else
							boneType = btVal as integer
					) catch ()
				)
			)
			writeLong f boneType
			local skinBoneID = -1
			if boneNode != undefined do (
				local sbVal = getUserProp boneNode "SkinBoneID"
				if sbVal != undefined do (
					try ( skinBoneID = sbVal as integer ) catch ()
				)
			)
			writeLong f skinBoneID
			local groupID = 0
			if boneNode != undefined do (
				local gidVal = getUserProp boneNode "GroupID"
				if gidVal != undefined do (
					try ( groupID = gidVal as integer ) catch ()
				)
			)
			writeLong f groupID
			local boneName = if boneNode != undefined then boneNode.name else ("Bone" + (i as string))
			writeFixedString f boneName 64
		)


		for i = 1 to numMats do (
			local mat = matList[i]
			local isMK = (classOf mat == MK_Mtl)

			if isMK then (
				local mc = mat.matColor
				writeByte f (mc.r as integer); writeByte f (mc.g as integer); writeByte f (mc.b as integer)
				writeByte f ((mat.matAlpha * 255.0) as integer)
			) else (
				local dc = if hasProperty mat #diffuse then mat.diffuse else (color 255 255 255)
				writeByte f (dc.r as integer); writeByte f (dc.g as integer); writeByte f (dc.b as integer)
				writeByte f 255
			)
			if isMK then (
				writeFloat f mat.rw_ambient; writeFloat f mat.rw_specular; writeFloat f mat.rw_diffuse
			) else (
				writeFloat f 1.0; writeFloat f 1.0; writeFloat f 1.0
			)
			local texName = ""
			local alphaTexName = ""
			if isMK then (
				if mat.texmap_diffuse != undefined do (
					if classOf mat.texmap_diffuse == Bitmaptexture do (
						texName = getFilenameFile mat.texmap_diffuse.filename
					)
				)
				writeLong f (if texName != "" then 1 else 0)
			) else (
				if hasProperty mat #diffuseMap and mat.diffuseMap != undefined do (
					if classOf mat.diffuseMap == Bitmaptexture do (
						texName = getFilenameFile mat.diffuseMap.filename
					)
				)
				writeLong f (if texName != "" then 1 else 0)
			)

			writeFixedString f texName 64

			writeFixedString f alphaTexName 64

			if isMK then (
				writeByte f mat.texture_filter
				local uvAddr = mat.uv_addressing
				writeByte f (bit.shift uvAddr -4)
				writeByte f (bit.and uvAddr 0xF)
			) else (
				writeByte f 0; writeByte f 0; writeByte f 0
			)
			writeByte f 0
			writeShort f 0
			if isMK then writeByte f (if mat.has_skymipmap then 1 else 0)
			else writeByte f 0
			writeByte f 0
			if isMK then (
				writeLong f mat.skymipmap_K; writeLong f mat.skymipmap_L
			) else (
				writeLong f 0; writeLong f 0
			)

			if isMK then (
				local mkFlagsUpper = 0
				if mat.mk_useBonePal do mkFlagsUpper = bit.or mkFlagsUpper 0x4000
				if mat.mk_uv_scroll do mkFlagsUpper = bit.or mkFlagsUpper 0x8000
				local mkFlag = bit.or mat.mk_type (bit.shift mkFlagsUpper 16)
				writeLong f mkFlag
				writeLong f mat.mk_type
				writeLong f mkFlagsUpper
				writeShort f mat.mk_unk1
				writeShort f mat.mk_unk2
				writeFloat f mat.mk_unk3
				writeLong f (if mat.mk_useBonePal then 1 else 0)
				local bpArr = #()
				if mat.mk_useBonePal and mat.mk_bonePalette != "" do (
					local parts = filterString mat.mk_bonePalette ","
					for p in parts do (
						local val = p as integer
						if val != undefined do append bpArr val
					)
				)
				writeLong f bpArr.count
				for j = 1 to 64 do (
					if j <= bpArr.count then writeLong f bpArr[j]
					else writeLong f 0
				)

				writeByte f mat.mk_spec_r; writeByte f mat.mk_spec_g
				writeByte f mat.mk_spec_b; writeByte f mat.mk_spec_a
				writeFloat f mat.mk_unk5
				writeFloat f mat.mk_unk6

				writeLong f (if mat.mk_uv_scroll then 1 else 0)
				writeFloat f mat.mk_uv_scroll_x; writeFloat f mat.mk_uv_scroll_y
				writeFloat f mat.mk_uv_scroll_z; writeFloat f 0.0

				writeLong f mat.mk_meshType
			) else (
				writeLong f 0   -- mkFlag
				writeLong f 0   -- mkType
				writeLong f 0   -- mkFlagsUpper
				writeShort f 0; writeShort f 0  -- unk1, unk2
				writeFloat f 0.0  -- unk3
				writeLong f 0   -- useBonePalette
				writeLong f 0   -- bonePaletteCount
				for j = 1 to 64 do writeLong f 0  -- bonePalette[64]
				writeByte f 127; writeByte f 127; writeByte f 127; writeByte f 255  -- specular
				writeFloat f 0.0; writeFloat f 0.0  -- unk5, unk6
				writeLong f 0   -- uvScroll
				writeFloat f 0.0; writeFloat f 0.0; writeFloat f 0.0; writeFloat f 0.0  -- scroll xyz pad
				writeLong f 0   -- meshType
			)
		)


		for i = 1 to numVerts do (
			local pos = newPositions[i]
			writeFloat f pos.x; writeFloat f pos.y; writeFloat f pos.z
			if hasNormals do (
				local n = newNormals[i]
				writeFloat f n.x; writeFloat f n.y; writeFloat f n.z
			)
			if hasUVs do (
				writeFloat f newUVs[i].x
				writeFloat f newUVs[i].y
			)
			if hasColors do (
				local vc = newColors[i]
				writeByte f ((vc.x * 255.0) as integer)
				writeByte f ((vc.y * 255.0) as integer)
				writeByte f ((vc.z * 255.0) as integer)
				writeByte f 255
			)
			if hasSkin do (
				local bi = newSkinBI[i]
				local bw = newSkinBW[i]
				writeByte f bi[1]; writeByte f bi[2]; writeByte f bi[3]; writeByte f bi[4]
				writeFloat f bw[1]; writeFloat f bw[2]; writeFloat f bw[3]; writeFloat f bw[4]
			)
		)


		local newSubFirstFace = #()
		local faceIdx = 0
		for s = 1 to numSubs do (
			append newSubFirstFace faceIdx
			faceIdx += subNumFaces[s]
		)

		for s = 1 to numSubs do (
			writeLong f submeshes[s].matIdx #unsigned
			writeLong f newSubFirstFace[s] #unsigned
			writeLong f subNumFaces[s] #unsigned
		)


		for i = 1 to totalFaces do (
			local fv = newFaceVerts[i]
			writeLong f ((fv.x as integer) - 1) #unsigned
			writeLong f ((fv.y as integer) - 1) #unsigned
			writeLong f ((fv.z as integer) - 1) #unsigned
		)

		-- HAnim bone list
		local storedHAnimList = getUserProp obj "HAnimList"
		if storedHAnimList != undefined and storedHAnimList != "" then (
			local entries = filterString storedHAnimList ";"
			writeLong f entries.count #unsigned
			for i = 1 to entries.count do (
				local parts = filterString entries[i] ","
				local packed = parts[1] as integer
				local sbID = parts[2] as integer
				local bType = parts[3] as integer
				writeLong f packed #unsigned
				writeLong f sbID
				writeLong f bType
			)
		) else (
			-- generate if no stored hanimlist
			local hanimEntries = #()
			for i = 1 to numBones do (
				local boneNode = skinBoneObjs[i]
				local bID = -1
				local bType = 0
				local sbID = -1
				local gID = 0
				if boneNode != undefined do (
					local bidStr = getUserProp boneNode "BoneID"
					if bidStr != undefined do (
						try (
							if (substring bidStr 1 2) == "0x" then bID = bit.hexAsInt (substring bidStr 3 -1)
							else bID = bidStr as integer
						) catch ()
					)
					local btStr = getUserProp boneNode "BoneType"
					if btStr != undefined do (
						try (
							if (substring btStr 1 2) == "0x" then bType = bit.hexAsInt (substring btStr 3 -1)
							else bType = btStr as integer
						) catch ()
					)
					local sbStr = getUserProp boneNode "SkinBoneID"
					if sbStr != undefined do ( try ( sbID = sbStr as integer ) catch () )
					local gidStr = getUserProp boneNode "GroupID"
					if gidStr != undefined do ( try ( gID = gidStr as integer ) catch () )
				)
				if bID >= 0 do (
					append hanimEntries #(bID, gID, sbID, bType)
				)
			)
			writeLong f hanimEntries.count #unsigned
			for i = 1 to hanimEntries.count do (
				local e = hanimEntries[i]
				local packed = (bit.and e[1] 0xFFFF) + (bit.shift (bit.and e[2] 0xFFFF) 16)
				writeLong f packed #unsigned
				writeLong f e[3]
				writeLong f e[4]
			)
		)

		fclose f
		delete snapMesh

		messageBox ("Exported: " + filePath + "\nRemember to convert the file to .dff using mkmodeltool!")
		true
	)