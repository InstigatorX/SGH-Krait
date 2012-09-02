timestamp=$(date '+%s')
export LOCALVERSION="$timestamp"
make -j8
rm -rf /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
mkdir /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp arch/arm/mach-msm/dma_test.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp arch/arm/mach-msm/msm-buspm-dev.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp arch/arm/mach-msm/reset_modem.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp crypto/ansi_cprng.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/crypto/msm/qce40.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/crypto/msm/qcedev.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/crypto/msm/qcrypto.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/input/evbug.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/media/video/gspca/gspca_main.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/net/wireless/bcmdhd/dhd.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/net/wireless/btlock/btlock.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/scsi/scsi_wait_scan.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp drivers/spi/spidev.ko /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/modules
cp arch/arm/boot/zImage /Volumes/Kernel/packaging/Boot-I747
cp lights.msm8960.so /Volumes/Kernel/packaging/Boot-I747/realsystem/lib/hw
cd /Volumes/Kernel/packaging/Boot-I747
zip -r ~/Downloads/0Builds/InstigatorX-I747-Kernel-"$timestamp".zip *
echo $timestamp
cd /Volumes/Kernel/InstigatorX-I747-Kernel
