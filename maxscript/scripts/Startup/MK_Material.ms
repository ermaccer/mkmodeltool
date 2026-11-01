plugin Material MK_Mtl
	name:"MK Material"
	classID:#(0x3a8b1942, 0x62509c80)
	extends:Standard replaceUI:true version:1
(
	parameters main rollout:params (
		-- RW base material
		matColor         type:#color   default:white  ui:matColor
		matAlpha         type:#float   default:1.0    ui:matAlpha
		rw_ambient       type:#float   default:1.0    ui:rwAmb
		rw_diffuse       type:#float   default:1.0    ui:rwDif
		rw_specular      type:#float   default:1.0    ui:rwSpec
		texmap_diffuse   type:#TextureMap              ui:texDiffuse
		texture_name     type:#string  default:""
		texture_filter   type:#integer default:0
		uv_addressing    type:#integer default:0
		has_skymipmap    type:#boolean default:false
		skymipmap_K      type:#integer default:0
		skymipmap_L      type:#integer default:0

		on matColor set val do (
			delegate.diffuse = val * rw_diffuse
			if delegate.diffuseMap != undefined do
				try(delegate.diffuseMap.color1 = val * rw_diffuse) catch()
		)
		on rw_diffuse set val do (
			delegate.diffuse = matColor * val
		)
		on matAlpha set val do delegate.opacity = val * 100
		on texmap_diffuse set tex do (
			delegate.diffuseMap = tex
			delegate.diffuseMapEnable = true
		)
	)

	parameters mkParams rollout:mkRollout (
		mk_type          type:#integer default:5      ui:mkType
		mk_meshType      type:#integer default:0      ui:mkMeshType
		mk_unk1          type:#integer default:0      ui:mkUnk1
		mk_unk2          type:#integer default:0      ui:mkUnk2
		mk_unk3          type:#float   default:0.0    ui:mkUnk3
		mk_spec_r        type:#integer default:127    ui:mkSpecR
		mk_spec_g        type:#integer default:127    ui:mkSpecG
		mk_spec_b        type:#integer default:127    ui:mkSpecB
		mk_spec_a        type:#integer default:255    ui:mkSpecA
		mk_unk5          type:#float   default:0.0    ui:mkUnk5
		mk_unk6          type:#float   default:0.0    ui:mkUnk6
		mk_uv_scroll     type:#boolean default:false  ui:mkUVScroll
		mk_uv_scroll_x   type:#float   default:0.0    ui:mkScrollX
		mk_uv_scroll_y   type:#float   default:0.0    ui:mkScrollY
		mk_uv_scroll_z   type:#float   default:0.0    ui:mkScrollZ
		mk_useBonePal    type:#boolean default:false  ui:mkUseBonePal
		mk_bonePalette   type:#string  default:""	  ui:mkBonePalette
	)

	rollout params "RenderWare Material" (
		group "RpMaterial Parameters" (
			label        colorlbl  "Color"    pos:[57,30]
			colorpicker  matColor  ""         width:51 height:16 pos:[89,28]
			label        alphalbl  "Alpha"    pos:[224,30]
			spinner      matAlpha  ""         width:49 height:16 type:#float range:[0,1.0,1.0] pos:[263,28]

			label        surfp1lbl "Surface"    pos:[44,62]
			label        surfp2lbl "Properties" pos:[32,75]
			label        amblbl    "Ambient"  pos:[96,50]
			spinner      rwAmb     ""         range:[0,1,0] width:49 height:16 pos:[92,67]
			label        diflbl    "Diffuse"  pos:[182,50]
			spinner      rwDif     ""         range:[0,1,0] width:49 height:16 pos:[177,67]
			label        speclbl   "Specular" pos:[267,50]
			spinner      rwSpec    ""         range:[0,1,0] width:49 height:16 pos:[263,67]

			label        texlbl    "Texture"  pos:[42,98]
			mapbutton    texDiffuse "None"    pos:[92,93] width:219 height:21
		)
	)

	rollout mkRollout "MK Material Properties" (
		group "MK Extension" (
			label        typelbl   "Type (flag)"  pos:[20,30]
			spinner      mkType    ""             range:[0,65535,5] type:#integer width:60 pos:[100,28]
			label        meshtlbl  "Mesh Type"    pos:[180,30]
			spinner      mkMeshType ""            range:[0,255,0] type:#integer width:60 pos:[250,28]
			label        u1lbl     "Unk1"         pos:[20,55]
			spinner      mkUnk1    ""             range:[-32768,32767,0] type:#integer width:60 pos:[55,53]
			label        u2lbl     "Unk2"         pos:[130,55]
			spinner      mkUnk2    ""             range:[-32768,32767,0] type:#integer width:60 pos:[165,53]
			label        u3lbl     "Unk3"         pos:[240,55]
			spinner      mkUnk3    ""             range:[-9999,9999,0] type:#float width:60 pos:[270,53]
		)
		group "Specular Color" (
			label        srlbl     "R" pos:[20,105]
			spinner      mkSpecR   ""  range:[0,255,127] type:#integer width:49 pos:[33,103]
			label        sglbl     "G" pos:[95,105]
			spinner      mkSpecG   ""  range:[0,255,127] type:#integer width:49 pos:[108,103]
			label        sblbl     "B" pos:[170,105]
			spinner      mkSpecB   ""  range:[0,255,127] type:#integer width:49 pos:[183,103]
			label        salbl     "A" pos:[245,105]
			spinner      mkSpecA   ""  range:[0,255,255] type:#integer width:49 pos:[258,103]
		)
		group "Extra Parameters" (
			label        u5lbl     "Unk5"  pos:[20,150]
			spinner      mkUnk5    ""      range:[-9999,9999,0] type:#float width:60 pos:[55,148]
			label        u6lbl     "Unk6"  pos:[130,150]
			spinner      mkUnk6    ""      range:[-9999,9999,0] type:#float width:60 pos:[165,148]
		)
		group "UV Scroll" (
			checkbox     mkUVScroll "Enable UV Scroll (requires meshtype 129)" pos:[20,192]
			label        sxlbl     "Speed X" pos:[20,215]
			spinner      mkScrollX ""        range:[-999,999,0] type:#float width:60 pos:[70,213]
			label        sylbl     "Speed Y" pos:[140,215]
			spinner      mkScrollY ""        range:[-999,999,0] type:#float width:60 pos:[190,213]
			label        szlbl     "Speed Z" pos:[260,215]
			spinner      mkScrollZ ""        range:[-999,999,0] type:#float width:60 pos:[300,213]
		)
		group "Internal" (
			checkbox     mkUseBonePal "Store Bone Palette" pos:[20,255]
			edittext     mkBonePalette "Stored Data" pos:[20,280]
		)
	)
	
	on create do (
		this.name = delegate.name
		--delegate.adLock = false
		delegate.diffuseMap = RGB_Multiply()
		delegate.diffuseMapEnable = true

	)
	on load do (
		this.name = delegate.name
	)
)
