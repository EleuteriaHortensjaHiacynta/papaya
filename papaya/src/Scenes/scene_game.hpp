#pragma once

// G³ówna funkcja rozgrywki
// windowWidth/Height - rozmiar okna
// shouldQuit - flaga do wyjœcia z gry
// state - do prze³¹czania scen (0=Menu, 1=Game, 2=Editor)
void sceneGame(int windowWidth, int windowHeight, bool& shouldQuit, int& state);