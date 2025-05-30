#!/bin/bash
echo "block_size(MB),speed(MB/s)"
for bs in 4K 8K 16K 32K 64K 128K 256K 512K 1M 2M 4M 8M 16M
do
    speed=$(dd if=/dev/zero of=/dev/null bs=$bs count=4096 2>&1 | grep copied | awk '{print $(NF-1)}')
    echo "$bs,$speed"
done