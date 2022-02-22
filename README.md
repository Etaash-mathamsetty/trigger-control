# trigger control  
a quick and dirty project that allows you to control the adaptive triggers of the dualsense controller on linux using a gui  
currently only works through usb though the controller is detected through bluetooth  
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

TODO: CRC for bluetooth

based on https://github.com/flok/pydualsense  
