#!/bin/sh

################################################################################
#
# Memory Writer
#
################################################################################

mw_kernel="memory_writer"
compute_kernel="compute_0"

# Compute Units (--connectivity.nk)
generate_nk() {
    local nk="--connectivity.nk=${mw_kernel}:${1}:"
    local idx=0
    while [ $idx -lt $((${1} - 1)) ]
    do
        nk="${nk}${mw_kernel}_${idx},"
        idx=$((idx + 1))
    done
    nk="${nk}${mw_kernel}_${idx}"
    echo $nk
}

# Axis Streams (--connectivity.sc)
generate_sc() {
    local sc=""
    local idx=0

    if [ $1 -eq 1 ]
    then
        sc="--connectivity.sc=${compute_kernel}.out_data:${mw_kernel}_0.in_data:16"
        sc="${sc} --connectivity.sc=${compute_kernel}.out_e_data:${mw_kernel}_0.in_e_data:16"
        echo $sc
        return
    fi

    while [ $idx -lt $1 ]
    do
        sc="${sc} --connectivity.sc=${compute_kernel}.out_data_${idx}:${mw_kernel}_${idx}.in_data:16"
        sc="${sc} --connectivity.sc=${compute_kernel}.out_e_data_${idx}:${mw_kernel}_${idx}.in_e_data:16"
        idx=$((idx + 1))
    done
    echo $sc
}

# HBM Banks (--connectivity.sp)
generate_sp() {
    local sp=""
    local idx=0
    while [ $idx -lt $1 ]
    do
        sp="${sp} --connectivity.sp=${mw_kernel}_${idx}.out:HBM[$((31 - ${1} + ${idx}))]"
        sp="${sp} --connectivity.sp=${mw_kernel}_${idx}.items_written:HBM[31]"
        sp="${sp} --connectivity.sp=${mw_kernel}_${idx}.eos:HBM[31]"
        idx=$((idx + 1))
    done
    echo $sp
}
