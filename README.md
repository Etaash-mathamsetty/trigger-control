# trigger control  
![image](https://user-images.githubusercontent.com/45927311/157797607-a7ce09dd-6e2c-4071-b88f-eefcda03d2f7.png)

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
`sudo pacman -S sdl2 libusb hidapi glew libgl base-devel glib2`  

**How to compile**  

`chmod +x compile.sh && ./compile.sh`  

**run**   
`./trigger-control`  

**install**  
install dependencies first
```
git clone https://github.com/Etaash-mathamsetty/trigger-control.git
cd trigger-control
chmod +x compile.sh
./compile.sh
sudo cp trigger-control /usr/bin
```

**update**
```
cd trigger-control
git pull
./compile.sh
sudo cp trigger-control /usr/bin
```

based on https://github.com/flok/pydualsense  
