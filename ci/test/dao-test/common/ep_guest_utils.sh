#!/bin/bash
# SPDX-License-Identifier: Marvell-MIT
# Copyright (c) 2024 Marvell.


EP_GUEST_DIR="/root/hostshare"
source "$EP_GUEST_DIR/testpmd.sh"

function get_avail_virtio_netdev()
{
        local net_bdf

        net_bdf=$(lspci -Dd ::0200 | awk '{print $1}')
        for dev in $net_bdf; do
                virtio_dir=$(echo /sys/bus/pci/devices/$dev/virtio*)
                if [[ -d $virtio_dir ]]; then
                        if [[ -d $virtio_dir/net ]]; then
                                echo $dev
                                break
                        fi
                fi
        done
}

function ep_guest_setup()
{
	echo "Setting up hugepages on guest"
	# Check for hugepages
	if mount | grep hugetlbfs | grep none; then
		echo "Hugepages already setup"
	else
		mkdir /dev/huge
		mount -t hugetlbfs none /dev/huge
	fi
	echo 512 > /proc/sys/vm/nr_hugepages
	dhclient &
	cd /home
	modprobe vfio-pci
	echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode
}

function ep_guest_start_ping_test()
{
	local pfx=$1
	local src_addr=$2
	local dst_addr=$3
	local count=$4
	local ping_out
	local pkt_sizes=(64 1000 1500)
	local pkt_size

        for pkt_size in "${pkt_sizes[@]}"
        do
                ping_out=$(ping -c $count -s $pkt_size -i 0.2 \
                                -I $src_addr $dst_addr || true)
                if [[ -n $(echo $ping_out | grep ", 0% packet loss,") ]]; then
                        echo "$pkt_size packet size ping test SUCCESS" \
				>> /root/hostshare/netdev.ping_pass.out.$pfx
                else
                        echo "$pkt_size packet size ping FAILED" \
				 >> /root/hostshare/netdev.ping_fail.out.$pfx
                        echo "stopping test execution" \
				>> /root/hostshare/netdev.ping_fail.out.$pfx
                        break
                fi
        done
}

function ep_guest_netdev_config()
{
        local net_bdf
        local ip_addr=$2
        local net_name
        local k=1

        net_bdf=$(lspci -Dd ::0200 | awk '{print $1}')
        for dev in $net_bdf; do
                virtio_dir=$(echo /sys/bus/pci/devices/$dev/virtio*)
                if [[ -d $virtio_dir ]]; then
                        if [[ -d $virtio_dir/net ]]; then
                                net_name=$(basename $(readlink -f $virtio_dir/net/*))
                                ip link set dev $net_name up
                                ip addr add $ip_addr dev $net_name
                                k=0
                                break
                        fi
                fi
        done
        return $k
}

function ep_guest_testpmd_launch()
{
	local pfx=$1
	local args=${@:2}
	local eal_args
	local app_args=""
	local dev

	dev=$(get_avail_virtio_netdev)
	echo "Binding $dev to vfio-pci driver"
	/home/usertools/dpdk-devbind.py -b vfio-pci $dev

	for a in $args; do
		if [[ $a == "--" ]]; then
			eal_args=$app_args
			app_args=""
			continue
		fi
		app_args+=" $a"
	done

	echo "Launching testpmd on Guest"
	testpmd_launch $pfx "$eal_args -a $dev" "$app_args"
	echo "Launched testpmd on Guest"
}

function ep_guest_testpmd_start()
{
	local pfx=$1

	echo "Starting Traffic on Guest"
	testpmd_cmd $pfx start tx_first 32
	echo "Started Traffic on Guest"
}

function ep_guest_testpmd_stop()
{
	local pfx=$1

	echo "Stopping testpmd on Guest"
	testpmd_quit $pfx
	testpmd_cleanup $pfx
	echo "Stopped testpmd on Guest"
}

function ep_guest_testpmd_pps()
{
	local pfx=$1
	local wait_time_sec=10

	while [[ wait_time_sec -ne 0 ]]; do
		local rx_pps=$(testpmd_pps $pfx 0)

		if [[ rx_pps -eq 0 ]]; then
			echo "Low PPS for ${pfx} ($rx_pps == 0)"
		else
			echo $rx_pps > /root/hostshare/testpmd.pps.$pfx
			return 0
		fi

		sleep 1
		wait_time_sec=$((wait_time_sec - 1))
	done
}

# If this script is directly invoked from the shell execute the
# op specified
if [[ ${BASH_SOURCE[0]} == ${0} ]]; then
	OP=$1
	ARGS=${@:2}
	if [[ $(type -t ep_guest_$OP) == function ]]; then
		ep_guest_$OP $ARGS
	else
		$OP $ARGS
	fi
fi
