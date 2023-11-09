#!/bin/sh

coreJSONDir="coreJSON"
tinyCborDir="tinycbor"
MQTTStreamingSourceDir="../../source"

UNWIND_COUNT=${UNWIND_COUNT:-10}

#If coreJSON not found, clone it
if [ ! -d "$coreJSONDir" ]; then
    git clone https://github.com/FreeRTOS/coreJSON.git --depth 1 --branch v3.2.0
fi

exec cbmc stubs/strnlen.c stubs/JSON_SearchT.c stubs/tinycbor.c proofs.c \
     $MQTTStreamingSourceDir/MQTTFileDownloader.c \
     $MQTTStreamingSourceDir/MQTTFileDownloader_cbor.c \
     $MQTTStreamingSourceDir/MQTTFileDownloader_base64.c \
     -I $MQTTStreamingSourceDir/include -I coreJSON/source/include  -I include \
     --unwindset strlen.0:36 \
     --unwindset __builtin___strncat_chk.0:192 \
     --unwindset __builtin___strncat_chk.1:205 \
     --bounds-check --pointer-check --memory-cleanup-check --div-by-zero-check \
     --signed-overflow-check --unsigned-overflow-check --pointer-overflow-check \
     --conversion-check --undefined-shift-check --enum-range-check \
     --pointer-primitive-check --drop-unused-functions --nondet-static \
     --unwinding-assertions --c99 --trace "$@" --unwind "$UNWIND_COUNT"  \
     -DUNWIND_COUNT="$UNWIND_COUNT" >&1 | tee output/latest/html/cbmcOutput.txt
