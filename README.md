### ⚠️ Notice: This repo has been deprecated in favor of https://github.com/Franticware/usb-to-ps1-mouse-pro
### Please note that firmwares between the two are *not* interchangeable.

# USB to PlayStation Mouse

![PlayStation with USB mouse connected](media/ps1-with-mouse.jpg)

To build the adapter, the following components are necessary:
* PlayStation controller cable with connector
* Raspberry Pi Pico
* Mini PFM Control DC-DC USB 0.9V-5V to 5V dc Boost Step up Power Module

![Components](media/components.jpg)

The cable has colored wires, but color coding is not guaranteed and has to be checked with a multimeter in continuity mode. Unused wires have to be isolated. A good way to do this is putting a thin heat shrink tube over a folded wire with isolation intact.

The components are to be wired as follows:
![Wiring diagram](media/wiring.svg)

Make sure the two PCBs are well isolated from each other.

![Adapter animation](media/usb-to-ps1-mouse-anim.gif)

Uploading the firmware is done in the usual manner, i.e. the Pi is connected to a PC via a micro USB cable while BOOTSEL button is pressed. The Pi appears as USB flash drive and **usb_to_ps1_mouse.uf2** is copied to it (https://github.com/Franticware/usb-to-playstation-mouse/releases/latest). USB mouse must not be connected to the adapter while programming (obviously).

## Notes
* This is an early version of the adapter, so issues may appear. It is recommended to watch this project so that you catch all upcoming updates and fixes.
* When connecting the adapter to a PS1 that is already on, USB mouse must be connected to the adapter first and 
then the adapter should be connected to PS1.
* To test the adapter, no special game is needed, because the mouse works in the PlayStation system menu that appears when there is no game in the CD drive (except some early Japanese models). Also, it can be tested with an ordinary audio CD (see [YouTube video](https://youtu.be/NdH5DYy-4_E)). For these tests, PS1 controller should be disconnected.
* Known issues:
  * Not every USB mouse is compatible with the adapter.

## Links

https://youtu.be/NdH5DYy-4_E

https://hackaday.com/2022/07/12/pi-pico-provides-practical-playstation-pointing/

https://www.high-voltage.cz/sortky/potrebujete-playstation-mouse-zbastlete-si-ji/

## Support this project

To express your support of this project, you can use your favorite service to listen to a [music track I produced](https://share.amuse.io/track/vojtech-salajka-color-graphics-pinball-original-game-soundtrack). It is not much, but it is the only digital product I am offering commercially as of writing this. Thank you!

## Show off

Share your build here: https://github.com/Franticware/usb-to-playstation-mouse/discussions/1

## Special thanks to

[@deefha](https://github.com/deefha) for lending me his original SCPH-1090 PlayStation Mouse
