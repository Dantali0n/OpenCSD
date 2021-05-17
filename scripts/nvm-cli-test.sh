#!/bin/bash

sudo nvme zns reset-zone -a /dev/nvme0n1 > /dev/null 2>&1
test_result=$?
while ((test_result != 0))
  do sudo nvme zns reset-zone -a /dev/nvme0n1 > /dev/null 2>&1
  test_result=$?
done

for i in {1..512}
do
  sudo nvme zns zone-append -s 0 -z 524288 -d integers.dat /dev/nvme0n1 > /dev/null 2>&1
  test_result=$?
  while ((test_result != 0))
    do sudo nvme zns zone-append -s 0 -z 524288 -d integers.dat /dev/nvme0n1 > /dev/null 2>&1
    test_result=$?
  done
done

for i in {0..511}
do
  ((y=i*128))
  sudo nvme read -s "${y}" -c 127 -z 524288 -d data_"${i}".dat /dev/nvme0n1 > /dev/null 2>&1
  test_result=$?
  while ((test_result != 0))
    do sudo nvme read -s "${y}" -c 127 -z 524288 -d data_"${i}".dat /dev/nvme0n1 > /dev/null 2>&1
    test_result=$?
  done
done

for i in {0..511}
do
  cat data_"${i}".dat >> data.dat
done

../playground/play-filter