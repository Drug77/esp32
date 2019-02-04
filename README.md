# ws2812b

## Project setup
```
yarn install
```

### Compiles and hot-reloads for development
```
yarn run serve
```

### Compiles and minifies for production
```
yarn run build
```

### Lints and fixes files
```
yarn run lint
```

### Erase ESP8266 erase flash
```
esptool.py --port /dev/ttyUSB0 erase_flash
```

### Erase ESP8266 flash image
```
esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash --flash_size=detect 0 esp8266-20180511-v1.9.4.bin
esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash --flash_size=detect -fm dio 0 esp8266-20180511-v1.9.4.bin
```

### Useful commands
```
lsusb
export AMPY_PORT=/dev/ttyUSB0
ampy ls
picocom /dev/ttyUSB0 -b115200
upip.install("micropython-uasyncio")
help('modules')
git clone https://github.com/espressif/esp-idf-template.git app
ls /dev/tty*
make
make app
make menuconfig
make monitor
make flash
make app-flash
Relay pin is 22
git rm -r --cached build
make -j4 flash
make -j4 all
make -j4 flash monitor
make erase_flash
make erase_flash flash
serve -s dist
xtensa-esp32-elf-gdb ./build/ws2812b.elf -b 115200 -ex 'target remote /dev/ttyUSB0'
xtensa-esp32-elf-gdb ./build/ws2812b.elf -b 115200 -x gdbinit
```
