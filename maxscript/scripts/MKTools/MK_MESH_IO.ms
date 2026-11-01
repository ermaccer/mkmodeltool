if MKMESHinInfo == undefined then fileIn (scriptspath+"\\MKTools\\ImportMKMesh.ms") quiet:true
else if MKMESHinInfo() != 051215 then fileIn (scriptspath+"\\MKTools\\ImportMKMesh.ms") quiet:true
if MKMESHoutInfo == undefined then fileIn (scriptspath+"\\MKTools\\ExportMKMesh.ms") quiet:true
else if MKMESHoutInfo() != 051219 then fileIn (scriptspath+"\\MKTools\\ExportMKMesh.ms") quiet:true



rollout MKaboutRoll "About" (
	label l1 "MK Mesh IO"
	label l2 " "

	label l4 "by ermaccer" 
	label l5 "with big help from Allen"
	label l6 "Inspired by Kam's GTA Plugin"
)


rollout MkMeshinPara "Import" (

local fname = undefined
local no_clump = #()

	button imp "Import MK Mesh" width:120 height:40 talign:#center
    
	on imp pressed do (
			local meshFile = getOpenFileName caption:"Import MK .mesh" types:"MK Mesh (*.mesh)|*.mesh|All Files (*.*)|*.*|"
	if meshFile != undefined do (
		with undo on (
			MKMeshImp meshFile
		)
	)
	)--end on imp
    
)--end rollout DFFinPara


rollout MkMeshexPara "Export" (
	group "Export Flags" (
		checkbox CPV "Vertex Color" width:140 align:#left tooltip:"export Prelit Vertex Color"
	)
	
	button exp "Export MK Mesh" width:120 height:40 tooltip:"Only selected objects will export" align:#center

	
	on exp pressed do (
		if selection.count != 1 then (
		messageBox "No objects selected or too many. Only one object is supported"
	) else (
		local obj = selection[1]
		local meshFile = getSaveFileName caption:"Export MK .mesh" types:"MK Mesh (*.mesh)|*.mesh|All Files (*.*)|*.*|"
		if meshFile != undefined do (	 
			MKMeshExp obj meshFile CPV.checked
		)
	)

	)--end on exp

)--end rollout


rollout MKMeshUtils "Utilities" (
    group "Hide Mesh By Mat ID" (
        spinner matID "ID" fieldwidth:100 type:#integer range:[0,10000,0]
        button hidematID "Hide" width:58 height:20 align:#left across:2
        button showmatID "Show" width:58 height:20 align:#right
        button showAll "Show All" width:120 height:20 align:#center
        button hideAllExcept "Show Only This" width:120 height:20 align:#center tooltip:"Hide everything except this material ID"
    )
    
	on hidematID pressed do (
    if selection.count == 1 do (
        local obj = selection[1]
        local m = snapshotAsMesh obj
        local hideFaces = #{}
        for fi = 1 to m.numfaces do (
            if getFaceMatID m fi == matID.value do hideFaces[fi] = true
        )
        delete m
        local current = meshop.getHiddenFaces obj
        meshop.setHiddenFaces obj (current + hideFaces)
        update obj
		redrawViews()
    )
	)
	
	on showmatID pressed do (
		if selection.count == 1 do (
			local obj = selection[1]
			local m = snapshotAsMesh obj
			local showFaces = #{}
			for fi = 1 to m.numfaces do (
				if getFaceMatID m fi == matID.value do showFaces[fi] = true
			)
			delete m
			local current = meshop.getHiddenFaces obj
			meshop.setHiddenFaces obj (current - showFaces)
			update obj
			redrawViews()
		)
	)
	
	on showAll pressed do (
		if selection.count == 1 do (
			local obj = selection[1]
			meshop.setHiddenFaces obj #{}
			update obj
			redrawViews()
		)
	)
	
	on hideAllExcept pressed do (
		if selection.count == 1 do (
			local obj = selection[1]
			local m = snapshotAsMesh obj
			local hideFaces = #{}
			for fi = 1 to m.numfaces do (
				if getFaceMatID m fi != matID.value do hideFaces[fi] = true
			)
			delete m
			meshop.setHiddenFaces obj hideFaces
			update obj
			redrawViews()
		)
	)
	group "Info" (
		button printmaxBoneID "Print Max Bone ID" width:120 height:20 align:#center
		on printmaxBoneID pressed do (
			if selection.count == 1 do (
				local obj = selection[1]
				local s = getUserProp obj "HAnimList"
				if s != undefined and s != "" then (
					local entries = filterString s ";"
					format "Current HAnim entries: %\n" entries.count
					local maxSBID = 0
					for e in entries do (
						local parts = filterString e ","
						local sbid = parts[2] as integer
						if sbid > maxSBID do maxSBID = sbid
					)
					format "Max skinBoneID: %\n" maxSBID
					format "Next free: %\n" (maxSBID + 1)
				) else (
					format "No stored HAnim list\n"
				)
			)
		)
	)
	group "Add Bone (test)" (
    spinner newBoneID "Bone ID" fieldwidth:50 type:#integer range:[0,65535,0]
    spinner newSkinBoneID "Skin Bone ID" fieldwidth:50 type:#integer range:[0,255,0]
    spinner newBoneType "Bone Type" fieldwidth:50 type:#integer range:[0,10,0]
    button appendHAnim "Add to HAnim List" width:100 height:20 align:#center
    on appendHAnim pressed do (
        if selection.count == 1 do (
            local obj = selection[1]
            local s = getUserProp obj "HAnimList"
            if s == undefined do s = ""
            if s != "" do s += ";"
            s += (newBoneID.value as string) + "," + (newSkinBoneID.value as string) + "," + (newBoneType.value as string)
            setUserProp obj "HAnimList" s
            format "Added: BoneID=% SkinBoneID=% BoneType=%\n" newBoneID.value newSkinBoneID.value newBoneType.value
        )
    )
)
)


if MKTOOLS != undefined then ( closeRolloutFloater MKTOOLS; gc() )
MKTOOLS = newRolloutFloater "MK MESH IO" 175 620 10 50		
addRollout MkMeshinPara MKTOOLS --rolledup:true
addRollout MkMeshexPara MKTOOLS --rolledup:true
addRollout MkMeshUtils MKTOOLS 
addRollout MKaboutRoll MKTOOLS rolledup:true
