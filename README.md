# Trigger Control

Dark Mode           |  Light Mode
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/45927311/166091284-a06013df-f443-48a1-8e15-514690b43200.png) | ![image](https://user-images.githubusercontent.com/45927311/166091275-4f970e14-5a16-4f1e-b96b-4e823cdcfcae.png)

**IF YOU ARE USING DS4WINDOWS:**  
Download this version, later versions have trigger effects which will overwrite the ones made by this app
https://github.com/Ryochan7/DS4Windows/releases/tag/v2.2.2  


Trigger Control is a project that allows you to control the adaptive triggers of the dualsense controller on linux and windows using a gui, works through bluetooth and usb.

requirements:  
c++17 compiler   

required libraries:  
libSDL2 >= 2.0.14  
libdbus  
libnotify (low controller battery notification)  
   
Ubuntu 20.04's SDL version is too out of date for this project, for distros like linux mint 20, and zorin os 16,  
you will have to manually compile SDL2 by yourself and point the pkg config path to the correct place  

optional:  
libSDL2 >= 2.0.22 (for wayland)  
libdecor (for wayland)  
ttf-dejavu (better font)

**For Ubuntu 22.04 LTS:**
```
sudo apt-get install cmake libsdl2-dev libnotify-dev
mkdir build
cd build
cmake ..
make
```

**Installing them on arch linux (or any arch based distro):**  
```
sudo pacman -S sdl2 base-devel libdecor ttf-dejavu dbus libnotify
```  

**How to compile**  

```
mkdir build  
cd build  
cmake ..  
make  
```  

**Run**   
```
./trigger-control
```  

**Install**  
install dependencies first
```
git clone https://github.com/Etaash-mathamsetty/trigger-control.git  
cd trigger-control    
mkdir build  
cd build  
cmake ..  
make  
sudo cp trigger-control /usr/bin  
```

**Update**  
enter the directory in which you cloned this repo  
```
git pull  
cd build    
make   
sudo cp trigger-control /usr/bin  
```

**Cross Compile for Windows**  
I recommend arch linux for this, but you can try your luck with any other distro  
first, clone the repo, enter the directory it was cloned in, then run the following commands (with any AUR helper)  
feel free to use the [ArchWSL](https://github.com/yuk7/ArchWSL) project to cross compile this for windows  
```
yay -S mingw-w64-pkg-config mingw-w64-sdl2 
```
then run,  
```
mkdir build-win  
cd build-win  
cmake -DCMAKE_TOOLCHAIN_FILE=../windows.cmake ..  
```
you will get a .exe file as an output, but you won't be able to run it without copying the nessessary dll files, so copy  
```
SDL2.dll
```
from  
```
/usr/x86_64-w64-mingw32/bin/
```  
to the folder the git repo is in, and now you should be able to run the windows version of the program... hopefully  
  
things I learned from this:  
cross compiling is a pain  
win32 api sucks  
programming on linux is kinda easy  
  
based on https://github.com/flok/pydualsense and DS4Windows  
