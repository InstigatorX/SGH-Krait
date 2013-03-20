set -e

timestamp=$(date '+%s')

export LOCALVERSION="$timestamp"

make -j8

rm build_dir/Boot-CM10.1/realsystem/lib/modules/* || true

cp arch/arm/mach-msm/dma_test.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp arch/arm/mach-msm/msm-buspm-dev.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp arch/arm/mach-msm/reset_modem.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp crypto/ansi_cprng.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/crypto/msm/qce40.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/crypto/msm/qcedev.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/crypto/msm/qcrypto.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/input/evbug.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/media/video/gspca/gspca_main.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/net/wireless/bcmdhd/dhd.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/net/wireless/btlock/btlock.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/scsi/scsi_wait_scan.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/spi/spidev.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/char/adsprpc.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp block/test-iosched.ko build_dir/Boot-CM10.1/realsystem/lib/modules
cp drivers/hid/hid-logitech-dj.ko build_dir/Boot-CM10.1/realsystem/lib/modules

cd ramdisk/initramfs
find . | cpio -o -H newc | gzip > ../../build_dir/boot.img-ramdisk.gz

cd ../..

#cp arch/arm/boot/zImage build_dir/Boot-CM10.1

cd build_dir/Boot-CM10.1

../../../packaging/mkbootimg.new --kernel ../../arch/arm/boot/zImage --ramdisk ../boot.img-ramdisk.gz --ramdisk_offset 0x01300000 --base 0x80200000 --pagesize 2048 --board MSM8960 --cmdline "androidboot.hardware=qcom user_debug=31 zcache" --output boot.img 

zip -r ~/Google\ Drive/I747\ Kernels/CM10.1/iX-CM10.1-I747-Kernel-"$timestamp".zip *

echo $timestamp

cd ..

git tag -a $timestamp -m buildtag
