# papaya
This is a platformer game made for a college project. 
[Presentation](.docs/papaaya.pptx) [Report](.docs/Papaya - sprawozdanie.docx)
It features:
 - Bosses
 - Advanced bosses
 - Map Editor
![Main game](./docs/main_game.png)
![Editor](./docs/editor.png)
# How to get the game
## Download the precompiled binary(Easy)
Download `.zip` for your system and launch it.
## Build
### Requirements
 - `raylib>=5.5`
 - `cmake>=3.16`
 -  `C++` 20
### Getting source files
Clone this repo with git
```
git clone https://github.com/EleuteriaHortensjaHiacynta/papaya
```
### Building
Create and enter your build catalog in this example `build/`
```
mkdir build
cd build
```
Initialize build folder with `cmake`
```
cmake ..
```
Build game 
```
cmake --build . --parallel
```
Now the compiled game is in `bin/` you can launch it with:
```
cd bin
./papaya
```
