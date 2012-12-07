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

cp ../cm10/out/target/product/d2att/system/lib/hw/lights.msm8960.so ../packaging/Bootv2/system/lib/hw

cp arch/arm/boot/zImage ../packaging/staging

cp -R ../cm10/out/target/product/d2att/root/* ../packaging/staging/initramfs

cd ../packaging/staging

../mkbootfs initramfs | gzip > ramdisk.gz
../mkbootimg --kernel zImage --ramdisk ramdisk.gz --cmdline "androidboot.hardware=qcom user_debug=31" -o ../Bootv2/boot.img --base 0x80200000 --pagesize 2048 --ramdiskaddr 0x81500000

cd ../Bootv2

zip -r ~/Downloads/0Builds/InstigatorX-I747-Kernel-"$timestamp".zip *

echo $timestamp

cd /Volumes/Android/InstigatorX-I747-Kernel
