ESP8266-transparent-bridge
==========================

Absolutely transparent bridge for the ESP8266 with support to reset Arduino and ARM processors.

This is really basic firmware for the ESP that creates a totally transparent TCP socket to
ESP UART0 bridge. Characters come in on one interface and go out the other. The totally
transparent bridge mode is something that has been missing and is available on higher priced boards.
In addition, the bridge can recognize the synchronization sequence of avrdude (used to load
sketches into Arduinos) and of NXP LPC8XX ARM processors and issue a reset to those processors
on the GPIO0 and GPIO2 pins in order to put them into upload/flash mode.

Parts of this firmware are from the stock AT firmware and the esphttpd project. Enjoy!

Important
---------
Current state:
- This firmware has been tested with a JeeNode (mini arduino with RFM12B radio)
- The arduino firmware upload is excrutiatingly slow
- The CONFIG_STATIC mode does not seem to work

Features
--------

* It works. Do you already have a serial project to which you just want to add WiFi? This is your
  ticket. No more dealing with the AT command set.
* You can program your Arduino over WiFi. Just hit the reset button and upload your sketch using
  avrdude. Or, connect GPIO0 or GPIO2 to your Arduino's reset pin and it's fully automatic.
  The upload happens over Wifi by telling avrdude to use a network port instead of a serial
  device using `avrdude -c avrisp -p m328p -P net:192.168.4.1:23 -F -U flash:w:mySketch.hex:i`
* You can statically configure the ESP when you build the firmware. Each time the unit boots
  it uses values defined in `config_wifi.h`.  For this create `./config_wifi.h` with content
  patterned after the following:
```
  #define CONFIG_STATIC  // use #undef if you do not want a static config
  #define CONFIG_DYNAMIC // use #undef to disable +++AT commands

  #define STA_SSID      "your-ssid"
  #define STA_PASSWORD  "password"
  #define AP_SSID       "esp8266"
  #define AP_PASSWORD   "tr@nsp@r#nt"
```
* You can also dynamically configure by remote telnet using +++AT prefixed commands. This is
  enabled by default.  To disable, comment the following line in `user/config.h` or add a
  `#undef` into `config_wifi.h`:
```
  #define CONFIG_DYNAMIC
```

### Configuration commands:

Telnet into the module and issue commands prefixed by +++AT to escape each command from bridge mode.
The dynamic configuration commands are:
```
+++AT                         # do nothing, print OK
+++AT MODE                    # print current opmode 
+++AT MODE <mode>             # set opmode 1=STA, 2=AP, 3=both
+++AT STA                     # print current ssid and password connected to
+++AT STA <ssid> <password>   # set ssid and password to connect to
+++AT AP                      # print the current soft ap settings
+++AT AP <ssid>               # set the AP as open with specified ssid
+++AT AP <ssid> <password> [<authmode> [<ch>]]] # set the AP ssid and password,
                              # authmode: 1=WEP, 2=WPA, 3=WPA2, 4=WPA+WPA2, channel: 1..13
+++AT BAUD                    # print current UART baud rate
+++AT BAUD <baud> [<authmode> [<ch>]]] # set currrent UART baud rate and optional
                              # data bits = 5/6/7/8, parity = N/E/O, stop bits = 1/1.5/2
+++AT PORT                    # print current incoming TCP socket port
+++AT PORT <port>             # set current incoming TCP socket port (restarts ESP)
+++AT FLASH                   # print current flash settings
+++AT FLASH <1|0>             # 1: save changed UART settings (++AT BAUD ...) (default),
                              # 0: do not save UART settings to flash
+++AT RESET                   # software reset the unit
```
Upon success, all commands send back "OK" as their final output.  Note that passwords may not
contain spaces.  For the softAP, the mode is fixed to AUTH_WPA_PSK.

By default the settings are saved after the commands `+++AT PORT <port>` and `+++AT BAUD <baud> ...`.

If `+++AT FLASH 0` is issued the parameters of `+++AT BAUD <baud> ...` are NOT saved to flash memory.
The new settings are applied to the UART and saved only in RAM.
But a `+++AT PORT <port>` always needs to flash the settings because it performs a reboot.
In that case the changed UART settings are also saved to flash.

The purpose of disabling the flahing of the settings is for devices where temporary baud rate changes
are required. For example, some electric meters start conversions at 300 baud and accept a command
to change to 9600.

Example session:
```
user@host:~$ telnet 192.168.1.197
Trying 192.168.1.197...
Connected to 192.168.1.197.
Escape character is '^]'.
+++AT MODE
MODE=3
OK
+++AT AP 
SSID=ESP_9E2EA6 PASSWORD= AUTHMODE=0 CHANNEL=3
OK
+++AT AP newSSID password
OK
+++AT AP
SSID=newSSID PASSWORD=password AUTHMODE=2 CHANNEL=3
OK
+++AT AP ESP_9E2EA6
OK
+++AT AP
SSID=ESP_9E2EA6 PASSWORD= AUTHMODE=0 CHANNEL=3
OK
^]c

telnet> c
Connection closed.
```
In order, this gets the current opmode. Good, it is 3 for STA + AP. Next, the current AP settings
are retrieved. Next, the AP ssid is changed to newSSID and the authmode set to WPA and a
password set. The AP settings are retrieved again to verify. Finally, the AP SSID is changed
back to the original and by not using a password, the authmode is set to OPEN.

