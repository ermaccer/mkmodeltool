// mkmodeltool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "MKModelTool.h"

int main(int argc, char* argv[])
{
	bool _e_switch = false;
	bool _c_switch = false;
	std::string t_param;
	std::string o_param;

	if (argc == 1) {
		std::cout << "Usage: mkmodeltool <optional params> <file>\n"
			<< "    -t <type> Specify model conversion type: CHARACTER, SKINNED, REFLECTION.\n"
			<< "    -o <name> Specify output filename\n"
			<< "    -e Export .mesh file from a .dff model\n"
			<< "    -c Convert .mesh file to a .dff model\n"
			;

		return 1;
	}

	// params
	for (int i = 1; i < argc - 1; i++)
	{
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
			return 1;
		}
		switch (argv[i][1])
		{
		case 'e': _e_switch = true;
			break;
		case 'c': _c_switch = true;
			break;
		case 'o':
			i++;
			o_param = argv[i];
			break;
		case 't':
			i++;
			t_param = argv[i];
			break;
		default:
			std::cout << "ERROR: Param does not exist: " << argv[i] << std::endl;
			return 0;
			break;
		}
	}


	const char* path = nullptr;
	if (argc > 2)
		path = argv[argc - 1];
	else if (argc == 2)
		path = argv[1];

	if (!path)
		return 0;


	std::string fileName = path;
	//fileName = fileName.substr(0, fileName.find_last_of(".") + 1);


	EExportType type = Export_SkinnedPalette;
	if (t_param == "CHARACTER") type = Export_SkinnedPalette;
	if (t_param == "SKINNED") type = Export_Skinned;
	if (t_param == "REFLECTION") type = Export_SkinnedNoUV;
	// not supported and probably wont be
	//if (t_param == "BGND") type = Export_Generic;

	std::string extension = ".mesh";

	if (_c_switch)
		extension = ".dff";

	std::string output = fileName + extension;

	if (!o_param.empty())
		output = o_param;


	MKModelTool model;

	if (_e_switch)
	{
		model.LoadDFF(path);
		model.DumpMesh(output);
		return 0;

	}

	if (_c_switch)
	{
		model.LoadMesh(path);
		model.DumpDFF(output, type);
		return 0;
	}

	return 0;
}
