# trigger control  
![image](https://user-images.githubusercontent.com/45927311/161355061-a773a3e1-c9b3-483b-ab5a-9eef4026885d.png)


a quick and dirty project that allows you to control the adaptive triggers of the dualsense controller on linux using a gui  
~~currently only works through usb though the controller is detected through bluetooth~~  

now works through bluetooth and usb (thx ds4windows and dualsensectl for crc32.h)

libraries:  
libSDL2  
libhidapi  
libgl  
glew  
glib2  

**installing them on arch linux (or any arch based distro):**  
`sudo pacman -S sdl2 hidapi glew libgl base-devel glib2`  

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
I recommend arch linux for this, but try your luck with any other distro  
first, clone the repo, then run the following commands(with any AUR helper)  
```
yay -S mingw-w64-glew
yay -S mingw-w64-sdl2
yay -S mingw-w64-hidapi
```
remove glu dependencies from glew.pc  
```
sudo nano /usr/i686-w64-mingw32/lib/pkgconfig/glew.pc
sudo nano /usr/x86_64-w64-mingw32/lib/pkgconfig/glew.pc
```
then run,  
```
./windows-compile.sh
```
you will get a .exe file as an output, but you won't be able to run it without copying the nessessary dll files, so copy  
libwinpthread-1.dll  
libhidapi-0.dll  
SDL2.dll  
glew32.dll  
libstdc++-6.dll  
libgcc_s_seh-1.dll  
from  
`/usr/x86_64-w64-mingw32/bin/`  
to the folder the git repo is in, and now you should be able to run the windows version of the program... hopefully  
ngl, shoulda used python lol  
based on https://github.com/flok/pydualsense  
