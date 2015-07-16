# PlayuavOSD
A Graphics OSD for FPV, Compatible with most FC

Introduction
============

PlayuavOSD is a open-source, open-hardware project. It is a graphics OSD for FPV. It is designed to display flying data from autopilot. It allows the user to customize what data to be displayed and the specific display position. More information can be found at our website.

Wiki:http://www.playuav.com/wiki/doku.php?id=projects:playuavosd:start

Developer
=====

You will notice the current firmware only supports MAVLink protocols. It will be great if supports more FC systems and protocols like UAVTalk, even the commercial FC like DJI can protocols. Besides, your suggestions are valuable to us. If you want to add a feature, or if you find any bug, please feel free to discuss on our website.

The firmware can be built with ARM GCC. It is highly recommended using PX4 Toolchain which can be download here:https://pixhawk.org/dev/toolchain_installation

Note:
The bin file can be flash to chip directly. The start address is 0x8004000. Address before 0x8004000 used for bootloader.
The hex flle generated from the playuavosd.bin file with "px_mkfw.py", can be used with the config tool.

Contact Us
==========

The home page of PlayuavOSD is http://www.playuav.com
