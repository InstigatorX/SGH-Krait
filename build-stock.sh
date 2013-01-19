set -e
timestamp=$(date '+%s')

export LOCALVERSION="$timestamp"

make -j8

cp arch/arm/mach-msm/dma_test.ko build_dir/system/lib/modules
cp arch/arm/mach-msm/msm-buspm-dev.ko build_dir/system/lib/modules
cp arch/arm/mach-msm/reset_modem.ko build_dir/system/lib/modules
cp crypto/ansi_cprng.ko build_dir/system/lib/modules
cp drivers/crypto/msm/qce40.ko build_dir/system/lib/modules
cp drivers/crypto/msm/qcedev.ko build_dir/system/lib/modules
cp drivers/crypto/msm/qcrypto.ko build_dir/system/lib/modules
cp drivers/input/evbug.ko build_dir/system/lib/modules
#cp drivers/interceptor/vpnclient.ko build_dir/system/lib/modules
cp drivers/media/video/gspca/gspca_main.ko build_dir/system/lib/modules
cp drivers/net/wireless/bcmdhd/dhd.ko build_dir/system/lib/modules
cp drivers/net/wireless/btlock/btlock.ko build_dir/system/lib/modules
cp drivers/scsi/scsi_wait_scan.ko build_dir/system/lib/modules
cp drivers/spi/spidev.ko build_dir/system/lib/modules

../packaging/mkbootfs ramdisk/initramfs | gzip > ramdisk/ramdisk.gz
../packaging/mkbootimg --kernel arch/arm/boot/zImage --ramdisk ramdisk/ramdisk.gz --cmdline "androidboot.hardware=qcom user_debug=31 no_console_suspend" -o build_dir/boot.img --base 0x80200000 --pagesize 2048 --ramdiskaddr 0x81500000

cd build_dir

zip -r ~/Google\ Drive/I747\ Kernels/InstigatorX-I747-Kernel-"$timestamp".zip *

echo $timestamp

cd ..

git tag -a $timestamp -m buildtag
