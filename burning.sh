rm app spl
gcc imx-burning.c
./a.out u-boot.bin
cp spl app
dd if=u-boot.bin of=app bs=1024 seek=3
sudo dd if=app of=/dev/sdc bs=512 seek=2
