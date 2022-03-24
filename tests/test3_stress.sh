#!/bin/bash
while true
do
   # choosing a random directory between 1 and 10
	printf -v i '%(%s)T' -1
	build/client -f LSOfiletorage.sk -w stubs$((1 + $i % 10))
done