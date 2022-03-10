# trigger control  
![image](https://user-images.githubusercontent.com/45927311/157581763-3e169f96-7434-417c-9eb9-c23c7f4fb235.png)   

a quick and dirty project that allows you to control the adaptive triggers of the dualsense controller on linux using a gui  
~~currently only works through usb though the controller is detected through bluetooth~~  

now works through bluetooth and usb (thx ds4windows and dualsensectl for crc32.h)

libraries:  
libusb  
libSDL2  
libhidapi  
libgl  
glew  

installing them on arch linux:  
`sudo pacman -S sdl2 libusb hidapi glew libgl base-devel`  

**How to compile**  

`chmod +x compile.sh && ./compile.sh`  

**run**   
`./trigger-control`  

based on https://github.com/flok/pydualsense  
