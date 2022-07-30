#!/bin/bash

list=$(binwalk prebuilt/cupid.dtb | grep "Flattened device tree" | awk '{print $1 "-" $7}')

i=0
rm -rf dtb
mkdir dtb

name="cupid"
for line in ${list}; do
	offset=$(echo ${line} | sed "s/-/ /g" | awk '{print $1}')
	length=$(echo ${line} | sed "s/-/ /g" | awk '{print $2}')
	echo offset: $offset length: $length
	dd skip=${offset} count=${length} ibs=1 if=prebuilt/${name}.dtb of=dtb/${name}.dtb.${i}
	/home/arian/android/lineage-19.1/prebuilts/misc/linux-x86/dtc/dtc -I dtb -O dts dtb/${name}.dtb.${i} -o dtb/${name}.dts.${i}
	let "i=i+1"
done
