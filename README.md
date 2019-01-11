# 평양 (Pyongyang) 2407 Android ROM

![Pyongyang](https://github.com/hackerhouse-opensource/pyongyang_2407/raw/master/screenshot.png)

평양 2407 is an aftermarket Android ROM used in North Korea compatible with Chinese
hardware. 평양 2407 or Pyongyang 2407 can be booted on similar Chinese hardware
available in countries such as India, China, Egypt & USA. 평양 2407 provides limited
outside connectivity and only a handful of applications, it is a modified Android
operating system that conducts surveillance and tracking on the user.  

This repository contains instructions on how to boot and load Pyongyang 2407 ROM from
"Pyongyang 2407 Cellphone disk image" onto a device. All needed tools are included in
[pyongyang_2407.tgz](https://github.com/hackerhouse-opensource/pyongyang_2407/releases/download/1.3/pyongyang_2407.tgz) and future updates to safe reverse engineered efforts can be downloaded from [releases](https://github.com/hackerhouse-opensource/pyongyang_2407/releases/tag/1.3)

You need a WBW5511_MAINBOARD_P2 hardware device for this ROM to work. These are
MTK6582 based cellphone devices - sold under a variety of different brand names. If
you want to see details of the board, check out the FCCID link.

* Walton Pro H3
* Gionee CTRL V5
* BLU Life Play 2 (YHLBLULIFEPLAY2) [FCCID](https://fccid.io/YHLBLULIFEPLAY2)

Originally mass-marketted to India, Egypt etc. from China - spread to USA as BLU export.
You may find other devices matching specs above with the same board internals. Specification,
internals will be a match, outer casing can be re-branded or coloured.


## Host Setup
Note you will need a native host as the boot process exploited with MTK tools requires a
native USB interface only accessible for a few seconds. Do not use a virtual machine at your
own peril. From Linux we are going to use MTK download agent to re-flash a modified ROM with
the KCC overlay files. When using a Linux system prevent ModemManager interfering with the
preloader ttyACM0 device you must stop all processess accessing it. e.g.

_"sudo systemctl stop ModemManager"_

You should then run flash tool (spflashtool) - SP Flash Tool is an application to flash
your MediaTek (MTK) SmartPhone.  You can find binaries for it here https://spflashtool.com/

## First Boot ROM
First flash the included WBW5511GI_0202_T5752 over the WBW5511_MAINBOARD_P2 based device,
this will provide you with a Android JellyBean 4.2.2 ROM. It doesnt matter which vendor
providing it is WBW5511_MAINBOARD_P2, easy to tell with the battery and cover removed.

![WBW511](https://raw.githubusercontent.com/hackerhouse-opensource/pyongyang_2407/master/WBW5511.png)

**WARNING THIS WILL COMPLETELY OVERWRITE YOUR FLASH CHIP USING EARLY PRELOADER ON DEVICE
AND IT IS ENTIRELY POSSIBLE TO BRICK INCOMPATIBLE HARDWARE.**

Check you can see the preloader of your device, this shows up very briefly on powering
device without battery. It contains a simple download & boot from RAM backdoor that lets 
you run any .bin code to read/write/format the EMMC / NAND and SDMMC (which MT6582 boots 
from). It is provided by the "preloader" function.

```
[136466.320357] usb 1-5: new high-speed USB device number 48 using xhci_hcd
[136466.462845] usb 1-5: New USB device found, idVendor=0e8d, idProduct=2000, bcdDevice= 1.00
[136466.462853] usb 1-5: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[136466.462858] usb 1-5: Product: MT65xx Preloader
[136466.462862] usb 1-5: Manufacturer: MediaTek
[136466.526864] cdc_acm 1-5:1.1: ttyACM0: USB ACM device
[136468.181907] usb 1-5: USB disconnect, device number 48 
```

If you can see something similar to above in dmesg output and no ModemManager is running
you can proceed.

1) load included jellybean (WBW5511GI_0202_T5752) based rom onto device, using SPFlashTool
2) Boot device, it will take a few minutes
3) root device - kingroot APK (works)
4) put *data.ext4* and *system.tar* onto /sdcard via _"adb push"_
5) upload _"gnutar"_ and _"busybox-android"_ from this repo
6) _"adb shell"_ and check _"su"_ works

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

## DPRK Surveillance (전자서명조작체계) RedFlag

An accompanying presentation on device internals and DPRK Android hacking released at DC562 in January
2019. DPRK & Android surveillance components "Digital signature modification system" are explored in 
more depth during presentation. The surveillance technology matches similar to that already seen in 
DPRK Red Star desktop OS, using Android native code to water mark and DRM / track documents. An event 
logger APK is included which tracks all the user operations on the device. When a user accesses illegal
or non-state approved media, an alert is generated and stored inside the phone - viewable to the user.

## DPRK jailbreak (전자서명조작체계) bypass

experimental exploit patch can be applied to the phone to allow loading of any media & bypass state
censorship tool 전자서명조작체계. 

``` 
$ adb push libmedianatsign.so /sdcard
libmedianatsign.so: 1 file pushed. 1.0 MB/s (26244 bytes in 0.025s)
```

You need to use "su" and from root just replace libmedianatsign.so

```
shell@평양:/ # mount -o rw,remount /system
shell@평양:/ # cp /sdcard/libmedianatsign.so /system/lib       
```

Any media file played on the device will now be treated as a "Nat_Sign_File" and not be
recorded in the events tracer. 
 
