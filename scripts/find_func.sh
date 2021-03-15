#!/bin/bash

# A small script to find which static or shared library object provides a certain
# function

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