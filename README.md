# Trigger Control  
Dark Mode           |  Light Mode
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/45927311/166091284-a06013df-f443-48a1-8e15-514690b43200.png) | ![image](https://user-images.githubusercontent.com/45927311/166091275-4f970e14-5a16-4f1e-b96b-4e823cdcfcae.png)


a quick and dirty project that allows you to control the adaptive triggers of the dualsense controller on linux and windows using a gui,  
works through bluetooth and usb.

requirments:  
c++17 compiler   

required libraries:  
libSDL2  
glib2    

optional:  
SDL2 >= 2.0.22 (for wayland)  
libdecor (for wayland)  
ttf-dejavu (better font)  

**installing them on arch linux (or any arch based distro):**  
`sudo pacman -S sdl2 base-devel glib2 libdecor ttf-dejavu`  

**How to compile**  

`make`  

**run**   
`./trigger-control`  

**install**  
install dependencies first
```
git clone https://github.com/Etaash-mathamsetty/trigger-control.git
cd trigger-control
chmod +x compile.sh
make
sudo cp trigger-control /usr/bin
```

**update**  
enter the directory in which you cloned this repo  
```
git pull
make
sudo cp trigger-control /usr/bin
```

**Cross Compile for Windows**  
I recommend arch linux for this, but you can try your luck with any other distro  
first, clone the repo, then run the following commands(with any AUR helper)  
```
yay -S mingw-w64-pkg-config mingw-w64-sdl2 
```
then run,  
```
./windows-compile.sh
```
you will get a .exe file as an output, but you won't be able to run it without copying the nessessary dll files, so copy  
`libwinpthread-1.dll`  
`SDL2.dll`   
`libstdc++-6.dll`  
`libgcc_s_seh-1.dll`  
from  
`/usr/x86_64-w64-mingw32/bin/`  
to the folder the git repo is in, and now you should be able to run the windows version of the program... hopefully  
  
things I learned from this:  
cross compiling is a pain  
win32 api sucks  
programming on linux is kinda easy  
  
based on https://github.com/flok/pydualsense  