``` 
D/gov_sign( 1724): MnsNative isNatSignFile : file name = /storage/sdcard0/Video/The.Interview.2014.720p.BluRay.x264.YIFY.mp4, result = 1
D/RSG     ( 1724): This file is Nat_Sign_File.
```

This is a good example of why governments should not have "Golden Keys" or encryption backdoors, 
the DPRK Android ROM can be abused through its trust model of state access. A user can abuse the 
functionalities in this case to escape DPRK security enforcements, evade logging and watch illegal
content.

## 전자서명조작체계 gov_sign exploit (bypass signed media checks) libmedianatsign.so

This is the high level pseudo-code called when isNatSignFile is passed inside Android OS,
this happens whenever a file is being checked for a valid certificate on opening, accessing,
reading directories etc. the result this function determines which checks are to be called
and also performs the verification of certificate to identify NATSIGN and SELFSIGN keyword.

```
int isNatSignFile(int arg0, int arg1, int arg2) {
    r3 = *(*arg0 + (0xa9 << 0x2));
    r0 = (r3)(arg0, arg2 + 0x0, 0x0, r3);
    r4 = verifyFileSign(r0, arg2 + 0x0, 0x0, r3, var_18, stack[-20], stack[-16]) + 0x0;
    __android_log_print();
    r0 = r4 + 0x0;
    return r0;
}
```

The original ARMv7 ASM for this function is below.

```
        ; ================ B E G I N N I N G   O F   P R O C E D U R E ================

        ; Variables:
        ;    var_18: int32_t, -24


             isNatSignFile:
00001978         push       {r4, r5, lr}
0000197a         ldr        r1, [r0]
0000197c         movs       r3, #0xa9
0000197e         lsls       r3, r3, #0x2
00001980         sub        sp, #0xc
00001982         ldr        r3, [r1, r3]
00001984         adds       r1, r2, #0x0
00001986         movs       r2, #0x0
00001988         blx        r3
0000198a         adds       r5, r0, #0x0
0000198c         bl         verifyFileSign                                      ; verifyFileSign
00001990         ldr        r1, =0x3e52                                         ; dword_19ac,0x3e52
00001992         ldr        r2, =0x3e9c                                         ; dword_19b0,0x3e9c
00001994         adds       r4, r0, #0x0
00001996         str        r0, [sp, #0x18 + var_18]
00001998         adds       r3, r5, #0x0
0000199a         add        r1, pc                                              ; "gov_sign"
0000199c         add        r2, pc                                              ; "MnsNative isNatSignFile : file name = %s, result = %d"
0000199e         movs       r0, #0x3
000019a0         blx        __android_log_print@PLT                             ; __android_log_print
000019a4         add        sp, #0xc
000019a6         adds       r0, r4, #0x0
000019a8         pop        {r4, r5, pc}
                        ; endp
```

It is possible to hijack this function and subvert the logic, by rewriting this function in ASM it
will treat all opened files as "NATSIGN" files with successful state authorisation for use, no logging
will be performed or any further checks on the file. By patching this function we can now use the phone
to open any media files and documents without "this file is not legal." errors and will not record use
of the file or offending applications using the file in 전자서명조작체계.

```
        ; ================ B E G I N N I N G   O F   P R O C E D U R E ================

        ; Variables:
        ;    var_18: int32_t, -24


             isNatSignFile:
00001978         push       {r4, r5, lr}
0000197a         ldr        r1, [r0]
0000197c         movs       r3, #0xa9
0000197e         lsls       r3, r3, #0x2
00001980         sub        sp, #0xc
00001982         ldr        r3, [r1, r3]
00001984         adds       r1, r2, #0x0
00001986         movs       r2, #0x0
00001988         blx        r3
0000198a         adds       r5, r0, #0x0
0000198c         movs       r0, #0x1   ; my milkshake brings a 0x1 into r0.
0000198e         nop                   ; nop out the actual verify checks entirely
00001990         ldr        r1, =0x3e52                                         ; dword_19ac,0x3e52
00001992         ldr        r2, =0x3e9c                                         ; dword_19b0,0x3e9c
00001994         movs       r4, #0x1
00001996         str        r0, [sp, #0x18 + var_18] ; store false "1" result, gov_sign result = 1
00001998         adds       r3, r5, #0x0
0000199a         add        r1, pc                                              ; "gov_sign"
0000199c         add        r2, pc                                              ; "MnsNative isNatSignFile : file name = %s, result = %d"
0000199e         movs       r0, #0x3
000019a0         blx        __android_log_print@PLT                             ; __android_log_print
000019a4         add        sp, #0xc
000019a6         movs       r0, #0x1  ; make function return 1
000019a8         pop        {r4, r5, pc}
                        ; endp
```

This patch when applied to the OS causes all isNatSignFile checks to return "1", and does not
perform any or additional verification checks on the file. This causes the OS to treat all opened 
files as NATISIGN files.

# Acknowledgements
Hacker Fantastic would like to thank the following people for taking part in the annual winter
eggnog DPRK & North Korean hacking festivities.

* Thanks to Will & team behind [KCC](http://www.koreacomputercenter.org)
* Hacker House for supporting open-source security research - [Hacker House](https://hacker.house)
* MediaTek for building backdoors - [MTK flasher](https://spflashtool.com)
* redstar-tools team for watermarking research - [redstar-tools](https://github.com/takeshixx/redstar-tools)
* DefCon562 for hacking fiends - [dc562](https://dc562.org/)
* slipstream - RSS

![DPRK ROM](https://raw.githubusercontent.com/hackerhouse-opensource/pyongyang_2407/master/booted.png)

*Booted 평양 2407 phone*
