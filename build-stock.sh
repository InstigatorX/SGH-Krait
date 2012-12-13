set -e
timestamp=$(date '+%s')

export LOCALVERSION="$timestamp"

make -j8

cp arch/arm/mach-msm/dma_test.ko ../packaging/Bootv2/system/lib/modules
cp arch/arm/mach-msm/msm-buspm-dev.ko ../packaging/Bootv2/system/lib/modules
cp arch/arm/mach-msm/reset_modem.ko ../packaging/Bootv2/system/lib/modules
cp crypto/ansi_cprng.ko ../packaging/Bootv2/system/lib/modules
cp drivers/crypto/msm/qce40.ko ../packaging/Bootv2/system/lib/modules
cp drivers/crypto/msm/qcedev.ko ../packaging/Bootv2/system/lib/modules
cp drivers/crypto/msm/qcrypto.ko ../packaging/Bootv2/system/lib/modules
cp drivers/input/evbug.ko ../packaging/Bootv2/system/lib/modules
cp drivers/media/video/gspca/gspca_main.ko ../packaging/Bootv2/system/lib/modules
cp drivers/net/wireless/bcmdhd/dhd.ko ../packaging/Bootv2/system/lib/modules
cp drivers/net/wireless/btlock/btlock.ko ../packaging/Bootv2/system/lib/modules
cp drivers/scsi/scsi_wait_scan.ko ../packaging/Bootv2/system/lib/modules
cp drivers/spi/spidev.ko ../packaging/Bootv2/system/lib/modules

../packaging/mkbootfs ramdisk/initramfs | gzip > ramdisk/ramdisk.gz
../packaging/mkbootimg --kernel arch/arm/boot/zImage --ramdisk ramdisk/ramdisk.gz --cmdline "androidboot.hardware=qcom user_debug=31" -o build_dir/boot.img --base 0x80200000 --pagesize 2048 --ramdiskaddr 0x81500000

cd build_dir

zip -r ~/Google\ Drive/I747\ Kernels/InstigatorX-I747-Kernel-"$timestamp".zip *

echo $timestamp

cd ..

git tag -a $timestamp -m buildtag
