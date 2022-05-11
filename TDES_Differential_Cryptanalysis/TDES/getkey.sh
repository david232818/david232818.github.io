#!/bin/bash

# key information
key9_15_l=(0x32 0x33 0x3a 0x3b 0x24 0x25 0x2c 0x2d)

# plaintext
p="7baa"

# expected ciphertext
c="d2cf"
c_="c5c9"

for i in {0..7}
do
	key9_15=${key9_15_l[$i]}
	for key0_8 in {0..511}
	do
		key=$(((key0_8 << 7) | key9_15))

		cmd=`printf "./tdes %04x %s" "$key" "$p"`
		result=`$cmd | grep "cipher" | cut -d "=" -f 2`
		if [ $result == $c ]
		then
			printf "key: %04x\n" "$key"
			echo "cipher: " $result
			break
		fi
	done
done
