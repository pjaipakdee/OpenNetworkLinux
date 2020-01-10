#!/bin/sh
#
######################################################################
#
# sample-postinstall.sh
#
# Example script for post-install hooks.
#
# Add this as a postinstall hook to your installer via
# the 'mkinstaller.py' command line:
#
# $ mkinstaller.py ... --postinstall-script sample-postinstall.sh ...
#
# At install time, this script will
#
# 1. be extracted into the working directory with the other installer
#    collateral
# 2. have the execute bit set
# 3. run in-place with the installer chroot directory passed
#    as the first command line parameter
#
# If the script fails (returns a non-zero exit code) then
# the install is aborted.
#
# This script is executed using the ONIE runtime (outside the chroot),
# after the actual installer (chrooted Python script) has finished.
#
# This script is run after the postinstall actions (e.g. proxy GRUB
# commands)
#
# At the time the script is run, the installer environment (chroot)
# is fully prepared, including filesystem mount-points.
# That is, the chroot mount points have not been unmounted yet.
#
######################################################################

rootdir=$1; shift

echo "Hello from postinstall"
echo "Chroot is $rootdir"

# Enable NOS mode using onie-nos-mode command to set the mode to not change the boot order.
# https://github.com/opencomputeproject/onie/pull/706#issuecomment-372457702
echo "Unmount onie-boot from Chroot"
umount $rootdir/mnt/onie-boot

echo "Mount ONIE-BOOT on /mnt/onie-boot for support onie-nos-mode to execute"
mkdir /mnt/onie-boot
mount -v /dev/sda$(sgdisk -p /dev/sda | grep "ONIE-BOOT" | awk '{print $1}') /mnt/onie-boot

echo "Enable onie NOS installed mode"
onie-nos-mode -s

echo "Umount /mnt/onie-boot"
umount /mnt/onie-boot

echo "Mount onie-boot back to Chroot at $rootdir/mnt/onie-boot"
mount -v /dev/sda$(sgdisk -p /dev/sda | grep "ONIE-BOOT" | awk '{print $1}') $rootdir/mnt/onie-boot

exit 0
