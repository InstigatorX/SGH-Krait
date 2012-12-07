timestamp=$(date '+%s')

export LOCALVERSION="$timestamp"

make -j8

cp arch/arm/mach-msm/dma_test.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp arch/arm/mach-msm/msm-buspm-dev.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp arch/arm/mach-msm/reset_modem.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp crypto/ansi_cprng.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/crypto/msm/qce40.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/crypto/msm/qcedev.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/crypto/msm/qcrypto.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/input/evbug.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/media/video/gspca/gspca_main.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/net/wireless/bcmdhd/dhd.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/net/wireless/btlock/btlock.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/scsi/scsi_wait_scan.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules
cp drivers/spi/spidev.ko ../packaging/Boot-JB-I747-Stock/realsystem/lib/modules

#cp ../cm10/out/target/product/d2att/realsystem/lib/hw/lights.msm8960.so ../packaging/Boot-JB-I747-Stock/realsystem/lib/hw

cp arch/arm/boot/zImage ../Boot-JB-I747-Stock

cd ../packaging/Boot-JB-I747-Stock

zip -r ~/Downloads/0Builds/InstigatorX-I747-Kernel-Stock-"$timestamp".zip *

echo $timestamp

cd /Volumes/Android/InstigatorX-I747-Kernel
