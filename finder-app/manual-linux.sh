#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_MIRROR=https://mirrors.edge.kernel.org/pub/linux/kernel/v5.x
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
KERNEL_SRC_FILE=linux-${KERNEL_VERSION:1}.tar.xz
KERNEL_FOLDER=linux-${KERNEL_VERSION:1}

build_linux() {
    cd ${OUTDIR}/${KERNEL_FOLDER}
    echo "Building version ${KERNEL_VERSION}"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    make -j4 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all
    # make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE modules
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs
}

create_rootfs() {
    mkdir -p ${OUTDIR}/rootfs
    cd ${OUTDIR}/rootfs
    mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
    mkdir -p usr/bin usr/lib usr/sbin
    mkdir -p var/log
}

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
    OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

# Use absoluth path
OUTDIR=$(realpath -s "$OUTDIR")
mkdir -p ${OUTDIR}
if [ $? != 0 ]; then
    exit 1
fi

cd "$OUTDIR"
if [ ! -f "${KERNEL_SRC_FILE}" ]; then
    #Clone only if the repository does not exist.
	# echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	# git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
	echo "DOWNLOADING LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
    curl -o ${KERNEL_SRC_FILE} https://mirrors.edge.kernel.org/pub/linux/kernel/v5.x/${KERNEL_SRC_FILE}
fi

if [ ! -d "${OUTDIR}/${KERNEL_FOLDER}" ]; then
    bsdtar -xf ${KERNEL_SRC_FILE}
fi
if [ ! -e ${OUTDIR}/${KERNEL_FOLDER}/arch/${ARCH}/boot/Image ]; then
    cd ${KERNEL_FOLDER}
    # echo "Checking out version ${KERNEL_VERSION}"
    # git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    build_linux
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/${KERNEL_FOLDER}/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
create_rootfs

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
TOOLCHAIN_BIN_DIR="$(whereis aarch64-none-linux-gnu-readelf | cut -d ":" -f 2)"
TOOLCHAIN_DIR="$(dirname ${TOOLCHAIN_BIN_DIR})/../${CROSS_COMPILE::-1}"
TOOLCHAIN_DIR="$(realpath ${TOOLCHAIN_DIR})"
cp -f ${TOOLCHAIN_DIR}/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp -f ${TOOLCHAIN_DIR}/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
cp -f ${TOOLCHAIN_DIR}/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
cp -f ${TOOLCHAIN_DIR}/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64


# TODO: Make device nodes
cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE writer

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp writer ${OUTDIR}/rootfs/home/
cp writer finder*.sh ${OUTDIR}/rootfs/home/
cp -r ../conf ${OUTDIR}/rootfs/home/
# patch finder-test.sh
sed -i -e 's/cat ..\//cat /g' ${OUTDIR}/rootfs/home/finder-test.sh
sed -i -e 's/\/bash/\/sh/g' ${OUTDIR}/rootfs/home/finder.sh

# TODO: Chown the root directory
sudo chown --recursive root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio

echo "Output: $(realpath initramfs.cpio.gz)"
echo "Done!"
