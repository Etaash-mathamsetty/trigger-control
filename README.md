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

**installing them on arch linux:**  
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

based on https://github.com/flok/pydualsense  
