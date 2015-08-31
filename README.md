
dev branch used to develop new feature. Below is the list of developing feature:

1) Using VBI transfer telemetry data.

Acknowledgments
============
It is created by integrating a variety of excellent open-source projects. Many thanks to the developers for their efforts and dedication.

* MinimOSD - arducam-osd (https://code.google.com/p/arducam-osd/)

* Tau Labs - Brain FPV Flight Controller  ( https://github.com/BrainFPV/TauLabs)
 
* minoposd - an implementation of OpenPilot UAVTalk on MinimOSD(https://code.google.com/p/minoposd/)
 
* Books "Tricks of the 3D Game Programming Gurus-Advanced 3D Graphics and Rasterization"
  and "Tricks of the Windows Game Programming Gurus". Both written by Andre LaMothe(Author)

Introduction
============

PlayuavOSD is a open-source, open-hardware project. It is a graphics OSD for FPV. It is designed to display flying data from autopilot. It allows the user to customize what data to be displayed and the specific display position. More information can be found at our website.

Wiki:http://www.playuav.com/wiki/doku.php?id=projects:playuavosd:start

Developer
=====

You will notice the current firmware only supports MAVLink protocols. It will be great if supports more FC systems and protocols like UAVTalk, even the commercial FC like DJI can protocols. Besides, your suggestions are valuable to us. If you want to add a feature, or if you find any bug, please feel free to discuss on our website.

The firmware can be built with ARM GCC. It is highly recommended using PX4 Toolchain which can be download here:https://pixhawk.org/dev/toolchain_installation
After the toolchain installing, start the app "PX4 Console". Then change to the directory where the source code stored and enter the command:make

Note:
The playuavosd.bin can be flashed to chip directly. The start address is 0x8004000. Address before 0x8004000 used for bootloader.
The playuavosd.hex can be used with the OSD config tool. It is automatically generated during the make process.

Contact Us
==========

The home page of PlayuavOSD is http://en.playuav.com

