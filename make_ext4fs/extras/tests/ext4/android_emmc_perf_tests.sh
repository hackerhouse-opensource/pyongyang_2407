#!/bin/bash

PERF="rand_emmc_perf"

if [ ! -r "$PERF" ]
then
  echo "Cannot read $PERF test binary"
fi

if ! adb shell true >/dev/null 2>&1
then
  echo "No device detected over adb"
fi

HARDWARE=`adb shell getprop ro.hardware | tr -d "\r"`

case "$HARDWARE" in
  tuna | steelhead)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/omap/omap_hsmmc.0/by-name/cache"
    MMCDEV="mmcblk0"
    ;;

  stingray | wingray)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/sdhci-tegra.3/by-name/cache"
    MMCDEV="mmcblk0"
    ;;

  herring)
    echo "This test will wipe the userdata partition on $HARDWARE devices."
    read -p "Do you want to proceed? " ANSWER

    if [ "$ANSWER" != "yes" ]
    then
      echo "aborting test"
      exit 1
    fi

    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/s3c-sdhci.0/by-name/userdata"
    MMCDEV="mmcblk0"
    ;;

  grouper)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/sdhci-tegra.3/by-name/CAC"
    MMCDEV="mmcblk0"
    ;;

  manta)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/dw_mmc.0/by-name/cache"
    MMCDEV="mmcblk0"
    ;;

  *)
    echo "Unknown hardware $HARDWARE.  Exiting."
    exit 1
esac

# prepare the device
adb root
adb wait-for-device
adb push "$PERF" /dev
adb shell stop
adb shell stop sdcard
adb shell stop ril-daemon
adb shell stop media
adb shell stop drm
adb shell stop keystore
adb shell stop tf_daemon
adb shell stop bluetoothd
adb shell stop hciattach
adb shell stop p2p_supplicant
adb shell stop wpa_supplicant
adb shell stop mobicore
adb shell umount /sdcard >/dev/null 2>&1
adb shell umount /mnt/shell/sdcard0 >/dev/null 2>&1
adb shell umount /data >/dev/null 2>&1
adb shell umount /cache >/dev/null 2>&1
# Add more services here that other devices need to stop.
# So far, this list is sufficient for:
#   Prime

# At this point, the device is quiescent, need to crank up the cpu speed,
# then run tests
adb shell "cat $CPUFREQ/cpuinfo_max_freq > $CPUFREQ/scaling_max_freq"
adb shell "cat $CPUFREQ/cpuinfo_max_freq > $CPUFREQ/scaling_min_freq"

# Start the tests

# Sequential read test
for I in 1 2 3
do
  adb shell "echo 3 > /proc/sys/vm/drop_caches"
  echo "Sequential read test $I"
  adb shell dd if="$CACHE" of=/dev/null bs=1048576 count=200
done

# Sequential write test
for I in 1 2 3
do
  echo "Sequential write test $I"
  adb shell dd if=/dev/zero of="$CACHE" bs=1048576 count=200
done

# Random read tests require that we read from a much larger range of offsets
# into the emmc chip than the write test.  If we only read though 100 Megabytes
# (and with a read-ahead of 128K), we quickly fill the buffer cache with 100
# Megabytes of data, and subsequent reads are nearly instantaneous.  Since
# reading is non-destructive, and we've never shipped a device with less than
# 8 Gbytes, for this test we read from the raw emmc device, and randomly seek
# in the first 6 Gbytes.  That is way more memory than any device we currently
# have and it should keep the cache from being poluted with entries from
# previous random reads.
#
# Also, test with the read-ahead set very low at 4K, and at the default

# Random read test, 4K read-ahead
ORIG_READAHEAD=`adb shell cat /sys/block/$MMCDEV/queue/read_ahead_kb | tr -d "\r"`
adb shell "echo 4 > /sys/block/$MMCDEV/queue/read_ahead_kb"
for I in 1 2 3
do
  adb shell "echo 3 > /proc/sys/vm/drop_caches"
  echo "Random read (4K read-ahead) test $I"
  adb shell /dev/"$PERF" -r 6000 "/dev/block/$MMCDEV"
done

# Random read test, default read-ahead
adb shell "echo $ORIG_READAHEAD > /sys/block/$MMCDEV/queue/read_ahead_kb"
for I in 1 2 3
do
  adb shell "echo 3 > /proc/sys/vm/drop_caches"
  echo "Random read (default read-ahead of ${ORIG_READAHEAD}K) test $I"
  adb shell /dev/"$PERF" -r 6000 "/dev/block/$MMCDEV"
done

# Random write test
for I in 1 2 3
do
  echo "Random write test $I"
  adb shell /dev/"$PERF" -w 100 "$CACHE"
done

# Random write test with O_SYNC
for I in 1 2 3
do
  echo "Random write with o_sync test $I"
  adb shell /dev/"$PERF" -w -o 100 "$CACHE"
done

# Make a new empty /cache filesystem
adb shell make_ext4fs "$CACHE"

