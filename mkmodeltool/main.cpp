// mkmodeltool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "MKModelTool.h"

int main(int argc, char* argv[])
{
	bool _d_switch = false;
	bool _b_switch = false;
	bool _v_switch = false;
	bool _e_switch = false;
	bool _g_switch = false;
	bool _c_switch = false;
	bool _p_switch = false;
	std::string m_param;
	std::string a_param;

	if (argc == 1) {
		std::cout << "Usage: mkmodeltool <optional params> <file>\n"
			<< "    -f <format> Specify model format.\n"
			<< "    -e <modelType>  Specify export format (.DFF only): obj, smd\n"
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
		case 'b': _b_switch = true;
			break;
		case 'd': _d_switch = true;
			break;
		case 'v': _v_switch = true;
			break;
		case 'e': _e_switch = true;
			break;
		case 'g': _g_switch = true;
			break;
		case 'c': _c_switch = true;
			break;
		case 'p': _p_switch = true;
			break;
		case 'm':
			i++;
			m_param = argv[i];
			break;
		case 'a':
			i++;
			a_param = argv[i];
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



    MKModelTool model;

	if (!_b_switch)
	{
		model.LoadDFF("COSTUME.dff");
		model.DumpMesh("OUTPUT.mesh");
	}
	else
	{
		model.LoadMesh("OUTPUT.mesh");
		model.DumpDFF("COSTUMEnew.dff");
	}

   // model.LoadMesh("SmokeFinalScript.mesh");
   // model.DumpDFF("tfg.dff");

    return 0;
}
