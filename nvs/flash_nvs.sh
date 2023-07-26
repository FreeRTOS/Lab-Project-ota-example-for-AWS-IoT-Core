#!/bin/sh
cd "$(dirname -- "$0")" || exit 1
nvs_partition_gen.py generate factory_nvs.csv factory_nvs.bin 0x6000
esptool.py "$@" write_flash 0x10000 factory_nvs.bin
