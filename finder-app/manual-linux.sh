#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
    if [ -f "${OUTDIR}/rootfs/initramfs.cpio.gz" ]
    then
        sudo rm -f ${OUTDIR}/initramfs.cpio.gz
    fi
fi

mkdir rootfs
mkdir rootfs/bin
mkdir rootfs/dev
mkdir rootfs/etc
mkdir rootfs/lib
mkdir rootfs/lib64
mkdir rootfs/home
mkdir rootfs/home/conf
mkdir rootfs/proc
mkdir rootfs/sys
mkdir rootfs/sbin
mkdir rootfs/tmp
mkdir rootfs/usr
mkdir rootfs/usr/bin
mkdir rootfs/usr/lib
mkdir rootfs/usr/sbin
mkdir rootfs/va
mkdir rootfs/var
mkdir rootfs/var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    make distclean
    make defconfig
else
    cd busybox
fi

sudo env "PATH=$PATH" make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- CONFIG_PREFIX=$OUTDIR/rootfs install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a busybox | grep "Shared library"

SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)

cd "$OUTDIR/rootfs"

cp $SYSROOT/lib/ld-linux-aarch64.so.1 lib
cp $SYSROOT/lib64/libc.so.6 lib64
cp $SYSROOT/lib64/libm.so.6 lib64
cp $SYSROOT/lib64/libresolv.so.2 lib64

sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-gcc

sudo cp writer $OUTDIR/rootfs/home
sudo cp finder-test.sh $OUTDIR/rootfs/home
sudo cp finder.sh $OUTDIR/rootfs/home
sudo cp autorun-qemu.sh $OUTDIR/rootfs/home
sudo cp conf/username.txt $OUTDIR/rootfs/home/conf

cd $OUTDIR/rootfs
sudo chown -R root:root *

sudo find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd $OUTDIR
gzip initramfs.cpio
