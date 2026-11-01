utility MortalKombatTools "MK Scripts" 
(
	button script00 "MK MESH IO" width:140 align:#center
	on script00 pressed do ( 
		try ( fileIn (scriptspath+"\\MKTools\\MK_MESH_IO.ms")   )
		catch messagebox "Script missing!" title:"Error"
	)	
	    	
)
openUtility MortalKombatTools