
wget -c https://www.kernel.org/pub/linux/kernel/v2.6/linux-2.6.22.tar.xz
cd linux-2.6.22
#patch -p1 < ../linux-2.6.22_9260.patch
zcat ../linux-2.6.22_9260.patch | patch -p1
make uImage
