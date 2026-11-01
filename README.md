# mkmodeltool

PS2/PSP model conversion tool for PS2 MK games.

# About

Encoded models from PS2/PSP are saved into a generic .mesh format which removes the need to decode
on any importing software (also speeds up). 

The generated models are not meant to be used with hardware due to increased sizes and likely other things, they were tested only on emulator.

This is meant to be used with skinned models only, backgrounds and or static objects aren't directly supported. Those can be officially exported with RW 3.5 SDK.

# Support
- MKDA/MKD/MKA PS2 and PSP (MKU) to .mesh
- .mesh to MKD/MKA .dff format

# Support (Software)
Only 3Ds Max MaxScript is supported, should work with any version after 2009. Tested on 2017.

Python Blender (4.0) proof of script by Allen to import/export directly is available, but it is not finished nor tested a lot.


# Credits
- Allen (https://github.com/leeao), majority of VIF encoding and decoding research and code comes from him, this tool is continuation of his work with permission
- aap (https://github.com/aap), native RW formats, librw, decoding
- Kam, for GTA plugins, inspiration in how this is supposed to work and also code base for maxscript
- NvTriStrip (https://github.com/turbulenz/NvTriStrip) for tri strips generation

# Credits (cont.)
- Fred Ward (RIP), amazing actor
- Relic Entertainment for creating Dawn Of War
