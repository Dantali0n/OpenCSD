#!/bin/bash

# A small script to solve some dependency hell caused by SPDK

for i in *.a
do
	echo "$i"
	nm $i | grep $1
done

for i in *.so
do
	echo "$i"
	nm -D $i | grep $1
done