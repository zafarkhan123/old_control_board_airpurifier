#!/bin/bash

FW_FILE="./FwPackageCheckValue.txt"

if [ -e "${FW_FILE}" ]
then
   rm -f "${FW_FILE}"
fi

for k in *.bin **/*.bin
do
   echo -n "${k},"
   printf "%d\\n" "0x$(crc32 ${k})"
done > "${FW_FILE}"
