#rm -rf in incomp outdecomp
#base64 /dev/urandom | head -c $1 > in
./compress 
./decomp
diff in outdecomp

