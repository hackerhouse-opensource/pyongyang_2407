# 평양 2407 Android ROM

평양 2407 is an aftermarket Android ROM used in North Korea compatible with Chinese
hardware. 평양 2407 or Pyongyang 2407 can be booted on similar Chinese hardware
available in countries such as India, China, Egypt & USA. 평양 2407 provides limited
outside connectivity and only a handful of applications, it is a modified Android
operating system that conducts surveillance and tracking on the user.  

This repository contains instructions on how to boot and load Pyongyang 2407 ROM from
"Pyongyang 2407 Cellphone disk image" onto a device. All needed tools except "spflashtool"
are included in the "release" repository.

You need a device with a WBW5511_MAINBOARD_P2 device for this ROM to work. These are
MTK6258 based cellphone devices - sold under a variety of different brand names.

* Walton Pro H3
* Gionee CTRL V5
* BLU Life Play 2

Originally mass-marketted to India, Egypt etc. from China - spread to USA as BLU export.
You may find other devices matching specs above with the same board internals. Specification,
internals will be a match, outer casing can be re-branded or coloured.

## Host Setup
Note you will need a native host as the boot process exploited with MTK tools requires a
native USB interface only accessible for a few seconds. Do not use a virtual machine at your
own peril. From Linux we are going to use MTK download agent to re-flash a modified ROM with
the KCC overlay files. When using a Linux system prevent ModemManager interfering with the
preloader ttyACM0 device you must stop all processess accessing it. e.g.

"sudo systemctl stop ModemManager"

You should then run flash tool (spflashtool) - SP Flash Tool is an application to flash
your MediaTek (MTK) SmartPhone.  You can find binaries for it here https://spflashtool.com/

## First Boot ROM
First flash the included WBW5511GI_0202_T5752 over the WBW5511_MAINBOARD_P2 based device,
this will provide you with a Android JellyBean 4.2.2 ROM. It doesnt matter which vendor
providing it is WBW5511_MAINBOARD_P2, easy to tell with the battery and cover removed.

WARNING THIS WILL COMPLETELY OVERWRITE YOUR FLASH CHIP USING EARLY PRELOADER ON DEVICE
AND IT IS ENTIRELY POSSIBLE TO BRICK INCOMPATIBLE HARDWARE.

1) load included jellybean (WBW5511GI_0202_T5752) based rom onto device, using SPFlashTool
2) Boot device, it will take a few minutes
3) root device - kingroot APK (works)
4) put "data.ext4" and "system.tar" onto /sdcard via "adb push"
5) upload "gnutar" and "busybox-android" from this repo
6) "adb shell" and check "su" works

## Install ROM 평양 files
I made some modifications to the KCC files to work with the supplied ROM. For instance I
have replaced missing kernel modules in "/usr/lib/modules" and created a compatible layout
file with a new kernel image and clean boot loaders. you still need to manually overwrite
"/system" and "/data" from a root shell. You must first remount both into rw mode from
"adb shell" as root.

"mount -o rw,remount /system"
"mount -o rw,remount /data"

You then want to "rm -rf /system/*" and "/data/*" but you cannot run "busybox-android" from
"/sdcard" so you must ensure that it survives your "rm -rf" brutality. You are going to wipe
important system files and replace them with the contents of the two tar files.

1) cp "busybox-android" and "gnutar" into a executable safe place such as /data.
2) wipe /system after remount rw with "rm -rf /system", run twice, some files will stick.
3) use supplied "gnutar" and PRESERVE PERMISSIONS with "gnutar -xvpf" and unpack system.tar /system
4) once you have /system unpacked, put busybox-android and gnutar in /system/xbin
5) now do the same "rm -rf /data/*" and "gnutar -xvpf" to /data with data.ext4
6) reboot device.

You will be greeted with the welcoming music of 평양 2407 Android ROM. This will contain only
the core user-space modifications and a clean Android kernel with some hacker friendly recovery
mode. If you want a totally "stock" "brand-new" out the box device reboot into recovery, wipe
all data to set back to factory and reboot. The phone will take a little longer but it will
default back to "korean" language settings and completely wipe any after-market DPRK apps and
keep only the Android user space modifications.

Happy hacking! Enjoy your 평양 2407 phone!

## DPRK Surveillance
An accompanying presentation on device internals and DPRK Android hacking released at DC562 January
2019. DPRK & Android surveillance components including "libLegalInterface.so" are explored in more
depth during presentation. The surveillance technology matches what was already seen in DPRK Red
Star desktop OS, using the same ciphers to water mark and DRM / track documents. An event logger
APK is included which tracks all the user operations on the device. When factory reseting & wiping
a device, the logger will crash as unique device identifiers are missing. It is not possible to use
the flashing / recovery img to wipe data on a Pyongyang device unless modified. We enabled some
features to assist hacking and exploration of the ROM contents.

# Acknowledgements
Hacker Fantastic would like to thank the following people for taking part in the annual winter
eggnog DPRK festivities.

Thanks to Will & team behind [KCC](http://www.koreacomputercenter.org)
Hacker House for supporting open-source security research - [Hacker House](https://hacker.house)
MediaTek for building backdoors - [MTK flasher](https://spflashtool.com)
redstar-tools team for watermarking research - [redstar-tools](https://github.com/takeshixx/redstar-tools)
DefCon562 for hacking fiends - [dc562](https://dc562.org/)
slim shady ~ slipstream 
