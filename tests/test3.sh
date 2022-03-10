#!/bin/bash
while true
do
   # choosing a random folder between stub1 and stub10
	printf -v i $[ ( $RANDOM % 10 )  + 1 ]
	build/client -f LSOfilestorage.sk -w stubs${i}
done
