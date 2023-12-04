#!/bin/sh

################################################################################
#
# Memory Reader
#
################################################################################

mr_kernel="memory_reader"
compute_kernel="compute_0"

# Compute Units (--connectivity.nk)
generate_nk() {
    local nk="--connectivity.nk=${mr_kernel}:${1}:"
    local idx=0
    while [ $idx -lt $((${1} - 1)) ]
    do
        nk="${nk}${mr_kernel}_${idx},"
        idx=$((idx + 1))
    done
    nk="${nk}${mr_kernel}_${idx}"
    echo $nk
}

# Axis Streams (--connectivity.sc)
generate_sc() {
    local sc=""
    local idx=0

    if [ $1 -eq 1 ]
    then
        sc="--connectivity.sc=${mr_kernel}_0.out_data:${compute_kernel}.in_data:16"
        sc="${sc} --connectivity.sc=${mr_kernel}_0.out_e_data:${compute_kernel}.in_e_data:16"
        echo $sc
        return
    fi

    while [ $idx -lt $1 ]
    do
        sc="${sc} --connectivity.sc=${mr_kernel}_${idx}.out_data:${compute_kernel}.in_data_${idx}:16"
        sc="${sc} --connectivity.sc=${mr_kernel}_${idx}.out_e_data:${compute_kernel}.in_e_data_${idx}:16"
        idx=$((idx + 1))
    done
    echo $sc
}

# HBM banks (--connectivity.sp)
generate_sp() {
    local sp=""
    local idx=0
    while [ $idx -lt $1 ]
    do
        sp="${sp} --connectivity.sp=${mr_kernel}_${idx}.in:HBM[${idx}]"
        idx=$((idx + 1))
    done
    echo $sp
}

# HOST MEMORY (--connectivity.sp)
generate_sp_host() {
    local sp=""
    local idx=0
    while [ $idx -lt $1 ]
    do
        sp="${sp} --connectivity.sp=${mr_kernel}_${idx}.in:HOST[0]"
        idx=$((idx + 1))
    done
    echo $sp
}