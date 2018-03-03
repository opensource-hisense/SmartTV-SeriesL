#!/bin/sh

srctree=${MAKETOP}/usr/src/linux


install -c -m 0644 -D ${srctree}/build-${ARCH}/include/generated/autoconf.h ${MAKETOP}/usr/include/generated/autoconf.h
install -c -m 0644 -D ${srctree}/include/linux/autoconf.h ${MAKETOP}/usr/include/linux/autoconf.h
install -c -m 0644 -D ${srctree}/include/linux/config.h ${MAKETOP}/usr/include/linux/config.h


install -c -m 0755 -d ${MAKETOP}/usr/include/memmap
cd ${srctree}/include/memmap
echo "install include/memmap"
for i in `find . -name '*.h' -print`; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/memmap/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/memmap/$i
done

install -c -m 0755 -d ${MAKETOP}/usr/include/memmap
cd ${srctree}/include/memmap
echo "install include/memmap -> include/dtvrecdd"
for i in memmap.h memtrans.h; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/dtvrecdd/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/dtvrecdd/$i
done

install -c -m 0755 -d ${MAKETOP}/usr/include/ucd
cd ${srctree}/include/ucd
echo "install include/ucd"
for i in `find . -name '*.h' -print`; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/ucd/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/ucd/$i
done

install -c -m 0755 -d ${MAKETOP}/usr/include/ucd
cd ${srctree}/include/ucd
echo "install include/ucd -> include/dtvrecdd"
for i in ucd.h; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/dtvrecdd/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/dtvrecdd/$i
done

install -c -m 0755 -d ${MAKETOP}/usr/include/dtvrecdd
cd ${srctree}/include/dtvrecdd
echo "install include/dtvrecdd"
for i in `find . -name '*.h' -print`; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/dtvrecdd/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/dtvrecdd/$i
done

install -c -m 0755 -d ${MAKETOP}/usr/include/dtvdd
cd ${srctree}/drivers/dtvdd/include/dtvdd
echo "install drivers/dtvdd/include/dtvdd"
for i in `find . -name '*.h' -print`; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/dtvdd/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/dtvdd/$i
done

cd ${srctree}/include/pie/asm
echo "install include/pie/asm"
for i in areg.h machine.h; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/asm/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/asm/$i
done

cd ${srctree}/include/pie/asm-generic
echo "install include/pie/asm-generic -> include/asm"
for i in dma.h; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/asm/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/asm/$i
done

cd ${srctree}/include/pie/linux/i2c
echo "install include/pie/linux/i2c"
for i in i2c-mn2ws.h; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/linux/i2c/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/linux/i2c/$i
done

cd ${srctree}/include/pie/sys
echo "install include/pie/sys"
for i in ioccom.h; do
  install -c -m 0644 -D $i ${MAKETOP}/usr/include/sys/$i
  cat $i | sed '1i\
#include <generated/autoconf.h>' > ${MAKETOP}/usr/include/sys/$i
done