### Arduino & ARM upload

The arduino upload assumes that avrdude opens a new connection to the ESP and sends '0 ' (zero, 
space) as the first characters. The ESP recognizes this sequence and pulses GPIO0 and GPIO2 low
for a millisecond to reset the arduino.

The ARM LPC8XX upload also assumes that the upload happens on a new connection and the ESP
recognizes '?\n' (question mark, newline) as the first characters. It then pulses GPIO0
and GPIO2 low, raises GPIO2 after a millisecond, and then GPIO0 a millisecond later. If
GPIO2 is connected to the LPC's reset pin and GPIO0 to the LPC's ISP pin this will reset the
chip and put it into firmware upload mode.

Issues
------

* There is limited buffering in the path from UART input to TCP output. The UART FIFO buffers
a few characters. After that, a memory buffer collects new uart chars until the previous packet
is sent. The SDK also buffers the "current" packet for a bit. Note that starting with
SDK 0.9.4 an espconn_send must not be made until after the espconn_sent_callback of the previous
packet. As a result of all this, the first few incoming UART characters in the FIFO get sent
immediately via the tx-buffer, and then often the next bunch of characters are buffered up.
As a result, many TCP packets only have a small number of bytes. 
* The static configuration does not work
* The flashing of arduinos is way, way too slow

Building the firmware on Linux
------------------------------

These instructions use the esp-open-sdk and the `Makefile_open`. Presumably the firmware
can also be built with Espressif's toolchain, YMMV. You will need the following installed:
- The ESP toolchain, an installation following
  https://github.com/esp8266/esp8266-wiki/wiki/Toolchain on Ubuntu 14.04 is a good option.
- The esptool image tool, consider using https://github.com/tommie/esptool-ck
- The esptool.py flash tool, https://github.com/themadinventor/esptool

Then build the firmware as follows:
- Edit the first few definitions in the Makefile to point to the tools you just installed.
- Edit `config_wifi.h` file with your desired wifi settings
- Run `make -f Makefile_open` and you should end up with:
```
  $ ls -ls firmware/
  total 180
   32 -rw-rw-r-- 1 tve tve  32608 Feb 14 16:45 0x00000.bin
   48 -rw-rw-r-- 1 tve tve 150748 Feb 14 16:45 0x40000.bin
```

You can now flash your ESP:
- Connect GPIO0 to ground and apply reset (briefly connect RST to ground)
- Run the flash upload command, something like:
```
  /opt/Espressif/esptool-py/esptool.py --port /dev/tty.USB0 write_flash \
       0x00000 eagle.app.v6.flash.bin 0x40000 eagle.app.v6.irom0text.bin
```

You can now test your firmware:
- Pull out your phone or tablet and run a wifi scanner (such as Wifi Analyzer on Android)
  and you should see you ESP's access point (named ESP8266 by default)
- Assuming you've configured your home wifi network into the ESP, you should be able to
  telnet to it. You can look at your DHCP server (typically in your wifi router) to see
  which IP address it got.

Visual Studio 2013 Integration
------------------------------

See "New Windows terminal/flasher apps & Visual Studio"
[www.esp8266.com](http://www.esp8266.com/viewtopic.php?f=9&t=911#p5113) to setup Visual Studio 2013.
Please install in a folder i.e. `c:\Projects\Espressif\`
- `ESP8266-transparent-bridge/`   # this project
- `esp_iot_sdk_v0.9.5/` # [bbs.espressif.com](http://bbs.espressif.com/download/file.php?id=189)
- `xtensa-lx106-elf/` # pre-built compiler, see
  [esp8266.com](http://www.esp8266.com/viewtopic.php?f=9&t=911#p5113),
  I used `xtensa-lx106-elf-141114.7z` from
  [drive.google.com](https://drive.google.com/uc?export=download&confirm=XHSI&id=0BzWyTGWIwcYQallNcTlxek1qNTQ)
- `esptool-py.py` # [www.esp8266.com](http://www.esp8266.com/download/file.php?id=321 )

The files used by Visual Studio are:
```
ESP8266-transparent-bridge.sln      #solution file
ESP8266-transparent-bridge.vcxproj  #project file, with IncludePath to xtensa-lx106-elf,
                                    #sdk for intellisense and "Go To Definition" F12
espmake.cmd                         #batch file called by build, rebuild, clean command,
                                    #which sets the path and calls make with Makefile_VS2013
Makefile_VS2013                     #the makefile called by the Visual Studio NMake project
                                    #via espmake.cmd
```
The Debug config is used for compile, Release for compile & flash with esptool-py.py  

The following absolute path names and COM Port number are expected:
```
C:\MinGW\bin;C:\MinGW\msys\1.0\bin       in espmake.cmd
C:\Python27\python                       in Makefile_VS2013 for flash
COM5                                     in Makefile_VS2013 for flash
```

To flash use ESP8266Flasher.exe from https://github.com/nodemcu/nodemcu-flasher with
```
eagle.app.v6.flash.bin at 0x00000
eagle.app.v6.irom0text.bin at 0x40000
```
