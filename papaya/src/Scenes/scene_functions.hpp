#pragma once
#include <raylib.h>

inline void changeScene(int& state, int value, bool& shouldLeave) {
	state = value;
	shouldLeave = true;
}

inline void exitProgram(bool& shouldQuit) {
	shouldQuit = true;
}
