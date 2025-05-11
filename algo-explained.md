# LZW-algo
lempel ziff welch compression algorithm, written in C. 

## What this code meant to do 
- use the algo to compress data with nothing lost, aka lossless
- compresses duplicate data into one "bit"

## how does it work
- takes an input string into the stream
- codes 0 to 255 represent the singular bytes of the intial data
- codes 256 to 4096 represent duplicates of the intial data
- ### algorithm itself
  1. create a dictionary with all single character (ascii code)
  2. create the empty string W
  3. create the emptry char C
  4. going through each charcter C,  combine W and C in WC.
       a. if WC is in the dictionary, set W to WC
       b. if WC is NOT in the dictionary, find the code that corresponds to W, set W to the code,
           add WC to the next available code in the dictionary,
           increase code index by 1,
           set new W to C
5. after finishing step 4 fully, if W is not empty, output W as code.
6. return list of codes that have been outputed
