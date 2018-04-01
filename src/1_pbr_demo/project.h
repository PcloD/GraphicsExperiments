#pragma once

#include <vector>
#include <string>

struct Project
{
	std::string				 name;
	std::vector<std::string> platforms;
	std::string				 project_directory;
};
