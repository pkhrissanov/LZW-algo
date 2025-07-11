rm -rf in incomp outdecomp
base64 /dev/urandom | head -c $1 > in
./compress |tee comp.out
./decomp|tee decomp.out
diff in outdecomp

