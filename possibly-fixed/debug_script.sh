#!/bin/bash
set -e

echo "[INFO] Generating test input..."
rm -f in incomp outdecomp comp.out decomp.out
base64 /dev/urandom | head -c 10000 > in

echo "[INFO] Running compressor..."
./compress 2>&1 | tee comp.out

echo "[INFO] Running decompressor..."
./decomp 2>&1 | tee decomp.out


echo "[INFO] Comparing input and decompressed output..."
if diff -u in outdecomp > /dev/null; then
    echo "[PASS] Output matches."
else
    echo "[FAIL] Output mismatch. See 'diff -u in outdecomp'."
fi

echo "[INFO] Entries inserted by compressor:"
if grep -q 'Inserted' comp.out; then
    grep 'Inserted' comp.out | cut -d"'" -f2 | sort | uniq
else
    echo "(none)"
fi

echo "[INFO] Entries inserted by decompressor:"
if grep -q 'Inserted' decomp.out; then
    grep 'Inserted' decomp.out | cut -d"'" -f2 | sort | uniq
else
    echo "(none)"
fi

