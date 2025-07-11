#!/bin/bash

set -e

echo "[INFO] Generating test input..."
rm -f in incomp outdecomp comp.out decomp.out
base64 /dev/urandom | head -c 1024 > in

echo "[INFO] Running compressor..."
./compress < in 2> comp.out

echo "[INFO] Running decompressor..."
./decomp < incomp 2> decomp.out

echo "[INFO] Comparing input and decompressed output..."
if diff -q in outdecomp > /dev/null; then
    echo "[PASS] Output matches input."
else
    echo "[FAIL] Output mismatch. See 'diff -u in outdecomp'."
fi

echo
echo "[INFO] Entries inserted by compressor:"
grep "Inserted" comp.out || echo "(none)"

echo
echo "[INFO] Entries inserted by decompressor:"
grep "Inserted" decomp.out || echo "(none)"
