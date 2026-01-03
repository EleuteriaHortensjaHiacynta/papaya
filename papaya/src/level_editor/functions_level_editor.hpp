#pragma once

#include <raylib.h>
#include <iostream>
#include <string>
#include "external_headers/tinyfiledialogs.hpp"

inline std::string openFileDialog(const char *tittle = "Select file to open") {
	const char *result = tinyfd_openFileDialog(tittle, "", 0, nullptr, nullptr, 0);
	if (result) return std::string(result);
	return "";
}

// either all arguments need default values or none of them have defaults
inline std::string saveFileDialog(const char* tittle = "Select file to save", int x = 0, int y = 0) {

	std::string currentPath = std::filesystem::current_path().string();
	std::string defaultName = currentPath + "/chunk" + std::to_string(x) + "-" + std::to_string(y) + ".json";
	const char* result = tinyfd_saveFileDialog(tittle, defaultName.c_str(), 0, nullptr, nullptr);
	if (result) return std::string(result);
	return "";
}